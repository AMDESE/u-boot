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

static uintptr_t get_remote_address(uintptr_t addr, int ltpi_idx)
{
	uintptr_t addr_lo = addr & GENMASK(25, 0);
	uintptr_t addr_hi = addr >> 26;
	uintptr_t remap_base = ASPEED_LTPI0_REMOTE_BASE;

	if (ltpi_idx)
		remap_base = ASPEED_LTPI1_REMOTE_BASE;

	if (addr_hi == LTPI_REMOTE_AST1700_IOD_SPACE)
		return (remap_base | (0x0 << 26) | addr_lo);
	else if (addr_hi == LTPI_REMOTE_AST1700_SPI2_SPACE)
		return (remap_base | (0x1 << 26) | addr_lo);
	else
		return addr;
}

static void ltpi_init_hardware(uintptr_t ltpi_base, uintptr_t ltpi_remote_base)
{
	uint32_t reg, remap = 0;
	bool is_ast1700 = false;

	// Tommy please help to delete this
	//printk("%s: %08lx %08lx\n", __func__, ltpi_base, ltpi_remote_base);
	// SPL console log:
	//ltpi_init_hardware: 14c34000 30c34000
	//ltpi_init_hardware: 14c35000 50c34000

	reg = readl((void *)ltpi_base + LTPI_LINK_MANAGE_ST_REG);

	/* Setup the remap rergister */
	is_ast1700 = !!(reg & LTPI_LINK_PARTNER_AST1700);
	if (is_ast1700)
		remap = FIELD_PREP(REMAP_ENTRY0, LTPI_REMOTE_AST1700_IOD_SPACE) |
			FIELD_PREP(REMAP_ENTRY1, LTPI_REMOTE_AST1700_SPI2_SPACE);
	writel(remap, (void *)ltpi_base + LTPI_ADDR_REMAP_REG0);

	if (!is_ast1700)
		return;

	/* Remap ready, it is okay to access the remote resource */
	/* Add I2C mux configuration here */
}

int ltpi_init(void)
{
	if (!(readl((void *)ASPEED_IO_HW_STRAP1) & SCU_IO_HWSTRAP_SCM))
		return 0;

	ltpi_init_hardware(ASPEED_LTPI0_BASE, get_remote_address(ASPEED_LTPI0_BASE, 0));
	ltpi_init_hardware(ASPEED_LTPI1_BASE, get_remote_address(ASPEED_LTPI0_BASE, 1));

	return 0;
}
