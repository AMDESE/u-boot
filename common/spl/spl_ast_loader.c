// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2014
 * Texas Instruments, <www.ti.com>
 *
 * Dan Murphy <dmurphy@ti.com>
 *
 * Derived work from spl_mmc.c
 */

#include "aspeed/fmc_hdr.h"
#include "linux/errno.h"
#include <common.h>
#include <image.h>
#include <log.h>
#include <spl.h>
#include <asm/u-boot.h>
#include <ast_loader.h>

static ulong ast_loader_read_fit(struct spl_load_info *load, ulong offset, ulong size, void *buf)
{
	int res;

	debug("%s: offset %lx, size %lx, buf %lx\n", __func__, offset, size, (ulong)buf);

	res = ast_loader_load_image(PBT_FIT, (u32 *)buf, false);

	if (res < 0)
		return -EIO;

	return size;
}

static int spl_ast_loader_load_image(struct spl_image_info *spl_image,
				     struct spl_boot_device *bootdev)
{
	int res;
	int ret;
	char buf[sizeof(struct legacy_img_hdr)];

	res = ast_loader_load_image(PBT_FIT_HEADER, (u32 *)buf, false);
	if (res < 0)
		return -EIO;

	if (IS_ENABLED(CONFIG_SPL_LOAD_FIT) &&
	    image_get_magic((struct legacy_img_hdr *)buf) == FDT_MAGIC) {
		struct spl_load_info load;

		debug("Found FIT\n");
		load.dev = NULL;
		load.priv = NULL;
		load.filename = NULL;
		load.bl_len = 1;
		load.read = ast_loader_read_fit;
		ret = spl_load_simple_fit(spl_image, &load, 0, (void *)buf);
	} else {
		return -EOPNOTSUPP;
	}

	return ret;
}

SPL_LOAD_IMAGE_METHOD("I3C", 0, BOOT_DEVICE_I3C, spl_ast_loader_load_image);
SPL_LOAD_IMAGE_METHOD("I2C", 0, BOOT_DEVICE_I2C, spl_ast_loader_load_image);
SPL_LOAD_IMAGE_METHOD("USB", 0, BOOT_DEVICE_USB, spl_ast_loader_load_image);
