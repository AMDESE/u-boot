// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) ASPEED Technology Inc.
 */
#include <common.h>
#include <dm.h>
#include <asm/arch-aspeed/sdram_ast2700.h>

#define DRAMC_UNLOCK_KEY		0x1688a8a8
#define DRAMC_VIDEO_UNLOCK_KEY		0x00440003

/*
 * Given a maximum RFC value for biggest capacity,
 * it will be updated after dram size is determined later
 */

#define RFC 880

struct sdramc g_sdramc;

struct sdramc_ac_timing ac_table[] = {
	/* DDR4 1600 */
	{
		DRAM_TYPE_4,
		"DDR4 1600",
		10, 9, 8,
	/*     rcd, rp, ras, rrd, rrd_l, faw, rtp */
		10, 10, 28,  5,   6,     28,  6,
		2,	/* t_wtr */
		6,	/* t_wtr_l */
		0,	/* t_wtr_a */
		12,	/* t_wtp */
		0,	/* t_rtw */
	/*      ccd_l, dllk, cksre, pd, xp, rfc */
		5, 597,  8,     4,  5,  RFC,
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
	/*      ccd_l, dllk, cksre, pd, xp, rfc */
		7, 768,  13,    7,  8,  RFC,
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
	/*      ccd_l, dllk, cksre, pd, xp, rfc */
		8, 1023, 16,    8,  10, RFC,
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
		26, 26, 52,  8,   8,     40,  12,
		4,	/* t_wtr */
		16,	/* t_wtr_l */
		36,	/* t_wtr_a */
		48,	/* t_wtp */
		0,
	/*      ccd_l, dllk, cksre, pd, xp, rfc */
		8, 1024, 9,     13, 13, RFC,
		23,	/* t_mrd */
		48,	/* t_refsbrd */
		208,	/* t_rfcsb */
		30,	/* t_cshsr */
		48,	/* zq */
	},
};

#define DRAMC_INIT_DONE		0x70
static bool is_ddr_initialized(void)
{
	if (readl((void *)SCU_CPU_SOC1_SCRATCH) & DRAMC_INIT_DONE) {
		printf("DDR has been initialized\n");
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

	val = readl((void *)SCU_CPU_SOC1_SCRATCH);
	val |= flag;
	writel(val, (void *)SCU_CPU_SOC1_SCRATCH);
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
			printf("Speed %d is not supported!!!\n", speed);
			return 1;
		}
	} else {
		/* DDR5 type */
		speed = DDR5_3200;
	}

	printf("%s is selected\n", tbl[speed].desc);

	/* Configure ac timing */
	sdramc_configure_ac_timing(sdramc, &tbl[speed]);

	/* Configure register */
	sdramc_configure_register(sdramc, &tbl[speed]);

	*ac = &tbl[speed];

	printf("ac_table type=%d\n", tbl[speed].type);
	return 0;
}

static void sdramc_phy_init(struct sdramc *sdramc, struct sdramc_ac_timing *ac)
{
	/* initialize phy */
	if (IS_ENABLED(CONFIG_ASPEED_FPGA))
		fpga_phy_init(sdramc);
	else
		dwc_phy_init(ac);
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

	printf("MR0: 0x%x\n", mr0_val);
	printf("MR1: 0x%x\n", mr1_val);
	printf("MR2: 0x%x\n", mr2_val);
	printf("MR3: 0x%x\n", mr3_val);
	printf("MR4: 0x%x\n", mr4_val);
	printf("MR5: 0x%x\n", mr5_val);
	printf("MR6: 0x%x\n", mr6_val);

	/* Power-up initialization sequence */
	sdramc_mr_send(sdramc, MR_ADDR(3), 0);
	sdramc_mr_send(sdramc, MR_ADDR(6), 0);
	sdramc_mr_send(sdramc, MR_ADDR(5), 0);
	sdramc_mr_send(sdramc, MR_ADDR(4), 0);
	sdramc_mr_send(sdramc, MR_ADDR(2), 0);
	sdramc_mr_send(sdramc, MR_ADDR(1), 0);
	sdramc_mr_send(sdramc, MR_ADDR(0), 0);
}

struct ddr_command {
	u8 desc[30];
	u32 type;
	u32 data;
};

struct ddr_command command_sequence_tbl[] = {
	//{"RTT_CK group A",
	//MR_MPC, (MPC_OP_RTT_CK_A + MR32_CK_ODT_480)},
	//{"RTT_CK group B",
	//MR_MPC, (MPC_OP_RTT_CK_B + MR32_CK_ODT_480)},
	//{"RTT_CS group A",
	//MR_MPC, (MPC_OP_RTT_CS_A + MR32_CK_ODT_480)},
	//{"RTT_CS group B",
	//MR_MPC, (MPC_OP_RTT_CS_B + MR32_CK_ODT_480)},
	//{"RTT_CA group A",
	//MR_MPC, (MPC_OP_RTT_CA_A + MR32_CK_ODT_480)},
	//{"RTT_CA group B",
	//MR_MPC, (MPC_OP_RTT_CA_B + MR32_CK_ODT_480)},
	//{"Set DQS_RTT_PARK",
	//MR_MPC, (MPC_OP_SET_DQS_RTT_PARK + MR33_DQS_RTT_PARK_34)},
	//{"Set RTT_PARK",
	//MR_MPC, (MPC_OP_SET_RTT_PARK + MR34_RTT_PARK_34)},
	//{"VrefCS",
	//MR_VREFCS, MR12_VREFCS_RANGE_75},
	//{"VrefCA",
	//MR_VREFCA, MR11_VREFCA_RANGE_75},
	//{"Apply VrefCA/VrefCS/RTT",
	//MR_MPC, MPC_OP_APPLY},
	//{"Set 1N command timing",
	//MR_MPC, MPC_OP_SET_1N_CMD},
	//{"MR0",
	//MR_ADDR(0), 8},//((((CL - 22) >> 1) << 2) + 0),
	{"MR4/5/6",
	MR_ADDR(4) | MR_NUM(2), 0x2000},
	{"MR8",
	MR_ADDR(8), MR8_WRITE_PREAMBLE_2TCK},
	////{"MR10",
	////MR_ADDR(10), MR10_VREFDQ_RANGE_75},
	////{"MR23",
	////MR_ADDR(23), 0},
	{"MR2",
	MR_ADDR(2), MR2_CS_ASSERTION},
	{"MR13",
	//MR_1T_MODE | MR_MPC, MPC_OP_CONFIG_DLLK_CCD},
	MR_MPC, MPC_OP_CONFIG_DLLK_CCD},
	//{"DLL Reset",
	//MR_1T_MODE | MR_DLL_RESET | MR_MPC, MPC_OP_DLL_RESET},
	//{"ZQ Cali",
	//MR_1T_MODE | MR_MPC, MPC_OP_ZQCAL_START},
	//{"ZQ Latch",
	//MR_1T_MODE | MR_MPC, MPC_OP_ZQCAL_LATCH},
};

static void sdramc_configure_ddr5_mrs(struct sdramc *sdramc, struct sdramc_ac_timing *ac)
{
	struct ddr_command *cmd = command_sequence_tbl;
	int i;

	for (i = 0; i < ARRAY_SIZE(command_sequence_tbl); i++) {
		printf("%s 0x%x\n", cmd[i].desc, cmd[i].data);
		sdramc_mr_send(sdramc, cmd[i].type, cmd[i].data);
	}
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

int dram_init(void)
{
	struct sdramc *sdramc = &g_sdramc;
	struct sdramc_ac_timing *ac;
	u32 bistcfg;
	int err = 0;

	sdramc->regs = (struct sdramc_regs *)0x12c00000;

	if (is_ddr_initialized())
		goto out;

	sdramc_unlock(sdramc);

	err = sdramc_init(sdramc, &ac);
	if (err)
		return err;

	sdramc_phy_init(sdramc, ac);

	sdramc_exit_self_refresh(sdramc);

	if (ac->type == DRAM_TYPE_4)
		sdramc_configure_mrs(sdramc, ac);
	else
		sdramc_configure_ddr5_mrs(sdramc, ac);

	sdramc_enable_refresh(sdramc);

	bistcfg = FIELD_PREP(DRAMC_BISTCFG_PMODE, BIST_PMODE_CRC)
		| FIELD_PREP(DRAMC_BISTCFG_BMODE, BIST_BMODE_RW_SWITCH)
		| DRAMC_BISTCFG_ENABLE;

	err = sdramc_bist(sdramc, 0, 0x10000, bistcfg, 0x200000);
	if (err) {
		printf("%s bist is failed\n", ac->desc);
		return err;
	}

	printf("%s is successfully initialized\n", ac->desc);
	sdramc_set_flag(DRAMC_INIT_DONE);

out:
	sdramc->info.base = 0x80000000;
	sdramc->info.size = 0x40000000;

	return 0;
}

//static int ast2700_sdramc_of_to_plat(struct udevice *dev)
//{
//	struct sdramc *sdramc = (struct sdramc *)dev_get_priv(dev);
//
//	sdramc->regs = (void *)(uintptr_t)devfdt_get_addr_index(dev, 0);
//	sdramc->phy_setting = (void *)(uintptr_t)devfdt_get_addr_index(dev, 1);
//
//	return 0;
//}
//
//static int ast2700_sdramc_get_info(struct udevice *dev, struct ram_info *info)
//{
//	struct sdramc *sdramc = (struct sdramc *)dev_get_priv(dev);
//
//	*info = sdramc->info;
//
//	return 0;
//}
//
//static struct ram_ops ast2700_sdramc_ops = {
//	.get_info = ast2700_sdramc_get_info,
//};
//
//static const struct udevice_id ast2700_sdramc_ids[] = {
//	{ .compatible = "aspeed,ast2700-sdrammc" },
//	{ }
//};
//
//U_BOOT_DRIVER(sdrammc_ast2700) = {
//	.name = "aspeed_ast2700_sdrammc",
//	.id = UCLASS_RAM,
//	.of_match = ast2700_sdramc_ids,
//	.ops = &ast2700_sdramc_ops,
//	.of_to_plat = ast2700_sdramc_of_to_plat,
//	.probe = ast2700_sdramc_probe,
//	.priv_auto = sizeof(struct sdramc),
//};
