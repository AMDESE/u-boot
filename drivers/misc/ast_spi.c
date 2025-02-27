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

struct bootspi_priv {
	u32 temp;
};

static int spi_init(struct udevice *dev)
{
	return 0;
}

static int spi_copy(struct udevice *dev, u32 *dst, u32 *src, u32 len)
{
	const void *base;

	debug("dst=0x%x, src=0x%x, len=0x%x\n", (u32)dst, (u32)src, len);

	base = (const void *)(ASPEED_FMC_CS0_BASE + (u32)src);// +
//			      aspeed_spi_abr_offset());
	debug("spi load image base = %x\n", (u32)base);

	memcpy(dst, base, len);

	return 0;
}

static const struct udevice_id bootspi_ids[] = {
	{ .compatible = "aspeed,bootspi" },
	{ }
};

static struct ast_loader_ops bootspi_ops = {
	.init = spi_init,
	.copy = spi_copy,
};

U_BOOT_DRIVER(bootspi) = {
	.name		= "bootspi",
	.id		= UCLASS_MISC,
	.of_match	= bootspi_ids,
	.priv_auto	= sizeof(struct bootspi_priv),
	.ops		= &bootspi_ops,
};
