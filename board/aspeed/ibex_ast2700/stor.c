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
#include <asm/arch-aspeed/spi.h>
#include <asm/arch-aspeed/mmc_ast2700.h>
#include <asm/arch-aspeed/ufs_ast2700.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/delay.h>

int boot_dev;

enum {
	BOOT_DEV_SPI,
	BOOT_DEV_MMC,
	BOOT_DEV_UFS,
};

struct stor_info {
	char name[10];
	int (*init_cb)(void);
	int (*copy_cb)(u32 *src, u32 *dest, u32 len);
};

struct stor_info stor_dev[] = {
	{"SPI", spi_init,	spi_load_image},
	{"MMC", emmc_init,	emmc_load_image},
	{"UFS", ufs_init,	ufs_load_image},
};

int stor_init(void)
{
	int ret;

	if ((readl((void *)ASPEED_IO_HW_STRAP1) & SCU_IO_HWSTRAP_EMMC)) {
		if (readl((void *)ASPEED_IO_HW_STRAP1) & SCU_IO_HWSTRAP_UFS)
			boot_dev = BOOT_DEV_UFS;
		else
			boot_dev = BOOT_DEV_MMC;
	} else {
		boot_dev = BOOT_DEV_SPI;
	}

	ret = stor_dev[boot_dev].init_cb();
	if (ret)
		printf("%s init failed\n", stor_dev[boot_dev].name);

	return 0;
}

int stor_copy(u32 *src, u32 *dest, u32 len)
{
	if (!stor_dev[boot_dev].copy_cb)
		return 1;

	return stor_dev[boot_dev].copy_cb(src, dest, len);
}
