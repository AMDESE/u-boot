// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */

#include <asm/arch/extrst.h>
#include <asm/arch/platform.h>
#include <asm/io.h>
#include <common.h>

int extrst_mask_init(void)
{
	u32 reg;

	/* only init EXTRST mask during power on reset */
	reg = readl((void *)ASPEED_CPU_RESET_LOG1);
	if (!(reg & BIT(0)))
		return 0;

	if (IS_ENABLED(CONFIG_EXTRST_MASK_DEFAULT_INIT)) {
		writel(SCU0_EXTRST_MASK_1_VAL, (void *)(ASPEED_CPU_SCU_BASE + 0x2F0));
		writel(SCU0_EXTRST_MASK_2_VAL, (void *)(ASPEED_CPU_SCU_BASE + 0x2F4));

		writel(SCU1_EXTRST_MASK_1_VAL, (void *)(ASPEED_IO_SCU_BASE + 0x2F0));
		writel(SCU1_EXTRST_MASK_2_VAL, (void *)(ASPEED_IO_SCU_BASE + 0x2F4));
		writel(SCU1_EXTRST_MASK_3_VAL, (void *)(ASPEED_IO_SCU_BASE + 0x2F8));
	}

	return 0;
}
