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

extern uint32_t ast2700_io_get_pll_rate(struct ast2700_io_clk *clk, int pll_idx)
{
#ifdef ASPEED_FPGA
	return 24000000;
#else
	union ast2700_pll_reg pll_reg;
	uint32_t hwstrap1;
	uint32_t cpu_freq;
	uint32_t mul = 1, div = 1;

	switch (pll_idx) {
	case ASPEED_CLK_APLL:
		pll_reg.w = readl(&clk->apll);
		break;
	case ASPEED_CLK_DPLL:
		pll_reg.w = readl(&clk->dpll);
		break;
	case ASPEED_CLK_EPLL:
		pll_reg.w = readl(&clk->epll);
		break;
	}

	if (!pll_reg.b.bypass) {
		/* F = 25Mhz * [(M + 2) / (n + 1)] / (p + 1)s
		 * HPLL Numerator (M) = fix 0x5F when SCU500[10]=1
		 * Fixed 0xBF when SCU500[10]=0 and SCU500[8]=1
		 * SCU200[12:0] (default 0x8F) when SCU510[10]=0 and SCU510[8]=0
		 * HPLL Denumerator (N) =	SCU200[18:13] (default 0x2)
		 * HPLL Divider (P)	 =	SCU200[22:19] (default 0x0)
		 * HPLL Bandwidth Adj (NB) =  fix 0x2F when SCU500[10]=1
		 * Fixed 0x5F when SCU500[10]=0 and SCU500[8]=1
		 * SCU204[11:0] (default 0x31) when SCU500[10]=0 and SCU500[8]=0
		 */
		if (pll_idx == ASPEED_CLK_HPLL) {
			hwstrap1 = readl(&clk->hwstrap1);
			cpu_freq = (hwstrap1 & SCU_HWSTRAP1_CPU_FREQ_MASK) >>
				    SCU_HWSTRAP1_CPU_FREQ_SHIFT;

			switch (cpu_freq) {
			case CPU_FREQ_800M_1:
			case CPU_FREQ_800M_2:
			case CPU_FREQ_800M_3:
			case CPU_FREQ_800M_4:
				pll_reg.b.m = 0x5f;
				break;
			case CPU_FREQ_1600M_1:
			case CPU_FREQ_1600M_2:
				pll_reg.b.m = 0xbf;
				break;
			default:
				pll_reg.b.m = 0x8f;
				break;
			}
		}

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
	u32 rate = ast2700_io_get_pll_rate(clk, ASPEED_CLK_HPLL);
	u32 clk_sel2 = readl(&clk->clk_sel2);
	u32 hclk_div = (clk_sel2 & SCU_CLKSEL2_HCLK_DIV_MASK) >>
			     SCU_CLKSEL2_HCLK_DIV_SHIFT;

	if (!hclk_div)
		hclk_div++;

	return (rate / ((hclk_div + 1) * 2));
#endif
}

static uint32_t ast2700_io_get_pclk_rate(struct ast2700_io_clk *clk)
{
#ifdef ASPEED_FPGA
	//hpll/4
	return 12000000;
#else
	u32 rate = ast2700_io_get_hclk_rate(clk);
	u32 clksrc4 = readl(&clk->clksrc4);
	u32 pclk_div = (clksrc4 & SCU_CLKSRC4_PCLK_DIV_MASK) >>
			     SCU_CLKSRC4_PCLK_DIV_SHIFT;

	return (rate / ((pclk_div + 1) * 2));
#endif
}

static uint32_t ast2700_io_get_uart_uxclk_rate(struct ast2700_io_clk *clk)
{
#ifdef ASPEED_FPGA
	return 24000000;
#else
	uint32_t rate = ast2700_get_uxclk_in_rate(clk);
	uint32_t uart_clkgen = readl(&clk->uart_clkgen);
	uint32_t n = (uart_clkgen & SCU_UART_CLKGEN_N_MASK) >>
		      SCU_UART_CLKGEN_N_SHIFT;
	uint32_t r = (uart_clkgen & SCU_UART_CLKGEN_R_MASK) >>
		      SCU_UART_CLKGEN_R_SHIFT;

	return ((rate * r) / (n * 2));
#endif
}

static uint32_t ast2700_io_get_uart_huxclk_rate(struct ast2700_io_clk *clk)
{
#ifdef ASPEED_FPGA
	return 24000000;
#else
	uint32_t rate = ast2700_get_huxclk_in_rate(clk);
	uint32_t huart_clkgen = readl(&clk->huart_clkgen);
	uint32_t n = (huart_clkgen & SCU_HUART_CLKGEN_N_MASK) >>
		      SCU_HUART_CLKGEN_N_SHIFT;
	uint32_t r = (huart_clkgen & SCU_HUART_CLKGEN_R_MASK) >>
		      SCU_HUART_CLKGEN_R_SHIFT;

	return ((rate * r) / (n * 2));
#endif
}

static uint32_t ast2700_io_get_sdio_clk_rate(struct ast2700_io_clk *clk)
{
#ifdef ASPEED_FPGA
	return 50000000;
#else
	uint32_t rate = 0;
	uint32_t clksrc4 = readl(&clk->clksrc4);
	uint32_t sdio_div = (clksrc4 & SCU_CLKSRC4_SDIO_DIV_MASK) >>
			     SCU_CLKSRC4_SDIO_DIV_SHIFT;

	if (clksrc4 & SCU_CLKSRC4_SDIO)
		rate = ast2700_io_get_pll_rate(clk, ASPEED_CLK_APLL);
	else
		rate = ast2700_io_get_hclk_rate(clk);

	return (rate / ((sdio_div + 1) * 2));
#endif
}

static uint32_t
ast2700_io_get_uart_clk_rate(struct ast2700_io_clk *clk, int uart_idx)
{
#ifdef ASPEED_FPGA
	return (24000000 / 13);
#else
	uint32_t rate = 0;
	uint32_t uart5_clk = 0;
	uint32_t clksrc2 = readl(&clk->clksrc2);
	uint32_t clksrc4 = readl(&clk->clksrc4);
	uint32_t clksrc5 = readl(&clk->clksrc5);
	uint32_t misc_ctrl1 = readl(&clk->misc_ctrl1);

	switch (uart_idx) {
	case 1:
	case 2:
	case 3:
	case 4:
	case 6:
		if (clksrc4 & BIT(uart_idx - 1))
			rate = ast2700_get_uart_huxclk_rate(clk);
		else
			rate = ast2700_get_uart_uxclk_rate(clk);
		break;
	case 5:
		/*
		 * SCU0C[12] and SCU304[14] together decide
		 * the UART5 clock generation
		 */
		if (misc_ctrl1 & SCU_MISC_CTRL1_UART5_DIV)
			uart5_clk = 0x1 << 1;

		if (clksrc2 & SCU_CLKSRC2_UART5)
			uart5_clk |= 0x1;

		switch (uart5_clk) {
		case 0:
			rate = 24000000;
			break;
		case 1:
			rate = 192000000;
			break;
		case 2:
			rate = 24000000 / 13;
			break;
		case 3:
			rate = 192000000 / 13;
			break;
		}

		break;
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
		if (clksrc5 & BIT(uart_idx - 1))
			rate = ast2700_get_uart_huxclk_rate(clk);
		else
			rate = ast2700_get_uart_uxclk_rate(clk);
		break;
	}

	return rate;
#endif
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
	case AST2700_IO_CLK_GATE_UART1CLK:
		rate = ast2700_io_get_uart_clk_rate(priv->clk, 1);
		break;
	case AST2700_IO_CLK_GATE_UART2CLK:
		rate = ast2700_io_get_uart_clk_rate(priv->clk, 2);
		break;
	case AST2700_IO_CLK_GATE_UART3CLK:
		rate = ast2700_io_get_uart_clk_rate(priv->clk, 3);
		break;
	case AST2700_IO_CLK_GATE_UART4CLK:
		rate = ast2700_io_get_uart_clk_rate(priv->clk, 4);
		break;
	case AST2700_IO_CLK_SDIO:
		rate = ast2700_io_get_sdio_clk_rate(priv->clk);
		break;
	case AST2700_IO_CLK_UARTX:
		rate = ast2700_io_get_uart_uxclk_rate(priv->clk);
		break;
	case AST2700_IO_CLK_HUARTX:
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
		clkgate_bit = BIT(AST2700_IO_CLK_GATE_MAC0CLK - AST2700_IO_CLK_GATE);
		writel(reset_bit, &clk->modrst_ctrl1);
		udelay(100);
		writel(clkgate_bit, &clk->clkgate_clr1);
		mdelay(10);
		writel(reset_bit, &clk->modrst_clr1);
		break;
	case 1:
		reset_bit = BIT(ASPEED_RESET_MAC1);
		clkgate_bit = BIT(AST2700_IO_CLK_GATE_MAC1CLK - AST2700_IO_CLK_GATE);
		writel(reset_bit, &clk->modrst_ctrl1);
		udelay(100);
		writel(clkgate_bit, &clk->clkgate_clr1);
		mdelay(10);
		writel(reset_bit, &clk->modrst_clr1);
		break;
	case 2:
		reset_bit = BIT(ASPEED_RESET_MAC2);
		clkgate_bit = BIT(AST2700_IO_CLK_GATE_MAC2CLK - AST2700_IO_CLK_GATE);
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
