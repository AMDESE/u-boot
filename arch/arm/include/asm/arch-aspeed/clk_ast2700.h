/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) Aspeed Technology Inc.
 */
#ifndef _ASM_ARCH_SCU_AST2700_H
#define _ASM_ARCH_SCU_AST2700_H

#define CLKIN_25M 25000000UL

/*
 * Clock divider/multiplier configuration struct.
 * For H-PLL and M-PLL the formula is
 * (Output Frequency) = CLKIN * ((M + 1) / (N + 1)) / (P + 1)
 * M - Numerator
 * N - Denumerator
 * P - Post Divider
 * They have the same layout in their control register.
 *
 */
union ast2700_pll_reg {
	uint32_t w;
	struct {
		unsigned int m : 13;			/* bit[12:0]	*/
		unsigned int n : 6;			/* bit[18:13]	*/
		unsigned int p : 4;			/* bit[22:19]	*/
		unsigned int off : 1;			/* bit[23]	*/
		unsigned int bypass : 1;		/* bit[24]	*/
		unsigned int reset : 1;			/* bit[25]	*/
		unsigned int reserved : 6;		/* bit[31:26]	*/

	} b;
};

struct ast2700_pll_cfg {
	union ast2700_pll_reg reg;
	unsigned int ext_reg;
};

struct ast2700_pll_desc {
	uint32_t in;
	uint32_t out;
	struct ast2700_pll_cfg cfg;
};

static const struct ast2700_pll_desc ast2700_pll_lookup[] = {
	{
		.in = CLKIN_25M,
		.out = 400000000,
		.cfg.reg.b.m = 95,
		.cfg.reg.b.n = 2,
		.cfg.reg.b.p = 1,
		.cfg.ext_reg = 0x31,
	},
	{
		.in = CLKIN_25M,
		.out = 200000000,
		.cfg.reg.b.m = 127,
		.cfg.reg.b.n = 0,
		.cfg.reg.b.p = 15,
		.cfg.ext_reg = 0x3f,
	},
	{
		.in = CLKIN_25M,
		.out = 334000000,
		.cfg.reg.b.m = 667,
		.cfg.reg.b.n = 4,
		.cfg.reg.b.p = 9,
		.cfg.ext_reg = 0x14d,
	},
	{
		.in = CLKIN_25M,
		.out = 1000000000,
		.cfg.reg.b.m = 119,
		.cfg.reg.b.n = 2,
		.cfg.reg.b.p = 0,
		.cfg.ext_reg = 0x3d,
	},
	{
		.in = CLKIN_25M,
		.out = 50000000,
		.cfg.reg.b.m = 95,
		.cfg.reg.b.n = 2,
		.cfg.reg.b.p = 15,
		.cfg.ext_reg = 0x31,
	},
};

struct aspeed_clks {
	ulong id;
	const char *name;
};

#ifndef __ASSEMBLY__
struct ast2700_cpu_clk {
	u32 modrst_ctrl;		/* 0x200 */
	u32 modrst_clr;		/* 0x204 */
	u32 rsv_0x208;		/* 0x208 */
	u32 rsv_0x20c;		/* 0x20c */
	u32 modrst_lock;		/* 0x210 */
	u32 modrst_secure1;		/* 0x214 */
	u32 modrst_secure2;		/* 0x218 */
	u32 modrst_secure3;		/* 0x21c */
	u32 rsv_0x220[8];		/* 0x220 */
	u32 clkgate_ctrl;		/* 0x240 */
	u32 clkgate_clr;		/* 0x244 */
	u32 rsv_0x248[2];		/* 0x248 */
	u32 clkgate_lock;		/* 0x250 */
	u32 clkgate_secure1;		/* 0x254 */
	u32 clkgate_secure2;		/* 0x258 */
	u32 clkgate_secure3;		/* 0x25c */
	u32 rsv_0x260[8];		/* 0x260 */
	u32 clk_sel1;		/* 0x280 */
	u32 clk_sel2;		/* 0x284 */
	u32 clk_sel3;		/* 0x288 */
	u32 rsv_0x28c;		/* 0x28c */
	u32 clk_sel1_lock;		/* 0x290 */
	u32 clk_sel2_lock;		/* 0x294 */
	u32 clk_sel3_lock;		/* 0x298 */
	u32 rsv_0x29c;		/* 0x29c */
	u32 clk_sel1_secure1;		/* 0x2a0 */
	u32 clk_sel1_secure2;		/* 0x2a4 */
	u32 clk_sel1_secure3;		/* 0x2a8 */
	u32 rsv_0x2ac;		/* 0x2ac */
	u32 clk_sel2_secure1;		/* 0x2b0 */
	u32 clk_sel2_secure2;		/* 0x2b4 */
	u32 clk_sel2_secure3;		/* 0x2b8 */
	u32 rsv_0x2bc;		/* 0x2bc */
	u32 clk_sel3_secure1;		/* 0x2c0 */
	u32 clk_sel3_secure2;		/* 0x2c4 */
	u32 clk_sel3_secure3;		/* 0x2c8 */
	u32 rsv_0x2cc[9];		/* 0x2cc */
	u32 extrst_sel;		/* 0x2f0 */
	u32 rsv_0x2f4[3];		/* 0x2f4 */
	u32 hpll;			/* 0x300 */
	u32 hpll_ext;		/* 0x304 */
	u32 dpll;			/* 0x308 */
	u32 dpll_ext;		/* 0x30C */
	u32 mpll;			/* 0x310 */
	u32 mpll_ext;		/* 0x314 */
	u32 rsv_0x318[2];		/* 0x318 ~ 0x31C */
	u32 d1clk_para;		/* 0x320 */
	u32 rsv_0x324[3];		/* 0x324 ~ 0x32C */
	u32 d2clk_para;		/* 0x330 */
	u32 rsv_0x334[3];		/* 0x334 ~ 0x33C */
	u32 crt1clk_para;		/* 0x340 */
	u32 rsv_0x344[3];		/* 0x344 ~ 0x34C */
	u32 crt2clk_para;		/* 0x350 */
	u32 rsv_0x354[3];		/* 0x354 ~ 0x35C */
	u32 mphyclk_para;		/* 0x360 */
	u32 rsv_0x364[7];		/* 0x364 ~ 0x37C */
	u32 clkduty_meas_ctrl;	/* 0x380 */
	u32 clkduty1;		/* 0x384 */
	u32 clkduty2;		/* 0x368 */
	u32 clkduty_meas_res;	/* 0x38c */
	u32 rsv_0x390[4];		/* 0x390 ~ 0x39C */
	u32 freq_counter_ctrl;	/* 0x3a0 */
	u32 freq_counter_cmp;	/* 0x3a4 */
	u32 rsv_0x3a8[2];		/* 0x3a8 ~ 0x3aC */
};

struct ast2700_io_clk {
	u32 modrst_ctrl1;		/* 0x200 */
	u32 modrst_clr1;		/* 0x204 */
	u32 rsv_0x208;		/* 0x208 */
	u32 rsv_0x20c;		/* 0x20c */
	u32 modrst_lock1;		/* 0x210 */
	u32 modrst_secure11;		/* 0x214 */
	u32 modrst_secure12;		/* 0x218 */
	u32 modrst_secure13;		/* 0x21c */
	u32 modrst_ctrl2;		/* 0x220 */
	u32 modrst_clr2;		/* 0x224 */
	u32 rsv_0x228;		/* 0x228 */
	u32 rsv_0x22c;		/* 0x22c */
	u32 modrst_lock2;		/* 0x230 */
	u32 modrst_secure21;		/* 0x234 */
	u32 modrst_secure22;		/* 0x238 */
	u32 modrst_secure23;		/* 0x23c */
	u32 clkgate_ctrl1;		/* 0x240 */
	u32 clkgate_clr1;		/* 0x244 */
	u32 rsv_0x248[2];		/* 0x248 */
	u32 clkgate_lock1;		/* 0x250 */
	u32 clkgate_secure11;		/* 0x254 */
	u32 clkgate_secure12;		/* 0x258 */
	u32 clkgate_secure13;		/* 0x25c */
	u32 clkgate_ctrl2;		/* 0x260 */
	u32 clkgate_clr2;		/* 0x264 */
	u32 rsv_0x268[2];		/* 0x268 */
	u32 clkgate_lock2;		/* 0x270 */
	u32 clkgate_secure21;		/* 0x274 */
	u32 clkgate_secure22;		/* 0x278 */
	u32 clkgate_secure23;		/* 0x27c */
	u32 clk_sel1;		/* 0x280 */
	u32 clk_sel2;		/* 0x284 */
	u32 rsv_0x288[2];		/* 0x288 */
	u32 clk_sel1_lock;		/* 0x290 */
	u32 clk_sel2_lock;		/* 0x294 */
	u32 rsv_0x298[2];		/* 0x298 */
	u32 clk_sel1_secure1;		/* 0x2a0 */
	u32 clk_sel1_secure2;		/* 0x2a4 */
	u32 rsv_0x2a8[2];		/* 0x2a8 */
	u32 clk_sel2_secure1;		/* 0x2b0 */
	u32 clk_sel2_secure2;		/* 0x2b4 */
	u32 rsv_0x2b8[2];		/* 0x2b8 */
	u32 clk_sel3_secure1;		/* 0x2c0 */
	u32 clk_sel3_secure2;		/* 0x2c4 */
	u32 rsv_0x2c8[10];		/* 0x2c8 */
	u32 extrst_sel1;		/* 0x2f0 */
	u32 extrst_sel2;		/* 0x2f4 */
	u32 rsv_0x2f8[2];		/* 0x2f8 */
	u32 hpll;			/* 0x300 */
	u32 hpll_ext;		/* 0x304 */
	u32 rsv_0x308[2];		/* 0x308 ~ 0x30C */
	u32 apll;			/* 0x310 */
	u32 apll_ext;		/* 0x314 */
	u32 rsv_0x318[2];		/* 0x318 ~ 0x31C */
	u32 dpll;			/* 0x320 */
	u32 dpll_ext;		/* 0x324 */
	u32 rsv_0x328[2];		/* 0x328 ~ 0x32C */
	u32 uxclk_ctrl;		/* 0x330 */
	u32 huxclk_ctrl;		/* 0x334 */
	u32 rsv_0x338[18];		/* 0x338 ~ 0x37C */
	u32 clkduty_meas_ctrl;	/* 0x380 */
	u32 clkduty1;		/* 0x384 */
	u32 clkduty2;		/* 0x388 */
	u32 rsv_0x38c;		/* 0x38c */
	u32 mac_delay;		/* 0x390 */
	u32 mac_100m_delay;		/* 0x394 */
	u32 mac_10m_delay;		/* 0x398 */
	u32 rsv_0x39c;		/* 0x39c */
	u32 freq_counter_ctrl;	/* 0x3a0 */
	u32 freq_counter_cmp;	/* 0x3a4 */
	u32 rsv_0x3a8[2];		/* 0x3a8 ~ 0x3aC */
};
#endif
#endif
