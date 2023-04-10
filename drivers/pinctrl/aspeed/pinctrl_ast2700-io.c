// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include <common.h>
#include <errno.h>
#include <asm/arch/pinctrl.h>
#include <asm/io.h>
#include <dm.h>
#include <dm/pinctrl.h>
#include <linux/bitops.h>
#include <linux/err.h>

/*
 * This driver works with very simple configuration that has the same name
 * for group and function. This way it is compatible with the Linux Kernel
 * driver.
 */
struct aspeed_sig_desc {
	u32 offset;
	u32 reg_clr;
	u32 reg_set;
};

struct aspeed_group_config {
	char *group_name;
	int ndescs;
	struct aspeed_sig_desc *descs;
};

struct aspeed_pinctrl_priv {
	void __iomem *base;
};

static int ast2700_io_pinctrl_probe(struct udevice *dev)
{
	struct aspeed_pinctrl_priv *priv = dev_get_priv(dev);

	priv->base = dev_remap_addr(dev);
	if (!priv->base)
		return -ENOMEM;

	return 0;
}

static struct aspeed_sig_desc mdio0_link[] = {
	{ 0x48, GENMASK(18, 16) | GENMASK(22, 20), BIT(16) | BIT(20) },
};

static struct aspeed_sig_desc mdio1_link[] = {
	{ 0x50, GENMASK(18, 16) | GENMASK(22, 20), BIT(16) | BIT(20) },
};

static struct aspeed_sig_desc mdio2_link[] = {
	{ 0x40, GENMASK(2, 0) | GENMASK(6, 4), BIT(0) | BIT(4) },
};

static struct aspeed_sig_desc spi0_link[] = {
	{ 0x34, GENMASK(2, 0) | GENMASK(6, 4) | GENMASK(10, 8) | GENMASK(14, 12),
		BIT(0) | BIT(4) | BIT(8) | BIT(12)  },
};

static struct aspeed_sig_desc spi0cs1_link[] = {
	{ 0x34, GENMASK(22, 20), BIT(20) },
};

static struct aspeed_sig_desc spi0abr_link[] = {
	{ 0x34, GENMASK(26, 24), BIT(24) },
};

static struct aspeed_sig_desc spi0wp_link[] = {
	{ 0x34, GENMASK(30, 28), BIT(28) },
};

static struct aspeed_sig_desc spi0quad_link[] = {
	{ 0x34, GENMASK(15, 14), 0 },
};

static struct aspeed_sig_desc spi1_link[] = {
	{ 0x38, GENMASK(2, 0) | GENMASK(6, 4) | GENMASK(10, 8) | GENMASK(14, 12),
		BIT(0) | BIT(4) | BIT(8) | BIT(12)  },
};

static struct aspeed_sig_desc spi1cs1_link[] = {
	{ 0x38, GENMASK(22, 20), BIT(20) },
};

static struct aspeed_sig_desc spi1abr_link[] = {
	{ 0x38, GENMASK(26, 24), BIT(24) },
};

static struct aspeed_sig_desc spi1wp_link[] = {
	{ 0x38, GENMASK(30, 28), BIT(28) },
};

static struct aspeed_sig_desc spi1quad_link[] = {
	{ 0x38, GENMASK(15, 14), 0 },
};

static struct aspeed_sig_desc pwm0[] = {
	{ 0x0c, GENMASK(2, 0), BIT(0) },
};

static struct aspeed_sig_desc pwm1[] = {
	{ 0x40c, GENMASK(6, 4), BIT(4) },
};

static struct aspeed_sig_desc pwm2[] = {
	{ 0x40c, GENMASK(10, 8), BIT(8) },
};

static struct aspeed_sig_desc pwm3[] = {
	{ 0x40c, GENMASK(14, 12), BIT(12) },
};

static struct aspeed_sig_desc pwm4[] = {
	{ 0x40c, GENMASK(18, 16), BIT(16) },
};

static struct aspeed_sig_desc pwm5[] = {
	{ 0x40c, GENMASK(22, 20), BIT(20) },
};

static struct aspeed_sig_desc pwm6[] = {
	{ 0x40c, GENMASK(26, 24), BIT(24) },
};

static struct aspeed_sig_desc pwm7[] = {
	{ 0x40c, GENMASK(30, 28), BIT(28) },
};

static const struct aspeed_group_config ast2700_io_groups[] = {
	{ "MDIO0", ARRAY_SIZE(mdio0_link), mdio0_link },
	{ "MDIO1", ARRAY_SIZE(mdio1_link), mdio1_link },
	{ "MDIO2", ARRAY_SIZE(mdio2_link), mdio2_link },
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
	{ "PWM0", ARRAY_SIZE(pwm0), pwm0 },
	{ "PWM1", ARRAY_SIZE(pwm1), pwm1 },
	{ "PWM2", ARRAY_SIZE(pwm2), pwm2 },
	{ "PWM3", ARRAY_SIZE(pwm3), pwm3 },
	{ "PWM4", ARRAY_SIZE(pwm4), pwm4 },
	{ "PWM5", ARRAY_SIZE(pwm5), pwm5 },
	{ "PWM6", ARRAY_SIZE(pwm6), pwm6 },
	{ "PWM7", ARRAY_SIZE(pwm7), pwm7 },
};

static int ast2700_io_pinctrl_get_groups_count(struct udevice *dev)
{
	debug("PINCTRL: get_(functions/groups)_count\n");

	return ARRAY_SIZE(ast2700_io_groups);
}

static const char *ast2700_io_pinctrl_get_group_name(struct udevice *dev,
						     unsigned int selector)
{
	debug("PINCTRL: get_(function/group)_name %u\n", selector);

	return ast2700_io_groups[selector].group_name;
}

static int ast2700_io_pinctrl_group_set(struct udevice *dev,
					unsigned int selector, unsigned int func_selector)
{
	struct aspeed_pinctrl_priv *priv = dev_get_priv(dev);
	const struct aspeed_group_config *config;
	const struct aspeed_sig_desc *descs;
	u32 i;

	debug("PINCTRL: group_set <%u, %u>\n", selector, func_selector);
	if (selector >= ARRAY_SIZE(ast2700_io_groups))
		return -EINVAL;

	config = &ast2700_io_groups[selector];
	for (i = 0; i < config->ndescs; i++) {
		descs = &config->descs[i];
		clrsetbits_le32(priv->base + descs->offset, descs->reg_clr, descs->reg_set);
	}

	return 0;
}

static struct pinctrl_ops ast2700_io_pinctrl_ops = {
	.set_state = pinctrl_generic_set_state,
	.get_groups_count = ast2700_io_pinctrl_get_groups_count,
	.get_group_name = ast2700_io_pinctrl_get_group_name,
	.get_functions_count = ast2700_io_pinctrl_get_groups_count,
	.get_function_name = ast2700_io_pinctrl_get_group_name,
	.pinmux_group_set = ast2700_io_pinctrl_group_set,
};

static const struct udevice_id ast2700_io_pinctrl_ids[] = {
	{ .compatible = "aspeed,g7_io-pinctrl" },
	{ }
};

U_BOOT_DRIVER(pinctrl_ast2700_io) = {
	.name = "aspeed_ast2700_io_pinctrl",
	.id = UCLASS_PINCTRL,
	.of_match = ast2700_io_pinctrl_ids,
	.priv_auto = sizeof(struct aspeed_pinctrl_priv),
	.ops = &ast2700_io_pinctrl_ops,
	.probe = ast2700_io_pinctrl_probe,
};
