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
#include <ast_loader.h>

struct recovery_info {
	int id;
	char *msg;
};

struct recovery_info message[] = {
	{PBT_END_MARK,			"\n"},
	{PBT_DDR4_PMU_TRAIN_IMEM,	"Please send \"ddr4_pmu_train_imem.bin\" through Ymodem.\n"},
	{PBT_DDR4_PMU_TRAIN_DMEM,	"Please send \"ddr4_pmu_train_dmem.bin\" through Ymodem.\n"},
	{PBT_DDR4_2D_PMU_TRAIN_IMEM,	"Please send \"ddr4_2d_pmu_train_imem.bin\" through Ymodem.\n"},
	{PBT_DDR4_2D_PMU_TRAIN_DMEM,	"Please send \"ddr4_2d_pmu_train_dmem.bin\" through Ymodem.\n"},
	{PBT_DDR5_PMU_TRAIN_IMEM,	"Please send \"ddr5_pmu_train_imem.bin\" through Ymodem.\n"},
	{PBT_DDR5_PMU_TRAIN_DMEM,	"Please send \"ddr5_pmu_train_dmem.bin\" through Ymodem.\n"},
	{PBT_DP_FW,			"Please send \"dp_fw.bin\" through Ymodem.\n"},
	{PBT_UEFI_X64_AST2700,		"Please send \"uefi_x64_ast2700.bin\" through Ymodem.\n"},
};

static int recovery_load(struct udevice *dev, u32 type, u32 *dst, u32 *len)
{
	struct ast_loader *ast = dev_get_priv(dev);
	struct ast_loader_ops *ops;
	u32 sz;
	int err;

	printf("%s", message[type].msg);

	ops = ast_loader_get_ops(ast->boot_dev);
	if (ops && ops->load)
		err = ops->load(ast->boot_dev, dst, &sz);

	*len = sz;

	return err;
}

int recovery_init(struct udevice *dev)
{
	struct ast_loader *ast = dev_get_priv(dev);
	struct ast_loader_ops *ops;
	struct udevice *boot_dev = NULL;
	enum boot_mode_t bootmode;
	int err = -ENODEV;

	bootmode = ast->bootmode;

	if (bootmode == BOOT_USB) {
		printf("bootusb_init\n");
		//err = bootusb_init(&boot_dev);
	} else if (bootmode == BOOT_I2C) {
		printf("booti2c_init\n");
		//err = booti2c_init();
	} else if (bootmode == BOOT_I3C) {
		printf("booti3c_init\n");
		//err = booti3c_init(&boot_dev);
	} else if (bootmode == BOOT_UART) {
		err = bootuart_init(&boot_dev);
	} else {
		return -ENODEV;
	}

	if (err && err != -ENODEV) {
		printf("Get recovery udevice Failed %d.\n", err);
		return err;
	}

	ast->boot_dev = boot_dev;
	ast->load = recovery_load;

	ops = ast_loader_get_ops(boot_dev);
	if (ops && ops->init)
		err = ops->init(boot_dev);

	return err;
}
