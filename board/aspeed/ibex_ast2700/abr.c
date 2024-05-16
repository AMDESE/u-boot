// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */

#include <asm/arch-aspeed/platform.h>
#include <asm/arch-aspeed/wdt.h>
#include <asm/arch-aspeed/abr.h>
#include <asm/io.h>
#include <common.h>

bool abr_enabled(void)
{
	return (readl((void *)ABR_REG) & ABR_EN);
}

u32 abr_get_indicator(void)
{
	u32 val;

	val = !!(readl((void *)(ASPEED_WDTA_BASE + WDT_ABR_CTRL)) & WDT_ABR_INDICATOR);

	return val;
}
