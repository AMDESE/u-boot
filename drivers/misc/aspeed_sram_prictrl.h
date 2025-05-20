/* SPDX-License-Identifier: GPL-2.0+
 *
 * Copyright (C) 2023 ASPEED Technology Inc.
 */
#ifndef __ASPEED_SRAM_PRICTRL_H__
#define __ASPEED_SRAM_PRICTRL_H__

/* CPU SRAM protect memory offset */
#define ESRAM_SPROT_CFG 0x100
#define ESRAM_SPROT_SIDG 0x110
#define ESRAM_SPROT_CTL 0x180
#define ESRAM_SPROT_ADR 0x1c0

/* IO SRAM protect register offset */
#define GSRAM_REG_SPROT_CFG 0x200
#define GSRAM_REG_SPROT_SIDG 0x210
#define GSRAM_REG_SPROT_CTL 0x280
#define GSRAM_REG_SPROT_ADR 0x2c0

/* IO SRAM protect memory offset */
#define GSRAM_SPROT_CFG 0x300
#define GSRAM_SPROT_SIDG 0x310
#define GSRAM_SPROT_CTL 0x380
#define GSRAM_SPROT_ADR 0x3c0

struct sprot_cfg_ast2700 {
	union {
		struct {
			uint8_t unit : 8;
			uint32_t reserved : 24;
		} b;
		uint32_t raw;
	};
};

struct sprot_sid_ast2700 {
	union {
		struct {
			uint8_t sid0 : 8;
			uint8_t sid1 : 8;
			uint8_t sid2 : 8;
			uint8_t sid3 : 8;
		} b;
		uint32_t raw;
	} sidg0;
	union {
		struct {
			uint8_t sid4 : 8;
			uint8_t sid5 : 8;
			uint8_t sid6 : 8;
			uint8_t sid7 : 8;
		} b;
		uint32_t raw;
	} sidg1;
};

struct sprot_region_enable {
	union {
		struct {
			uint8_t w_enable_sid0 : 1;
			uint8_t w_enable_sid1 : 1;
			uint8_t w_enable_sid2 : 1;
			uint8_t w_enable_sid3 : 1;
			uint8_t w_enable_sid4 : 1;
			uint8_t w_enable_sid5 : 1;
			uint8_t w_enable_sid6 : 1;
			uint8_t w_enable_sid7 : 1;
			uint8_t r_enable_sid0 : 1; //BIT8
			uint8_t r_enable_sid1 : 1; //BIT9
			uint8_t r_enable_sid2 : 1; //BIT10
			uint8_t r_enable_sid3 : 1; //BIT11
			uint8_t r_enable_sid4 : 1; //BIT12
			uint8_t r_enable_sid5 : 1; //BIT13
			uint8_t r_enable_sid6 : 1; //BIT14
			uint8_t r_enable_sid7 : 1; //BIT15
			uint16_t reserved : 16;
		} b;
		uint32_t raw;
	} ctrl;
};

struct sprot_region_enable_ast2700 {
	struct sprot_region_enable region0_enable;
	struct sprot_region_enable region1_enable;
	struct sprot_region_enable region2_enable;
	struct sprot_region_enable region3_enable;
	struct sprot_region_enable region4_enable;
	struct sprot_region_enable region5_enable;
	struct sprot_region_enable region6_enable;
	struct sprot_region_enable region7_enable;
	struct sprot_region_enable region8_enable;
	struct sprot_region_enable region9_enable;
	struct sprot_region_enable region10_enable;
	struct sprot_region_enable region11_enable;
	struct sprot_region_enable region12_enable;
	struct sprot_region_enable region13_enable;
	struct sprot_region_enable region14_enable;
	struct sprot_region_enable region15_enable;
};

struct sprot_region {
	union {
		struct {
			uint16_t start_address : 16;
			uint16_t size : 16;
		} b;
		uint32_t raw;
	};
};

struct sprot_addr_ast2700 {
	struct sprot_region region0;
	struct sprot_region region1;
	struct sprot_region region2;
	struct sprot_region region3;
	struct sprot_region region4;
	struct sprot_region region5;
	struct sprot_region region6;
	struct sprot_region region7;
	struct sprot_region region8;
	struct sprot_region region9;
	struct sprot_region region10;
	struct sprot_region region11;
	struct sprot_region region12;
	struct sprot_region region13;
	struct sprot_region region14;
	struct sprot_region region15;
};

struct sram_prictrl_aspeed_config {
	uintptr_t esram_base;
	uint32_t esram_size;
	uintptr_t esram_ctrl_base;
	uintptr_t gsram_base;
	uint32_t gsram_size;
	uintptr_t gsram_ctrl_base;
};

#endif /* __ASPEED_SRAM_PRICTRL_H__ */
