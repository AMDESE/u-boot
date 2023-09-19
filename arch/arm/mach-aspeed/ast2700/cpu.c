// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include <common.h>
#include <asm/armv8/mmu.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <env.h>
#include <env_internal.h>

DECLARE_GLOBAL_DATA_PTR;

static struct mm_region aspeed2700_mem_map[] = {
		{
				.virt =  0x0UL,
				.phys =  0x0UL,
				.size =  0x40000000UL,
				.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
					 PTE_BLOCK_NON_SHARE |
					 PTE_BLOCK_PXN | PTE_BLOCK_UXN
		},
		{
				.virt =  0x40000000UL,
				.phys =  0x40000000UL,
				.size = 0x2C0000000UL,
				.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
					 PTE_BLOCK_NON_SHARE
		},
		{
				.virt = 0x400000000UL,
				.phys = 0x400000000UL,
				.size = 0x200000000UL,
				.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
					 PTE_BLOCK_INNER_SHARE
		},
		{
				/* List terminator */
				0,
		}
};

struct mm_region *mem_map = aspeed2700_mem_map;

u64 get_page_table_size(void)
{
	return 0x80000;
}

enum env_location env_get_location(enum env_operation op, int prio)
{
	enum env_location env_loc = ENVL_UNKNOWN;

	if (prio)
		return env_loc;

	if ((readl(ASPEED_CPU_HW_STRAP1) & STRAP_BOOTMODE_BIT))
		env_loc =  ENVL_MMC;
	else
		env_loc =  ENVL_SPI_FLASH;

	return env_loc;
}
