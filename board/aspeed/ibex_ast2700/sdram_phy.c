// SPDX-License-Identifier: GPL-2.0+
#include <binman_sym.h>
#include <spl.h>
#include <asm/arch/fmc_hdr.h>
#include <asm/arch/sdram_ast2700.h>
#include <asm/arch/stor_ast2700.h>
#include <asm/arch/recovery.h>

#define DWC_PHY_IMEM_OFFSET	(0x50000)
#define DWC_PHY_DMEM_OFFSET	(0x58000)
#define SCU0_DDR_PHY_CLOCK	BIT(11)
#define SCU0_CLOCK_STOP_CLR_REG	(ASPEED_CPU_SCU_BASE + 0x244)

binman_sym_declare(u32, u_boot_spl_ddr, image_pos);

binman_sym_declare(u32, ddr4_1d_imem_fw, image_pos);
binman_sym_declare(u32, ddr4_1d_imem_fw, size);
binman_sym_declare(u32, ddr4_1d_dmem_fw, image_pos);
binman_sym_declare(u32, ddr4_1d_dmem_fw, size);
binman_sym_declare(u32, ddr4_2d_imem_fw, image_pos);
binman_sym_declare(u32, ddr4_2d_imem_fw, size);
binman_sym_declare(u32, ddr4_2d_dmem_fw, image_pos);
binman_sym_declare(u32, ddr4_2d_dmem_fw, size);
binman_sym_declare(u32, ddr5_imem_fw, image_pos);
binman_sym_declare(u32, ddr5_imem_fw, size);
binman_sym_declare(u32, ddr5_dmem_fw, image_pos);
binman_sym_declare(u32, ddr5_dmem_fw, size);

struct train_bin dwc_train[DRAM_TYPE_MAX][2] = {
	{{0, 0, 0, 0}, {0, 0, 0, 0}},
	{{0, 0, 0, 0}, {0, 0, 0, 0}},
};

void dwc_decode_streaming_message(void);
#define DWC_UCTWRITEPROTSHADOW		BIT(0)
#define DWC_UCTSHADOWREGS		(0xd0004)
#define DWC_DCTWRITEPROT		(0xd0031)
#define DWC_UCTWRITEONLYSHADOW		(0xd0032)
#define	DWC_UCTWRITEPROT		(0xc0033)
#define DWC_UCTDATWRITEONLYSHADOW	(0xd0034)

void dwc_get_mailbox(const int mode, uint32_t *mail)
{
	/* 1. Poll the UctWriteProtShadow, looking for a 0 */
	while (dwc_ddrphy_apb_rd(DWC_UCTSHADOWREGS) & DWC_UCTWRITEPROTSHADOW)
		;

	/* 2. When a 0 is seen, read the UctWriteOnlyShadow register to get the major message number. */
	*mail = dwc_ddrphy_apb_rd(DWC_UCTWRITEONLYSHADOW) & 0xffff;

	/* 3. If reading a streaming or SMBus message, also read the UctDatWriteOnlyShadow register. */
	if (mode)
		*mail |= ((dwc_ddrphy_apb_rd(DWC_UCTDATWRITEONLYSHADOW) & 0xffff) << 16);

	/* 4. Write the DctWriteProt to 0 to acknowledge the reception of the message */
	dwc_ddrphy_apb_wr(DWC_DCTWRITEPROT, 0);

	/* 5. Poll the UctWriteProtShadow, looking for a 1 */
	while (!(dwc_ddrphy_apb_rd(DWC_UCTSHADOWREGS) & DWC_UCTWRITEPROTSHADOW))
		;

	/* 6. When a 1 is seen, write the DctWriteProt to 1 to complete the protocol */
	dwc_ddrphy_apb_wr(DWC_DCTWRITEPROT, 1);
}

void dwc_init_mailbox(void)
{
	dwc_ddrphy_apb_wr(DWC_DCTWRITEPROT, 1);
	dwc_ddrphy_apb_wr(DWC_UCTWRITEPROT, 1);
}

uint32_t dwc_readMsgBlock(const uint32_t addr_half)
{
	uint32_t data_word;

	data_word = dwc_ddrphy_apb_rd_32b((addr_half >> 1) << 1);

	if (addr_half & 0x1)
		data_word = data_word >> 16;
	else
		data_word &= 0xffff;

	return data_word;
}

#define DWC_PHY_DDR4_MB_PMU_REV		(0x58001)
#define DWC_PHY_DDR4_MB_RESULT		(0x5800a)
#define DWC_PHY_DDR5_MB_PMU_REV		(0x58001)
#define DWC_PHY_DDR5_MB_RESULT		(0x58007)
#define DWC_PHY_DDR5_MB_RESULT_ADR	(0x5800a)

int dwc_ddrphy_phyinit_userCustom_H_readMsgBlock(int train2D)
{
	uint32_t  message;

	if (is_ddr4()) {
		/* 2. Check pass */
		message = dwc_readMsgBlock(DWC_PHY_DDR4_MB_RESULT);
		if (message & 0xff)
			debug("%s: Training Failure index (0x%x)\n", __func__, message);
		else
			debug("%s: %dD Training Passed\n", __func__, train2D ? 2 : 1);
	} else {
		/* 2. Check pass / Failure of the training (CsTestFail) */
		message = dwc_readMsgBlock(DWC_PHY_DDR5_MB_RESULT);
		if (message & 0xff00)
			debug("%s: Training Failure index (0x%x)\n", __func__, message);
		else
			debug("%s: DDR5 1D/2D Training Passed\n", __func__);

		/* 3. Read ResultAddrOffset */
		message = dwc_readMsgBlock(DWC_PHY_DDR5_MB_RESULT_ADR);
		debug("%s: Result Address Offset (0x%x)\n", __func__, message);
	}

	return 0;
}

void dwc_ddrphy_phyinit_userCustom_A_bringupPower(void)
{
}

void dwc_ddrphy_phyinit_userCustom_B_startClockResetPhy(struct sdramc *sdramc)
{
	struct sdramc_regs *regs = sdramc->regs;

	/*
	 * 1. Drive PwrOkIn to 0. Note: Reset, DfiClk, and APBCLK can be X.
	 * 2. Start DfiClk and APBCLK
	 * 3. Drive Reset to 1 and PRESETn_APB to 0.
	 * Note: The combination of PwrOkIn=0 and Reset=1 signals a cold reset to the PHY.
	 */
	writel(DRAMC_MCTL_PHY_RESET, (void *)&regs->mctl);
	udelay(2);

	/*
	 * 5. Drive PwrOkIn to 1. Once the PwrOkIn is asserted (and Reset is still asserted),
	 * DfiClk synchronously switches to any legal input frequency.
	 */
	writel(DRAMC_MCTL_PHY_RESET | DRAMC_MCTL_PHY_POWER_ON, (void *)&regs->mctl);
	udelay(2);

	/*
	 * 7. Drive Reset to 0. Note: All DFI and APB inputs must be driven at valid reset states
	 * before the deassertion of Reset.
	 */
	writel(DRAMC_MCTL_PHY_POWER_ON, (void *)&regs->mctl);
	udelay(2);

	/*
	 * 9. Drive PRESETn_APB to 1 to de-assert reset on the ABP bus.
	 * 10. The PHY is now in the reset state and is ready to accept APB transactions.
	 */
}

void dwc_ddrphy_phyinit_userCustom_overrideUserInput(void)
{
}

void dwc_ddrphy_phyinit_userCustom_customPostTrain(void)
{
}

void dwc_ddrphy_phyinit_userCustom_E_setDfiClk(int a)
{
	dwc_init_mailbox();
}

#if defined(CONFIG_ASPEED_PHY_TRAINING_MESSAGE)
void dwc_decode_streaming_message(void)
{
	u32 str, msg, msg2, count, i;

	dwc_get_mailbox(1, &msg);

	printf("\n");
	printf("Message:\n");
	printf("0x%x\n", msg);

	str = (msg & 0xffff0000) >> 16;
	count = msg & 0xffff;

	printf("Para:\n");
	for (i = 0; i < count; i++) {
		dwc_get_mailbox(1, &msg2);
		printf("0x%x ", msg2);
	}

	printf("\n");
}
#endif

#define DWC_PHY_MB_START_STREAM_MSG	0x8
#define DWC_PHY_MB_TRAIN_SUCCESS	0x7
#define DWC_PHY_MB_TRAIN_FAIL		0xff
void dwc_ddrphy_phyinit_userCustom_G_waitFwDone(void)
{
	uint32_t message = 0, mail;

	while (message != DWC_PHY_MB_TRAIN_SUCCESS && message != DWC_PHY_MB_TRAIN_FAIL) {
		dwc_get_mailbox(0, &mail);
		message = mail & 0xffff;

		if (IS_ENABLED(CONFIG_ASPEED_PHY_TRAINING_MESSAGE)) {
			if (message == DWC_PHY_MB_START_STREAM_MSG)
				dwc_decode_streaming_message();
		}
	}
}

void dwc_ddrphy_phyinit_userCustom_J_enterMissionMode(struct sdramc *sdramc)
{
	struct sdramc_regs *regs = sdramc->regs;
	uint32_t val;

	/*
	 * 1. Set the PHY input clocks to the desired frequency.
	 * 2. Initialize the PHY to mission mode by performing DFI Initialization.
	 * Please see the DFI specification for more information. See the DFI frequency bus encoding in section <XXX>.
	 * Note: The PHY training firmware initializes the DRAM state. if skip
	 * training is used, the DRAM state is not initialized.
	 */

	writel(0xffffffff, (void *)&regs->intr_mask);

	writel(0x0, (void *)&regs->dcfg); // [16] reset=0

	if (!is_ddr4()) {
		dwc_ddrphy_apb_wr(0xd0000, 0);		// DWC_DDRPHYA_APBONLY0_MicroContMuxSel
		dwc_ddrphy_apb_wr(0x20240, 0x3900);	// DWC_DDRPHYA_MASTER0_base0_D5ACSMPtr0lat0
		dwc_ddrphy_apb_wr(0x900da, 8);		// DWC_DDRPHYA_INITENG0_base0_SequenceReg0b59s0
		dwc_ddrphy_apb_wr(0xd0000, 1);		// DWC_DDRPHYA_APBONLY0_MicroContMuxSel
	}

	/* phy init start */
	val = readl((void *)&regs->mctl);
	val = val | DRAMC_MCTL_PHY_INIT_START;
	writel(val, (void *)&regs->mctl);

	/* wait phy complete */
	while ((readl((void *)&regs->intr_status) & DRAMC_IRQSTA_PHY_INIT_DONE) != DRAMC_IRQSTA_PHY_INIT_DONE)
		;

	writel(0xffff, (void *)&regs->intr_clear);

	while (readl((void *)&regs->intr_status))
		;

	if (!is_ddr4()) {
		dwc_ddrphy_apb_wr(0xd0000, 0);		// DWC_DDRPHYA_APBONLY0_MicroContMuxSel
		dwc_ddrphy_apb_wr(0x20240, 0x4300);	// DWC_DDRPHYA_MASTER0_base0_D5ACSMPtr0lat0
		dwc_ddrphy_apb_wr(0x900da, 0);		// DWC_DDRPHYA_INITENG0_base0_SequenceReg0b59s0
		dwc_ddrphy_apb_wr(0xd0000, 1);		// DWC_DDRPHYA_APBONLY0_MicroContMuxSel
	}
}

int dwc_ddrphy_phyinit_userCustom_D_loadIMEM(const int train2D)
{
	u32 imem_base = DWC_PHY_IMEM_OFFSET;
	int type;
	int ret = 0;

	debug("%s %d\n", __func__, train2D);

	type = is_ddr4();

	if (is_recovery()) {
		aspeed_spl_ddr_image_ymodem_load(dwc_train, type,
						 1, train2D);
		memcpy((void *)(DRAMC_PHY_BASE + 2 * imem_base),
		       (void *)dwc_train[type][train2D].imem_base,
		       dwc_train[type][train2D].imem_len);
		return ret;
	}

	stor_copy((u32 *)dwc_train[type][train2D].imem_base,
		  (u32 *)(DRAMC_PHY_BASE + 2 * imem_base),
		  dwc_train[type][train2D].imem_len);

	return ret;
}

int dwc_ddrphy_phyinit_userCustom_F_loadDMEM(const int pState, const int train2D)
{
	u32 dmem_base = DWC_PHY_DMEM_OFFSET;
	int type;
	int ret = 0;

	debug("%s %d\n", __func__, train2D);

	type = is_ddr4();

	if (is_recovery()) {
		aspeed_spl_ddr_image_ymodem_load(dwc_train, type,
						 0, train2D);
		memcpy((void *)(DRAMC_PHY_BASE + 2 * dmem_base),
		       (void *)dwc_train[type][train2D].dmem_base,
		       dwc_train[type][train2D].dmem_len);
		return ret;
	}

	stor_copy((u32 *)dwc_train[type][train2D].dmem_base,
		  (u32 *)(DRAMC_PHY_BASE + 2 * dmem_base),
		  dwc_train[type][train2D].dmem_len);

	return ret;
}

void dwc_phy_init(struct sdramc *sdramc)
{
	u32 ctx_start, imem_start, dmem_start, imem_2d_start, dmem_2d_start;
	u32 imem_len, dmem_len, imem_2d_len, dmem_2d_len;
	u32 ddr5_imem, ddr5_imem_len, ddr5_dmem, ddr5_dmem_len;
	u32 base = CONFIG_SPL_TEXT_BASE;

	imem_start = binman_sym(u32, ddr4_1d_imem_fw, image_pos);
	imem_len = binman_sym(u32, ddr4_1d_imem_fw, size);
	dmem_start = binman_sym(u32, ddr4_1d_dmem_fw, image_pos);
	dmem_len = binman_sym(u32, ddr4_1d_dmem_fw, size);
	imem_2d_start = binman_sym(u32, ddr4_2d_imem_fw, image_pos);
	imem_2d_len = binman_sym(u32, ddr4_2d_imem_fw, size);
	dmem_2d_start = binman_sym(u32, ddr4_2d_dmem_fw, image_pos);
	dmem_2d_len = binman_sym(u32, ddr4_2d_dmem_fw, size);
	ddr5_imem = binman_sym(u32, ddr5_imem_fw, image_pos);
	ddr5_imem_len = binman_sym(u32, ddr5_imem_fw, size);
	ddr5_dmem = binman_sym(u32, ddr5_dmem_fw, image_pos);
	ddr5_dmem_len = binman_sym(u32, ddr5_dmem_fw, size);

	if (BINMAN_SYMS_OK) {
		ctx_start = binman_sym(u32, u_boot_spl_ddr, image_pos);
		imem_start = binman_sym(u32, ddr4_1d_imem_fw, image_pos);
		imem_len = binman_sym(u32, ddr4_1d_imem_fw, size);
		dmem_start = binman_sym(u32, ddr4_1d_dmem_fw, image_pos);
		dmem_len = binman_sym(u32, ddr4_1d_dmem_fw, size);
		imem_2d_start = binman_sym(u32, ddr4_2d_imem_fw, image_pos);
		imem_2d_len = binman_sym(u32, ddr4_2d_imem_fw, size);
		dmem_2d_start = binman_sym(u32, ddr4_2d_dmem_fw, image_pos);
		dmem_2d_len = binman_sym(u32, ddr4_2d_dmem_fw, size);
		ddr5_imem = binman_sym(u32, ddr5_imem_fw, image_pos);
		ddr5_imem_len = binman_sym(u32, ddr5_imem_fw, size);
		ddr5_dmem = binman_sym(u32, ddr5_dmem_fw, image_pos);
		ddr5_dmem_len = binman_sym(u32, ddr5_dmem_fw, size);

		// ddr5
		dwc_train[0][0].imem_base = ddr5_imem - base;
		dwc_train[0][0].imem_len = ddr5_imem_len;
		dwc_train[0][0].dmem_base = ddr5_dmem - base;
		dwc_train[0][0].dmem_len = ddr5_dmem_len;

		// ddr4 1d
		dwc_train[1][0].imem_base = imem_start - base;
		dwc_train[1][0].imem_len = imem_len;
		dwc_train[1][0].dmem_base = dmem_start - base;
		dwc_train[1][0].dmem_len = dmem_len;

		// ddr4 2d
		dwc_train[1][1].imem_base = imem_2d_start - base;
		dwc_train[1][1].imem_len = imem_2d_len;
		dwc_train[1][1].dmem_base = dmem_2d_start - base;
		dwc_train[1][1].dmem_len = dmem_2d_len;
	} else {
		fmc_hdr_get_prebuilt(PBT_DDR4_PMU_TRAIN_IMEM, &imem_start, &imem_len, NULL);
		fmc_hdr_get_prebuilt(PBT_DDR4_PMU_TRAIN_DMEM, &dmem_start, &dmem_len, NULL);
		fmc_hdr_get_prebuilt(PBT_DDR4_2D_PMU_TRAIN_IMEM, &imem_2d_start, &imem_2d_len, NULL);
		fmc_hdr_get_prebuilt(PBT_DDR4_2D_PMU_TRAIN_DMEM, &dmem_2d_start, &dmem_2d_len, NULL);
		fmc_hdr_get_prebuilt(PBT_DDR5_PMU_TRAIN_IMEM, &ddr5_imem, &ddr5_imem_len, NULL);
		fmc_hdr_get_prebuilt(PBT_DDR5_PMU_TRAIN_DMEM, &ddr5_dmem, &ddr5_dmem_len, NULL);

		// ddr5
		dwc_train[0][0].imem_base = ddr5_imem;
		dwc_train[0][0].imem_len = ddr5_imem_len;
		dwc_train[0][0].dmem_base = ddr5_dmem;
		dwc_train[0][0].dmem_len = ddr5_dmem_len;

		// ddr4 1d
		dwc_train[1][0].imem_base = imem_start;
		dwc_train[1][0].imem_len = imem_len;
		dwc_train[1][0].dmem_base = dmem_start;
		dwc_train[1][0].dmem_len = dmem_len;

		// ddr4 2d
		dwc_train[1][1].imem_base = imem_2d_start;
		dwc_train[1][1].imem_len = imem_2d_len;
		dwc_train[1][1].dmem_base = dmem_2d_start;
		dwc_train[1][1].dmem_len = dmem_2d_len;
	}

	// enable ddrphy free-run clock
	writel(SCU0_DDR_PHY_CLOCK, (void *)SCU0_CLOCK_STOP_CLR_REG);

	if (is_ddr4()) {
		debug("%s: Starting ddr4 training\n", __func__);
		#include "dwc_ddrphy_phyinit_ddr4-3200-nodimm-train2D.c"
	} else {
		debug("%s: Starting ddr5 training\n", __func__);
		#include "dwc_ddrphy_phyinit_ddr5-3200-nodimm-train2D.c"
	}
}
