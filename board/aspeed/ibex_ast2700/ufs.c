// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */
#include <asm/io.h>
#include <dm.h>
#include <ram.h>
#include <spl.h>
#include <common.h>
#include <asm/csr.h>
#include <asm/arch-aspeed/platform.h>
#include <asm/arch-aspeed/scu_ast2700.h>
#include <asm/arch-aspeed/ufs_ast2700.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/delay.h>

#include <scsi.h>
#include <ufs.h>

int ufs_init(void)
{
	int ret = 0;
	struct udevice *dev;

	if (spl_boot_device() != BOOT_DEVICE_SATA)
		return 0;

	if (IS_ENABLED(CONFIG_DM_SCSI)) {
		ret = uclass_get_device(UCLASS_SCSI, 0, &dev);
		if (ret) {
			printf("Get SCSI failed.\n");
			return ret;
		}
	}

	return ret;
}

int ufs_load_image(u32 *src, u32 *dest, u32 len)
{
	printf("Not implemented yet!!!\n");
	return 0;
}
