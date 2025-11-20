// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include "dt-bindings/gpio/aspeed-gpio.h"
#include <common.h>
#include <errno.h>
#include <asm/arch/pinctrl.h>
#include <asm/io.h>
#include <dm.h>
#include <dm/pinctrl.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/bitfield.h>

#define SCU_MULTI_FUNC_SEL_BASE 0x400

/*
 * This driver works with very simple configuration that has the same name
 * for group and function. This way it is compatible with the Linux Kernel
 * driver.
 */
struct ast2700_soc1_sig_desc {
	u32 offset;
	u32 reg_clr;
	u32 reg_set;
};

struct aspeed_group_config {
	char *group_name;
	int ndescs;
	struct ast2700_soc1_sig_desc *descs;
};

struct aspeed_pinctrl_priv {
	void __iomem *base;
};

static int ast2700_soc1_pinctrl_probe(struct udevice *dev)
{
	struct aspeed_pinctrl_priv *priv = dev_get_priv(dev);

	priv->base = dev_remap_addr(dev->parent);
	if (!priv->base)
		return -ENOMEM;

	return 0;
}

static struct ast2700_soc1_sig_desc espi1_link[] = {
	{ 0x458, GENMASK(30, 28) | GENMASK(26, 24) | GENMASK(22, 20) | GENMASK(18, 16) |
	GENMASK(14, 12) | GENMASK(10, 8) | GENMASK(6, 4) | GENMASK(2, 0),
	(2 << 28) | (2 << 24) | (2 << 20) | (2 << 16) | (2 << 12) | (2 << 8) | (2 << 4) | 2},
};

static struct ast2700_soc1_sig_desc sdio_link[] = {
	{ 0x400, GENMASK(30, 28) | GENMASK(26, 24) | GENMASK(22, 20) | GENMASK(18, 16) |
	GENMASK(14, 12) | GENMASK(10, 8) | GENMASK(6, 4) | GENMASK(2, 0),
	(3 << 28) | (3 << 24) | (3 << 20) | (3 << 16) | (3 << 12) | (3 << 8) | (3 << 4) | 3},
};

static struct ast2700_soc1_sig_desc mdio0_link[] = {
	{ 0x448, GENMASK(22, 16), (1 << 20) | (1 << 16) },
};

static struct ast2700_soc1_sig_desc mdio1_link[] = {
	{ 0x450, GENMASK(22, 16), (1 << 20) | (1 << 16) },
};

static struct ast2700_soc1_sig_desc mdio2_link[] = {
	{ 0x440, GENMASK(6, 0), (1 << 4) | 1 },
};

static struct ast2700_soc1_sig_desc rgmii0_link[] = {
	{ 0x444, GENMASK(31, 0),
	  BIT(0) | BIT(4) | BIT(8) | BIT(12) | BIT(16) | BIT(20) | BIT(24) | BIT(28) },
	{ 0x448, GENMASK(14, 0), BIT(0) | BIT(4) | BIT(8) | BIT(12) },
	/* IO Driving */
	{ 0x4D4, GENMASK(23, 0),
	  BIT(0) | BIT(2) | BIT(4) | BIT(6) | BIT(8) | BIT(10) | BIT(12) | BIT(14) | BIT(16) |
		  BIT(18) | BIT(20) | BIT(22) }
};

static struct ast2700_soc1_sig_desc rgmii1_link[] = {
	{ 0x44c, GENMASK(31, 0),
	  BIT(0) | BIT(4) | BIT(8) | BIT(12) | BIT(16) | BIT(20) | BIT(24) | BIT(28) },
	{ 0x450, GENMASK(14, 0), BIT(0) | BIT(4) | BIT(8) | BIT(12) },
	/* IO Driving */
	{ 0x4D8, GENMASK(23, 0),
	  BIT(0) | BIT(2) | BIT(4) | BIT(6) | BIT(8) | BIT(10) | BIT(12) | BIT(14) | BIT(16) |
		  BIT(18) | BIT(20) | BIT(22) }
};

static struct ast2700_soc1_sig_desc rmii0_link[] = {
	{ 0x444,
	  GENMASK(30, 28) | GENMASK(22, 20) | GENMASK(18, 16) |
	  GENMASK(14, 12) | GENMASK(10, 8) | GENMASK(2, 0),
	  BIT(1) | BIT(9) | BIT(13) | BIT(17) | BIT(21) | BIT(29) },
	{ 0x448, GENMASK(6, 0), BIT(1) | BIT(5) },
	/* IO Driving */
	{ 0x4D4, GENMASK(23, 0),
	  BIT(0) | BIT(4) | BIT(6) | BIT(8) | BIT(10) | BIT(12) | BIT(14) | BIT(16) |
		  BIT(18) }
};

static struct ast2700_soc1_sig_desc rmii0rclko_link[] = {
	{ 0x444, GENMASK(26, 24), BIT(25) },
};

static struct ast2700_soc1_sig_desc rmii1_link[] = {
	{ 0x44c,
	  GENMASK(30, 28) | GENMASK(22, 20) | GENMASK(18, 16) |
	  GENMASK(14, 12) | GENMASK(10, 8) | GENMASK(2, 0),
	  BIT(1) | BIT(9) | BIT(13) | BIT(17) | BIT(21) | BIT(29) },
	{ 0x450, GENMASK(6, 0), BIT(1) | BIT(5) },
	/* IO Driving */
	{ 0x4D8, GENMASK(23, 0),
	  BIT(0) | BIT(4) | BIT(6) | BIT(8) | BIT(10) | BIT(12) | BIT(14) | BIT(16) |
		  BIT(18) }
};

static struct ast2700_soc1_sig_desc rmii1rclko_link[] = {
	{ 0x44c, GENMASK(26, 24), BIT(25) },
};

static struct ast2700_soc1_sig_desc sgmii_link[] = {
	{0x7c, BIT(0), BIT(0)},
};

static struct ast2700_soc1_sig_desc spi0_link[] = {
	{ 0x434, GENMASK(2, 0) | GENMASK(6, 4) | GENMASK(10, 8) | GENMASK(14, 12),
		BIT(0) | BIT(4) | BIT(8) | BIT(12)  },
};

static struct ast2700_soc1_sig_desc spi0cs1_link[] = {
	{ 0x434, GENMASK(22, 20), BIT(20) },
};

static struct ast2700_soc1_sig_desc spi0abr_link[] = {
	{ 0x434, GENMASK(26, 24), BIT(24) },
};

static struct ast2700_soc1_sig_desc spi0wp_link[] = {
	{ 0x434, GENMASK(30, 28), BIT(28) },
};

static struct ast2700_soc1_sig_desc spi0quad_link[] = {
	{ 0x434, GENMASK(18, 16) | GENMASK(14, 12), BIT(16) | BIT(12) },
};

static struct ast2700_soc1_sig_desc spi1_link[] = {
	{ 0x438, GENMASK(2, 0) | GENMASK(6, 4) | GENMASK(10, 8) | GENMASK(14, 12),
		BIT(0) | BIT(4) | BIT(8) | BIT(12)  },
};

static struct ast2700_soc1_sig_desc spi1cs1_link[] = {
	{ 0x438, GENMASK(22, 20), BIT(20) },
};

static struct ast2700_soc1_sig_desc spi1abr_link[] = {
	{ 0x438, GENMASK(26, 24), BIT(24) },
};

static struct ast2700_soc1_sig_desc spi1wp_link[] = {
	{ 0x438, GENMASK(30, 28), BIT(28) },
};

static struct ast2700_soc1_sig_desc spi1quad_link[] = {
	{ 0x438, GENMASK(18, 16) | GENMASK(14, 12), BIT(16) | BIT(12) },
};

static struct ast2700_soc1_sig_desc spi2_link[] = {
	{ 0x43c, GENMASK(2, 0) | GENMASK(6, 4) | GENMASK(10, 8) | GENMASK(14, 12),
		BIT(0) | BIT(4) | BIT(8) | BIT(12)  },
};

static struct ast2700_soc1_sig_desc spi2cs1_link[] = {
	{ 0x43c, GENMASK(26, 24), BIT(24) },
};

static struct ast2700_soc1_sig_desc spi2quad_link[] = {
	{ 0x43c, GENMASK(22, 20) | GENMASK(18, 16), BIT(20) | BIT(16) },
};

static struct ast2700_soc1_sig_desc fwspiquad_link[] = {
	{ 0x450, GENMASK(30, 28) | GENMASK(26, 24), BIT(28) | BIT(24) },
};

static struct ast2700_soc1_sig_desc i2c0[] = {
	{ 0x454, GENMASK(6, 4) | GENMASK(2, 0), BIT(4) | BIT(0) },
};

static struct ast2700_soc1_sig_desc i2c1[] = {
	{ 0x454, GENMASK(14, 12) | GENMASK(10, 8), BIT(12) | BIT(8) },
};

static struct ast2700_soc1_sig_desc i2c2[] = {
	{ 0x454, GENMASK(22, 20) | GENMASK(18, 16), BIT(20) | BIT(16) },
};

static struct ast2700_soc1_sig_desc i2c3[] = {
	{ 0x454, GENMASK(30, 28) | GENMASK(26, 24), BIT(28) | BIT(24) },
};

static struct ast2700_soc1_sig_desc i2c4[] = {
	{ 0x458, GENMASK(6, 4) | GENMASK(2, 0), BIT(4) | BIT(0) },
};

static struct ast2700_soc1_sig_desc i2c5[] = {
	{ 0x458, GENMASK(14, 12) | GENMASK(10, 8), BIT(12) | BIT(8) },
};

static struct ast2700_soc1_sig_desc i2c6[] = {
	{ 0x458, GENMASK(22, 20) | GENMASK(18, 16), BIT(20) | BIT(16) },
};

static struct ast2700_soc1_sig_desc i2c7[] = {
	{ 0x458, GENMASK(30, 28) | GENMASK(26, 24), BIT(28) | BIT(24) },
};

static struct ast2700_soc1_sig_desc i2c8[] = {
	{ 0x45c, GENMASK(6, 4) | GENMASK(2, 0), BIT(4) | BIT(0) },
};

static struct ast2700_soc1_sig_desc i2c9[] = {
	{ 0x45c, GENMASK(14, 12) | GENMASK(10, 8), BIT(12) | BIT(8) },
};

static struct ast2700_soc1_sig_desc i2c10[] = {
	{ 0x45c, GENMASK(22, 20) | GENMASK(18, 16), BIT(20) | BIT(16) },
};

static struct ast2700_soc1_sig_desc i2c11[] = {
	{ 0x45c, GENMASK(30, 28) | GENMASK(26, 24), BIT(28) | BIT(24) },
};

static struct ast2700_soc1_sig_desc i2c12[] = {
	{ 0x418, GENMASK(14, 12) | GENMASK(6, 4), BIT(14) | BIT(6) },
};

static struct ast2700_soc1_sig_desc i2c13[] = {
	{ 0x418, GENMASK(22, 20) | GENMASK(18, 16), BIT(22) | BIT(18) },
};

static struct ast2700_soc1_sig_desc i2c14[] = {
	{ 0x418, GENMASK(30, 28) | GENMASK(26, 24), BIT(30) | BIT(26) },
};

static struct ast2700_soc1_sig_desc i2c15[] = {
	{ 0x41c, GENMASK(6, 4) | GENMASK(2, 0), BIT(5) | BIT(1) },
};

static struct ast2700_soc1_sig_desc di2c0[] = {
	{ 0x40, GENMASK(6, 4) | GENMASK(2, 0), BIT(6) | BIT(2) },
};

static struct ast2700_soc1_sig_desc di2c1[] = {
	{ 0x40, GENMASK(14, 12) | GENMASK(10, 8), BIT(14) | BIT(10) },
};

static struct ast2700_soc1_sig_desc di2c2[] = {
	{ 0x40, GENMASK(22, 20) | GENMASK(18, 16), BIT(22) | BIT(18) },
};

static struct ast2700_soc1_sig_desc di2c3[] = {
	{ 0x40, GENMASK(30, 28) | GENMASK(26, 24), BIT(30) | BIT(26) },
};

static struct ast2700_soc1_sig_desc di2c8[] = {
	{ 0x42c, GENMASK(6, 4) | GENMASK(2, 0), BIT(5) | BIT(1) },
};

static struct ast2700_soc1_sig_desc di2c9[] = {
	{ 0x42c, GENMASK(14, 12) | GENMASK(10, 8), BIT(13) | BIT(9) },
};

static struct ast2700_soc1_sig_desc di2c10[] = {
	{ 0x42c, GENMASK(22, 20) | GENMASK(18, 16), BIT(21) | BIT(17) },
};

static struct ast2700_soc1_sig_desc di2c11[] = {
	{ 0x42c, GENMASK(30, 28) | GENMASK(26, 24), BIT(29) | BIT(25) },
};

static struct ast2700_soc1_sig_desc di2c12[] = {
	{ 0x420, GENMASK(6, 4) | GENMASK(2, 0), BIT(5) | BIT(1) },
};

static struct ast2700_soc1_sig_desc di2c13[] = {
	{ 0x420, GENMASK(14, 12) | GENMASK(10, 8), BIT(13) | BIT(9) },
};

static struct ast2700_soc1_sig_desc di2c14[] = {
	{ 0x420, GENMASK(22, 20) | GENMASK(18, 16), BIT(21) | BIT(17) },
};

static struct ast2700_soc1_sig_desc di2c15[] = {
	{ 0x420, GENMASK(30, 28) | GENMASK(26, 24), BIT(29) | BIT(25) },
};

static struct ast2700_soc1_sig_desc pwm0[] = {
	{ 0x40c, GENMASK(2, 0), BIT(0) },
};

static struct ast2700_soc1_sig_desc pwm1[] = {
	{ 0x40c, GENMASK(6, 4), BIT(4) },
};

static struct ast2700_soc1_sig_desc pwm2[] = {
	{ 0x40c, GENMASK(10, 8), BIT(8) },
};

static struct ast2700_soc1_sig_desc pwm3[] = {
	{ 0x40c, GENMASK(14, 12), BIT(12) },
};

static struct ast2700_soc1_sig_desc pwm4[] = {
	{ 0x40c, GENMASK(18, 16), BIT(16) },
};

static struct ast2700_soc1_sig_desc pwm5[] = {
	{ 0x40c, GENMASK(22, 20), BIT(20) },
};

static struct ast2700_soc1_sig_desc pwm6[] = {
	{ 0x40c, GENMASK(26, 24), BIT(24) },
};

static struct ast2700_soc1_sig_desc pwm7[] = {
	{ 0x40c, GENMASK(30, 28), BIT(28) },
};

static struct ast2700_soc1_sig_desc mac0_link[] = {
	{ 0x468, GENMASK(2, 0), BIT(2) },
};

static struct ast2700_soc1_sig_desc mac1_link[] = {
	{ 0x468, GENMASK(30, 28), BIT(28) | BIT(29) },
};

static struct ast2700_soc1_sig_desc mac2_link[] = {
	{ 0x468, GENMASK(6, 4), BIT(6) },
};

static struct ast2700_soc1_sig_desc pcie2_perst[] = {
	{ 0x440, GENMASK(2, 0), 2 },
};

static struct ast2700_soc1_sig_desc usb2cud_link[] = {
	{ 0x3B0, GENMASK(1, 0), 0 },
};

static struct ast2700_soc1_sig_desc usb2cd_link[] = {
	{ 0x3B0, GENMASK(1, 0), 1 },
};

static struct ast2700_soc1_sig_desc usb2ch_link[] = {
	{ 0x3B0, GENMASK(1, 0), 2 },
};

static struct ast2700_soc1_sig_desc usb2cu_link[] = {
	{ 0x3B0, GENMASK(1, 0), 3 },
};

static struct ast2700_soc1_sig_desc usb2dd_link[] = {
	{ 0x3B0, GENMASK(3, 2), 1 << 2 },
};

static struct ast2700_soc1_sig_desc usb2dh_link[] = {
	{ 0x3B0, GENMASK(3, 2), 2 << 2 },
};

static const struct aspeed_group_config ast2700_soc1_groups[] = {
	{ "ESPI1", ARRAY_SIZE(espi1_link), espi1_link },
	{ "SD", ARRAY_SIZE(sdio_link), sdio_link },
	{ "MDIO0", ARRAY_SIZE(mdio0_link), mdio0_link },
	{ "MDIO1", ARRAY_SIZE(mdio1_link), mdio1_link },
	{ "MDIO2", ARRAY_SIZE(mdio2_link), mdio2_link },
	{ "RGMII0", ARRAY_SIZE(rgmii0_link), rgmii0_link },
	{ "RGMII1", ARRAY_SIZE(rgmii1_link), rgmii1_link },
	{ "RMII0", ARRAY_SIZE(rmii0_link), rmii0_link },
	{ "RMII0RCLKO", ARRAY_SIZE(rmii0rclko_link), rmii0rclko_link },
	{ "RMII1", ARRAY_SIZE(rmii1_link), rmii1_link },
	{ "RMII1RCLKO", ARRAY_SIZE(rmii1rclko_link), rmii1rclko_link },
	{ "SGMII", ARRAY_SIZE(sgmii_link), sgmii_link },
	{ "FWSPIQUAD", ARRAY_SIZE(fwspiquad_link), fwspiquad_link },
	{ "SPI0", ARRAY_SIZE(spi0_link), spi0_link },
	{ "SPI0ABR", ARRAY_SIZE(spi0abr_link), spi0abr_link },
	{ "SPI0CS1", ARRAY_SIZE(spi0cs1_link), spi0cs1_link },
	{ "SPI0WP", ARRAY_SIZE(spi0wp_link), spi0wp_link },
	{ "SPI0QUAD", ARRAY_SIZE(spi0quad_link), spi0quad_link },
	{ "SPI1", ARRAY_SIZE(spi1_link), spi1_link },
	{ "SPI1ABR", ARRAY_SIZE(spi1abr_link), spi1abr_link },
	{ "SPI1CS1", ARRAY_SIZE(spi1cs1_link), spi1cs1_link },
	{ "SPI1WP", ARRAY_SIZE(spi1wp_link), spi1wp_link },
	{ "SPI1QUAD", ARRAY_SIZE(spi1quad_link), spi1quad_link },
	{ "SPI2", ARRAY_SIZE(spi2_link), spi2_link },
	{ "SPI2CS1", ARRAY_SIZE(spi2cs1_link), spi2cs1_link },
	{ "SPI2QUAD", ARRAY_SIZE(spi2quad_link), spi2quad_link },
	{ "I2C0", ARRAY_SIZE(i2c0), i2c0 },
	{ "I2C1", ARRAY_SIZE(i2c1), i2c1 },
	{ "I2C2", ARRAY_SIZE(i2c2), i2c2 },
	{ "I2C3", ARRAY_SIZE(i2c3), i2c3 },
	{ "I2C4", ARRAY_SIZE(i2c4), i2c4 },
	{ "I2C5", ARRAY_SIZE(i2c5), i2c5 },
	{ "I2C6", ARRAY_SIZE(i2c6), i2c6 },
	{ "I2C7", ARRAY_SIZE(i2c7), i2c7 },
	{ "I2C8", ARRAY_SIZE(i2c8), i2c8 },
	{ "I2C9", ARRAY_SIZE(i2c9), i2c9 },
	{ "I2C10", ARRAY_SIZE(i2c10), i2c10 },
	{ "I2C11", ARRAY_SIZE(i2c11), i2c11 },
	{ "I2C12", ARRAY_SIZE(i2c12), i2c12 },
	{ "I2C13", ARRAY_SIZE(i2c13), i2c13 },
	{ "I2C14", ARRAY_SIZE(i2c14), i2c14 },
	{ "I2C15", ARRAY_SIZE(i2c15), i2c15 },
	{ "DI2C0", ARRAY_SIZE(di2c0), di2c0 },
	{ "DI2C1", ARRAY_SIZE(di2c1), di2c1 },
	{ "DI2C2", ARRAY_SIZE(di2c2), di2c2 },
	{ "DI2C3", ARRAY_SIZE(di2c3), di2c3 },
	{ "DI2C8", ARRAY_SIZE(di2c8), di2c8 },
	{ "DI2C9", ARRAY_SIZE(di2c9), di2c9 },
	{ "DI2C10", ARRAY_SIZE(di2c10), di2c10 },
	{ "DI2C11", ARRAY_SIZE(di2c11), di2c11 },
	{ "DI2C12", ARRAY_SIZE(di2c12), di2c12 },
	{ "DI2C13", ARRAY_SIZE(di2c13), di2c13 },
	{ "DI2C14", ARRAY_SIZE(di2c14), di2c14 },
	{ "DI2C15", ARRAY_SIZE(di2c15), di2c15 },
	{ "PWM0", ARRAY_SIZE(pwm0), pwm0 },
	{ "PWM1", ARRAY_SIZE(pwm1), pwm1 },
	{ "PWM2", ARRAY_SIZE(pwm2), pwm2 },
	{ "PWM3", ARRAY_SIZE(pwm3), pwm3 },
	{ "PWM4", ARRAY_SIZE(pwm4), pwm4 },
	{ "PWM5", ARRAY_SIZE(pwm5), pwm5 },
	{ "PWM6", ARRAY_SIZE(pwm6), pwm6 },
	{ "PWM7", ARRAY_SIZE(pwm7), pwm7 },
	{ "MAC0LINK", ARRAY_SIZE(mac0_link), mac0_link },
	{ "MAC1LINK", ARRAY_SIZE(mac1_link), mac1_link },
	{ "MAC2LINK", ARRAY_SIZE(mac2_link), mac2_link },
	{ "PCIE2PERST", ARRAY_SIZE(pcie2_perst), pcie2_perst },
	{ "USB2CUD", ARRAY_SIZE(usb2cud_link), usb2cud_link },
	{ "USB2CD", ARRAY_SIZE(usb2cd_link), usb2cd_link },
	{ "USB2CH", ARRAY_SIZE(usb2ch_link), usb2ch_link },
	{ "USB2CU", ARRAY_SIZE(usb2cu_link), usb2cu_link },
	{ "USB2DD", ARRAY_SIZE(usb2dd_link), usb2dd_link },
	{ "USB2DH", ARRAY_SIZE(usb2dh_link), usb2dh_link },
};

static int ast2700_soc1_pinctrl_get_groups_count(struct udevice *dev)
{
	debug("PINCTRL: get_(functions/groups)_count\n");

	return ARRAY_SIZE(ast2700_soc1_groups);
}

static const char *ast2700_soc1_pinctrl_get_group_name(struct udevice *dev,
						       unsigned int selector)
{
	debug("PINCTRL: get_(function/group)_name %u\n", selector);

	return ast2700_soc1_groups[selector].group_name;
}

static int ast2700_soc1_pinctrl_group_set(struct udevice *dev,
					  unsigned int selector, unsigned int func_selector)
{
	struct aspeed_pinctrl_priv *priv = dev_get_priv(dev);
	const struct aspeed_group_config *config;
	const struct ast2700_soc1_sig_desc *descs;
	u32 i;

	debug("PINCTRL: group_set <%u, %u>\n", selector, func_selector);
	if (selector >= ARRAY_SIZE(ast2700_soc1_groups))
		return -EINVAL;

	config = &ast2700_soc1_groups[selector];
	for (i = 0; i < config->ndescs; i++) {
		descs = &config->descs[i];
		clrsetbits_le32(priv->base + descs->offset, descs->reg_clr, descs->reg_set);
	}

	return 0;
}

static int ast2700_soc1_pinctrl_gpio_request_enable(struct udevice *dev, unsigned int selector)
{
	struct aspeed_pinctrl_priv *priv = dev_get_priv(dev);
	u32 offset = SCU_MULTI_FUNC_SEL_BASE;
	u32 reg = offset + (selector / 8) * 4;
	u32 shift = (selector % 8) * 4;
	u32 mask = 0x7 << shift;
	u32 val;

	/* GPIO function is 0, Expect GPIOY and GPIOZ */
	if (selector >= ASPEED_GPIO(Y, 0) && selector <= ASPEED_GPIO(Z, 7))
		val = 1 << shift;
	else
		val = 0 << shift;

	clrsetbits_le32(priv->base + reg, mask, val);

	return 0;
}

static struct pinctrl_ops ast2700_soc1_pinctrl_ops = {
	.set_state = pinctrl_generic_set_state,
	.get_groups_count = ast2700_soc1_pinctrl_get_groups_count,
	.get_group_name = ast2700_soc1_pinctrl_get_group_name,
	.get_functions_count = ast2700_soc1_pinctrl_get_groups_count,
	.get_function_name = ast2700_soc1_pinctrl_get_group_name,
	.pinmux_group_set = ast2700_soc1_pinctrl_group_set,
	.gpio_request_enable = ast2700_soc1_pinctrl_gpio_request_enable,
};

static const struct udevice_id ast2700_soc1_pinctrl_ids[] = {
	{ .compatible = "aspeed,ast2700-soc1-pinctrl" },
	{ }
};

U_BOOT_DRIVER(pinctrl_ast2700_soc1) = {
	.name = "aspeed_ast2700_soc1_pinctrl",
	.id = UCLASS_PINCTRL,
	.of_match = ast2700_soc1_pinctrl_ids,
	.priv_auto = sizeof(struct aspeed_pinctrl_priv),
	.ops = &ast2700_soc1_pinctrl_ops,
	.probe = ast2700_soc1_pinctrl_probe,
};
