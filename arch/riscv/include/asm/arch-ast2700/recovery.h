/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) ASPEED Technology Inc.
 * Chin-Ting Kuo <chin-ting_kuo@aspeedtech.com>
 *
 */

#ifndef _ASPEED_RECOVERY_H_
#define _ASPEED_RECOVERY_H_

#include <asm/arch/platform.h>
#include <asm/arch/sdram_ast2700.h>
#include <linux/types.h>

#define SPL_RECOVERY_IMAGE_MAX_SZ	(64 * 1024)

enum recovery_ddr_type {
	TYPE_DDR5,
	TYPE_DDR4,
};

enum recovery_ddr_mem_type {
	DDR_D_MEM,
	DDR_I_MEM,
};

bool is_recovery(void);
int aspeed_spl_recovery_load_dp(u32 addr);
int aspeed_spl_recovery_ddr_image(u32 dest, enum recovery_ddr_type type,
				  enum recovery_ddr_mem_type mem_type,
				  const int train2D);

/* recovery from uart */
bool recovery_from_uart(void);
int aspeed_spl_dp_image_ymodem_load(u32 addr);
int aspeed_spl_ddr_image_ymodem_load(u32 dest, enum recovery_ddr_type ddr_type,
				     enum recovery_ddr_mem_type mem_type,
				     const int train2D);

#endif
