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
#include <asm/arch/clk_ast2700.h>
#include <asm/global_data.h>
#include <dt-bindings/clock/ast2700-clock.h>
#include <dt-bindings/reset/ast2700-reset.h>

#define ASPEED_FPGA

DECLARE_GLOBAL_DATA_PTR;

#define CLKIN_25M 25000000UL

struct ast2700_cpu_clk_priv {
	struct ast2700_cpu_clk *clk;
};

extern uint32_t ast2700_cpu_get_pll_rate(struct ast2700_cpu_clk *clk, int pll_idx)
{
#ifdef ASPEED_FPGA
	return 24000000;
#else
	union ast2700_pll_reg pll_reg;
	uint32_t hwstrap1;
	uint32_t cpu_freq;
	uint32_t mul = 1, div = 1;

	switch (pll_idx) {
	case AST2700_CPU_CLK_HPLL:
		pll_reg.w = readl(clk->hpll);
		break;
	case AST2700_CPU_CLK_DPLL:
		pll_reg.w = readl(&clk->dpll);
		break;
	}
	case AST2700_CPU_CLK_MPLL:
		pll_reg.w = readl(&clk->mpll);
		break;
	}

	if (!pll_reg.b.bypass) {
		if (pll_idx == AST2700_CPU_CLK_MPLL) {
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
#endif
}

static uint32_t ast2700_cpu_get_hclk_rate(struct ast2700_cpu_clk *clk)
{
#ifdef ASPEED_FPGA
	return 50000000;
#else
	u32 rate = ast2700_cpu_get_pll_rate(clk, AST2700_CPU_CLK_HPLL);
	u32 fixed_div = 4;

	return (rate / fixed_div);
#endif
}

#define SCU_CLKSEL1_PCLK_DIV_MASK		GENMASK(25, 23)
#define SCU_CLKSEL1_PCLK_DIV_SHIFT		23

static uint32_t ast2700_cpu_get_pclk_rate(struct ast2700_cpu_clk *clk)
{
#ifdef ASPEED_FPGA
	//hpll/4
	return 6000000;
#else
	u32 rate = ast2700_cpu_get_pll_rate(clk, AST2700_CPU_CLK_HPLL);
	u32 clksel1 = readl(&clk->clk_sel1);
	u32 pclk_div = (clksel1 & SCU_CLKSEL1_PCLK_DIV_MASK) >>
			    SCU_CLKSEL1_PCLK_DIV_SHIFT;

	return (rate / ((pclk_div + 1) * 4));
#endif
}

#define SCU_CLKSEL1_BCLK_DIV_MASK		GENMASK(22, 20)
#define SCU_CLKSEL1_BCLK_DIV_SHIFT		20

static uint32_t ast2700_cpu_get_bclk_rate(struct ast2700_cpu_clk *clk)
{
#ifdef ASPEED_FPGA
	return 50000000;
#else
	u32 rate = ast2700_cpu_get_pll_rate(clk, AST2700_CPU_CLK_HPLL);
	u32 clksel1 = readl(&clk->clk_sel1);
	u32 bclk_div = (clksel1 & SCU_CLKSEL1_BCLK_DIV_MASK) >>
			     SCU_CLKSEL1_BCLK_DIV_SHIFT;

	return (rate / ((bclk_div + 1) * 4));
#endif
}

static ulong ast2700_cpu_clk_get_rate(struct clk *clk)
{
	struct ast2700_cpu_clk_priv *priv = dev_get_priv(clk->dev);
	ulong rate = 0;

	switch (clk->id) {
	case AST2700_CPU_CLK_HPLL:
	case AST2700_CPU_CLK_DPLL:
	case AST2700_CPU_CLK_MPLL:
		rate = ast2700_cpu_get_pll_rate(priv->clk, clk->id);
		break;
	case AST2700_CPU_CLK_AHB:
		rate = ast2700_cpu_get_hclk_rate(priv->clk);
		break;
	case AST2700_CPU_CLK_APB:
		rate = ast2700_cpu_get_pclk_rate(priv->clk);
		break;
	case AST2700_CPU_CLK_BCLK:
		rate = ast2700_cpu_get_bclk_rate(priv->clk);
		break;
	default:
		debug("%s: unknown clk %ld\n", __func__, clk->id);
		return -ENOENT;
	}

	return rate;
}

/**
 * @brief	lookup PLL divider config by input/output rate
 * @param[in]	*pll - PLL descriptor
 * Return:	true - if PLL divider config is found, false - else
 * The function caller shall fill "pll->in" and "pll->out",
 * then this function will search the lookup table
 * to find a valid PLL divider configuration.
 */
static bool ast2700_search_clock_config(struct ast2700_pll_desc *pll)
{
	uint32_t i;
	const struct ast2700_pll_desc *def_desc;
	bool is_found = false;

	for (i = 0; i < ARRAY_SIZE(ast2700_pll_lookup); i++) {
		def_desc = &ast2700_pll_lookup[i];

		if (def_desc->in == pll->in && def_desc->out == pll->out) {
			is_found = true;
			pll->cfg.reg.w = def_desc->cfg.reg.w;
			pll->cfg.ext_reg = def_desc->cfg.ext_reg;
			break;
		}
	}
	return is_found;
}

static uint32_t ast2700_configure_pll(struct ast2700_cpu_clk *clk,
				      struct ast2700_pll_cfg *p_cfg,
				      int pll_idx)
{
	void __iomem *addr, *addr_ext;
	uint32_t reg;

	switch (pll_idx) {
	case AST2700_CPU_CLK_HPLL:
		addr = (void __iomem *)(&clk->hpll);
		addr_ext = (void __iomem *)(&clk->hpll_ext);
		break;
	case AST2700_CPU_CLK_MPLL:
		addr = (void __iomem *)(&clk->mpll);
		addr_ext = (void __iomem *)(&clk->mpll_ext);
		break;
	default:
		debug("unknown PLL index\n");
		return 1;
	}

	p_cfg->reg.b.bypass = 0;
	p_cfg->reg.b.off = 1;
	p_cfg->reg.b.reset = 1;

	reg = readl(addr);
	reg &= ~GENMASK(25, 0);
	reg |= p_cfg->reg.w;
	writel(reg, addr);

	/* write extend parameter */
	writel(p_cfg->ext_reg, addr_ext);
	udelay(100);
	p_cfg->reg.b.off = 0;
	p_cfg->reg.b.reset = 0;
	reg &= ~GENMASK(25, 0);
	reg |= p_cfg->reg.w;
	writel(reg, addr);

	while (!(readl(addr_ext) & BIT(31)))
		;

	return 0;
}

static uint32_t ast2700_configure_ddr(struct ast2700_cpu_clk *clk, ulong rate)
{
	struct ast2700_pll_desc mpll;

	mpll.in = CLKIN_25M;
	mpll.out = rate;
	if (ast2700_search_clock_config(&mpll) == false) {
		printf("error!! unable to find valid DDR clock setting\n");
		return 0;
	}
	ast2700_configure_pll(clk, &mpll.cfg, AST2700_CPU_CLK_MPLL);

	return ast2700_cpu_get_pll_rate(clk, AST2700_CPU_CLK_MPLL);
}

static ulong ast2700_cpu_clk_set_rate(struct clk *clk, ulong rate)
{
	struct ast2700_cpu_clk_priv *priv = dev_get_priv(clk->dev);
	ulong new_rate;

	switch (clk->id) {
	case AST2700_CPU_CLK_MPLL:
		new_rate = ast2700_configure_ddr(priv->clk, rate);
		break;
	default:
		return -ENOENT;
	}

	return new_rate;
}

static ulong ast2700_enable_emmcclk(struct ast2700_cpu_clk *clk)
{
	u32 reset_bit;
	u32 clkgate_bit;

	reset_bit = BIT(ASPEED_RESET_EMMC);
	clkgate_bit = AST2700_CPU_CLK_GATE_EMMCCLK;

	writel(reset_bit, &clk->modrst_ctrl);
	udelay(100);
	writel(clkgate_bit, &clk->clkgate_clr);
	mdelay(10);
	writel(reset_bit, &clk->modrst_clr);

	return 0;
}

#define SCU_CLKSRC1_MAC_DIV_MASK		GENMASK(18, 16)
#define SCU_CLKSRC1_MAC_DIV_SHIFT		16
#define SCU_CLKSRC1_EMMC_EN			BIT(15)
#define SCU_CLKSRC1_EMMC_DIV_MASK		GENMASK(14, 12)
#define SCU_CLKSRC1_EMMC_DIV_SHIFT		12
#define SCU_CLKSRC1_EMMC			BIT(11)

static ulong ast2700_enable_extemmcclk(struct ast2700_cpu_clk *clk)
{
	int i = 0;
	u32 div = 0;
	u32 rate = 0;
	u32 clksrc1 = readl(&clk->clk_sel1);

	rate = ast2700_cpu_get_pll_rate(clk, AST2700_CPU_CLK_MPLL);
	for (i = 0; i < 8; i++) {
		div = (i + 1) * 2;
		if ((rate / div) <= 200000000)
			break;
	}

	clksrc1 &= ~SCU_CLKSRC1_EMMC_DIV_MASK;
	clksrc1 |= (i << SCU_CLKSRC1_EMMC_DIV_SHIFT);
	clksrc1 |= SCU_CLKSRC1_EMMC;
	writel(clksrc1, &clk->clk_sel1);

	setbits_le32(&clk->clk_sel1, SCU_CLKSRC1_EMMC_EN);

	return 0;
}

static int ast2700_cpu_clk_enable(struct clk *clk)
{
	struct ast2700_cpu_clk_priv *priv = dev_get_priv(clk->dev);
	struct ast2700_cpu_clk *cpu_clk = priv->clk;

	switch (clk->id) {
	case AST2700_CPU_CLK_GATE_EMMCCLK:
		ast2700_enable_emmcclk(cpu_clk);
		break;
	case AST2700_CPU_CLK_EMMC:
		ast2700_enable_extemmcclk(cpu_clk);
		break;
	default:
		debug("%s: unknown clk %ld\n", __func__, clk->id);
		return -ENOENT;
	}

	return 0;
}

struct clk_ops ast2700_cpu_clk_ops = {
	.get_rate = ast2700_cpu_clk_get_rate,
	.set_rate = ast2700_cpu_clk_set_rate,
	.enable = ast2700_cpu_clk_enable,
};

static int ast2700_cpu_clk_probe(struct udevice *dev)
{
	struct ast2700_cpu_clk_priv *priv = dev_get_priv(dev);

	priv->clk = devfdt_get_addr_ptr(dev);
	if (IS_ERR(priv->clk))
		return PTR_ERR(priv->clk);

	return 0;
}

static int ast2700_cpu_clk_bind(struct udevice *dev)
{
	int ret;

	/* The reset driver does not have a device node, so bind it here */
	ret = device_bind_driver(gd->dm_root, "ast_sysreset", "reset", &dev);
	if (ret)
		debug("Warning: No reset driver: ret=%d\n", ret);

	return 0;
}

static const struct udevice_id ast2700_cpu_clk_ids[] = {
	{ .compatible = "aspeed,ast2700_cpu-clk", },
	{ },
};

U_BOOT_DRIVER(aspeed_ast2700_cpu_clk) = {
	.name = "aspeed_ast2700_cpu_clk",
	.id = UCLASS_CLK,
	.of_match = ast2700_cpu_clk_ids,
	.priv_auto = sizeof(struct ast2700_cpu_clk_priv),
	.ops = &ast2700_cpu_clk_ops,
	.bind = ast2700_cpu_clk_bind,
	.probe = ast2700_cpu_clk_probe,
};
