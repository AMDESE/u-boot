/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) ASPEED Technology Inc.
 * Chin-Ting Kuo <chin-ting_kuo@aspeedtech.com>
 *
 */

#ifndef _ASPEED_ABR_H_
#define _ASPEED_ABR_H_

#include <asm/arch-aspeed/wdt.h>
#include <linux/types.h>

#define ABR_REG			(ASPEED_IO_SCU_BASE + 0x030)
#define ABR_EN			BIT(0)
#define ABR_MODE		BIT(29)

bool abr_enabled(void);
u32 abr_get_indicator(void);

#endif
