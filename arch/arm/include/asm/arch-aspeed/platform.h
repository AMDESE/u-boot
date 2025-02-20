/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) ASPEED Technology Inc.
 * Ryan Chen <ryan_chen@aspeedtech.com>
 *
 */

#ifndef _ASPEED_PLATFORM_H_
#define _ASPEED_PLATFORM_H_

#define AST_PLL_25MHZ			25000000
#define AST_PLL_24MHZ			24000000
#define AST_PLL_12MHZ			12000000

/*********************************************************************************/
#if defined(CONFIG_ASPEED_AST2400)
#define ASPEED_MAC_COUNT	2
#define ASPEED_HW_STRAP1	0x1e6e2070
#define ASPEED_REVISION_ID	0x1e6e207C
#define ASPEED_SYS_RESET_CTRL	0x1e6e203C
#define ASPEED_VGA_HANDSHAKE0	0x1e6e2040	/*	VGA function handshake register */
#define ASPEED_DRAM_BASE	0x40000000
#define ASPEED_SRAM_BASE	0x1E720000
#define ASPEED_SRAM_SIZE	0x8000
#define ASPEED_FMC_CS0_BASE	0x20000000
#elif defined(CONFIG_ASPEED_AST2500)
#define ASPEED_MAC_COUNT	2
#define ASPEED_HW_STRAP1	0x1e6e2070
#define ASPEED_HW_STRAP2	0x1e6e20D0
#define ASPEED_REVISION_ID	0x1e6e207C
#define ASPEED_SYS_RESET_CTRL	0x1e6e203C
#define ASPEED_VGA_HANDSHAKE0	0x1e6e2040	/*	VGA function handshake register */
#define ASPEED_MAC_COUNT	2
#define ASPEED_DRAM_BASE	0x80000000
#define ASPEED_SRAM_BASE	0x1E720000
#define ASPEED_SRAM_SIZE	0x9000
#define ASPEED_FMC_CS0_BASE	0x20000000
#elif defined(CONFIG_ASPEED_AST2600)
#define ASPEED_FMC_WDT2		0x1e620064
#define ASPEED_SPI1_BOOT_CTRL	0x1e630064
#define ASPEED_MULTI_CTRL10	0x1e6e2438
#define ASPEED_HW_STRAP1	0x1e6e2500
#define ASPEED_HW_STRAP2	0x1e6e2510
#define ASPEED_REVISION_ID0	0x1e6e2004
#define ASPEED_REVISION_ID1	0x1e6e2014
#define ASPEED_EMMC_WDT_CTRL	0x1e6f20a0
#define ASPEED_SYS_RESET_CTRL	0x1e6e2064
#define ASPEED_SYS_RESET_CTRL3	0x1e6e206c
#define ASPEED_GPIO_YZ_DATA	0x1e7801e0
#define ASPEED_VGA_HANDSHAKE0	0x1e6e2100	/*	VGA function handshake register */
#define ASPEED_SB_STS		0x1e6f2014
#define ASPEED_OTP_QSR		0x1e6f2040
#define ASPEED_MAC_COUNT	4
#define ASPEED_DRAM_BASE	0x80000000
#define ASPEED_SRAM_BASE	0x10000000
#define ASPEED_SRAM_SIZE	0x16000
#define ASPEED_FMC_CS0_BASE	0x20000000
#elif defined(CONFIG_ASPEED_AST2700)
#define ASPEED_CPU_AHBC_BASE	0x12000000
#define ASPEED_CPU_REVISION_ID	0x12C02000
#define ASPEED_CPU_HW_STRAP1	0x12C02010
#define ASPEED_CPU_RESET_LOG1	0x12C02050
#define ASPEED_CPU_RESET_LOG2	0x12C02060
#define ASPEED_CPU_RESET_LOG3	0x12C02070
#define ASPEED_MAC_COUNT	3
#define ASPEED_DRAM_BASE	0x400000000
#define ASPEED_SRAM_BASE	0x10000000
#define ASPEED_SRAM_SIZE	0x20000
#define ASPEED_FMC_REG_BASE	0x14000000
#define ASPEED_FMC_CS0_BASE	0x100000000
#define ASPEED_FMC_CS0_SIZE	0x80000000
#define ASPEED_IO_AHBC_BASE	0x140b0000
#define ASPEED_IO_REVISION_ID	0x14C02000
#define ASPEED_IO_HW_STRAP1	0x14C02010
#define ASPEED_IO_RESET_LOG1	0x14C02050
#define ASPEED_IO_RESET_LOG2	0x14C02060
#define ASPEED_IO_RESET_LOG3	0x14C02070
#define ASPEED_IO_RESET_LOG4	0x14C02080
#else
#err "Unrecognized Aspeed platform."
#endif

#endif
