/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) ASPEED Technology Inc.
 * Chin-Ting Kuo <chin-ting_kuo@aspeedtech.com>
 *
 */

#ifndef _ASPEED_SPI_H_
#define _ASPEED_SPI_H_

#include <asm/arch-aspeed/platform.h>
#include <linux/types.h>

#define ASPEED_FMC_CS0_SIZE		0x10000000

#define INTR_CTRL			(ASPEED_FMC_REG_BASE + 0x008)
#define DRAM_HI_ADDR			(ASPEED_FMC_REG_BASE + 0x07C)
#define DMA_CTRL			(ASPEED_FMC_REG_BASE + 0x080)
#define DMA_FLASH_ADDR			(ASPEED_FMC_REG_BASE + 0x084)
#define DMA_RAM_ADDR			(ASPEED_FMC_REG_BASE + 0x088)
#define DMA_LEN				(ASPEED_FMC_REG_BASE + 0x08C)

#define SPI_DMA_DONE			BIT(11)
#define DMA_ENABLE			BIT(0)

#define ASPEED_IO_FWSPI_DRIVING		(ASPEED_IO_SCU_BASE + 0x4E0)
#define ASPEED_IO_SPI0_DRIVING		(ASPEED_IO_SCU_BASE + 0x4CC)
#define ASPEED_IO_SPI1_DRIVING		(ASPEED_IO_SCU_BASE + 0x4CC)
#define ASPEED_IO_SPI2_DRIVING		(ASPEED_IO_SCU_BASE + 0x4D0)

#define SCU_SPI_ABR_REG			(ASPEED_IO_SCU_BASE + 0x030)
#define SPI_ABR_MODE			BIT(29)
#define SPI_ABR_EN			BIT(0)

#define WDTA_BASE			0x14c37400
#define WDT_ABR_CTRL			(WDTA_BASE + 0x04c)
#define WDT_ABR_INDICATOR		BIT(1)

#define SNOR_SZ_UNSET			0x0
#define SNOR_SZ_8MB			0x800000
#define SNOR_SZ_16MB			0x1000000
#define SNOR_SZ_32MB			0x2000000
#define SNOR_SZ_64MB			0x4000000
#define SNOR_SZ_128MB			0x8000000
#define SNOR_SZ_256MB			0x10000000
#define SNOR_SZ_512MB			0x20000000

enum spi_abr_mode {
	DUAL_FLASH_ABR = 0,
	SINGLE_FLASH_ABR,
};

int spi_init(void);
int spi_load_image(u32 *src, u32 *dest, u32 len);
u32 aspeed_spi_abr_offset(void);

#endif
