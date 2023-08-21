// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */

#include <common.h>

void aspeed_spi_fastcpy(u32 mem_addr, u32 spi_addr, u32 count);

size_t aspeed_memmove_dma_op(void *dest, const void *src, size_t count)
{
	if (dest == src)
		return 0;

	if (((u32)src >= ASPEED_FMC_CS0_BASE) &&
	    ((u32)src < (ASPEED_FMC_CS0_BASE + 0x10000000))) {
		count = ((count + 3) / 4) * 4;
		aspeed_spi_fastcpy((u32)dest, (u32)src, count);
		return count;
	}

	return 0;
}
