// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 Aspeed Technology Inc.
 */

#include <common.h>
#include <asm/io.h>
#include <clk.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <errno.h>
#include <log.h>
#include <pci_ep.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/sizes.h>

#define   SCU_CPU_REVISION_ID_EFUSE	GENMASK(15, 8)
#define SCU_CPU_HWSTRAP1		(0x010)
#define SCU_CPU_HWSTRAP1_CLR		(0x014)
#define SCU_CPU_VGA_FUNC		(0x414)
#define   VGA_DAC_OUTPUT		GENMASK(11, 10)
#define   VGA_DP_OUTPUT			GENMASK(9, 8)
#define   VGA_DAC_DISABLE		BIT(7)
#define SCU_CPU_VGA0_SAR0		(0xa0c)
#define SCU_PCI_MISC70			(0xa70)
#define SCU_CPU_VGA1_SAR0		(0xa8c)
#define SCU_PCI_MISCF0			(0xaf0)

// for SCU_CPU_VGAx_SAR0
 // [4:0] Frame buffer decode size
 // 0 would not decode
 // 1 decode 4KB
 // 2 decode 8KB
 // ...
 // E decode 32MB (Framebuffer support size)
 // F decode 64MB (Framebuffer support size)
#define VGA_FB_SIZE     GENMASK(4, 0)
 //[31:8] memory base [35:12]
#define VGA_FB_BASE	GENMASK(31, 8)

struct ast_pcie_priv {
	void __iomem *scu_base;
	void __iomem *dram_base;
	void __iomem *e2m0_base;
	void __iomem *e2m1_base;
	void __iomem *vga_cpu_base;
	void __iomem *vga_io_base;
	struct clk vga0_clk;
	struct clk vga1_clk;
	struct clk dac_clk;
};

// To support 64bit address calculation for e2m
static u32 _ast_get_e2m_addr(struct ast_pcie_priv *priv, u32 addr)
{
	u32 val;

	val = readl(priv->dram_base + 0x10) >> 2 & 0x07;

	debug("%s: DRAMC val(%x)\n", __func__, val);
	return (((1 << val) - 1) << 24) | (addr >> 4);
}

static int ast_pci_ep_probe(struct udevice *dev)
{
	struct ast_pcie_priv *priv = dev_get_priv(dev);
	u32 val, vram_size, vram_addr;
	u8 vram_size_cfg;
	bool is_pcie0_enable =
		(readl(priv->scu_base + SCU_PCI_MISC70) & BIT(0));
	bool is_pcie1_enable =
		(readl(priv->scu_base + SCU_PCI_MISCF0) & BIT(0));
	bool is_64vram = readl(priv->dram_base + 0x100) & BIT(0);
	u8 dac_src = readl(priv->scu_base + SCU_CPU_HWSTRAP1) & BIT(28);
	u8 dp_src = readl(priv->scu_base + SCU_CPU_HWSTRAP1) & BIT(29);
	u8 efuse = FIELD_GET(SCU_CPU_REVISION_ID_EFUSE, readl(priv->scu_base));
	int ret;

	if (efuse == 1) {
		// 2700 single node only
		is_pcie1_enable = false;
		dac_src = 0;
		dp_src = 0;
	} else if (efuse == 2) {
		debug("%s: 2720 has no VGA\n", __func__);
		return -1;
	}

	debug("%s: ENABLE 0(%d) 1(%d)\n", __func__, is_pcie0_enable, is_pcie1_enable);
	debug("%s: dac_src(%d) dp_src(%d)\n", __func__, dac_src, dp_src);

	vram_addr = 0x10000000;

	vram_size_cfg = is_64vram ? 0xf : 0xe;
	vram_size = 2 << (vram_size_cfg + 10);
	debug("%s: VRAM size(%x) cfg(%x)\n", __func__, vram_size, vram_size_cfg);

	// for CRAA[1:0]
	setbits_le32(priv->scu_base + SCU_CPU_HWSTRAP1, BIT(11));
	if (is_64vram)
		setbits_le32(priv->scu_base + SCU_CPU_HWSTRAP1, BIT(10));
	else
		setbits_le32(priv->scu_base + SCU_CPU_HWSTRAP1_CLR, BIT(10));

	if ((is_pcie0_enable || is_pcie1_enable)) {
		ret = clk_enable(&priv->dac_clk);
		if (ret) {
			dev_err(dev, "%s(): Failed to enable dac clk\n", __func__);
			return ret;
		}

		val = readl(priv->scu_base + SCU_CPU_VGA_FUNC);
		val &= ~(VGA_DAC_OUTPUT | VGA_DP_OUTPUT | VGA_DAC_DISABLE);
		val |= FIELD_PREP(VGA_DAC_OUTPUT, dac_src)
		     | FIELD_PREP(VGA_DP_OUTPUT, dp_src)
		     | FIELD_PREP(VGA_DAC_DISABLE, 0);
		writel(val, priv->scu_base + SCU_CPU_VGA_FUNC);

		// vga link init
		writel(0x00030009, priv->vga_cpu_base + 0x10);
		val = 0x10000000 | dac_src;
		writel(val, priv->vga_cpu_base + 0x50);
		writel(0x00010002, priv->vga_cpu_base + 0x44);
		writel(0x00030009, priv->vga_cpu_base + 0x110);
		writel(0x00030009, priv->vga_io_base + 0x10);
		writel(0x00030009, priv->vga_io_base + 0x110);
	}

	if (is_pcie0_enable) {
		ret = clk_enable(&priv->vga0_clk);
		if (ret) {
			dev_err(dev, "%s(): Failed to enable vga0 clk\n", __func__);
			return ret;
		}

		vram_addr -= vram_size;
		debug("pcie0 e2m addr(%x)\n", _ast_get_e2m_addr(priv, vram_addr));
		val = _ast_get_e2m_addr(priv, vram_addr) | FIELD_PREP(VGA_FB_SIZE, vram_size_cfg);
		debug("pcie0 debug reg(%x)\n", val);
		writel(val, priv->e2m0_base + 0x100);
		writel(val, priv->scu_base + SCU_CPU_VGA0_SAR0);

		// Enable VRAM address offset: cursor, rvas, 2d
		writel(BIT(10) | BIT(24) | BIT(27), priv->dram_base + 0x104);
	}

	if (is_pcie1_enable) {
		ret = clk_enable(&priv->vga1_clk);
		if (ret) {
			dev_err(dev, "%s(): Failed to enable vga1 clk\n", __func__);
			return ret;
		}

		vram_addr -= vram_size;
		debug("pcie1 e2m addr(%x)\n", _ast_get_e2m_addr(priv, vram_addr));
		val = _ast_get_e2m_addr(priv, vram_addr) | FIELD_PREP(VGA_FB_SIZE, vram_size_cfg);
		debug("pcie1 debug reg(%x)\n", val);
		writel(val, priv->e2m1_base + 0x120);
		writel(val, priv->scu_base + SCU_CPU_VGA1_SAR0);

		// Enable VRAM address offset: cursor, rvas, 2d
		writel(BIT(19) | BIT(25) | BIT(28), priv->dram_base + 0x108);
	}

	return 0;
}

static int ast_pci_ep_remove(struct udevice *dev)
{
	return 0;
}

static int ast_pcie_ep_of_to_plat(struct udevice *dev)
{
	struct ast_pcie_priv *priv = dev_get_priv(dev);
	int ret;

	priv->scu_base = dev_read_addr_index_ptr(dev, 0);
	if (!priv->scu_base) {
		dev_err(dev, "get scu reg failed\n");
		return -ENOMEM;
	}

	priv->dram_base = dev_read_addr_index_ptr(dev, 1);
	if (!priv->dram_base) {
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

	priv->vga_cpu_base = dev_read_addr_index_ptr(dev, 4);
	if (!priv->vga_cpu_base) {
		dev_err(dev, "get vgalink cpu reg failed\n");
		return -ENOMEM;
	}

	priv->vga_io_base = dev_read_addr_index_ptr(dev, 5);
	if (!priv->vga_io_base) {
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

	return 0;
}

const struct udevice_id aspeed_pci_ep_of_match[] = {
	{ .compatible = "aspeed,ast2700-pcie-ep" },
	{ }
};

U_BOOT_DRIVER(aspeed_pcie_ep) = {
	.name		= "aspeed,pcie-ep",
	.id		= UCLASS_PCI_EP,
	.of_match	= aspeed_pci_ep_of_match,
	.probe		= ast_pci_ep_probe,
	.remove		= ast_pci_ep_remove,
	.of_to_plat	= ast_pcie_ep_of_to_plat,
	.priv_auto	= sizeof(struct ast_pcie_priv),
};
