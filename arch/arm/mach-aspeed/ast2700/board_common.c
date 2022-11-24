// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */
#include <common.h>
#include <dm.h>
#include <ram.h>
#include <timer.h>
#include <asm/io.h>
#include <asm/arch/timer.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <dm/uclass.h>

DECLARE_GLOBAL_DATA_PTR;

int dram_init(void)
{
	int ret;
	struct udevice *dev;
	struct ram_info ram;

	ret = uclass_get_device(UCLASS_RAM, 0, &dev);
	if (ret) {
		debug("cannot get DRAM driver\n");
		return ret;
	}

	ret = ram_get_info(dev, &ram);
	if (ret) {
		debug("cannot get DRAM information\n");
		return ret;
	}

	gd->ram_size = ram.size;
	return 0;
}

int board_init(void)
{
	return 0;
}
