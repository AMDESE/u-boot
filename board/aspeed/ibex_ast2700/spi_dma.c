// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */

#include <asm/arch-aspeed/platform.h>
#include <asm/io.h>
#include <common.h>

#define INTR_CTRL	(ASPEED_FMC_REG_BASE + 0x008)
#define DRAM_HI_ADDR	(ASPEED_FMC_REG_BASE + 0x07C)
#define DMA_CTRL	(ASPEED_FMC_REG_BASE + 0x080)
#define DMA_FLASH_ADDR	(ASPEED_FMC_REG_BASE + 0x084)
#define DMA_RAM_ADDR	(ASPEED_FMC_REG_BASE + 0x088)
#define DMA_LEN		(ASPEED_FMC_REG_BASE + 0x08C)

#define SPI_DMA_DONE			BIT(11)
#define DMA_ENABLE			BIT(0)

size_t aspeed_memmove_dma_op(void *dest, const void *src, size_t count)
{
	u32 dma_busy = 0;

	if (dest == src)
		return 0;

	if ((u32)src >= ASPEED_FMC_CS0_BASE &&
	    (u32)src < (ASPEED_FMC_CS0_BASE + ASPEED_FMC_CS0_SIZE) &&
	    (u32)dest >= ASPEED_DRAM_BASE) {
		if ((u32)dest >= ASPEED_DRAM_BASE)
			writel(0x4, (void *)DRAM_HI_ADDR);

		writel((u32)dest - ASPEED_DRAM_BASE, (void *)DMA_RAM_ADDR);
		writel((u32)src - ASPEED_FMC_CS0_BASE, (void *)DMA_FLASH_ADDR);
		writel(count - 1, (void *)DMA_LEN);

		writel(DMA_ENABLE, (void *)DMA_CTRL);

		do {
			dma_busy = readl((void *)INTR_CTRL) & SPI_DMA_DONE;
		} while (dma_busy == 0);

		writel(0x0, (void *)DMA_CTRL);

		return count;
	}

	return 0;
}
