// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */
#include <asm/io.h>
#include <dm.h>
#include <ram.h>
#include <spl.h>
#include <common.h>
#include <asm/csr.h>
#include <asm/arch-aspeed/platform.h>
#include <asm/arch-aspeed/sli_ast2700.h>
#include <asm/arch-aspeed/scu_ast2700.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/iopoll.h>

#define SLI_POLL_TIMEOUT_US	100

static void sli_clear_interrupt_status(uint32_t base)
{
	writel(0xfffff, (void *)base + SLI_INTR_STATUS);
}

static int sli_wait(uint32_t base, uint32_t mask)
{
	uint32_t value;

	sli_clear_interrupt_status(base);

	do {
		value = readl((void *)base + SLI_INTR_STATUS);
		if (value & SLI_INTR_RX_ERRORS)
			return -1;
	} while ((value & mask) != mask);

	return 0;
}

static int sli_wait_suspend(uint32_t base)
{
	return sli_wait(base, SLI_INTR_TX_SUSPEND | SLI_INTR_RX_SUSPEND);
}

static int is_sli_suspend(uint32_t base)
{
	uint32_t value;
	uint32_t suspend = SLI_INTR_TX_SUSPEND | SLI_INTR_RX_SUSPEND;

	value = readl((void *)base + SLI_INTR_STATUS);
	if (value & SLI_INTR_RX_ERRORS)
		return -1;
	else if ((value & suspend) == suspend)
		return 1;
	else
		return 0;
}

static int sli_wait_clear_done(uint32_t base, uint32_t mask)
{
	uint32_t value;

	return readl_poll_timeout((void *)base + SLI_CTRL_I, value, (value & mask) == 0,
				  SLI_POLL_TIMEOUT_US);
}

static int sli_clear(uint32_t base, uint32_t clr)
{
	setbits_le32(base + SLI_CTRL_I, clr);

	return sli_wait_clear_done(base, clr & ~SLI_CLEAR_BUS);
}

static void __maybe_unused sli_set_ahb_rx_delay_single(uint32_t base, int index, int d)
{
	uint32_t offset = index * 6;
	uint32_t mask = SLIH_PAD_DLY_RX0 << offset;

	clrsetbits_le32(base + SLI_CTRL_III, mask, d << offset);
	udelay(8);
}

static void sli_set_ahb_rx_delay(uint32_t base, int d0, int d1)
{
	uint32_t value;

	value = FIELD_PREP(SLIH_PAD_DLY_RX1, d1) | FIELD_PREP(SLIH_PAD_DLY_RX0, d0);
	clrsetbits_le32(base + SLI_CTRL_III, SLIH_PAD_DLY_RX1 | SLIH_PAD_DLY_RX0, value);
	udelay(8);
}

static void sli_calibrate_ahb_delay(int config)
{
	int dc;
	int d_first_pass = -1;
	int d_last_pass = -1;

	if (config)
		setbits_le32(SLIH_IOD_BASE + SLI_CTRL_I, SLI_RX_PHY_LAH_SEL_NEG);
	else
		clrbits_le32(SLIH_IOD_BASE + SLI_CTRL_I, SLI_RX_PHY_LAH_SEL_NEG);

	for (dc = 6; dc < 20; dc++) {
		sli_set_ahb_rx_delay(SLIH_IOD_BASE, dc, dc);
		sli_clear(SLIH_IOD_BASE, SLI_CLEAR_RX | SLI_CLEAR_BUS);

		/* Check result */
		sli_clear_interrupt_status(SLIH_IOD_BASE);
		udelay(10);
		if (is_sli_suspend(SLIH_IOD_BASE) > 0) {
			if (d_first_pass == -1)
				d_first_pass = dc;

			d_last_pass = dc;
		} else if (d_last_pass != -1) {
			break;
		}
	}

	dc = (d_first_pass + d_last_pass) >> 1;
	debug("IOD SLIH DS coarse win: {%d, %d} -> select %d\n", d_first_pass, d_last_pass, dc);

	sli_set_ahb_rx_delay(SLIH_IOD_BASE, dc, dc);

	/* Reset IOD SLIH Bus (to reset the counters) and RX */
	sli_clear(SLIH_IOD_BASE, SLI_CLEAR_RX | SLI_CLEAR_BUS);

	/* Turn on the hardware training and wait suspend state */
	clrbits_le32(SLIH_IOD_BASE + SLI_CTRL_I, SLI_AUTO_SEND_TRN_OFF);
	sli_wait_suspend(SLIH_IOD_BASE);

	/* SLI-H is available now */
}

static void sli_set_mbus_rx_delay_single(uint32_t base, int index, int d)
{
	uint32_t offset = index * 6;
	uint32_t mask = SLIM_PAD_DLY_RX0 << offset;

	clrsetbits_le32(base + SLI_CTRL_III, mask, d << offset);
	udelay(8);
}

static void sli_set_mbus_rx_delay(uint32_t base, int d0, int d1, int d2, int d3)
{
	uint32_t clr, set;

	clr = SLIM_PAD_DLY_RX3 | SLIM_PAD_DLY_RX2 | SLIM_PAD_DLY_RX1 | SLIM_PAD_DLY_RX0;
	set = FIELD_PREP(SLIM_PAD_DLY_RX3, d3) | FIELD_PREP(SLIM_PAD_DLY_RX2, d2) |
	      FIELD_PREP(SLIM_PAD_DLY_RX1, d1) | FIELD_PREP(SLIM_PAD_DLY_RX0, d0);
	clrsetbits_le32(base + SLI_CTRL_III, clr, set);
	udelay(8);
}

static int sli_calibrate_mbus_pad_delay(int index, int begin, int end)
{
	int d;
	int d_first_pass = -1;
	int d_last_pass = -1;

	for (d = begin; d < end; d++) {
		sli_set_mbus_rx_delay_single(SLIM_IOD_BASE, index, d);

		/* Reset CPU-die TX and IO-die RX */
		sli_clear(SLIM_CPU_BASE, SLI_CLEAR_TX | SLI_CLEAR_BUS);
		sli_clear(SLIM_IOD_BASE, SLI_CLEAR_RX | SLI_CLEAR_BUS);

		/* Check result */
		sli_clear_interrupt_status(SLIM_IOD_BASE);
		udelay(10);
		if (is_sli_suspend(SLIM_IOD_BASE) > 0) {
			if (d_first_pass == -1)
				d_first_pass = d;

			d_last_pass = d;
		} else if (d_last_pass != -1) {
			break;
		}
	}

	if (d_first_pass == -1)
		d = SLIM_DEFAULT_DELAY;
	else
		d = (d_first_pass + d_last_pass) >> 1;

	debug("IOD SLIM[%d] DS win: {%d, %d} -> select %d\n", index, d_first_pass, d_last_pass, d);

	return d;
}

static void sli_calibrate_mbus_delay(int config)
{
	int dc, d0, d1, d2, d3;
	int begin, end;
	int d_first_pass = -1;
	int d_last_pass = -1;

	if (config)
		setbits_le32(SLIM_IOD_BASE + SLI_CTRL_I, SLI_RX_PHY_LAH_SEL_NEG);
	else
		clrbits_le32(SLIM_IOD_BASE + SLI_CTRL_I, SLI_RX_PHY_LAH_SEL_NEG);

	/* Find coarse delay */
	for (dc = 0; dc < 16; dc++) {
		sli_set_mbus_rx_delay(SLIM_IOD_BASE, dc, dc, dc, dc);

		/* Reset CPU-die TX and IO-die RX */
		sli_clear(SLIM_CPU_BASE, SLI_CLEAR_TX | SLI_CLEAR_BUS);
		sli_clear(SLIM_IOD_BASE, SLI_CLEAR_RX | SLI_CLEAR_BUS);

		/* Check result */
		sli_clear_interrupt_status(SLIM_IOD_BASE);
		udelay(10);
		if (is_sli_suspend(SLIM_IOD_BASE) > 0) {
			if (d_first_pass == -1)
				d_first_pass = dc;

			d_last_pass = dc;
		} else if (d_last_pass != -1) {
			break;
		}
	}

	if (d_first_pass < 0)
		dc = SLIM_DEFAULT_DELAY;
	else
		dc = (d_first_pass + d_last_pass) >> 1;

	debug("IOD SLIM DS coarse win: {%d, %d} -> select %d\n", d_first_pass, d_last_pass, dc);

	sli_set_mbus_rx_delay(SLIM_IOD_BASE, dc, dc, dc, dc);

	begin = max(dc - 5, 0);
	end = min(dc + 5, 32);

	/* Fine-tune per-PAD delay */
	d0 = sli_calibrate_mbus_pad_delay(0, begin, end);
	sli_set_mbus_rx_delay_single(SLIM_IOD_BASE, 0, d0);

	d1 = sli_calibrate_mbus_pad_delay(1, begin, end);
	sli_set_mbus_rx_delay_single(SLIM_IOD_BASE, 1, d1);

	d2 = sli_calibrate_mbus_pad_delay(2, begin, end);
	sli_set_mbus_rx_delay_single(SLIM_IOD_BASE, 2, d2);

	d3 = sli_calibrate_mbus_pad_delay(3, begin, end);
	sli_set_mbus_rx_delay_single(SLIM_IOD_BASE, 3, d3);

	/* Reset CPU-die TX and IO-die RX */
	sli_clear(SLIM_CPU_BASE, SLI_CLEAR_TX | SLI_CLEAR_BUS);
	sli_clear(SLIM_IOD_BASE, SLI_CLEAR_RX | SLI_CLEAR_BUS);

	/* Turn on the hardware training and wait suspend state */
	clrbits_le32(SLIM_IOD_BASE + SLI_CTRL_I, SLI_AUTO_SEND_TRN_OFF);
	sli_wait_suspend(SLIM_IOD_BASE);

	/* Enable the MARB RR mode for AST2700A0 */
	setbits_le32(SLIM_IOD_BASE + SLIM_MARB_FUNC_I, SLIM_SLI_MARB_RR);
}

static void sli_set_cpu_die_hpll(void)
{
	uint32_t value;

	/* Switch CPU-die HPLL to 1575M */
	value = readl((void *)ASPEED_CPU_HPLL);
	value &= ~(SCU_CPU_HPLL_P | SCU_CPU_HPLL_N | SCU_CPU_HPLL_M);
	value |= FIELD_PREP(SCU_CPU_HPLL_P, 0x0) |
		 FIELD_PREP(SCU_CPU_HPLL_N, 0x0) |
		 FIELD_PREP(SCU_CPU_HPLL_M, 0x7d);
	writel(value, (void *)ASPEED_CPU_HPLL);

	value = readl((void *)ASPEED_CPU_HPLL2);
	value &= ~SCU_CPU_HPLL2_BWADJ;
	value |= FIELD_PREP(SCU_CPU_HPLL2_BWADJ, 0x3e);
	writel(value, (void *)ASPEED_CPU_HPLL2);
	do {
		value = readl((void *)ASPEED_CPU_HPLL2);
	} while ((value & SCU_CPU_HPLL2_LOCK) == 0);
}

/*
 * CPU die  --- downstream pads ---> I/O die
 * CPU die  <--- upstream pads ----- I/O die
 *
 * US/DS PAD[3:0] : SLIM[3:0]
 * US/DS PAD[5:4] : SLIH[1:0]
 * US/DS PAD[7:6] : SLIV[1:0]
 */
int sli_init(void)
{
	uint32_t value;
	bool is_ast2700a0;

	__maybe_unused int phyclk_lookup[8] = {
		25, 800, 400, 200, 2000, 1000, 500, 250,
	};

	/* The following training sequence is designed for AST2700A0 */
	value = FIELD_GET(SCU_CPU_REVISION_ID_HW,
			  readl((void *)ASPEED_IO_REVISION_ID));
	is_ast2700a0 = value ? false : true;

	/* Return if SLI had been calibrated */
	value = readl((void *)SLIH_IOD_BASE + SLI_CTRL_III);
	value = FIELD_GET(SLI_CLK_SEL, value);
	if (value) {
		debug("SLI has been initialized\n");
		return 0;
	}

	if (is_ast2700a0) {
		/* 25MHz PAD delay for AST2700A0 */
		value = SLI_RX_PHY_LAH_SEL_NEG | SLI_TRANS_EN | SLI_CLEAR_BUS;
		writel(value, (void *)SLIH_IOD_BASE + SLI_CTRL_I);
		writel(value, (void *)SLIM_IOD_BASE + SLI_CTRL_I);
		writel(value | SLIV_RAW_MODE, (void *)SLIV_IOD_BASE + SLI_CTRL_I);
		sli_wait_suspend(SLIH_IOD_BASE);
		sli_wait_suspend(SLIH_CPU_BASE);
		debug("SLI US/DS @ 25MHz init done\n");

		/* AST2700A0 workaround to save SD waveform */
		sli_set_cpu_die_hpll();
		phyclk_lookup[5] = 788;
	}

	if (IS_ENABLED(CONFIG_SLI_TARGET_PHYCLK_25MHZ))
		return 0;

	/* Change SLIH engine clock to 500M */
	value = FIELD_PREP(SLI_CLK_SEL, SLI_CLK_500M);
	clrsetbits_le32(SLIH_IOD_BASE + SLI_CTRL_III, SLI_CLK_SEL, value);
	clrsetbits_le32(SLIH_CPU_BASE + SLI_CTRL_III, SLI_CLK_SEL, value);

	/* Turn off auto-clear for AST2700A1 */
	if (!is_ast2700a0) {
		setbits_le32(SLIH_IOD_BASE + SLI_CTRL_I,
			     SLI_AUTO_CLR_OFF_DAT | SLI_AUTO_CLR_OFF_CLK | SLI_NO_RST_TXCLK_CHG);
		setbits_le32(SLIH_CPU_BASE + SLI_CTRL_I,
			     SLI_AUTO_CLR_OFF_DAT | SLI_AUTO_CLR_OFF_CLK | SLI_NO_RST_TXCLK_CHG);
	}

	/* Speed up the CPU SLIH PHY clock. Don't access CPU-die from now on */
	value = FIELD_PREP(SLI_PHYCLK_SEL, SLI_TARGET_PHYCLK);
	clrsetbits_le32(SLIH_CPU_BASE + SLI_CTRL_III,
			SLI_PHYCLK_SEL | SLIH_PAD_DLY_TX1 | SLIH_PAD_DLY_TX0,
			value);

	/* IOD SLIM/H/V training off */
	setbits_le32(SLIH_IOD_BASE + SLI_CTRL_I, SLI_AUTO_SEND_TRN_OFF);
	setbits_le32(SLIM_IOD_BASE + SLI_CTRL_I, SLI_AUTO_SEND_TRN_OFF);
	setbits_le32(SLIV_IOD_BASE + SLI_CTRL_I, SLI_AUTO_SEND_TRN_OFF);

	/* Calibrate SLIH DS delay */
	sli_calibrate_ahb_delay(0);

	/* It's okay to access CPU-die now. Calibrate SLIM DS delay */
	sli_calibrate_mbus_delay(SLIM_LAH_CONFIG);

	debug("SLI DS @ %dMHz init done\n", phyclk_lookup[SLI_TARGET_PHYCLK]);
	return 0;
}
