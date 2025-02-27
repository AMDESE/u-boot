/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) Aspeed Technology Inc.
 */

#ifndef _AST_LOADER_H
#define _AST_LOADER_H

#include <aspeed/fmc_hdr.h>

#define IS_RECOVERY(n) ((n) > BOOT_UFS)

#define ast_loader_get_ops(dev) \
		((struct ast_loader_ops *)(dev)->driver->ops)

struct ast_loader_ops {
	int (*init)(struct udevice *dev);
	int (*copy)(struct udevice *udev, u32 *dst, u32 *src, u32 len);
	int (*load)(struct udevice *udev, u32 *dst, u32 *len);
};

struct stor_priv {
	u32 temp;
	struct udevice *boot_dev;
};

struct ast_loader {
	struct udevice *dev;
	struct udevice *boot_dev;

	struct ast_loader_ops *ops;

	int bootmode;

	int (*load)(struct udevice *dev, u32 type, u32 *dst, u32 *len);
	int (*verify)(u32 type, u32 *message, u32 len);

	int rev_id;
};

struct stor_ops {
	int (*init)(struct udevice *dev);
	int (*copy)(struct udevice *dev, u32 *dst, u32 *src, u32 len);
};

int ast_get_boot_mode(void);
int ast_loader_load_image(u32 type, u32 *dst);
int ast_loader_init(void);

int stor_init(struct udevice *dev);
int recovery_init(struct udevice *dev);
#endif
