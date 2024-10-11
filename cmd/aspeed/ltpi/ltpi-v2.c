// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright (c) 2023 ASPEED Technology Inc.
 *
 */
#include <stdint.h>
#include <asm/io.h>
#include <time.h>
#include <linux/bitfield.h>
#include <linux/delay.h>
#include <bootstage.h>
#include <asm/arch-aspeed/scu_ast2700.h>
#include <command.h>
#include <getopt.h>
#include <console.h>
#include "ltpi-v2.h"
#include "ltpi_top.h"
#include "ltpi_ctrl.h"
#include "internal.h"

#define SCU1_REG				0x14c02000
#define LTPI_REG				0x14c34000
#define SGPIOS_REG				0x14c3c000

#define SCU1_HWSTRAP1				(SCU1_REG + 0x010)
#define   SCU1_HWSTRAP1_LTPI0_IO_DRIVING	GENMASK(15, 14)
#define   SCU1_HWSTRAP1_EN_RECOVERY_BOOT	BIT(4)
#define   SCU1_HWSTRAP1_LTPI0_EN		BIT(3)
#define   SCU1_HWSTRAP1_LTPI_IDX		BIT(2)
#define   SCU1_HWSTRAP1_LTPI1_EN		BIT(1)
#define   SCU1_HWSTRAP1_LTPI_MODE		BIT(0)
#define SCU1_RSTCTL2				(SCU1_REG + 0x220)
#define   SCU1_RSTCTL2_LTPI1			BIT(22)
#define   SCU1_RSTCTL2_LTPI0			BIT(20)
#define SCU1_RSTCTL2_CLR			(SCU1_REG + 0x224)
#define SCU1_CLKGATE2				(SCU1_REG + 0x260)
#define   SCU1_CLKGATE2_LTPI1_TX		BIT(19)
#define   SCU1_CLKGATE2_LTPI_AHB		BIT(10)
#define   SCU1_CLKGATE2_LTPI0_TX		BIT(9)
#define SCU1_CLKGATE2_CLR			(SCU1_REG + 0x264)
#define SCU1_HPLL_1				(SCU1_REG + 0x300)
#define SCU1_HPLL_2				(SCU1_REG + 0x304)
#define SCU1_APLL_1				(SCU1_REG + 0x310)
#define SCU1_APLL_2				(SCU1_REG + 0x314)
#define SCU1_DPLL_1				(SCU1_REG + 0x320)
#define SCU1_DPLL_2				(SCU1_REG + 0x324)
#define SCU1_L0PLL_1				(SCU1_REG + 0x340)
#define SCU1_L0PLL_2				(SCU1_REG + 0x344)
#define SCU1_L1PLL_1				(SCU1_REG + 0x350)
#define SCU1_L1PLL_2				(SCU1_REG + 0x354)
#define SCU1_PINMUX_GRP_B			(SCU1_REG + 0x404)
#define SCU1_PINMUX_GRP_C			(SCU1_REG + 0x408)
#define SCU1_PINMUX_GRP_D			(SCU1_REG + 0x40c)
#define SCU1_PINMUX_GRP_F			(SCU1_REG + 0x414)
#define SCU1_PINMUX_GRP_H			(SCU1_REG + 0x41c)
#define SCU1_PINMUX_GRP_P			(SCU1_REG + 0x43c)
#define SCU1_PINMUX_GRP_T			(SCU1_REG + 0x44c)
#define SCU1_PINMUX_GRP_U			(SCU1_REG + 0x450)
#define SCU1_PINMUX_GRP_AA			(SCU1_REG + 0x468)
#define   SCU1_PINMUX_PIN7			GENMASK(31, 28)
#define   SCU1_PINMUX_PIN6			GENMASK(27, 24)
#define   SCU1_PINMUX_PIN5			GENMASK(23, 20)
#define   SCU1_PINMUX_PIN4			GENMASK(19, 16)
#define   SCU1_PINMUX_PIN3			GENMASK(15, 12)
#define   SCU1_PINMUX_PIN2			GENMASK(11, 8)
#define   SCU1_PINMUX_PIN1			GENMASK(7, 4)
#define   SCU1_PINMUX_PIN0			GENMASK(3, 0)
#define SCU1_OTPCFG				(SCU1_REG + 0x880)
#define SCU1_OTPCFG_03_02			(SCU1_OTPCFG + 0x4)
#define   OTPCFG2_DIS_RECOVERY_MODE		BIT(3)
#define SCU1_OTPCFG_23_22			(SCU1_OTPCFG + 0x2c)
#define   SCU1_OTPCFG23_RESERVED		GENMASK(31, 24)
#define   SCU1_OTPCFG23_LTPI1_PHYCLK_INV	GENMASK(23, 22)
#define   SCU1_OTPCFG23_LTPI0_PHYCLK_INV	GENMASK(21, 20)
#define   SCU1_OTPCFG23_LTPI1_IO_DRIVING	GENMASK(19, 18)
#define   SCU1_OTPCFG23_LTPI_FW_DL_ENA		BIT(17)
#define   SCU1_OTPCFG23_LTPI_CRC_FORMAT		BIT(16)
#define SCU1_OTPCFG_31_30			(SCU1_OTPCFG + 0x3c)
#define   SCU1_OTPCFG31_LTPI1_DDR_DIS		BIT(31)
#define   SCU1_OTPCFG31_LTPI1_SPEED_CAPA_DIS	GENMASK(30, 16)
#define   SCU1_OTPCFG30_LTPI0_DDR_DIS		BIT(15)
#define   SCU1_OTPCFG30_LTPI0_SPEED_CAPA_DIS	GENMASK(14, 0)

#define ADVERTISE_TIMEOUT_US			105000 /* 105 ms */

struct bootstage_t {
	uint8_t errno;
	uint8_t syndrome;
	union {
		uint16_t boot2fmc;
		uint16_t panic;
	};
};

struct rom_context {
	uint32_t dummy;
};

struct ltpi_reset {
	uintptr_t regs_assert;
	uintptr_t regs_deassert;
	uint32_t bit_mask;
};

struct ltpi_clk_ctrl {
	uintptr_t regs_gate;
	uintptr_t regs_ungate;
	uint32_t bit_mask;
};

struct ltpi_clk_info {
	int16_t freq;		/* clock frequency in MHz*/
	int16_t clk_sel;	/* clock selection */
};

static const struct ltpi_clk_info ltpi_clk_lookup_sdr[13] = {
	{ 25, REG_LTPI_PLL_25M },
	{ 50, REG_LTPI_PLL_LPLL },
	{ 75, REG_LTPI_PLL_LPLL },
	{ 100, REG_LTPI_PLL_LPLL },
	{ 150, REG_LTPI_PLL_LPLL },
	{ 200, REG_LTPI_PLL_LPLL },
	{ 250, REG_LTPI_PLL_LPLL },
	{ 300, REG_LTPI_PLL_LPLL },
	{ 400, REG_LTPI_PLL_LPLL },
	{ 600, REG_LTPI_PLL_LPLL },
	{ -1, -1 },
	{ -1, -1 },
	{ 500, REG_LTPI_PLL_LPLL }
};

static const struct ltpi_clk_info ltpi_clk_lookup_ddr[13] = {
	{ 50, REG_LTPI_PLL_LPLL },
	{ 100, REG_LTPI_PLL_LPLL },
	{ 150, REG_LTPI_PLL_LPLL },
	{ 200, REG_LTPI_PLL_LPLL },
	{ 300, REG_LTPI_PLL_LPLL },
	{ 400, REG_LTPI_PLL_LPLL },
	{ 500, REG_LTPI_PLL_LPLL },
	{ 600, REG_LTPI_PLL_LPLL },
	{ 800, REG_LTPI_PLL_LPLL },
	{ 1200, REG_LTPI_PLL_LPLL },
	{ -1, -1 },
	{ -1, -1 },
	{ 1000, REG_LTPI_PLL_LPLL }
};

struct ltpi_priv {
	uintptr_t base;
	uintptr_t phy_base;
	uintptr_t top_base;
	uintptr_t gpio_base;

	struct ltpi_reset reset;
	struct ltpi_clk_ctrl clk_ctrl;

	/* encoding as LTPI speed capability */
	uint16_t otp_speed_cap;	/* limit the speed via OTP strap */
	uint16_t phy_speed_cap; /* limit the speed with physical line status */
	bool otp_ddr_dis;

	int crc_format;
	int io_driving;

#define RX_CLK_INVERSE		BIT(1)
#define TX_CLK_INVERSE		BIT(0)
	int clk_inverse;

	int index;
	struct bootstage_t *bootstage;

	/* Advertise timeout in us */
	int ad_timeout;
};

static struct ltpi_priv ltpi_data[2];

/**
 * @brief Count the number of leading zeros of a uint16_t
 * @param [IN] x - the uint16_t to be counted
 * @return the number of leading zeros in x
 */
static int clz16(uint16_t x)
{
	int n = 0;

	if (x == 0)
		return 16;

	if (x <= 0x00ff) {
		n += 8;
		x <<= 8;
	}
	if (x <= 0x0fff) {
		n += 4;
		x <<= 4;
	}
	if (x <= 0x3fff) {
		n += 2;
		x <<= 2;
	}
	if (x <= 0x7fff)
		n += 1;

	return n;
}

/**
 * @brief find the bit index of the max attainable speed from the bitmap
 * @param [IN] cap - bitmap of the speed capability
 * @return the bit index of the max attainable speed
 */
static uint16_t find_max_speed(uint16_t cap)
{
	return 15 - clz16(cap & ~LTPI_SP_CAP_DDR);
}

static void ltpi_phy_unlock(struct ltpi_priv *ltpi)
{
	writel(LTPI_PROT_KEY_UNLOCK, (void *)ltpi->phy_base + LTPI_PROT_KEY);
}

static void ltpi_enable_rx_bias(struct ltpi_priv *ltpi)
{
	setbits_le32((void *)ltpi->top_base + LTPI_LVDS_RX_CTRL,
		     REG_LTPI_LVDS_RX1_BIAS_EN | REG_LTPI_LVDS_RX0_BIAS_EN);
	udelay(1);
}

static int ltpi_phy_get_mode(struct ltpi_priv *ltpi)
{
	uint32_t reg = readl((void *)ltpi->phy_base + LTPI_PHY_CTRL);

	return FIELD_GET(REG_LTPI_PHY_MODE, reg);
}

static int ltpi_phy_set_mode(struct ltpi_priv *ltpi, int mode)
{
	uint32_t reg;

	if (mode < 0 || mode > LTPI_PHY_MODE_CDR_HI_SP) {
		printf("%s: invalid mode %d\n", __func__, mode);
		return -1;
	}

	reg = readl((void *)ltpi->phy_base + LTPI_PHY_CTRL);
	reg &= ~REG_LTPI_PHY_MODE;
	reg |= mode;
	writel(reg, (void *)ltpi->phy_base + LTPI_PHY_CTRL);

	return 0;
}

static int ltpi_phy_set_clksel(struct ltpi_priv *ltpi, int clksel, bool is_op_clk)
{
	uint32_t reg;

	reg = readl((void *)ltpi->phy_base + LTPI_PLL_CTRL);
	reg &= ~(REG_LTPI_PLL_SELECT | REG_LTPI_PLL_SET |
		 REG_LTPI_RX_PHY_CLK_INV | REG_LTPI_TX_PHY_CLK_INV);
	reg |= FIELD_PREP(REG_LTPI_PLL_SELECT, clksel);

	if (ltpi->clk_inverse & RX_CLK_INVERSE)
		reg |= REG_LTPI_RX_PHY_CLK_INV;

	if (ltpi->clk_inverse & TX_CLK_INVERSE)
		reg |= REG_LTPI_TX_PHY_CLK_INV;

	if (is_op_clk)
		reg |= REG_LTPI_PLL_SET;

	writel(reg, (void *)ltpi->phy_base + LTPI_PLL_CTRL);

	return 0;
}

static void ltpi_set_crc_format(struct ltpi_priv *ltpi, int crc_fmt)
{
	uint32_t val = readl((void *)ltpi->base + LTPI_CRC_OPTION);

	val &= ~(REG_LTPI_SW_CRC_OUT_ML_FIRST | REG_LTPI_SW_CRC_IN_LSB_FIRST);
	if (crc_fmt)
		val |= REG_LTPI_SW_CRC_OUT_ML_FIRST | REG_LTPI_SW_CRC_IN_LSB_FIRST;

	writel(val, (void *)ltpi->base + LTPI_CRC_OPTION);
}

static int ltpi_reset(struct ltpi_priv *ltpi)
{
	writel(ltpi->reset.bit_mask, (void *)ltpi->reset.regs_assert);
	readl((void *)ltpi->reset.regs_assert);

	udelay(1);

	writel(ltpi->clk_ctrl.bit_mask, (void *)ltpi->clk_ctrl.regs_ungate);
	readl((void *)ltpi->clk_ctrl.regs_ungate);

	writel(ltpi->reset.bit_mask, (void *)ltpi->reset.regs_deassert);
	readl((void *)ltpi->reset.regs_deassert);

	ltpi_phy_unlock(ltpi);

	return 0;
}

static uint32_t ltpi_get_link_mng_state(struct ltpi_priv *ltpi)
{
	return FIELD_GET(REG_LTPI_LINK_MNG_ST,
			 readl((void *)ltpi->base + LTPI_LINK_MNG_ST));
}

static int ltpi_poll_link_mng_state(struct ltpi_priv *ltpi, uint32_t expected,
				    uint32_t unexpected, int timeout_us)
{
	uint64_t start, timeout_tick;
	uintptr_t addr = ltpi->base + LTPI_LINK_MNG_ST;
	uint32_t reg = readl((void *)addr);
	uint32_t state;
	int ret = LTPI_ERR_NONE;

	if (timeout_us) {
		start = get_ticks();
		timeout_tick = usec2ticks(timeout_us);
	}

	do {
		reg = readl((void *)addr);
		state = FIELD_GET(REG_LTPI_LINK_MNG_ST, reg);

		if (state == expected)
			break;

		if (state == unexpected) {
			/* link is disconnected, break the loop directly */
			ret = LTPI_ERR_DISCON;
			break;
		}

		if (timeout_us && ((get_ticks() - start) > timeout_tick)) {
			ret = LTPI_ERR_TIMEOUT;
			break;
		}
	} while (1);

	return ret;
}

static int ltpi_wait_state_pll_set(struct ltpi_priv *ltpi, int timeout_us)
{
	return ltpi_poll_link_mng_state(ltpi, LTPI_LINK_MNG_ST_WAIT_PLL_SET, -1,
					timeout_us);
}

static int ltpi_wait_state_op(struct ltpi_priv *ltpi)
{
	setbits_le32((void *)ltpi->base + LTPI_LINK_ST,
		     REG_LTPI_CON_ACC_TO_ERR | REG_LTPI_FRM_CRC_ERR | REG_LTPI_LINK_LOST_ERR);

	return ltpi_poll_link_mng_state(ltpi, LTPI_LINK_MNG_ST_OP,
					LTPI_LINK_MNG_ST_DETECT_ALIGN,
					ADVERTISE_TIMEOUT_US);
}

static int ltpi_set_lvds_io_driving(struct ltpi_priv *ltpi, int driving)
{
	uint32_t val;

	setbits_le32((void *)ltpi->top_base + LTPI_SW_FORCE_EN, REG_LTPI_SW_FORCE_LVDS_TX_DS_EN);

	val = readl((void *)ltpi->top_base + LTPI_LVDS_TX_CTRL);
	val &= ~(REG_LTPI_LVDS_TX1_DS1 | REG_LTPI_LVDS_TX1_DS0 |
		 REG_LTPI_LVDS_TX0_DS1 | REG_LTPI_LVDS_TX0_DS0);

	/* tx1: clk, tx0: data */
	if (driving & BIT(1))
		val |= (REG_LTPI_LVDS_TX1_DS1 | REG_LTPI_LVDS_TX0_DS1);

	if (driving & BIT(0))
		val |= (REG_LTPI_LVDS_TX1_DS0 | REG_LTPI_LVDS_TX0_DS0);

	writel(val, (void *)ltpi->top_base + LTPI_LVDS_TX_CTRL);

	return 0;
}

static int ltpi_set_local_speed_cap(struct ltpi_priv *ltpi, uint32_t speed_cap)
{
	uint32_t reg;

	/* only set bits that Aspeed SOC supported */
	speed_cap &= LTPI_SP_CAP_ASPEED_SUPPORTED;
	if (ltpi->otp_ddr_dis)
		speed_cap &= ~LTPI_SP_CAP_DDR;

	reg = readl((void *)ltpi->base + LTPI_CAP_LOCAL);
	reg &= ~REG_LTPI_SP_CAP_LOCAL;
	reg |= FIELD_PREP(REG_LTPI_SP_CAP_LOCAL, speed_cap);
	writel(reg, (void *)ltpi->base + LTPI_CAP_LOCAL);

	return 0;
}

void bootstage_prologue(const char *mark)
{
	if (!mark)
		return;

	printf("%s", mark);
}

void bootstage_epilogue(struct bootstage_t sts)
{
	printf(" %02x%02x\n", sts.errno, sts.syndrome);
}

static void ltpi_log_exit(struct ltpi_priv *ltpi, int reason)
{
	ltpi->bootstage->errno |= LTPI_STATUS_EXIT;
	ltpi->bootstage->syndrome = reason;
}

static void ltpi_log_restart(struct ltpi_priv *ltpi, int reason)
{
	ltpi->bootstage->errno |= LTPI_STATUS_RESTART;
	ltpi->bootstage->syndrome = reason;
	bootstage_epilogue(*ltpi->bootstage);

	/* Restart a boot log */
	bootstage_prologue(BOOTSTAGE_LTPI_INIT);
	ltpi->bootstage->errno &= ~LTPI_STATUS_RESTART;
	ltpi->bootstage->errno &= ~LTPI_STATUS_HAS_CRC_ERR;
	ltpi->bootstage->syndrome = LTPI_SYND_OK;
}

static void ltpi_log_phy_mode(struct ltpi_priv *ltpi, int phy_mode)
{
	ltpi->bootstage->errno &= ~LTPI_STATUS_MODE;
	ltpi->bootstage->errno |= FIELD_PREP(LTPI_STATUS_MODE, phy_mode);
}

/*
 * Link training phase:
 * Link lost -> Link detect frame alignment -> Link detect -> Link speed -> Wait PLL set
 */
static void ltpi_do_link_training(struct ltpi_priv *ltpi)
{
	/* Reset the PHY to PHY_MODE_OFF */
	ltpi_reset(ltpi);

	ltpi_enable_rx_bias(ltpi);
	ltpi_set_local_speed_cap(ltpi, ltpi->phy_speed_cap);
	ltpi_set_lvds_io_driving(ltpi, ltpi->io_driving);
	ltpi_set_crc_format(ltpi, ltpi->crc_format);

	/* Set the clock source to the base frequency 25MHz */
	ltpi_phy_set_clksel(ltpi, REG_LTPI_PLL_25M, false);

	/* To make the remote side back to the link lost state */
	udelay(ADVERTISE_TIMEOUT_US);

	ltpi_phy_set_mode(ltpi, LTPI_PHY_MODE_SDR);
}

#define SCU_PLL_ID_IO_HPLL	0
#define SCU_PLL_ID_IO_APLL	1
#define SCU_PLL_ID_IO_L0PLL	2
#define SCU_PLL_ID_IO_L1PLL	3
#define SCU_PLL_ID_MAX		SCU_PLL_ID_IO_L1PLL

#define PLL_REG1_RESET		BIT(25)
#define PLL_REG1_BYPASS		BIT(24)
#define PLL_REG1_DIS		BIT(23)
#define PLL_REG1_P		GENMASK(22, 19)
#define PLL_REG1_N		GENMASK(18, 13)
#define PLL_REG1_M		GENMASK(12, 0)

#define PLL_REG2_LOCK		BIT(31)
#define PLL_REG2_BWADJ		GENMASK(11, 0)

struct pll_info {
	uintptr_t reg_offset0;
	uintptr_t reg_offset1;
};

struct pll_param {
	int freq;
	uint32_t n_m_p;
	uint16_t bwadj;
};

static const struct pll_info scu_pll_info[SCU_PLL_ID_MAX + 1] = {
	[SCU_PLL_ID_IO_HPLL] = { .reg_offset0 = SCU1_HPLL_1, .reg_offset1 = SCU1_HPLL_2 },
	[SCU_PLL_ID_IO_APLL] = { .reg_offset0 = SCU1_APLL_1, .reg_offset1 = SCU1_APLL_2 },
	[SCU_PLL_ID_IO_L0PLL] = { .reg_offset0 = SCU1_L0PLL_1, .reg_offset1 = SCU1_L0PLL_2 },
	[SCU_PLL_ID_IO_L1PLL] = { .reg_offset0 = SCU1_L1PLL_1, .reg_offset1 = SCU1_L1PLL_2 },
};

#define MHZ(x)			((x) * 1000000)
#define NUM_PLL_PARAM		13
#define REG_N_M_P(n, m, p)	((((n) - 1) << 13) | ((m) - 1) | (((p) - 1) << 19))
#define REG_BWADJ(bwadj)	((bwadj) - 1)

static const struct pll_param pll_param_lookup[NUM_PLL_PARAM] = {
	{ .freq = MHZ(50), .n_m_p = REG_N_M_P(1, 32, 16), .bwadj = REG_BWADJ(16) },
	{ .freq = MHZ(75), .n_m_p = REG_N_M_P(1, 48, 16), .bwadj = REG_BWADJ(24) },
	{ .freq = MHZ(100), .n_m_p = REG_N_M_P(1, 56, 14), .bwadj = REG_BWADJ(28) },
	{ .freq = MHZ(150), .n_m_p = REG_N_M_P(1, 60, 10), .bwadj = REG_BWADJ(30) },
	{ .freq = MHZ(200), .n_m_p = REG_N_M_P(1, 48, 6), .bwadj = REG_BWADJ(24) },
	{ .freq = MHZ(250), .n_m_p = REG_N_M_P(1, 60, 6), .bwadj = REG_BWADJ(30) },
	{ .freq = MHZ(300), .n_m_p = REG_N_M_P(1, 48, 4), .bwadj = REG_BWADJ(24) },
	{ .freq = MHZ(400), .n_m_p = REG_N_M_P(1, 32, 2), .bwadj = REG_BWADJ(16) },
	{ .freq = MHZ(500), .n_m_p = REG_N_M_P(1, 40, 2), .bwadj = REG_BWADJ(20) },
	{ .freq = MHZ(600), .n_m_p = REG_N_M_P(1, 48, 2), .bwadj = REG_BWADJ(24) },
	{ .freq = MHZ(800), .n_m_p = REG_N_M_P(1, 32, 1), .bwadj = REG_BWADJ(16) },
	{ .freq = MHZ(1000), .n_m_p = REG_N_M_P(1, 40, 1), .bwadj = REG_BWADJ(20) },
	{ .freq = MHZ(1200), .n_m_p = REG_N_M_P(1, 48, 1), .bwadj = REG_BWADJ(24) },
};

int scu_get_pll_freq(int pll_id)
{
	const struct pll_info *info;
	uint32_t reg;
	int m, n, p;

	if (pll_id < 0 || pll_id > SCU_PLL_ID_MAX)
		return -1;

	info = &scu_pll_info[pll_id];
	reg = readl((void *)info->reg_offset0);
	m = FIELD_GET(PLL_REG1_M, reg);
	n = FIELD_GET(PLL_REG1_N, reg);
	p = FIELD_GET(PLL_REG1_P, reg);

	return (25000000 * (m + 1) / (n + 1) / (p + 1));
}

int scu_set_pll_freq(int pll_id, int freq)
{
	const struct pll_info *info;
	const struct pll_param *param;
	int curr_freq, i;
	bool match = false;

	if (pll_id < 0 || pll_id > SCU_PLL_ID_MAX)
		return -EINVAL;

	curr_freq = scu_get_pll_freq(pll_id);
	if (curr_freq == freq)
		return 0;

	for (i = 0; i < NUM_PLL_PARAM; i++) {
		if (freq == pll_param_lookup[i].freq) {
			match = true;
			break;
		}
	}

	if (!match)
		return -EINVAL;

	param = &pll_param_lookup[i];

	info = &scu_pll_info[pll_id];
	setbits_le32((void *)info->reg_offset0, PLL_REG1_RESET);
	clrsetbits_le32((void *)info->reg_offset0, PLL_REG1_P | PLL_REG1_N | PLL_REG1_M,
			param->n_m_p);

	clrsetbits_le32((void *)info->reg_offset1, PLL_REG2_BWADJ, param->bwadj);

	/* Wait 5us to ensure the parameters are set  */
	udelay(5);
	clrbits_le32((void *)info->reg_offset0, PLL_REG1_RESET);

	/* PLL should be locked after 20us */
	udelay(20);

	return 0;
}

static int ltpi_set_operational_clk(struct ltpi_priv *ltpi, uint16_t speed_cap)
{
	const struct ltpi_clk_info *info;
	int target_speed, clksel, phy_mode, pll_id;

	if (ltpi->index)
		pll_id = SCU_PLL_ID_IO_L1PLL;
	else
		pll_id = SCU_PLL_ID_IO_L0PLL;

	/* find max attainable speed */
	target_speed = find_max_speed(speed_cap);

	/* set phy mode "OFF" */
	ltpi_phy_set_mode(ltpi, LTPI_PHY_MODE_OFF);

	if (speed_cap & LTPI_SP_CAP_DDR) {
		phy_mode = LTPI_PHY_MODE_DDR;
		info = ltpi_clk_lookup_ddr;
	} else {
		phy_mode = LTPI_PHY_MODE_SDR;
		info = ltpi_clk_lookup_sdr;
	}
	clksel = info[target_speed].clk_sel;
	ltpi_phy_set_clksel(ltpi, clksel, true);
	if (clksel == REG_LTPI_PLL_LPLL)
		scu_set_pll_freq(pll_id, info[target_speed].freq * 1000000);

	/* Start TX with the operational frequency */
	ltpi_phy_set_mode(ltpi, phy_mode);

	ltpi_log_phy_mode(ltpi, phy_mode);

	return target_speed;
}

static void ltpi_scm_init(struct ltpi_priv *ltpi)
{
	int ret, target_speed;
	uint32_t reg, state;

	/* Check whether LTPI is initialized */
	state = ltpi_get_link_mng_state(ltpi);
	if (state == LTPI_LINK_MNG_ST_OP) {
		ltpi_log_phy_mode(ltpi, ltpi_phy_get_mode(ltpi));
		ltpi_log_exit(ltpi, LTPI_SYND_OK_ALREADY_INIT);
		return;
	}

	/* LTPI initialization is required, start link training phase */
	do {
		ltpi_do_link_training(ltpi);
		do {
			ret = ltpi_wait_state_pll_set(ltpi, 20000);
			if (ret == LTPI_ERR_NONE)
				break;

			if (ctrlc()) {
				ltpi_log_exit(ltpi, LTPI_SYND_EXTRST_LINK_TRAINING);
				goto ltpi_scm_exit;
			}
		} while (1);

		/* read intersection of the speed capabilities */
		reg = FIELD_GET(REG_LTPI_SP_INTERSETION, readl((void *)ltpi->base + LTPI_LINK_MNG_ST));
		if (reg == 0) {
			ltpi_log_exit(ltpi, LTPI_SYND_NO_COMMOM_SPEED);
			goto ltpi_scm_exit;
		}

		target_speed = ltpi_set_operational_clk(ltpi, reg);

		/* poll link state 0x7 */
		ret = ltpi_wait_state_op(ltpi);
		if (ret == LTPI_ERR_NONE) {
			/* Start OEM TX & RX if the link partner is AST1700 */
			if (readl((void *)ltpi->base + LTPI_LINK_MNG_ST) & REG_LTPI_LINK_PARTNER_FLAG)
				setbits_le32((void *)ltpi->base + LTPI_OEM_BUS_SETTING,
					     REG_LTPI_OEM_RX_START_TRIG |
					     REG_LTPI_OEM_TX_START_TRIG);

			break;
		}

		if (readl((void *)ltpi->base + LTPI_LINK_ST) & REG_LTPI_FRM_CRC_ERR)
			ltpi->bootstage->errno |= LTPI_STATUS_HAS_CRC_ERR;

		if (ctrlc()) {
			ltpi_log_exit(ltpi, LTPI_SYND_EXTRST_LINK_CONFIG);
			goto ltpi_scm_exit;
		}

		/* clear the bit to specify the current speed doesn't work */
		ltpi->phy_speed_cap &= ~BIT(target_speed);

		/* the lowest speed 25M should always be supported */
		if (ltpi->phy_speed_cap == 0)
			ltpi->phy_speed_cap |= BIT(0);

		ltpi_log_restart(ltpi, LTPI_SYND_WAIT_OP_TO);
	} while (1);

	return;

ltpi_scm_exit:
	ltpi_reset(ltpi);
}

#define LTPI_SGPIOS_PARALLEL_PIN_NUM 64
#define SGPIOS_INVERT_POINT 10
/*
 * Default value:
 * BMC_SGPO [3:0] = 1
 * BMC_SGPO [15:4] = 0
 * BMC_SGPO [19:16] = 1
 * BMC_SGPO [23:20] = 0
 * BMC_SGPO [27:24] = 1
 * BMC_SGPO [31:28] = 0
 * BMC_SGPO [35:32] = 1
 * BMC_SGPO [47:36] = 0
 * BMC_SGPO [51:48] = 1
 * BMC_SGPO [63:52] = 0
 */
static u8 sgpo_hl_invert_point[SGPIOS_INVERT_POINT] = {
	4, 16, 20, 24, 28, 32, 36, 48, 52, LTPI_SGPIOS_PARALLEL_PIN_NUM
};

#define SGPIO_G7_CTRL_REG_BASE 0x80
#define SGPIO_G7_CTRL_REG_OFFSET(x) (SGPIO_G7_CTRL_REG_BASE + (x) * 0x4)
#define SGPIO_G7_PARALLEL_OUT_DATA BIT(1)
static void ltpi_scm_sgpios_init(void)
{
	u32 invert_index = 0;
	u8 val = 1;

	for (u32 i = 0; i < LTPI_SGPIOS_PARALLEL_PIN_NUM;) {
		if (val)
			setbits_le32((void *)SGPIOS_REG + SGPIO_G7_CTRL_REG_OFFSET(i),
				     SGPIO_G7_PARALLEL_OUT_DATA);
		else
			clrbits_le32((void *)SGPIOS_REG + SGPIO_G7_CTRL_REG_OFFSET(i),
				     SGPIO_G7_PARALLEL_OUT_DATA);
		i++;
		if (i == sgpo_hl_invert_point[invert_index]) {
			val = !val;
			invert_index++;
		}
	}
	writel(0x1, (void *)SGPIOS_REG);
}

static void ltpi_scm_set_pins(void)
{
	/*
	 * Pin       MF
	 * --------------------------
	 * GPIOB7    5: SCM_GPO7
	 * GPIOB6    5: SCM_GPO6
	 * GPIOB5    5: SCM_GPO5
	 * GPIOB4    5: SCM_GPO4
	 * GPIOB3    5: SCM_GPO3
	 * GPIOB2    5: SCM_GPO2
	 * GPIOB1    5: SCM_GPO1
	 * GPIOB0    5: SCM_GPO0
	 */
	writel(0x55555555, (void *)SCU1_PINMUX_GRP_B);

	/*
	 * Pin       MF
	 * --------------------------
	 * GPIOC7    5: SCM_GPI3
	 * GPIOC6    5: SCM_GPI2
	 * GPIOC5    5: SCM_GPI1
	 * GPIOC4    5: SCM_GPI0
	 * GPIOC3    5: SCM_GPO11
	 * GPIOC2    5: SCM_GPO10
	 * GPIOC1    5: SCM_GPO9
	 * GPIOC0    5: SCM_GPO8
	 */
	writel(0x55555555, (void *)SCU1_PINMUX_GRP_C);

	/*
	 * Pin       MF
	 * --------------------------
	 * GPIOD7    5: HPM0_GPO3
	 * GPIOD6    5: HPM0_GPO2
	 * GPIOD5    5: HPM0_GPO1
	 * GPIOD4    5: HPM0_GPO0
	 * GPIOD3    5: HPM0_GPI3
	 * GPIOD2    5: HPM0_GPI2
	 * GPIOD1    5: HPM0_GPI1
	 * GPIOD0    5: HPM0_GPI0
	 */
	writel(0x55555555, (void *)SCU1_PINMUX_GRP_D);

	/*
	 * Pin         MF
	 * ----------------------------
	 * GPIOP6    5: HPM1_GPO3
	 *
	 * GPIOF4    5: HPM1_GPO2
	 * GPIOF1    5: HPM1_GPO1
	 * GPIOF0    5: HPM1_GPO0
	 */
	clrsetbits_le32((void *)SCU1_PINMUX_GRP_P, SCU1_PINMUX_PIN6,
			FIELD_PREP(SCU1_PINMUX_PIN6, 0x5));

	clrsetbits_le32((void *)SCU1_PINMUX_GRP_F,
			SCU1_PINMUX_PIN4 | SCU1_PINMUX_PIN1 | SCU1_PINMUX_PIN0,
			FIELD_PREP(SCU1_PINMUX_PIN4, 0x5) |
			FIELD_PREP(SCU1_PINMUX_PIN1, 0x5) |
			FIELD_PREP(SCU1_PINMUX_PIN0, 0x5));

	/*
	 * Pin       MF
	 * --------------------------
	 * GPIOH3    5: HPM1_GPI3
	 * GPIOH2    5: HPM1_GPI2
	 *
	 * GPIOAA7   5: HPM1_GPI1
	 * GPIOAA6   5: HPM1_GPI0
	 */
	clrsetbits_le32((void *)SCU1_PINMUX_GRP_H, SCU1_PINMUX_PIN3 | SCU1_PINMUX_PIN2,
			FIELD_PREP(SCU1_PINMUX_PIN3, 0x5) |
			FIELD_PREP(SCU1_PINMUX_PIN2, 0x5));

	clrsetbits_le32((void *)SCU1_PINMUX_GRP_AA, SCU1_PINMUX_PIN7 | SCU1_PINMUX_PIN6,
			FIELD_PREP(SCU1_PINMUX_PIN7, 0x5) |
			FIELD_PREP(SCU1_PINMUX_PIN6, 0x5));

	/*
	 * Pin       MF
	 * --------------------------
	 * GPIOT6    5: SGPSCK
	 * GPIOT1    5: SGPSLD
	 *
	 * GPIOU3    5: SGPSMI
	 * GPIOU2    5: SGPSMO
	 */
	clrsetbits_le32((void *)SCU1_PINMUX_GRP_T, SCU1_PINMUX_PIN6 | SCU1_PINMUX_PIN1,
			FIELD_PREP(SCU1_PINMUX_PIN6, 0x5) |
			FIELD_PREP(SCU1_PINMUX_PIN1, 0x5));

	clrsetbits_le32((void *)SCU1_PINMUX_GRP_U, SCU1_PINMUX_PIN3 | SCU1_PINMUX_PIN2,
			FIELD_PREP(SCU1_PINMUX_PIN3, 0x5) |
			FIELD_PREP(SCU1_PINMUX_PIN2, 0x5));

	/* Enable SGPIO slave */
	ltpi_scm_sgpios_init();
}

struct bootstage_t ltpi_init(struct rom_context *rom_ctx)
{
	struct bootstage_t sts = { 0, 0 };

	struct ltpi_priv *ltpi0 = &ltpi_data[0];
	struct ltpi_priv *ltpi1 = &ltpi_data[1];
	uint32_t pin_strap = readl(SCU1_HWSTRAP1);
	uint32_t otpcfg_31_30, otpcfg_23_22, otpcfg_03_02;

	otpcfg_03_02 = readl(SCU1_OTPCFG_03_02);
	if (!(otpcfg_03_02 & OTPCFG2_DIS_RECOVERY_MODE) &&
	    (pin_strap & SCU1_HWSTRAP1_EN_RECOVERY_BOOT)) {
		sts.errno |= LTPI_STATUS_EXIT;
		sts.syndrome = LTPI_SYND_SOC_RECOVERY;
		return sts;
	}

	ltpi0->index = 0;
	ltpi1->index = 1;
	ltpi0->bootstage = &sts;
	ltpi1->bootstage = &sts;

	ltpi0->base = LTPI_REG;
	ltpi1->base = LTPI_REG + 0x1000;

	ltpi0->phy_base = ltpi0->base + 0x200;
	ltpi0->top_base = ltpi0->base + 0x800;
	ltpi0->gpio_base = ltpi0->base + 0xc00;
	ltpi0->reset.regs_assert = SCU1_RSTCTL2;
	ltpi0->reset.regs_deassert = SCU1_RSTCTL2_CLR;
	ltpi0->reset.bit_mask = SCU1_RSTCTL2_LTPI0;
	ltpi0->clk_ctrl.regs_gate = SCU1_CLKGATE2;
	ltpi0->clk_ctrl.regs_ungate = SCU1_CLKGATE2_CLR;
	ltpi0->clk_ctrl.bit_mask = SCU1_CLKGATE2_LTPI0_TX;

	ltpi1->phy_base = ltpi1->base + 0x200;
	ltpi1->top_base = ltpi1->base + 0x800;
	ltpi1->gpio_base = ltpi1->base + 0xc00;
	ltpi1->reset.regs_assert = SCU1_RSTCTL2;
	ltpi1->reset.regs_deassert = SCU1_RSTCTL2_CLR;
	ltpi1->reset.bit_mask = SCU1_RSTCTL2_LTPI1;
	ltpi1->clk_ctrl.regs_gate = SCU1_CLKGATE2;
	ltpi1->clk_ctrl.regs_ungate = SCU1_CLKGATE2_CLR;
	ltpi1->clk_ctrl.bit_mask = SCU1_CLKGATE2_LTPI1_TX;

	/*
	 * Enable the LTPI AHB clock.
	 * This clock should always be on and can be safely enabled regardless
	 * of its current status.
	 */
	writel(SCU1_CLKGATE2_LTPI_AHB, (void *)SCU1_CLKGATE2_CLR);

	/* Parse configurations from the OTP */
	otpcfg_31_30 = readl(SCU1_OTPCFG_31_30);
	otpcfg_23_22 = readl(SCU1_OTPCFG_23_22);

	//unsupported_speed = FIELD_GET(SCU1_OTPCFG30_LTPI0_SPEED_CAPA_DIS, otpcfg_31_30);
	//ltpi0->otp_speed_cap = LTPI_SP_CAP_ASPEED_SUPPORTED & ~unsupported_speed;
	//ltpi0->otp_ddr_dis = !!(otpcfg_31_30 & SCU1_OTPCFG30_LTPI0_DDR_DIS);
	ltpi0->io_driving = FIELD_GET(SCU1_HWSTRAP1_LTPI0_IO_DRIVING, pin_strap);

	//unsupported_speed = FIELD_GET(SCU1_OTPCFG31_LTPI1_SPEED_CAPA_DIS, otpcfg_31_30);
	//ltpi1->otp_speed_cap = LTPI_SP_CAP_ASPEED_SUPPORTED & ~unsupported_speed;
	//ltpi1->otp_ddr_dis = !!(otpcfg_31_30 & SCU1_OTPCFG31_LTPI1_DDR_DIS);
	ltpi1->io_driving = FIELD_GET(SCU1_OTPCFG23_LTPI1_IO_DRIVING, otpcfg_23_22);

	ltpi0->crc_format = !!(otpcfg_23_22 & SCU1_OTPCFG23_LTPI_CRC_FORMAT);
	ltpi1->crc_format = ltpi0->crc_format;

	//ltpi0->phy_speed_cap = ltpi0->otp_speed_cap;
	//ltpi1->phy_speed_cap = ltpi1->otp_speed_cap;
	ltpi0->phy_speed_cap = 0x1;
	ltpi1->phy_speed_cap = 0x1;

	ltpi0->clk_inverse = FIELD_GET(SCU1_OTPCFG23_LTPI0_PHYCLK_INV, otpcfg_23_22);
	ltpi1->clk_inverse = FIELD_GET(SCU1_OTPCFG23_LTPI1_PHYCLK_INV, otpcfg_23_22);

	/*
	 * LTPI_MODE    LTPI0_EN    LTPI1_EN    Description
	 * --------------------------------------------------------------------
	 *         1           x           x    SOC is AST1700, HPM mode
	 *         0           0           0    SOC is AST2700, not in SCM mode, skip
	 *         0           0           1    SOC is AST2700, illegal case, skip
	 *         0           1           0    SOC is AST2700, SCM mode + single node
	 *         0           1           1    SOC is AST2700, SCM mode + dual node
	 */
	if (pin_strap & SCU1_HWSTRAP1_LTPI0_EN) {
		ltpi_scm_set_pins();
		ltpi_scm_init(ltpi0);

		if (pin_strap & SCU1_HWSTRAP1_LTPI1_EN) {
			bootstage_epilogue(*ltpi0->bootstage);
			bootstage_prologue(BOOTSTAGE_LTPI_INIT);
			ltpi1->bootstage->errno = LTPI_STATUS_IDX;
			ltpi1->bootstage->syndrome = LTPI_SYND_OK;
			ltpi_scm_init(ltpi1);
		}
	}

	return sts;
}

static int ltpi_get_link_partner(struct ltpi_priv *ltpi)
{
	uint32_t reg = readl((void *)ltpi->base + LTPI_LINK_MNG_ST);

	return FIELD_GET(REG_LTPI_LINK_PARTNER_FLAG, reg);
}

static void ltpi_show_status(struct ltpi_priv *ltpi)
{
	int speed, ret, i;
	uint32_t phy_mode, clk_select;
	char modes[9][8] = { "OFF", "SDR", "DDR", "NA",	   "CDR_LO",
			     "NA",  "NA",  "NA",  "CDR_HI" };

	ret = ltpi_get_link_mng_state(ltpi);
	if (ret != LTPI_LINK_MNG_ST_OP) {
		printf("LTPI%d: Not linked\n", ltpi->index);
		return;
	}

	phy_mode = FIELD_GET(REG_LTPI_PHY_MODE,
			     readl((void *)ltpi->phy_base + LTPI_PHY_CTRL));
	clk_select = FIELD_GET(REG_LTPI_PLL_SELECT,
			       readl((void *)ltpi->phy_base + LTPI_PLL_CTRL));
	if (phy_mode == LTPI_PHY_MODE_SDR) {
		for (i = 0; i < 13; i++)
			if (clk_select == ltpi_clk_lookup_sdr[i].clk_sel)
				speed = ltpi_clk_lookup_sdr[i].freq;
	} else {
		for (i = 0; i < 13; i++)
			if (clk_select == ltpi_clk_lookup_ddr[i].clk_sel)
				speed = ltpi_clk_lookup_ddr[i].freq;
	}

	printf("LTPI%d:\n"
	       "    link partner    : %s\n"
	       "    link mode       : %s\n"
	       "    link bandwidth  : %dMbps\n",
	       ltpi->index, ltpi_get_link_partner(ltpi) ? "ast1700" : "fpga",
	       &modes[phy_mode][0], speed);
}

static int do_ltpi(struct cmd_tbl *cmdtp, int flag, int argc,
		   char *const argv[])
{
	struct rom_context rc;
	struct getopt_state gs;
	uint32_t pin_strap;
	int opt, speed = 0, mode = 0;
	char *endp;

	ltpi_data[0].ad_timeout = ADVERTISE_TIMEOUT_US;
	ltpi_data[1].ad_timeout = ADVERTISE_TIMEOUT_US;
	ltpi_data[0].clk_inverse = 0x0;
	ltpi_data[1].clk_inverse = 0x0;

	getopt_init_state(&gs);
	while ((opt = getopt(&gs, argc, argv, "l:m:p:i:t:sh")) > 0) {
		switch (opt) {
		case 'l':
			speed = simple_strtoul(gs.arg, &endp, 16);
			break;
		case 'm':
			mode = simple_strtoul(gs.arg, &endp, 0);
			break;
		case 'i':
			ltpi_data[0].clk_inverse = simple_strtoul(gs.arg, &endp, 0);
			ltpi_data[1].clk_inverse = ltpi_data[0].clk_inverse;
			break;
		case 't':
			ltpi_data[0].ad_timeout = simple_strtoul(gs.arg, &endp, 0);
			ltpi_data[1].ad_timeout = ltpi_data[0].ad_timeout;
			break;
		case 's':
			ltpi_show_status(&ltpi_data[0]);
			ltpi_show_status(&ltpi_data[1]);
			return CMD_RET_SUCCESS;
		case 'h':
			fallthrough;
		default:
			return CMD_RET_USAGE;
		}
	}

	/* Set pin strap according to the command argument */
	writel(0xf, (void *)SCU1_HWSTRAP1 + 0x4);
	setbits_le32((void *)SCU1_HWSTRAP1, SCU1_HWSTRAP1_LTPI0_EN);
	if (mode)
		setbits_le32((void *)SCU1_HWSTRAP1, SCU1_HWSTRAP1_LTPI1_EN);

	pin_strap = readl((void *)SCU1_HWSTRAP1);

	/* Set otp strap according to the command argument */
	ltpi_data[0].otp_speed_cap = speed;
	ltpi_data[0].otp_ddr_dis = !!(speed & LTPI_SP_CAP_DDR);
	ltpi_data[1].otp_speed_cap = speed;
	ltpi_data[1].otp_ddr_dis = !!(speed & LTPI_SP_CAP_DDR);

	ltpi_init(&rc);

	if (pin_strap & SCU1_HWSTRAP1_LTPI0_EN) {
		uint32_t reg;

		if (ltpi_get_link_partner(&ltpi_data[0]))
			reg = FIELD_PREP(REG_LTPI_AHB_ADDR_MAP0, 0x5) |
			      FIELD_PREP(REG_LTPI_AHB_ADDR_MAP1, 0xa0);
		else
			reg = 0;

		writel(reg, (void *)ltpi_data[0].base + LTPI_AHB_CTRL0);

		if (pin_strap & SCU1_HWSTRAP1_LTPI1_EN) {
			if (ltpi_get_link_partner(&ltpi_data[1]))
				reg = FIELD_PREP(REG_LTPI_AHB_ADDR_MAP0, 0x5) |
				      FIELD_PREP(REG_LTPI_AHB_ADDR_MAP1, 0xa0);
			else
				reg = 0;

			writel(reg, (void *)ltpi_data[1].base + LTPI_AHB_CTRL0);
		}

		ltpi_show_status(&ltpi_data[0]);
		ltpi_show_status(&ltpi_data[1]);
	}

	return CMD_RET_SUCCESS;
}

static char ltpi_help_text[] = {
	"-m <LTPI mode>, 0=enable ltpi0, 1=enable ltpi0 & ltpi1\n"
	"-l <speed mask>, set bitmask to disable the corresponding speeds\n"
	"    [15] DDR [14] N/A [13] N/A [12] 500M [11] N/A [10] N/A [9] 600M [8] 400M\n"
	"    [7] 300M [6] 250M [5] 200M [4] 150M  [3] 100M [2] 75M  [1] 50M  [0] 25M\n"
	"-p <port enabling>, 0=ltpi0, 1=ltpi0+ltpi1\n"
	"-i <clk inverse>, 0x0=no inverse, 0x1=inverse tx, 0x2=inverse rx, 0x3=inverse both\n"
	"-t <advertise timeout in us>, default 100000\n"
	"-s, Display current link status\n"
};

U_BOOT_CMD(ltpi, 11, 0, do_ltpi, "ASPEED LTPI commands", ltpi_help_text);
