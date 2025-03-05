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

struct bootusb_priv {
	u32 temp;
};

static int usb_init(struct udevice *dev)
{
	return 0;
}

static int usb_load(struct udevice *dev, u32 *dst, u32 *len)
{
	return 0;
}

static const struct udevice_id bootusb_ids[] = {
	{ .compatible = "aspeed,bootusb" },
	{ }
};

static struct ast_loader_ops bootusb_ops = {
	.init = usb_init,
	.load = usb_load,
};

U_BOOT_DRIVER(bootusb) = {
	.name		= "bootusb",
	.id		= UCLASS_MISC,
	.of_match	= bootusb_ids,
	.priv_auto	= sizeof(struct bootusb_priv),
	.ops		= &bootusb_ops,
};
