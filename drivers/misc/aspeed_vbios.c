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
#include "ast_vbios.h"

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

struct aspeed_vbios_priv {
	struct regmap *scu_ctl_base;
	void *e2m0_ctl_base;
	void *e2m1_ctl_base;
	void *vbios0_base;
	void *vbios1_base;
};

static int aspeed_vbios_probe(struct udevice *dev)
{
	struct aspeed_vbios_priv *vbios = dev_get_priv(dev);
	u32 value, efuse;
	bool is_pcie0_enable, is_pcie1_enable;
	u32 vbios_e2m_value;
	u64 vbios_mem_base;
	u32 vbios_mem_base_add = 0;
	u32 vbios_x64_size;
	u32 vbios_arm_size;
	struct fdt_resource res;

	/* Get chip version */
	regmap_read(vbios->scu_ctl_base, SCU_REVISION, &value);
	efuse = FIELD_GET(SCU_REVISION_ID_EFUSE, value);

	/* Get PCIE status */
	regmap_read(vbios->scu_ctl_base, SCU_PCI_MISC70, &value);
	is_pcie0_enable = value & BIT(0);

	regmap_read(vbios->scu_ctl_base, SCU_PCI_MISCF0, &value);
	is_pcie1_enable = value & BIT(0);

	if (efuse == EFUSE_AST2700) {
		/* single display */
		is_pcie1_enable = 0;
	} else if (efuse == EFUSE_AST2720) {
		/* without display*/
		return -1;
	}

	/* get x64 and arm bios length */
	vbios_x64_size = sizeof(uefi2000);
	dev_dbg(dev, "len0: 0x%08x\n", vbios_x64_size);
	vbios_arm_size = sizeof(uefi2000_arm);
	dev_dbg(dev, "len1: 0x%08x\n", vbios_arm_size);

	/* load vbios for pcie 0 node */
	if (is_pcie0_enable) {
		/* obtain the vbios0 reserved memory */
		value = fdt_path_offset(gd->fdt_blob, "/reserved-memory/pcie_vbios0");
		fdt_get_resource(gd->fdt_blob, value, "reg", 0, &res);
		vbios->vbios0_base = (void *)res.start;
		if (IS_ERR(vbios->vbios0_base)) {
			dev_err(dev, "can't obtain pcie_vbios0 reserved\n");
			return PTR_ERR(vbios->vbios0_base);
		}

		dev_dbg(dev, "vbios0\n");

		/* Get the controller base address */
		vbios_mem_base = (uintptr_t)(vbios->vbios0_base);

		/* Initial memory region and copy vbios into it */
		memset((u32 *)vbios->vbios0_base, 0x0, 0x10000);

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
	} else {
		/* clear VBIOS setting into e2m */
		writel(0, vbios->e2m0_ctl_base + 0x4);

		/* clear VBIOS setting into scu */
		regmap_write(vbios->scu_ctl_base, VBIOS0_SCU_OFFSET, 0);
	}

	/* load vbios for pcie 1 node */
	if (is_pcie1_enable) {
		/* obtain the vbios0 reserved memory */
		value = fdt_path_offset(gd->fdt_blob, "/reserved-memory/pcie_vbios1");
		fdt_get_resource(gd->fdt_blob, value, "reg", 0, &res);
		vbios->vbios1_base = (void *)res.start;
		if (IS_ERR(vbios->vbios1_base)) {
			dev_err(dev, "can't obtain pcie_vbios1 reserved\n");
			return PTR_ERR(vbios->vbios1_base);
		}

		dev_dbg(dev, "vbios1\n");
		vbios_mem_base_add = 0;

		/* Get the controller base address */
		vbios_mem_base = (uintptr_t)(vbios->vbios1_base);

		/* Initial memory region and copy vbios into it */
		memset((u32 *)vbios->vbios1_base, 0x0, 0x10000);

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
	} else {
		/* clear VBIOS1 setting into e2m */
		writel(0, vbios->e2m1_ctl_base + 0x24);

		/* clear VBIOS1 setting into scu */
		regmap_write(vbios->scu_ctl_base, VBIOS1_SCU_OFFSET, 0);
	}

	return 0;
}

static int aspeed_vbios_of_to_plat(struct udevice *dev)
{
	struct aspeed_vbios_priv *vbios = dev_get_priv(dev);
	/* Get the controller base address */

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
