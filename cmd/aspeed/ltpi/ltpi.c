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
#include "ltpi.h"
#include "ltpi_top.h"
#include "ltpi_ctrl.h"
#include "internal.h"

#define ADVERTISE_TIMEOUT_US			3000 /* 2.696ms */
//#define CONFIG_AST1700	1

/* Use AST2700-EVB to emulate AST1700-EVB */
//#define CONFIG_AS1700_EMU	1

#define SCU_IO_REG		0x14c02000
#define   SCU_IO_LPLL_1		0x340
#define     LPLL_1_RESET	BIT(25)
#define     LPLL_1_BYPASS	BIT(24)
#define     LPLL_1_DIS		BIT(23)
#define     LPLL_1_P		GENMASK(22, 19)
#define     LPLL_1_N		GENMASK(18, 13)
#define     LPLL_1_M		GENMASK(12, 0)
#define   SCU_IO_LPLL_2		0x344
#define     LPLL_2_LOCK		BIT(31)
#define     LPLL_2_BWADJ	GENMASK(11, 0)

#define GPIO_IO_REG		0x14c0b000
#define LTPI_REG		0x14c34000
#define SGPIOS_REG		0x14c3c000
#define SGPIO1_REG		0x14c0d000

/* I_SCU010 */
#define SCU_I_LTPI_MODE		BIT(0)
#define SCU_I_LTPI_NUM		BIT(1)
#define SCU_I_LTPI_IDX		BIT(2)
#define SCU_I_SCM_MODE		BIT(3)

/* I_SCU030 */
#define SCU_I_LTPI_MAX		GENMASK(21, 19)
#define SCU_I_LTPI_IO_TYPE	BIT(22)

#define GPIO_PORT_AA		26
/* number of pins in a port */
#define N_PINS			8
/* size of a pin register in bytes */
#define PIN_REG_SZ		4
/* size of the global control registers in bytes */
#define GPIO_GLOBAL_SZ		0x180

/*
 * This macro returns the address offset of a specific pin. The actual address
 * of the pin control register is "gpio_reg_base + offset".
 *
 * For example, assuming gpio_reg_base is 0x14c0b000, the actual register
 * address of GPIOA5 would be (0x14c0b000 + GPIO(A, 5)).
 */
#define GPIO(port, pin_index)	\
	(GPIO_GLOBAL_SZ + (PIN_REG_SZ * ((GPIO_PORT_##port * N_PINS) + (pin_index))))

/* bitfield of the pin register */
#define GPIO_DATA_IN		BIT(13)	/* data in */
#define GPIO_BLINK_2		BIT(11)
#define GPIO_BLINK_1		BIT(10)
#define GPIO_IN_MASK		BIT(9)
#define GPIO_DEB2		BIT(8)
#define GPIO_DEB1		BIT(7)
#define GPIO_RST_TOL		BIT(6)
#define GPIO_INT_T2		BIT(5)
#define GPIO_INT_T1		BIT(4)
#define GPIO_INT_T0		BIT(3)
#define GPIO_INT_EN		BIT(2)
#define GPIO_DIRECT		BIT(1)
#define   GPIO_DIRECT_IN	0b0
#define   GPIO_DIRECT_OUT	0b1
#define GPIO_DATA		BIT(0)	/* data out */

struct ltpi_reset {
	uintptr_t regs_assert;
	uintptr_t regs_deassert;
	uint32_t bit_mask;
};

struct ltpi_clk_info {
	int16_t freq;		/* clock frequency in MHz*/
	int16_t clk_sel;	/* clock selection */
	int16_t bandwidth;	/* data bandwidth in Mbits/s */
};

struct ltpi_common {
	uintptr_t top_base;

	/* for master mode only.  0: unknown 1:normal mode, 2:cdr mode */
	int bus_topology;

	/* limit the speed via OTP strap */
	uint16_t otp_speed_cap;

#define CDR_MASK_LTPI1		BIT(1)
#define CDR_MASK_LTPI0		BIT(0)
#define CDR_MASK_ALL		(CDR_MASK_LTPI1 | CDR_MASK_LTPI0)
#define CDR_MASK_NONE		0
	int cdr_mask;

#define SOC_REV_AST2700A0	0x0
#define SOC_REV_AST2700A1	0x1
	int soc_rev;

	const struct ltpi_clk_info *sdr_lookup;
	const struct ltpi_clk_info *ddr_lookup;
	const struct ltpi_clk_info *cdr_lookup;
	const struct ltpi_clk_info *cdr2x_lookup;
};

struct ltpi_priv {
	uintptr_t base;
	uintptr_t phy_base;

	struct ltpi_reset reset;
	struct ltpi_common *common;

	uint16_t phy_speed_cap; /* limit the speed with physical line status */
	uint16_t phy_mode_attempt; /* the current phy_mode attempts to link */

#define RX_CLK_INVERSE		BIT(1)
#define TX_CLK_INVERSE		BIT(0)
	int clk_inverse;
	int index;
};

static struct ltpi_common ltpi_common_data;
static struct ltpi_priv ltpi_data[2];

/* constant lookup tables */
static const struct ltpi_clk_info ltpi_clk_lookup_sdr_a0[13] = {
	{ 25, REG_LTPI_PLL_25M, 25 },
	{ 50, REG_LTPI_PLL_50M, 50 },
	{ -1, -1, -1 },
	{ 100, REG_LTPI_PLL_100M, 100 },
	{ -1, -1, -1 },
	{ 200, REG_LTPI_PLL_200M, 200 },
	{ 250, REG_LTPI_PLL_250M, 250 },
	{ -1, -1, -1 },
	{ 400, REG_LTPI_PLL_400M, 400 },
	{ -1, -1, -1 },
	{ 800, REG_LTPI_PLL_800M, 800 },
	{ 1000, REG_LTPI_PLL_1G, 1000 },
	{ -1, -1, -1 }
};

static const struct ltpi_clk_info ltpi_clk_lookup_cdr_a0[13] = {
	{ 25, REG_LTPI_PLL_100M, 25 },
	{ 50, REG_LTPI_PLL_200M, 50 },
	{ -1, -1, -1 },
	{ 100, REG_LTPI_PLL_400M, 100 },
	{ -1, -1, -1 },
	{ 200, REG_LTPI_PLL_800M, 200 },
	{ 250, REG_LTPI_PLL_250M, 250 },
	{ -1, -1, -1 },
	{ 400, REG_LTPI_PLL_400M, 400 },
	{ -1, -1, -1 },
	{ 800, REG_LTPI_PLL_800M, 800 },
	{ 1000, REG_LTPI_PLL_1G, 1000 },
	{ -1, -1, -1 }
};

static const int16_t otp_to_speed_mask_lookup[8] = {
	LTPI_SP_CAP_MASK_1G & LTPI_SP_CAP_ASPEED_SUPPORTED,
	LTPI_SP_CAP_MASK_800M & LTPI_SP_CAP_ASPEED_SUPPORTED,
	LTPI_SP_CAP_MASK_400M & LTPI_SP_CAP_ASPEED_SUPPORTED,
	LTPI_SP_CAP_MASK_250M & LTPI_SP_CAP_ASPEED_SUPPORTED,
	LTPI_SP_CAP_MASK_200M & LTPI_SP_CAP_ASPEED_SUPPORTED,
	LTPI_SP_CAP_MASK_100M & LTPI_SP_CAP_ASPEED_SUPPORTED,
	LTPI_SP_CAP_MASK_50M & LTPI_SP_CAP_ASPEED_SUPPORTED,
	LTPI_SP_CAP_MASK_25M & LTPI_SP_CAP_ASPEED_SUPPORTED
};

/* A1 */
static const struct ltpi_clk_info ltpi_clk_lookup_sdr[13] = {
	{ 25, REG_LTPI_PLL_25M, 25 },
	{ 50, REG_LTPI_PLL_LPLL, 50 },
	{ 75, REG_LTPI_PLL_LPLL, 75 },
	{ 100, REG_LTPI_PLL_HPLL_DIV_10, 100 },
	{ 150, REG_LTPI_PLL_LPLL, 150 },
	{ 200, REG_LTPI_PLL_HCLK, 200 },
	{ 250, REG_LTPI_PLL_HPLL_DIV_4, 250 },
	{ 300, REG_LTPI_PLL_LPLL_DIV_4, 300 },
	{ 400, REG_LTPI_PLL_LPLL, 400 },
	{ 600, REG_LTPI_PLL_LPLL_DIV_2, 600 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
	{ 500, REG_LTPI_PLL_HPLL_DIV_2, 500 }
};

static const struct ltpi_clk_info ltpi_clk_lookup_ddr[13] = {
	{ 50, REG_LTPI_PLL_LPLL, 50 },
	{ 100, REG_LTPI_PLL_HPLL_DIV_10, 100 },
	{ 150, REG_LTPI_PLL_LPLL, 150 },
	{ 200, REG_LTPI_PLL_HCLK, 200 },
	{ 300, REG_LTPI_PLL_LPLL_DIV_4, 300 },
	{ 400, REG_LTPI_PLL_LPLL, 400 },
	{ 500, REG_LTPI_PLL_HPLL_DIV_2, 500 },
	{ 600, REG_LTPI_PLL_LPLL_DIV_2, 600 },
	{ 800, REG_LTPI_PLL_LPLL, 800 },
	{ 1200, REG_LTPI_PLL_LPLL, 1200 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
	{ 1000, REG_LTPI_PLL_LPLL, 1000 }
};

static const struct ltpi_clk_info ltpi_clk_lookup_cdr[13] = {
	{ 100, REG_LTPI_PLL_HPLL_DIV_10, 25 },
	{ -1, -1, -1 },
	{ 300, REG_LTPI_PLL_LPLL_DIV_4, 75 },
	{ -1, -1, -1 },
	{ 600, REG_LTPI_PLL_LPLL_DIV_2, 150 },
	{ -1, -1, -1 },
	{ 250, REG_LTPI_PLL_HPLL_DIV_4, 250 },
	{ 300, REG_LTPI_PLL_LPLL_DIV_4, 300 },
	{ -1, -1, -1 },
	{ 600, REG_LTPI_PLL_LPLL_DIV_2, 600 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
	{ -1, -1, -1 }
};

static const struct ltpi_clk_info ltpi_clk_lookup_cdr2x[13] = {
	{ 200, REG_LTPI_PLL_HCLK, 50 },
	{ -1, -1, -1 },
	{ 600, REG_LTPI_PLL_LPLL_DIV_2, 150 },
	{ -1, -1, -1 },
	{ 300, REG_LTPI_PLL_LPLL_DIV_4, 300 },
	{ -1, -1, -1 },
	{ 500, REG_LTPI_PLL_HPLL_DIV_2, 500 },
	{ 600, REG_LTPI_PLL_LPLL_DIV_2, 600 },
	{ -1, -1, -1 },
	{ 1200, REG_LTPI_PLL_LPLL, 1200 },
	{ -1, -1, -1 },
	{ -1, -1, -1 },
	{ -1, -1, -1 }
};

static uint32_t get_pin_strap(void);

static void bootstage_end_status(uint8_t status)
{
	printf(" %02x\n", status);
}

static void bootstage_start_mark(char *str)
{
	printf("%s", str);
}

#define SCU_IO_REG		0x14c02000
#define   SCU_IO_LPLL_1		0x340
#define     LPLL_1_RESET	BIT(25)
#define     LPLL_1_BYPASS	BIT(24)
#define     LPLL_1_DIS		BIT(23)
#define     LPLL_1_P		GENMASK(22, 19)
#define     LPLL_1_N		GENMASK(18, 13)
#define     LPLL_1_M		GENMASK(12, 0)
#define   SCU_IO_LPLL_2		0x344
#define     LPLL_2_LOCK		BIT(31)
#define     LPLL_2_BWADJ	GENMASK(11, 0)

#define PLL_ID_LPLL		0
static int scu_get_pll_freq(int pll_id)
{
	uint32_t reg;
	int m, n, p;

	if (pll_id != PLL_ID_LPLL)
		return 0;

	reg = readl((void *)SCU_IO_REG + SCU_IO_LPLL_1);
	m = FIELD_GET(LPLL_1_M, reg);
	n = FIELD_GET(LPLL_1_N, reg);
	p = FIELD_GET(LPLL_1_P, reg);

	return (25000000 * (m + 1) / (n + 1) / (p + 1));
}

static int scu_set_pll_freq(int pll_id, int freq)
{
	int curr_freq = scu_get_pll_freq(pll_id);
	int m, n, p, bwadj;
	uint32_t reg;

	if (curr_freq == freq)
		return 0;

	switch (freq) {
	case 50:
		m = 32;
		n = 1;
		p = 16;
		bwadj = 16;
		break;
	case 75:
		m = 48;
		n = 1;
		p = 16;
		bwadj = 24;
		break;
	case 150:
		m = 60;
		n = 1;
		p = 10;
		bwadj = 30;
		break;
	case 400:
		m = 32;
		n = 1;
		p = 2;
		bwadj = 16;
		break;
	case 800:
		m = 32;
		n = 1;
		p = 1;
		bwadj = 16;
		break;
	case 1000:
		m = 40;
		n = 1;
		p = 1;
		bwadj = 20;
		break;
	case 1200:
	default:
		m = 48;
		n = 1;
		p = 1;
		bwadj = 24;
		break;
	}

	reg = readl((void *)SCU_IO_REG + SCU_IO_LPLL_1);
	reg &= ~(LPLL_1_M | LPLL_1_N | LPLL_1_P);
	reg |= FIELD_PREP(LPLL_1_M, m - 1) | FIELD_PREP(LPLL_1_N, n - 1) |
	       FIELD_PREP(LPLL_1_P, p - 1);
	writel(reg, (void *)SCU_IO_REG + SCU_IO_LPLL_1);

	reg = readl((void *)SCU_IO_REG + SCU_IO_LPLL_2);
	reg &= ~LPLL_2_BWADJ;
	reg |= FIELD_PREP(LPLL_2_BWADJ, bwadj - 1);
	writel(reg, (void *)SCU_IO_REG + SCU_IO_LPLL_2);

	return 0;
}

/**
 * @brief Count the number of leading zeros of a uint16_t
 * @param [IN] x - the uint16_t to be counted
 * @return the number of leading zeros in x
 */
static int clz16(uint16_t x)
{
	int n = 0;

	if (x == 0)
		return sizeof(uint16_t) * 8;

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

static int ltpi_phy_set_mode(struct ltpi_priv *ltpi, int mode)
{
	uint32_t reg;

	if (mode < 0 || mode > LTPI_PHY_MODE_CDR_HI_SP) {
		debug("%s: invalid mode %d\n", __func__, mode);
		return -1;
	}

	reg = readl((void *)ltpi->phy_base + LTPI_PHY_CTRL);
	reg &= ~REG_LTPI_PHY_MODE;
	reg |= mode;
	writel(reg, ltpi->phy_base + LTPI_PHY_CTRL);

	return 0;
}

static int ltpi_phy_set_pll(struct ltpi_priv *ltpi, const struct ltpi_clk_info *info, int set)
{
	uint32_t reg;

	if (info->clk_sel < REG_LTPI_PLL_25M || info->clk_sel > REG_LTPI_PLL_1G) {
		debug("%s: invalid freq %d\n", __func__, info->clk_sel);
		return -1;
	}

	if (ltpi->common->soc_rev == SOC_REV_AST2700A1) {
		if (info->clk_sel == REG_LTPI_PLL_LPLL)
			scu_set_pll_freq(PLL_ID_LPLL, info->freq);
		else if (info->clk_sel == REG_LTPI_PLL_LPLL_DIV_4 ||
			 info->clk_sel == REG_LTPI_PLL_LPLL_DIV_2)
			scu_set_pll_freq(PLL_ID_LPLL, 1200);
	}

	reg = readl((void *)ltpi->phy_base + LTPI_PLL_CTRL);
	reg &= ~(REG_LTPI_PLL_SELECT | REG_LTPI_PLL_SET |
		 REG_LTPI_RX_PHY_CLK_INV | REG_LTPI_TX_PHY_CLK_INV);
	reg |= FIELD_PREP(REG_LTPI_PLL_SELECT, info->clk_sel);

	if (set)
		reg |= REG_LTPI_PLL_SET;

	if (ltpi->clk_inverse & TX_CLK_INVERSE)
		reg |= REG_LTPI_TX_PHY_CLK_INV;

	if (ltpi->clk_inverse & RX_CLK_INVERSE)
		reg |= REG_LTPI_RX_PHY_CLK_INV;

	writel(reg, ltpi->phy_base + LTPI_PLL_CTRL);

	return 0;
}

static int ltpi_reset(struct ltpi_priv *ltpi)
{
	writel(ltpi->reset.bit_mask, ltpi->reset.regs_assert);
	readl(ltpi->reset.regs_assert);

	udelay(1);

	writel(ltpi->reset.bit_mask, ltpi->reset.regs_deassert);
	readl(ltpi->reset.regs_deassert);

	if (IS_ENABLED(CONFIG_AST1700)) {
		writel(REG_LTPI_SW_FORCE_1700_EN,
		       ltpi->common->top_base + LTPI_SW_FORCE_EN);
		writel(REG_LTPI_SW_FORCE_1700_EN,
		       ltpi->common->top_base + LTPI_SW_FORCE_VAL);
	}

	return 0;
}

/**
 * @brief Set LTPI_LINK_ALIGNED (GPIOAA7) pin
 * @param [IN] value - 0: not aligned, others: aligned
 */
static void ltpi_set_link_aligned_pin(int value)
{
	uintptr_t addr = GPIO_IO_REG + GPIO(AA, 7);
	uint32_t reg = 0;

	reg |= FIELD_PREP(GPIO_DIRECT, GPIO_DIRECT_OUT);
	reg |= FIELD_PREP(GPIO_DATA, !!value);
	writel(reg, addr);
}

static void ltpi_hpm_init_sgpio(void)
{
	uint32_t value;
	uint32_t pin_strap = get_pin_strap();

	/*
	 * SGPIO master controller #1:
	 *
	 * Register 0x0
	 * - bit[31:16] clock divider
	 *     SGPIO clock = (PCLK 100M / 2) / (divider + 1)
	 *     select divider = 4 -> SGPIO clock = 50MHz / (4 + 1) = 10MHz
	 * - bit[11: 3] pin number
	 *     enable 112 parallel pins (BMC_GPIO 96 pins + PFR_GPIO 16 pins)
	 * - bit[0] SGPIO controller enabling
	 */
	if ((pin_strap & SCU_I_LTPI_NUM) && (pin_strap & SCU_I_LTPI_IDX)) {
		/*
		 * The 2nd AST1700 does't need to configure SGPIO since GPIO
		 * channel only exists in the 1st AST1700
		 */
		writel(0, SGPIO1_REG + 0x0);
	} else {
		value = FIELD_PREP(GENMASK(31, 16), 4) |
			FIELD_PREP(GENMASK(11, 3), 112) | BIT(0);
		writel(value, SGPIO1_REG + 0x0);
	}
}

static uint32_t ltpi_get_link_manage_state(struct ltpi_priv *ltpi)
{
	uint32_t reg;

	reg = readl((void *)ltpi->base + LTPI_LINK_MANAGE_ST);
	return FIELD_GET(REG_LTPI_LINK_MANAGE_ST, reg);
}

static int ltpi_poll_link_manage_state(struct ltpi_priv *ltpi, uint32_t expected,
				       uint32_t unexpected, int timeout_us)
{
	uint64_t start, timeout_tick;
	uintptr_t addr = ltpi->base + LTPI_LINK_MANAGE_ST;
	uint32_t reg = readl((void *)addr);
	uint32_t state;
	int ret = LTPI_OK;

	if (timeout_us) {
		start = get_ticks();
		timeout_tick = usec2ticks(timeout_us);
	}

	do {
		reg = readl((void *)addr);
		state = FIELD_GET(REG_LTPI_LINK_MANAGE_ST, reg);

		if (state == expected)
			break;

		if (state == unexpected) {
			/* link is disconnected, break the loop directly */
			ret = LTPI_ERR_REMOTE_DISCON;
			break;
		}

		if (timeout_us && ((get_ticks() - start) > timeout_tick)) {
			ret = LTPI_ERR_TIMEOUT;
			break;
		}
	} while (1);

	return ret;
}

static int ltpi_wait_state_pll_set(struct ltpi_priv *ltpi)
{
	return ltpi_poll_link_manage_state(ltpi, LTPI_LINK_MANAGE_ST_WAIT_PLL_SET, -1, 0);
}

static int ltpi_wait_state_op(struct ltpi_priv *ltpi)
{
	return ltpi_poll_link_manage_state(ltpi, LTPI_LINK_MANAGE_ST_OP,
					   LTPI_LINK_MANAGE_ST_DETECT_ALIGN, ADVERTISE_TIMEOUT_US);
}

static int ltpi_wait_state_link_speed(struct ltpi_priv *ltpi)
{
	return ltpi_poll_link_manage_state(ltpi, LTPI_LINK_MANAGE_ST_SPEED,
					   -1, ADVERTISE_TIMEOUT_US);
}

static int ltpi_get_link_partner(struct ltpi_priv *ltpi)
{
	uint32_t reg = readl((void *)ltpi->base + LTPI_LINK_MANAGE_ST);

	return FIELD_GET(REG_LTPI_LINK_PARTNER_FLAG, reg);
}

static int ltpi_set_local_speed_cap(struct ltpi_priv *ltpi, uint32_t speed_cap)
{
	uint32_t reg;

	/* only set bits that Aspeed SOC supported */
	if (ltpi->common->soc_rev == SOC_REV_AST2700A0) {
		speed_cap &= LTPI_SP_CAP_ASPEED_SUPPORTED;
	} else {
		if (ltpi->phy_mode_attempt == LTPI_PHY_MODE_CDR_LO_SP)
			speed_cap &= LTPI_SP_CAP_ASPEED_SUPPORTED_CDR;
		else
			speed_cap &= LTPI_SP_CAP_ASPEED_SUPPORTED_NORMAL;

		/* TODO: Check pinstrap DDR_Dis here */
		speed_cap |= LTPI_SP_CAP_DDR;
	}

	reg = readl((void *)ltpi->base + LTPI_CAP_LOCAL);
	reg &= ~REG_LTPI_SP_CAP_LOCAL;
	reg |= FIELD_PREP(REG_LTPI_SP_CAP_LOCAL, speed_cap);
	writel(reg, ltpi->base + LTPI_CAP_LOCAL);

	return 0;
}

static void ltpi_scm_set_sdr_mode(struct ltpi_priv *ltpi)
{
	ltpi_reset(ltpi);
	ltpi->phy_mode_attempt = LTPI_PHY_MODE_SDR;
	ltpi_set_local_speed_cap(ltpi, ltpi->phy_speed_cap);
	ltpi_phy_set_pll(ltpi, &ltpi->common->sdr_lookup[0], 0);

	/* To ensure the remote side is timed out */
	udelay(ADVERTISE_TIMEOUT_US);
	ltpi_phy_set_mode(ltpi, LTPI_PHY_MODE_SDR);
}

static void ltpi_scm_set_cdr_mode(struct ltpi_priv *ltpi)
{
	struct ltpi_common *common = ltpi->common;

	ltpi_reset(ltpi);
	ltpi->phy_mode_attempt = LTPI_PHY_MODE_CDR_LO_SP;
	ltpi_set_local_speed_cap(ltpi, ltpi->phy_speed_cap);

	/*
	 * For AST2700A0: LTPI0 and LTPI1 share the same SCU reset line and the LTPI-PHY,
	 * so LTPI1's local capabilities need to be re-initialized after ltpi_reset().
	 */
	if (common->soc_rev == SOC_REV_AST2700A0) {
		ltpi_set_local_speed_cap(&ltpi_data[1], ltpi->phy_speed_cap);
		writel(0x18, ltpi_data[1].base + LTPI_AD_CAP_LOW_LOCAL);
		writel(0x0, ltpi_data[1].base + LTPI_AD_CAP_HIGH_LOCAL);
	}

	if (ltpi->index) {
		/* Disable LTPI1 I2C/UART/GPIO passthrough */
		writel(0x18, ltpi->base + LTPI_AD_CAP_LOW_LOCAL);
		writel(0x0, ltpi->base + LTPI_AD_CAP_HIGH_LOCAL);
	}

	ltpi_phy_set_pll(ltpi, &ltpi->common->cdr_lookup[0], 0);

	/* To ensure the remote side is timed out */
	udelay(ADVERTISE_TIMEOUT_US);
	ltpi_phy_set_mode(ltpi, LTPI_PHY_MODE_CDR_LO_SP);
}

static int ltpi_set_normal_operation_clk(struct ltpi_priv *ltpi)
{
	int target_speed;

	/* find max attainable speed */
	target_speed = find_max_speed(ltpi->phy_speed_cap);

	/* set phy mode "OFF" */
	ltpi_phy_set_mode(ltpi, LTPI_PHY_MODE_OFF);

	if (ltpi->phy_speed_cap & LTPI_SP_CAP_DDR) {
		ltpi_phy_set_pll(ltpi, &ltpi->common->ddr_lookup[target_speed], 1);
		ltpi_phy_set_mode(ltpi, LTPI_PHY_MODE_DDR);
	} else {
		ltpi_phy_set_pll(ltpi, &ltpi->common->sdr_lookup[target_speed], 1);
		ltpi_phy_set_mode(ltpi, LTPI_PHY_MODE_SDR);
	}

	return target_speed;
}

static void ltpi_scm_normal_mode_training(struct ltpi_priv *ltpi)
{
	uint32_t reg, speed_cap;
	int target_speed, ret;

	do {
		ret = ltpi_wait_state_pll_set(ltpi);

		/* read intersection of the speed capabilities */
		reg = readl((void *)ltpi->base + LTPI_LINK_MANAGE_ST);
		speed_cap = FIELD_GET(REG_LTPI_SP_INTERSETION, reg);
		if (speed_cap == 0) {
			bootstage_start_mark(BOOTSTAGE_LTPI_SP_CAP);
			bootstage_end_status(BOOTSTAGE_LTPI_SP_CAP_E_NC |
					     BOOTSTAGE_LTPI_MODE_SDR);
			return;
		}
		ltpi->phy_speed_cap = speed_cap;

		target_speed = ltpi_set_normal_operation_clk(ltpi);

		/* poll link state 0x7 */
		ret = ltpi_wait_state_op(ltpi);
		if (ret == 0)
			break;

		bootstage_start_mark(BOOTSTAGE_LTPI_WAIT_OP);
		bootstage_end_status(ret | BOOTSTAGE_LTPI_MODE_SDR);

		/* clear the bit to specify the current speed doesn't work */
		ltpi->phy_speed_cap &= ~BIT(target_speed);

		/* the lowest speed 25M should always be supported */
		if (ltpi->phy_speed_cap == 0)
			ltpi->phy_speed_cap |= BIT(0);

		/* Restart the link speed detection */
		ltpi_scm_set_sdr_mode(ltpi);
		ret = ltpi_wait_state_link_speed(ltpi);
		if (ret) {
			printf("Timeout for the link-speed detection\n");
			return;
		}
	} while (1);
}

static int ltpi_set_cdr_operation_clk(struct ltpi_priv *ltpi)
{
	const struct ltpi_clk_info *info;
	uint32_t reg;
	int target_speed, phy_mode;
	bool dll2x = false;

	/* find max attainable speed */
	target_speed = find_max_speed(ltpi->phy_speed_cap);

	/* set phy mode "OFF" */
	ltpi_phy_set_mode(ltpi, LTPI_PHY_MODE_OFF);

	/* set PLL freq to the target speed */
	info = &ltpi->common->cdr_lookup[target_speed];
	if (ltpi->phy_speed_cap & LTPI_SP_CAP_DDR) {
		info = &ltpi->common->cdr2x_lookup[target_speed];
		if (info->bandwidth == 1200)
			dll2x = true;
	}

	phy_mode = LTPI_PHY_MODE_CDR_LO_SP;
	if (info->bandwidth > 200)
		phy_mode = LTPI_PHY_MODE_CDR_HI_SP;

	if (ltpi->common->soc_rev == SOC_REV_AST2700A1) {
		reg = readl((void *)ltpi->phy_base + LTPI_DLL_CTRL);
		reg &= ~REG_LTPI_DLL_CLK_2X;
		if (dll2x)
			reg |= REG_LTPI_DLL_CLK_2X;
		writel(reg, (void *)ltpi->phy_base + LTPI_DLL_CTRL);

		reg = readl((void *)ltpi->phy_base + LTPI_PLL_CTRL);
		reg &= ~REG_LTPI_RX_PLL_DIV2;
		if (dll2x)
			reg |= REG_LTPI_RX_PLL_DIV2;
		writel(reg, (void *)ltpi->phy_base + LTPI_PLL_CTRL);
	}

	ltpi_phy_set_pll(ltpi, info, 1);
	ltpi_phy_set_mode(ltpi, phy_mode);

	return target_speed;
}

static void ltpi_scm_cdr_mode_training(struct ltpi_priv *ltpi)
{
	uint32_t reg, speed_cap;
	int target_speed, ret;

	do {
		ret = ltpi_wait_state_pll_set(ltpi);

		/* read intersection of the speed capabilities */
		reg = readl((void *)ltpi->base + LTPI_LINK_MANAGE_ST);
		speed_cap = FIELD_GET(REG_LTPI_SP_INTERSETION, reg);
		ltpi->phy_speed_cap = speed_cap;

		if (ltpi->phy_speed_cap == 0) {
			bootstage_start_mark(BOOTSTAGE_LTPI_SP_CAP);
			bootstage_end_status(BOOTSTAGE_LTPI_SP_CAP_E_NC |
					     BOOTSTAGE_LTPI_MODE_CDR);
			return;
		}

		target_speed = ltpi_set_cdr_operation_clk(ltpi);

		/* poll link state 0x7 */
		ret = ltpi_wait_state_op(ltpi);
		if (ret == LTPI_OK)
			break;

		bootstage_start_mark(BOOTSTAGE_LTPI_WAIT_OP);
		bootstage_end_status(ret | BOOTSTAGE_LTPI_MODE_CDR);

		/* clear the bit to specify the current speed doesn't work */
		ltpi->phy_speed_cap &= ~BIT(target_speed);

		/* the lowest speed 25M should always be supported */
		if (ltpi->phy_speed_cap == 0)
			ltpi->phy_speed_cap |= BIT(0);

		ltpi_scm_set_cdr_mode(ltpi);
		ret = ltpi_wait_state_link_speed(ltpi);
		if (ret) {
			printf("Timeout for the link-speed detection\n");
			return;
		}
	} while (1);
}

static void ltpi_scm_init(struct ltpi_priv *ltpi0, struct ltpi_priv *ltpi1)
{
	struct ltpi_common *common = ltpi0->common;
	int ret;
	uint32_t reg, phy_mode, state0, state1;

	bootstage_start_mark(BOOTSTAGE_LTPI_INIT);
	/* start checking whether LTPI is initialized */
	reg = readl((void *)ltpi0->phy_base + LTPI_PHY_CTRL);
	phy_mode = FIELD_GET(REG_LTPI_PHY_MODE, reg);
	state0 = ltpi_get_link_manage_state(ltpi0);
	state1 = ltpi_get_link_manage_state(ltpi1);
	if (state0 == LTPI_LINK_MANAGE_ST_OP) {
		if (phy_mode == LTPI_PHY_MODE_SDR ||
		    phy_mode == LTPI_PHY_MODE_DDR) {
			bootstage_end_status(BOOTSTAGE_LTPI_INIT_SKIP |
					     BOOTSTAGE_LTPI_MODE_SDR);
			if (common->cdr_mask == CDR_MASK_LTPI1)
				printf("Warning: try to init LTPI1 in SDR/DDR mode\n");
			return;
		}

		if (phy_mode == LTPI_PHY_MODE_CDR_HI_SP ||
		    phy_mode == LTPI_PHY_MODE_CDR_LO_SP) {
			if (common->cdr_mask == CDR_MASK_LTPI0) {
				bootstage_end_status(BOOTSTAGE_LTPI_INIT_SKIP |
						     BOOTSTAGE_LTPI_MODE_CDR);
				return;
			}

			if (common->cdr_mask == CDR_MASK_ALL &&
			    state1 == LTPI_LINK_MANAGE_ST_OP) {
				bootstage_end_status(BOOTSTAGE_LTPI_INIT_SKIP |
						     BOOTSTAGE_LTPI_MODE_CDR);
				return;
			}
		}
	}

	state1 = ltpi_get_link_manage_state(ltpi1);
	if (state1 == LTPI_LINK_MANAGE_ST_OP &&
	    common->cdr_mask == CDR_MASK_LTPI1) {
		bootstage_end_status(BOOTSTAGE_LTPI_INIT_SKIP |
				     BOOTSTAGE_LTPI_MODE_CDR);
		return;
	}

	/*
	 * LTPI initialization is required. At this stage, the topology (SDR or
	 * CDR) is not known yet.
	 */
	bootstage_end_status(BOOTSTAGE_LTPI_INIT_REQUIRE |
			     BOOTSTAGE_LTPI_MODE_NONE);

	common->bus_topology = 0;
	while (1) {
		ltpi_scm_set_sdr_mode(ltpi0);
		ret = ltpi_wait_state_link_speed(ltpi0);
		if (ret == LTPI_OK) {
			common->bus_topology = 1;
			break;
		}

		if (common->cdr_mask == CDR_MASK_NONE) {
			continue;
		} else if (common->cdr_mask == CDR_MASK_LTPI1) {
			ltpi_scm_set_cdr_mode(ltpi1);
			ret = ltpi_wait_state_link_speed(ltpi1);
		} else {
			ltpi_scm_set_cdr_mode(ltpi0);
			ret = ltpi_wait_state_link_speed(ltpi0);

			if (common->cdr_mask & CDR_MASK_LTPI1)
				ret |= ltpi_wait_state_link_speed(ltpi1);
		}

		if (ret == LTPI_OK) {
			common->bus_topology = 2;
			break;
		}
	}

	debug("master bus topology: %d\n", common->bus_topology);

	if (common->bus_topology == 2)
		if (common->cdr_mask == CDR_MASK_LTPI1)
			ltpi_scm_cdr_mode_training(ltpi1);
		else
			ltpi_scm_cdr_mode_training(ltpi0);
	else
		ltpi_scm_normal_mode_training(ltpi0);
}

static int ltpi_hpm_init_normal_mode(struct ltpi_priv *ltpi)
{
	uint32_t reg, speed_cap;
	int target_speed, ret;

	ltpi_set_local_speed_cap(ltpi, ltpi->phy_speed_cap);
	ltpi_phy_set_mode(ltpi, LTPI_PHY_MODE_SDR);

	/* wait forever until PLL_SET_STATE */
	ret = ltpi_wait_state_pll_set(ltpi);

	reg = readl((void *)ltpi->base + LTPI_LINK_MANAGE_ST);
	speed_cap = FIELD_GET(REG_LTPI_SP_INTERSETION, reg);
	if (speed_cap == 0) {
		bootstage_start_mark(BOOTSTAGE_LTPI_SP_CAP);
		bootstage_end_status(BOOTSTAGE_LTPI_SP_CAP_E_NC |
				     BOOTSTAGE_LTPI_MODE_SDR);
		ret = LTPI_ERR_SEVERE;
		goto end;
	}
	ltpi->phy_speed_cap = speed_cap;

	target_speed = ltpi_set_normal_operation_clk(ltpi);

	/* poll link state 0x7 */
	ret = ltpi_wait_state_op(ltpi);
end:
	if (ret) {
		ltpi_phy_set_mode(ltpi, LTPI_PHY_MODE_OFF);
		ltpi_phy_set_pll(ltpi, &ltpi->common->sdr_lookup[0], 0);
		ltpi_reset(ltpi);
		bootstage_start_mark(BOOTSTAGE_LTPI_WAIT_OP);
		bootstage_end_status(ret | BOOTSTAGE_LTPI_MODE_SDR);
		return ret;
	}

	return LTPI_OK;
}

static void ltpi_hpm_normal_mode_loop(struct ltpi_priv *ltpi)
{
	int ret;
	uint32_t state = ltpi_get_link_manage_state(ltpi);

	bootstage_start_mark(BOOTSTAGE_LTPI_INIT);
	if (state == LTPI_LINK_MANAGE_ST_OP) {
		bootstage_end_status(BOOTSTAGE_LTPI_INIT_SKIP |
				     BOOTSTAGE_LTPI_MODE_SDR);
		return;
	}
	bootstage_end_status(BOOTSTAGE_LTPI_INIT_REQUIRE |
			     BOOTSTAGE_LTPI_MODE_SDR);

	ltpi_set_link_aligned_pin(0);

	do {
		ret = ltpi_hpm_init_normal_mode(ltpi);

		/* Severe error! do not retry */
		if (ret == LTPI_ERR_SEVERE)
			break;

	} while (ret != LTPI_OK);

	if (ret == LTPI_OK) {
		ltpi_hpm_init_sgpio();
		ltpi_set_link_aligned_pin(1);
	}
}

static int ltpi_hpm_init_cdr(struct ltpi_priv *ltpi)
{
	uint32_t reg, speed_cap;
	int target_speed, ret;

	ltpi_set_local_speed_cap(ltpi, ltpi->phy_speed_cap);
	ltpi_phy_set_pll(ltpi, &ltpi->common->cdr_lookup[0], 0);
	ltpi_phy_set_mode(ltpi, LTPI_PHY_MODE_CDR_LO_SP);

	ret = ltpi_wait_state_pll_set(ltpi);
	reg = readl((void *)ltpi->base + LTPI_LINK_MANAGE_ST);
	speed_cap = FIELD_GET(REG_LTPI_SP_INTERSETION, reg);
	ltpi->phy_speed_cap = speed_cap;

	if (speed_cap == 0) {
		bootstage_start_mark(BOOTSTAGE_LTPI_SP_CAP);
		bootstage_end_status(BOOTSTAGE_LTPI_SP_CAP_E_NC |
				     BOOTSTAGE_LTPI_MODE_CDR);
		ret = LTPI_ERR_SEVERE;
		goto cdr_end;
	}

	target_speed = ltpi_set_cdr_operation_clk(ltpi);

	/* poll link state 0x7 */
	ret = ltpi_wait_state_op(ltpi);
cdr_end:
	if (ret) {
		ltpi_phy_set_mode(ltpi, LTPI_PHY_MODE_OFF);
		ltpi_phy_set_pll(ltpi, &ltpi->common->cdr_lookup[0], 0);
		ltpi_reset(ltpi);
		bootstage_start_mark(BOOTSTAGE_LTPI_WAIT_OP);
		bootstage_end_status(ret | BOOTSTAGE_LTPI_MODE_CDR);
		return ret;
	}

	return LTPI_OK;
}

static void ltpi_hpm_cdr_loop(struct ltpi_priv *ltpi)
{
	int ret;
	uint32_t state = ltpi_get_link_manage_state(ltpi);

	bootstage_start_mark(BOOTSTAGE_LTPI_INIT);
	if (state == LTPI_LINK_MANAGE_ST_OP) {
		bootstage_end_status(BOOTSTAGE_LTPI_INIT_SKIP |
				     BOOTSTAGE_LTPI_MODE_CDR);
		return;
	}
	bootstage_end_status(BOOTSTAGE_LTPI_INIT_REQUIRE |
			     BOOTSTAGE_LTPI_MODE_CDR);

	ltpi_set_link_aligned_pin(0);

	do {
		ret = ltpi_hpm_init_cdr(ltpi);

		/* Severe error! do not retry */
		if (ret == LTPI_ERR_SEVERE)
			break;

	} while (ret != LTPI_OK);

	if (ret == LTPI_OK) {
		ltpi_hpm_init_sgpio();
		ltpi_set_link_aligned_pin(1);
	}
}

static uint32_t get_pin_strap(void)
{
	return readl((void *)SCU_IO_REG + 0x10);
}

static uint32_t get_otp_strap(void)
{
	return readl((void *)SCU_IO_REG + 0x30);
}

static void ltpi_scm_set_pins(void)
{
	uint32_t otp_strap = get_otp_strap();
	uint32_t reg;

	if (IS_ENABLED(CONFIG_AS1700_EMU)) {
		writel(0x00003333, SCU_IO_REG + 0x42c);
		return;
	}

	/* if LTPI_IO_TYPE == 1, FSI mode */
	if (otp_strap & SCU_I_LTPI_IO_TYPE) {
		/*
		 * 0x42c[15:12] = 0x3 -> LTPI_RXDAT
		 * 0x42c[11:8]  = 0x3 -> LTPI_TXDAT
		 * 0x42c[7:4]   = 0x3 -> LTPI_RXCLK
		 * 0x42c[3:0]   = 0x3 -> LTPI_TXCLK
		 */
		reg = readl((unsigned int)SCU_IO_REG + 0x42c);
		reg &= ~GENMASK(15, 0);
		reg |= 0x3333;
		writel(reg, SCU_IO_REG + 0x42c);
	}

	/*
	 * 0x404[31:28] = 0x6 -> SCM_GPO7
	 * 0x404[27:24] = 0x6 -> SCM_GPO6
	 * 0x404[23:20] = 0x6 -> SCM_GPO5
	 * 0x404[19:16] = 0x6 -> SCM_GPO4
	 * 0x404[15:12] = 0x6 -> SCM_GPO3
	 * 0x404[11:8]  = 0x6 -> SCM_GPO2
	 * 0x404[7:4]   = 0x6 -> SCM_GPO1
	 * 0x404[3:0]   = 0x6 -> SCM_GPO0
	 */
	writel(0x66666666, SCU_IO_REG + 0x404);

	/*
	 * 0x408[31:28] = 0x6 -> SCM_GPI3
	 * 0x408[27:24] = 0x6 -> SCM_GPI2
	 * 0x408[23:20] = 0x6 -> SCM_GPI1
	 * 0x408[19:16] = 0x6 -> SCM_GPI0
	 * 0x408[15:12] = 0x6 -> SCM_GPO11
	 * 0x408[11:8]  = 0x6 -> SCM_GPO10
	 * 0x408[7:4]   = 0x6 -> SCM_GPO9
	 * 0x408[3:0]   = 0x6 -> SCM_GPO8
	 *
	 * 0x40c[31:28] = 0x6 -> HPM_GPO3
	 * 0x40c[27:24] = 0x6 -> HPM_GPO2
	 * 0x40c[23:20] = 0x6 -> HPM_GPO1
	 * 0x40c[19:16] = 0x6 -> HPM_GPO0
	 * 0x40c[15:12] = 0x6 -> HPM_GPI3
	 * 0x40c[11:8]  = 0x6 -> HPM_GPI2
	 * 0x40c[7:4]   = 0x6 -> HPM_GPI1
	 * 0x40c[3:0]   = 0x6 -> HPM_GPI0
	 */
	writel(0x66666666, SCU_IO_REG + 0x408);
	writel(0x66666666, SCU_IO_REG + 0x40c);

	/*
	 * 0x44c[27:24] = 0x5 -> SGPSCK
	 * 0x44c[7:4]   = 0x5 -> SGPSLD
	 */
	reg = readl((unsigned int)SCU_IO_REG + 0x44c);
	reg &= ~(GENMASK(27, 24) | GENMASK(7, 4));
	reg |= ((0x5 << 24) | (0x5 << 4));
	writel(reg, SCU_IO_REG + 0x44c);

	/*
	 * 0x450[15:12] = 0x5 -> SGPSMI
	 * 0x450[11:8]  = 0x5 -> SGPSMO
	 */
	reg = readl((unsigned int)SCU_IO_REG + 0x450);
	reg &= ~GENMASK(15, 8);
	reg |= (0x55 << 8);
	writel(reg, SCU_IO_REG + 0x450);

	/* Enable SGPIO slave */
	writel(0x1, SGPIOS_REG);
}

static void ltpi_hpm_set_pins(void)
{
	uint32_t value;
	uint32_t otp_strap = get_otp_strap();

	if (IS_ENABLED(CONFIG_AS1700_EMU)) {
		writel(0x00003333, SCU_IO_REG + 0x42c);
		writel(0x1, 0x14c34224);
		writel(0x1, 0x14c34228);
		return;
	}

	/*
	 * 0x400[31:28] = 0x7 -> GPILL7 (SIOPWRGD1)
	 * 0x400[27:24] = 0x7 -> GPOLL7 (SIOPWREQN1)
	 * 0x400[23:20] = 0x7 -> GPILL6 (SIOS3N1)
	 * 0x400[19:16] = 0x7 -> GPOLL6 (SIOSCIN1) OD
	 * 0x400[15:12] = 0x7 -> GPILL5 (SIOPBIN1)
	 * 0x400[11:8]  = 0x7 -> GPOLL5 (SIOONCTRLN1) OD
	 * 0x400[7:4]   = 0x7 -> GPILL4 (SIOS5N1)
	 * 0x400[3:0]   = 0x7 -> GPOLL4 (SIOPBON1) OD
	 */
	writel(0x77777777, SCU_IO_REG + 0x400);

	/*
	 * 0x404[31:28] = 0x1 -> TACH7
	 * 0x404[27:24] = 0x1 -> TACH6
	 * 0x404[23:20] = 0x1 -> TACH5
	 * 0x404[19:16] = 0x1 -> TACH4
	 * 0x404[15:12] = 0x1 -> TACH3
	 * 0x404[11:8]  = 0x1 -> TACH2
	 * 0x404[7:4]   = 0x1 -> TACH1
	 * 0x404[3:0]   = 0x1 -> TACH0
	 *
	 * 0x408[31:28] = 0x1 -> TACH15
	 * 0x408[27:24] = 0x1 -> TACH14
	 * 0x408[23:20] = 0x1 -> TACH13
	 * 0x408[19:16] = 0x1 -> TACH12
	 * 0x408[15:12] = 0x1 -> TACH11
	 * 0x408[11:8]  = 0x1 -> TACH10
	 * 0x408[7:4]   = 0x1 -> TACH9
	 * 0x408[3:0]   = 0x1 -> TACH8
	 *
	 * 0x40c[31:28] = 0x1 -> PWM7
	 * 0x40c[27:24] = 0x1 -> PWM6
	 * 0x40c[23:20] = 0x1 -> PWM5
	 * 0x40c[19:16] = 0x1 -> PWM4
	 * 0x40c[15:12] = 0x1 -> PWM3
	 * 0x40c[11:8]  = 0x1 -> PWM2
	 * 0x40c[7:4]   = 0x1 -> PWM1
	 * 0x40c[3:0]   = 0x1 -> PWM0
	 */
	writel(0x11111111, SCU_IO_REG + 0x404);
	writel(0x11111111, SCU_IO_REG + 0x408);
	writel(0x11111111, SCU_IO_REG + 0x40c);

	/*
	 * 0x410[31:28] = 0x1 -> UART_RXD0
	 * 0x410[27:24] = 0x1 -> UART_TXD0
	 * 0x410[23:20] = 0x1 -> UART_RTS0
	 * 0x410[19:16] = 0x6 -> GPINL12
	 * 0x410[15:12] = 0x6 -> GPONL12
	 * 0x410[11:8]  = 0x6 -> GPINL11
	 * 0x410[7:4]   = 0x6 -> GPONL11
	 * 0x410[3:0]   = 0x1 -> UART_CTS0
	 */
	writel(0x11166661, SCU_IO_REG + 0x410);

	/*
	 * 0x414[31:28] = 0x1 -> UART_RXD1
	 * 0x414[27:24] = 0x1 -> UART_TXD1
	 * 0x414[23:20] = 0x1 -> UART_RTS1
	 * 0x414[19:16] = 0x0 -> GPIOF4
	 * 0x414[15:12] = 0x6 -> GPONL14
	 * 0x414[11:8]  = 0x6 -> GPINL13
	 * 0x414[7:4]   = 0x6 -> GPONL13
	 * 0x414[3:0]   = 0x1 -> UART_CTS1
	 */
	writel(0x11106661, SCU_IO_REG + 0x414);

	/*
	 * 0x418[31:28] = 0x6 -> GPINL7
	 * 0x418[27:24] = 0x6 -> GPONL7
	 * 0x418[23:20] = 0x6 -> GPINL6
	 * 0x418[19:16] = 0x6 -> GPONL6
	 * 0x418[15:12] = 0x6 -> GPINL5
	 * 0x418[11:8]  = 0x6 -> GPONL5
	 * 0x418[7:4]   = 0x6 -> GPINL4
	 * 0x418[3:0]   = 0x6 -> GPONL4
	 */
	writel(0x66666666, SCU_IO_REG + 0x418);

	/*
	 * 0x41c[15:12] = 0x1 -> SGPM1I
	 * 0x41c[11:8]  = 0x1 -> SGPM1O
	 * 0x41c[7:4]   = 0x6 -> GPINL8
	 * 0x41c[3:0]   = 0x6 -> GPONL8
	 */
	value = readl((unsigned int)SCU_IO_REG + 0x41c);
	value &= ~GENMASK(15, 0);
	value |= 0x1166;
	writel(value, SCU_IO_REG + 0x41c);

	/*
	 * 0x420[31:28] = 0x6 -> GPINL3
	 * 0x420[27:24] = 0x6 -> GPONL3
	 * 0x420[23:20] = 0x6 -> GPINL2
	 * 0x420[19:16] = 0x6 -> GPONL2
	 * 0x420[15:12] = 0x6 -> GPINL1
	 * 0x420[11:8]  = 0x6 -> GPONL1
	 * 0x420[7:4]   = 0x6 -> GPINL0
	 * 0x420[3:0]   = 0x6 -> GPONL0
	 */
	writel(0x66666666, SCU_IO_REG + 0x420);

	/*
	 * 0x424[31:28] = 0x1 -> I3CSDA3
	 * 0x424[27:24] = 0x1 -> I3CSCL3
	 * 0x424[23:20] = 0x1 -> I3CSDA2
	 * 0x424[19:16] = 0x1 -> I3CSCL2
	 * 0x424[15:12] = 0x1 -> I3CSDA1
	 * 0x424[11:8]  = 0x1 -> I3CSCL1
	 * 0x424[7:4]   = 0x1 -> I3CSDA0
	 * 0x424[3:0]   = 0x1 -> I3CSCL0
	 *
	 * 0x428[31:28] = 0x1 -> I3CSDA7
	 * 0x428[27:24] = 0x1 -> I3CSCL7
	 * 0x428[23:20] = 0x1 -> I3CSDA6
	 * 0x428[19:16] = 0x1 -> I3CSCL6
	 * 0x428[15:12] = 0x1 -> I3CSDA5
	 * 0x428[11:8]  = 0x1 -> I3CSCL5
	 * 0x428[7:4]   = 0x1 -> I3CSDA4
	 * 0x428[3:0]   = 0x1 -> I3CSCL4
	 */
	writel(0x11111111, SCU_IO_REG + 0x424);
	writel(0x11111111, SCU_IO_REG + 0x428);

	/*
	 * if LTPI_IO_TYPE == 1, FSI mode
	 *   0x42c[15:12] = 0x3 -> LTPI_RXDAT
	 *   0x42c[11:8]  = 0x3 -> LTPI_TXDAT
	 *   0x42c[7:4]   = 0x3 -> LTPI_RXCLK
	 *   0x42c[3:0]   = 0x3 -> LTPI_TXCLK
	 * else
	 *   0x42c[15:12] = 0x6 -> GPONL16
	 *   0x42c[11:8]  = 0x6 -> GPINL16
	 *   0x42c[7:4]   = 0x6 -> GPONL15
	 *   0x42c[3:0]   = 0x6 -> GPINL15
	 */
	if (otp_strap & SCU_I_LTPI_IO_TYPE)
		writel(0x00003333, SCU_IO_REG + 0x42c);
	else
		writel(0x00006666, SCU_IO_REG + 0x42c);

	/*
	 * 0x430[31:28] = 0x6 -> GPILL3 (SIOPWRGD0)
	 * 0x430[27:24] = 0x6 -> GPOLL3 (SIOPWREQN0)
	 * 0x430[23:20] = 0x6 -> GPILL2 (SIOS3N0)
	 * 0x430[19:16] = 0x6 -> GPOLL2 (SIOSCIN0) OD
	 * 0x430[15:12] = 0x6 -> GPILL1 (SIOPBIN0)
	 * 0x430[11:8]  = 0x6 -> GPOLL1 (SIOONCTRLN0) OD
	 * 0x430[7:4]   = 0x6 -> GPILL0 (SIOS5N0)
	 * 0x430[3:0]   = 0x6 -> GPOLL0 (SIOPBON0) OD
	 */
	writel(0x66666666, SCU_IO_REG + 0x430);

	/*
	 * 0x434[31:28] = 0x0 -> GPION7
	 * 0x434[27:24] = 0x0 -> GPION6
	 * 0x434[23:20] = 0x0 -> GPION5
	 * 0x434[19:16] = 0x0 -> GPION4
	 * 0x434[15:12] = 0x0 -> GPION3
	 * 0x434[11:8]  = 0x0 -> GPION2
	 * 0x434[7:4]   = 0x0 -> GPION1
	 * 0x434[3:0]   = 0x0 -> GPION0
	 */
	writel(0x00000000, SCU_IO_REG + 0x434);

	/*
	 * 0x438[31:28] = 0x0 -> GPIOO7
	 * 0x438[27:24] = 0x6 -> GPILL10
	 * 0x438[23:20] = 0x0 -> GPIOO5
	 * 0x438[19:16] = 0x6 -> GPOLL10
	 * 0x438[15:12] = 0x6 -> GPILL19
	 * 0x438[11:8]  = 0x6 -> GPOLL9
	 * 0x438[7:4]   = 0x6 -> GPILL8
	 * 0x438[3:0]   = 0x6 -> GPOLL8
	 */
	writel(0x06066666, SCU_IO_REG + 0x438);

	/*
	 * 0x43c[31:28] = 0x0 -> GPIOP7
	 * 0x43c[27:24] = 0x6 -> GPINL14
	 * 0x43c[23:20] = 0x1 -> SPIDQ3
	 * 0x43c[19:16] = 0x1 -> SPIDQ2
	 * 0x43c[15:12] = 0x1 -> SPIMISO
	 * 0x43c[11:8]  = 0x1 -> SPIMOSI
	 * 0x43c[7:4]   = 0x1 -> SPICK
	 * 0x43c[3:0]   = 0x1 -> SPICS0#
	 */
	writel(0x06111111, SCU_IO_REG + 0x43c);

	/*
	 * 0x440[31:28] = 0x0 -> GPIOQ7
	 * 0x440[27:24] = 0x1 -> MTDO
	 * 0x440[23:20] = 0x1 -> MTDI
	 * 0x440[19:16] = 0x1 -> MTMS
	 * 0x440[15:12] = 0x1 -> MTCK
	 * 0x440[11:8]  = 0x1 -> MNTRST
	 * 0x440[7:4]   = 0x0 -> GPIOQ1
	 * 0x440[3:0]   = 0x0 -> GPIOQ0
	 */
	writel(0x01111100, SCU_IO_REG + 0x440);

	/*
	 * 0x444[31:28] = 0x6 -> GPINL26
	 * 0x444[27:24] = 0x6 -> GPONL26
	 * 0x444[23:20] = 0x6 -> GPINL25
	 * 0x444[19:16] = 0x6 -> GPONL25
	 * 0x444[15:12] = 0x6 -> GPINL24
	 * 0x444[11:8] = 0x6 -> GPONL24
	 * 0x444[7:4] = 0x6 -> GPINL23
	 * 0x444[3:0] = 0x6 -> GPONL23
	 */
	writel(0x66666666, SCU_IO_REG + 0x444);

	/*
	 * 0x448[31:28] = 0x0 -> GPIOS7
	 * 0x448[27:24] = 0x0 -> GPIOS6
	 * 0x448[23:20] = 0x1 -> MDIO
	 * 0x448[19:16] = 0x1 -> MDC
	 * 0x448[15:12] = 0x0 -> GPIOS3
	 * 0x448[11:8]  = 0x0 -> GPIOS2
	 * 0x448[7:4]   = 0x6 -> GPINL27
	 * 0x448[3:0]   = 0x6 -> GPONL27
	 */
	writel(0x00110066, SCU_IO_REG + 0x448);

	/*
	 * 0x44c[31:28] = 0x6 -> GPILL13
	 * 0x44c[27:24] = 0x6 -> GPOLL13
	 * 0x44c[23:20] = 0x6 -> GPILL12
	 * 0x44c[19:16] = 0x6 -> GPOLL12
	 * 0x44c[15:12] = 0x6 -> GPILL11
	 * 0x44c[11:8]  = 0x6 -> GPOLL11
	 * 0x44c[7:4]   = 0x0 -> GPIOT1
	 * 0x44c[3:0]   = 0x0 -> GPIOT0
	 */
	writel(0x66666600, SCU_IO_REG + 0x44c);

	/*
	 * 0x450[31:28] = 0x0 -> GPIOU7
	 * 0x450[27:24] = 0x0 -> GPIOU6
	 * 0x450[23:20] = 0x0 -> GPIOU5
	 * 0x450[19:16] = 0x0 -> GPIOU4
	 * 0x450[15:12] = 0x6 -> GPILL15
	 * 0x450[11:8]  = 0x6 -> GPOLL15
	 * 0x450[7:4]   = 0x6 -> GPILL14
	 * 0x450[3:0]   = 0x6 -> GPOLL14
	 */
	writel(0x00006666, SCU_IO_REG + 0x450);

	/*
	 * 0x454[31:28] = 0x1 -> I2CSDA3
	 * 0x454[27:24] = 0x1 -> I2CSCL3
	 * 0x454[23:20] = 0x1 -> I2CSDA2
	 * 0x454[19:16] = 0x1 -> I2CSCL2
	 * 0x454[15:12] = 0x1 -> I2CSDA1
	 * 0x454[11:8]  = 0x1 -> I2CSCL1
	 * 0x454[7:4]   = 0x1 -> I2CSDA0
	 * 0x454[3:0]   = 0x1 -> I2CSCL0
	 */
	writel(0x11111111, SCU_IO_REG + 0x454);

	/*
	 * 0x458[31:28] = 0x0 -> GPIOW7
	 * 0x458[27:24] = 0x0 -> GPIOW6
	 * 0x458[23:20] = 0x0 -> GPIOW5
	 * 0x458[19:16] = 0x0 -> GPIOW4
	 * 0x458[15:12] = 0x1 -> I2CSDA5
	 * 0x458[11:8]  = 0x1 -> I2CSCL5
	 * 0x458[7:4]   = 0x1 -> I2CSDA4
	 * 0x458[3:0]   = 0x1 -> I2CSCL4
	 */
	writel(0x00001111, SCU_IO_REG + 0x458);

	/*
	 * 0x45c[31:28] = 0x6 -> GPONL31
	 * 0x45c[27:24] = 0x6 -> GPINL31
	 * 0x45c[23:20] = 0x6 -> GPONL30
	 * 0x45c[19:16] = 0x6 -> GPINL30
	 * 0x45c[15:12] = 0x6 -> GPONL29
	 * 0x45c[11:8]  = 0x6 -> GPINL29
	 * 0x45c[7:4]   = 0x6 -> GPONL28
	 * 0x45c[3:0]   = 0x6 -> GPINL28
	 */
	writel(0x66666666, SCU_IO_REG + 0x45c);

	/*
	 * 0x460[31:28] = 0x0 -> ADC7
	 * 0x460[27:24] = 0x0 -> ADC6
	 * 0x460[23:20] = 0x0 -> ADC5
	 * 0x460[19:16] = 0x0 -> ADC4
	 * 0x460[15:12] = 0x0 -> ADC3
	 * 0x460[11:8]  = 0x0 -> ADC2
	 * 0x460[7:4]   = 0x0 -> ADC1
	 * 0x460[3:0]   = 0x0 -> ADC0
	 *
	 * 0x464[31:28] = 0x0 -> ADC15
	 * 0x464[27:24] = 0x0 -> ADC14
	 * 0x464[23:20] = 0x0 -> ADC13
	 * 0x464[19:16] = 0x0 -> ADC12
	 * 0x464[15:12] = 0x0 -> ADC11
	 * 0x464[11:8]  = 0x0 -> ADC10
	 * 0x464[7:4]   = 0x0 -> ADC9
	 * 0x464[3:0]   = 0x0 -> ADC8
	 */
	writel(0x00000000, SCU_IO_REG + 0x460);
	writel(0x00000000, SCU_IO_REG + 0x464);

	/*
	 * 0x468[31:28] = 0x0 -> GPIOAA7  (LTPI_LINK_ALIGNED)
	 * 0x468[27:24] = 0x1 -> SGPM1LD
	 * 0x468[23:20] = 0x1 -> SGPM1CK
	 * 0x468[19:16] = 0x6 -> GPINL10
	 * 0x468[15:12] = 0x6 -> GPONL10
	 * 0x468[11:8]  = 0x2 -> HBLED#
	 * 0x468[7:4]   = 0x6 -> GPINL9
	 * 0x468[3:0]   = 0x6 -> GPONL9
	 */
	writel(0x01166266, SCU_IO_REG + 0x468);
}

static void ltpi_hpm_init_addr_map(struct ltpi_priv *ltpi)
{
	uint32_t reg;

	/*
	 * AHB_ADDR_MAP0: bit[34:26] of the destination address
	 *
	 * For AST1700, set AHB_ADDR_MAP0 to 0x4 so that:
	 * AST1700 address [0x3000_0000, 0x33FF_FFFF] will map to
	 * AST2700 address [0x1000_0000, 0x13FF_FFFF]
	 *
	 * This is to redirect the AST1700 IRQ signal to AST2700
	 * interrupt controller (0x1210_0000)
	 */
	reg = readl((void *)ltpi->base + LTPI_AHB_CTRL0);
	reg &= ~REG_LTPI_AHB_ADDR_MAP0;
	reg |= FIELD_PREP(REG_LTPI_AHB_ADDR_MAP0, 0x10000000 >> 26);
	writel(reg, ltpi->base + LTPI_AHB_CTRL0);
}

bool ltpi_query_link_status(int index)
{
	struct ltpi_priv *ltpi = &ltpi_data[index];
	uint32_t status;

	status = ltpi_get_link_manage_state(ltpi);
	if (status == LTPI_LINK_MANAGE_ST_OP)
		return true;

	return false;
}

void ltpi_init(void)
{
	struct ltpi_common *common = &ltpi_common_data;
	struct ltpi_priv *ltpi0 = &ltpi_data[0];
	struct ltpi_priv *ltpi1 = &ltpi_data[1];
	uint32_t pin_strap = get_pin_strap();
	uint32_t otp_strap = get_otp_strap();

	common->soc_rev = FIELD_GET(SCU_CPU_REVISION_ID_HW,
				    readl(ASPEED_CPU_REVISION_ID));
	ltpi0->common = common;
	ltpi1->common = common;
	ltpi0->index = 0;
	ltpi1->index = 1;

	if (common->soc_rev == SOC_REV_AST2700A0) {
		ltpi0->base = LTPI_REG;
		ltpi1->base = LTPI_REG + 0x1000;

		/* AST2700A0 only has one LTPI PHY */
		ltpi0->phy_base = LTPI_REG + 0x200;
		ltpi1->phy_base = LTPI_REG + 0x200;

		ltpi0->reset.regs_assert = SCU_IO_REG + 0x220;
		ltpi0->reset.regs_deassert = SCU_IO_REG + 0x224;
		ltpi0->reset.bit_mask = BIT(20);

		ltpi1->reset.regs_assert = SCU_IO_REG + 0x220;
		ltpi1->reset.regs_deassert = SCU_IO_REG + 0x224;
		ltpi1->reset.bit_mask = BIT(20);

		common->top_base = LTPI_REG + 0x214;
		common->otp_speed_cap =
			otp_to_speed_mask_lookup[FIELD_GET(SCU_I_LTPI_MAX, otp_strap)];
		ltpi0->phy_speed_cap = common->otp_speed_cap;
		ltpi1->phy_speed_cap = common->otp_speed_cap;

		common->sdr_lookup = ltpi_clk_lookup_sdr_a0;
		common->cdr_lookup = ltpi_clk_lookup_cdr_a0;
		common->ddr_lookup = ltpi_clk_lookup_sdr_a0;
		common->cdr2x_lookup = ltpi_clk_lookup_cdr_a0;
	} else {
		uint32_t rx_ctrl;

		ltpi0->base = LTPI_REG;
		ltpi1->base = LTPI_REG + 0x1000;
		ltpi0->phy_base = ltpi0->base + 0x200;
		ltpi1->phy_base = ltpi1->base + 0x200;

		ltpi0->reset.regs_assert = SCU_IO_REG + 0x220;
		ltpi0->reset.regs_deassert = SCU_IO_REG + 0x224;
		ltpi0->reset.bit_mask = BIT(20);

		ltpi1->reset.regs_assert = SCU_IO_REG + 0x220;
		ltpi1->reset.regs_deassert = SCU_IO_REG + 0x224;
		ltpi1->reset.bit_mask = BIT(22);

		common->top_base = LTPI_REG + 0x800;

		/* FIXME: the read OTPCFG31 for the speed mask on A1 */
		common->otp_speed_cap =
			otp_to_speed_mask_lookup[FIELD_GET(SCU_I_LTPI_MAX, otp_strap)];
		ltpi0->phy_speed_cap = common->otp_speed_cap;
		ltpi1->phy_speed_cap = common->otp_speed_cap;

		common->sdr_lookup = ltpi_clk_lookup_sdr;
		common->ddr_lookup = ltpi_clk_lookup_ddr;
		common->cdr_lookup = ltpi_clk_lookup_cdr;
		common->cdr2x_lookup = ltpi_clk_lookup_cdr2x;

		/* Unlock the LTPI registers */
		writel(LTPI_PROT_KEY_UNLOCK, ltpi0->phy_base + LTPI_PROT_KEY);
		writel(LTPI_PROT_KEY_UNLOCK, ltpi1->phy_base + LTPI_PROT_KEY);

		rx_ctrl = readl(common->top_base + LTPI_LVDS_RX_CTRL);
		rx_ctrl |= REG_LTPI_LVDS_RX0_BIAS_EN |
			   REG_LTPI_LVDS_RX1_BIAS_EN;
		writel(rx_ctrl, common->top_base + LTPI_LVDS_RX_CTRL);
		udelay(1);
	}

	if (pin_strap & SCU_I_LTPI_MODE) {
		bootstage_start_mark(BOOTSTAGE_LTPI_SLAVE);
		bootstage_end_status(BOOTSTAGE_STATUS_SUCCESS);
		ltpi_hpm_set_pins();
		if (pin_strap & SCU_I_LTPI_NUM)
			ltpi_hpm_cdr_loop(ltpi0);
		else
			ltpi_hpm_normal_mode_loop(ltpi0);

		ltpi_hpm_init_addr_map(ltpi0);
	} else {
		if (pin_strap & SCU_I_SCM_MODE) {
			bootstage_start_mark(BOOTSTAGE_LTPI_MASTER);
			bootstage_end_status(BOOTSTAGE_STATUS_SUCCESS);
			ltpi_scm_set_pins();
			ltpi_scm_init(ltpi0, ltpi1);
		} else {
			/* AST2700 non-SCM mode, skip LTPI */
			bootstage_start_mark(BOOTSTAGE_LTPI_INIT);
			bootstage_end_status(BOOTSTAGE_LTPI_INIT_SKIP |
					     BOOTSTAGE_LTPI_MODE_NONE);
		}
	}
}

static void ltpi_show_status(struct ltpi_priv *ltpi)
{
	int speed, ret, i;
	uint32_t phy_mode, clk_select;
	char modes[9][8] = { "OFF", "SDR", "DDR", "NA",	   "CDR_LO",
			     "NA",  "NA",  "NA",  "CDR_HI" };

	ret = ltpi_get_link_manage_state(ltpi);
	if (ret != LTPI_LINK_MANAGE_ST_OP) {
		printf("LTPI%d: Not linked\n", ltpi->index);
		return;
	}

	phy_mode = FIELD_GET(REG_LTPI_PHY_MODE,
			     readl((void *)ltpi->phy_base + LTPI_PHY_CTRL));
	clk_select = FIELD_GET(REG_LTPI_PLL_SELECT,
			       readl((void *)ltpi->phy_base + LTPI_PLL_CTRL));
	if (phy_mode == LTPI_PHY_MODE_SDR) {
		for (i = 0; i < 13; i++)
			if (clk_select == ltpi->common->sdr_lookup[i].clk_sel)
				speed = ltpi->common->sdr_lookup[i].bandwidth;
	}

	if (phy_mode == LTPI_PHY_MODE_CDR_LO_SP) {
		for (i = 0; i < 6; i++)
			if (clk_select == ltpi->common->cdr_lookup[i].clk_sel)
				speed = ltpi->common->cdr_lookup[i].bandwidth;
	}

	if (phy_mode == LTPI_PHY_MODE_CDR_HI_SP) {
		for (i = 6; i < 13; i++)
			if (clk_select == ltpi->common->cdr_lookup[i].clk_sel)
				speed = ltpi->common->cdr_lookup[i].bandwidth;
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
	int opt, speed = 0, mode = 0;
	struct getopt_state gs;
	char *endp;
	uint32_t pin_strap, otp_strap;

	ltpi_common_data.cdr_mask = CDR_MASK_ALL;
	ltpi_data[0].clk_inverse = 0x0;
	ltpi_data[1].clk_inverse = 0x0;

	getopt_init_state(&gs);
	while ((opt = getopt(&gs, argc, argv, "l:m:c:i:h")) > 0) {
		switch (opt) {
		case 'l':
			speed = simple_strtoul(gs.arg, &endp, 0);
			break;
		case 'm':
			mode = simple_strtoul(gs.arg, &endp, 0);
			break;
		case 'c':
			ltpi_common_data.cdr_mask = simple_strtoul(gs.arg, &endp, 0);
			break;
		case 'i':
			ltpi_data[0].clk_inverse = simple_strtoul(gs.arg, &endp, 0);
			ltpi_data[1].clk_inverse = ltpi_data[0].clk_inverse;
			break;
		case 'h':
			fallthrough;
		default:
			return CMD_RET_USAGE;
		}
	}

	/* Set pin strap according to the command argument */
	pin_strap = readl((void *)SCU_IO_REG + 0x10);
	pin_strap &= 0xfffffff0;
	switch (mode) {
	case 3:
		pin_strap |= SCU_I_LTPI_IDX;
		fallthrough;
	case 2:
		pin_strap |= SCU_I_LTPI_NUM;
		fallthrough;
	case 1:
		pin_strap |= SCU_I_LTPI_MODE;
		break;
	default:
		pin_strap |= SCU_I_SCM_MODE;
		break;
	}
	writel(pin_strap, (void *)SCU_IO_REG + 0x10);

	/* Set otp strap according to the command argument */
	otp_strap = readl((void *)SCU_IO_REG + 0x30);
	otp_strap &= ~SCU_I_LTPI_MAX;
	otp_strap |= FIELD_PREP(SCU_I_LTPI_MAX, speed);
	/* AST2700A0 workaround */
	if (ltpi_common_data.soc_rev == SOC_REV_AST2700A0)
		writel(0x80000000, (void *)SCU_IO_REG + 0x34);
	writel(otp_strap, (void *)SCU_IO_REG + 0x30);

	ltpi_init();
	if (pin_strap & SCU_I_SCM_MODE) {
		uint32_t reg;

		if (ltpi_get_link_partner(&ltpi_data[0]) ||
		    ltpi_get_link_partner(&ltpi_data[1]))
			reg = FIELD_PREP(REG_LTPI_AHB_ADDR_MAP0, 0x5) |
			      FIELD_PREP(REG_LTPI_AHB_ADDR_MAP1, 0xa0);
		else
			reg = 0;

		writel(reg, (void *)ltpi_data[0].base + LTPI_AHB_CTRL0);
		writel(reg, (void *)ltpi_data[1].base + LTPI_AHB_CTRL0);

		ltpi_show_status(&ltpi_data[0]);
		ltpi_show_status(&ltpi_data[1]);
	}

	return CMD_RET_SUCCESS;
}

static char ltpi_help_text[] = {
	"-m 0: AST2700 mode\n"
	"-m 1: AST1700 mode, SDR\n"
	"-m 2: AST1700 mode, CDR, 1st AST1700\n"
	"-m 3: AST1700 mode, CDR, 2nd AST1700\n"
	"-l <speed limit>, 0=1G, 1=800M, 2=400M, 3=250M, 4=200M, 5=100M, 6=50M, 7=25M\n"
	"-c <cdr mask>, 0x0=disable CDR, 0x1=ltpi0, 0x2=ltpi1, 0x3=ltpi0+ltpi1\n"
	"-i <clk inverse>, 0x0=no inverse, 0x1=inverse tx, 0x2=inverse rx, 0x3=inverse both\n"
};

U_BOOT_CMD(ltpi, 5, 0, do_ltpi, "ASPEED LTPI commands", ltpi_help_text);
