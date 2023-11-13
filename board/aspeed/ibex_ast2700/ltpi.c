// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */
#include <asm/io.h>
#include <dm.h>
#include <ram.h>
#include <spl.h>
#include <common.h>
#include <asm/csr.h>
#include <asm/arch-aspeed/platform.h>
#include <asm/arch-aspeed/scu_ast2700.h>
#include <asm/arch-aspeed/ltpi_ast2700.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/delay.h>

int ltpi_init(void)
{
	uint32_t reg = readl((void *)ASPEED_LTPI0_BASE + LTPI_LINK_MANAGE_ST);

	if (!(readl((void *)ASPEED_IO_HW_STRAP1) & SCU_IO_HWSTRAP_SCM))
		return 0;

	if (reg & LTPI_LINK_PARTNER_AST1700)
		reg = FIELD_PREP(REMAP_ENTRY0, LTPI_REMOTE_AST1700_IOD_SPACE);
	else
		reg = 0;

	writel(reg, (void *)ASPEED_LTPI0_BASE + LTPI_ADDR_REMAP_REG0);
	writel(reg, (void *)ASPEED_LTPI1_BASE + LTPI_ADDR_REMAP_REG0);

	return 0;
}
