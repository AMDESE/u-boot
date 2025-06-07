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

#define MMC_BLK_LEN	0x200

struct bootmmc_priv {
	struct blk_desc *bd;
	u32 tmp;
};

static int mmc_init(struct udevice *dev)
{
	struct bootmmc_priv *bootmmc = dev_get_priv(dev);
	struct udevice *udev;
	struct blk_desc *bd;
	int err;

	/* release emmc pin from emmc boot */
	writel(0, (void *)0x12c0b00c);

	/* config gpio18 a0 to A5 to emmc mode */
	writel(0xff, (void *)0x12c02400);

	err = uclass_get_device(UCLASS_BLK, 0, &udev);
	if (err) {
		printf("Get block udevice failed!\n");
		return err;
	}

	bd = dev_get_uclass_plat(udev);

	err = blk_dselect_hwpart(bd, 1 << abr_get_indicator());
	if (err) {
		printf("%s: blk_dselect_hwpart fail\n", __func__);
		return err;
	}

	bootmmc->bd = bd;

	return err;
}

static int mmc_copy(struct udevice *dev, u32 *dst, u32 *src, u32 len)
{
	struct bootmmc_priv *bootmmc = dev_get_priv(dev);
	struct blk_desc *bd = bootmmc->bd;
	u32 *base;
	int ret;
	u32 blks;
	u32 offset, lba, trans, extra;
	u8 blk_buf[MMC_BLK_LEN], *out = (u8 *)dst, *in = (u8 *)src;

	lba = (u32)src / MMC_BLK_LEN;
	offset = (u32)src % MMC_BLK_LEN;

	/* Handle the case where the source address is not aligned to block size */
	if (offset) {
		if (len < (MMC_BLK_LEN - offset))
			trans = len;
		else
			trans = MMC_BLK_LEN - offset;

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
		blks = len / MMC_BLK_LEN;
		extra = len % MMC_BLK_LEN;

		lba = (u32)in / MMC_BLK_LEN;
		offset = (u32)in % MMC_BLK_LEN;

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

			out += (MMC_BLK_LEN * blks);
			in += (MMC_BLK_LEN * blks);
			len -= (MMC_BLK_LEN * blks);
		}
	}

	return 0;
}

static const struct udevice_id bootmmc_ids[] = {
	{ .compatible = "aspeed,bootmmc" },
	{ }
};

static struct ast_loader_ops bootmmc_ops = {
	.init = mmc_init,
	.copy = mmc_copy,
};

U_BOOT_DRIVER(bootmmc) = {
	.name		= "bootmmc",
	.id		= UCLASS_MISC,
	.of_match	= bootmmc_ids,
	.priv_auto	= sizeof(struct bootmmc_priv),
	.ops		= &bootmmc_ops,
};
