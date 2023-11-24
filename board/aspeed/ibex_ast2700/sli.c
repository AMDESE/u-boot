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
#include <asm/arch-aspeed/scu_ast2700.h>
#include <asm/arch-aspeed/sli_ast2700.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/delay.h>

static int sli_wait(uint32_t base, uint32_t mask)
{
	uint32_t value;

	writel(0xffffffff, (void *)base + SLI_INTR_STATUS);

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

static void sli_calibrate_ahb_delay(int config)
{
	uint32_t value;
	int d0, d1, i;
	int d0_start = -1;
	int d0_end = -1;
	int *buf, *win_s, *win_e;
	int latch_sel = 0;

	if (config)
		latch_sel = SLI_RX_PHY_LAH_SEL_NEG;

	buf = malloc(sizeof(int) * 32 * 2);
	if (!buf) {
		printf("ERROR in SLI init. Failed to allocate memory\n");
		return;
	}

	memset(buf, -1, sizeof(int) * 32 * 2);
	win_s = &buf[0];
	win_e = &buf[32];

	/* calibrate IOD-SLIH DS pad delay */
	for (d0 = 6; d0 < 20; d0++) {
		for (d1 = 6; d1 < 20; d1++) {
			/* set IOD SLIH DS (RX) delay */
			value = readl((void *)SLIH_IOD_BASE + SLI_CTRL_III);
			value &= ~(SLIH_PAD_DLY_RX1 | SLIH_PAD_DLY_RX0);
			value |= FIELD_PREP(SLIH_PAD_DLY_RX1, d1) |
				 FIELD_PREP(SLIH_PAD_DLY_RX0, d0);
			writel(value, (void *)SLIH_IOD_BASE + SLI_CTRL_III);

			writel(latch_sel | SLI_TRANS_EN | SLI_CLEAR_RX,
			       (void *)SLIH_IOD_BASE + SLI_CTRL_I);
			udelay(100);

			/* check interrupt status */
			writel(0xffffffff,
			       (void *)SLIH_IOD_BASE + SLI_INTR_STATUS);
			udelay(10);
			if (is_sli_suspend(SLIH_IOD_BASE) > 0) {
				if (win_s[d0] == -1)
					win_s[d0] = d1;

				win_e[d0] = d1;
			} else if (win_e[d0] != -1) {
				break;
			}
		}
	}

	d0_start = -1;
	d0_end = -1;
	for (i = 0; i < 32; i++) {
		if (win_s[i] != -1) {
			if (d0_start == -1)
				d0_start = i;

			d0_end = i;
		} else if (d0_end != -1) {
			break;
		}
	}

	if (d0_start == -1) {
		d0 = SLIH_DEFAULT_DELAY;
		d1 = SLIH_DEFAULT_DELAY;
	} else {
		d0 = (d0_start + d0_end) >> 1;
		d1 = (win_s[d0] + win_e[d0]) >> 1;
	}

	printf("IOD SLIH[0] DS win: {%d, %d} -> select %d\n", d0_start, d0_end, d0);
	printf("IOD SLIH[1] DS win: {%d, %d} -> select %d\n", win_s[d0], win_e[d0], d1);

	/* Load the calibrated delay values */
	writel(latch_sel | SLI_AUTO_SEND_TRN_OFF | SLI_TRANS_EN,
	       (void *)SLIH_IOD_BASE + SLI_CTRL_I);
	value = readl((void *)SLIH_IOD_BASE + SLI_CTRL_III);
	value &= ~(SLIH_PAD_DLY_RX1 | SLIH_PAD_DLY_RX0);
	value |= FIELD_PREP(SLIH_PAD_DLY_RX1, d1) |
		 FIELD_PREP(SLIH_PAD_DLY_RX0, d0);
	writel(value, (void *)SLIH_IOD_BASE + SLI_CTRL_III);
	udelay(1);

	/* Reset IOD SLIH Bus (to reset the counters) and RX */
	writel(latch_sel | SLI_CLEAR_BUS | SLI_TRANS_EN | SLI_CLEAR_RX,
	       (void *)SLIH_IOD_BASE + SLI_CTRL_I);
	sli_wait_suspend(SLIH_IOD_BASE);

	free(buf);
}

static int sli_calibrate_mbus_pad_delay(int config, int index, int begin,
					int end)
{
	uint32_t value;
	uint32_t mask = 0x3f << (index * 6);
	int d, latch_sel = 0;
	int win[2] = { -1, -1 };

	if (config)
		latch_sel = SLI_RX_PHY_LAH_SEL_NEG;

	for (d = begin; d < end; d++) {
		value = readl((void *)SLIM_IOD_BASE + SLI_CTRL_III);
		value &= ~mask;
		value |= (d << (index * 6));
		writel(value, (void *)SLIM_IOD_BASE + SLI_CTRL_III);
		udelay(1);

		/* Reset CPU TX */
		writel(SLI_TRANS_EN | SLI_CLEAR_TX,
		       (void *)SLIM_CPU_BASE + SLI_CTRL_I);
		udelay(1);

		/* Reset IOD RX */
		writel(latch_sel | SLI_TRANS_EN | SLI_CLEAR_RX,
		       (void *)SLIM_IOD_BASE + SLI_CTRL_I);
		udelay(1);

		/* Check interrupt status */
		writel(0xffffffff, (void *)SLIM_IOD_BASE + SLI_INTR_STATUS);
		udelay(10);
		if (is_sli_suspend(SLIM_IOD_BASE) > 0) {
			if (win[0] == -1)
				win[0] = d;

			win[1] = d;
		} else if (win[1] != -1) {
			break;
		}
	}

	if (win[0] == -1)
		d = SLIM_DEFAULT_DELAY;
	else
		d = (win[0] + win[1]) >> 1;

	printf("IOD SLIM[%d] DS win: {%d, %d} -> select %d\n", index, win[0],
	       win[1], d);

	return d;
}

static void sli_calibrate_mbus_delay(int config)
{
	uint32_t value;
	int dc, d0, d1, d2, d3;
	int begin, end;
	int win[2] = { -1, -1 };
	int latch_sel = 0;

	if (config)
		latch_sel = SLI_RX_PHY_LAH_SEL_NEG;

	/* Find coarse delay */
	for (dc = 0; dc < 16; dc++) {
		value = readl((void *)SLIM_IOD_BASE + SLI_CTRL_III);
		value &= ~(SLIM_PAD_DLY_RX3 | SLIM_PAD_DLY_RX2 |
			   SLIM_PAD_DLY_RX1 | SLIM_PAD_DLY_RX0);
		value |= FIELD_PREP(SLIM_PAD_DLY_RX3, dc) |
			 FIELD_PREP(SLIM_PAD_DLY_RX2, dc) |
			 FIELD_PREP(SLIM_PAD_DLY_RX1, dc) |
			 FIELD_PREP(SLIM_PAD_DLY_RX0, dc);
		writel(value, (void *)SLIM_IOD_BASE + SLI_CTRL_III);
		udelay(1);

		/* Reset CPU TX */
		writel(SLI_TRANS_EN | SLI_CLEAR_TX,
		       (void *)SLIM_CPU_BASE + SLI_CTRL_I);
		udelay(1);

		/* Reset IOD RX */
		writel(latch_sel | SLI_TRANS_EN | SLI_CLEAR_RX,
		       (void *)SLIM_IOD_BASE + SLI_CTRL_I);
		udelay(1);

		/* Check interrupt status */
		writel(0xffffffff, (void *)SLIM_IOD_BASE + SLI_INTR_STATUS);
		udelay(10);
		if (is_sli_suspend(SLIM_IOD_BASE) > 0) {
			if (win[0] == -1)
				win[0] = dc;

			win[1] = dc;
		} else if (win[1] != -1) {
			break;
		}
	}

	if (win[0] < 0)
		dc = SLIM_DEFAULT_DELAY;
	else
		dc = (win[0] + win[1]) >> 1;

	printf("IOD SLIM DS coarse win: {%d, %d} -> select %d\n", win[0],
	       win[1], dc);

	value = readl((void *)SLIM_IOD_BASE + SLI_CTRL_III);
	value &= ~(SLIM_PAD_DLY_RX3 | SLIM_PAD_DLY_RX2 | SLIM_PAD_DLY_RX1 |
		   SLIM_PAD_DLY_RX0);
	value |= FIELD_PREP(SLIM_PAD_DLY_RX3, dc) |
		 FIELD_PREP(SLIM_PAD_DLY_RX2, dc) |
		 FIELD_PREP(SLIM_PAD_DLY_RX1, dc) |
		 FIELD_PREP(SLIM_PAD_DLY_RX0, dc);
	writel(value, (void *)SLIM_IOD_BASE + SLI_CTRL_III);
	udelay(1);

	begin = max(dc - 5, 0);
	end = min(dc + 5, 32);

	/* Fine-tune per-PAD delay */
	d0 = sli_calibrate_mbus_pad_delay(config, 0, begin, end);
	if (d0 < 0)
		d0 = SLIM_DEFAULT_DELAY;
	value = readl((void *)SLIM_IOD_BASE + SLI_CTRL_III);
	value &= ~SLIM_PAD_DLY_RX0;
	value |= FIELD_PREP(SLIM_PAD_DLY_RX0, d0);
	writel(value, (void *)SLIM_IOD_BASE + SLI_CTRL_III);

	d1 = sli_calibrate_mbus_pad_delay(config, 1, begin, end);
	if (d1 < 0)
		d1 = SLIM_DEFAULT_DELAY;
	value = readl((void *)SLIM_IOD_BASE + SLI_CTRL_III);
	value &= ~SLIM_PAD_DLY_RX1;
	value |= FIELD_PREP(SLIM_PAD_DLY_RX1, d1);
	writel(value, (void *)SLIM_IOD_BASE + SLI_CTRL_III);

	d2 = sli_calibrate_mbus_pad_delay(config, 2, begin, end);
	if (d2 < 0)
		d2 = SLIM_DEFAULT_DELAY;
	value = readl((void *)SLIM_IOD_BASE + SLI_CTRL_III);
	value &= ~SLIM_PAD_DLY_RX2;
	value |= FIELD_PREP(SLIM_PAD_DLY_RX2, d2);
	writel(value, (void *)SLIM_IOD_BASE + SLI_CTRL_III);

	d3 = sli_calibrate_mbus_pad_delay(config, 3, begin, end);
	if (d3 < 0)
		d3 = SLIM_DEFAULT_DELAY;
	value = readl((void *)SLIM_IOD_BASE + SLI_CTRL_III);
	value &= ~SLIM_PAD_DLY_RX3;
	value |= FIELD_PREP(SLIM_PAD_DLY_RX3, d3);
	writel(value, (void *)SLIM_IOD_BASE + SLI_CTRL_III);
	udelay(1);

	/* Reset CPU SLIM TX */
	writel(SLI_TRANS_EN | SLI_CLEAR_TX, (void *)SLIM_CPU_BASE + SLI_CTRL_I);
	udelay(1);

	/* Reset IOD SLIM Bus (to reset the counters) and RX */
	writel(latch_sel | SLI_CLEAR_BUS | SLI_TRANS_EN | SLI_CLEAR_RX,
	       (void *)SLIM_IOD_BASE + SLI_CTRL_I);
	sli_wait_suspend(SLIM_IOD_BASE);

	/* Enable the MARB RR mode for AST2700A0 */
	value = readl((void *)SLIM_IOD_BASE + SLIM_MARB_FUNC_I);
	value |= SLIM_SLI_MARB_RR;
	writel(value, (void *)SLIM_IOD_BASE + SLIM_MARB_FUNC_I);
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
	const int phyclk_lookup[8] = {
		25, 800, 400, 200, 2000, 1000, 500, 250,
	};

	if (IS_ENABLED(CONFIG_ASPEED_FPGA))
		return 0;

	/* Return if SLI had been calibrated */
	value = readl((void *)SLIH_IOD_BASE + SLI_CTRL_III);
	value = FIELD_GET(SLI_CLK_SEL, value);
	if (value) {
		printf("SLI has been initialized\n");
		return 0;
	}

	/* 25MHz PAD delay for AST2700A0 */
	value = SLI_RX_PHY_LAH_SEL_NEG | SLI_TRANS_EN | SLI_CLEAR_BUS;
	writel(value, (void *)SLIH_IOD_BASE + SLI_CTRL_I);
	writel(value, (void *)SLIM_IOD_BASE + SLI_CTRL_I);
	writel(value | SLIV_RAW_MODE, (void *)SLIV_IOD_BASE + SLI_CTRL_I);
	sli_wait_suspend(SLIH_IOD_BASE);
	sli_wait_suspend(SLIH_CPU_BASE);
	printf("SLI US/DS @ 25MHz init done\n");

	/* IOD SLIM/H/V training off */
	value = SLI_RX_PHY_LAH_SEL_NEG | SLI_TRANS_EN | SLI_AUTO_SEND_TRN_OFF;
	writel(value, (void *)SLIH_IOD_BASE + SLI_CTRL_I);
	writel(value, (void *)SLIM_IOD_BASE + SLI_CTRL_I);
	writel(value | SLIV_RAW_MODE, (void *)SLIV_IOD_BASE + SLI_CTRL_I);

	/* Change IOD SLIH engine clock to 500M */
	value = FIELD_PREP(SLI_CLK_SEL, SLI_CLK_500M);
	writel(value, (void *)SLIH_IOD_BASE + SLI_CTRL_III);
	writel(value, (void *)SLIH_CPU_BASE + SLI_CTRL_III);
	sli_wait_suspend(SLIH_IOD_BASE);
	sli_wait_suspend(SLIH_CPU_BASE);

	/* Speed up the CPU SLIH PHY clock. Don't access CPU-die from now on */
	value |= FIELD_PREP(SLI_PHYCLK_SEL, SLI_TARGET_PHYCLK);
	writel(value, (void *)SLIH_CPU_BASE + SLI_CTRL_III);
	mdelay(10);

	/* Calibrate SLIH DS delay */
	sli_calibrate_ahb_delay(0);

	/* It's okay to access CPU-die now. Calibrate SLIM DS delay */
	sli_calibrate_mbus_delay(SLIM_LAH_CONFIG);

	printf("SLI DS @ %dMHz init done\n", phyclk_lookup[SLI_TARGET_PHYCLK]);
	return 0;
}
