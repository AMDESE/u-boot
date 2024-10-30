// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include <clk-uclass.h>
#include <dm.h>
#include <asm/io.h>
#include <dm/lists.h>
#include <linux/delay.h>
#include <linux/bitfield.h>
#include <asm/arch-aspeed/scu_ast2700.h>
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
#define RGMII_DEFAULT_CLK_SRC	SCU1_CLK_HPLL

/* MAC Clock Delay settings */
#define MAC01_DEF_DELAY_1G		0x00CF4D75
#define MAC01_DEF_DELAY_100M		0x00410410
#define MAC01_DEF_DELAY_10M		0x00410410

/*
 * MAC Clock Delay settings
 */
#define MAC_CLK_RMII1_50M_RCLK_O_CTRL		BIT(30)
#define   MAC_CLK_RMII1_50M_RCLK_O_DIS		0
#define   MAC_CLK_RMII1_50M_RCLK_O_EN		1
#define MAC_CLK_RMII0_50M_RCLK_O_CTRL		BIT(29)
#define   MAC_CLK_RMII0_5M_RCLK_O_DIS		0
#define   MAC_CLK_RMII0_5M_RCLK_O_EN		1
#define MAC_CLK_RMII_TXD_FALLING_2		BIT(27)
#define MAC_CLK_RMII_TXD_FALLING_1		BIT(26)
#define MAC_CLK_RXCLK_INV_2			BIT(25)
#define MAC_CLK_RXCLK_INV_1			BIT(24)
#define MAC_CLK_1G_INPUT_DELAY_2		GENMASK(23, 18)
#define MAC_CLK_1G_INPUT_DELAY_1		GENMASK(17, 12)
#define MAC_CLK_1G_OUTPUT_DELAY_2		GENMASK(11, 6)
#define MAC_CLK_1G_OUTPUT_DELAY_1		GENMASK(5, 0)

#define MAC_CLK_100M_10M_RESERVED		GENMASK(31, 26)
#define MAC_CLK_100M_10M_RXCLK_INV_2		BIT(25)
#define MAC_CLK_100M_10M_RXCLK_INV_1		BIT(24)
#define MAC_CLK_100M_10M_INPUT_DELAY_2		GENMASK(23, 18)
#define MAC_CLK_100M_10M_INPUT_DELAY_1		GENMASK(17, 12)
#define MAC_CLK_100M_10M_OUTPUT_DELAY_2		GENMASK(11, 6)
#define MAC_CLK_100M_10M_OUTPUT_DELAY_1		GENMASK(5, 0)

struct mac_delay_config {
	u32 tx_delay_1000;
	u32 rx_delay_1000;
	u32 tx_delay_100;
	u32 rx_delay_100;
	u32 tx_delay_10;
	u32 rx_delay_10;
};

struct ast2700_soc1_clk_priv {
	struct ast2700_scu1 *scu;
};

static uint32_t ast2700_soc1_get_pll_rate(struct ast2700_scu1 *scu, int pll_idx)
{
	union ast2700_pll_reg pll_reg;
	uint32_t mul = 1, div = 1;

	switch (pll_idx) {
	case SCU1_CLK_HPLL:
		pll_reg.w = readl(&scu->hpll);
		break;
	case SCU1_CLK_APLL:
		pll_reg.w = readl(&scu->apll);
		break;
	case SCU1_CLK_DPLL:
		pll_reg.w = readl(&scu->dpll);
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

static uint32_t ast2700_soc1_get_hclk_rate(struct ast2700_scu1 *scu)
{
	u32 rate = ast2700_soc1_get_pll_rate(scu, SCU1_CLK_HPLL);
	u32 clk_sel2 = readl(&scu->clk_sel2);
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

static uint32_t ast2700_soc1_get_pclk_rate(struct ast2700_scu1 *scu)
{
	u32 rate = ast2700_soc1_get_pll_rate(scu, SCU1_CLK_HPLL);

	u32 clk_sel1 = readl(&scu->clk_sel1);
	u32 pclk_div = (clk_sel1 & SCU_CLKSEL1_PCLK_DIV_MASK) >>
			     SCU_CLKSEL1_PCLK_DIV_SHIFT;

	return (rate / ((pclk_div + 1) * 2));
}

#define SCU_UART_CLKGEN_N_MASK			GENMASK(17, 8)
#define SCU_UART_CLKGEN_N_SHIFT			8
#define SCU_UART_CLKGEN_R_MASK			GENMASK(7, 0)
#define SCU_UART_CLKGEN_R_SHIFT			0

static uint32_t ast2700_soc1_get_uart_uxclk_rate(struct ast2700_scu1 *scu)
{
	u32 uxclk_sel = readl(&scu->clk_sel2) & GENMASK(1, 0);
	u32 uxclk_ctrl = readl(&scu->uxclk_ctrl);
	u32 rate;

	switch (uxclk_sel) {
	case 0:
		rate = ast2700_soc1_get_pll_rate(scu, SCU1_CLK_APLL) / 4;
		break;
	case 1:
		rate = ast2700_soc1_get_pll_rate(scu, SCU1_CLK_APLL) / 2;
		break;
	case 2:
		rate = ast2700_soc1_get_pll_rate(scu, SCU1_CLK_APLL);
		break;
	case 3:
		rate = ast2700_soc1_get_pll_rate(scu, SCU1_CLK_HPLL);
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

static uint32_t ast2700_soc1_get_uart_huxclk_rate(struct ast2700_scu1 *scu)
{
	u32 huxclk_sel = (readl(&scu->clk_sel2) & GENMASK(4, 3)) >> 3;
	u32 huxclk_ctrl = readl(&scu->huxclk_ctrl);
	uint32_t n = (huxclk_ctrl & SCU_HUART_CLKGEN_N_MASK) >>
		      SCU_HUART_CLKGEN_N_SHIFT;
	uint32_t r = (huxclk_ctrl & SCU_HUART_CLKGEN_R_MASK) >>
		      SCU_HUART_CLKGEN_R_SHIFT;
	u32 rate;

	switch (huxclk_sel) {
	case 0:
		rate = ast2700_soc1_get_pll_rate(scu, SCU1_CLK_APLL) / 4;
		break;
	case 1:
		rate = ast2700_soc1_get_pll_rate(scu, SCU1_CLK_APLL) / 2;
		break;
	case 2:
		rate = ast2700_soc1_get_pll_rate(scu, SCU1_CLK_APLL);
		break;
	case 3:
		rate = ast2700_soc1_get_pll_rate(scu, SCU1_CLK_HPLL);
		break;
	}

	return ((rate * r) / (n * 2));
}

#define SCU_CLKSRC4_SDIO_DIV_MASK		GENMASK(16, 14)
#define SCU_CLKSRC4_SDIO_DIV_SHIFT		14

static uint32_t ast2700_soc1_get_sdio_clk_rate(struct ast2700_scu1 *scu)
{
	uint32_t rate = 0;
	uint32_t clk_sel1 = readl(&scu->clk_sel1);
	uint32_t div = (clk_sel1 & SCU_CLKSRC4_SDIO_DIV_MASK) >>
			     SCU_CLKSRC4_SDIO_DIV_SHIFT;

	if (clk_sel1 & BIT(13))
		rate = ast2700_soc1_get_pll_rate(scu, SCU1_CLK_APLL);
	else
		rate = ast2700_soc1_get_pll_rate(scu, SCU1_CLK_HPLL);

	if (!div)
		div = 1;

	div++;

	return (rate / div);
}

static uint32_t
ast2700_soc1_get_uart_clk_rate(struct ast2700_scu1 *scu, int uart_idx)
{
	uint32_t rate = 0;

	if (readl(&scu->clk_sel1) & BIT(uart_idx))
		rate = ast2700_soc1_get_uart_huxclk_rate(scu);
	else
		rate = ast2700_soc1_get_uart_uxclk_rate(scu);

	return rate;
}

static void ast2700_init_mac_clk(struct udevice *dev)
{
	struct ast2700_soc1_clk_priv *priv = dev_get_priv(dev);
	uint32_t src_clk = ast2700_soc1_get_pll_rate(priv->scu, SCU1_CLK_HPLL);
	uint32_t reg_280;
	uint8_t div_idx;

	/* The MAC source clock selects HPLL only, and the default clock
	 * setting is 200 Mhz.
	 * Calculate the corresponding divider:
	 * 1: div 2
	 * 2: div 3
	 * ...
	 * 7: div 8
	 */
	for (div_idx = 1; div_idx <= 7; div_idx++)
		if (DIV_ROUND_UP(src_clk, div_idx + 1) == 200000000)
			break;

	if (div_idx == 8) {
		dev_err(dev, "Error: MAC clock cannot divide to 200 MHz\n");
		return;
	}

	/* set HPLL clock divider */
	reg_280 = readl(&priv->scu->clk_sel1);
	reg_280 &= ~GENMASK(31, 29);
	reg_280 |= div_idx << 29;
	writel(reg_280, &priv->scu->clk_sel1);
}

static void ast2700_init_rgmii_clk(struct udevice *dev)
{
	struct ast2700_soc1_clk_priv *priv = dev_get_priv(dev);
	uint32_t reg_284 = readl(&priv->scu->clk_sel2);
	uint32_t src_clk = ast2700_soc1_get_pll_rate(priv->scu, RGMII_DEFAULT_CLK_SRC);

	if (RGMII_DEFAULT_CLK_SRC == SCU1_CLK_HPLL) {
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
		reg_280 = readl(&priv->scu->clk_sel1);
		reg_280 &= ~GENMASK(27, 25);
		reg_280 |= div_idx << 25;
		writel(reg_280, &priv->scu->clk_sel1);

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

	writel(reg_284, &priv->scu->clk_sel2);
}

static void ast2700_init_rmii_clk(struct udevice *dev)
{
	struct ast2700_soc1_clk_priv *priv = dev_get_priv(dev);
	uint32_t src_clk = ast2700_soc1_get_pll_rate(priv->scu, SCU1_CLK_HPLL);
	uint32_t reg_280;
	uint8_t div_idx;

	/* The RMII source clock selects HPLL only.
	 * Calculate the corresponding divider:
	 * 1: div 8
	 * 2: div 12
	 * ...
	 * 7: div 32
	 */
	for (div_idx = 1; div_idx <= 7; div_idx++) {
		uint8_t div = 8 + 4 * (div_idx - 1);

		if (DIV_ROUND_UP(src_clk, div) == 50000000)
			break;
	}
	if (div_idx == 8) {
		dev_err(dev, "Error: RMII using HPLL cannot divide to 50 MHz\n");
		return;
	}

	/* set RMII clock divider */
	reg_280 = readl(&priv->scu->clk_sel1);
	reg_280 &= ~GENMASK(23, 21);
	reg_280 |= div_idx << 21;
	writel(reg_280, &priv->scu->clk_sel1);
}

static void ast2700_configure_mac01_clk(struct udevice *dev)
{
	struct ast2700_soc1_clk_priv *priv = dev_get_priv(dev);
	struct ast2700_scu1 *scu = priv->scu;
	struct mac_delay_config mac1_cfg, mac2_cfg;
	u32 reg[3];
	int ret;

	reg[0] = MAC01_DEF_DELAY_1G;
	reg[1] = MAC01_DEF_DELAY_100M;
	reg[2] = MAC01_DEF_DELAY_10M;

	ret = dev_read_u32_array(dev, "mac0-clk-delay", (u32 *)&mac1_cfg,
				 sizeof(mac1_cfg) / sizeof(u32));
	if (!ret) {
		reg[0] &= ~(MAC_CLK_1G_INPUT_DELAY_1 | MAC_CLK_1G_OUTPUT_DELAY_1);
		reg[0] |= FIELD_PREP(MAC_CLK_1G_INPUT_DELAY_1, mac1_cfg.rx_delay_1000) |
			  FIELD_PREP(MAC_CLK_1G_OUTPUT_DELAY_1, mac1_cfg.tx_delay_1000);

		reg[1] &= ~(MAC_CLK_100M_10M_INPUT_DELAY_1 | MAC_CLK_100M_10M_OUTPUT_DELAY_1);
		reg[1] |= FIELD_PREP(MAC_CLK_100M_10M_INPUT_DELAY_1, mac1_cfg.rx_delay_100) |
			  FIELD_PREP(MAC_CLK_100M_10M_OUTPUT_DELAY_1, mac1_cfg.tx_delay_100);

		reg[2] &= ~(MAC_CLK_100M_10M_INPUT_DELAY_1 | MAC_CLK_100M_10M_OUTPUT_DELAY_1);
		reg[2] |= FIELD_PREP(MAC_CLK_100M_10M_INPUT_DELAY_1, mac1_cfg.rx_delay_10) |
			  FIELD_PREP(MAC_CLK_100M_10M_OUTPUT_DELAY_1, mac1_cfg.tx_delay_10);
	}

	ret = dev_read_u32_array(dev, "mac1-clk-delay", (u32 *)&mac2_cfg,
				 sizeof(mac2_cfg) / sizeof(u32));
	if (!ret) {
		reg[0] &= ~(MAC_CLK_1G_INPUT_DELAY_2 | MAC_CLK_1G_OUTPUT_DELAY_2);
		reg[0] |= FIELD_PREP(MAC_CLK_1G_INPUT_DELAY_2, mac2_cfg.rx_delay_1000) |
			  FIELD_PREP(MAC_CLK_1G_OUTPUT_DELAY_2, mac2_cfg.tx_delay_1000);

		reg[1] &= ~(MAC_CLK_100M_10M_INPUT_DELAY_2 | MAC_CLK_100M_10M_OUTPUT_DELAY_2);
		reg[1] |= FIELD_PREP(MAC_CLK_100M_10M_INPUT_DELAY_2, mac2_cfg.rx_delay_100) |
			  FIELD_PREP(MAC_CLK_100M_10M_OUTPUT_DELAY_2, mac2_cfg.tx_delay_100);

		reg[2] &= ~(MAC_CLK_100M_10M_INPUT_DELAY_2 | MAC_CLK_100M_10M_OUTPUT_DELAY_2);
		reg[2] |= FIELD_PREP(MAC_CLK_100M_10M_INPUT_DELAY_2, mac2_cfg.rx_delay_10) |
			  FIELD_PREP(MAC_CLK_100M_10M_OUTPUT_DELAY_2, mac2_cfg.tx_delay_10);
	}

	reg[0] |= (readl(&scu->mac_delay) & ~GENMASK(25, 0));
	writel(reg[0], &scu->mac_delay);
	writel(reg[1], &scu->mac_100m_delay);
	writel(reg[2], &scu->mac_10m_delay);
}

static void ast2700_init_mac(struct udevice *dev)
{
	ast2700_init_mac_clk(dev);
	ast2700_init_rgmii_clk(dev);
	ast2700_init_rmii_clk(dev);
	ast2700_configure_mac01_clk(dev);
}

#define SCU_CLKSRC1_SD_DIV_SEL_MASK	GENMASK(17, 13)
#define SCU_CLKSRC1_SD_DIV_SHIFT	14
const int ast2700_sd_div_tbl[] = {
	2, 2, 3, 4, 5, 6, 7, 8
};

static void ast2700_init_sdclk(struct udevice *dev)
{
	struct ast2700_soc1_clk_priv *priv = dev_get_priv(dev);
	uint32_t src_clk = ast2700_soc1_get_pll_rate(priv->scu, SCU1_CLK_HPLL);
	uint32_t reg_280;
	int i;

	for (i = 0; i < 8; i++) {
		if (src_clk / ast2700_sd_div_tbl[i] <= 200000000)
			break;
	}

	reg_280 = readl(&priv->scu->clk_sel1);
	reg_280 &= ~SCU_CLKSRC1_SD_DIV_SEL_MASK;
	reg_280 |= i << SCU_CLKSRC1_SD_DIV_SHIFT;
	writel(reg_280, &priv->scu->clk_sel1);
}

static ulong ast2700_soc1_clk_get_rate(struct clk *clk)
{
	struct ast2700_soc1_clk_priv *priv = dev_get_priv(clk->dev);
	ulong rate = 0;

	switch (clk->id) {
	case SCU1_CLK_HPLL:
	case SCU1_CLK_APLL:
	case SCU1_CLK_DPLL:
		rate = ast2700_soc1_get_pll_rate(priv->scu, clk->id);
		break;
	case SCU1_CLK_AHB:
		rate = ast2700_soc1_get_hclk_rate(priv->scu);
		break;
	case SCU1_CLK_APB:
		rate = ast2700_soc1_get_pclk_rate(priv->scu);
		break;
	case SCU1_CLK_GATE_UART0CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->scu, 0);
		break;
	case SCU1_CLK_GATE_UART1CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->scu, 1);
		break;
	case SCU1_CLK_GATE_UART2CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->scu, 2);
		break;
	case SCU1_CLK_GATE_UART3CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->scu, 3);
		break;
	case SCU1_CLK_GATE_UART5CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->scu, 5);
		break;
	case SCU1_CLK_GATE_UART6CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->scu, 6);
		break;
	case SCU1_CLK_GATE_UART7CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->scu, 7);
		break;
	case SCU1_CLK_GATE_UART8CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->scu, 8);
		break;
	case SCU1_CLK_GATE_UART9CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->scu, 9);
		break;
	case SCU1_CLK_GATE_UART10CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->scu, 10);
		break;
	case SCU1_CLK_GATE_UART11CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->scu, 11);
		break;
	case SCU1_CLK_GATE_UART12CLK:
		rate = ast2700_soc1_get_uart_clk_rate(priv->scu, 12);
		break;
	case SCU1_CLK_GATE_SDCLK:
		rate = ast2700_soc1_get_sdio_clk_rate(priv->scu);
		break;
	case SCU1_CLK_UXCLK:
		rate = ast2700_soc1_get_uart_uxclk_rate(priv->scu);
		break;
	case SCU1_CLK_HUXCLK:
		rate = ast2700_soc1_get_uart_huxclk_rate(priv->scu);
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
	struct ast2700_scu1 *scu = priv->scu;
	u32 clkgate_bit;

	if (clk->id > 32)
		clkgate_bit = BIT(clk->id - 32);
	else
		clkgate_bit = BIT(clk->id);

	writel(clkgate_bit, &scu->clkgate_clr1);

	return 0;
}

struct clk_ops ast2700_soc1_clk_ops = {
	.get_rate = ast2700_soc1_clk_get_rate,
	.enable = ast2700_soc1_clk_enable,
};

static int ast2700_soc1_clk_probe(struct udevice *dev)
{
	struct ast2700_soc1_clk_priv *priv = dev_get_priv(dev);

	priv->scu = dev_read_addr_ptr(dev->parent);
	if (IS_ERR(priv->scu))
		return PTR_ERR(priv->scu);

	ast2700_init_mac(dev);
	ast2700_init_sdclk(dev);

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
