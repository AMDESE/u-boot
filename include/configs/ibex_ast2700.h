/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) Aspeed Technology Inc.
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#define CFG_SYS_SDRAM_BASE	0x80000000

#define CFG_SYS_UBOOT_BASE		CONFIG_TEXT_BASE
#if defined(CONFIG_ASPEED_FPGA)
#define CFG_SYS_TIMER_RATE		(24000000)
#else
#define CFG_SYS_TIMER_RATE		(200000000)
#endif

#endif	/* __CONFIG_H */
