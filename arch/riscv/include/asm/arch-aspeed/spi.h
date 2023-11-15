/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) ASPEED Technology Inc.
 * Chin-Ting Kuo <chin-ting_kuo@aspeedtech.com>
 *
 */

#ifndef _ASPEED_SPI_H_
#define _ASPEED_SPI_H_

#include <asm/arch-aspeed/platform.h>

#define ASPEED_FMC_CS0_SIZE		0x10000000

#define ASPEED_IO_FWSPI_DRIVING		(ASPEED_IO_SCU_BASE + 0x4E0)
#define ASPEED_IO_SPI0_DRIVING		(ASPEED_IO_SCU_BASE + 0x4CC)
#define ASPEED_IO_SPI1_DRIVING		(ASPEED_IO_SCU_BASE + 0x4CC)
#define ASPEED_IO_SPI2_DRIVING		(ASPEED_IO_SCU_BASE + 0x4D0)

#endif
