/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) ASPEED Technology Inc.
 * Chin-Ting Kuo <chin-ting_kuo@aspeedtech.com>
 *
 */

#ifndef _ASPEED_EXTRST_H_
#define _ASPEED_EXTRST_H_

#include <asm/arch/platform.h>
#include <linux/types.h>

#define SCU0_EXTRST_MASK_1_VAL	0x8207e771
#define SCU0_EXTRST_MASK_2_VAL	0x000003f6

#define SCU1_EXTRST_MASK_1_VAL	0x000093ec
#define SCU1_EXTRST_MASK_2_VAL	0x40303801
#define SCU1_EXTRST_MASK_3_VAL	0x003e0000

int extrst_mask_init(void);

#endif
