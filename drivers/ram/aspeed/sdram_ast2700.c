// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) ASPEED Technology Inc.
 */
#include <common.h>
#include <clk.h>
#include <dm.h>
#include <errno.h>
#include <ram.h>
#include <regmap.h>
#include <reset.h>
#include <asm/io.h>
#include <asm/global_data.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/sizes.h>
#include <dt-bindings/clock/ast2700-clock.h>
#include <asm/arch/sdram_ast2700.h>
#include <asm/arch/scu_ast2700.h>

#define DRAMC_UNLOCK_KEY		0x1688a8a8
#define DRAMC_VIDEO_UNLOCK_KEY		0x00440003

#define SCU_CPU_REG                     0x12c02000
#define SCU_CPU_VGA0_SCRATCH            (SCU_CPU_REG + 0x900)
#define SCU_CPU_VGA1_SCRATCH            (SCU_CPU_REG + 0x910)

#define RFC 880

struct sdramc_ac_timing ac_table[] = {
	/* DDR4 1600 */
	{
		DRAM_TYPE_4,
		"DDR4 1600",
		10, 9, 8,
	/*     rcd, rp, ras, rrd, rrd_l, faw, rtp */
		10, 10, 28,  5,   6,	 28,  6,
		2,	/* t_wtr */
		6,	/* t_wtr_l */
		0,	/* t_wtr_a */
		12,	/* t_wtp */
		0,	/* t_rtw */
	/*	ccd_l, dllk, cksre, pd, xp, rfc */
		5, 597,  8,	4,  5,	RFC,
		24,	/* t_mrd */
		0,	/* t_refsbrd */
		0,	/* t_rfcsb */
		0,	/* t_cshsr */
		80,	/* zq */
	},
	/* DDR4 2400 */
	{
		DRAM_TYPE_4,
		"DDR4 2400",
		15, 12, 8,
	/*     rcd, rp, ras, rrd, rrd_l, faw, rtp */
		16, 16, 39, 7, 8, 37, 10,
		4,	/* t_wtr */
		10,	/* t_wtr_l */
		0,	/* t_wtr_a */
		19,	/* t_wtp */
		0,
	/*	ccd_l, dllk, cksre, pd, xp, rfc */
		7, 768,  13,	7,  8,	RFC,
		24,	/* t_mrd */
		0,	/* t_refsbrd */
		0,	/* t_rfcsb */
		0,	/* t_cshsr */
		80,	/* zq */
	},
	/* DDR4 3200 */
	{
		DRAM_TYPE_4,
		"DDR4 3200",
		20, 16, 8,
	/*     rcd, rp, ras, rrd, rrd_l, faw, rtp */
		20, 20, 52, 9, 11, 48, 12,
		4,	/* t_wtr */
		12,	/* t_wtr_l */
		0,	/* t_wtr_a */
		24,	/* t_wtp */
		0,	/* t_rtw */
	/*	ccd_l, dllk, cksre, pd, xp, rfc */
		8, 1023, 16,	8,  10, RFC,
		24,	/* t_mrd */
		0,	/* t_refsbrd */
		0,	/* t_rfcsb */
		0,	/* t_cshsr */
		80,	/* zq */
	},
	/* DDR5 3200 */
	{
		DRAM_TYPE_5,
		"DDR5 3200",
		26, 24, 16,
	/*     rcd, rp, ras, rrd, rrd_l, faw, rtp */
		26, 26, 52,  8,   8,	 40,  12,
		4,	/* t_wtr */
		16,	/* t_wtr_l */
		36,	/* t_wtr_a */
		48,	/* t_wtp */
		0,
	/*	ccd_l, dllk, cksre, pd, xp, rfc */
		8, 1024, 9,	13, 13, RFC,
		23,	/* t_mrd */
		48,	/* t_refsbrd */
		208,	/* t_rfcsb */
		30,	/* t_cshsr */
		48,	/* zq */
	},
};

#define DRAMC_INIT_DONE		BIT(6)
static bool is_ddr_initialized(void)
{
	if (readl((void *)SCU_CPU_VGA0_SCRATCH) & DRAMC_INIT_DONE) {
		debug("DDR has been initialized\n");
		return 1;
	}

	return 0;
}

bool is_ddr4(void)
{
	if (IS_ENABLED(CONFIG_ASPEED_FPGA))
		/* made fpga strap reverse */
		return ((readl((void *)SCU_IO_HWSTRAP1) & IO_HWSTRAP1_DRAM_TYPE) ? 0 : 1);

	/* asic strap default 0 is ddr5, 1 is ddr4 */
	return ((readl((void *)SCU_IO_HWSTRAP1) & IO_HWSTRAP1_DRAM_TYPE) ? 1 : 0);
}

#define ACTIME1(ccd, rrd_l, rrd, mrd)	\
	(((ccd) << 24) | (((rrd_l) >> 1) << 16) | (((rrd) >> 1) << 8) | ((mrd) >> 1))

#define ACTIME2(faw, rp, ras, rcd)	\
	((((faw) >> 1) << 24) | (((rp) >> 1) << 16) | (((ras) >> 1) << 8) | ((rcd) >> 1))

#define ACTIME3(wtr, rtw, wtp, rtp)	\
	((((wtr) >> 1) << 24) | \
	(((rtw) >> 1) << 16) | \
	(((wtp) >> 1) << 8) | \
	((rtp) >> 1))

#define ACTIME4(wtr_a, wtr_l)		\
	((((wtr_a) >> 1) << 8) | ((wtr_l) >> 1))

#define ACTIME5(refsbrd, rfcsb, rfc)	\
	((((refsbrd) >> 1) << 20) | (((rfcsb) >> 1) << 10) | ((rfc) >> 1))

#define ACTIME6(cshsr, pd, xp, cksre)	\
	((((cshsr) >> 1) << 24) | (((pd) >> 1) << 16) | (((xp) >> 1) << 8) | ((cksre) >> 1))

#define ACTIME7(zqcs, dllk)	\
	((((zqcs) >> 1) << 10) | ((dllk) >> 1))

static void sdramc_configure_ac_timing(struct sdramc *sdramc, struct sdramc_ac_timing *ac)
{
	struct sdramc_regs *regs = sdramc->regs;

	writel(ACTIME1(ac->t_ccd_l, ac->t_rrd_l, ac->t_rrd, ac->t_mrd),
	       &regs->actime1);
	writel(ACTIME2(ac->t_faw, ac->t_rp, ac->t_ras, ac->t_rcd),
	       &regs->actime2);
	writel(ACTIME3(ac->t_cwl + ac->t_bl / 2 + ac->t_wtr,
		       ac->t_cl - ac->t_cwl + (ac->t_bl / 2) + 2,
		       ac->t_cwl + ac->t_bl / 2 + ac->t_wtp,
		       ac->t_rtp),
	       &regs->actime3);
	writel(ACTIME4(ac->t_cwl + ac->t_bl / 2 + ac->t_wtr_a,
		       ac->t_cwl + ac->t_bl / 2 + ac->t_wtr_l),
	       &regs->actime4);
	writel(ACTIME5(ac->t_refsbrd, ac->t_rfcsb, ac->t_rfc),
	       &regs->actime5);
	writel(ACTIME6(ac->t_cshsr, ac->t_pd, ac->t_xp, ac->t_cksre), &regs->actime6);
	writel(ACTIME7(ac->t_zq, ac->t_dllk), &regs->actime7);
}

static void sdramc_configure_register(struct sdramc *sdramc, struct sdramc_ac_timing *ac)
{
	struct sdramc_regs *regs = sdramc->regs;

	u32 dram_size = 5;
	u32 t_phy_wrdata;
	u32 t_phy_wrlat;
	u32 t_phy_rddata_en;
	u32 t_phy_odtlat;
	u32 t_phy_odtext;

	if (IS_ENABLED(CONFIG_ASPEED_FPGA)) {
		t_phy_wrlat = ac->t_cwl - 6;
		t_phy_rddata_en = ac->t_cl - 5;
		t_phy_wrdata = 1;
		t_phy_odtlat = 1;
		t_phy_odtext = 0;
	} else {
		if (ac->type == DRAM_TYPE_4) {
			t_phy_wrlat = ac->t_cwl - 5 - 4;
			t_phy_rddata_en = ac->t_cl - 5 - 4;
			t_phy_wrdata = 2;
			t_phy_odtlat = ac->t_cwl - 5 - 4;
			t_phy_odtext = 0;
		} else {
			t_phy_wrlat = ac->t_cwl - 13 - 3;
			t_phy_rddata_en = ac->t_cl - 13 - 3;
			t_phy_wrdata = 6;
			t_phy_odtlat = 0;
			t_phy_odtext = 0;
		}
	}

	writel(0x20 + (dram_size << 2) + ac->type, &regs->mcfg);

	/*
	 * [5:0], t_phy_wrlat, for cycles from WR command to write data enable.
	 * [8:6], t_phy_wrdata, for cycles from write data enable to write data.
	 * [9], reserved
	 * [15:10] t_phy_rddata_en, for cycles from RD command to read data enable.
	 * [19:16], t_phy_odtlat, for cycles from WR command to ODT signal control.
	 * [21:20], ODT signal extension control
	 * [22], ODT signal enable
	 * [23], ODT signal auto mode
	 */
	writel((t_phy_odtext << 20) + (t_phy_odtlat << 16) + (t_phy_rddata_en << 10) + (t_phy_wrdata << 6) + t_phy_wrlat, &regs->dfi_timing);
	writel(0, &regs->dctl);

	/*
	 * [31:24]: refresh felxibility time period
	 * [23:16]: refresh time interfal
	 * [15]   : refresh function disable
	 * [14:10]: reserved
	 * [9:6]  : refresh threshold
	 * [5]	  : refresh option
	 * [4]	  : auto MR command sending for mode change
	 * [3]	  : same bank refresh operation
	 * [2]	  : refresh rate selection
	 * [1]	  : refresh mode selection
	 * [0]	  : refresh mode update trigger
	 */
	writel(0x40b48200, &regs->refctl);

	/*
	 * [31:16]: ZQ calibration period
	 * [15:8] : ZQ latch time period
	 * [7]	  : ZQ control status
	 * [6:3]  : reserved
	 * [2]	  : ZQCL command enable
	 * [1]	  : ZQ calibration auto mode
	 */
	writel(0x42aa1800, &regs->zqctl);

	/*
	 * [31:14]: reserved
	 * [13:12]: selection of limited request number for page-hit request
	 * [11]   : enable control of limitation for page-hit request counter
	 * [10]   : arbiter read threshold limitation disable control
	 * [9]	  : arbiter write threshold limitation disable control
	 * [8:5]  : read access limit threshold selection
	 * [4]	  : read request limit threshold enable
	 * [3:1]  : write request limit threshold selection
	 * [0]	  : write request limit enable
	 */
	writel(0, &regs->arbctl);

	if (ac->type)
		writel(0, &regs->refmng_ctl);

	writel(0xffffffff, &regs->intr_mask);
}

static void sdramc_mr_send(struct sdramc *sdramc, u32 ctrl, u32 op)
{
	struct sdramc_regs *regs = sdramc->regs;

	writel(op, &regs->mrwr);
	writel(ctrl | DRAMC_MRCTL_CMD_START, &regs->mrctl);

	while (!(readl(&regs->intr_status) & DRAMC_IRQSTA_MR_DONE))
		;

	writel(DRAMC_IRQSTA_MR_DONE, &regs->intr_clear);
}

static void sdramc_unlock(struct sdramc *sdramc)
{
	struct sdramc_regs *regs = sdramc->regs;

	writel(DRAMC_UNLOCK_KEY, &regs->prot_key);

	while (!readl(&regs->prot_key))
		;
}

static void sdramc_set_flag(u32 flag)
{
	u32 val;

	val = readl((void *)SCU_CPU_VGA0_SCRATCH);
	val |= flag;
	writel(val, (void *)SCU_CPU_VGA0_SCRATCH);

	val = readl((void *)SCU_CPU_VGA1_SCRATCH);
	val |= flag;
	writel(val, (void *)SCU_CPU_VGA1_SCRATCH);
}

static int sdramc_init(struct sdramc *sdramc, struct sdramc_ac_timing **ac)
{
	struct sdramc_ac_timing *tbl = ac_table;
	int speed;

	/* Detect dram type by a hw strap at IO SCU010 */
	if (is_ddr4()) {
		/* DDR4 type */
		if (IS_ENABLED(CONFIG_ASPEED_DDR_1600)) {
			speed = DDR4_1600;
		} else if (IS_ENABLED(CONFIG_ASPEED_DDR_2400)) {
			speed = DDR4_2400;
		} else if (IS_ENABLED(CONFIG_ASPEED_DDR_3200)) {
			speed = DDR4_3200;
		} else {
			debug("Speed %d is not supported!!!\n", speed);
			return 1;
		}
	} else {
		/* DDR5 type */
		speed = DDR5_3200;
	}

	debug("%s is selected\n", tbl[speed].desc);

	/* Configure ac timing */
	sdramc_configure_ac_timing(sdramc, &tbl[speed]);

	/* Configure register */
	sdramc_configure_register(sdramc, &tbl[speed]);

	*ac = &tbl[speed];

	return 0;
}

static void sdramc_phy_init(struct sdramc *sdramc, struct sdramc_ac_timing *ac)
{
	/* initialize phy */
	if (IS_ENABLED(CONFIG_ASPEED_FPGA))
		fpga_phy_init(sdramc);
#ifdef CONFIG_RISCV
	else
		dwc_phy_init(sdramc);
#endif
}

static int sdramc_exit_self_refresh(struct sdramc *sdramc)
{
	struct sdramc_regs *regs = sdramc->regs;

	/* exit self-refresh after phy init */
	setbits(le32, &regs->mctl, DRAMC_MCTL_SELF_REF_START);

	/* query if self-ref done */
	while (!(readl(&regs->intr_status) & DRAMC_IRQSTA_REF_DONE))
		;

	/* clear status */
	writel(DRAMC_IRQSTA_REF_DONE, &regs->intr_clear);

	udelay(1);

	return 0;
}

static void sdramc_enable_refresh(struct sdramc *sdramc)
{
	struct sdramc_regs *regs = sdramc->regs;

	/* refresh update */
	clrbits(le32, &regs->refctl, 0x8000);
}

static void sdramc_configure_mrs(struct sdramc *sdramc, struct sdramc_ac_timing *ac)
{
	struct sdramc_regs *regs = sdramc->regs;
	u32 mr0_cas, mr0_rtp, mr2_cwl, mr6_tccd_l;
	u32 mr0_val, mr1_val, mr2_val, mr3_val, mr4_val, mr5_val, mr6_val;

	if (ac->type == DRAM_TYPE_5)
		return;

	//-------------------------------------------------------------------
	// CAS Latency (Table-15)
	//-------------------------------------------------------------------
	switch (ac->t_cl) {
	case 9:
		mr0_cas = 0x00; //5'b00000;
		break;
	case 10:
		mr0_cas = 0x01; //5'b00001;
		break;
	case 11:
		mr0_cas = 0x02; //5'b00010;
		break;
	case 12:
		mr0_cas = 0x03; //5'b00011;
		break;
	case 13:
		mr0_cas = 0x04; //5'b00100;
		break;
	case 14:
		mr0_cas = 0x05; //5'b00101;
		break;
	case 15:
		mr0_cas = 0x06; //5'b00110;
		break;
	case 16:
		mr0_cas = 0x07; //5'b00111;
		break;
	case 18:
		mr0_cas = 0x08; //5'b01000;
		break;
	case 20:
		mr0_cas = 0x09; //5'b01001;
		break;
	case 22:
		mr0_cas = 0x0a; //5'b01010;
		break;
	case 24:
		mr0_cas = 0x0b; //5'b01011;
		break;
	case 23:
		mr0_cas = 0x0c; //5'b01100;
		break;
	case 17:
		mr0_cas = 0x0d; //5'b01101;
		break;
	case 19:
		mr0_cas = 0x0e; //5'b01110;
		break;
	case 21:
		mr0_cas = 0x0f; //5'b01111;
		break;
	case 25:
		mr0_cas = 0x10; //5'b10000;
		break;
	case 26:
		mr0_cas = 0x11; //5'b10001;
		break;
	case 27:
		mr0_cas = 0x12; //5'b10010;
		break;
	case 28:
		mr0_cas = 0x13; //5'b10011;
		break;
	case 30:
		mr0_cas = 0x15; //5'b10101;
		break;
	case 32:
		mr0_cas = 0x17; //5'b10111;
		break;
	}

	//-------------------------------------------------------------------
	// WR and RTP (Table-14)
	//-------------------------------------------------------------------
	switch (ac->t_rtp) {
	case 5:
		mr0_rtp = 0x0; //4'b0000;
		break;
	case 6:
		mr0_rtp = 0x1; //4'b0001;
		break;
	case 7:
		mr0_rtp = 0x2; //4'b0010;
		break;
	case 8:
		mr0_rtp = 0x3; //4'b0011;
		break;
	case 9:
		mr0_rtp = 0x4; //4'b0100;
		break;
	case 10:
		mr0_rtp = 0x5; //4'b0101;
		break;
	case 12:
		mr0_rtp = 0x6; //4'b0110;
		break;
	case 11:
		mr0_rtp = 0x7; //4'b0111;
		break;
	case 13:
		mr0_rtp = 0x8; //4'b1000;
		break;
	}

	//-------------------------------------------------------------------
	// CAS Write Latency (Table-21)
	//-------------------------------------------------------------------
	switch (ac->t_cwl)  {
	case 9:
		mr2_cwl = 0x0; // 3'b000; // 1600
		break;
	case 10:
		mr2_cwl = 0x1; // 3'b001; // 1866
		break;
	case 11:
		mr2_cwl = 0x2; // 3'b010; // 2133
		break;
	case 12:
		mr2_cwl = 0x3; // 3'b011; // 2400
		break;
	case 14:
		mr2_cwl = 0x4; // 3'b100; // 2666
		break;
	case 16:
		mr2_cwl = 0x5; // 3'b101; // 2933/3200
		break;
	case 18:
		mr2_cwl = 0x6; // 3'b110;
		break;
	case 20:
		mr2_cwl = 0x7; // 3'b111;
		break;
	}

	//-------------------------------------------------------------------
	// tCCD_L and tDLLK
	//-------------------------------------------------------------------
	switch (ac->t_ccd_l) {
	case 4:
		mr6_tccd_l = 0x0; //3'b000;  // rate <= 1333
		break;
	case 5:
		mr6_tccd_l = 0x1; //3'b001;  // 1333 < rate <= 1866
		break;
	case 6:
		mr6_tccd_l = 0x2; //3'b010;  // 1866 < rate <= 2400
		break;
	case 7:
		mr6_tccd_l = 0x3; //3'b011;  // 2400 < rate <= 2666
		break;
	case 8:
		mr6_tccd_l = 0x4; //3'b100;  // 2666 < rate <= 3200
		break;
	}

	/*
	 * mr0_val = {
	 * mr0_rtp[3],		// 13
	 * mr0_cas[4],		// 12
	 * mr0_rtp[2:0],	// 13,11-9: WR and RTP
	 * 1'b0,		// 8: DLL reset
	 * 1'b0,		// 7: TM
	 * mr0_cas[3:1],	// 6-4,2: CAS latency
	 * 1'b0,		// 3: sequential
	 * mr0_cas[0],
	 * 2'b00		// 1-0: burst length
	 */
	mr0_val = ((mr0_cas & 0x1) << 2) | (((mr0_cas >> 1) & 0x7) << 4) | (((mr0_cas >> 4) & 0x1) << 12) |
		  ((mr0_rtp & 0x7) << 9) | (((mr0_rtp >> 3) & 0x1) << 13);

	/*
	 * 3'b2 //[10:8]: rtt_nom, 000:disable,001:rzq/4,010:rzq/2,011:rzq/6,100:rzq/1,101:rzq/5,110:rzq/3,111:rzq/7
	 * 1'b0 //[7]: write leveling enable
	 * 2'b0 //[6:5]: reserved
	 * 2'b0 //[4:3]: additive latency
	 * 2'b0 //[2:1]: output driver impedance
	 * 1'b1 //[0]: enable dll
	 */
	mr1_val = 0x201;

	/*
	 * [10:9]: rtt_wr, 00:dynamic odt off, 01:rzq/2, 10:rzq/1, 11: hi-z
	 * [8]: 0
	 */
	mr2_val = ((mr2_cwl & 0x7) << 3) | 0x200;

	mr3_val = 0;

	mr4_val = 0;

	/*
	 * mr5_val = {
	 * 1'b0,		// 13: RFU
	 * 1'b0,		// 12: read DBI
	 * 1'b0,		// 11: write DBI
	 * 1'b1,		// 10: Data mask
	 * 1'b0,		// 9: C/A parity persistent error
	 * 3'b000,		// 8-6: RTT_PARK (disable)
	 * 1'b1,		// 5: ODT input buffer during power down mode
	 * 1'b0,		// 4: C/A parity status
	 * 1'b0,		// 3: CRC error clear
	 * 3'b0			// 2-0: C/A parity latency mode
	 * };
	 */
	mr5_val = 0x420;

	/*
	 * mr6_val = {
	 * 1'b0,		// 13, 9-8: RFU
	 * mr6_tccd_l[2:0],	// 12-10: tCCD_L
	 * 2'b0,		// 13, 9-8: RFU
	 * 1'b0,		// 7: VrefDQ training enable
	 * 1'b0,		// 6: VrefDQ training range
	 * 6'b0			// 5-0: VrefDQ training value
	 * };
	 */
	mr6_val = ((mr6_tccd_l & 0x7) << 10);

	writel((mr1_val << 16) + mr0_val, &regs->mr01);
	writel((mr3_val << 16) + mr2_val, &regs->mr23);
	writel((mr5_val << 16) + mr4_val, &regs->mr45);
	writel(mr6_val, &regs->mr67);

	/* Power-up initialization sequence */
	sdramc_mr_send(sdramc, MR_ADDR(3), 0);
	sdramc_mr_send(sdramc, MR_ADDR(6), 0);
	sdramc_mr_send(sdramc, MR_ADDR(5), 0);
	sdramc_mr_send(sdramc, MR_ADDR(4), 0);
	sdramc_mr_send(sdramc, MR_ADDR(2), 0);
	sdramc_mr_send(sdramc, MR_ADDR(1), 0);
	sdramc_mr_send(sdramc, MR_ADDR(0), 0);
}

static int sdramc_bist(struct sdramc *sdramc, u32 addr, u32 size, u32 cfg, u32 timeout)
{
	struct sdramc_regs *regs = sdramc->regs;
	u32 val;
	u32 err = 0;

	writel(0, &regs->bistcfg);
	writel(cfg, &regs->bistcfg);
	writel(addr >> 4, &regs->bist_addr);
	writel(size >> 4, &regs->bist_size);
	writel(0x89abcdef, &regs->bist_patt);
	writel(cfg | DRAMC_BISTCFG_START, &regs->bistcfg);

	while (!(readl(&regs->intr_status) & DRAMC_IRQSTA_BIST_DONE))
		;

	writel(DRAMC_IRQSTA_BIST_DONE, &regs->intr_clear);

	val = readl(&regs->bist_res);

	/* bist done */
	if (val & DRAMC_BISTRES_DONE) {
		/* bist pass [9]=0 */
		if (val & DRAMC_BISTRES_FAIL)
			err++;
	} else {
		err++;
	}

	return err;
}

static void sdramc_aes_enable(struct sdramc *sdramc)
{
	struct sdramc_regs *regs = sdramc->regs;
	u32 addr_min = 0;
	u32 addr_max = sdramc->aes_size;

	writel(addr_min >> 4, &regs->enc_min_addr);
	writel(addr_max >> 4, &regs->enc_max_addr);
	writel(1, &regs->enccfg);
}

#define DRAM_SIZE_DEF	3
static int sdramc_ecc_enable(struct sdramc *sdramc)
{
	size_t ram_size_ary[] = {
		0x10000000, // 256MB
		0x20000000, // 512MB
		0x40000000, // 1GB
		0x80000000, // 2GB
		};
	size_t ecc_sz, ram_size = ram_size_ary[DRAM_SIZE_DEF];
	struct sdramc_regs *regs = sdramc->regs;
	u32 bistcfg;
	u32 val;
	int err;

	bistcfg = 0x82;
	err = sdramc_bist(sdramc, 0, ram_size, bistcfg, 0x200000);
	if (err) {
		printf("bist is failed\n");
		return err;
	}

	/* config ecc range */
	ecc_sz = sdramc->ecc_size >> 4;
	writel(ecc_sz, &regs->ecc_addr_range);

	/* enable ecc, page matching should be disabled */
	val = readl(&regs->mcfg);
	val &= ~(DRAMC_MCFG_PGM_EN | 0x1c);
	val |= (DRAMC_MCFG_ECC_EN | (DRAM_SIZE_DEF << 2));
	writel(val, &regs->mcfg);

	return err;
}

static void sdramc_qos_init(struct sdramc *sdramc)
{
	/* raise SLI write/read priority */
	writel(QOS_SLI_LEVEL(10),
	       (void *)&sdramc->regs->port[4].write_qos);
	writel(QOS_SLI_LEVEL(9),
	       (void *)&sdramc->regs->port[4].read_qos);
	writel(DRAMC_PORT_CFG_RDQOS_EN | DRAMC_PORT_CFG_WRQOS_EN | DEFAULT_RDQOS_LEVEL,
	       (void *)&sdramc->regs->port[4].cfg);

	/* raise usb 2.0 B1/B2, vga1 priority */
	writel(QOS_USB2_B1_LEVEL(9) | QOS_USB2_B2_LEVEL(9) | QOS_VGA1_CR_LEVEL(9),
	       (void *)&sdramc->regs->port[2].read_qos);
	writel(DRAMC_PORT_CFG_RDQOS_EN | DRAMC_PORT_CFG_WRQOS_EN | DEFAULT_RDQOS_LEVEL,
	       (void *)&sdramc->regs->port[2].cfg);

	/* raise vga2 priority */
	writel(QOS_VGA2_CR_LEVEL(9),
	       (void *)&sdramc->regs->port[3].read_qos);
	writel(DRAMC_PORT_CFG_RDQOS_EN | DRAMC_PORT_CFG_WRQOS_EN | DEFAULT_RDQOS_LEVEL,
	       (void *)&sdramc->regs->port[3].cfg);

	/* raise u2 A1/A2 priority */
	writel(QOS_USB2_A1_LEVEL(9) | QOS_USB2_A2_LEVEL(9),
	       (void *)&sdramc->regs->port[1].read_qos);
	writel(DRAMC_PORT_CFG_RDQOS_EN | DRAMC_PORT_CFG_WRQOS_EN | DEFAULT_RDQOS_LEVEL,
	       (void *)&sdramc->regs->port[1].cfg);
}

static int sdram_init(struct udevice *dev)
{
	struct sdramc *sdramc = (struct sdramc *)dev_get_priv(dev);
	struct sdramc_ac_timing *ac;
	u32 bistcfg;
	int err = 0;

	if (is_ddr_initialized())
		return 0;

	sdramc_unlock(sdramc);

	err = sdramc_init(sdramc, &ac);
	if (err)
		return err;

	sdramc_phy_init(sdramc, ac);

	sdramc_exit_self_refresh(sdramc);

	sdramc_configure_mrs(sdramc, ac);

	sdramc_enable_refresh(sdramc);

	if (dev_read_bool(dev, "ecc-enable")) {
		sdramc->ecc_size = dev_read_u32_default(dev, "ecc-size", 0);
		sdramc_ecc_enable(sdramc);
	}

	if (dev_read_bool(dev, "aes-enable")) {
		sdramc->aes_size = dev_read_u32_default(dev, "aes-size", 0);
		sdramc_aes_enable(sdramc);
	}

	bistcfg = FIELD_PREP(DRAMC_BISTCFG_PMODE, BIST_PMODE_CRC)
		| FIELD_PREP(DRAMC_BISTCFG_BMODE, BIST_BMODE_RW_SWITCH)
		| DRAMC_BISTCFG_ENABLE;

	err = sdramc_bist(sdramc, 0, 0x10000, bistcfg, 0x200000);
	if (err) {
		printf("%s bist is failed\n", ac->desc);
		return err;
	}

	sdramc_qos_init(sdramc);

	debug("%s is successfully initialized\n", ac->desc);
	sdramc_set_flag(DRAMC_INIT_DONE);

	return 0;
}

static int ast2700_sdrammc_probe(struct udevice *dev)
{
	int err;

	err = sdram_init(dev);

	return err;
}

static int ast2700_sdrammc_of_to_plat(struct udevice *dev)
{
	struct sdramc *sdramc = dev_get_priv(dev);

	sdramc->regs = (void *)(uintptr_t)devfdt_get_addr_index(dev, 0);
	sdramc->phy_setting = (void *)(uintptr_t)devfdt_get_addr_index(dev, 1);

	return 0;
}

#ifndef CONFIG_RISCV
static size_t ast2700_sdrammc_get_vga_mem_size(struct sdramc *sdramc)
{
	struct ast2700_scu0 *scu;
	struct sdramc_regs *regs = sdramc->regs;
	int nodeoffset;
	u32 vga_ram_size[] = {
		0x2000000, // 32MB
		0x4000000, // 64MB
		};
	int vga_sz_sel;
	ofnode node;
	int dual = 0;

	vga_sz_sel = readl(&regs->gfmcfg) & 0x1;

	/* find the offset of compatible node */
	nodeoffset = fdt_node_offset_by_compatible(gd->fdt_blob, -1,
						   "aspeed,ast2700-scu0");
	if (nodeoffset < 0) {
		printf("%s: failed to get aspeed,ast2700-scu0\n", __func__);
		return -ENODEV;
	}

	/* get ast2700-scu0 node */
	node = offset_to_ofnode(nodeoffset);

	scu = (struct ast2700_scu0 *)ofnode_get_addr(node);

	if (scu->pci0_misc[28] & BIT(0)) {
		printf("VGA0:%dMiB, ", vga_ram_size[vga_sz_sel] / SZ_1M);
		dual++;
	}

	if (scu->pci1_misc[28] & BIT(0)) {
		printf("VGA1:%dMiB, ", vga_ram_size[vga_sz_sel] / SZ_1M);
		dual++;
	}

	return vga_ram_size[vga_sz_sel] * dual;
}

struct ddr_capacity {
	size_t size;
	int rfc[2];
};

#define SCU_IO_REG			0x14c02000
#define SCU_IO_HWSTRAP1			(SCU_IO_REG + 0x010)
#define IO_HWSTRAP1_DRAM_TYPE		BIT(10)

static int ast2700_sdrammc_calc_size(struct sdramc *sdramc)
{
	struct ast2700_scu1 *scu;
	struct sdramc_regs *regs = sdramc->regs;
	struct ddr_capacity ram_size[] = {
		{0x10000000,	{208, 256}}, // 256MB
		{0x20000000,	{208, 416}}, // 512MB
		{0x40000000,	{208, 560}}, // 1GB
		{0x80000000,	{472, 880}}, // 2GB
		{0x100000000,	{656, 880}}, // 4GB
		{0x200000000,	{880, 880}}, // 8GB
		};
	u32 test_pattern = 0xdeadbeef;
	u32 val;
	int sz, ddr4;
	int nodeoffset;
	ofnode node;

	/* find the offset of compatible node */
	nodeoffset = fdt_node_offset_by_compatible(gd->fdt_blob, -1,
						   "aspeed,ast2700-scu1");
	if (nodeoffset < 0) {
		printf("%s: failed to get aspeed,ast2700-scu1\n", __func__);
		return -ENODEV;
	}

	/* get ast2700-scu0 node */
	node = offset_to_ofnode(nodeoffset);

	scu = (struct ast2700_scu1 *)ofnode_get_addr(node);

	/* Configure ram size to max to enable whole area */
	val = readl(&regs->mcfg);
	val &= ~(0x7 << 2);
	writel(val | (SDRAM_SIZE_8GB << 2), &regs->mcfg);

	/* Clear basement. */
	writel(0, (void *)CFG_SYS_SDRAM_BASE);

	for (sz = SDRAM_SIZE_8GB - 1; sz > SDRAM_SIZE_256MB; sz--) {
		test_pattern = (test_pattern << 4) + sz;
		writel(test_pattern, (void *)CFG_SYS_SDRAM_BASE + ram_size[sz].size);

		if (readl((void *)CFG_SYS_SDRAM_BASE) != test_pattern)
			break;
	}

	/* re-configure ram size to dramc. */
	val = readl(&regs->mcfg);
	val &= ~(0x7 << 2);
	writel(val | ((sz + 1) << 2), &regs->mcfg);

	ddr4 = is_ddr4();

	/* update rfc in ac_timing5 register. */
	val = readl(&regs->actime5);
	val &= ~(0x3ff);
	val |= (ram_size[sz + 1].rfc[ddr4] >> 1);
	writel(val, &regs->actime5);

	/* report actual ram base and size to kernel */
	sdramc->info.base = CFG_SYS_SDRAM_BASE;
	sdramc->info.size = ram_size[sz + 1].size - ast2700_sdrammc_get_vga_mem_size(sdramc);

	return 0;
}

static int ast2700_sdrammc_get_info(struct udevice *dev, struct ram_info *info)
{
	struct sdramc *sdramc = dev_get_priv(dev);
	struct sdramc_regs *regs = sdramc->regs;

	if (regs->mcfg & DRAMC_MCFG_ECC_EN) {
		ast2700_sdrammc_get_vga_mem_size(sdramc);

		info->base = CFG_SYS_SDRAM_BASE;
		info->size = regs->ecc_addr_range << 4;
		printf("ECC on, ");
	} else {
		ast2700_sdrammc_calc_size(sdramc);
		info->base = CFG_SYS_SDRAM_BASE;
		info->size = sdramc->info.size;
	}

	return 0;
}

static struct ram_ops ast2700_sdrammc_ops = {
	.get_info = ast2700_sdrammc_get_info,
};
#endif

static const struct udevice_id ast2700_sdrammc_ids[] = {
	{ .compatible = "aspeed,ast2700-sdrammc" },
	{ }
};

U_BOOT_DRIVER(sdrammc_ast2700) = {
	.name = "aspeed_ast2700_sdrammc",
	.id = UCLASS_RAM,
	.of_match = ast2700_sdrammc_ids,
#ifndef CONFIG_RISCV
	.ops = &ast2700_sdrammc_ops,
#endif
	.of_to_plat = ast2700_sdrammc_of_to_plat,
	.probe = ast2700_sdrammc_probe,
	.priv_auto = sizeof(struct sdramc),
};
