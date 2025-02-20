// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include <asm/io.h>
#include <clk-uclass.h>
#include <dm.h>
#include <regmap.h>
#include <syscon.h>
#include <asm/arch-aspeed/scu_ast2700.h>
#include <dt-bindings/clock/ast2700-clock.h>
#include <dt-bindings/reset/ast2700-reset.h>

DECLARE_GLOBAL_DATA_PTR;

struct ast2700_soc0_clk_priv {
	struct ast2700_soc0_scu *scu;
};

static uint32_t ast2700_soc0_get_pll_rate(struct ast2700_soc0_scu *scu, int pll_idx)
{
	union ast2700_pll_reg pll_reg;
	uint32_t mul = 1, div = 1;
	uint32_t rate;

	switch (pll_idx) {
	case AST2700_SOC0_CLK_HPLL:
		pll_reg.w = readl(&scu->hpll);
		break;
	case AST2700_SOC0_CLK_DPLL:
		pll_reg.w = readl(&scu->dpll);
		break;
	case AST2700_SOC0_CLK_MPLL:
		pll_reg.w = readl(&scu->mpll);
		break;
	}

	if (pll_idx == AST2700_SOC0_CLK_HPLL && ((scu->hwstrap1 & GENMASK(3, 2)) != 0)) {
		switch ((scu->hwstrap1 & GENMASK(3, 2)) >> 2) {
		case 1:
			rate = 1900000;
			break;
		case 2:
			rate = 1800000;
			break;
		case 3:
			rate = 1700000;
			break;
		}
	} else {
		if (!pll_reg.b.bypass) {
			if (pll_idx == AST2700_SOC0_CLK_MPLL) {
				/* F = 25Mhz * [M / (n + 1)] / (p + 1) */
				mul = (pll_reg.b.m) / ((pll_reg.b.n + 1));
				div = (pll_reg.b.p + 1);
			} else {
				/* F = 25Mhz * [(M + 2) / 2 * (n + 1)] / (p + 1) */
				mul = (pll_reg.b.m + 1) / ((pll_reg.b.n + 1) * 2);
				div = (pll_reg.b.p + 1);
			}
		}

		rate = ((CLKIN_25M * mul) / div);
	}

	return rate;
}

static uint32_t ast2700_soc0_get_pspclk_rate(struct ast2700_soc0_scu *scu)
{
	if (readl(&scu->hwstrap1) & BIT(4))
		return ast2700_soc0_get_pll_rate(scu, AST2700_SOC0_CLK_HPLL);
	else
		return ast2700_soc0_get_pll_rate(scu, AST2700_SOC0_CLK_MPLL);
}

static uint32_t ast2700_soc0_get_axi0clk_rate(struct ast2700_soc0_scu *scu)
{
	return ast2700_soc0_get_pspclk_rate(scu) / 2;
}

#define SCU_AHB_DIV_MASK		GENMASK(6, 5)
#define SCU_AHB_DIV_SHIFT		5
static uint32_t ast2700_soc0_get_hclk_rate(struct ast2700_soc0_scu *scu)
{
	u32 hwstrap1 = readl(&scu->hwstrap1);
	u32 src_clk;
	int div;

	if (hwstrap1 & BIT(7))
		src_clk = ast2700_soc0_get_pll_rate(scu, AST2700_SOC0_CLK_HPLL) / 2;
	else
		src_clk = ast2700_soc0_get_pll_rate(scu, AST2700_SOC0_CLK_MPLL) / 2;

	div = (hwstrap1 & SCU_AHB_DIV_MASK) >> SCU_AHB_DIV_SHIFT;

	if (!div)
		div = 2;
	else
		div++;

	return (src_clk / div);
}

static uint32_t ast2700_soc0_get_axi1clk_rate(struct ast2700_soc0_scu *scu)
{
	return ast2700_soc0_get_hclk_rate(scu);
}

#define SCU_CLKSEL1_PCLK_DIV_MASK		GENMASK(25, 23)
#define SCU_CLKSEL1_PCLK_DIV_SHIFT		23

static uint32_t ast2700_soc0_get_pclk_rate(struct ast2700_soc0_scu *scu)
{
	u32 rate = ast2700_soc0_get_axi0clk_rate(scu);
	u32 clksel1 = readl(&scu->clk_sel1);
	int div;

	div = (clksel1 & SCU_CLKSEL1_PCLK_DIV_MASK) >>
			    SCU_CLKSEL1_PCLK_DIV_SHIFT;

	return (rate / ((div + 1) * 2));
}

#define SCU_CLKSEL1_BCLK_DIV_MASK		GENMASK(22, 20)
#define SCU_CLKSEL1_BCLK_DIV_SHIFT		20
static uint32_t ast2700_soc0_get_bclk_rate(struct ast2700_soc0_scu *scu)
{
	u32 rate = ast2700_soc0_get_pll_rate(scu, AST2700_SOC0_CLK_MPLL);
	u32 clksel1 = readl(&scu->clk_sel1);
	int div;

	div = (clksel1 & SCU_CLKSEL1_BCLK_DIV_MASK) >>
			     SCU_CLKSEL1_BCLK_DIV_SHIFT;

	return (rate / ((div + 1) * 4));
}

#define SCU_CLKSEL1_MPHYCLK_DIV_MASK		GENMASK(7, 0)
static uint32_t ast2700_soc0_get_mphyclk_rate(struct ast2700_soc0_scu *scu)
{
	u32 rate = ast2700_soc0_get_pll_rate(scu, AST2700_SOC0_CLK_HPLL);
	u32 mphy_para = readl(&scu->mphyclk_para);
	int div;

	div = (mphy_para & SCU_CLKSEL1_BCLK_DIV_MASK);

	return (rate / (div + 1));
}

#define SCU_CLKSRC1_EMMC_DIV_MASK		GENMASK(14, 12)
#define SCU_CLKSRC1_EMMC_DIV_SHIFT		12
#define SCU_CLKSRC1_EMMC_SEL			BIT(11)
static uint32_t ast2700_soc0_get_emmcclk_rate(struct ast2700_soc0_scu *scu)
{
	u32 clksel1 = readl(&scu->clk_sel1);
	u32 rate;
	int div;

	div = (clksel1 & SCU_CLKSRC1_EMMC_DIV_MASK) >> SCU_CLKSRC1_EMMC_DIV_SHIFT;

	if (clksel1 & SCU_CLKSRC1_EMMC_SEL)
		rate = ast2700_soc0_get_pll_rate(scu, AST2700_SOC0_CLK_HPLL) / 4;
	else
		rate = ast2700_soc0_get_pll_rate(scu, AST2700_SOC0_CLK_MPLL) / 4;

	return (rate / ((div + 1) * 2));
}

static uint32_t ast2700_soc0_get_uartclk_rate(struct ast2700_soc0_scu *scu)
{
	u32 clksel2 = readl(&scu->clk_sel2);
	u32 div = 1;
	u32 rate;

	if (clksel2 & BIT(15))
		rate = 192000000;
	else
		rate = 24000000;

	if (clksel2 & BIT(30))
		div = 13;
	return (rate / div);
}

static ulong ast2700_soc0_clk_get_rate(struct clk *clk)
{
	struct ast2700_soc0_clk_priv *priv = dev_get_priv(clk->dev);
	ulong rate = 0;

	switch (clk->id) {
	case AST2700_SOC0_CLK_PSP:
		rate = ast2700_soc0_get_pspclk_rate(priv->scu);
		break;
	case AST2700_SOC0_CLK_HPLL:
	case AST2700_SOC0_CLK_DPLL:
	case AST2700_SOC0_CLK_MPLL:
		rate = ast2700_soc0_get_pll_rate(priv->scu, clk->id);
		break;
	case AST2700_SOC0_CLK_AXI1:
		rate = ast2700_soc0_get_axi1clk_rate(priv->scu);
		break;
	case AST2700_SOC0_CLK_AHB:
		rate = ast2700_soc0_get_hclk_rate(priv->scu);
		break;
	case AST2700_SOC0_CLK_APB:
		rate = ast2700_soc0_get_pclk_rate(priv->scu);
		break;
	case AST2700_SOC0_CLK_BCLK:
		rate = ast2700_soc0_get_bclk_rate(priv->scu);
		break;
	case AST2700_SOC0_CLK_GATE_EMMCCLK:
		rate = ast2700_soc0_get_emmcclk_rate(priv->scu);
		break;
	case AST2700_SOC0_CLK_GATE_UART4CLK:
		rate = ast2700_soc0_get_uartclk_rate(priv->scu);
		break;
	case AST2700_SOC0_CLK_MPHY:
		rate = ast2700_soc0_get_mphyclk_rate(priv->scu);
		break;
	default:
		debug("%s: unknown clk %ld\n", __func__, clk->id);
		return -ENOENT;
	}

	return rate;
}

static int ast2700_soc0_clk_enable(struct clk *clk)
{
	struct ast2700_soc0_clk_priv *priv = dev_get_priv(clk->dev);
	struct ast2700_soc0_scu *scu = priv->scu;
	u32 clkgate_bit = BIT(clk->id);

	writel(clkgate_bit, &scu->clkgate_clr);

	return 0;
}

struct clk_ops ast2700_soc0_clk_ops = {
	.get_rate = ast2700_soc0_clk_get_rate,
	.enable = ast2700_soc0_clk_enable,
};

static int ast2700_soc0_clk_probe(struct udevice *dev)
{
	struct ast2700_soc0_clk_priv *priv = dev_get_priv(dev);
	struct ast2700_soc0_scu *scu;
	u32 rate = 0;
	u32 div = 0;
	int i = 0;

	u32 clksrc1;

	priv->scu = dev_read_addr_ptr(dev->parent);
	if (IS_ERR(priv->scu))
		return PTR_ERR(priv->scu);

	scu = priv->scu;
	/* set emmc clk src mpll/4:400Mhz */
	clksrc1 = readl(&scu->clk_sel1);
	rate = ast2700_soc0_get_pll_rate(priv->scu, AST2700_SOC0_CLK_MPLL) / 4;
	for (i = 0; i < 8; i++) {
		div = (i + 1) * 2;
		if ((rate / div) <= 200000000)
			break;
	}

	clksrc1 &= ~(SCU_CLKSRC1_EMMC_DIV_MASK | SCU_CLKSRC1_EMMC_SEL);
	clksrc1 |= (i << SCU_CLKSRC1_EMMC_DIV_SHIFT);
	writel(clksrc1, &scu->clk_sel1);

	/* set mphy clk from hpll */
	rate = ast2700_soc0_get_pll_rate(priv->scu, AST2700_SOC0_CLK_HPLL);
	for (i = 1; i < 256; i++) {
		if ((rate / i) <= 26000000)
			break;
	}

	/* register defined the value plus 1 is divider*/
	i--;
	writel(i, &scu->mphyclk_para);

	return 0;
}

static const struct udevice_id ast2700_soc0_clk_ids[] = {
	{ .compatible = "aspeed,ast2700-soc0-clk", },
	{ },
};

U_BOOT_DRIVER(aspeed_ast2700_soc0_clk) = {
	.name = "aspeed_ast2700_soc0_clk",
	.id = UCLASS_CLK,
	.of_match = ast2700_soc0_clk_ids,
	.priv_auto = sizeof(struct ast2700_soc0_clk_priv),
	.ops = &ast2700_soc0_clk_ops,
	.probe = ast2700_soc0_clk_probe,
};
