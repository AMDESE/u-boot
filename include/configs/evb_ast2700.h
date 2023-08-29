/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <configs/aspeed-common.h>

#define CONFIG_SYS_MEMTEST_START	(CONFIG_SYS_SDRAM_BASE + 0x300000)
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_MEMTEST_START + 0x5000000)

#define CONFIG_SYS_UBOOT_BASE		CONFIG_TEXT_BASE

#define CFG_EXTRA_ENV_SETTINGS \
	"bootspi=fdt addr 1002a0000 && fdt header get fitsize totalsize && " \
	"cp.b 1002a0000 ${loadaddr} ${fitsize} && bootm; " \
	"echo Error loading kernel FIT image\0" \
	""

#endif	/* __CONFIG_H */
