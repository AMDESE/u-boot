// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */
#include <asm/io.h>
#include <dm.h>
#include <ram.h>
#include <spl.h>
#include <image.h>
#include <binman_sym.h>
#include <common.h>
#include <asm/csr.h>
#include <asm/arch/platform.h>
#include <asm/arch/dp_ast2700.h>
#include <asm/arch/e2m_ast2700.h>
#include <asm/arch/pci_ast2700.h>
#include <asm/arch/sdram_ast2700.h>
#include <asm/arch/scu_ast2700.h>
#include <asm/arch/vga_ast2700.h>
#include <asm/arch/stor_ast2700.h>
#include <aspeed/fmc_hdr.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/delay.h>

binman_sym_declare(u32, dp_fw, image_pos);
binman_sym_declare(u32, dp_fw, size);
binman_sym_declare(u32, VBIOS, image_pos);
binman_sym_declare(u32, VBIOS, size);

static u32 _ast_get_e2m_addr(struct sdramc_regs *ram, u8 node)
{
	u32 val;

	// get GM's base address
	val = (ram->reserved3[0] >> (node * 16)) & 0xffff;
	// e2m memory accessing address[36:24] will be replaced as
	// map_addr[31:20]
	val = (val << 20) | ASPEED_DRAM_BASE;

	return val;
}

static int vbios_init(struct ast2700_scu0 *scu, u8 node)
{
	u32 *vbios_addr;
	u32 vbios_ofst;
	u32 vbios_size;
	u64 value;
	u32 vbios_mem_base;
	struct fdt_resource res;
	void *vbios_base;
	u32 vbios_e2m_value;
	u32 arm_dram_base = ASPEED_DRAM_BASE >> 1;

	if (BINMAN_SYMS_OK) {
		vbios_size = binman_sym(u32, VBIOS, size);
		debug("vbios size : 0x%x\n", vbios_size);

		vbios_addr = (u32 *)(binman_sym(u32, VBIOS, image_pos) - CONFIG_SPL_TEXT_BASE);
		debug("vbios addr : 0x%p\n", vbios_addr);
	} else {
		fmc_hdr_get_prebuilt(PBT_UEFI_X64_AST2700, &vbios_ofst, &vbios_size, NULL);
	}

	if (node == 0)
		value = fdt_path_offset(gd->fdt_blob, "/reserved-memory/vbios-base0");
	else
		value = fdt_path_offset(gd->fdt_blob, "/reserved-memory/vbios-base1");

	fdt_get_resource(gd->fdt_blob, value, "reg", 0, &res);
	vbios_base = (void *)res.start;
	debug("vbios%d mem : 0x%p\n", node, vbios_base);

	/* Get the controller base address */
	vbios_mem_base = (uintptr_t)(vbios_base);
	debug("vbios_mem_base : 0x%x\n", vbios_mem_base);

	/* Initial memory region and copy vbios into it */
	memset((u32 *)vbios_base, 0x0, 0x10000);
	stor_copy((u32 *)vbios_ofst, vbios_base, vbios_size);

	/* Remove riscv Dram base */
	vbios_mem_base &= ~(ASPEED_DRAM_BASE);

	/* Set VBIOS 64KB into reserved buffer */
	vbios_e2m_value = (vbios_mem_base >> 4) | 0x05 | arm_dram_base;

	debug("vbios_e2m_value : 0x%x\n", vbios_e2m_value);

	/* Set VBIOS setting into e2m */
	if (node == 0) {
		writel(vbios_e2m_value, (void *)E2M0_VBIOS_RAM);
		writel(vbios_e2m_value, &scu->pci0_misc[11]);
	} else {
		writel(vbios_e2m_value, (void *)E2M1_VBIOS_RAM);
		writel(vbios_e2m_value, &scu->pci1_misc[11]);
	}

	return 0;
}

static int pci_vga_init(struct ast2700_scu0 *scu)
{
	struct sdramc_regs *ram = (struct sdramc_regs *)DRAMC_BASE;
	u32 val, vram_size;
	u8 vram_size_cfg;
	bool is_pcie0_enable;
	bool is_pcie1_enable;
	bool is_64vram;
	u8 dac_src;
	u8 dp_src;
	u8 efuse;

	is_pcie0_enable = scu->pci0_misc[28] & BIT(0);
	is_pcie1_enable = scu->pci1_misc[28] & BIT(0);
	is_64vram = ram->gfmcfg & BIT(0);
	dac_src = scu->hwstrap1 & BIT(28);
	dp_src = scu->hwstrap1 & BIT(29);
	efuse = FIELD_GET(SCU_CPU_REVISION_ID_EFUSE, scu->chip_id1);
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

		/* load node0 vbios */
		vbios_init(scu, 0);

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

		/* load node1 vbios */
		vbios_init(scu, 1);

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
		// release vga reset
		setbits_le32(&scu->modrst2_clr, SCU_CPU_RST2_VGA);

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
		packer_cpu->REG1C.value  = 0x80f0f002;
		retimer_cpu->REG10.value = 0x00030009;
		packer_io->REG10.value   = 0x00030009;
		retimer_io->REG10.value  = 0x00230009;
		retimer_io->REG44.value  = 0x00100010;
	}

	return 0;
}

int pci_init(void)
{
	int nodeoffset;
	ofnode node;
	struct ast2700_scu0 *scu;
	u8 efuse;

	/* find the offset of compatible node */
	nodeoffset = fdt_node_offset_by_compatible(gd->fdt_blob, -1,
						   "aspeed,ast2700-scu0");
	if (nodeoffset < 0) {
		printf("%s: failed to get aspeed,ast2700-scu0\n", __func__);
		return -ENODEV;
	}

	/* get ast2700-scu0 node */
	node = offset_to_ofnode(nodeoffset);

	scu = (struct ast2700_scu0 *)ofnode_get_addr(node);
	if (IS_ERR_OR_NULL(scu)) {
		printf("%s: cannot get SYSCON address pointer\n", __func__);
		return PTR_ERR(scu);
	}

	// leave works to u-boot
	if (FIELD_GET(SCU_CPU_REVISION_ID_HW, scu->chip_id1) == 0) {
		debug("%s: Do nothing in A0\n", __func__);
		return 0;
	}

	efuse = FIELD_GET(SCU_CPU_REVISION_ID_EFUSE, scu->chip_id1);
	if (efuse == 2) {
		debug("%s: 2720 has no PCIE\n", __func__);
		return 0;
	}

	if (efuse == 0) {
		// setup preset for plda2
		writel(0x12600000, (void *)ASPEED_PLDA2_PRESET0);
		writel(0x00012600, (void *)ASPEED_PLDA2_PRESET1);

		// clk/reset for e2m
		setbits_le32(&scu->clkgate_clr, SCU_CPU_CLKGATE1_E2M1);
		mdelay(10);
		setbits_le32(&scu->modrst2_clr, SCU_CPU_RST2_E2M1);
	}

	// setup preset for plda1
	writel(0x12600000, (void *)ASPEED_PLDA1_PRESET0);
	writel(0x00012600, (void *)ASPEED_PLDA1_PRESET1);

	// clk/reset for e2m
	setbits_le32(&scu->clkgate_clr, SCU_CPU_CLKGATE1_E2M0);
	mdelay(10);
	setbits_le32(&scu->modrst2_clr, SCU_CPU_RST2_E2M0);

	pci_vga_init(scu);

	return 0;
}
