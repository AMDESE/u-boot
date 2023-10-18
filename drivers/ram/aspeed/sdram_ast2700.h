/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 ASPEED Technology Inc.
 */
#ifndef _SDRAM_AST2700_H
#define _SDRAM_AST2700_H

#include <clk.h>
#include <errno.h>
#include <ram.h>
#include <regmap.h>
#include <reset.h>
#include <asm/io.h>
#include <asm/global_data.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/bitfield.h>
#include <time.h>

#define SCU_CPU_REG                     0x12c02000
#define SCU_CPU_SOC1_SCRATCH            (SCU_CPU_REG + 0x900)

#define SCU_IO_REG                      0x14c02000
#define SCU_IO_HWSTRAP1                 (SCU_IO_REG + 0x010)
#define IO_HWSTRAP1_DRAM_TYPE           BIT(10)

#define DRAMC_PHY_BASE			(0x13000000)
#define dwc_ddrphy_apb_wr(addr, value)		(*(volatile unsigned short *)(DRAMC_PHY_BASE + 2 * (addr)) = (unsigned short)value)
#define dwc_ddrphy_apb_rd(addr)			(*(volatile unsigned short *)(DRAMC_PHY_BASE + 2 * (addr)))

#define dwc_ddrphy_apb_wr_32b(addr, value)	(*((volatile unsigned int *)(DRAMC_PHY_BASE + 2 * (addr))) = (unsigned int)value)
#define dwc_ddrphy_apb_rd_32b(addr)		(*(volatile unsigned int *)(DRAMC_PHY_BASE + 2 * (addr)))

/* offset 0x04 */
#define DRAMC_IRQSTA_PWRCTL_ERR			BIT(16)
#define DRAMC_IRQSTA_PHY_ERR			BIT(15)
#define DRAMC_IRQSTA_LOWPOWER_DONE		BIT(12)
#define DRAMC_IRQSTA_FREQ_CHG_DONE		BIT(11)
#define DRAMC_IRQSTA_REF_DONE			BIT(10)
#define DRAMC_IRQSTA_ZQ_DONE			BIT(9)
#define DRAMC_IRQSTA_BIST_DONE			BIT(8)
#define DRAMC_IRQSTA_ECC_RCVY_ERR		BIT(5)
#define DRAMC_IRQSTA_ECC_ERR			BIT(4)
#define DRAMC_IRQSTA_PROT_ERR			BIT(3)
#define DRAMC_IRQSTA_OVERSZ_ERR			BIT(2)
#define DRAMC_IRQSTA_MR_DONE			BIT(1)
#define DRAMC_IRQSTA_PHY_INIT_DONE		BIT(0)

/* offset 0x14 */
#define DRAMC_MCTL_WB_SOFT_RESET		BIT(24)
#define DRAMC_MCTL_PHY_CLK_DIS			BIT(18)
#define DRAMC_MCTL_PHY_RESET			BIT(17)
#define DRAMC_MCTL_PHY_POWER_ON			BIT(16)
#define DRAMC_MCTL_FREQ_CHG_START		BIT(3)
#define DRAMC_MCTL_PHY_LOWPOWER_START		BIT(2)
#define DRAMC_MCTL_SELF_REF_START		BIT(1)
#define DRAMC_MCTL_PHY_INIT_START		BIT(0)

/* offset 0x40 */
#define DRAMC_DFICFG_WD_POL			BIT(18)
#define DRAMC_DFICFG_CKE_OUT			BIT(17)
#define DRAMC_DFICFG_RESET			BIT(16)

/* offset 0x48 */
#define DRAMC_MRCTL_ERR_STATUS			BIT(31)
#define DRAMC_MRCTL_READY_STATUS		BIT(30)
#define DRAMC_MRCTL_MR_ADDR			BIT(8)
#define DRAMC_MRCTL_CMD_DLL_RST			BIT(7)
#define DRAMC_MRCTL_CMD_DQ_SEL			BIT(6)
#define DRAMC_MRCTL_CMD_TYPE			BIT(2)
#define DRAMC_MRCTL_CMD_WR_CTL			BIT(1)
#define DRAMC_MRCTL_CMD_START			BIT(0)

/* DRAMC048 MR Control Register */
#define MR_TYPE_SHIFT				2
#define MR_RW					(0 << MR_TYPE_SHIFT)
#define MR_MPC					BIT(2)
#define MR_VREFCS				(2 << MR_TYPE_SHIFT)
#define MR_VREFCA				(3 << MR_TYPE_SHIFT)

#define MR_ADDRESS_SHIFT			8
#define MR_ADDR(n)				(((n) << MR_ADDRESS_SHIFT) | DRAMC_MRCTL_CMD_WR_CTL)

#define MR_NUM_SHIFT				4
#define MR_NUM(n)				((n) << MR_NUM_SHIFT)

#define MR_DLL_RESET				BIT(7)
#define MR_1T_MODE				BIT(16)

/* MPC command definition */
#define MPC_OP_EXIT_CS			0
#define MPC_OP_ENTER_CS			1
#define MPC_OP_DLL_RESET		2
#define MPC_OP_ENTER_CA			3
#define MPC_OP_ZQCAL_LATCH		4
#define MPC_OP_ZQCAL_START		5
#define MPC_OP_STOP_DQS			6
#define MPC_OP_START_DQS		7
#define MPC_OP_SET_2N_CMD		8
#define MPC_OP_SET_1N_CMD		9
#define MPC_OP_EXIT_PDA			10
#define MPC_OP_ENTER_PDA		11
#define MPC_OP_MANUAL_ECS		12
#define MPC_OP_APPLY			0x1f
#define MPC_OP_RTT_CK_A			0x20
#define MPC_OP_RTT_CK_B			0x28
#define MPC_OP_RTT_CS_A			0x30
#define MPC_OP_RTT_CS_B			0x38
#define MPC_OP_RTT_CA_A			0x40
#define MPC_OP_RTT_CA_B			0x48
#define MPC_OP_SET_DQS_RTT_PARK		0x50
#define MPC_OP_SET_RTT_PARK		0x58
#define MPC_OP_PDA_ENUM_ID		0x60
#define MPC_OP_PDA_SEL_ID		0x70
#define MPC_OP_CONFIG_DLLK_CCD		0x80

/* MR2 */
#define MR2_READ_PREAMBLE_TRAIN		BIT(0)
#define MR2_WRITE_LEVELING		BIT(1)
#define MR2_2N_MODE			BIT(2)
#define MR2_POWER_SAVING		BIT(3)
#define MR2_CS_ASSERTION		BIT(4)
#define MR2_DEVICE_15_PWR_SAVE		BIT(5)
#define MR2_INTERN_WR_TIMING		BIT(7)

/* MR8 */
#define MR8_READ_PREAMBLE_1TCK		0
#define MR8_READ_PREAMBLE_2TCK		1
#define MR8_READ_PREAMBLE_2TCK_DDR4	2
#define MR8_READ_PREAMBLE_3TCK		3
#define MR8_READ_PREAMBLE_4TCK		4
#define MR8_WRITE_PREAMBLE_2TCK		BIT(3)
#define MR8_WRITE_PREAMBLE_3TCK		(2 << 3)
#define MR8_WRITE_PREAMBLE_4TCK		(3 << 3)
#define MR8_READ_POST_0_5TCK		(0 << 6)
#define MR8_READ_POST_1_5TCK		BIT(6)
#define MR8_WRITE_POST_0_5TCK		(0 << 7)
#define MR8_WRITE_POST_1_5TCK		BIT(7)

/* MR10 */
#define MR10_VREFDQ_RANGE_90		(0x0f)
#define MR10_VREFDQ_RANGE_85		(0x19)
#define MR10_VREFDQ_RANGE_80		(0x23)
#define MR10_VREFDQ_RANGE_75		(0x2d)
#define MR10_VREFDQ_RANGE_70		(0x37)

/* MR11 */
#define MR11_VREFCA_RANGE_90		(0x0f)
#define MR11_VREFCA_RANGE_85		(0x19)
#define MR11_VREFCA_RANGE_80		(0x23)
#define MR11_VREFCA_RANGE_75		(0x2d)
#define MR11_VREFCA_RANGE_70		(0x37)

/* MR12 */
#define MR12_VREFCS_RANGE_90		(0x0f)
#define MR12_VREFCS_RANGE_85		(0x19)
#define MR12_VREFCS_RANGE_80		(0x23)
#define MR12_VREFCS_RANGE_75		(0x2d)
#define MR12_VREFCS_RANGE_70		(0x37)

/* MR32 */
#define MR32_CK_ODT_RTT_OFF		(0)
#define MR32_CK_ODT_480			(1)
#define MR32_CK_ODT_240			(2)
#define MR32_CK_ODT_120			(3)
#define MR32_CK_ODT_80			(4)
#define MR32_CK_ODT_60			(5)
#define MR32_CK_ODT_40			(7)

#define MR32_CS_ODT_RTT_OFF		(0)
#define MR32_CS_ODT_480			(1)
#define MR32_CS_ODT_240			(2)
#define MR32_CS_ODT_120			(3)
#define MR32_CS_ODT_80			(4)
#define MR32_CS_ODT_60			(5)
#define MR32_CS_ODT_40			(7)

#define MR32_CA_ODT_STRAP_BIT		BIT(6)

/* MR33 */
#define MR33_CA_ODT_RTT_OFF		(0)
#define MR33_CA_ODT_480			(1)
#define MR33_CA_ODT_240			(2)
#define MR33_CA_ODT_120			(3)
#define MR33_CA_ODT_80			(4)
#define MR33_CA_ODT_60			(5)
#define MR33_CA_ODT_40			(7)

#define MR33_DQS_RTT_PARK_OFF		(0)
#define MR33_DQS_RTT_PARK_240		(1)
#define MR33_DQS_RTT_PARK_120		(2)
#define MR33_DQS_RTT_PARK_80		(3)
#define MR33_DQS_RTT_PARK_60		(4)
#define MR33_DQS_RTT_PARK_40		(5)
#define MR33_DQS_RTT_PARK_34		(7)

/* MR34 */
#define MR34_RTT_PARK_OFF		(0)
#define MR34_RTT_PARK_240		(1)
#define MR34_RTT_PARK_120		(2)
#define MR34_RTT_PARK_80		(3)
#define MR34_RTT_PARK_60		(4)
#define MR34_RTT_PARK_40		(5)
#define MR34_RTT_PARK_34		(7)

/* offset 0xC0 */
#define DRAMC_BISTRES_RUNNING			BIT(10)
#define DRAMC_BISTRES_FAIL			BIT(9)
#define DRAMC_BISTRES_DONE			BIT(8)
#define DRAMC_BISTCFG_INIT_MODE			BIT(7)
#define DRAMC_BISTCFG_PMODE			GENMASK(6, 4)
#define DRAMC_BISTCFG_BMODE			GENMASK(3, 2)
#define DRAMC_BISTCFG_ENABLE			BIT(1)
#define DRAMC_BISTCFG_START			BIT(0)
#define BIST_PMODE_CRC				(3)
#define BIST_BMODE_RW_SWITCH			(3)

struct sdramc {
	struct ram_info info;
	struct sdramc_regs *regs;
	void __iomem *phy_setting;
	void __iomem *phy_status;
	ulong clock_rate;
};

struct sdramc_port {
	u32 configuration;
	u32 timeout;
	u32 read_qos;
	u32 write_qos;
	u32 monitor_config;
	u32 monitor_limit;
	u32 monitor_timer;
	u32 monitor_status;
	u32 bandwidth_log;
	u32 intf_monitor[3];
};

struct sdramc_protect {
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

struct sdramc_regs {
	u32 prot_key;			/* offset 0x00 */
	u32 intr_status;		/* offset 0x04 */
	u32 intr_clear;			/* offset 0x08 */
	u32 intr_mask;			/* offset 0x0C */
	u32 mcfg;			/* offset 0x10 */
	u32 mctl;
	u32 msts;
	u32 error_status;
	u32 actime1;
	u32 actime2;
	u32 actime3;
	u32 actime4;
	u32 actime5;
	u32 actime6;
	u32 actime7;
	u32 dfi_timing;
	u32 dcfg;
	u32 dctl;
	u32 mrctl;
	u32 mrwr;
	u32 mrrd;
	u32 mr01;
	u32 mr23;
	u32 mr45;
	u32 mr67;
	u32 refctl;
	u32 refmng_ctl;
	u32 refsts;
	u32 zqctl;			/* offset 0x70 */
	u32 ecc_addr_range;		/* offset 0x74 */
	u32 ecc_failure_status;		/* offset 0x78 */
	u32 ecc_failure_addr;		/* offset 0x7C */
	u32 ecc_test_control;		/* offset 0x80 */
	u32 ecc_test_status;		/* offset 0x84 */
	u32 arbctl;			/* offset 0x88 */
	u32 enc_configuration;		/* offset 0x8c */
	u32 protect_lock_set;		/* offset 0x90 */
	u32 protect_lock_status;	/* offset 0x94 */
	u32 protect_lock_reset;		/* offset 0x98 */
	u32 enc_min_address;		/* offset 0x9c */
	u32 enc_max_address;		/* offset 0xa0 */
	u32 enc_key[4];			/* offset 0xa4~0xb0 */
	u32 enc_iv[3];			/* offset 0xb4~0xbc */
	u32 bistcfg;			/* offset 0xc0 */
	u32 bist_addr;
	u32 bist_size;
	u32 bist_patt;
	u32 bist_res;
	u32 bist_fail_addr;
	u32 bist_fail_data[4];
	u32 reserved2[2];
	u32 debug_control;		/* offset 0xf0 */
	u32 debug_status;
	u32 phy_intf_status;
	u32 testcfg;
	u32 gfmcfg;			/* 0x100 */
	u32 gfm0ctl;
	u32 gfm1ctl;
	u32 reserved3[0xf8];
	struct sdramc_port port[6];	/* 0x200 */
	struct sdramc_protect region[16];/* 0x600 */
};

enum {
	SDRAM_SIZE_256MB = 0,
	SDRAM_SIZE_512MB,
	SDRAM_SIZE_1GB,
	SDRAM_SIZE_2GB,
	SDRAM_SIZE_4GB,
	SDRAM_SIZE_8GB,
	SDRAM_SIZE_MAX,
};

enum {
	SDRAM_VGA_RSVD_32MB = 0,
	SDRAM_VGA_RSVD_64MB,
};

enum ddr_speed_bin {
	DDR4_1600,
	DDR4_2400,
	DDR4_3200,
	DDR5_3200,
};

enum ddr_type {
	DRAM_TYPE_4,
	DRAM_TYPE_5,
	DRAM_TYPE_MAX,
};

struct sdramc_ac_timing {
	u32 type;
	char desc[30];
	u32 t_cl;
	u32 t_cwl;
	u32 t_bl;
	u32 t_rcd;		/* ACT-to-read/write command delay */
	u32 t_rp;		/* PRE command period */
	u32 t_ras;		/* ACT-to-PRE command delay */
	u32 t_rrd;		/* ACT-to-ACT delay for different BG */
	u32 t_rrd_l;		/* ACT-to-ACT delay for same BG */
	u32 t_faw;		/* Four active window */
	u32 t_rtp;		/* Read-to-PRE command delay */
	u32 t_wtr;		/* Minimum write to read command for different BG */
	u32 t_wtr_l;		/* Minimum write to read command for same BG */
	u32 t_wtr_a;		/* Write to read command for same BG with auto precharge */
	u32 t_wtp;		/* Minimum write to precharge command delay */
	u32 t_rtw;		/* minimum read to write command */
	u32 t_ccd_l;		/* CAS-to-CAS delay for same BG */
	u32 t_dllk;		/* DLL locking time */
	u32 t_cksre;		/* valid clock before after self-refresh or power-down entry/exit process */
	u32 t_pd;		/* power-down entry to exit minimum width */
	u32 t_xp;		/* exit power-down to valid command delay */
	u32 t_rfc;		/* refresh time period */
	u32 t_mrd;
	u32 t_refsbrd;
	u32 t_rfcsb;
	u32 t_cshsr;
	u32 t_zq;
};

void fpga_phy_init(struct sdramc *sdramc);
void dwc_phy_init(struct sdramc_ac_timing *ac);
bool is_ddr4(void);
#endif
