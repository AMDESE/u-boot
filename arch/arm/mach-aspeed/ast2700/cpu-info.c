// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 * Ryan Chen <ryan_chen@aspeedtech.com>
 */

#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <asm/arch/platform.h>

/* SoC mapping Table */
#define SOC_ID(str, rev) { .name = str, .rev_id = rev, }

struct soc_id {
	const char *name;
	u64 rev_id;
};

static struct soc_id soc_map_table[] = {
	SOC_ID("AST2700-A0", 0x0600030306000303),
};

void ast2700_print_soc_id(void)
{
	int i;
	u64 rev_id;

	rev_id = readl(ASPEED_REVISION_ID0);
	rev_id = ((u64)readl(ASPEED_REVISION_ID1) << 32) | rev_id;

	for (i = 0; i < ARRAY_SIZE(soc_map_table); i++) {
		if (rev_id == soc_map_table[i].rev_id)
			break;
	}
	if (i == ARRAY_SIZE(soc_map_table))
		printf("UnKnow-SOC: %llx\n", rev_id);
	else
		printf("SOC: %4s\n", soc_map_table[i].name);
}

#define SYS_DRAM_ECCRST	BIT(3)
#define SYS_ABRRST		BIT(2)
#define SYS_EXTRST		BIT(1)
#define SYS_SRST		BIT(0)

#define WDT_RST_BIT_MASK(s)	(GENMASK(3, 0) << (s))
#define BIT_WDT_SOC(s)		(BIT(0) << (s))
#define BIT_WDT_FULL(s)		(BIT(1) << (s))
#define BIT_WDT_ARM(s)		(BIT(2) << (s))
#define BIT_WDT_SW(s)		(BIT(3) << (s))

void ast2700_print_wdtrst_info(void)
{
	u32 wdt_rst = readl(ASPEED_IO_RESET_LOG4);
	int i;

	for (i = 0; i < 8; i++) {
		if (wdt_rst & WDT_RST_BIT_MASK(i * 4)) {
			printf("RST: WDT%d ", i);
			if (wdt_rst & BIT_WDT_SOC(i * 4)) {
				printf("SOC ");
				writel(BIT_WDT_SOC(i * 4), ASPEED_IO_RESET_LOG4);
			}
			if (wdt_rst & BIT_WDT_FULL(i * 4)) {
				printf("FULL ");
				writel(BIT_WDT_FULL(i * 4), ASPEED_IO_RESET_LOG4);
			}
			if (wdt_rst & BIT_WDT_ARM(i * 4)) {
				printf("ARM ");
				writel(BIT_WDT_ARM(i * 4), ASPEED_IO_RESET_LOG4);
			}
			if (wdt_rst & BIT_WDT_SW(i * 4)) {
				printf("SW ");
				writel(BIT_WDT_SW(i * 4), ASPEED_IO_RESET_LOG4);
			}
			printf("\n");
		}
	}
}

#define SYS_DRAM_ECCRST	BIT(3)
#define SYS_ABRRST		BIT(2)
#define SYS_EXTRST		BIT(1)
#define SYS_SRST		BIT(0)

void ast2700_print_sysrst_info(void)
{
	u32 sys_rst = readl(ASPEED_IO_RESET_LOG1);

	if (sys_rst & SYS_SRST) {
		printf("RST: Power On\n");
		writel(sys_rst, ASPEED_IO_RESET_LOG1);
	} else {
		if (sys_rst & SYS_DRAM_ECCRST) {
			printf("RST: DRAM_ECC\n");
			writel(SYS_DRAM_ECCRST, ASPEED_IO_RESET_LOG1);
		}

		if (sys_rst & SYS_ABRRST) {
			printf("RST: FLASH_ABR\n");
			writel(SYS_ABRRST, ASPEED_IO_RESET_LOG1);
		}

		if (sys_rst & SYS_EXTRST) {
			printf("RST: EXTERNEL\n");
			writel(SYS_EXTRST, ASPEED_IO_RESET_LOG1);
		}

		ast2700_print_wdtrst_info();
	}
}

int print_cpuinfo(void)
{
	ast2700_print_soc_id();
	ast2700_print_sysrst_info();

	return 0;
}
