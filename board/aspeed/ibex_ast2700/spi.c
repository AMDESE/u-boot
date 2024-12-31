// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */

#include <asm/arch-aspeed/spi.h>
#include <asm/arch-aspeed/abr.h>
#include <asm/io.h>
#include <common.h>

enum spi_abr_mode get_spi_flash_abr_mode(void)
{
	u32 abr_mode;

	abr_mode = readl((void *)ABR_REG) & ABR_MODE;

	return (abr_mode != 0) ? SINGLE_FLASH_ABR : DUAL_FLASH_ABR;
}

/*
 * SCU flash size: SCU030[15:13]
 * 7: 512MB
 * 6: 256MB
 * 5: 128MB
 * 4: 64MB
 * 3: 32MB
 * 2: 16MB
 * 1: 8MB
 * 0: disabled
 */
u32 spi_get_flash_sz_strap(void)
{
	u32 scu_flash_sz;
	u32 flash_sz_phy;

	scu_flash_sz = (readl((void *)ABR_REG) >> 13) & 0x7;
	switch (scu_flash_sz) {
	case 0x7:
		flash_sz_phy = SNOR_SZ_512MB;
		break;
	case 0x6:
		flash_sz_phy = SNOR_SZ_256MB;
		break;
	case 0x5:
		flash_sz_phy = SNOR_SZ_128MB;
		break;
	case 0x4:
		flash_sz_phy = SNOR_SZ_64MB;
		break;
	case 0x3:
		flash_sz_phy = SNOR_SZ_32MB;
		break;
	case 0x2:
		flash_sz_phy = SNOR_SZ_16MB;
		break;
	case 0x1:
		flash_sz_phy = SNOR_SZ_8MB;
		break;
	case 0x0:
		flash_sz_phy = SNOR_SZ_UNSET;
		break;
	default:
		flash_sz_phy = SNOR_SZ_UNSET;
		break;
	}

	return flash_sz_phy;
}

u32 aspeed_spi_abr_offset(void)
{
	u32 flash_sz_strap;

	if (!abr_enabled())
		return 0;

	/* no need for dual flash ABR */
	if (get_spi_flash_abr_mode() == DUAL_FLASH_ABR)
		return 0;

	/* no need when ABR is not triggerred */
	if (abr_get_indicator() == 0)
		return 0;

	flash_sz_strap = spi_get_flash_sz_strap();

	if (flash_sz_strap == SNOR_SZ_512MB)
		return 0x40000000;

	return flash_sz_strap / 2;
}

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
