// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */
#include <asm/io.h>

#define ASPEED_SCU_CPU_BASE           0x12c02000

int board_early_init_f(void)
{
	if (IS_ENABLED(CONFIG_AVENUE_CITY_CRB)) {
		// Testing AST2700 DCSCM AvenueCity Video.
		// Disable e2m for pcie device.
		printf("AST2700 DCSCM disable PCIE E2M\n");
		writel(0, (void *)(ASPEED_SCU_CPU_BASE + 0xa60));
		writel(0, (void *)(ASPEED_SCU_CPU_BASE + 0xae0));
	}
	return 0;
}
