// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <asm/io.h>
#include <dm/lists.h>
#include <linux/delay.h>
#include <asm/arch-aspeed/clk_ast2700.h>
#include <asm/global_data.h>
#include <dt-bindings/clock/ast2700-clock.h>
#include <dt-bindings/reset/ast2700-reset.h>
#include <dm/device_compat.h>

DECLARE_GLOBAL_DATA_PTR;

/**
 * RGMII clock source tree
 * HPLL -->|\
 *         | |---->| divider |---->RGMII 125M for MAC#0 & MAC#1
 * APLL -->|/
 */
#define RGMII_DEFAULT_CLK_SRC	AST2700_SOC1_CLK_HPLL

/* MAC Clock Delay settings */
#define MAC01_DEF_DELAY_1G		0x00aac105
#define MAC01_DEF_DELAY_100M		0x00410410
#define MAC01_DEF_DELAY_10M		0x00410410

struct ast2700_soc1_clk_priv {
	struct ast2700_soc1_clk *clk;
};

static uint32_t ast2700_soc1_get_pll_rate(struct ast2700_soc1_clk *clk, int pll_idx)
{
	union ast2700_pll_reg pll_reg;
	uint32_t mul = 1, div = 1;

	switch (pll_idx) {
	case AST2700_SOC1_CLK_HPLL:
		pll_reg.w = readl(&clk->hpll);
		break;
	case AST2700_SOC1_CLK_APLL:
		pll_reg.w = readl(&clk->apll);
		break;
	case AST2700_SOC1_CLK_DPLL:
		pll_reg.w = readl(&clk->dpll);
		break;
	}

	if (!pll_reg.b.bypass) {
		mul = (pll_reg.b.m + 1) / (pll_reg.b.n + 1);
		div = (pll_reg.b.p + 1);
	}

	return ((CLKIN_25M * mul) / div);
}

#define SCU_CLKSEL2_HCLK_DIV_MASK		GENMASK(22, 20)
#define SCU_CLKSEL2_HCLK_DIV_SHIFT		20

static uint32_t ast2700_soc1_get_hclk_rate(struct ast2700_soc1_clk *clk)
{
	u32 rate = ast2700_soc1_get_pll_rate(clk, AST2700_SOC1_CLK_HPLL);
	u32 clk_sel2 = readl(&clk->clk_sel2);
	u32 hclk_div = (clk_sel2 & SCU_CLKSEL2_HCLK_DIV_MASK) >>
			     SCU_CLKSEL2_HCLK_DIV_SHIFT;

	if (!hclk_div)
		hclk_div = 2;
	else
		hclk_div++;

	return (rate / hclk_div);
}

#define SCU_CLKSEL1_PCLK_DIV_MASK		GENMASK(20, 18)
#define SCU_CLKSEL1_PCLK_DIV_SHIFT		18

static uint32_t ast2700_soc1_get_pclk_rate(struct ast2700_soc1_clk *clk)
{
	u32 rate = ast2700_soc1_get_hclk_rate(clk);
	u32 clk_sel1 = readl(&clk->clk_sel1);
	u32 pclk_div = (clk_sel1 & SCU_CLKSEL1_PCLK_DIV_MASK) >>
			     SCU_CLKSEL1_PCLK_DIV_SHIFT;

	return (rate / ((pclk_div + 1) * 2));
}

#define SCU_UART_CLKGEN_N_MASK			GENMASK(17, 8)
#define SCU_UART_CLKGEN_N_SHIFT			8
#define SCU_UART_CLKGEN_R_MASK			GENMASK(7, 0)
#define SCU_UART_CLKGEN_R_SHIFT			0

static uint32_t ast2700_soc1_get_uart_uxclk_rate(struct ast2700_soc1_clk *clk)
{
	u32 uxclk_sel = readl(&clk->clk_sel2) & GENMASK(1, 0);
	u32 uxclk_ctrl = readl(&clk->uxclk_ctrl);
	u32 rate;

	switch (uxclk_sel) {
	case 0:
		rate = ast2700_soc1_get_pll_rate(clk, AST2700_SOC1_CLK_APLL) / 4;
		break;
	case 1:
		rate = ast2700_soc1_get_pll_rate(clk, AST2700_SOC1_CLK_APLL) / 2;
		break;
	case 2:
		rate = ast2700_soc1_get_pll_rate(clk, AST2700_SOC1_CLK_APLL);
		break;
	case 3:
		rate = ast2700_soc1_get_pll_rate(clk, AST2700_SOC1_CLK_HPLL);
		break;
	}

	uint32_t n = (uxclk_ctrl & SCU_UART_CLKGEN_N_MASK) >>
		      SCU_UART_CLKGEN_N_SHIFT;
	uint32_t r = (uxclk_ctrl & SCU_UART_CLKGEN_R_MASK) >>
		      SCU_UART_CLKGEN_R_SHIFT;

	return ((rate * r) / (n * 2));
}

#define SCU_HUART_CLKGEN_N_MASK			GENMASK(17, 8)
#define SCU_HUART_CLKGEN_N_SHIFT		8
#define SCU_HUART_CLKGEN_R_MASK			GENMASK(7, 0)
#define SCU_HUART_CLKGEN_R_SHIFT		0

static uint32_t ast2700_soc1_get_uart_huxclk_rate(struct ast2700_soc1_clk *clk)
{
	u32 huxclk_sel = readl(&clk->clk_sel2) & GENMASK(4, 3);
	u32 huxclk_ctrl = readl(&clk->huxclk_ctrl);
	uint32_t n = (huxclk_ctrl & SCU_HUART_CLKGEN_N_MASK) >>
		      SCU_HUART_CLKGEN_N_SHIFT;
	uint32_t r = (huxclk_ctrl & SCU_HUART_CLKGEN_R_MASK) >>
		      SCU_HUART_CLKGEN_R_SHIFT;
	u32 rate;

	switch (huxclk_sel) {
	case 0:
		rate = ast2700_soc1_get_pll_rate(clk, AST2700_SOC1_CLK_APLL) / 4;
		break;
	case 1:
		rate = ast2700_soc1_get_pll_rate(clk, AST2700_SOC1_CLK_APLL) / 2;
		break;
	case 2:
		rate = ast2700_soc1_get_pll_rate(clk, AST2700_SOC1_CLK_APLL);
		break;
	case 3:
		rate = ast2700_soc1_get_pll_rate(clk, AST2700_SOC1_CLK_HPLL);
		break;
	}

	return ((rate * r) / (n * 2));
}

#define SCU_CLKSRC4_SDIO_DIV_MASK		GENMASK(16, 14)
#define SCU_CLKSRC4_SDIO_DIV_SHIFT		14

static uint32_t ast2700_soc1_get_sdio_clk_rate(struct ast2700_soc1_clk *clk)
{
	uint32_t rate = 0;
	uint32_t clk_sel1 = readl(&clk->clk_sel1);
	uint32_t div = (clk_sel1 & SCU_CLKSRC4_SDIO_DIV_MASK) >>
			     SCU_CLKSRC4_SDIO_DIV_SHIFT;

	if (clk_sel1 & BIT(13))
		rate = ast2700_soc1_get_pll_rate(clk, AST2700_SOC1_CLK_APLL);
	else
		rate = ast2700_soc1_get_pll_rate(clk, AST2700_SOC1_CLK_HPLL);

	if (!div)
		div = 1;

	div++;

	return (rate / div);
}

static uint32_t
ast2700_soc1_get_uart_clk_rate(struct ast2700_soc1_clk *clk, int uart_idx)
{
	uint32_t rate = 0;

	if (readl(&clk->clk_sel1) & BIT(uart_idx))
		rate = ast2700_soc1_get_uart_huxclk_rate(clk);
	else
		rate = ast2700_soc1_get_uart_uxclk_rate(clk);

	return rate;
}

static void ast2700_init_rgmii_clk(struct udevice *dev)
{
	struct ast2700_soc1_clk_priv *priv = dev_get_priv(dev);
	uint32_t reg_284 = readl(&priv->clk->clk_sel2);
	uint32_t src_clk = ast2700_soc1_get_pll_rate(priv->clk, RGMII_DEFAULT_CLK_SRC);

	if (RGMII_DEFAULT_CLK_SRC == AST2700_SOC1_CLK_HPLL) {
		uint32_t reg_280;
		uint8_t div_idx;

		/* Calculate the corresponding divider:
		 * 1: div 4
		 * 2: div 6
		 * ...
		 * 7: div 16
		 */
		for (div_idx = 1; div_idx <= 7; div_idx++) {
			uint8_t div = 4 + 2 * (div_idx - 1);

			if (DIV_ROUND_UP(src_clk, div) == 125000000)
				break;
		}
		if (div_idx == 8) {
			dev_err(dev, "Error: RGMII using HPLL cannot divide to 125 MHz\n");
			return;
		}

		/* set HPLL clock divider */
		reg_280 = readl(&priv->clk->clk_sel1);
		reg_280 &= ~GENMASK(27, 25);
		reg_280 |= div_idx << 25;
		writel(reg_280, &priv->clk->clk_sel1);

		/* select HPLL clock source */
		reg_284 &= ~BIT(18);
	} else {
		/* APLL clock divider is fixed to 8 */
		if (DIV_ROUND_UP(src_clk, 8) != 125000000) {
			dev_err(dev, "Error: RGMII using APLL cannot divide to 125 MHz\n");
			return;
		}

		/* select APLL clock source */
		reg_284 |= BIT(18);
	}

	writel(reg_284, &priv->clk->clk_sel2);
}

static void ast2700_configure_mac01_clk(struct ast2700_soc1_clk *clk)
{
	/* set 1000M/100M/10M default delay */
	clrsetbits_le32(&clk->mac_delay, GENMASK(25, 0), MAC01_DEF_DELAY_1G);
	writel(MAC01_DEF_DELAY_100M, &clk->mac_100m_delay);
	writel(MAC01_DEF_DELAY_10M, &clk->mac_10m_delay);
}

static ulong ast2700_soc1_clk_get_rate(struct clk *clk)
{
	struct ast2700_soc1_clk_priv *priv = dev_get_priv(clk->dev);
	ulong rate = 0;

	switch (clk->id) {
	case AST2700_SOC1_CLK_HPLL:
	case AST2700_SOC1_CLK_APLL:
	case AST2700_SOC1_CLK_DPLL:
		rate = ast2700_soc1_get_pll_rate(priv->clk, clk->id);
		break;
	case AST2700_SOC1_CLK_AHB:
		rate = ast2700_soc1_get_hclk_rate(priv->clk);
		break;
	case AST2700_SOC1_CLK_APB:
		rate = ast2700_soc1_get_pclk_rate(priv->clk);
		break;
	case AST2700_SOC1_CLK_GATE_UART0CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->clk, 0);
		break;
	case AST2700_SOC1_CLK_GATE_UART1CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->clk, 1);
		break;
	case AST2700_SOC1_CLK_GATE_UART2CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->clk, 2);
		break;
	case AST2700_SOC1_CLK_GATE_UART3CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->clk, 3);
		break;
	case AST2700_SOC1_CLK_GATE_UART5CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->clk, 5);
		break;
	case AST2700_SOC1_CLK_GATE_UART6CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->clk, 6);
		break;
	case AST2700_SOC1_CLK_GATE_UART7CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->clk, 7);
		break;
	case AST2700_SOC1_CLK_GATE_UART8CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->clk, 8);
		break;
	case AST2700_SOC1_CLK_GATE_UART9CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->clk, 9);
		break;
	case AST2700_SOC1_CLK_GATE_UART10CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->clk, 10);
		break;
	case AST2700_SOC1_CLK_GATE_UART11CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->clk, 11);
		break;
	case AST2700_SOC1_CLK_GATE_UART12CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->clk, 12);
		break;
	case AST2700_SOC1_CLK_GATE_SDCLK:
		rate = ast2700_soc1_get_sdio_clk_rate(priv->clk);
		break;
	case AST2700_SOC1_CLK_UXCLK:
		rate = ast2700_soc1_get_uart_uxclk_rate(priv->clk);
		break;
	case AST2700_SOC1_CLK_HUXCLK:
		rate = ast2700_soc1_get_uart_huxclk_rate(priv->clk);
		break;
	default:
		debug("%s: unknown clk %ld\n", __func__, clk->id);
		return -ENOENT;
	}

	return rate;
}

static int ast2700_soc1_clk_enable(struct clk *clk)
{
	struct ast2700_soc1_clk_priv *priv = dev_get_priv(clk->dev);
	struct ast2700_soc1_clk *io_clk = priv->clk;
	u32 clkgate_bit;

	if (clk->id > 32)
		clkgate_bit = BIT(clk->id - 32);
	else
		clkgate_bit = BIT(clk->id);
	if (readl(&io_clk->clkgate_ctrl1) & clkgate_bit)
		writel(clkgate_bit, &io_clk->clkgate_clr1);

	return 0;
}

struct clk_ops ast2700_soc1_clk_ops = {
	.get_rate = ast2700_soc1_clk_get_rate,
	.enable = ast2700_soc1_clk_enable,
};

static int ast2700_soc1_clk_probe(struct udevice *dev)
{
	struct ast2700_soc1_clk_priv *priv = dev_get_priv(dev);

	priv->clk = dev_read_addr_ptr(dev);
	if (IS_ERR(priv->clk))
		return PTR_ERR(priv->clk);

	ast2700_init_rgmii_clk(dev);
	ast2700_configure_mac01_clk(priv->clk);

	return 0;
}

static int ast2700_soc1_clk_bind(struct udevice *dev)
{
	int ret;

	/* The reset driver does not have a device node, so bind it here */
	ret = device_bind_driver(gd->dm_root, "ast_sysreset", "reset", &dev);
	if (ret)
		debug("Warning: No reset driver: ret = %d\n", ret);

	return 0;
}

static const struct udevice_id ast2700_soc1_clk_ids[] = {
	{ .compatible = "aspeed,ast2700-soc1-clk", },
	{ },
};

U_BOOT_DRIVER(aspeed_ast2700_soc1_clk) = {
	.name = "aspeed_ast2700_soc1_clk",
	.id = UCLASS_CLK,
	.of_match = ast2700_soc1_clk_ids,
	.priv_auto = sizeof(struct ast2700_soc1_clk_priv),
	.ops = &ast2700_soc1_clk_ops,
	.probe = ast2700_soc1_clk_probe,
	.bind = ast2700_soc1_clk_bind,
};
