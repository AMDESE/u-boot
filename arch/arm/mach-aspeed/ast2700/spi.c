// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */

#include <asm/arch-aspeed/spi.h>
#include <asm/io.h>
#include <common.h>
#include <malloc.h>

size_t aspeed_memmove_dma_op(void *dest, const void *src, size_t count)
{
	u32 dma_busy = 0;
	u64 dma_dest = (u64)dest;
	u64 dma_src = (u64)src;
	u32 remaining = (u32)count;
	u32 dma_len = (u32)count;

	if (dma_dest == dma_src)
		return 0;

	if (dma_dest % 4 != 0 || dma_src % 4 != 0)
		return 0;

	if (dma_src >= ASPEED_FMC_CS0_BASE &&
	    dma_src + dma_len < (ASPEED_FMC_CS0_BASE + ASPEED_FMC_CS0_SIZE) &&
	    dma_dest >= ASPEED_DRAM_BASE) {
		while (remaining > 0) {
			if (dma_len > SPI_DMA_MAX_LEN)
				dma_len = SPI_DMA_MAX_LEN;
			else
				dma_len = remaining;

			if (dma_dest >= ASPEED_DRAM_BASE)
				writel(0x4, (void *)DRAM_HI_ADDR);

			writel((u32)(dma_dest - ASPEED_DRAM_BASE),
			       (void *)DMA_RAM_ADDR);
			writel((u32)(dma_src - ASPEED_FMC_CS0_BASE),
			       (void *)DMA_FLASH_ADDR);
			writel(dma_len - 1, (void *)DMA_LEN);

			writel(DMA_ENABLE, (void *)DMA_CTRL);

			do {
				dma_busy = readl((void *)INTR_CTRL) & SPI_DMA_DONE;
			} while (dma_busy == 0);

			writel(0x0, (void *)DMA_CTRL);
			dma_dest += dma_len;
			dma_src += dma_len;
			remaining -= dma_len;
		}

		return count;
	}
	return 0;
}

