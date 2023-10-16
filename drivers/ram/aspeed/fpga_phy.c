// SPDX-License-Identifier: GPL-2.0+
#include "sdram_ast2700.h"

#define SCU_CPU_PINMUX1                 (SCU_CPU_REG + 0x400)

void fpga_phy_init(struct sdramc *sdramc)
{
	struct sdramc_regs *regs = sdramc->regs;

	/* adjust CLK4RX delay */
	writel(0x10f, (void *)SCU_CPU_PINMUX1);
	writel(0x30f, (void *)SCU_CPU_PINMUX1);

	/* adjust DQS window */
	writel(0x18, &regs->testcfg);

	/* assert DFI reset */
	writel(DRAMC_DFICFG_RESET | DRAMC_DFICFG_WD_POL, &regs->dcfg);

	mdelay(1);

	/* power up control (switch power FSM) */
	writel(DRAMC_MCTL_PHY_RESET, &regs->mctl);
	writel(DRAMC_MCTL_PHY_RESET | DRAMC_MCTL_PHY_POWER_ON, &regs->mctl);
	writel(DRAMC_MCTL_PHY_POWER_ON, &regs->mctl);

	mdelay(1);

	/* de-assert DFI reset */
	clrbits(le32, &regs->dcfg, DRAMC_DFICFG_RESET);

	mdelay(1);

	/* DFI start */
	setbits(le32, &regs->mctl, DRAMC_MCTL_PHY_INIT_START);

	/* query phy init done */
	while (!(readl(&regs->intr_status) & DRAMC_IRQSTA_PHY_INIT_DONE))
		;

	writel(DRAMC_IRQSTA_PHY_INIT_DONE, &regs->intr_clear);
}

#if defined(ASPEED_FPGA_DDR_CALI)
int fpga_dq_shift_cali(void)
{
	int i, err, dq[128 + 16], dq_left_0, dq_right_0;
	uint32_t shift_val = 0x100;
	uint32_t shift_en = (1 << 9);
	uint32_t bistcfg;
	int flag = 0, found = 0;

	writel(0x0f, SCU_CPU_PINMUX1);
	writel(0x20f, SCU_CPU_PINMUX1);

	bistcfg = FIELD_PREP(DRAMC_BISTCFG_PMODE, BIST_PMODE_CRC)
		| FIELD_PREP(DRAMC_BISTCFG_BMODE, BIST_BMODE_RW_SWITCH)
		| DRAMC_BISTCFG_ENABLE;
	dramc_bist(0, 0x10000, bistcfg, 200);

	writel(0x0f, SCU_CPU_PINMUX1);
	writel(0x20f, SCU_CPU_PINMUX1);

	for (i = 0; i < 128 + 16; i++) {
		writel(shift_val, SCU_CPU_PINMUX1);
		writel(shift_en | shift_val, SCU_CPU_PINMUX1);

		err = dramc_bist(0, 0x1000000, bistcfg, 200);
		if (err) {
			printf("0");
			dq[i] = 0;
		} else {
			printf("1");
			dq[i] = 1;
			found = 1;
		}
	}

	printf("\n");
	if (!found) {
		printf("No window found !!!\n");
		return 1;
	}

	for (i = 0; i < 128 + 16; i++) {
		if (dq[i] && !flag) {
			printf("left=%d\n", i);
			dq_left_0 = i;
			flag = 1;
		} else if (!dq[i] && flag) {
			printf("right=%d\n", i);
			dq_right_0 = i;
			flag = 0;
		}
	}

	printf("dq left delay=%d\n", dq_left_0 - 16 + 1);
	printf("dq right delay=%d\n", dq_right_0 - 16 + 1);
	printf("dq center delay= %d\n", ((dq_right_0 + dq_left_0) / 2) - 16 + 1);
	printf("\n");

	return 0;
}
#endif
