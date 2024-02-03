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

#define INTR_CTRL			(ASPEED_FMC_REG_BASE + 0x008)
#define DRAM_HI_ADDR			(ASPEED_FMC_REG_BASE + 0x07C)
#define DMA_CTRL			(ASPEED_FMC_REG_BASE + 0x080)
#define DMA_FLASH_ADDR			(ASPEED_FMC_REG_BASE + 0x084)
#define DMA_RAM_ADDR			(ASPEED_FMC_REG_BASE + 0x088)
#define DMA_LEN				(ASPEED_FMC_REG_BASE + 0x08C)

#define SPI_DMA_MAX_LEN			0x2000000

#define SPI_DMA_DONE			BIT(11)
#define DMA_ENABLE			BIT(0)
u32 aspeed_spi_abr_offset(void);

#endif
