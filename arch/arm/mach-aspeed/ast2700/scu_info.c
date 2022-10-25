// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 * Ryan Chen <ryan_chen@aspeedtech.com>
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/arch/aspeed_scu_info.h>

/* SoC mapping Table */
#define SOC_ID(str, rev) { .name = str, .rev_id = rev, }

struct soc_id {
	const char *name;
	u64 rev_id;
};

static struct soc_id soc_map_table[] = {
	SOC_ID("AST2700-A0", 0x0500030305000303),
};

void aspeed_print_soc_id(void)
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

int aspeed_get_mac_phy_interface(u8 num)
{
	return 0;
}

void aspeed_print_security_info(void)
{
}

void aspeed_print_sysrst_info(void)
{
}

void aspeed_print_dram_initializer(void)
{
}

void aspeed_print_2nd_wdt_mode(void)
{
}

void aspeed_print_fmc_aux_ctrl(void)
{
}

void aspeed_print_spi1_abr_mode(void)
{
}

void aspeed_print_spi1_aux_ctrl(void)
{
}

void aspeed_print_spi_strap_mode(void)
{
}

void aspeed_print_espi_mode(void)
{
}

void aspeed_print_mac_info(void)
{
}
