// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */

#include <asm/arch-aspeed/wdt.h>
#include <asm/io.h>
#include <common.h>

int wdt_init(void)
{
	u32 idx;
	u32 wdt_base_addr;

	for (idx = 0; idx < 8; idx++) {
		wdt_base_addr = ASPEED_WDT_BASE + idx * 0x80;
		/* SoC reset mask */
		writel(WDT_RST_MASK_1_VAL, (void *)(wdt_base_addr + WDT_RST_MASK_1));
		writel(WDT_RST_MASK_2_VAL, (void *)(wdt_base_addr + WDT_RST_MASK_2));
		writel(WDT_RST_MASK_3_VAL, (void *)(wdt_base_addr + WDT_RST_MASK_3));
		writel(WDT_RST_MASK_4_VAL, (void *)(wdt_base_addr + WDT_RST_MASK_4));
		writel(WDT_RST_MASK_5_VAL, (void *)(wdt_base_addr + WDT_RST_MASK_5));

		/* SW reset mask */
		writel(WDT_RST_MASK_1_VAL, (void *)(wdt_base_addr + WDT_SW_RST_MASK_1));
		writel(WDT_RST_MASK_2_VAL, (void *)(wdt_base_addr + WDT_SW_RST_MASK_2));
		writel(WDT_RST_MASK_3_VAL, (void *)(wdt_base_addr + WDT_SW_RST_MASK_3));
		writel(WDT_RST_MASK_4_VAL, (void *)(wdt_base_addr + WDT_SW_RST_MASK_4));
		writel(WDT_RST_MASK_5_VAL, (void *)(wdt_base_addr + WDT_SW_RST_MASK_5));
	}

	return 0;
}

