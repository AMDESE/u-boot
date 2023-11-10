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
#include <asm/arch-aspeed/platform.h>
#include <asm/arch-aspeed/clk_ast2700.h>
#include <asm/arch-aspeed/dp_ast2700.h>
#include <asm/arch-aspeed/e2m_ast2700.h>
#include <asm/arch-aspeed/scu_ast2700.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/delay.h>

binman_sym_declare(u32, dp_fw, image_pos);
binman_sym_declare(u32, dp_fw, size);

static void *fdt_get_syscon_addr_ptr(struct udevice *dev)
{
	ofnode node = dev_ofnode(dev), parent;
	fdt_addr_t addr;

	if (!ofnode_valid(node)) {
		printf("%s: node invalid\n", __func__);
		return NULL;
	}

	parent = ofnode_get_parent(node);
	addr = ofnode_get_addr(parent);
	if (addr == FDT_ADDR_T_NONE) {
		printf("%s: node addr none\n", __func__);
		return NULL;
	}

	debug("scu reg: %x\n", addr);
	return (void *)(uintptr_t)addr;
}

// To support 64bit address calculation for e2m
static u32 _ast_get_e2m_addr(u32 addr)
{
	u32 val;

	val = readl((void *)0x12c00010) >> 2 & 0x07;

	debug("%s: DRAMC val(%x)\n", __func__, val);
	return (((1 << val) - 1) << 24) | (addr >> 4);
}

static int dp_init(struct ast2700_soc0_scu *scu, struct ast2700_soc0_clk *clk)
{
	u32 *fw_addr;
	u32 fw_size;
	u32 mcu_ctrl, val;
	void *scu_offset;
	bool is_mcu_stop = false;

	fw_size = binman_sym(u32, dp_fw, size);
	if (fw_size == BINMAN_SYM_MISSING) {
		printf("%s: Can't get dp-firmware\n", __func__);
		return -1;
	}
	// TODO: SPL boot only now
	fw_addr = (u32 *)(binman_sym(u32, dp_fw, image_pos) - CONFIG_SPL_TEXT_BASE + 0x20000000);

	val = readl(&scu->vga_func_ctrl);
	scu_offset = (((val >> 8) & 0x3) == 1)
		   ? &scu->vga1_scratch1[0] : &scu->vga0_scratch1[0];
	val = readl(scu_offset);
	is_mcu_stop = ((val & BIT(13)) == 0);

	/* reset for DPTX and DPMCU if MCU isn't running */
	if (is_mcu_stop) {
		setbits_le32(&scu->modrst1_ctrl, SCU_CPU_RST_DP);
		setbits_le32(&scu->modrst1_ctrl, SCU_CPU_RST_DPMCU);
		udelay(100);

		// enable clk
		setbits_le32(&clk->clkgate_clr, SCU_CPU_CLKGATE1_DP);
		mdelay(10);

		setbits_le32(&scu->modrst1_clr, SCU_CPU_RST_DP);
		setbits_le32(&scu->modrst1_clr, SCU_CPU_RST_DPMCU);
		udelay(1);
	}

	/* select HOST or BMC as display control master
	 * enable or disable sending EDID to Host
	 */
	val = readl((void *)DP_HANDSHAKE);
	val &= ~(DP_HANDSHAKE_HOST_READ_EDID | DP_HANDSHAKE_VIDEO_FMT_SRC);
	writel(val, (void *)DP_HANDSHAKE);

	/* DPMCU */
	/* clear display format and enable region */
	writel(0, (void *)(MCU_DMEM_BASE + 0x0de0));

	/* load DPMCU firmware to internal instruction memory */
	if (is_mcu_stop) {
		mcu_ctrl = MCU_CTRL_CONFIG | MCU_CTRL_IMEM_CLK_OFF | MCU_CTRL_IMEM_SHUT_DOWN |
		      MCU_CTRL_DMEM_CLK_OFF | MCU_CTRL_DMEM_SHUT_DOWN | MCU_CTRL_AHBS_SW_RST;
		writel(mcu_ctrl, (void *)MCU_CTRL);

		mcu_ctrl &= ~(MCU_CTRL_IMEM_SHUT_DOWN | MCU_CTRL_DMEM_SHUT_DOWN);
		writel(mcu_ctrl, (void *)MCU_CTRL);

		mcu_ctrl &= ~(MCU_CTRL_IMEM_CLK_OFF | MCU_CTRL_DMEM_CLK_OFF);
		writel(mcu_ctrl, (void *)MCU_CTRL);

		mcu_ctrl |= MCU_CTRL_AHBS_IMEM_EN;
		writel(mcu_ctrl, (void *)MCU_CTRL);

		for (int i = 0; i < fw_size / sizeof(u32); i++)
			writel(fw_addr[i], (void *)(MCU_IMEM_BASE + (i * 4)));

		/* release DPMCU internal reset */
		mcu_ctrl &= ~MCU_CTRL_AHBS_IMEM_EN;
		writel(mcu_ctrl, (void *)MCU_CTRL);
		mcu_ctrl |= MCU_CTRL_CORE_SW_RST | MCU_CTRL_AHBM_SW_RST;
		writel(mcu_ctrl, (void *)MCU_CTRL);
		//disable dp interrupt
		writel(FIELD_PREP(MCU_INTR_CTRL_EN, 0xff), (void *)MCU_INTR_CTRL);
	}

	//set vga ASTDP with DPMCU FW handling scratch
	val = readl(scu_offset);
	val &= ~(0x7 << 9);
	val |= 0x7 << 9;
	writel(val, scu_offset);

	return 0;
}

int pci_vga_init(void)
{
	struct ast2700_soc0_clk *clk;
	struct ast2700_soc0_scu *scu;
	u32 val, vram_size, vram_addr;
	u8 vram_size_cfg;
	bool is_pcie0_enable;
	bool is_pcie1_enable;
	bool is_64vram = readl((void *)0x12c00100) & BIT(0);
	u8 dac_src;
	u8 dp_src;
	int rc;
	struct udevice *clk_dev;

	rc = uclass_get_device_by_driver(UCLASS_CLK,
					 DM_DRIVER_GET(aspeed_ast2700_soc0_clk), &clk_dev);
	if (rc) {
		printf("%s: cannot find CLK device, rc=%d\n", __func__, rc);
		return rc;
	}

	clk = devfdt_get_addr_ptr(clk_dev);
	if (IS_ERR_OR_NULL(clk)) {
		printf("%s: cannot get CLK address pointer\n", __func__);
		return PTR_ERR(clk);
	}

	scu = fdt_get_syscon_addr_ptr(clk_dev);
	if (IS_ERR_OR_NULL(scu)) {
		printf("%s: cannot get SYSCON address pointer\n", __func__);
		return PTR_ERR(scu);
	}

	if (!((FIELD_GET(SCU_CPU_REVISION_ID_EFUSE, scu->chip_id1) != 2) &&
	      (FIELD_GET(SCU_CPU_REVISION_ID_HW, scu->chip_id1) != 0)))
		return 0;

	is_pcie0_enable = readl(&scu->pci0_misc[28]) & BIT(0);
	is_pcie1_enable = readl(&scu->pci1_misc[28]) & BIT(0);
	dac_src = readl(&scu->hwstrap1) & BIT(28);
	dp_src = readl(&scu->hwstrap1) & BIT(29);

	// 2700 has only 1 VGA
	if (FIELD_GET(SCU_CPU_REVISION_ID_EFUSE, scu->chip_id1) == 1) {
		is_pcie1_enable = false;
		dac_src = 0;
		dp_src = 0;
	}

	debug("%s: ENABLE 0(%d) 1(%d)\n", __func__, is_pcie0_enable, is_pcie1_enable);

	// for CRAA[1:0]
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
		setbits_le32(&clk->clkgate_clr, SCU_CPU_CLKGATE1_VGA0);

		vram_addr -= vram_size;
		debug("pcie0 e2m addr(%x)\n", _ast_get_e2m_addr(vram_addr));
		val = _ast_get_e2m_addr(vram_addr)
		    | FIELD_PREP(SCU_CPU_PCI_MISC0C_FB_SIZE, vram_size_cfg);
		debug("pcie0 debug reg(%x)\n", val);
		writel(val, (void *)E2M0_VGA_RAM);
		writel(val, &scu->pci0_misc[3]);

		// Enable VRAM address offset: cursor, rvas, 2d
		writel(BIT(10) | BIT(24) | BIT(27), (void *)0x12c00104);
	}

	if (is_pcie1_enable) {
		// enable clk
		setbits_le32(&clk->clkgate_clr, SCU_CPU_CLKGATE1_VGA1);

		vram_addr -= vram_size;
		debug("pcie1 e2m addr(%x)\n", _ast_get_e2m_addr(vram_addr));
		val = _ast_get_e2m_addr(vram_addr)
		    | FIELD_PREP(SCU_CPU_PCI_MISC0C_FB_SIZE, vram_size_cfg);
		debug("pcie1 debug reg(%x)\n", val);
		writel(val, (void *)E2M1_VGA_RAM);
		writel(val, &scu->pci1_misc[3]);

		// Enable VRAM address offset: cursor, rvas, 2d
		writel(BIT(19) | BIT(25) | BIT(28), (void *)0x12c00108);
	}

	if (is_pcie0_enable || is_pcie1_enable) {
		// enable dac clk
		setbits_le32(&clk->clkgate_clr, SCU_CPU_CLKGATE1_DAC);

		val = readl(&scu->vga_func_ctrl);
		val &= ~(SCU_CPU_VGA_FUNC_DAC_OUTPUT
			| SCU_CPU_VGA_FUNC_DP_OUTPUT
			| SCU_CPU_VGA_FUNC_DAC_DISABLE);
		val |= FIELD_PREP(SCU_CPU_VGA_FUNC_DAC_OUTPUT, dac_src)
		     | FIELD_PREP(SCU_CPU_VGA_FUNC_DP_OUTPUT, dp_src)
		     | FIELD_PREP(SCU_CPU_VGA_FUNC_DAC_DISABLE, 0);
		writel(val, &scu->vga_func_ctrl);

		// vga link init
		writel(0x00030009, (void *)0x12c1d010);
		val = 0x10000000 | dac_src;
		writel(val, (void *)0x12c1d050);
		writel(0x00010002, (void *)0x12c1d044);
		writel(0x00030009, (void *)0x12c1d110);
		writel(0x00030009, (void *)0x14c3a010);
		writel(0x00030009, (void *)0x14c3a110);

		dp_init(scu, clk);
	}

	return 0;
}
