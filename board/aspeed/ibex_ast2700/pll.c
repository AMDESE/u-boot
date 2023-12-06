// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) ASPEED Technology Inc.
 */
#include <common.h>
#include <dm.h>
#include <asm/arch-aspeed/sdram_ast2700.h>
#include <asm/arch-aspeed/platform.h>

#define MPLL_RESET	BIT(25)
#define MPLL_BYPASS	BIT(24)
#define MPLL_OFF	BIT(23)
#define MPLL_LOCK	BIT(31)

int mpll_init(void)
{
	u32 mpll = 0, rate;

	if (IS_ENABLED(CONFIG_ASPEED_MPLL_800))
		rate = 800;
	else if (IS_ENABLED(CONFIG_ASPEED_MPLL_1000))
		rate = 1000;
	else if (IS_ENABLED(CONFIG_ASPEED_MPLL_1200))
		rate = 1200;
	else if (IS_ENABLED(CONFIG_ASPEED_MPLL_1400))
		rate = 1400;
	else
		return 0;

	printf("%s %d\n", __func__, rate);

	switch (rate) {
	case 1600:
		mpll |= 0x40;
		break;
	case 1500:
		mpll |= 0x3c;
		break;
	case 1400:
		mpll |= 0x38;
		break;
	case 1300:
		mpll |= 0x34;
		break;
	case 1200:
		mpll |= 0x30;
		break;
	case 1100:
		mpll |= 0x2c;
		break;
	case 1000:
		mpll |= 0x28;
		break;
	case 900:
		mpll |= 0x24;
		break;
	case 800:
		mpll |= 0x20;
		break;
	default:
		printf("error input rate %d\n", rate);
		return 1;
	}

	writel(MPLL_RESET | MPLL_BYPASS, (void *)ASPEED_CPU_MPLL);

	writel(mpll | MPLL_BYPASS, (void *)ASPEED_CPU_MPLL);

	writel(mpll, (void *)ASPEED_CPU_MPLL);

	while (!(readl((void *)ASPEED_CPU_MPLL2) & MPLL_LOCK))
		;

	return 0;
}
