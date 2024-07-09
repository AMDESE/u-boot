// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright ASPEED Technology Inc.
 *
 */

#include <debug_uart.h>
#include <dm.h>
#include <spl.h>
#include <init.h>
#include <hang.h>
#include <image.h>
#include <linux/err.h>
#include <asm/io.h>
#include <asm/arch/scu_ast2600.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

#define AST_BOOTMODE_SPI    0
#define AST_BOOTMODE_EMMC   1

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
	if ((readl(ASPEED_IO_HW_STRAP1) & STRAP_BOOTMODE_BIT))
		return BOOT_DEVICE_MMC1;
	else
		return BOOT_DEVICE_RAM;
}

void *board_spl_fit_buffer_addr(ulong fit_size, int sectors, int bl_len)
{
	return spl_get_load_buffer(0, sectors * bl_len);
}

void board_fit_image_post_process(const void *fit, int node, void **p_image, size_t *p_size)
{
	uint8_t os;

	fit_image_get_os(fit, node, &os);

	/* skip if no TEE */
	if (os != IH_OS_TEE)
		return;
}
