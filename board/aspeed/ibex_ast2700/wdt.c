// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */

#include <asm/arch/platform.h>
#include <asm/arch/wdt.h>
#include <asm/io.h>
#include <common.h>

int wdt_init(void)
{
	u32 idx;
	u32 wdt_base_addr;
	u32 project_id;

	project_id = readl((void *)ASPEED_IO_SCU_BASE);
	if (project_id & BIT(16)) {
		/* ast2700a1 */
		for (idx = 0; idx < 8; idx++) {
			wdt_base_addr = ASPEED_WDT_BASE + idx * 0x80;
			/* SoC reset mask */
			writel(0x8207e771, (void *)(wdt_base_addr + WDT_RST_MASK_1));
			writel(0x000003f6, (void *)(wdt_base_addr + WDT_RST_MASK_2));
			writel(0x000093ec, (void *)(wdt_base_addr + WDT_RST_MASK_3));
			writel(0x40303803, (void *)(wdt_base_addr + WDT_RST_MASK_4));
			writel(0x003e0000, (void *)(wdt_base_addr + WDT_RST_MASK_5));

			/* SW reset mask */
			writel(0x8207e771, (void *)(wdt_base_addr + WDT_SW_RST_MASK_1));
			writel(0x000003f6, (void *)(wdt_base_addr + WDT_SW_RST_MASK_2));
			writel(0x000093ec, (void *)(wdt_base_addr + WDT_SW_RST_MASK_3));
			writel(0x40303803, (void *)(wdt_base_addr + WDT_SW_RST_MASK_4));
			writel(0x003e0000, (void *)(wdt_base_addr + WDT_SW_RST_MASK_5));
		}
	} else {
		/* ast2700a0 */
		for (idx = 0; idx < 8; idx++) {
			wdt_base_addr = ASPEED_WDT_BASE + idx * 0x80;
			/* SoC reset mask */
			writel(0x00030421, (void *)(wdt_base_addr + WDT_RST_MASK_1));
			writel(0x00000036, (void *)(wdt_base_addr + WDT_RST_MASK_2));
			writel(0x000093ec, (void *)(wdt_base_addr + WDT_RST_MASK_3));
			writel(0x01303803, (void *)(wdt_base_addr + WDT_RST_MASK_4));
			writel(0x00000000, (void *)(wdt_base_addr + WDT_RST_MASK_5));

			/* SW reset mask */
			writel(0x00030421, (void *)(wdt_base_addr + WDT_SW_RST_MASK_1));
			writel(0x00000036, (void *)(wdt_base_addr + WDT_SW_RST_MASK_2));
			writel(0x000093ec, (void *)(wdt_base_addr + WDT_SW_RST_MASK_3));
			writel(0x01303803, (void *)(wdt_base_addr + WDT_SW_RST_MASK_4));
			writel(0x00000000, (void *)(wdt_base_addr + WDT_SW_RST_MASK_5));
		}
	}

	return 0;
}

