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

/* keys for unlocking HW */
#define SDRAM_UNLOCK_KEY			0x1688a8a8
#define SDRAM_VIDEO_UNLOCK_KEY		0x00440003

/* Fixed priority DRAM Requests mask */
#define REQ_PRI_VGA_HW_CURSOR_R         0
#define REQ_PRI_VGA_CRT_R               1
#define REQ_PRI_SOC_DISPLAY_CTRL_R      2
#define REQ_PRI_PCIE_BUS1_RW            3
#define REQ_PRI_VIDEO_HIGH_PRI_W        4
#define REQ_PRI_CPU_RW                  5
#define REQ_PRI_SLI_RW                  6
#define REQ_PRI_PCIE_BUS2_RW            7
#define REQ_PRI_USB2_0_HUB_EHCI1_DMA_RW 8
#define REQ_PRI_USB2_0_DEV_EHCI2_DMA_RW 9
#define REQ_PRI_USB1_1_UHCI_HOST_RW     10
#define REQ_PRI_AHB_BUS_RW              11
#define REQ_PRI_CM3_DATA_RW             12
#define REQ_PRI_CM3_INST_R              13
#define REQ_PRI_MAC0_DMA_RW             14
#define REQ_PRI_MAC1_DMA_RW             15
#define REQ_PRI_SDIO_DMA_RW             16
#define REQ_PRI_PILOT_ENGINE_RW         17
#define REQ_PRI_XDMA1_RW                18
#define REQ_PRI_MCTP1_RW                19
#define REQ_PRI_VIDEO_FLAG_RW           20
#define REQ_PRI_VIDEO_LOW_PRI_W         21
#define REQ_PRI_2D_ENGINE_DATA_RW       22
#define REQ_PRI_ENC_ENGINE_RW           23
#define REQ_PRI_MCTP2_RW                24
#define REQ_PRI_XDMA2_RW                25
#define REQ_PRI_ECC_RSA_RW              26

#define MCR30_RESET_DLL_DELAY_EN	BIT(4)
#define MCR30_MODE_REG_SEL_SHIFT	1
#define MCR30_MODE_REG_SEL_MASK		GENMASK(3, 1)
#define MCR30_SET_MODE_REG		BIT(0)

#define MCR30_SET_MR(mr) (((mr) << MCR30_MODE_REG_SEL_SHIFT) | MCR30_SET_MODE_REG)

#define MCR34_SELF_REFRESH_STATUS_MASK	GENMASK(30, 28)

#define MCR34_ODT_DELAY_SHIFT		12
#define MCR34_ODT_DELAY_MASK		GENMASK(15, 12)
#define MCR34_ODT_EXT_SHIFT		10
#define MCR34_ODT_EXT_MASK		GENMASK(11, 10)
#define MCR34_ODT_AUTO_ON		BIT(9)
#define MCR34_ODT_EN			BIT(8)
#define MCR34_RESETN_DIS		BIT(7)
#define MCR34_MREQI_DIS			BIT(6)
#define MCR34_MREQ_BYPASS_DIS		BIT(5)
#define MCR34_RGAP_CTRL_EN		BIT(4)
#define MCR34_CKE_OUT_IN_SELF_REF_DIS	BIT(3)
#define MCR34_FOURCE_SELF_REF_EN	BIT(2)
#define MCR34_AUTOPWRDN_EN		BIT(1)
#define MCR34_CKE_EN			BIT(0)

#define MCR38_RW_MAX_GRANT_CNT_RQ_SHIFT	16
#define MCR38_RW_MAX_GRANT_CNT_RQ_MASK	GENMASK(20, 16)

/* default request queued limitation mask (0xFFBBFFF4) */
#define MCR3C_DEFAULT_MASK                                                     \
	~(REQ_PRI_VGA_HW_CURSOR_R | REQ_PRI_VGA_CRT_R | REQ_PRI_PCIE_BUS1_RW | \
	  REQ_PRI_XDMA1_RW | REQ_PRI_2D_ENGINE_DATA_RW)

#define MCR50_RESET_ALL_INTR		BIT(31)
#define SDRAM_CONF_ECC_AUTO_SCRUBBING	BIT(9)
#define SDRAM_CONF_SCRAMBLE		BIT(8)
#define SDRAM_CONF_ECC_EN		BIT(7)
#define SDRAM_CONF_DUALX8		BIT(5)
#define SDRAM_CONF_DDR4			BIT(4)
#define SDRAM_CONF_VGA_SIZE_SHIFT	2
#define SDRAM_CONF_VGA_SIZE_MASK	GENMASK(3, 2)
#define SDRAM_CONF_CAP_SHIFT		0
#define SDRAM_CONF_CAP_MASK		GENMASK(1, 0)

#define SDRAM_CONF_CAP_256M		0
#define SDRAM_CONF_CAP_512M		1
#define SDRAM_CONF_CAP_1024M		2
#define SDRAM_CONF_CAP_2048M		3
#define SDRAM_CONF_ECC_SETUP		(SDRAM_CONF_ECC_AUTO_SCRUBBING | SDRAM_CONF_ECC_EN)

#define SDRAM_MISC_DDR4_TREFRESH	BIT(3)

#define SDRAM_PHYCTRL0_PLL_LOCKED	BIT(4)
#define SDRAM_PHYCTRL0_NRST		BIT(2)
#define SDRAM_PHYCTRL0_INIT		BIT(0)

/* MCR0C */
#define SDRAM_REFRESH_PERIOD_ZQCS_SHIFT	16
#define SDRAM_REFRESH_PERIOD_ZQCS_MASK	GENMASK(31, 16)
#define SDRAM_REFRESH_PERIOD_SHIFT	8
#define SDRAM_REFRESH_PERIOD_MASK	GENMASK(15, 8)
#define SDRAM_REFRESH_ZQCS_EN		BIT(7)
#define SDRAM_RESET_DLL_ZQCL_EN		BIT(6)
#define SDRAM_LOW_PRI_REFRESH_EN	BIT(5)
#define SDRAM_FORCE_PRECHARGE_EN	BIT(4)
#define SDRAM_REFRESH_EN		BIT(0)

#define SDRAM_TEST_LEN_SHIFT		4
#define SDRAM_TEST_LEN_MASK		0xfffff
#define SDRAM_TEST_START_ADDR_SHIFT	24
#define SDRAM_TEST_START_ADDR_MASK	0x3f

#define SDRAM_TEST_EN			BIT(0)
#define SDRAM_TEST_MODE_SHIFT		1
#define SDRAM_TEST_MODE_MASK		(0x3 << SDRAM_TEST_MODE_SHIFT)
#define SDRAM_TEST_MODE_WO		(0x0 << SDRAM_TEST_MODE_SHIFT)
#define SDRAM_TEST_MODE_RB		(0x1 << SDRAM_TEST_MODE_SHIFT)
#define SDRAM_TEST_MODE_RW		(0x2 << SDRAM_TEST_MODE_SHIFT)

#define SDRAM_TEST_GEN_MODE_SHIFT	3
#define SDRAM_TEST_GEN_MODE_MASK	(7 << SDRAM_TEST_GEN_MODE_SHIFT)
#define SDRAM_TEST_TWO_MODES		BIT(6)
#define SDRAM_TEST_ERRSTOP		BIT(7)
#define SDRAM_TEST_DONE			BIT(12)
#define SDRAM_TEST_FAIL			BIT(13)

#define SDRAM_AC_TRFC_SHIFT		0
#define SDRAM_AC_TRFC_MASK		0xff

DECLARE_GLOBAL_DATA_PTR;

struct dramc_port {
	u32 configuration;
	u32 timeout;
	u32 read_qos;
	u32 write_qos;
	u32 monitor_config;
	u32 monitor_limit;
	u32 monitor_timer;
	u32 monitor_status;
	u32 bandwidth_log;
};

struct dramc_protect {
	u32 control;
	u32 err_status;
	u32 lo_addr;
	u32 hi_addr;
	u32 wr_master_0;
	u32 wr_master_1;
	u32 rd_master_0;
	u32 rd_master_1;
	u32 wr_secure_0;
	u32 wr_secure_1;
	u32 rd_secure_0;
	u32 rd_secure_1;
};

struct dramc_regs {
	u32 protection_key;			/* offset 0x00 */
	u32 intr_status;			/* offset 0x04 */
	u32 intr_clear;             /* offset 0x08 */
	u32 intr_mask;              /* offset 0x0C */
	u32 main_configuration;     /* offset 0x10 */
	u32 main_control;
	u32 main_status;
	u32 error_status;
	u32 ac_timing[7];
	u32 dfi_timing;
	u32 dfi_configuration;
	u32 dfi_control_msg;
	u32 mode_reg_control;
	u32 mode_reg_wr_op;
	u32 mode_reg_rd_op;
	u32 mr01_setting;
	u32 mr23_setting;
	u32 mr45_setting;
	u32 mr67_setting;
	u32 refresh_control;
	u32 refresh_mng_control;
	u32 refresh_status;
	u32 zqc_control;            /* offset 0x70 */
	u32 ecc_addr_range;         /* offset 0x74 */
	u32 ecc_failure_status;     /* offset 0x78 */
	u32 ecc_failure_addr;       /* offset 0x7C */
	u32 ecc_test_control;       /* offset 0x80 */
	u32 ecc_test_status;        /* offset 0x84 */
	u32 arbitration_control;    /* offset 0x88 */
	u32 reserved0;				/* offset 0x8c */
	u32 protect_lock_set;		/* offset 0x90 */
	u32 protect_lock_status;	/* offset 0x94 */
	u32 protect_lock_reset;		/* offset 0x98 */
	u32 enc_min_address;		/* offset 0x9c */
	u32 enc_max_address;		/* offset 0xa0 */
	u32 enc_key[4];				/* offset 0xa4~0xb0 */
	u32 enc_iv[3];				/* offset 0xb4~0xbc */
	u32 built_in_test_config;
	u32 built_in_test_addr;
	u32 built_in_test_size;
	u32 built_in_test_pattern;
	u32 built_in_test_result;
	u32 built_in_fail_addr;
	u32 built_in_fail_data[4];
	u32 reserved2[2];
	u32 debug_control;
	u32 debug_status;
	u32 phy_intf_status;
	u32 test_configuration;
	u32 graphic_memory_config;	/* 0x100 */
	u32 graphic_memory_0_ctrl;
	u32 graphic_memory_1_ctrl;
	struct dramc_protect region0[8];
	struct dramc_port port[6];	/* 0x400 */
	struct dramc_protect region1[8];
};

struct dram_info {
	struct ram_info info;
	struct dramc_regs *regs;
	//struct ast2600_scu *scu;
	//struct ast2600_ddr_phy *phy;
	void __iomem *phy_setting;
	void __iomem *phy_status;
	ulong clock_rate;
};

#define FPGA_ASPEED
#define ASPEED_DDR4_1600
#define DDR4
#define ASPEED_DDR4_4G

#if defined(ASPEED_DDR4_3200)
#define CL		20
#define RTP		12
#define CWL		16
#define BL		8
#define CCD_L	8
#elif defined(ASPEED_DDR4_2400)
#define CL		15
#define RTP		10
#define CWL		12
#define BL		8
#define CCD_L	c7
#elif defined(ASPEED_DDR4_1600)
#define CL		10
#define RTP		6
#define CWL		9
#define BL		8
#define CCD_L	5
#endif

#if defined(ASPEED_DDR4_16G)
#define RFC		440
#elif defined(ASPEED_DDR4_8G)
#define RFC		280
#elif defined(ASPEED_DDR4_4G)
#define RFC		208
#else
#define RFC		128
#endif

struct dramc_ac_timing {
	int t_rcd;
	int t_rp;
	int t_ras;
	int t_rrd;
	int t_rrd_l;
	int t_faw;
	int t_rtp;
	int t_wtr;
	int t_wtr_l;
	int t_wtp;
	int t_rtw;
	int t_ccd_l;
	int t_dllk;
	int t_cksre;
	int t_pd;
	int t_xp;
	int t_rfc;
};

struct dramc_ac_timing ac_table[] = {
	{/* DDR4 1600 */
		10, 10, 28, 5, 6, 28, 6,
		(CWL + BL / 2 + 2), /* t_wtr */
		(CWL + BL / 2 + 6), /* t_wtr_l */
		(CWL + BL / 2 + 12), /* t_wtp */
		(CL - CWL + BL / 2 + 2), /* t_rtw */
		CCD_L, 597, 8, 4, 5, RFC,
	},

};

void dramc_convert_actiming_table(struct dramc_regs *regs, struct dramc_ac_timing *tbl)
{
	u32 ac_timing[7];
	u8 t_zqcs  = 80; //TBD
	u8 t_rfcsb = 0;
	u8 t_mrd   = 4;
	u8 t_wtr_a = 0; // not used
	u8 t_cshsr = 0; // not used

#if defined(FPGA_ASPEED)
	u32 i = 1;
#else
	u32 i = 0;
#endif
	ac_timing[0] = ((tbl->t_ccd_l) << 24) +
				((tbl->t_rrd_l >> 1) << 16) +
				((tbl->t_rrd >> 1) << 8) + t_mrd;
	ac_timing[1] = ((tbl->t_faw >> 1) << 24) +
				((tbl->t_rp >> 1) << 16) +
				((tbl->t_ras >> 1) << 8) +
				(tbl->t_rcd >> 1);
	ac_timing[2] = ((tbl->t_wtr >> 1) << 24) +
				(((tbl->t_rtw >> 1) + i) << 16) +
				((tbl->t_wtp >> 1) << 8) +
				(tbl->t_rtp >> 1);
	ac_timing[3] = (t_wtr_a << 8) +
				(tbl->t_wtr_l >> 1);
	ac_timing[4] = (t_zqcs << 20) +
				(t_rfcsb << 10) +
				(tbl->t_rfc >> 1);
	ac_timing[5] = (t_cshsr << 24) +
				(((tbl->t_pd >> 1) - 1) << 16) +
				(((tbl->t_xp >> 1) - 1) << 8) +
				((tbl->t_cksre >> 1) - 1);
	ac_timing[6] = (tbl->t_dllk >> 1) - 1;

	for (i = 0; i < ARRAY_SIZE(ac_timing); ++i)
		writel(ac_timing[i], &regs->ac_timing[i]);
}

void dramc_configure_ac_timing(struct dramc_regs *regs)
{
	/* load controller setting */
	dramc_convert_actiming_table(regs, ac_table);
}

void dramc_configure_register(struct dramc_regs *regs, int ddr5_mode)
{
	u32 dram_size;

#if defined(FPGA_ASPEED)
	u32 t_phy_wrdata = 1;
	u32 t_phy_wrlat = CWL - 6;
	u32 t_phy_rddata_en = CL - 5;
	u32 t_phy_odtlat = 1;
	u32 t_phy_odtext = 0;
#else
	// - Tphy_wrlat = N+4 (nCK) for DDR4, N+3 (nCK) for DDR5, where N is setting value.
	//   * For DDR4 SDRAM, t_{phy_wrlat} = WL - 5.
	//   * For DDR5 SDRAM, t_{phy_wrlat} = WL - 13.
	u32 t_phy_wrlat = CWL - 5 - 4;
	u32 t_phy_odtlat = CWL - 5 - 4;
	u32 t_phy_odtext  = 0;

	// - Tphy_rddata_en = N+4 (nCK) for DDR4, N+3 (nCK) for DDR5, where N is setting value.
	//   * For DDR4 SDRAM, t_{phy_rddata_en} = RL - 5.
	//   * For DDR5 SDRAM, t_{phy_rddata_en} = RL - 13.
	u32 t_phy_rddata_en = CL - 5 - 4;

	// - Tphy_wrdata = N (nCK)
	//   * For DDR4 SDRAM, t_{phy_wrdata} = 2.
	//   * For DDR5 SDRAM, t_{phy_wrdata} = 6.
	u32 t_phy_wrdata = 2;
#endif

#ifdef DDR4
#if defined(ASPEED_DDR4_64G)
	dram_size = 5;
#elif defined(ASPEED_DDR4_32G)
	dram_size = 4;
#elif defined(ASPEED_DDR4_16G)
	dram_size = 3;
#elif defined(ASPEED_DDR4_8G)
	dram_size = 2;
#elif defined(ASPEED_DDR4_4G)
	dram_size = 1;
#elif defined(ASPEED_DDR4_2G)
	dram_size = 0;
#else
	dram_size = 0;
#endif
#endif

	writel(0x20 + (dram_size << 2) + ddr5_mode, &regs->main_configuration);
	//printf("main_config=0x%x\n", (u32)&regs->main_configuration);

	/* [5:0], t_phy_wrlat, for cycles from WR command to write data enable.
	 * [8:6], t_phy_wrdata, for cycles from write data enable to write data.
	 * [9], reserved
	 * [15:10] t_phy_rddata_en, for cycles from RD command to read data enable.
	 * [19:16], t_phy_odtlat, for cycles from WR command to ODT signal control.
	 * [21:20], ODT signal extension control
	 * [22], ODT signal enable
	 * [23], ODT signal auto mode
	 */
	writel((t_phy_odtext << 16) + (t_phy_odtlat << 16) +
			(t_phy_rddata_en << 10) + (t_phy_wrdata << 6) +
			t_phy_wrlat, &regs->dfi_timing);

	writel(0x00000000, &regs->dfi_control_msg);

	/* [31:24]: refresh felxibility time period
	 * [23:16]: refresh time interfal
	 * [15]   : refresh function disable
	 * [14:10]: reserved
	 * [9:6]  : refresh threshold
	 * [5]    : refresh option
	 * [4]    : auto MR command sending for mode change
	 * [3]    : same bank refresh operation
	 * [2]    : refresh rate selection
	 * [1]    : refresh mode selection
	 * [0]    : refresh mode update trigger
	 */
	writel(0x40b48200, &regs->refresh_control);

	/* [31:16]: ZQ calibration period
	 * [15:8] : ZQ latch time period
	 * [7]    : ZQ control status
	 * [6:3]  : reserved
	 * [2]    : ZQCL command enable
	 * [1]    : ZQ calibration auto mode
	 */
	writel(0x42aa1800, &regs->zqc_control);

	/* [31:14]: reserved
	 * [13:12]: selection of limited request number for page-hit request
	 * [11]   : enable control of limitation for page-hit request counter
	 * [10]   : arbiter read threshold limitation disable control
	 * [9]    : arbiter write threshold limitation disable control
	 * [8:5]  : read access limit threshold selection
	 * [4]    : read request limit threshold enable
	 * [3:1]  : write request limit threshold selection
	 * [0]    : write request limit enable
	 */
	writel(0x00000000, &regs->arbitration_control); /* write read threshold enable */

	writel(0xffffffff, &regs->intr_mask);
}

static void ast2700_sdrammc_common_init(struct dram_info *info)
{
	/* 2.2 configure ac timing */
	dramc_configure_ac_timing(info->regs);

	/* 2.3 configure register */
	dramc_configure_register(info->regs, 0);
}

static void ast2700_sdrammc_unlock(struct dram_info *info)
{
	writel(SDRAM_UNLOCK_KEY, &info->regs->protection_key);
	while (!readl(&info->regs->protection_key))
		;
}

static void ast2700_sdrammc_lock(struct dram_info *info)
{
	writel(~SDRAM_UNLOCK_KEY, &info->regs->protection_key);
	while (readl(&info->regs->protection_key))
		;
}

void dramc_configure_mrs(struct dramc_regs *regs)
{
	u32 mr0_cas, mr0_rtp, mr2_cwl, mr6_tccd_l;
	u32 mr0_val, mr1_val, mr2_val, mr3_val, mr4_val, mr5_val, mr6_val;

	//-------------------------------------------------------------------
	// CAS Latency (Table-15)
	//-------------------------------------------------------------------
	switch (CL) {
	case 9:
		mr0_cas = 0x00; break;  //5'b00000;
	case 10:
		mr0_cas = 0x01; break;  //5'b00001;
	case 11:
		mr0_cas = 0x02; break;  //5'b00010;
	case 12:
		mr0_cas = 0x03; break;  //5'b00011;
	case 13:
		mr0_cas = 0x04; break;  //5'b00100;
	case 14:
		mr0_cas = 0x05; break;  //5'b00101;
	case 15:
		mr0_cas = 0x06; break;  //5'b00110;
	case 16:
		mr0_cas = 0x07; break;  //5'b00111;
	case 18:
		mr0_cas = 0x08; break;  //5'b01000;
	case 20:
		mr0_cas = 0x09; break;  //5'b01001;
	case 22:
		mr0_cas = 0x0a; break;  //5'b01010;
	case 24:
		mr0_cas = 0x0b; break;  //5'b01011;
	case 23:
		mr0_cas = 0x0c; break;  //5'b01100;
	case 17:
		mr0_cas = 0x0d; break;  //5'b01101;
	case 19:
		mr0_cas = 0x0e; break;  //5'b01110;
	case 21:
		mr0_cas = 0x0f; break;  //5'b01111;
	case 25:
		mr0_cas = 0x10; break;  //5'b10000;
	case 26:
		mr0_cas = 0x11; break;  //5'b10001;
	case 27:
		mr0_cas = 0x12; break;  //5'b10010;
	case 28:
		mr0_cas = 0x13; break;  //5'b10011;
	case 30:
		mr0_cas = 0x15; break;  //5'b10101;
	case 32:
		mr0_cas = 0x17; break;  //5'b10111;
	}

	//-------------------------------------------------------------------
	// WR and RTP (Table-14)
	//-------------------------------------------------------------------
	switch (RTP)  {
	case 5:
		mr0_rtp = 0x0; break;  //4'b0000;
	case 6:
		mr0_rtp = 0x1; break;  //4'b0001;
	case 7:
		mr0_rtp = 0x2; break;  //4'b0010;
	case 8:
		mr0_rtp = 0x3; break;  //4'b0011;
	case 9:
		mr0_rtp = 0x4; break;  //4'b0100;
	case 10:
		mr0_rtp = 0x5; break;  //4'b0101;
	case 12:
		mr0_rtp = 0x6; break;  //4'b0110;
	case 11:
		mr0_rtp = 0x7; break;  //4'b0111;
	case 13:
		mr0_rtp = 0x8; break;  //4'b1000;
	}

	//-------------------------------------------------------------------
	// CAS Write Latency (Table-21)
	//-------------------------------------------------------------------
	switch (CWL)  {
	case 9:
		mr2_cwl = 0x0; break;  // 3'b000; // 1600
	case 10:
		mr2_cwl = 0x1; break;  // 3'b001; // 1866
	case 11:
		mr2_cwl = 0x2; break;  // 3'b010; // 2133
	case 12:
		mr2_cwl = 0x3; break;  // 3'b011; // 2400
	case 14:
		mr2_cwl = 0x4; break;  // 3'b100; // 2666
	case 16:
		mr2_cwl = 0x5; break;  // 3'b101; // 2933/3200
	case 18:
		mr2_cwl = 0x6; break;  // 3'b110;
	case 20:
		mr2_cwl = 0x7; break;  // 3'b111;
	}

	//-------------------------------------------------------------------
	// tCCD_L and tDLLK
	//-------------------------------------------------------------------
	switch (CCD_L) {
	case 4:
		mr6_tccd_l = 0x0; break;  //3'b000;  // rate <= 1333
	case 5:
		mr6_tccd_l = 0x1; break;  //3'b001;  // 1333 < rate <= 1866
	case 6:
		mr6_tccd_l = 0x2; break;  //3'b010;  // 1866 < rate <= 2400
	case 7:
		mr6_tccd_l = 0x3; break;  //3'b011;  // 2400 < rate <= 2666
	case 8:
		mr6_tccd_l = 0x4; break;  //3'b100;  // 2666 < rate <= 3200
	}

	mr0_val = ((mr0_cas & 0x1) << 2) | (((mr0_cas >> 1) & 0x7) << 4) |
			(((mr0_cas >> 4) & 0x1) << 12) | ((mr0_rtp & 0x7) << 9) |
			(((mr0_rtp >> 3) & 0x1) << 13);
	mr1_val = 0x201;
	mr2_val = ((mr2_cwl & 0x7) << 3) | 0x200;
	mr3_val = 0;
	mr4_val = 0;

	/* mr5_val = {
	 * 1'b0,         // 13: RFU
	 * 1'b0,         // 12: read DBI
	 * 1'b0,         // 11: write DBI
	 * 1'b1,         // 10: Data mask
	 * 1'b0,         // 9: C/A parity persistent error
	 * 3'b000,       // 8-6: RTT_PARK (disable)
	 * 1'b1,         // 5: ODT input buffer during power down mode
	 * 1'b0,         // 4: C/A parity status
	 * 1'b0,         // 3: CRC error clear
	 * 3'b0          // 2-0: C/A parity latency mode
	 * };
	 */
	mr5_val = 0x420;

	/* mr6_val = {
	 *	1'b0,         // 13, 9-8: RFU
	 *	mr6_tccd_l[2:0],   // 12-10: tCCD_L
	 *	2'b0,         // 13, 9-8: RFU
	 *	1'b0,         // 7: VrefDQ training enable
	 *	1'b0,         // 6: VrefDQ training range
	 *	6'b0          // 5-0: VrefDQ training value
	 *	};
	 */
	mr6_val = ((mr6_tccd_l & 0x7) << 10);

	writel((mr1_val << 16) + mr0_val, &regs->mr01_setting);
	writel((mr3_val << 16) + mr2_val, &regs->mr23_setting);
	writel((mr5_val << 16) + mr4_val, &regs->mr45_setting);
	writel(mr6_val, &regs->mr67_setting);

	/* Power-up initialization sequence */
	writel((0x3 << 8) | 0x3, &regs->mode_reg_control);
	while (!((readl(&regs->intr_status) & 0x2)))
		;

	writel((0x6 << 8) | 0x3, &regs->mode_reg_control);
	while (!((readl(&regs->intr_status) & 0x2)))
		;
	writel(2, &regs->intr_clear);

	writel((0x5 << 8) | 0x3, &regs->mode_reg_control);
	while (!(readl(&regs->intr_status) & 0x2))
		;
	writel(2, &regs->intr_clear);

	writel((0x4 << 8) | 0x3, &regs->mode_reg_control);
	while (!((readl(&regs->intr_status) & 0x2)))
		;
	writel(2, &regs->intr_clear);

	writel((0x2 << 8) | 0x3, &regs->mode_reg_control);
	while (!((readl(&regs->intr_status) & 0x2)))
		;
	writel(2, &regs->intr_clear);

	writel((0x1 << 8) | 0x3, &regs->mode_reg_control);
	while (!((readl(&regs->intr_status) & 0x2)))
		;
	writel(2, &regs->intr_clear);

	writel((0x0 << 8) | 0x3, &regs->mode_reg_control);
	while (!((readl(&regs->intr_status) & 0x2)))
		;
	writel(2, &regs->intr_clear);

	/* refresh update */
	writel(0x40b40201, &regs->refresh_control);
}

static int ast2700_sdrammc_probe(struct udevice *dev)
{
	struct dram_info *priv = (struct dram_info *)dev_get_priv(dev);
	struct dramc_regs *regs = priv->regs;
	int i;
	u32 val;

	if (readl(0x12c02900) & 0x40) {
		printf("ddr has already initialized\n");
		goto done;
	}

	ast2700_sdrammc_unlock(priv);
	ast2700_sdrammc_common_init(priv);

	writel(0x10f, 0x12C02000 + 0x400);
	writel(0x30f, 0x12C02000 + 0x400);
	//writel(0x18, &regs->debug_control);
	writel(0x18, &regs->test_configuration);

	writel(0x50000, &regs->dfi_configuration);

	for (i = 0; i < 1000000; ++i)
		;

	writel(0x00020000, &regs->main_control);
	writel(0x00030000, &regs->main_control);
	writel(0x00010000, &regs->main_control);

	for (i = 0; i < 4; i++)
		val = readl(&regs->main_control);

	for (i = 0; i < 1000000; ++i)
		;

	val = readl(&regs->dfi_configuration);
	val &= 0xfffeffff;
	writel(val, &regs->dfi_configuration);

	for (i = 0; i < 4; i++)
		val = readl(&regs->dfi_configuration);

	for (i = 0; i < 1000000; ++i)
		;

	writel(0x00010001, &regs->main_control);

	while ((readl(&regs->intr_status) & 0x1) == 0)
		;

	writel(1, &regs->intr_clear);

	val = readl(&regs->main_control);
	writel(val | 0x2, &regs->main_control);
	while (!(readl(&regs->intr_status) & 0x400))
		;
	writel(0x400, &regs->intr_clear);

	for (i = 0; i < 8; i++)
		val = readl(&regs->dfi_configuration);

	/* 4. initialize dram */
	dramc_configure_mrs(regs);

	val = readl(&regs->refresh_control);
	val &= 0xffff7fff;
	writel(val, &regs->refresh_control);

	val = readl(0x12c02900);
	val |= 0x70;
	writel(val, 0x12c02900);

	writel(0x3c, 0x12c00010);
done:
	priv->info.base = 0x80000000;
	priv->info.size = 0x80000000;

	return 0;
}

static int ast2700_sdrammc_of_to_plat(struct udevice *dev)
{
	struct dram_info *priv = dev_get_priv(dev);

	priv->regs = (void *)(uintptr_t)devfdt_get_addr_index(dev, 0);
	priv->phy_setting = (void *)(uintptr_t)devfdt_get_addr_index(dev, 1);

	return 0;
}

static int ast2700_sdrammc_get_info(struct udevice *dev, struct ram_info *info)
{
	struct dram_info *priv = dev_get_priv(dev);

	*info = priv->info;

	return 0;
}

static struct ram_ops ast2700_sdrammc_ops = {
	.get_info = ast2700_sdrammc_get_info,
};

static const struct udevice_id ast2700_sdrammc_ids[] = {
	{ .compatible = "aspeed,ast2700-sdrammc" },
	{ }
};

U_BOOT_DRIVER(sdrammc_ast2700) = {
	.name = "aspeed_ast2700_sdrammc",
	.id = UCLASS_RAM,
	.of_match = ast2700_sdrammc_ids,
	.ops = &ast2700_sdrammc_ops,
	.of_to_plat = ast2700_sdrammc_of_to_plat,
	.probe = ast2700_sdrammc_probe,
	.priv_auto = sizeof(struct dram_info),
};
