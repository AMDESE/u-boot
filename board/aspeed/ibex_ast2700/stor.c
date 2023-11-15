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

int spi_init(void)
{
	uint32_t reg_val;

	reg_val = readl((void *)ASPEED_IO_FWSPI_DRIVING);
	reg_val |= 0x00000fff;
	writel(reg_val, (void *)ASPEED_IO_FWSPI_DRIVING);

	reg_val = readl((void *)ASPEED_IO_SPI0_DRIVING);
	reg_val |= 0x00000fff;
	writel(reg_val, (void *)ASPEED_IO_SPI0_DRIVING);

	reg_val = readl((void *)ASPEED_IO_SPI1_DRIVING);
	reg_val |= 0x0fff0000;
	writel(reg_val, (void *)ASPEED_IO_SPI1_DRIVING);

	reg_val = readl((void *)ASPEED_IO_SPI2_DRIVING);
	reg_val |= 0x00000fff;
	writel(reg_val, (void *)ASPEED_IO_SPI2_DRIVING);

	return 0;
}

int spi_load_image(u32 *src, u32 *dest, u32 len)
{
	u32 i;
	u32 *base = (u32 *)(ASPEED_FMC_CS0_BASE + (u32)src);

	printf("spi load image base = %x\n", (u32)base);
	printf("spi load image base[0] = %x\n", *base);

	for (i = 0; i < len / 4; i++)
		writel(*(base + i), dest + i);

	return 0;
}

struct stor_info {
	char name[10];
	int (*init_cb)(void);
	int (*copy_cb)(u32 *src, u32 *dest, u32 len);
};

struct stor_info stor_dev[] = {
	{"SPI", spi_init,	spi_load_image},
	{"MMC", emmc_init,	emmc_load_image},
	{"UFS", NULL,		NULL},
};

int stor_init(void)
{
	int ret, i;

	if ((readl((void *)ASPEED_IO_HW_STRAP1) & SCU_IO_HWSTRAP_EMMC)) {
		if (readl((void *)ASPEED_IO_HW_STRAP1) & SCU_IO_HWSTRAP_UFS)
			boot_dev = BOOT_DEV_UFS;
		else
			boot_dev = BOOT_DEV_MMC;
	} else {
		boot_dev = BOOT_DEV_SPI;
	}

	for (i = 0; i < ARRAY_SIZE(stor_dev); i++) {
		if (stor_dev[i].init_cb) {
			ret = stor_dev[i].init_cb();

			if (ret)
				printf("%s init failed\n", stor_dev[i].name);
		}
	}

	return 0;
}

int stor_copy(u32 *src, u32 *dest, u32 len)
{
	if (!stor_dev[boot_dev].copy_cb)
		return 1;

	return stor_dev[boot_dev].copy_cb(src, dest, len);
}
