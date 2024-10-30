// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include <dm.h>
#include <asm/arch-aspeed/e2m_ast2700.h>
#include <asm/arch-aspeed/pci_ast2700.h>
#include <asm/arch-aspeed/sdram_ast2700.h>
#include <asm/arch-aspeed/vga_ast2700.h>
#include <asm/io.h>
#include <env.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/err.h>

static u32 _ast_get_e2m_addr(struct sdramc_regs *ram, u8 node)
{
	u32 val;

	// get GM's base address
	val = (ram->reserved3[0] >> (node * 16)) & 0xffff;
	// e2m memory accessing address[36:24] will be replaced as
	// map_addr[31:20]
	val = (val << 20) | (ASPEED_DRAM_BASE >> 4);

	return val;
}

static int pci_vga_init(struct ast2700_scu0 *scu)
{
	struct sdramc_regs *ram = (struct sdramc_regs *)DRAMC_BASE;
	u32 val, vram_size;
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

	vram_size_cfg = is_64vram ? 0xf : 0xe;
	vram_size = 2 << (vram_size_cfg + 10);
	debug("%s: VRAM size(%x) cfg(%x)\n", __func__, vram_size, vram_size_cfg);

	if (is_pcie0_enable) {
		// enable clk
		setbits_le32(&scu->clkgate_clr, SCU_CPU_CLKGATE1_VGA0);

		debug("pcie0 e2m addr(%x)\n", _ast_get_e2m_addr(ram, 0));
		val = _ast_get_e2m_addr(ram, 0)
		    | FIELD_PREP(SCU_CPU_PCI_MISC0C_FB_SIZE, vram_size_cfg);
		debug("pcie0 debug reg(%x)\n", val);
		writel(val, (void *)E2M0_VGA_RAM);
		writel(val, &scu->pci0_misc[3]);

		// scratch for VGA CRD0[12]: Disable P2A
		setbits_le32(&scu->vga0_scratch1[0], BIT(7));
		setbits_le32(&scu->vga0_scratch1[0], BIT(12));

		// Enable VRAM address offset: cursor, 2d
		writel(BIT(10) | BIT(27), &ram->gfm0ctl);
	}

	if (is_pcie1_enable) {
		// enable clk
		setbits_le32(&scu->clkgate_clr, SCU_CPU_CLKGATE1_VGA1);

		debug("pcie1 e2m addr(%x)\n", _ast_get_e2m_addr(ram, 1));
		val = _ast_get_e2m_addr(ram, 1)
		    | FIELD_PREP(SCU_CPU_PCI_MISC0C_FB_SIZE, vram_size_cfg);
		debug("pcie1 debug reg(%x)\n", val);
		writel(val, (void *)E2M1_VGA_RAM);
		writel(val, &scu->pci1_misc[3]);

		// scratch for VGA CRD0[12]: Disable P2A
		setbits_le32(&scu->vga1_scratch1[0], BIT(7));
		setbits_le32(&scu->vga1_scratch1[0], BIT(12));

		// Enable VRAM address offset: cursor, 2d
		writel(BIT(19) | BIT(28), &ram->gfm1ctl);
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

void pci_config(struct ast2700_scu0 *scu)
{
	u8 efuse = FIELD_GET(SCU_CPU_REVISION_ID_EFUSE, scu->chip_id1);

	if (efuse == 2) {
		debug("%s: 2720 has no PCIE\n", __func__);
		return;
	}

	if (efuse == 0) {
		// setup preset for plda2
		writel(0x12600000, (void *)ASPEED_PLDA2_PRESET0);
		writel(0x00012600, (void *)ASPEED_PLDA2_PRESET1);
	}

	// setup preset for plda1
	writel(0x12600000, (void *)ASPEED_PLDA1_PRESET0);
	writel(0x00012600, (void *)ASPEED_PLDA1_PRESET1);

	pci_vga_init(scu);
}
