// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 Aspeed Technology Inc.
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/platform.h>
#include <asm/arch/scu_ast2700.h>
#include <asm/arch/sdram_ast2700.h>
#include <clk.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <errno.h>
#include <log.h>
#include <reset.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>

#define E2M0_VGA_RAM			(0x100)
#define E2M1_VGA_RAM			(0x120)

struct ast_vga_priv {
	struct ast2700_scu0 *scu;
	struct sdramc_regs *ram;
	void __iomem *e2m0_base;
	void __iomem *e2m1_base;
	void __iomem *vgal_cpu_base;
	void __iomem *vgal_io_base;
	struct clk vga0_clk;
	struct clk vga1_clk;
	struct clk dac_clk;
	struct reset_ctl vgal_reset_ctl;
};

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

static void _ast_update_e2m(struct ast_vga_priv *priv)
{
	u32 val, vram_size;
	u8 vram_size_cfg;
	bool is_pcie0_enable = priv->scu->pci0_misc[28] & BIT(0);
	bool is_pcie1_enable = priv->scu->pci1_misc[28] & BIT(0);
	bool is_64vram = priv->ram->gfmcfg & BIT(0);

	vram_size_cfg = is_64vram ? 0xf : 0xe;
	vram_size = 2 << (vram_size_cfg + 10);
	debug("%s: VRAM size(%x) cfg(%x)\n", __func__, vram_size, vram_size_cfg);

	/* scratch for VGA CRAA[1:0] : 10b: 32Mbytes, 11b: 64Mbytes */
	setbits_le32(&priv->scu->hwstrap1, BIT(11));
	if (is_64vram)
		setbits_le32(&priv->scu->hwstrap1, BIT(10));
	else
		setbits_le32(&priv->scu->hwstrap1_clr, BIT(10));

	if (is_pcie0_enable) {
		debug("pcie0 e2m addr(%x)\n", _ast_get_e2m_addr(priv->ram, 0));
		val = _ast_get_e2m_addr(priv->ram, 0)
		    | FIELD_PREP(SCU_CPU_PCI_MISC0C_FB_SIZE, vram_size_cfg);
		debug("pcie0 debug reg(%x)\n", val);
		writel(val, priv->e2m0_base + E2M0_VGA_RAM);
		writel(val, &priv->scu->pci0_misc[3]);
	}

	if (is_pcie1_enable) {
		debug("pcie1 e2m addr(%x)\n", _ast_get_e2m_addr(priv->ram, 1));
		val = _ast_get_e2m_addr(priv->ram, 1)
		    | FIELD_PREP(SCU_CPU_PCI_MISC0C_FB_SIZE, vram_size_cfg);
		debug("pcie1 debug reg(%x)\n", val);
		writel(val, priv->e2m1_base + E2M1_VGA_RAM);
		writel(val, &priv->scu->pci1_misc[3]);
	}
}

static int ast_vga_probe(struct udevice *dev)
{
	struct ast_vga_priv *priv = dev_get_priv(dev);
	u32 val;
	bool is_pcie0_enable = priv->scu->pci0_misc[28] & BIT(0);
	bool is_pcie1_enable = priv->scu->pci1_misc[28] & BIT(0);
	u8 dac_src = priv->scu->hwstrap1 & BIT(28);
	u8 dp_src = priv->scu->hwstrap1 & BIT(29);
	u8 efuse = FIELD_GET(SCU_CPU_REVISION_ID_EFUSE, priv->scu->chip_id1);
	int ret;

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
		return -1;
	}

	debug("%s: ENABLE 0(%d) 1(%d)\n", __func__, is_pcie0_enable, is_pcie1_enable);
	debug("%s: dac_src(%d) dp_src(%d)\n", __func__, dac_src, dp_src);

	_ast_update_e2m(priv);

	if (priv->scu->clk_sel3 & BIT(12)) {
		debug("%s: Skip probe since it has been done.\n", __func__);
		return 0;
	}

	// Use d0clk/d1clk which generated from hpll for vga0/1 after A0
	if (FIELD_GET(SCU_CPU_REVISION_ID_HW, priv->scu->chip_id1) != 0)
		setbits_le32(&priv->scu->clk_sel3, BIT(13) | BIT(12));

	if (is_pcie0_enable) {
		ret = clk_enable(&priv->vga0_clk);
		if (ret) {
			dev_err(dev, "%s(): Failed to enable vga0 clk\n", __func__);
			return ret;
		}

		// scratch for VGA CRD0[12]: Disable P2A
		setbits_le32(&priv->scu->vga0_scratch1[0], BIT(7));
		setbits_le32(&priv->scu->vga0_scratch1[0], BIT(12));

		// Enable VRAM address offset: cursor, 2d
		writel(BIT(10) | BIT(27), &priv->ram->gfm0ctl);
	}

	if (is_pcie1_enable) {
		ret = clk_enable(&priv->vga1_clk);
		if (ret) {
			dev_err(dev, "%s(): Failed to enable vga1 clk\n", __func__);
			return ret;
		}

		// scratch for VGA CRD0[12]: Disable P2A
		setbits_le32(&priv->scu->vga1_scratch1[0], BIT(7));
		setbits_le32(&priv->scu->vga1_scratch1[0], BIT(12));

		// Enable VRAM address offset: cursor, 2d
		writel(BIT(19) | BIT(28), &priv->ram->gfm1ctl);
	}

	if ((is_pcie0_enable || is_pcie1_enable)) {
		ret = clk_enable(&priv->dac_clk);
		if (ret) {
			dev_err(dev, "%s(): Failed to enable dac clk\n", __func__);
			return ret;
		}

		reset_deassert(&priv->vgal_reset_ctl);

		val = priv->scu->vga_func_ctrl;
		val &= ~(SCU_CPU_VGA_FUNC_DAC_OUTPUT
			| SCU_CPU_VGA_FUNC_DP_OUTPUT
			| SCU_CPU_VGA_FUNC_DAC_DISABLE);
		val |= FIELD_PREP(SCU_CPU_VGA_FUNC_DAC_OUTPUT, dac_src)
		     | FIELD_PREP(SCU_CPU_VGA_FUNC_DP_OUTPUT, dp_src)
		     | FIELD_PREP(SCU_CPU_VGA_FUNC_DAC_DISABLE, 0);
		writel(val, &priv->scu->vga_func_ctrl);

		// vga link init
		writel(0x00030009, priv->vgal_cpu_base + 0x10);
		val = 0x10000000 | dac_src;
		writel(val, priv->vgal_cpu_base + 0x50);
		writel(0x00100020, priv->vgal_cpu_base + 0x44);
		writel(0x00030009, priv->vgal_cpu_base + 0x110);
		writel(0x00030009, priv->vgal_io_base + 0x10);
		writel(0x00230009, priv->vgal_io_base + 0x110);
		writel(0x00100010, priv->vgal_io_base + 0x144);
	}

	return 0;
}

static int ast_vga_remove(struct udevice *dev)
{
	return 0;
}

static int ast_vga_of_to_plat(struct udevice *dev)
{
	struct ast_vga_priv *priv = dev_get_priv(dev);
	int ret;

	priv->scu = dev_read_addr_index_ptr(dev, 0);
	if (!priv->scu) {
		dev_err(dev, "get scu reg failed\n");
		return -ENOMEM;
	}

	priv->ram = dev_read_addr_index_ptr(dev, 1);
	if (!priv->ram) {
		dev_err(dev, "get dram reg failed\n");
		return -ENOMEM;
	}

	priv->e2m0_base = dev_read_addr_index_ptr(dev, 2);
	if (!priv->e2m0_base) {
		dev_err(dev, "get e2m0 reg failed\n");
		return -ENOMEM;
	}

	priv->e2m1_base = dev_read_addr_index_ptr(dev, 3);
	if (!priv->e2m1_base) {
		dev_err(dev, "get e2m1 reg failed\n");
		return -ENOMEM;
	}

	priv->vgal_cpu_base = dev_read_addr_index_ptr(dev, 4);
	if (!priv->vgal_cpu_base) {
		dev_err(dev, "get vgalink cpu reg failed\n");
		return -ENOMEM;
	}

	priv->vgal_io_base = dev_read_addr_index_ptr(dev, 5);
	if (!priv->vgal_io_base) {
		dev_err(dev, "get vgalink io reg failed\n");
		return -ENOMEM;
	}

	ret = clk_get_by_index(dev, 0, &priv->vga0_clk);
	if (ret) {
		dev_err(dev, "get vga0 clk failed\n");
		return -ENOMEM;
	}

	ret = clk_get_by_index(dev, 1, &priv->vga1_clk);
	if (ret) {
		dev_err(dev, "get vga1 clk failed\n");
		return -ENOMEM;
	}

	ret = clk_get_by_index(dev, 2, &priv->dac_clk);
	if (ret) {
		dev_err(dev, "get dac clk failed\n");
		return -ENOMEM;
	}

	ret = reset_get_by_index(dev, 0, &priv->vgal_reset_ctl);
	if (ret) {
		dev_err(dev, "%s(): Failed to get vga-link reset signal\n", __func__);
		return ret;
	}

	return 0;
}

const struct udevice_id aspeed_vga_of_match[] = {
	{ .compatible = "aspeed,ast2700-vga" },
	{ }
};

U_BOOT_DRIVER(aspeed_vga) = {
	.name		= "aspeed,vga",
	.id		= UCLASS_MISC,
	.of_match	= aspeed_vga_of_match,
	.probe		= ast_vga_probe,
	.remove		= ast_vga_remove,
	.of_to_plat	= ast_vga_of_to_plat,
	.priv_auto	= sizeof(struct ast_vga_priv),
};
