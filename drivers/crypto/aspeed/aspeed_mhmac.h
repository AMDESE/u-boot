/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright 2021 ASPEED Technology Inc.
 */

#ifndef __ASPEED_MHMAC_H
#define __ASPEED_MHMAC_H

#include <clk.h>
#include <reset.h>

/********************************************************************
 *                 SEC-MHMAC register definition                    *
 ********************************************************************/
/* MHMAC register offset definition */
#define MHMAC_CTRL_OFFSET (0x300)
#define MHMAC_REG_PROT_OFFSET (0x304)
#define MHMAC_RESERVE_OFFSET (0x308)
#define MHMAC_REG_MID_OFFSET (0x30C)
#define MHMAC_ZONE0_RANGE_OFFSET (0x310)
#define MHMAC_ZONE1_RANGE_OFFSET (0x314)
#define MHMAC_ZONE2_RANGE_OFFSET (0x318)
#define MHMAC_ZONE3_RANGE_OFFSET (0x31C)
#define MHMAC_ZONE0_MIDS_OFFSET (0x320)
#define MHMAC_ZONE1_MIDS_OFFSET (0x324)
#define MHMAC_ZONE2_MIDS_OFFSET (0x328)
#define MHMAC_ZONE3_MIDS_OFFSET (0x32C)
#define MHMAC_INT_EN_OFFSET (0x330)
#define MHMAC_INT_ST_OFFSET (0x334)
#define MHMAC_ZONE0_KEY_OFFSET (0x340)
#define MHMAC_ZONE1_KEY_OFFSET (0x360)
#define MHMAC_ZONE2_KEY_OFFSET (0x380)
#define MHMAC_ZONE3_KEY_OFFSET (0x3A0)

/* MHMAC control register bit definition */
#define MHMAC_CTRL_CLEAN_CACHE BIT(0)
#define MHMAC_CTRL_REGION_NUMBER_SEL_MASK GENMASK(3, 2)
#define MHMAC_CTRL_ZONE_0_EN_BIT (4)
#define MHMAC_CTRL_ZONE_ENABLE(_zone) BIT(MHMAC_CTRL_ZONE_0_EN_BIT + (_zone))

/* MHMAC mid register bit definition */
#define MHMAC_MID_PROT_EN BIT(4)
#define MHMAC_REG_MID_PROT_EN BIT(5)

/* MHMAC zone control register bit definition */
#define MHMAC_ZONE_RANGE_ADDR_MASK (GENMASK(17, 0))
#define MHMAC_ZONE_RANGE_SIZE_MASK (GENMASK(31, 18))

/* MHMAC zone mid register bit definition */
#define MHMAC_ZONE_MIDS_MID0_MASK GENMASK(5, 0)
#define MHMAC_ZONE_MIDS_MID1_MASK GENMASK(11, 6)
#define MHMAC_ZONE_MIDS_MID2_MASK GENMASK(17, 12)
#define MHMAC_ZONE_MIDS_MID3_MASK GENMASK(23, 18)
#define MHMAC_ZONE_MIDS_MID0_SEC BIT(24)
#define MHMAC_ZONE_MIDS_MID1_SEC BIT(25)
#define MHMAC_ZONE_MIDS_MID2_SEC BIT(26)
#define MHMAC_ZONE_MIDS_MID3_SEC BIT(27)

/* MHMAC interrupt control register bit definition */
#define MHMAC_CTRL_ZONE_0_IRQ_BIT (0)
#define MHMAC_CTRL_ZONE_INT_EN(_zone) BIT(MHMAC_CTRL_ZONE_0_IRQ_BIT + (_zone))

/* MHMAC zone configuration software defined macro */
#define MHMAC_MAXIMUM_ZONE_NUMBER (4)
#define MHMAC_ZONE_0_REMAP_ADDR (0xC0000000)
#define MHMAC_ZONE_1_REMAP_ADDR (0xD0000000)
#define MHMAC_ZONE_2_REMAP_ADDR (0xC8000000)
#define MHMAC_ZONE_3_REMAP_ADDR (0xD8000000)
#define MHMAC_FULL_ZONE_SIZE BIT(29)
#define MHMAX_MAX_ZONE_NUMBER(_sel) (1 << ((_sel) - 1))
#define MHMAC_MAX_ZONE_SIZE(_sel) \
	(MHMAC_FULL_ZONE_SIZE / MHMAX_MAX_ZONE_NUMBER((_sel)))
#define MHMAC_ZONE_ALIGN_SIZE BIT(16)
#define MHMAC_ZONE_ALIGN_MASK GENMASK(15, 0)
#define MHMAC_ZONE_UNIT_DATA_SIZE (64)
#define MHMAC_ZONE_UNIT_CHECKSUM_SIZE (32)
#define MHMAC_ZONE_UNIT_SIZE \
	(MHMAC_ZONE_UNIT_DATA_SIZE + MHMAC_ZONE_UNIT_CHECKSUM_SIZE)
#define MHMAC_SIZE_TO_DRAM_SIZE(_mhmac_size) \
	(((_mhmac_size) / MHMAC_ZONE_UNIT_DATA_SIZE) * MHMAC_ZONE_UNIT_SIZE)
#define DRAM_SIZE_TO_MHMAC_SIZE(_dram_size) \
	(((_dram_size) / MHMAC_ZONE_UNIT_SIZE) * MHMAC_ZONE_UNIT_DATA_SIZE)
#define MHMAC_INTERNAL_CACHE_SIZE (64 * 7)

/* MHMAC driver software defined macro */
#define MHMAC_KEY_LEN_BYTES (32)
#define MHMAC_DTS_REGION_CFG_NUM (2)
#define MHMAC_MAXMUM_MASTER (4)

/********************************************************************
 *                      SCU register definition                     *
 ********************************************************************/
/* SCU1 register offset definition */
#define MHMAC_SCU_RNG_OFFSET (0x0F4)
#define MHMAC_SCU_REMAP_OFFSET (0x110)

/* BootMCU remap bit definition */
#define MHMAC_BOOTMCU_REMAP_MASK GENMASK(22, 16)
#define MHMAC_MOST_BYTE_SHIFT (28)

/********************************************************************
 *                 SEC-MHMAC DEBUG feature macro                    *
 ********************************************************************/
#define MHMAC_DEBUG (0)
#if MHMAC_DEBUG
#define aspeed_mhmac_debug(_fmt, ...) \
	printf("[%s@%d][DBG] " _fmt, __func__, __LINE__, __VA_ARGS__)
#else
#define aspeed_mhmac_debug(...)
#endif

#if MHMAC_DEBUG
#define aspeed_mhmac_debug_hexdump(_pre, _buf, _len)                     \
	do {                                                             \
		int i = 0;                                               \
		uint8_t *__buf = (uint8_t *)(_buf);                      \
		printf("[%s@%d][DBG] %s\n", __func__, __LINE__, (_pre)); \
		for (i = 0; i < (_len); i++) {                           \
			if (i % 16 == 0) {                               \
				printf("[%s@%d][DBG] 0x%02x ", __func__, \
				       __LINE__, *(__buf + i));          \
			} else {                                         \
				printf("0x%02x%c", *(__buf + i),         \
				       (i % 16 == 15 ? '\n' : ' '));     \
			}                                                \
		}                                                        \
	} while (0)
#else
#define aspeed_mhmac_debug_hexdump(_pre, _buf, _len)
#endif

/********************************************************************
 *                 MHMAC software defined structure                 *
 ********************************************************************/
struct aspeed_mhmac_master_info {
	char *name;
	uint8_t master_id[4];
};

struct aspeed_mhmac_region {
	uint32_t dram_base;
	uint32_t dram_size;
	uint32_t mhmac_base;
	uint32_t mhmac_size;
	uint8_t region_num;
	uint8_t *master;
	uint8_t hmac_key[MHMAC_KEY_LEN_BYTES];
};

struct aspeed_mhmac_config {
	void *base;
	void *scu_base;
	uint32_t region_sel;
	struct clk hace_clk;
	struct reset_ctl hace_rst;
	struct aspeed_mhmac_region region[MHMAC_MAXIMUM_ZONE_NUMBER];
};

enum {
	/* Normal world master */
	CA35_W_NS = 0x0,
	CA35_R_NS = 0x1,
	CM4_SSP_I = 0x2,
	CM4_SSP_D = 0x3,
	CM4_SSP_S = 0x4,
	CM4_TSP = 0xb,
	MCU0_I = 0x20,
	MCU0_D = 0x21,
	/* Secure world master */
	CA35_W_S = 0x40,
	CA35_R_S = 0x41,
	/* Special purpose configuration */
	NO_MASTER = 0xfe,
	MASTER_ALL = 0xff,
};

#endif //__ASPEED_MHMAC_H
