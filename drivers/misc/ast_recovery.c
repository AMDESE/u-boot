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
#include <spl.h>
#include <ast_loader.h>

struct recovery_info {
	int id;
	char *msg;
};

struct recovery_info message[] = {
	{PBT_END_MARK,			"\n"},
	{PBT_DDR4_PMU_TRAIN_IMEM,	"Please send \"ddr4_pmu_train_imem.bin\" through recovery interface.\n"},
	{PBT_DDR4_PMU_TRAIN_DMEM,	"Please send \"ddr4_pmu_train_dmem.bin\" through recovery interface.\n"},
	{PBT_DDR4_2D_PMU_TRAIN_IMEM,	"Please send \"ddr4_2d_pmu_train_imem.bin\" through recovery interface.\n"},
	{PBT_DDR4_2D_PMU_TRAIN_DMEM,	"Please send \"ddr4_2d_pmu_train_dmem.bin\" through recovery interface.\n"},
	{PBT_DDR5_PMU_TRAIN_IMEM,	"Please send \"ddr5_pmu_train_imem.bin\" through recovery interface.\n"},
	{PBT_DDR5_PMU_TRAIN_DMEM,	"Please send \"ddr5_pmu_train_dmem.bin\" through recovery interface.\n"},
	{PBT_DP_FW,			"Please send \"dp_fw.bin\" through recovery interface.\n"},
	{PBT_UEFI_X64_AST2700,		"Please send \"uefi_x64_ast2700.bin\" through recovery interface.\n"},
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
	int bootmode;
	int err = -ENODEV;

	bootmode = ast->bootmode;

	if (bootmode == BOOT_DEVICE_USB)
		err = uclass_get_device_by_name(UCLASS_MISC, "bootusb", &boot_dev);
	else if (bootmode == BOOT_DEVICE_I2C)
		err = uclass_get_device_by_name(UCLASS_MISC, "booti2c", &boot_dev);
	else if (bootmode == BOOT_DEVICE_I3C)
		err = uclass_get_device_by_name(UCLASS_MISC, "booti3c", &boot_dev);
	else if (bootmode == BOOT_DEVICE_UART)
		err = uclass_get_device_by_name(UCLASS_MISC, "bootuart", &boot_dev);
	else
		return -ENODEV;

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
