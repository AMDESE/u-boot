// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <errno.h>
#include <regmap.h>
#include <syscon.h>
#include <reset.h>
#include <fdtdec.h>
#include <asm/io.h>
#include <linux/bitfield.h>
#include <linux/delay.h>

#include <asm/arch/platform.h>
#include <ast_loader.h>

struct booti2c_priv {
	u32 temp;
};

static int i2c_init(struct udevice *dev)
{
	return 0;
}

static int i2c_load(struct udevice *dev, u32 *dst, u32 *len)
{
	return 0;
}

static const struct udevice_id booti2c_ids[] = {
	{ .compatible = "aspeed,booti2c" },
	{ }
};

static struct ast_loader_ops booti2c_ops = {
	.init = i2c_init,
	.load = i2c_load,
};

U_BOOT_DRIVER(booti2c) = {
	.name		= "booti2c",
	.id		= UCLASS_MISC,
	.of_match	= booti2c_ids,
	.priv_auto	= sizeof(struct booti2c_priv),
	.ops		= &booti2c_ops,
};
