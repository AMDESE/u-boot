/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright 2021 ASPEED Technology Inc.
 */

#ifndef __ASPEED_RSSS_H
#define __ASPEED_RSSS_H

#include <clk.h>
#include <reset.h>
#include <asm/types.h>
#include <u-boot/rsa.h>

enum aspeed_rsa_mode {
	ASPEED_RSSS_RSA_AHB_CPU_MODE,
	ASPEED_RSSS_RSA_AHB_ENGINE_MODE,
};

struct aspeed_engine_rsa {
	enum aspeed_rsa_mode mode;
	void __iomem *sram_exp;
	void __iomem *sram_mod;
	void __iomem *sram_data;
};

struct aspeed_rsss {
	void __iomem *base;
	struct clk clk;
	struct reset_ctl rst;
	struct aspeed_engine_rsa rsa_engine;
};

/* RSSS general definition */
#define ASPEED_RSSS_TIMEOUT		(100000)	/* 100ms */

/* RSSS register layout definition */
#define SRAM_BLOCK_SIZE			(0x400)

/* RSSS register layout offset definition */
#define SRAM_OFFSET_EXP			(0x0000)	/* RSSS RSA exponential sram */
#define SRAM_OFFSET_MOD			(0x0400)	/* RSSS RSA modular sram */
#define SRAM_OFFSET_DATA		(0x0800)	/* RSSS RSA data sram */
#define ASPEED_RSSS_INT_STS		(0x0C00)	/* RSSS interrupt status */
#define ASPEED_RSSS_INT_EN		(0x0C04)	/* RSSS interrupt enable */
#define ASPEED_RSSS_CTRL		(0x0C08)	/* RSSS generic control */
#define ASPEED_RSA_TRIGGER		(0x0E00)	/* RSA Engine Control: trigger */
#define ASPEED_RSA_KEY_INFO		(0x0E08)	/* RSA Exp/Mod Key Length (Bits) */
#define ASPEED_RSA_ENG_STS		(0x0E0c)	/* RSA Engine Status */

/* ASPEED_RSSS_INT_STS */
#define RSA_INT_DONE			BIT(0)

/* ASPEED_RSSS_INT_EN */
#define RSA_INT_EN			BIT(0)

/* ASPEED_RSSS_CTRL */
#define SRAM_AHB_MODE_CPU		BIT(16)

/* ASPEED_RSA_TRIGGER */
#define RSA_TRIGGER			BIT(0)

/* ASPEED_RSA_ENG_STS */
#define RSA_STS				(BIT(0) | BIT(1))

#define SWAP(_dest, _src)			\
{						\
	typeof(_dest) *dst = &(_dest);		\
	typeof(_src) *src = &(_src);		\
	typeof(_src) tmp = 0;			\
	tmp = *dst; *dst = *src; *src = tmp;	\
}

#define TO_BITS(_bytes)			((_bytes) << 3)
#define TO_BYTES(_bits)			((_bits) >> 3)

#endif //__ASPEED_RSSS_H
