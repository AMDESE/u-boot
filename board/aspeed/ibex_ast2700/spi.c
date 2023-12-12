// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */

#include <asm/arch-aspeed/spi.h>
#include <asm/io.h>
#include <common.h>

int spi_init(void)
{
	u32 reg_val;

	reg_val = readl((void *)ASPEED_IO_FWSPI_DRIVING);
	reg_val |= 0x00000fff;
	writel(reg_val, (void *)ASPEED_IO_FWSPI_DRIVING);

	reg_val = readl((void *)ASPEED_IO_SPI0_DRIVING);
	reg_val |= 0x00000fff;
	writel(reg_val, (void *)ASPEED_IO_SPI0_DRIVING);

	reg_val = readl((void *)ASPEED_IO_SPI1_DRIVING);
	reg_val |= 0x0fff0000;
	writel(reg_val, (void *)ASPEED_IO_SPI1_DRIVING);

	reg_val = readl((void *)ASPEED_IO_SPI2_DRIVING);
	reg_val |= 0x00000fff;
	writel(reg_val, (void *)ASPEED_IO_SPI2_DRIVING);

	return 0;
}

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

	if ((u32)dest % 4 != 0 || (u32)src % 4 != 0)
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

int spi_load_image(u32 *src, u32 *dest, u32 len)
{
	const void *base = (const void *)(ASPEED_FMC_CS0_BASE + (u32)src);

	debug("spi load image base = %x\n", (u32)base);

	memcpy((void *)dest, base, len);

	return 0;
}
