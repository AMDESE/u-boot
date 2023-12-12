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
#include <asm/arch-aspeed/mmc_ast2700.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/delay.h>

#define MMC_BLK_LEN	512

struct udevice *dev;
int emmc_init(void)
{
	int ret = 0;

	/* set clk/cmd driving */
	writel(2, (void *)MMC_CLK_DRIVING_REG);
	writel(1, (void *)MMC_CMD_DRIVING_REG);
	writel(1, (void *)MMC_DAT0_DRIVING_REG);
	writel(1, (void *)MMC_DAT1_DRIVING_REG);
	writel(1, (void *)MMC_DAT2_DRIVING_REG);
	writel(1, (void *)MMC_DAT3_DRIVING_REG);

	if (spl_boot_device() != BOOT_DEVICE_MMC1)
		return 0;

	/* release emmc pin from emmc boot */
	writel(0, (void *)0x12c0b00c);

	ret = uclass_get_device(UCLASS_BLK, 0, &dev);
	if (ret) {
		printf("cannot get BLK driver\n");
		return -ENODEV;
	}

	ret = blk_select_hwpart(dev, 1);
	if (ret) {
		printf("%s: bd selet part fail\n", __func__);
		return 1;
	}

	return ret;
}

int emmc_load_image(u32 *src, u32 *dest, u32 len)
{
	int ret;
	u32 blk, blks;
	u32 ofst_in_blk = (u32)src;
	u32 i;
	u32 *base;

	blk = (u32)src / MMC_BLK_LEN;
	blks = len / MMC_BLK_LEN;
	ofst_in_blk %= MMC_BLK_LEN;

	blks++;

	debug("blk read blk=0x%x, blks=0x%x\n", blk, blks);
	ret = blk_read(dev, blk, blks, (void *)ASPEED_SRAM_BASE);
	debug("blk read cnt=%d\n", ret);
	if (ret != blks) {
		printf("blk read is incomplete!!!\n");
		return 1;
	}

	base = (u32 *)(ASPEED_SRAM_BASE + ofst_in_blk);

	debug("mmc load image base = %x\n", (u32)base);
	debug("mmc load image base[0] = %x\n", *base);

	for (i = 0; i < len / 4; i++)
		writel(*(base + i), dest + i);

	return 0;
}
