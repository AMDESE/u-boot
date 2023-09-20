// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2016 Google, Inc
 */
#include <common.h>
#include <dm.h>
#include <init.h>
#include <log.h>
#include <ram.h>
#include <timer.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/arch/timer.h>
#include <asm/arch/wdt.h>
#include <linux/bitfield.h>
#include <linux/err.h>
#include <dm/uclass.h>

/*
 * Second Watchdog Timer by default is configured
 * to trigger secondary boot source.
 */
#define AST_2ND_BOOT_WDT		1

/*
 * Third Watchdog Timer by default is configured
 * to toggle Flash address mode switch before reset.
 */
#define AST_FLASH_ADDR_DETECT_WDT	2

#define DRAMC_CFG_REG		0x1e6e0004
#define   DRAMC_VRAM_SIZE	GENMASK(3, 2)
#define   DRAMC_DRAM_SIZE	GENMASK(1, 0)
#define   DRAMC_ECC_EN		BIT(7)
#define DRAMC_ECC_RANGE_REG	0x1e6e0054

#define DRAM_SIZE_GRANULARITY	(128 * 1024 * 1024)
#define VRAM_SIZE_GRANULARITY	(8 * 1024 * 1024)

DECLARE_GLOBAL_DATA_PTR;

int board_init(void)
{
	gd->bd->bi_boot_params = CFG_SYS_SDRAM_BASE + 0x100;

	return 0;
}

static u32 get_dram_size(void)
{
	u32 size_conf = FIELD_GET(DRAMC_DRAM_SIZE, readl(DRAMC_CFG_REG));

	return (1 << size_conf) * DRAM_SIZE_GRANULARITY;
}

static u32 get_vram_size(void)
{
	u32 size_conf = FIELD_GET(DRAMC_VRAM_SIZE, readl(DRAMC_CFG_REG));

	return (1 << size_conf) * VRAM_SIZE_GRANULARITY;
}

static bool is_ecc_on(void)
{
	u32 ecc_status = FIELD_GET(DRAMC_ECC_EN, readl(DRAMC_CFG_REG));

	return !!ecc_status;
}

static u32 get_ecc_size(void)
{
	if (is_ecc_on())
		return readl(DRAMC_ECC_RANGE_REG) + (1 << 20);
	else
		return 0;
}

int dram_init(void)
{
	if (IS_ENABLED(CONFIG_RAM)) {
		struct udevice *dev;
		struct ram_info ram;
		int ret;

		ret = uclass_get_device(UCLASS_RAM, 0, &dev);
		if (ret) {
			debug("DRAM FAIL1\r\n");
			return ret;
		}

		ret = ram_get_info(dev, &ram);
		if (ret) {
			debug("DRAM FAIL2\r\n");
			return ret;
		}

		gd->ram_size = ram.size;
	} else {
		u32 vga = get_vram_size();
		u32 dram = get_dram_size();

		if (IS_ENABLED(CONFIG_ARCH_FIXUP_FDT_MEMORY)) {
			/*
			 * U-boot will fixup the memory node in kernel's DT.
			 * The ECC redundancy is unable to handle now, just
			 * report the ECC size as the ram size.
			 */
			if (is_ecc_on())
				gd->ram_size = get_ecc_size();
			else
				gd->ram_size = dram - vga;
		} else {
			/*
			 * Report the memory size regardless the ECC redundancy,
			 * let kernel handle the ram paritions
			 */
			gd->ram_size = dram - vga;
		}
	}

	return 0;
}

void board_add_ram_info(int use_default)
{
	u32 act_size = get_dram_size() >> 20;
	u32 vga_rsvd = get_vram_size() >> 20;
	u32 ecc_size = get_ecc_size() >> 20;
	bool ecc_on = is_ecc_on();

	printf(" (capacity:%d MiB, VGA:%d MiB, ECC:%s", act_size, vga_rsvd,
	       ecc_on ? "on" : "off");

	if (ecc_on)
		printf(", ECC size:%d MiB", ecc_size);

	printf(")");
}
