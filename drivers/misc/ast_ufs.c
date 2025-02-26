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
	u32 blk, blks, blk_len;
	u32 ofst_in_blk = (u32)src;
	u32 i;

	blk_len = UFS_BLK_LEN;
	blk = (u32)src / blk_len;
	blks = len / blk_len;
	ofst_in_blk %= blk_len;

	if (len % blk_len)
		blks++;

	if ((u32)src % blk_len)
		blks++;

	debug("blk read blk=0x%x, blks=0x%x\n", blk, blks);
	ret = blk_dread(bd, blk, blks, (void *)ASPEED_SRAM_BASE);
	debug("blk read cnt=%d\n", ret);
	if (ret != blks) {
		printf("blk read is incomplete!!!\n");
		return -1;
	}

	base = (u32 *)(ASPEED_SRAM_BASE + ofst_in_blk);

	debug("blk load image base = %x\n", (u32)base);
	debug("blk load image base[0] = %x\n", *base);

	for (i = 0; i < len / 4; i++)
		writel(*(base + i), dst + i);

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
