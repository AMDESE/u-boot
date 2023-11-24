/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) ASPEED Technology Inc.
 * Chin-Ting Kuo <chin-ting_kuo@aspeedtech.com>
 *
 */

#ifndef _ASPEED_RECOVERY_H_
#define _ASPEED_RECOVERY_H_

#include <asm/arch-aspeed/platform.h>
#include <asm/arch-aspeed/sdram_ast2700.h>
#include <linux/types.h>

bool is_recovery(void);
int aspeed_spl_ddr_image_ymodem_load(struct train_bin dwc_train[][2], int ddr4,
				     int i_mem, const int train2D);

#endif
