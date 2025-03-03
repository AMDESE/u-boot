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
	u32 blk, blks, blk_len;
	u32 ofst_in_blk = (u32)src;
	u32 i;

	blk_len = MMC_BLK_LEN;
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
