// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright ASPEED Technology Inc.
 *
 */
#include <common.h>
#include <debug_uart.h>
#include <dm.h>
#include <spl.h>
#include <init.h>
#include <linux/err.h>
#include <asm/io.h>
#include <asm/arch/scu_ast2600.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

void board_init_f(ulong dummy)
{
	if (CONFIG_IS_ENABLED(OF_CONTROL)) {
		int ret;

		ret = spl_early_init();
		if (ret) {
			debug("spl_early_init() failed: %d\n", ret);
			hang();
		}
	}

	preloader_console_init();

	dram_init();
}

struct legacy_img_hdr *spl_get_load_buffer(ssize_t offset, size_t size)
{
	return (struct legacy_img_hdr *)(CONFIG_SYS_LOAD_ADDR);
}

/*
 * Try to detect the boot mode. Fallback to the default,
 * memory mapped SPI XIP booting if detection failed.
 */
u32 spl_boot_device(void)
{
	return BOOT_DEVICE_RAM;
}
