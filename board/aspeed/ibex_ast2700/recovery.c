// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2024 Aspeed Technology Inc.
 */

#include <asm/arch/platform.h>
#include <asm/arch/recovery.h>
#include <asm/arch/sdram_ast2700.h>
#include <asm/io.h>
#include <asm/u-boot.h>
#include <common.h>
#include <log.h>
#include <spl.h>
#include <xyzModem.h>

#define BUF_SIZE	(1024)
#define SCU1_HWSTRAP1	(ASPEED_IO_SCU_BASE + 0x010)
#define   RECOVERY_INTERFACE	GENMASK(27, 26)
#define   RECOVERY_BOOT_EN	BIT(4)

bool is_recovery(void)
{
	u32 recovery_pin = readl((void *)(SCU1_HWSTRAP1));

	return (recovery_pin & RECOVERY_BOOT_EN) ? true : false;
}

bool recovery_from_uart(void)
{
	u32 recovery_mode = readl((void *)(SCU1_HWSTRAP1));

	if (!(recovery_mode & RECOVERY_INTERFACE))
		return true;

	return false;
}

int aspeed_spl_recovery_load_dp(u32 addr)
{
	int ret = -1;

	if (recovery_from_uart())
		ret = aspeed_spl_dp_image_ymodem_load(addr);

	if (ret > SPL_RECOVERY_IMAGE_MAX_SZ) {
		memset((void *)addr, 0x0, ret);
		return -1;
	}

	return ret;
}

int aspeed_spl_recovery_ddr_image(u32 dest, enum recovery_ddr_type type,
				  enum recovery_ddr_mem_type mem_type,
				  const int train2D)
{
	int ret = -1;

	if (recovery_from_uart()) {
		ret = aspeed_spl_ddr_image_ymodem_load(dest, type,
						       mem_type, train2D);
	}

	if (ret > SPL_RECOVERY_IMAGE_MAX_SZ) {
		memset((void *)dest, 0x0, ret);
		return -1;
	}

	return ret;
}

