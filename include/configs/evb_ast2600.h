/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) Aspeed Technology Inc.
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <configs/aspeed-common.h>

#define CFG_SYS_UBOOT_BASE		CONFIG_TEXT_BASE

/* Misc */
#define STR_HELPER(s)	#s
#define STR(s)		STR_HELPER(s)

#define CFG_EXTRA_ENV_SETTINGS \
	"loadaddr=" STR(CONFIG_SYS_LOAD_ADDR) "\0" \
	"bootspi=fdt addr 20100000 && fdt header get fitsize totalsize && " \
	"cp.b 20100000 ${loadaddr} ${fitsize} && bootm; " \
	"echo Error loading kernel FIT image\0" \
	""

#if CONFIG_IS_ENABLED(OPTEE_IMAGE)
#define CONFIG_ARMV7_SECURE_BASE	0x88000000
#endif

#endif	/* __CONFIG_H */
