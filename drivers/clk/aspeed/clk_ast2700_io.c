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

struct ast2700_io_clk_priv {
	struct ast2700_io_clk *clk;
};

static uint32_t ast2700_io_get_pll_rate(struct ast2700_io_clk *clk, int pll_idx)
{
#ifdef ASPEED_FPGA
	return 24000000;
#else
	union ast2700_pll_reg pll_reg;
	uint32_t mul = 1, div = 1;

	switch (pll_idx) {
	case AST2700_IO_CLK_HPLL:
		pll_reg.w = readl(&clk->hpll);
		break;
	case AST2700_IO_CLK_APLL:
		pll_reg.w = readl(&clk->apll);
		break;
	case AST2700_IO_CLK_DPLL:
		pll_reg.w = readl(&clk->dpll);
		break;
	}

	if (!pll_reg.b.bypass) {
		mul = (pll_reg.b.m + 1) / (pll_reg.b.n + 1);
		div = (pll_reg.b.p + 1);
	}

	return ((CLKIN_25M * mul) / div);
#endif
}

#define SCU_CLKSEL2_HCLK_DIV_MASK		GENMASK(22, 20)
#define SCU_CLKSEL2_HCLK_DIV_SHIFT		20

static uint32_t ast2700_io_get_hclk_rate(struct ast2700_io_clk *clk)
{
#ifdef ASPEED_FPGA
	//hpll/4
	return 12000000;
#else
	u32 rate = ast2700_io_get_pll_rate(clk, AST2700_IO_CLK_HPLL);
	u32 clk_sel2 = readl(&clk->clk_sel2);
	u32 hclk_div = (clk_sel2 & SCU_CLKSEL2_HCLK_DIV_MASK) >>
			     SCU_CLKSEL2_HCLK_DIV_SHIFT;

	if (!hclk_div)
		hclk_div++;

	return (rate / ((hclk_div + 1) * 2));
#endif
}

#define SCU_CLKSEL1_PCLK_DIV_MASK		GENMASK(20, 18)
#define SCU_CLKSEL1_PCLK_DIV_SHIFT		18

static uint32_t ast2700_io_get_pclk_rate(struct ast2700_io_clk *clk)
{
#ifdef ASPEED_FPGA
	//hpll/4
	return 12000000;
#else
	u32 rate = ast2700_io_get_hclk_rate(clk);
	u32 clk_sel1 = readl(&clk->clk_sel1);
	u32 pclk_div = (clk_sel1 & SCU_CLKSEL1_PCLK_DIV_MASK) >>
			     SCU_CLKSEL1_PCLK_DIV_SHIFT;

	return (rate / ((pclk_div + 1) * 2));
#endif
}

#define SCU_UART_CLKGEN_N_MASK			GENMASK(17, 8)
#define SCU_UART_CLKGEN_N_SHIFT			8
#define SCU_UART_CLKGEN_R_MASK			GENMASK(7, 0)
#define SCU_UART_CLKGEN_R_SHIFT			0

static uint32_t ast2700_io_get_uart_uxclk_rate(struct ast2700_io_clk *clk)
{
#ifdef ASPEED_FPGA
	return (24000000 / 13);
#else
	u32 uxclk_sel = readl(&clk->clk_sel2) & GENMASK(1, 0);
	u32 uxclk_ctrl = readl(&clk->uxclk_ctrl);
	u32 rate;

	switch (uxclk_sel) {
	case 0:
		rate = ast2700_io_get_pll_rate(clk, AST2700_IO_CLK_APLL) / 4;
		break;
	case 1:
		rate = ast2700_io_get_pll_rate(clk, AST2700_IO_CLK_APLL) / 2;
		break;
	case 2:
		rate = ast2700_io_get_pll_rate(clk, AST2700_IO_CLK_APLL);
		break;
	case 3:
		rate = ast2700_io_get_pll_rate(clk, AST2700_IO_CLK_HPLL);
		break;
	}

	uint32_t n = (uxclk_ctrl & SCU_UART_CLKGEN_N_MASK) >>
		      SCU_UART_CLKGEN_N_SHIFT;
	uint32_t r = (uxclk_ctrl & SCU_UART_CLKGEN_R_MASK) >>
		      SCU_UART_CLKGEN_R_SHIFT;

	return ((rate * r) / (n * 2));
#endif
}

#define SCU_HUART_CLKGEN_N_MASK			GENMASK(17, 8)
#define SCU_HUART_CLKGEN_N_SHIFT		8
#define SCU_HUART_CLKGEN_R_MASK			GENMASK(7, 0)
#define SCU_HUART_CLKGEN_R_SHIFT		0

static uint32_t ast2700_io_get_uart_huxclk_rate(struct ast2700_io_clk *clk)
{
#ifdef ASPEED_FPGA
	return (768000000 / 13);
#else
	u32 huxclk_sel = readl(&clk->clk_sel2) & GENMASK(4, 3);
	u32 huxclk_ctrl = readl(&clk->huxclk_ctrl);
	uint32_t n = (huxclk_ctrl & SCU_HUART_CLKGEN_N_MASK) >>
		      SCU_HUART_CLKGEN_N_SHIFT;
	uint32_t r = (huxclk_ctrl & SCU_HUART_CLKGEN_R_MASK) >>
		      SCU_HUART_CLKGEN_R_SHIFT;
	u32 rate;

	switch (huxclk_sel) {
	case 0:
		rate = ast2700_io_get_pll_rate(clk, AST2700_IO_CLK_APLL) / 4;
		break;
	case 1:
		rate = ast2700_io_get_pll_rate(clk, AST2700_IO_CLK_APLL) / 2;
		break;
	case 2:
		rate = ast2700_io_get_pll_rate(clk, AST2700_IO_CLK_APLL);
		break;
	case 3:
		rate = ast2700_io_get_pll_rate(clk, AST2700_IO_CLK_HPLL);
		break;
	}

	return ((rate * r) / (n * 2));
#endif
}

#define SCU_CLKSRC4_SDIO_DIV_MASK		GENMASK(16, 14)
#define SCU_CLKSRC4_SDIO_DIV_SHIFT		14

static uint32_t ast2700_io_get_sdio_clk_rate(struct ast2700_io_clk *clk)
{
#ifdef ASPEED_FPGA
	return 50000000;
#else
	uint32_t rate = 0;
	uint32_t clk_sel1 = readl(&clk->clk_sel1);
	uint32_t div = (clk_sel1 & SCU_CLKSRC4_SDIO_DIV_MASK) >>
			     SCU_CLKSRC4_SDIO_DIV_SHIFT;

	if (clk_sel1 & BIT(13))
		rate = ast2700_io_get_pll_rate(clk, AST2700_IO_CLK_HPLL);
	else
		rate = ast2700_io_get_pll_rate(clk, AST2700_IO_CLK_APLL);

	if (!div)
		div = 1;
	else
		div++;

	return (rate / div);
#endif
}

static uint32_t
ast2700_io_get_uart_clk_rate(struct ast2700_io_clk *clk, int uart_idx)
{
	uint32_t rate = 0;

	if (readl(&clk->clk_sel1) & BIT(uart_idx))
		rate = ast2700_io_get_uart_huxclk_rate(clk);
	else
		rate = ast2700_io_get_uart_uxclk_rate(clk);

	return rate;
}

static ulong ast2700_io_clk_get_rate(struct clk *clk)
{
	struct ast2700_io_clk_priv *priv = dev_get_priv(clk->dev);
	ulong rate = 0;

	switch (clk->id) {
	case AST2700_IO_CLK_HPLL:
	case AST2700_IO_CLK_APLL:
	case AST2700_IO_CLK_DPLL:
		rate = ast2700_io_get_pll_rate(priv->clk, clk->id);
		break;
	case AST2700_IO_CLK_AHB:
		rate = ast2700_io_get_hclk_rate(priv->clk);
		break;
	case AST2700_IO_CLK_APB:
		rate = ast2700_io_get_pclk_rate(priv->clk);
		break;
	case AST2700_IO_CLK_GATE_UART0CLK:
		rate = ast2700_io_get_uart_clk_rate(priv->clk, 0);
		break;
	case AST2700_IO_CLK_GATE_UART1CLK:
		rate = ast2700_io_get_uart_clk_rate(priv->clk, 1);
		break;
	case AST2700_IO_CLK_GATE_UART2CLK:
		rate = ast2700_io_get_uart_clk_rate(priv->clk, 2);
		break;
	case AST2700_IO_CLK_GATE_UART3CLK:
		rate = ast2700_io_get_uart_clk_rate(priv->clk, 3);
		break;
	case AST2700_IO_CLK_GATE_UART5CLK:
		rate = ast2700_io_get_uart_clk_rate(priv->clk, 5);
		break;
	case AST2700_IO_CLK_GATE_UART6CLK:
		rate = ast2700_io_get_uart_clk_rate(priv->clk, 6);
		break;
	case AST2700_IO_CLK_GATE_UART7CLK:
		rate = ast2700_io_get_uart_clk_rate(priv->clk, 7);
		break;
	case AST2700_IO_CLK_GATE_UART8CLK:
		rate = ast2700_io_get_uart_clk_rate(priv->clk, 8);
		break;
	case AST2700_IO_CLK_GATE_UART9CLK:
		rate = ast2700_io_get_uart_clk_rate(priv->clk, 9);
		break;
	case AST2700_IO_CLK_GATE_UART10CLK:
		rate = ast2700_io_get_uart_clk_rate(priv->clk, 10);
		break;
	case AST2700_IO_CLK_GATE_UART11CLK:
		rate = ast2700_io_get_uart_clk_rate(priv->clk, 11);
		break;
	case AST2700_IO_CLK_GATE_UART12CLK:
		rate = ast2700_io_get_uart_clk_rate(priv->clk, 12);
		break;
	case AST2700_IO_CLK_GATE_SDCLK:
		rate = ast2700_io_get_sdio_clk_rate(priv->clk);
		break;
	case AST2700_IO_CLK_UXCLK:
		rate = ast2700_io_get_uart_uxclk_rate(priv->clk);
		break;
	case AST2700_IO_CLK_HUXCLK:
		rate = ast2700_io_get_uart_huxclk_rate(priv->clk);
		break;
	default:
		debug("%s: unknown clk %ld\n", __func__, clk->id);
		return -ENOENT;
	}

	return rate;
}

static uint32_t ast2700_configure_mac(struct ast2700_io_clk *clk, int index)
{
	u32 reset_bit;
	u32 clkgate_bit;

	switch (index) {
	case 0:
		reset_bit = BIT(ASPEED_RESET_MAC0);
		clkgate_bit = BIT(AST2700_IO_CLK_GATE_MAC0CLK);
		writel(reset_bit, &clk->modrst_ctrl1);
		udelay(100);
		writel(clkgate_bit, &clk->clkgate_clr1);
		mdelay(10);
		writel(reset_bit, &clk->modrst_clr1);
		break;
	case 1:
		reset_bit = BIT(ASPEED_RESET_MAC1);
		clkgate_bit = BIT(AST2700_IO_CLK_GATE_MAC1CLK);
		writel(reset_bit, &clk->modrst_ctrl1);
		udelay(100);
		writel(clkgate_bit, &clk->clkgate_clr1);
		mdelay(10);
		writel(reset_bit, &clk->modrst_clr1);
		break;
	case 2:
		reset_bit = BIT(ASPEED_RESET_MAC2);
		clkgate_bit = BIT(AST2700_IO_CLK_GATE_MAC2CLK);
		writel(reset_bit, &clk->modrst_ctrl2);
		udelay(100);
		writel(clkgate_bit, &clk->clkgate_clr2);
		mdelay(10);
		writel(reset_bit, &clk->modrst_clr2);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static uint32_t ast2700_configure_uart(struct ast2700_io_clk *clk, int index)
{
	u32 clkgate_bit;

	switch (index) {
	case 0:
		clkgate_bit = BIT(AST2700_IO_CLK_GATE_UART0CLK);
		break;
	case 1:
		clkgate_bit = BIT(AST2700_IO_CLK_GATE_UART1CLK);
		break;
	case 2:
		clkgate_bit = BIT(AST2700_IO_CLK_GATE_UART2CLK);
		break;
	case 3:
		clkgate_bit = BIT(AST2700_IO_CLK_GATE_UART3CLK);
		break;
	case 5:
		clkgate_bit = BIT(AST2700_IO_CLK_GATE_UART5CLK - 32);
		break;
	case 6:
		clkgate_bit = BIT(AST2700_IO_CLK_GATE_UART6CLK - 32);
		break;
	case 7:
		clkgate_bit = BIT(AST2700_IO_CLK_GATE_UART7CLK - 32);
		break;
	case 8:
		clkgate_bit = BIT(AST2700_IO_CLK_GATE_UART8CLK - 32);
		break;
	case 9:
		clkgate_bit = BIT(AST2700_IO_CLK_GATE_UART9CLK - 32);
		break;
	case 10:
		clkgate_bit = BIT(AST2700_IO_CLK_GATE_UART10CLK - 32);
		break;
	case 11:
		clkgate_bit = BIT(AST2700_IO_CLK_GATE_UART11CLK - 32);
		break;
	case 12:
		clkgate_bit = BIT(AST2700_IO_CLK_GATE_UART12CLK - 32);
		break;
	default:
		return -EINVAL;
	}

	writel(clkgate_bit, &clk->clkgate_clr1);

	return 0;
}

static int ast2700_io_clk_enable(struct clk *clk)
{
	struct ast2700_io_clk_priv *priv = dev_get_priv(clk->dev);
	struct ast2700_io_clk *io_clk = priv->clk;

	switch (clk->id) {
	case AST2700_IO_CLK_GATE_MAC0CLK:
		ast2700_configure_mac(io_clk, 0);
		break;
	case AST2700_IO_CLK_GATE_MAC1CLK:
		ast2700_configure_mac(io_clk, 1);
		break;
	case AST2700_IO_CLK_GATE_MAC2CLK:
		ast2700_configure_mac(io_clk, 2);
		break;
	case AST2700_IO_CLK_GATE_UART0CLK:
		ast2700_configure_uart(io_clk, 0);
		break;
	case AST2700_IO_CLK_GATE_UART1CLK:
		ast2700_configure_uart(io_clk, 1);
		break;
	case AST2700_IO_CLK_GATE_UART2CLK:
		ast2700_configure_uart(io_clk, 2);
		break;
	case AST2700_IO_CLK_GATE_UART3CLK:
		ast2700_configure_uart(io_clk, 3);
		break;
	case AST2700_IO_CLK_GATE_UART5CLK:
		ast2700_configure_uart(io_clk, 5);
		break;
	case AST2700_IO_CLK_GATE_UART6CLK:
		ast2700_configure_uart(io_clk, 6);
		break;
	case AST2700_IO_CLK_GATE_UART7CLK:
		ast2700_configure_uart(io_clk, 7);
		break;
	case AST2700_IO_CLK_GATE_UART8CLK:
		ast2700_configure_uart(io_clk, 8);
		break;
	case AST2700_IO_CLK_GATE_UART9CLK:
		ast2700_configure_uart(io_clk, 9);
		break;
	case AST2700_IO_CLK_GATE_UART10CLK:
		ast2700_configure_uart(io_clk, 10);
		break;
	case AST2700_IO_CLK_GATE_UART11CLK:
		ast2700_configure_uart(io_clk, 11);
		break;
	case AST2700_IO_CLK_GATE_UART12CLK:
		ast2700_configure_uart(io_clk, 12);
		break;
	default:
		debug("%s: unknown clk %ld\n", __func__, clk->id);
		return -ENOENT;
	}

	return 0;
}

struct clk_ops ast2700_io_clk_ops = {
	.get_rate = ast2700_io_clk_get_rate,
	.enable = ast2700_io_clk_enable,
};

static int ast2700_io_clk_probe(struct udevice *dev)
{
	struct ast2700_io_clk_priv *priv = dev_get_priv(dev);

	priv->clk = devfdt_get_addr_ptr(dev);
	if (IS_ERR(priv->clk))
		return PTR_ERR(priv->clk);

	return 0;
}

static int ast2700_io_clk_bind(struct udevice *dev)
{
	int ret;

	/* The reset driver does not have a device node, so bind it here */
	ret = device_bind_driver(gd->dm_root, "ast_sysreset", "reset", &dev);
	if (ret)
		debug("Warning: No reset driver: ret=%d\n", ret);

	return 0;
}

static const struct udevice_id ast2700_io_clk_ids[] = {
	{ .compatible = "aspeed,ast2700_io-clk", },
	{ },
};

U_BOOT_DRIVER(aspeed_ast2700_io_clk) = {
	.name = "aspeed_ast2700_io_clk",
	.id = UCLASS_CLK,
	.of_match = ast2700_io_clk_ids,
	.priv_auto = sizeof(struct ast2700_io_clk_priv),
	.ops = &ast2700_io_clk_ops,
	.bind = ast2700_io_clk_bind,
	.probe = ast2700_io_clk_probe,
};
