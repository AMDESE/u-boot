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
#include <asm/arch/abr.h>
#include <blk.h>
#include <ast_loader.h>
#include <scsi.h>
#include <ufs.h>

#define UFS_BLK_LEN	0x1000

struct bootufs_priv {
	struct blk_desc *bd;
	u32 tmp;
};

static int ufs_init(struct udevice *dev)
{
	struct bootufs_priv *bootufs = dev_get_priv(dev);
	struct blk_desc *bd;
	int err = 0;

	if (IS_ENABLED(CONFIG_DM_SCSI) && IS_ENABLED(CONFIG_SPL_SATA)) {
		scsi_scan(false);

		bd = blk_get_devnum_by_uclass_id(UCLASS_SCSI, 1 << abr_get_indicator());
		if (!bd) {
			printf("Get scsi device failed\n");
			return -ENODEV;
		}

		bootufs->bd = bd;
	}

	return err;
}

static int ufs_copy(struct udevice *dev, u32 *dst, u32 *src, u32 len)
{
	struct bootufs_priv *bootufs = dev_get_priv(dev);
	struct blk_desc *bd = bootufs->bd;
	u32 *base;
	int ret;
	u32 blks;
	u32 offset, lba, trans, extra;
	u8 blk_buf[UFS_BLK_LEN], *out = (u8 *)dst, *in = (u8 *)src;

	lba = (u32)src / UFS_BLK_LEN;
	offset = (u32)src % UFS_BLK_LEN;

	/* Handle the case where the source address is not aligned to block size */
	if (offset) {
		if (len < (UFS_BLK_LEN - offset))
			trans = len;
		else
			trans = UFS_BLK_LEN - offset;

		/* Read the first block to get the offset */
		ret = blk_dread(bd, lba, 1, blk_buf);
		if (ret != 1) {
			printf("blk read is incomplete!!!\n");
			return -1;
		}

		base = (u32 *)(blk_buf + offset);
		memcpy(dst, base, trans);

		out += trans;
		in  += trans;
		len -= trans;
	}

	/* Read the rest of the blocks */
	while (len)  {
		blks = len / UFS_BLK_LEN;
		extra = len % UFS_BLK_LEN;

		lba = (u32)in / UFS_BLK_LEN;
		offset = (u32)in % UFS_BLK_LEN;

		if (len == extra) {
			/* Read out the last block */
			ret = blk_dread(bd, lba, 1, blk_buf);
			if (ret != 1) {
				printf("blk read is incomplete!!!\n");
				return -1;
			}

			memcpy(out, blk_buf + offset, extra);

			out += extra;
			in += extra;
			len -= extra;
		} else {
			/* Read out the whole block */
			ret = blk_dread(bd, lba, blks, (void *)out);
			debug("blk read cnt=%d\n", ret);
			if (ret != blks) {
				printf("blk read is incomplete!!!\n");
				return -1;
			}

			out += (UFS_BLK_LEN * blks);
			in += (UFS_BLK_LEN * blks);
			len -= (UFS_BLK_LEN * blks);
		}
	}

	return 0;
}

static const struct udevice_id bootufs_ids[] = {
	{ .compatible = "aspeed,bootufs" },
	{ }
};

static struct ast_loader_ops bootufs_ops = {
	.init = ufs_init,
	.copy = ufs_copy,
};

U_BOOT_DRIVER(bootufs) = {
	.name		= "bootufs",
	.id		= UCLASS_MISC,
	.of_match	= bootufs_ids,
	.priv_auto	= sizeof(struct bootufs_priv),
	.ops		= &bootufs_ops,
};
