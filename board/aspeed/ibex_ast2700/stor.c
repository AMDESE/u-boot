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
#include <binman_sym.h>
#include <asm/arch/platform.h>
#include <asm/arch/scu_ast2700.h>
#include <asm/arch/spi.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <scsi.h>
#include <ufs.h>

int boot_dev;
struct udevice *dev;

int stor_init(void)
{
	int ret = 0;

	if ((readl((void *)ASPEED_IO_HW_STRAP1) & SCU_IO_HWSTRAP_EMMC)) {
		if (readl((void *)ASPEED_IO_HW_STRAP1) & SCU_IO_HWSTRAP_UFS)
			boot_dev = BOOT_DEVICE_UFS;
		else
			boot_dev = BOOT_DEVICE_MMC1;
	} else {
		boot_dev = BOOT_DEVICE_RAM;
	}

	switch (boot_dev) {
	case BOOT_DEVICE_RAM:
		break;
	case BOOT_DEVICE_MMC1:
		puts("EMMC_MODE\n");

		/* release emmc pin from emmc boot */
		writel(0, (void *)0x12c0b00c);

		/* config gpio18 a0 to A5 to emmc mode */
		writel(0xff, (void *)0x12c02400);

		if (uclass_get_device_by_name(UCLASS_MMC, "sdhci@12090100", &dev)) {
			printf("emmc udevice failed!\n");
			ret = -1;
		}
		ret = blk_select_hwpart(dev, 1);
		if (ret) {
			printf("%s: bd selet part fail\n", __func__);
			ret = -1;
		}
		break;
	case BOOT_DEVICE_UFS:
		if (uclass_get_device_by_name(UCLASS_UFS, "ufshc@12c08200", &dev)) {
			printf("ufs udevice failed!\n");
			ret = -1;
		}
		break;
	default:
		printf("Invalid Boot Mode:0x%x\n", boot_dev);
		ret = -1;
		break;
	}

	return ret;
}

int stor_copy(u32 *src, u32 *dest, u32 len)
{
#define CHIP_REVID_AST2700A0	0x06000003
#define CHIP_REVID_AST2700A1	0x06010003
#define UFS_BLK_LEN 0x1000
#define MMC_BLK_LEN	0x200
	u32 addr = (u32)src;
	u32 stor_ofst = 0x0;
	u32 rev_id = readl((void *)ASPEED_IO_REVISION_ID);

	stor_ofst = (rev_id == CHIP_REVID_AST2700A1) ? 0x20000 : 0x0;

	src = (u32 *)(addr + stor_ofst);

	if (boot_dev == BOOT_DEVICE_RAM) {
		const void *base;

		base = (const void *)(ASPEED_FMC_CS0_BASE + (u32)src +
				      aspeed_spi_abr_offset());
		debug("spi load image base = %x\n", (u32)base);
		memcpy((void *)dest, base, len);
	} else {
		u32 *base;
		int ret;
		u32 blk, blks, blk_len;
		u32 ofst_in_blk = (u32)src;
		u32 i;

		if (boot_dev == BOOT_DEVICE_MMC1)
			blk_len = MMC_BLK_LEN;
		else if (boot_dev == BOOT_DEVICE_UFS)
			blk_len = UFS_BLK_LEN;

		blk = (u32)src / blk_len;
		blks = len / blk_len;
		ofst_in_blk %= blk_len;

		if (len % blk_len)
			blks++;

		if ((u32)src % blk_len)
			blks++;

		debug("blk read blk=0x%x, blks=0x%x\n", blk, blks);
		ret = blk_read(dev, blk, blks, (void *)ASPEED_SRAM_BASE);
		debug("blk read cnt=%d\n", ret);
		if (ret != blks) {
			printf("blk read is incomplete!!!\n");
			return -1;
		}

		base = (u32 *)(ASPEED_SRAM_BASE + ofst_in_blk);

		debug("blk load image base = %x\n", (u32)base);
		debug("blk load image base[0] = %x\n", *base);

		for (i = 0; i < len / 4; i++)
			writel(*(base + i), dest + i);
	}
	return 0;
}
