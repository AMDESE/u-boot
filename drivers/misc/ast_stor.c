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
#include <asm/arch/scu_ast2700.h>
#include <asm/arch/platform.h>
#include <ast_loader.h>

static int stor_load(struct udevice *dev, u32 type, u32 *dst, u32 *len)
{
	struct ast_loader *ast = dev_get_priv(dev);
	struct ast_loader_ops *ops;
	int err = 0;
	u32 *src, ofst, sz;
	u32 rev_id = readl((void *)ASPEED_IO_REVISION_ID);
	u32 stor_ofst = 0x0;

	err = fmc_hdr_get_prebuilt(type, &ofst, &sz, NULL);
	if (err)
		return err;

	stor_ofst = !!(rev_id & CHIP_AST2700A1_ID_MASK) ? 0x20000 : 0x0;
	src = (u32 *)(ofst + stor_ofst);

	ops = ast_loader_get_ops(ast->boot_dev);
	if (ops && ops->copy)
		err = ops->copy(ast->boot_dev, dst, src, sz);

	*len = sz;

	return err;
}

int stor_init(struct udevice *dev)
{
	struct ast_loader *ast = dev_get_priv(dev);
	struct ast_loader_ops *ops;
	struct udevice *boot_dev = NULL;
	enum boot_mode_t bootmode;
	int err = -ENODEV;

	bootmode = ast->bootmode;

	if (bootmode == BOOT_SPI) {
		err = bootspi_init(&boot_dev);
	} else if (bootmode == BOOT_EMMC) {
		printf("bootmmc_init\n");
		//err = bootmmc_init(&boot_dev);
	} else if (bootmode == BOOT_UFS) {
		printf("bootufs_init\n");
		//err = bootufs_init(&boot_dev);
	} else {
		return -ENODEV;
	}

	if (err && err != -ENODEV) {
		printf("Get stor udevice Failed %d.\n", err);
		return err;
	}

	ast->boot_dev = boot_dev;
	ast->load = stor_load;

	ops = ast_loader_get_ops(boot_dev);
	if (ops && ops->init)
		err = ops->init(boot_dev);

	return err;
}
