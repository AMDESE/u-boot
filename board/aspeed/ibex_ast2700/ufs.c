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

#define UFS_BLK_LEN (0x1000)

struct blk_desc *blk_dev;

int ufs_init(void)
{
	int ret = 0;
	struct udevice *dev;

	if (spl_boot_device() != BOOT_DEVICE_SATA)
		return 0;

	if (IS_ENABLED(CONFIG_DM_SCSI) && IS_ENABLED(CONFIG_SPL_SATA)) {
		ret = uclass_get_device(UCLASS_SCSI, 0, &dev);
		if (ret) {
			printf("Get SCSI failed.\n");
			return ret;
		}

		scsi_scan(false);

		blk_dev = blk_get_devnum_by_uclass_id(UCLASS_SCSI, 0);
		if (!blk_dev)
			return -ENODEV;
	}

	return ret;
}

int ufs_load_image(u32 *src, u32 *dest, u32 len)
{
	u32 blk, blks;
	u32 ofst_in_blk = (u32)src;
	u32 i, count;
	u32 *base;
	u8 *tmp = (u8 *)ASPEED_SRAM_BASE;

	blk = (u32)src / UFS_BLK_LEN;
	blks = len / UFS_BLK_LEN;
	ofst_in_blk %= UFS_BLK_LEN;

	if (len % UFS_BLK_LEN)
		blks++;

	if ((u32)src % UFS_BLK_LEN)
		blks++;

	while (blks) {
		printf("blk read blk=0x%x, blks=0x%x\n", blk, blks);

		count = blk_dread(blk_dev, blk++, 1, (void *)tmp);
		if (count == 0)
			return -EIO;

		base = (u32 *)(tmp + ofst_in_blk);
		printf("ufs load image base = %x\n", (u32)base);
		printf("ufs load image base[0] = %x\n", *base);

		for (i = 0; i < len / 4; i++)
			writel(*(base + i), dest + i);

		blks--;
	};

	return 0;
}
