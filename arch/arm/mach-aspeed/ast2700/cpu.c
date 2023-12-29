// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include <common.h>
#include <dm.h>
#include <asm/armv8/mmu.h>
#include <asm/arch-aspeed/e2m_ast2700.h>
#include <asm/arch-aspeed/sdram_ast2700.h>
#include <asm/arch-aspeed/scu_ast2700.h>
#include <asm/arch-aspeed/vga_ast2700.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <env.h>
#include <env_internal.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/err.h>

DECLARE_GLOBAL_DATA_PTR;

static struct mm_region aspeed2700_mem_map[] = {
		{
				.virt =  0x10000000UL,
				.phys =  0x10000000UL,
				.size =  0x40000000UL,
				.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
					 PTE_BLOCK_NON_SHARE |
					 PTE_BLOCK_PXN | PTE_BLOCK_UXN
		},
		{
				.virt =  0x40000000UL,
				.phys =  0x40000000UL,
				.size = 0x2C0000000UL,
				.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
					 PTE_BLOCK_NON_SHARE
		},
		{
				.virt = 0x400000000UL,
				.phys = 0x400000000UL,
				.size = 0x200000000UL,
				.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
					 PTE_BLOCK_INNER_SHARE
		},
		{
				/* List terminator */
				0,
		}
};

struct mm_region *mem_map = aspeed2700_mem_map;

u64 get_page_table_size(void)
{
	return 0x80000;
}

enum env_location env_get_location(enum env_operation op, int prio)
{
	enum env_location env_loc = ENVL_UNKNOWN;

	if (prio)
		return env_loc;

	if ((readl(ASPEED_IO_HW_STRAP1) & SCU_IO_HWSTRAP_EMMC))
		env_loc =  ENVL_MMC;
	else
		env_loc =  ENVL_SPI_FLASH;

	return env_loc;
}

static u32 _ast_get_e2m_addr(struct sdramc_regs *ram, u32 addr)
{
	u32 val;

	val = ram->mcfg >> 2 & 0x07;

	debug("%s: DRAMC val(%x)\n", __func__, val);
	return (((1 << val) - 1) << 24) | (addr >> 4);
}

static int pci_vga_init(struct ast2700_soc0_scu *scu)
{
	struct sdramc_regs *ram = (struct sdramc_regs *)DRAMC_BASE;
	u32 val, vram_size, vram_addr;
	u8 vram_size_cfg;
	bool is_pcie0_enable = scu->pci0_misc[28] & BIT(0);
	bool is_pcie1_enable = scu->pci1_misc[28] & BIT(0);
	bool is_64vram = ram->gfmcfg & BIT(0);
	u8 dac_src = scu->hwstrap1 & BIT(28);
	u8 dp_src = scu->hwstrap1 & BIT(29);
	u8 efuse = FIELD_GET(SCU_CPU_REVISION_ID_EFUSE, scu->chip_id1);

	/* Decide feature by efuse
	 *  0: 2750 has full function
	 *  1: 2700 has only 1 VGA
	 *  2: 2720 has no VGA
	 */
	if (efuse == 1) {
		is_pcie1_enable = false;
		dac_src = 0;
		dp_src = 0;
	} else if (efuse == 2) {
		debug("%s: 2720 has no VGA\n", __func__);
		return 0;
	}

	debug("%s: ENABLE 0(%d) 1(%d)\n", __func__, is_pcie0_enable, is_pcie1_enable);

	/* scratch for VGA CRAA[1:0] : 10b: 32Mbytes, 11b: 64Mbytes */
	setbits_le32(&scu->hwstrap1, BIT(11));
	if (is_64vram)
		setbits_le32(&scu->hwstrap1, BIT(10));
	else
		setbits_le32(&scu->hwstrap1_clr, BIT(10));

	vram_addr = 0x10000000;

	vram_size_cfg = is_64vram ? 0xf : 0xe;
	vram_size = 2 << (vram_size_cfg + 10);
	debug("%s: VRAM size(%x) cfg(%x)\n", __func__, vram_size, vram_size_cfg);

	if (is_pcie0_enable) {
		// enable clk
		setbits_le32(&scu->clkgate_clr, SCU_CPU_CLKGATE1_VGA0);

		vram_addr -= vram_size;
		debug("pcie0 e2m addr(%x)\n", _ast_get_e2m_addr(ram, vram_addr));
		val = _ast_get_e2m_addr(ram, vram_addr)
		    | FIELD_PREP(SCU_CPU_PCI_MISC0C_FB_SIZE, vram_size_cfg)
		    | (ASPEED_DRAM_BASE >> 4);
		debug("pcie0 debug reg(%x)\n", val);
		writel(val, (void *)E2M0_VGA_RAM);
		writel(val, &scu->pci0_misc[3]);

		// scratch for VGA CRD0[12]: Disable P2A
		setbits_le32(&scu->vga0_scratch1[0], BIT(12));

		// Enable VRAM address offset: cursor, rvas, 2d
		writel(BIT(10) | BIT(24) | BIT(27), &ram->gfm0ctl);
	}

	if (is_pcie1_enable) {
		// enable clk
		setbits_le32(&scu->clkgate_clr, SCU_CPU_CLKGATE1_VGA1);

		vram_addr -= vram_size;
		debug("pcie1 e2m addr(%x)\n", _ast_get_e2m_addr(ram, vram_addr));
		val = _ast_get_e2m_addr(ram, vram_addr)
		    | FIELD_PREP(SCU_CPU_PCI_MISC0C_FB_SIZE, vram_size_cfg)
		    | (ASPEED_DRAM_BASE >> 4);
		debug("pcie1 debug reg(%x)\n", val);
		writel(val, (void *)E2M1_VGA_RAM);
		writel(val, &scu->pci1_misc[3]);

		// scratch for VGA CRD0[12]: Disable P2A
		setbits_le32(&scu->vga1_scratch1[0], BIT(12));

		// Enable VRAM address offset: cursor, rvas, 2d
		writel(BIT(19) | BIT(25) | BIT(28), &ram->gfm1ctl);
	}

	if (is_pcie0_enable || is_pcie1_enable) {
		struct ast2700_vga_link *packer_cpu, *retimer_cpu, *packer_io,
					*retimer_io;

		// enable dac clk
		setbits_le32(&scu->clkgate_clr, SCU_CPU_CLKGATE1_DAC);

		val = scu->vga_func_ctrl;
		val &= ~(SCU_CPU_VGA_FUNC_DAC_OUTPUT
			| SCU_CPU_VGA_FUNC_DP_OUTPUT
			| SCU_CPU_VGA_FUNC_DAC_DISABLE);
		val |= FIELD_PREP(SCU_CPU_VGA_FUNC_DAC_OUTPUT, dac_src)
		     | FIELD_PREP(SCU_CPU_VGA_FUNC_DP_OUTPUT, dp_src)
		     | FIELD_PREP(SCU_CPU_VGA_FUNC_DAC_DISABLE, 0);
		writel(val, &scu->vga_func_ctrl);

		// vga link init
		packer_cpu = (struct ast2700_vga_link *)VGA_PACKER_CPU_BASE;
		retimer_cpu = (struct ast2700_vga_link *)VGA_RETIMER_CPU_BASE;
		packer_io = (struct ast2700_vga_link *)VGA_PACKER_IO_BASE;
		retimer_io = (struct ast2700_vga_link *)VGA_RETIMER_IO_BASE;

		packer_cpu->REG10.value  = 0x00030008;
		val = 0x10000000 | dac_src;
		packer_cpu->REG50.value  = val;
		packer_cpu->REG44.value  = 0x00100010;
		retimer_cpu->REG10.value = 0x00030009;
		packer_io->REG10.value   = 0x00030009;
		retimer_io->REG10.value  = 0x00230009;
		retimer_io->REG44.value  = 0x00100010;
	}

	return 0;
}

int arch_misc_init(void)
{
	int nodeoffset;
	ofnode node;

	if (IS_ENABLED(CONFIG_ARCH_MISC_INIT)) {
		if ((readl(ASPEED_IO_HW_STRAP1) & SCU_IO_HWSTRAP_EMMC))
			env_set("boot_device", "mmc");
		else
			env_set("boot_device", "spi");
	}

	/* find the offset of compatible node */
	nodeoffset = fdt_node_offset_by_compatible(gd->fdt_blob, -1,
						   "aspeed,ast2700-scu0");
	if (nodeoffset < 0) {
		printf("%s: failed to get aspeed,ast2700-scu0\n", __func__);
		return -ENODEV;
	}

	/* get ast2700-scu0 node */
	node = offset_to_ofnode(nodeoffset);

	pci_vga_init((struct ast2700_soc0_scu *)ofnode_get_addr(node));

	return 0;
}
