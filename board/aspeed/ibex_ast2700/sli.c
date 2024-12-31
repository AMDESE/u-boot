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

#define SLI0_REG		0x12c17000
#define SLI1_REG		0x14c1e000

struct sli_config {
	uintptr_t slim; /* SLI MBUS */
	uintptr_t slih; /* SLI AHB */
	uintptr_t sliv; /* SLI VIDEO */
	int eng_clk_freq;
	int phy_clk_freq;
};

struct sli_data {
	struct sli_config die0;	/* CPU die */
	struct sli_config die1;	/* IO die */

#define SLI_FLAG_AST2700A0		BIT(0)
#define SLI_FLAG_RX_LAH_NEG_IO_SLIH	BIT(1)
#define SLI_FLAG_RX_LAH_NEG_IO_SLIM	BIT(2)
	uint32_t flags;
};

static void sli_clear_interrupt_status(uintptr_t base)
{
	writel(0xfffff, (void *)base + SLI_INTR_STATUS);
}

static int sli_wait(uintptr_t base, uint32_t mask)
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

static int sli_wait_suspend(uintptr_t base)
{
	return sli_wait(base, SLI_INTR_TX_SUSPEND | SLI_INTR_RX_SUSPEND);
}

static int is_sli_suspend(uintptr_t base)
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

static int sli_wait_clear_done(uintptr_t base, uint32_t mask)
{
	uint32_t value;

	return readl_poll_timeout((void *)base + SLI_CTRL_I, value, (value & mask) == 0,
				  SLI_POLL_TIMEOUT_US);
}

static int sli_clear(uintptr_t base, uint32_t clr)
{
	setbits_le32(base + SLI_CTRL_I, clr);

	return sli_wait_clear_done(base, clr & ~SLI_CLEAR_BUS);
}

static void __maybe_unused sli_set_ahb_rx_delay_single(uintptr_t base, int index, int d)
{
	uint32_t offset = index * 6;
	uint32_t mask = SLIH_PAD_DLY_RX0 << offset;

	clrsetbits_le32(base + SLI_CTRL_III, mask, d << offset);
	udelay(8);
}

static void sli_set_ahb_rx_delay(uintptr_t base, int d0, int d1)
{
	uint32_t value;

	value = FIELD_PREP(SLIH_PAD_DLY_RX1, d1) | FIELD_PREP(SLIH_PAD_DLY_RX0, d0);
	clrsetbits_le32(base + SLI_CTRL_III, SLIH_PAD_DLY_RX1 | SLIH_PAD_DLY_RX0, value);
	udelay(8);
}

static void sli_calibrate_ahb_delay(struct sli_data *data)
{
	int dc;
	int d_first_pass = -1;
	int d_last_pass = -1;

	if (data->flags & SLI_FLAG_RX_LAH_NEG_IO_SLIH)
		setbits_le32(data->die1.slih + SLI_CTRL_I, SLI_RX_PHY_LAH_SEL_NEG);
	else
		clrbits_le32(data->die1.slih + SLI_CTRL_I, SLI_RX_PHY_LAH_SEL_NEG);

	for (dc = 6; dc < 20; dc++) {
		sli_set_ahb_rx_delay(data->die1.slih, dc, dc);
		sli_clear(data->die1.slih, SLI_CLEAR_RX | SLI_CLEAR_BUS);

		/* Check result */
		sli_clear_interrupt_status(data->die1.slih);
		udelay(10);
		if (is_sli_suspend(data->die1.slih) > 0) {
			if (d_first_pass == -1)
				d_first_pass = dc;

			d_last_pass = dc;
		} else if (d_last_pass != -1) {
			break;
		}
	}

	dc = (d_first_pass + d_last_pass) >> 1;
	debug("IOD SLIH DS coarse win: {%d, %d} -> select %d\n", d_first_pass, d_last_pass, dc);

	sli_set_ahb_rx_delay(data->die1.slih, dc, dc);

	/* Reset IOD SLIH Bus (to reset the counters) and RX */
	sli_clear(data->die1.slih, SLI_CLEAR_RX | SLI_CLEAR_BUS);

	/* Turn on the hardware training and wait suspend state */
	clrbits_le32(data->die1.slih + SLI_CTRL_I, SLI_AUTO_SEND_TRN_OFF);
	sli_wait_suspend(data->die1.slih);

	/* SLI-H is available now */
}

static void sli_set_mbus_rx_delay_single(uintptr_t base, int index, int d)
{
	uint32_t offset = index * 6;
	uint32_t mask = SLIM_PAD_DLY_RX0 << offset;

	clrsetbits_le32(base + SLI_CTRL_III, mask, d << offset);
	udelay(8);
}

static void sli_set_mbus_rx_delay(uintptr_t base, int d0, int d1, int d2, int d3)
{
	uint32_t clr, set;

	clr = SLIM_PAD_DLY_RX3 | SLIM_PAD_DLY_RX2 | SLIM_PAD_DLY_RX1 | SLIM_PAD_DLY_RX0;
	set = FIELD_PREP(SLIM_PAD_DLY_RX3, d3) | FIELD_PREP(SLIM_PAD_DLY_RX2, d2) |
	      FIELD_PREP(SLIM_PAD_DLY_RX1, d1) | FIELD_PREP(SLIM_PAD_DLY_RX0, d0);
	clrsetbits_le32(base + SLI_CTRL_III, clr, set);
	udelay(8);
}

static int sli_calibrate_mbus_pad_delay(struct sli_data *data, int index, int begin, int end)
{
	int d;
	int d_first_pass = -1;
	int d_last_pass = -1;
	int d_def = 12;

	if (data->die0.phy_clk_freq == SLI_PHYCLK_800M ||
	    data->die0.phy_clk_freq == SLI_PHYCLK_788M)
		d_def = 5;

	for (d = begin; d < end; d++) {
		sli_set_mbus_rx_delay_single(data->die1.slim, index, d);

		/* Reset CPU-die TX and IO-die RX */
		sli_clear(data->die0.slim, SLI_CLEAR_TX | SLI_CLEAR_BUS);
		sli_clear(data->die1.slim, SLI_CLEAR_RX | SLI_CLEAR_BUS);

		/* Check result */
		sli_clear_interrupt_status(data->die1.slim);
		udelay(10);
		if (is_sli_suspend(data->die1.slim) > 0) {
			if (d_first_pass == -1)
				d_first_pass = d;

			d_last_pass = d;
		} else if (d_last_pass != -1) {
			break;
		}
	}

	if (d_first_pass == -1)
		d = d_def;
	else
		d = (d_first_pass + d_last_pass) >> 1;

	debug("IOD SLIM[%d] DS win: {%d, %d} -> select %d\n", index, d_first_pass, d_last_pass, d);

	return d;
}

static void sli_calibrate_mbus_delay(struct sli_data *data)
{
	int dc, d0, d1, d2, d3;
	int begin, end;
	int d_first_pass = -1;
	int d_last_pass = -1;
	int d_def = 12;

	if (data->die0.phy_clk_freq == SLI_PHYCLK_800M ||
	    data->die0.phy_clk_freq == SLI_PHYCLK_788M)
		d_def = 5;

	if (data->flags & SLI_FLAG_RX_LAH_NEG_IO_SLIM)
		setbits_le32(data->die1.slim + SLI_CTRL_I, SLI_RX_PHY_LAH_SEL_NEG);
	else
		clrbits_le32(data->die1.slim + SLI_CTRL_I, SLI_RX_PHY_LAH_SEL_NEG);

	/* Find coarse delay */
	for (dc = 0; dc < 16; dc++) {
		sli_set_mbus_rx_delay(data->die1.slim, dc, dc, dc, dc);

		/* Reset CPU-die TX and IO-die RX */
		sli_clear(data->die0.slim, SLI_CLEAR_TX | SLI_CLEAR_BUS);
		sli_clear(data->die1.slim, SLI_CLEAR_RX | SLI_CLEAR_BUS);

		/* Check result */
		sli_clear_interrupt_status(data->die1.slim);
		udelay(10);
		if (is_sli_suspend(data->die1.slim) > 0) {
			if (d_first_pass == -1)
				d_first_pass = dc;

			d_last_pass = dc;
		} else if (d_last_pass != -1) {
			break;
		}
	}

	if (d_first_pass < 0)
		dc = d_def;
	else
		dc = (d_first_pass + d_last_pass) >> 1;

	debug("IOD SLIM DS coarse win: {%d, %d} -> select %d\n", d_first_pass, d_last_pass, dc);

	sli_set_mbus_rx_delay(data->die1.slim, dc, dc, dc, dc);

	begin = max(dc - 5, 0);
	end = min(dc + 5, 31);

	/* Fine-tune per-PAD delay */
	d0 = sli_calibrate_mbus_pad_delay(data, 0, begin, end);
	sli_set_mbus_rx_delay_single(data->die1.slim, 0, d0);

	d1 = sli_calibrate_mbus_pad_delay(data, 1, begin, end);
	sli_set_mbus_rx_delay_single(data->die1.slim, 1, d1);

	d2 = sli_calibrate_mbus_pad_delay(data, 2, begin, end);
	sli_set_mbus_rx_delay_single(data->die1.slim, 2, d2);

	d3 = sli_calibrate_mbus_pad_delay(data, 3, begin, end);
	sli_set_mbus_rx_delay_single(data->die1.slim, 3, d3);

	/* Reset CPU-die TX and IO-die RX */
	sli_clear(data->die0.slim, SLI_CLEAR_TX | SLI_CLEAR_BUS);
	sli_clear(data->die1.slim, SLI_CLEAR_RX | SLI_CLEAR_BUS);

	/* Turn on the hardware training and wait suspend state */
	clrbits_le32(data->die1.slim + SLI_CTRL_I, SLI_AUTO_SEND_TRN_OFF);
	sli_wait_suspend(data->die1.slim);

	/* Enable the MARB RR mode for AST2700A0 */
	setbits_le32(data->die1.slim + SLIM_MARB_FUNC_I, SLIM_SLI_MARB_RR);
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
int sli_init_f(void)
{
	struct sli_data ast2700_sli_data[1];
	struct sli_data *data = ast2700_sli_data;
	uint32_t reg_val;

	__maybe_unused int phyclk_lookup[8] = {
		25, 800, 400, 200, 2000, 1000, 500, 250,
	};

	/* CPU die */
	data->die0.slim = SLI0_REG + 0x000;
	data->die0.slih = SLI0_REG + 0x200;
	data->die0.sliv = SLI0_REG + 0x400;
	data->die0.eng_clk_freq = SLI_CLK_500M;
	if (IS_ENABLED(CONFIG_SLI_TARGET_PHYCLK_1GHZ))
		data->die0.phy_clk_freq = SLI_PHYCLK_1G;
	else if (IS_ENABLED(CONFIG_SLI_TARGET_PHYCLK_800MHZ))
		data->die0.phy_clk_freq = SLI_PHYCLK_800M;
	else if (IS_ENABLED(CONFIG_SLI_TARGET_PHYCLK_500MHZ))
		data->die0.phy_clk_freq = SLI_PHYCLK_500M;
	else if (IS_ENABLED(CONFIG_SLI_TARGET_PHYCLK_400MHZ))
		data->die0.phy_clk_freq = SLI_PHYCLK_400M;
	else
		data->die0.phy_clk_freq = SLI_PHYCLK_25M;

	/* IO die */
	data->die1.slim = SLI1_REG + 0x000;
	data->die1.slih = SLI1_REG + 0x200;
	data->die1.sliv = SLI1_REG + 0x400;
	data->die1.eng_clk_freq = SLI_CLK_500M;
	data->die1.phy_clk_freq = SLI_PHYCLK_25M;

	data->flags = 0;

	reg_val = readl((void *)ASPEED_IO_REVISION_ID);
	if (FIELD_GET(SCU_CPU_REVISION_ID_HW, reg_val) == 0)
		data->flags |= SLI_FLAG_AST2700A0;

	/* Return if SLI had been calibrated */
	reg_val = readl((void *)data->die1.slih + SLI_CTRL_III);
	reg_val = FIELD_GET(SLI_CLK_SEL, reg_val);
	if (reg_val) {
		debug("SLI has been initialized\n");
		return 0;
	}

	/* AST2700A0 workaround for 25MHz */
	if (data->flags & SLI_FLAG_AST2700A0) {
		reg_val = SLI_RX_PHY_LAH_SEL_NEG | SLI_TRANS_EN | SLI_CLEAR_BUS;
		writel(reg_val, (void *)data->die1.slih + SLI_CTRL_I);
		writel(reg_val, (void *)data->die1.slim + SLI_CTRL_I);
		writel(reg_val | SLIV_RAW_MODE, (void *)data->die1.sliv + SLI_CTRL_I);
		sli_wait_suspend(data->die1.slih);
		sli_wait_suspend(data->die0.slih);
		debug("SLI US/DS @ 25MHz init done\n");

		/* AST2700A0 workaround to save SD waveform */
		sli_set_cpu_die_hpll();
		phyclk_lookup[5] = 788;

		if (data->die0.phy_clk_freq == SLI_PHYCLK_800M ||
		    data->die0.phy_clk_freq == SLI_PHYCLK_788M)
			data->flags |= SLI_FLAG_RX_LAH_NEG_IO_SLIM;
	}

	if (IS_ENABLED(CONFIG_SLI_TARGET_PHYCLK_25MHZ))
		return 0;

	/* Speed up engine clock before adjusting PHY TX clock and delay */
	reg_val = FIELD_PREP(SLI_CLK_SEL, data->die1.eng_clk_freq);
	clrsetbits_le32(data->die1.slih + SLI_CTRL_III, SLI_CLK_SEL, reg_val);
	reg_val = FIELD_PREP(SLI_CLK_SEL, data->die0.eng_clk_freq);
	clrsetbits_le32(data->die0.slih + SLI_CTRL_III, SLI_CLK_SEL, reg_val);

	/* Turn off auto-clear for AST2700A1 */
	if (!(data->flags & SLI_FLAG_AST2700A0)) {
		setbits_le32(data->die1.slih + SLI_CTRL_I,
			     SLI_AUTO_CLR_OFF_DAT | SLI_AUTO_CLR_OFF_CLK | SLI_NO_RST_TXCLK_CHG);
		setbits_le32(data->die0.slih + SLI_CTRL_I,
			     SLI_AUTO_CLR_OFF_DAT | SLI_AUTO_CLR_OFF_CLK | SLI_NO_RST_TXCLK_CHG);
	}

	/* Speed up CPU die PHY TX clock and clear TX PAD delay */
	reg_val = FIELD_PREP(SLI_PHYCLK_SEL, data->die0.phy_clk_freq);
	clrsetbits_le32(data->die0.slih + SLI_CTRL_III,
			SLI_PHYCLK_SEL | SLIH_PAD_DLY_TX1 | SLIH_PAD_DLY_TX0, reg_val);

	/* Turn off auto-training */
	setbits_le32(data->die1.slih + SLI_CTRL_I, SLI_AUTO_SEND_TRN_OFF);
	setbits_le32(data->die1.slim + SLI_CTRL_I, SLI_AUTO_SEND_TRN_OFF);
	setbits_le32(data->die1.sliv + SLI_CTRL_I, SLI_AUTO_SEND_TRN_OFF);

	/* Calibrate SLIH DS delay */
	sli_calibrate_ahb_delay(data);
	sli_calibrate_mbus_delay(data);

	debug("SLI DS @ %dMHz init done\n", phyclk_lookup[data->die0.phy_clk_freq]);
	return 0;
}

#define SCU1_SCRATCH31_SLI_READY	BIT(0)

int sli_init_r(void)
{
	struct ast2700_scu1 *scu;
	uint32_t reg_val;
	int retry = 10;
	bool sli0_ready = false;

	/*
	 * On AST2700A0, SLI0 RX calibration is handled by ATF. SPL does
	 * not need to wait for its completion.
	 */
	reg_val = readl((void *)ASPEED_IO_REVISION_ID);
	if (FIELD_GET(SCU_CPU_REVISION_ID_HW, reg_val) == 0)
		return 0;

	if (IS_ENABLED(CONFIG_SLI_TARGET_PHYCLK_25MHZ)) {
		debug("AST2700 SLI0 ready, 25MHz\n");
		return 0;
	}

	scu = (struct ast2700_scu1 *)ASPEED_IO_SCU_BASE;
	while (--retry > 0) {
		reg_val = readl((void *)&scu->scratch[31]);
		if (reg_val & SCU1_SCRATCH31_SLI_READY) {
			sli0_ready = true;
			break;
		}

		mdelay(100);
	}

	if (sli0_ready) {
		debug("SLI0 calibration completed\n");
		return 0;
	}

	debug("Timeout to wait SLI0 calibration\n");
	return -1;
}
