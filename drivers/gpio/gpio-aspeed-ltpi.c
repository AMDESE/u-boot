// SPDX-License-Identifier: GPL-2.0
/*
 * Aspeed LTPI GPIO Driver for U-Boot
 *
 * Copyright (C) ASPEED Technology Inc.
 * Author: Billy Tsai <billy_tsai@aspeedtech.com>
 */

#include <common.h>
#include <dm.h>
#include <asm/io.h>
#include <dm/device_compat.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/log2.h>
#include <linux/types.h>
#include <dm/pinctrl.h>
#include <asm/gpio.h>

#define LTPI_GPIO_CTRL_REG_BASE 0x0
#define LTPI_GPIO_CTRL_REG_OFFSET(x) (LTPI_GPIO_CTRL_REG_BASE + (x) * 0x4)
#define LTPI_GPIO_OUT_DATA BIT(0)
#define LTPI_GPIO_IN_DATA BIT(13)

struct aspeed_ltpi_gpio_priv {
	void __iomem *base;
};

static int aspeed_ltpi_gpio_get_value(struct udevice *dev, unsigned int offset)
{
	struct aspeed_ltpi_gpio_priv *priv = dev_get_priv(dev);
	void __iomem *addr = priv->base + LTPI_GPIO_CTRL_REG_OFFSET(offset >> 1);
	u32 reg = readl(addr);

	if (offset % 2 == 0) // Input GPIO
		return !!(reg & LTPI_GPIO_IN_DATA);
	else // Output GPIO
		return !!(reg & LTPI_GPIO_OUT_DATA);
}

static int aspeed_ltpi_gpio_set_value(struct udevice *dev, unsigned int offset, int value)
{
	struct aspeed_ltpi_gpio_priv *priv = dev_get_priv(dev);
	void __iomem *addr = priv->base + LTPI_GPIO_CTRL_REG_OFFSET(offset >> 1);
	u32 reg = readl(addr);

	if (offset % 2 == 0) // Input GPIOs cannot be set
		return -EINVAL;

	if (value)
		reg |= LTPI_GPIO_OUT_DATA;
	else
		reg &= ~LTPI_GPIO_OUT_DATA;

	writel(reg, addr);
	return 0;
}

static int aspeed_ltpi_gpio_direction_input(struct udevice *dev, unsigned int offset)
{
	// Only even-numbered GPIOs are inputs
	return (offset % 2 == 0) ? 0 : -EINVAL;
}

static int aspeed_ltpi_gpio_get_function(struct udevice *dev, unsigned int offset)
{
	return aspeed_ltpi_gpio_direction_input(dev, offset) ? GPIOF_OUTPUT : GPIOF_INPUT;
}

static int aspeed_ltpi_gpio_set_flags(struct udevice *dev, unsigned int offset, ulong flags)
{
	int ret = -EOPNOTSUPP;

	if (flags & GPIOD_IS_OUT) {
		bool value = flags & GPIOD_IS_OUT_ACTIVE;

		ret = aspeed_ltpi_gpio_set_value(dev, offset, value);
	} else if (flags & GPIOD_IS_IN) {
		ret = aspeed_ltpi_gpio_direction_input(dev, offset);
	}
	return ret;
}

static int aspeed_ltpi_gpio_probe(struct udevice *dev)
{
	struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);
	struct aspeed_ltpi_gpio_priv *priv = dev_get_priv(dev);
	int ret;

	uc_priv->bank_name = dev->name;
	ret = ofnode_read_u32(dev_ofnode(dev), "ngpios", &uc_priv->gpio_count);
	if (ret < 0) {
		dev_err(dev, "Could not read ngpios property\n");
		return -EINVAL;
	}
	uc_priv->gpio_count = uc_priv->gpio_count * 2;

	priv->base = devfdt_get_addr_ptr(dev);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	return 0;
}

static const struct dm_gpio_ops aspeed_ltpi_gpio_ops = {
	.get_value = aspeed_ltpi_gpio_get_value,
	.set_value = aspeed_ltpi_gpio_set_value,
	.get_function = aspeed_ltpi_gpio_get_function,
	.set_flags = aspeed_ltpi_gpio_set_flags,
};

static const struct udevice_id aspeed_ltpi_gpio_ids[] = {
	{ .compatible = "aspeed,ast2700-ltpi-gpio", },
	{ },
};

U_BOOT_DRIVER(aspeed_ltpi_gpio) = {
	.name = "aspeed_ltpi_gpio",
	.id = UCLASS_GPIO,
	.of_match = aspeed_ltpi_gpio_ids,
	.probe = aspeed_ltpi_gpio_probe,
	.priv_auto = sizeof(struct aspeed_ltpi_gpio_priv),
	.ops = &aspeed_ltpi_gpio_ops,
};
