// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include <linux/bitfield.h>
#include <common.h>
#include <clk.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <errno.h>
#include <reset.h>
#include <regmap.h>
#include <syscon.h>
#include <fdtdec.h>
#include <asm/io.h>

#ifdef CONFIG_RISCV
#include <aspeed/fmc_hdr.h>
#include <asm/arch/platform.h>
#include <asm/arch/scu_ast2700.h>
#include <asm/arch/stor_ast2700.h>
#else
#include "ast_vbios.h"
#endif

#define VBIOS0_SCU_OFFSET	(0xa2c)
#define VBIOS1_SCU_OFFSET	(0xaac)
#define SCU_PCI_MISC70		(0xa70)
#define SCU_PCI_MISCF0		(0xaf0)
#define SCU_REVISION		(0x0)
#define SCU_REVISION_ID_EFUSE	GENMASK(15, 8)
#define EFUSE_AST2700		(0x1)
#define EFUSE_AST2720		(0x2)
#define AST2700_PCI_ID		(0x27001a03)
#define UEFI_ID_OFFSET		(0x20)
#define UEFI_NEXT_OFFSET	(0x30)
#define UEFI_NEXT_VALUE	(0x3)

#define E2M0_BASE		(0x12c21000)
#define E2M0_VBIOS_RAM  (E2M0_BASE + 0x104)
#define E2M1_BASE		(0x12c22000)
#define E2M1_VBIOS_RAM  (E2M1_BASE + 0x124)

struct aspeed_vbios_priv {
	struct regmap *scu_ctl_base;
	void *e2m0_ctl_base;
	void *e2m1_ctl_base;
	void *vbios0_base;
	void *vbios1_base;
#ifdef CONFIG_RISCV
	struct ast2700_scu0 *scu;
#endif
};

static int aspeed_vbios_probe(struct udevice *dev)
{
	struct aspeed_vbios_priv *vbios = dev_get_priv(dev);
	u32 value, efuse;
	bool is_pcie0_enable, is_pcie1_enable;
	u32 vbios_e2m_value;
	u64 vbios_mem_base;
	u32 vbios_mem_base_add = 0;
	struct fdt_resource res;
#ifdef CONFIG_RISCV
	u32 vbios_ofst;
	u32 vbios_size;
	u32 arm_dram_base = ASPEED_DRAM_BASE >> 1;
#else
	u32 vbios_x64_size;
	u32 vbios_arm_size;
#endif

#ifdef CONFIG_RISCV
	is_pcie0_enable = vbios->scu->pci0_misc[28] & BIT(0);
	is_pcie1_enable = vbios->scu->pci1_misc[28] & BIT(0);
	efuse = FIELD_GET(SCU_CPU_REVISION_ID_EFUSE, vbios->scu->chip_id1);
#else
	/* Get chip version */
	regmap_read(vbios->scu_ctl_base, SCU_REVISION, &value);
	efuse = FIELD_GET(SCU_REVISION_ID_EFUSE, value);

	/* Get PCIE status */
	regmap_read(vbios->scu_ctl_base, SCU_PCI_MISC70, &value);
	is_pcie0_enable = value & BIT(0);

	regmap_read(vbios->scu_ctl_base, SCU_PCI_MISCF0, &value);
	is_pcie1_enable = value & BIT(0);
#endif

	if (efuse == EFUSE_AST2700) {
		/* single display */
		is_pcie1_enable = 0;
	} else if (efuse == EFUSE_AST2720) {
		/* without display*/
		return -1;
	}

#ifdef CONFIG_RISCV
	fmc_hdr_get_prebuilt(PBT_UEFI_X64_AST2700, &vbios_ofst, &vbios_size, NULL);
#else
	/* get x64 and arm bios length */
	vbios_x64_size = sizeof(uefi2000);
	dev_dbg(dev, "len0: 0x%08x\n", vbios_x64_size);
	vbios_arm_size = sizeof(uefi2000_arm);
	dev_dbg(dev, "len1: 0x%08x\n", vbios_arm_size);
#endif
	/* load vbios for pcie 0 node */
	if (is_pcie0_enable) {
		/* obtain the vbios0 reserved memory */
		value = fdt_path_offset(gd->fdt_blob, "/reserved-memory/vbios-base0");
		fdt_get_resource(gd->fdt_blob, value, "reg", 0, &res);
		vbios->vbios0_base = (void *)res.start;
		if (IS_ERR_OR_NULL(vbios->vbios0_base)) {
			dev_err(dev, "can't obtain vbios-base0 reserved\n");
			return PTR_ERR(vbios->vbios0_base);
		}

		/* Get the controller base address */
		vbios_mem_base = (uintptr_t)(vbios->vbios0_base);

		/* Initial memory region and copy vbios into it */
		memset((u32 *)vbios->vbios0_base, 0x0, 0x10000);

#ifdef CONFIG_RISCV
		stor_copy((u32 *)vbios_ofst, (u32 *)vbios->vbios0_base, vbios_size);

		/* Remove riscv Dram base */
		vbios_mem_base &= ~(ASPEED_DRAM_BASE);

		/* Set VBIOS 64KB into reserved buffer */
		vbios_e2m_value = (vbios_mem_base >> 4) | 0x05 | arm_dram_base;

		debug("vbios_e2m_value : 0x%x\n", vbios_e2m_value);

		writel(vbios_e2m_value, (void *)E2M0_VBIOS_RAM);
		writel(vbios_e2m_value, &vbios->scu->pci0_misc[11]);
#else
		dev_dbg(dev, "base0: 0x%p\n", vbios->vbios0_base);
		memcpy((u32 *)vbios->vbios0_base, uefi2000, vbios_x64_size);
		*(u32 *)(vbios->vbios0_base + vbios_mem_base_add + UEFI_NEXT_OFFSET) = UEFI_NEXT_VALUE;
		dev_dbg(dev, "next0: 0x%x\n", *(u32 *)(vbios->vbios0_base + vbios_mem_base_add + UEFI_NEXT_OFFSET));
		vbios_mem_base_add += vbios_x64_size;

		dev_dbg(dev, "base1: 0x%p\n", (vbios->vbios0_base + vbios_mem_base_add));
		memcpy((u32 *)(vbios->vbios0_base + vbios_mem_base_add), uefi2000, vbios_x64_size);
		*(u32 *)(vbios->vbios0_base + vbios_mem_base_add + UEFI_ID_OFFSET) = AST2700_PCI_ID;
		dev_dbg(dev, "id1_c: 0x%x\n", *(u32 *)(vbios->vbios0_base + vbios_mem_base_add + UEFI_ID_OFFSET));
		*(u32 *)(vbios->vbios0_base + vbios_mem_base_add + UEFI_NEXT_OFFSET) = UEFI_NEXT_VALUE;
		dev_dbg(dev, "next1: 0x%x\n", *(u32 *)(vbios->vbios0_base + vbios_mem_base_add + UEFI_NEXT_OFFSET));
		vbios_mem_base_add += vbios_x64_size;

		dev_dbg(dev, "base2: 0x%p\n", (vbios->vbios0_base + vbios_mem_base_add));
		memcpy((u32 *)(vbios->vbios0_base + vbios_mem_base_add), uefi2000_arm, vbios_arm_size);
		*(u32 *)(vbios->vbios0_base + vbios_mem_base_add + UEFI_NEXT_OFFSET) = UEFI_NEXT_VALUE;
		dev_dbg(dev, "next2_c: 0x%x\n", *(u32 *)(vbios->vbios0_base + vbios_mem_base_add + UEFI_NEXT_OFFSET));
		vbios_mem_base_add += vbios_arm_size;

		dev_dbg(dev, "base3: 0x%p\n", (vbios->vbios0_base + vbios_mem_base_add));
		memcpy((u32 *)(vbios->vbios0_base + vbios_mem_base_add), uefi2000_arm, vbios_arm_size);
		*(u32 *)(vbios->vbios0_base + vbios_mem_base_add + UEFI_ID_OFFSET) = AST2700_PCI_ID;
		dev_dbg(dev, "id3_c: 0x%x\n", *(u32 *)(vbios->vbios0_base + vbios_mem_base_add + UEFI_ID_OFFSET));

		invalidate_dcache_range(vbios_mem_base, vbios_mem_base
		+ 0x10000);

		/* Set VBIOS 64KB into reserved buffer */
		vbios_e2m_value = (vbios_mem_base >> 4) | 0x05;

		/* Set VBIOS setting into e2m */
		writel(vbios_e2m_value, vbios->e2m0_ctl_base + 0x4);

		/* Set VBIOS setting into scu */
		regmap_write(vbios->scu_ctl_base, VBIOS0_SCU_OFFSET, vbios_e2m_value);
#endif
	} else {
#ifdef CONFIG_RISCV
		writel(0x0, (void *)E2M0_VBIOS_RAM);
		writel(0x0, &vbios->scu->pci0_misc[11]);
#else
		/* clear VBIOS setting into e2m */
		writel(0, vbios->e2m0_ctl_base + 0x4);

		/* clear VBIOS setting into scu */
		regmap_write(vbios->scu_ctl_base, VBIOS0_SCU_OFFSET, 0);
#endif
	}

	/* load vbios for pcie 1 node */
	if (is_pcie1_enable) {
		/* obtain the vbios0 reserved memory */
		value = fdt_path_offset(gd->fdt_blob, "/reserved-memory/vbios-base1");
		fdt_get_resource(gd->fdt_blob, value, "reg", 0, &res);
		vbios->vbios1_base = (void *)res.start;
		if (IS_ERR_OR_NULL(vbios->vbios1_base)) {
			dev_err(dev, "can't obtain vbios-base1 reserved\n");
			return PTR_ERR(vbios->vbios1_base);
		}

		dev_dbg(dev, "vbios1\n");
		vbios_mem_base_add = 0;

		/* Get the controller base address */
		vbios_mem_base = (uintptr_t)(vbios->vbios1_base);

		/* Initial memory region and copy vbios into it */
		memset((u32 *)vbios->vbios1_base, 0x0, 0x10000);
#ifdef CONFIG_RISCV
		stor_copy((u32 *)vbios_ofst, vbios->vbios1_base, vbios_size);

		/* Remove riscv Dram base */
		vbios_mem_base &= ~(ASPEED_DRAM_BASE);

		/* Set VBIOS 64KB into reserved buffer */
		vbios_e2m_value = (vbios_mem_base >> 4) | 0x05 | arm_dram_base;

		debug("vbios_e2m_value : 0x%x\n", vbios_e2m_value);

		writel(vbios_e2m_value, (void *)E2M1_VBIOS_RAM);
		writel(vbios_e2m_value, &vbios->scu->pci1_misc[11]);
#else
		/* Initial memory region and copy vbios into it */
		dev_dbg(dev, "base0: 0x%p\n", vbios->vbios1_base);
		memcpy((u32 *)vbios->vbios1_base, uefi2000, vbios_x64_size);
		*(u32 *)(vbios->vbios1_base + vbios_mem_base_add + UEFI_NEXT_OFFSET) = UEFI_NEXT_VALUE;
		dev_dbg(dev, "next0_c: 0x%x\n", *(u32 *)(vbios->vbios1_base + vbios_mem_base_add + UEFI_NEXT_OFFSET));
		vbios_mem_base_add += vbios_x64_size;

		dev_dbg(dev, "base1: 0x%p\n", (vbios->vbios1_base + vbios_mem_base_add));
		memcpy((u32 *)(vbios->vbios1_base + vbios_mem_base_add), uefi2000, vbios_x64_size);
		*(u32 *)(vbios->vbios1_base + vbios_mem_base_add + UEFI_ID_OFFSET) = AST2700_PCI_ID;
		dev_dbg(dev, "id1_c: 0x%x\n", *(u32 *)(vbios->vbios1_base + vbios_mem_base_add + UEFI_ID_OFFSET));
		*(u32 *)(vbios->vbios1_base + vbios_mem_base_add + UEFI_NEXT_OFFSET) = UEFI_NEXT_VALUE;
		dev_dbg(dev, "next1: 0x%x\n", *(u32 *)(vbios->vbios1_base + vbios_mem_base_add + UEFI_NEXT_OFFSET));
		vbios_mem_base_add += vbios_x64_size;

		dev_dbg(dev, "base2: 0x%p\n", (vbios->vbios1_base + vbios_mem_base_add));
		memcpy((u32 *)(vbios->vbios1_base + vbios_mem_base_add), uefi2000_arm, vbios_arm_size);
		*(u32 *)(vbios->vbios1_base + vbios_mem_base_add + UEFI_NEXT_OFFSET) = UEFI_NEXT_VALUE;
		dev_dbg(dev, "next2_c: 0x%x\n", *(u32 *)(vbios->vbios1_base + vbios_mem_base_add + UEFI_NEXT_OFFSET));
		vbios_mem_base_add += vbios_arm_size;

		dev_dbg(dev, "base3: 0x%p\n", (vbios->vbios1_base + vbios_mem_base_add));
		memcpy((u32 *)(vbios->vbios1_base + vbios_mem_base_add), uefi2000_arm, vbios_arm_size);
		*(u32 *)(vbios->vbios1_base + vbios_mem_base_add + UEFI_ID_OFFSET) = AST2700_PCI_ID;
		dev_dbg(dev, "id3_c: 0x%x\n", *(u32 *)(vbios->vbios1_base + vbios_mem_base_add + UEFI_ID_OFFSET));

		invalidate_dcache_range(vbios_mem_base, vbios_mem_base
		+ 0x10000);

		/* Set VBIOS 64KB into reserved buffer */
		vbios_e2m_value = (vbios_mem_base >> 4) | 0x05;

		/* Set VBIOS setting into e2m */
		writel(vbios_e2m_value, vbios->e2m1_ctl_base + 0x24);
		/* Set VBIOS setting into scu */
		regmap_write(vbios->scu_ctl_base, VBIOS1_SCU_OFFSET, vbios_e2m_value);
#endif
	} else {
#ifdef CONFIG_RISCV
		writel(0x0, (void *)E2M1_VBIOS_RAM);
		writel(0x0, &vbios->scu->pci1_misc[11]);
#else
		/* clear VBIOS1 setting into e2m */
		writel(0, vbios->e2m1_ctl_base + 0x24);

		/* clear VBIOS1 setting into scu */
		regmap_write(vbios->scu_ctl_base, VBIOS1_SCU_OFFSET, 0);
#endif
	}

	return 0;
}

static int aspeed_vbios_of_to_plat(struct udevice *dev)
{
	struct aspeed_vbios_priv *vbios = dev_get_priv(dev);

#ifdef CONFIG_RISCV
	vbios->scu = dev_read_addr_index_ptr(dev, 0);
	if (!vbios->scu) {
		dev_err(dev, "get scu reg failed\n");
		return -ENOMEM;
	}
#else
	vbios->e2m0_ctl_base = (void *)devfdt_get_addr_index(dev, 0);
	if (IS_ERR(vbios->e2m0_ctl_base)) {
		dev_err(dev, "can't allocate e2m0_ctl\n");
		return PTR_ERR(vbios->e2m0_ctl_base);
	}

	vbios->e2m1_ctl_base = (void *)devfdt_get_addr_index(dev, 1);
	if (IS_ERR(vbios->e2m1_ctl_base)) {
		dev_err(dev, "can't allocate e2m1_ctl\n");
		return PTR_ERR(vbios->e2m1_ctl_base);
	}

	vbios->scu_ctl_base = syscon_regmap_lookup_by_phandle(dev, "aspeed,scu");
	if (IS_ERR(vbios->scu_ctl_base)) {
		dev_err(dev, "can't allocate scu_ctl\n");
		return PTR_ERR(vbios->scu_ctl_base);
	}
#endif
	return 0;
}

static const struct udevice_id aspeed_vbios_ids[] = {
	{ .compatible = "aspeed,ast2700-vbios" },
	{ }
};

U_BOOT_DRIVER(aspeed_vbios) = {
	.name		= "aspeed_vbios",
	.id			= UCLASS_MISC,
	.of_match	= aspeed_vbios_ids,
	.probe		= aspeed_vbios_probe,
	.of_to_plat = aspeed_vbios_of_to_plat,
	.priv_auto  = sizeof(struct aspeed_vbios_priv),
};
