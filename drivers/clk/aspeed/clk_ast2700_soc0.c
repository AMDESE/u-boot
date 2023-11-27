// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include <clk-uclass.h>
#include <dm.h>
#include <regmap.h>
#include <syscon.h>
#include <asm/arch-aspeed/clk_ast2700.h>
#include <dt-bindings/clock/ast2700-clock.h>
#include <dt-bindings/reset/ast2700-reset.h>

DECLARE_GLOBAL_DATA_PTR;

#define CLKIN_25M 25000000UL

struct ast2700_soc0_clk_priv {
	struct regmap *map;
};

static uint32_t ast2700_soc0_get_pll_rate(struct regmap *map, int pll_idx)
{
	union ast2700_pll_reg pll_reg;
	uint32_t mul = 1, div = 1;

	switch (pll_idx) {
	case AST2700_SOC0_CLK_HPLL:
		regmap_read(map, SOC0_CLK_HPLL, &pll_reg.w);
		break;
	case AST2700_SOC0_CLK_DPLL:
		regmap_read(map, SOC0_CLK_DPLL, &pll_reg.w);
		break;
	case AST2700_SOC0_CLK_MPLL:
		regmap_read(map, SOC0_CLK_MPLL, &pll_reg.w);
		break;
	}

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

	return ((CLKIN_25M * mul) / div);
}

static uint32_t ast2700_soc0_get_axiclk_rate(struct regmap *map)
{
	uint reg;

	regmap_read(map, SOC0_HWSTRAP1, &reg);
	if (reg & BIT(7))
		return ast2700_soc0_get_pll_rate(map, AST2700_SOC0_CLK_HPLL) / 2;
	else
		return ast2700_soc0_get_pll_rate(map, AST2700_SOC0_CLK_MPLL) / 2;
}

#define SCU_AHB_DIV_MASK		GENMASK(6, 5)
#define SCU_AHB_DIV_SHIFT		5
static uint32_t ast2700_soc0_get_hclk_rate(struct regmap *map)
{
	u32 rate = ast2700_soc0_get_axiclk_rate(map);
	uint reg;
	int div;

	regmap_read(map, SOC0_HWSTRAP1, &reg);
	div = (reg & SCU_AHB_DIV_MASK) >> SCU_AHB_DIV_SHIFT;

	if (!div)
		div = 2;
	else
		div++;

	return (rate / div);
}

#define SCU_CLKSEL1_PCLK_DIV_MASK		GENMASK(25, 23)
#define SCU_CLKSEL1_PCLK_DIV_SHIFT		23

static uint32_t ast2700_soc0_get_pclk_rate(struct regmap *map)
{
	u32 rate = ast2700_soc0_get_axiclk_rate(map);
	uint reg;
	int div;

	regmap_read(map, SOC0_CLK_SELECT1, &reg);
	div = (reg & SCU_CLKSEL1_PCLK_DIV_MASK) >>
			    SCU_CLKSEL1_PCLK_DIV_SHIFT;

	return (rate / ((div + 1) * 2));
}

#define SCU_CLKSEL1_BCLK_DIV_MASK		GENMASK(22, 20)
#define SCU_CLKSEL1_BCLK_DIV_SHIFT		20
static uint32_t ast2700_soc0_get_bclk_rate(struct regmap *map)
{
	u32 rate = ast2700_soc0_get_pll_rate(map, AST2700_SOC0_CLK_MPLL);
	uint reg;
	int div;

	regmap_read(map, SOC0_CLK_SELECT1, &reg);
	div = (reg & SCU_CLKSEL1_BCLK_DIV_MASK) >>
			     SCU_CLKSEL1_BCLK_DIV_SHIFT;

	return (rate / ((div + 1) * 4));
}

#define SCU_CLKSEL1_MPHYCLK_DIV_MASK		GENMASK(7, 0)
static uint32_t ast2700_soc0_get_mphyclk_rate(struct regmap *map)
{
	u32 rate = ast2700_soc0_get_pll_rate(map, AST2700_SOC0_CLK_HPLL);
	uint reg;
	int div;

	regmap_read(map, SOC0_CLK_MPHY_PARA, &reg);
	div = (reg & SCU_CLKSEL1_BCLK_DIV_MASK);

	return (rate / (div + 1));
}

#define SCU_CLKSRC1_EMMC_DIV_MASK		GENMASK(14, 12)
#define SCU_CLKSRC1_EMMC_DIV_SHIFT		12
#define SCU_CLKSRC1_EMMC_SEL			BIT(11)
static uint32_t ast2700_soc0_get_emmcclk_rate(struct regmap *map)
{
	u32 rate;
	uint reg;
	int div;

	regmap_read(map, SOC0_CLK_SELECT1, &reg);
	div = (reg & SCU_CLKSRC1_EMMC_DIV_MASK) >>
			     SCU_CLKSRC1_EMMC_DIV_SHIFT;

	if (reg & SCU_CLKSRC1_EMMC_SEL)
		rate = ast2700_soc0_get_pll_rate(map, AST2700_SOC0_CLK_HPLL) / 4;
	else
		rate = ast2700_soc0_get_pll_rate(map, AST2700_SOC0_CLK_MPLL) / 4;

	return (rate / ((div + 1) * 2));
}

static uint32_t ast2700_soc0_get_uartclk_rate(struct regmap *map)
{
	u32 div = 1;
	u32 rate;
	uint reg;

	regmap_read(map, SOC0_CLK_SELECT2, &reg);

	if (reg & BIT(15))
		rate = 192000000;
	else
		rate = 24000000;

	if (reg & BIT(30))
		div = 13;
	return (rate / div);
}

static ulong ast2700_soc0_clk_get_rate(struct clk *clk)
{
	struct ast2700_soc0_clk_priv *priv = dev_get_priv(clk->dev);
	ulong rate = 0;

	switch (clk->id) {
	case AST2700_SOC0_CLK_HPLL:
	case AST2700_SOC0_CLK_DPLL:
	case AST2700_SOC0_CLK_MPLL:
		rate = ast2700_soc0_get_pll_rate(priv->map, clk->id);
		break;
	case AST2700_SOC0_CLK_AHB:
		rate = ast2700_soc0_get_hclk_rate(priv->map);
		break;
	case AST2700_SOC0_CLK_APB:
		rate = ast2700_soc0_get_pclk_rate(priv->map);
		break;
	case AST2700_SOC0_CLK_BCLK:
		rate = ast2700_soc0_get_bclk_rate(priv->map);
		break;
	case AST2700_SOC0_CLK_GATE_EMMCCLK:
		rate = ast2700_soc0_get_emmcclk_rate(priv->map);
		break;
	case AST2700_SOC0_CLK_GATE_UART4CLK:
		rate = ast2700_soc0_get_uartclk_rate(priv->map);
		break;
	case AST2700_SOC0_CLK_MPHY:
		rate = ast2700_soc0_get_mphyclk_rate(priv->map);
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

	regmap_write(priv->map, SOC0_CLKGATE_CLR, BIT(clk->id));

	return 0;
}

struct clk_ops ast2700_soc0_clk_ops = {
	.get_rate = ast2700_soc0_clk_get_rate,
	.enable = ast2700_soc0_clk_enable,
};

static int ast2700_soc0_clk_probe(struct udevice *dev)
{
	struct ast2700_soc0_clk_priv *priv = dev_get_priv(dev);
	u32 rate = 0;
	u32 div = 0;
	int i = 0;

	priv->map = syscon_node_to_regmap(dev_ofnode(dev_get_parent(dev)));
	if (IS_ERR(priv->map))
		return PTR_ERR(priv->map);

	/* set emmc clk src mpll/4:400Mhz */
	rate = ast2700_soc0_get_pll_rate(priv->map, AST2700_SOC0_CLK_MPLL) / 4;
	for (i = 0; i < 8; i++) {
		div = (i + 1) * 2;
		if ((rate / div) <= 200000000)
			break;
	}

	regmap_update_bits(priv->map, SOC0_CLK_SELECT1,
			   SCU_CLKSRC1_EMMC_DIV_MASK | SCU_CLKSRC1_EMMC_SEL,
			   (i << SCU_CLKSRC1_EMMC_DIV_SHIFT));

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
