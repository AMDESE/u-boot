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
	u32 reg_set;
	int clr;
};

struct aspeed_group_config {
	char *group_name;
	int ndescs;
	struct aspeed_sig_desc *descs;
};

struct aspeed_pinctrl_priv {
	void __iomem *base;
};

static int ast2700_cpu_pinctrl_probe(struct udevice *dev)
{
	struct aspeed_pinctrl_priv *priv = dev_get_priv(dev);

	priv->base = dev_remap_addr(dev);
	if (!priv->base)
		return -ENOMEM;

	return 0;
}

static struct aspeed_sig_desc emmc_link[] = {
	{ 0x00, GENMASK(7, 0), 0 },
};

static struct aspeed_sig_desc emmc_8bit_link[] = {
	{ 0x00, GENMASK(11, 0), 0 },
};

static struct aspeed_sig_desc usb2ad_link[] = {
	{ 0x10, BIT(24), 0 },
	{ 0x10, BIT(25), 1 },
};

static struct aspeed_sig_desc usb2ah_link[] = {
	{ 0x10, BIT(24), 1 },
	{ 0x10, BIT(25), 0 },
};

static struct aspeed_sig_desc usb2bd_link[] = {
	{ 0x10, BIT(28), 0 },
	{ 0x10, BIT(29), 1 },
};

static struct aspeed_sig_desc usb2bh_link[] = {
	{ 0x10, BIT(28), 1 },
	{ 0x10, BIT(29), 0 },
};

static const struct aspeed_group_config ast2700_cpu_groups[] = {
	{ "EMMC", ARRAY_SIZE(emmc_link), emmc_link },
	{ "EMMC8BIT", ARRAY_SIZE(emmc_8bit_link), emmc_8bit_link },
	{ "USB2AD", ARRAY_SIZE(usb2ad_link), usb2ad_link },
	{ "USB2AH", ARRAY_SIZE(usb2ah_link), usb2ah_link },
	{ "USB2BD", ARRAY_SIZE(usb2bd_link), usb2bd_link },
	{ "USB2BH", ARRAY_SIZE(usb2bh_link), usb2bh_link },
};

static int ast2700_cpu_pinctrl_get_groups_count(struct udevice *dev)
{
	debug("PINCTRL: get_(functions/groups)_count\n");

	return ARRAY_SIZE(ast2700_cpu_groups);
}

static const char *ast2700_cpu_pinctrl_get_group_name(struct udevice *dev,
						      unsigned int selector)
{
	debug("PINCTRL: get_(function/group)_name %u\n", selector);

	return ast2700_cpu_groups[selector].group_name;
}

static int ast2700_cpu_pinctrl_group_set(struct udevice *dev, unsigned int selector,
					 unsigned int func_selector)
{
	struct aspeed_pinctrl_priv *priv = dev_get_priv(dev);
	const struct aspeed_group_config *config;
	const struct aspeed_sig_desc *descs;
	u32 i;

	debug("PINCTRL: group_set <%u, %u>\n", selector, func_selector);
	if (selector >= ARRAY_SIZE(ast2700_cpu_groups))
		return -EINVAL;

	config = &ast2700_cpu_groups[selector];
	for (i = 0; i < config->ndescs; i++) {
		descs = &config->descs[i];
		if (descs->clr)
			clrbits_le32(priv->base + descs->offset, descs->reg_set);
		else
			setbits_le32(priv->base + descs->offset, descs->reg_set);
	}

	return 0;
}

static struct pinctrl_ops ast2700_cpu_pinctrl_ops = {
	.set_state = pinctrl_generic_set_state,
	.get_groups_count = ast2700_cpu_pinctrl_get_groups_count,
	.get_group_name = ast2700_cpu_pinctrl_get_group_name,
	.get_functions_count = ast2700_cpu_pinctrl_get_groups_count,
	.get_function_name = ast2700_cpu_pinctrl_get_group_name,
	.pinmux_group_set = ast2700_cpu_pinctrl_group_set,
};

static const struct udevice_id ast2700_cpu_pinctrl_ids[] = {
	{ .compatible = "aspeed,g7_cpu-pinctrl" },
	{ }
};

U_BOOT_DRIVER(pinctrl_ast2700_cpu) = {
	.name = "aspeed_ast2700_cpu_pinctrl",
	.id = UCLASS_PINCTRL,
	.of_match = ast2700_cpu_pinctrl_ids,
	.priv_auto = sizeof(struct aspeed_pinctrl_priv),
	.ops = &ast2700_cpu_pinctrl_ops,
	.probe = ast2700_cpu_pinctrl_probe,
};
