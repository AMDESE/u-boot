// SPDX-License-Identifier: GPL-2.0+
#include "sdram_ast2700.h"
#include <binman_sym.h>
#include <spl.h>

#define DWC_PHY_BIN_BASE_SPI	(0x20000000)
#define DWC_PHY_BIN_BASE_SRAM	(0x10000000)

#define DWC_PHY_IMEM_OFFSET	(0x50000)
#define DWC_PHY_DMEM_OFFSET	(0x58000)

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

struct train_bin {
	u32 imem_base;
	u32 imem_len;
	u32 dmem_base;
	u32 dmem_len;
};

struct train_bin dwc_train[DRAM_TYPE_MAX][2] = {
	/* sync with hw strap 0:ddr5, 1:ddr4 */
	{// one binary for ddr5 1d/2d together
		{
		//CONFIG_DDR5_PMU_TRAIN_IMEM_OFS, CONFIG_DDR5_PMU_TRAIN_IMEM_LEN,
		//CONFIG_DDR5_PMU_TRAIN_DMEM_OFS, CONFIG_DDR5_PMU_TRAIN_DMEM_LEN,
		0, 0, 0, 0,
		},
		{
		0, 0, 0, 0,
		},
	},
	{// two binary for ddr4 1d and 2d separatedly
		{
		//CONFIG_DDR4_PMU_TRAIN_IMEM_OFS, CONFIG_DDR4_PMU_TRAIN_IMEM_LEN,
		//CONFIG_DDR4_PMU_TRAIN_DMEM_OFS, CONFIG_DDR4_PMU_TRAIN_DMEM_LEN,
		0, 0, 0, 0,
		},
		{
		//CONFIG_DDR4_2D_PMU_TRAIN_IMEM_OFS, CONFIG_DDR4_2D_PMU_TRAIN_IMEM_LEN,
		//CONFIG_DDR4_2D_PMU_TRAIN_DMEM_OFS, CONFIG_DDR4_2D_PMU_TRAIN_DMEM_LEN,
		0, 0, 0, 0,
		},
	},
};

void dwc_get_mailbox(const int bit32_mode, uint32_t *mail)
{
	uint32_t ddrphy_val;

	/* 1. Poll the UctWriteProtShadow, looking for a 0 */
	while (1) {
		ddrphy_val = dwc_ddrphy_apb_rd(0xd0004);
		if (!(ddrphy_val & 0x1))
			break;
	}

	/* 2. When a 0 is seen, read the UctWriteOnlyShadow register to get the major message number. */
	*mail = dwc_ddrphy_apb_rd(0xd0032) & 0xffff;

	/* 3. If reading a streaming or SMBus message, also read the UctDatWriteOnlyShadow register. */
	if (bit32_mode)
		*mail += (dwc_ddrphy_apb_rd(0xd0034) & 0xffff) << 16;

	/* 4. Write the DctWriteProt to 0 to acknowledge the reception of the message */
	dwc_ddrphy_apb_wr(0xd0031, 0);

	/* 5. Poll the UctWriteProtShadow, looking for a 1 */
	while (1) {
		ddrphy_val = dwc_ddrphy_apb_rd(0xd0004);
		if (ddrphy_val & 0x1)
			break;
	};

	/* 6. When a 1 is seen, write the DctWriteProt to 1 to complete the protocol */
	dwc_ddrphy_apb_wr(0xd0031, 1);
}

void dwc_init_mailbox(void)
{
	dwc_ddrphy_apb_wr(0xd0031, 1);
	dwc_ddrphy_apb_wr(0xd0033, 1);
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

int dwc_ddrphy_phyinit_userCustom_H_readMsgBlock(int train2D)
{
	uint32_t  message;

	if (is_ddr4()) {
		if (train2D) {
			printf("%s: Read 2D message block by train2D = 1\n", __func__);

			/* uint16_t PmuRevision;  // Byte offset 0x02, CSR Addr 0x58001, Direction=Out */
			message = dwc_readMsgBlock(0x58001);
			printf("%s: PMU firmware revision ID (0x%x)\n", __func__, message);

			/* uint8_t CsTestFail;   // Byte offset 0x14, CSR Addr 0x5800a, Direction=Out */
			message = dwc_readMsgBlock(0x5800a);

			if ((message & 0xff) > 0) {
				printf("%s: Training Failure index (0x%x)\n", __func__, message);
			} else {
				message = dwc_readMsgBlock(0x58012);
				printf("%s:- <DWC_DDRPHY Message Block>: R0_RxClkDly_Margin=0x%x\n", __func__, (message & 0xff00) >> 8);

				message = dwc_readMsgBlock(0x58013);
				printf("%s:- <DWC_DDRPHY Message Block>: R0_VrefDac_Margin=0x%x\n", __func__, (message & 0xff));
				printf("%s:- <DWC_DDRPHY Message Block>: R0_VrefDac_Margin=0x%x\n", __func__, (message & 0xff00) >> 8);

				message = dwc_readMsgBlock(0x58014);
				printf("%s:- <DWC_DDRPHY Message Block>: R0_DeviceVref_Margin=0x%x\n", __func__, (message & 0xff));

				message = dwc_readMsgBlock(0x58016);
				printf("%s:- <DWC_DDRPHY Message Block>: MR1_RX_CTLE_CTRL0=0x%x\n", __func__, (message & 0xff));
				printf("%s:- <DWC_DDRPHY Message Block>: MR1_RX_CTLE_CTRL1=0x%x\n", __func__, (message & 0xff00) >> 8);

				message = dwc_readMsgBlock(0x58017);
				printf("%s:- <DWC_DDRPHY Message Block>: MR1_RX_CTLE_CTRL2=0x%x\n", __func__, (message & 0xff));
				printf("%s:- <DWC_DDRPHY Message Block>: MR1_RX_CTLE_CTRL3=0x%x\n", __func__, (message & 0xff00) >> 8);

				message = dwc_readMsgBlock(0x5801d);
				printf("%s:- <DWC_DDRPHY Message Block>: MR6_RX_CTLE_CTRL0=0x%x\n", __func__, (message & 0xff));
				printf("%s:- <DWC_DDRPHY Message Block>: MR6_RX_CTLE_CTRL1=0x%x\n", __func__, (message & 0xff00) >> 8);

				message = dwc_readMsgBlock(0x5801e);
				printf("%s:- <DWC_DDRPHY Message Block>: MR6_RX_CTLE_CTRL2=0x%x\n", __func__, (message & 0xff));
				printf("%s:- <DWC_DDRPHY Message Block>: MR6_RX_CTLE_CTRL3=0x%x\n", __func__, (message & 0xff00) >> 8);
				printf("%s: 2D Training Passed\n", __func__);
			}
		} else {  // train 1D
			printf("%s: Read 1D message block by train2D = 0\n", __func__);
			// 1. Check PMU revision
			message = dwc_readMsgBlock(0x58001);
			printf("%s: PMU firmware revision ID (0x%x)\n", __func__, message);

			// 2. Check pass / Failure of the training (CsTestFail)
			message = dwc_readMsgBlock(0x5800a);
			if ((message & 0xff) > 0)
				printf("%s: Training Failure index (0x%x)\n", __func__, message);
			else
				printf("%s: 1D Training Passed\n", __func__);
		}
	} else {
		printf("%s: Read 2D message block by train2D = 1\n", __func__);
		// 1. Check PMU revision
		// uint16_t PmuRevision;  // Byte offset 0x02, CSR Addr 0x58001, Direction=Out
		message = dwc_readMsgBlock(0x58001);
		printf("%s: PMU firmware revision ID (0x%x)\n", __func__, message);

		// 2. Check pass / Failure of the training (CsTestFail)
		// uint8_t CsTestFail;   // Byte offset 0x0f, CSR Addr 0x58007, Direction=Out
		message = dwc_readMsgBlock(0x58007);
		if (((message >> 8) & 0xff) > 0)
			printf("%s: Training Failure index (0x%x)\n", __func__, message);
		else
			printf("%s: DDR5 1D/2D Training Passed\n", __func__);

		// 3. Read ResultAddrOffset
		// uint16_t ResultAddrOffset; // Byte offset 0x14, CSR Addr 0x5800a, Direction=Out
		message = dwc_readMsgBlock(0x5800a);
		printf("%s: Result Address Offset (0x%x)\n", __func__, message);
	}

	return 0;
}

void dwc_ddrphy_phyinit_userCustom_A_bringupPower(void)
{
}

void dwc_ddrphy_phyinit_userCustom_B_startClockResetPhy(void)
{
	//// 1. Drive PwrOkIn to 0. Note: Reset, DfiClk, and APBCLK can be X.
	//// 2. Start DfiClk and APBCLK
	//// 3. Drive Reset to 1 and PRESETn_APB to 0.
	////	Note: The combination of PwrOkIn=0 and Reset=1 signals a cold reset to the PHY.
	writel(0x00020000, (void *)0x12c00014);//DRAMC_MCTL);
	//// 4. Wait a minimum of 8 cycles.
	udelay(2);
	//// 5. Drive PwrOkIn to 1. Once the PwrOkIn is asserted (and Reset is still asserted),
	////	DfiClk synchronously switches to any legal input frequency.
	writel(0x00030000, (void *)0x12c00014);//DRAMC_MCTL);
	//// 6. Wait a minimum of 64 cycles. Note: This is the reset period for the PHY.
	udelay(2);
	//// 7. Drive Reset to 0. Note: All DFI and APB inputs must be driven at valid reset states before the deassertion of Reset.
	writel(0x00010000, (void *)0x12c00014);//DRAMC_MCTL);
	//// 8. Wait a minimum of 1 Cycle.
	//// 9. Drive PRESETn_APB to 1 to de-assert reset on the ABP bus.
	////10. The PHY is now in the reset state and is ready to accept APB transactions.
	udelay(2);
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

void dwc_ddrphy_phyinit_userCustom_G_waitFwDone(void)
{
	uint32_t message = 0, mail;

	printf("dwc_ddrphy wait fw done\n");

	while (message != 0x07 && message != 0xff) {
		dwc_get_mailbox(0, &mail);
		message = mail & 0xffff;
	}

	printf("Firmware training process is complete!!!\n");
}

void dwc_ddrphy_phyinit_userCustom_J_enterMissionMode(void)
{
	uint32_t  value;
  //// 1. Set the PHY input clocks to the desired frequency.
  //// 2. Initialize the PHY to mission mode by performing DFI Initialization.
  ////	  Please see the DFI specification for more information. See the DFI frequency bus encoding in section <XXX>.
  //// Note: The PHY training firmware initializes the DRAM state. if skip
  //// training is used, the DRAM state is not initialized.
	writel(0xffffffff, (void *)0x12c0000c);//DRAMC_IRQMSK);

	value = readl((void *)0x12c00004);//DRAMC_IRQSTA);
	while (value) {
		value = readl((void *)0x12c00004);//DRAMC_IRQSTA);
	};

	writel(0x0, (void *)0x12c00040);//DRAMC_DFICFG); // [16] reset=0
	value = readl((void *)0x12c00014);//DRAMC_MCTL);
	value = value | 0x1;
	writel(value, (void *)0x12c00014);//DRAMC_MCTL); // [0] init_start

	value = readl((void *)0x12c00004);//DRAMC_IRQSTA);
	while ((value & 0x1) != 1) {
		value = readl((void *)0x12c00004);//DRAMC_IRQSTA);
	};
	writel(0x1, (void *)0x12c00008);//DRAMC_IRQCLR);

	value = readl((void *)0x12c00004);//DRAMC_IRQSTA);
	while (value) {
		value = readl((void *)0x12c00004);//DRAMC_IRQSTA);
	};
}

u32 spl_boot_list;
int dwc_ddrphy_phyinit_userCustom_D_loadIMEM(const int train2D)
{
	u32 i;
	u32 imem_base = DWC_PHY_IMEM_OFFSET;
	u32 *src;
	int type;
	int ret = 0;
	u32 blk, blks, target_base;
	struct mmc *mmc;
	struct blk_desc *bd;

	printf("%s %d\n", __func__, train2D);

	type = is_ddr4();

	if (spl_boot_device() == BOOT_DEVICE_RAM) {
		target_base = DWC_PHY_BIN_BASE_SPI;
	} else if (spl_boot_device() == BOOT_DEVICE_MMC1) {
		target_base = DWC_PHY_BIN_BASE_SRAM;

		printf("%s: mmc init device\n", __func__);
		ret = mmc_init_device(0);
		if (ret)
			printf("cannot init mmc\n");

		printf("%s: find mmc device\n", __func__);
		mmc = find_mmc_device(0);
		ret = mmc ? 0 : -ENODEV;
		if (ret)
			printf("cannot find mmc device\n");

		printf("mmc=0x%x\n", (u32)mmc);

		bd = mmc_get_blk_desc(mmc);
		printf("bd=0x%x\n", (u32)bd);

		ret = blk_dselect_hwpart(bd, 1);
		if (ret)
			printf("bd selet part fail\n");

		blk = dwc_train[type][train2D].imem_base / 512;
		dwc_train[type][train2D].imem_base %= 512;
		blks = dwc_train[type][train2D].imem_len / 512;

		printf("blk read blk=0x%x, blks=0x%x\n", blk, blks);
		ret = blk_dread(bd, blk, blks + 1, (void *)target_base);

		printf("blk read cnt=%d\n", ret);
	} else {
		printf("Unsupported Device!\n");
	}

	src = (u32 *)(dwc_train[type][train2D].imem_base + target_base);
	printf("imem target src = 0x%x\n", (u32)src);
	printf("imem target 1st dword = 0x%x\n", (u32)*src);
	printf("imem target len = 0x%x\n", dwc_train[type][train2D].imem_len);

	for (i = 0; i < dwc_train[type][train2D].imem_len / 4; i++)
		writel(*(src + i), (void *)DRAMC_PHY_BASE + 2 * (imem_base + 2 * i));

	return ret;
}

int dwc_ddrphy_phyinit_userCustom_F_loadDMEM(const int pState, const int train2D)
{
	u32 i;
	u32 dmem_base = DWC_PHY_DMEM_OFFSET;
	u32 *src;
	int type;
	int ret = 0;
	u32 blk, blks, target_base;
	struct mmc *mmc;
	struct blk_desc *bd;

	printf("%s %d\n", __func__, train2D);

	type = is_ddr4();

	if (spl_boot_device() == BOOT_DEVICE_RAM) {
		target_base = DWC_PHY_BIN_BASE_SPI;
	} else if (spl_boot_device() == BOOT_DEVICE_MMC1) {
		target_base = DWC_PHY_BIN_BASE_SRAM;

		printf("%s: mmc init device\n", __func__);
		ret = mmc_init_device(0);
		if (ret)
			printf("cannot init mmc\n");

		printf("%s: find mmc device\n", __func__);
		mmc = find_mmc_device(0);
		ret = mmc ? 0 : -ENODEV;
		if (ret)
			printf("cannot find mmc device\n");

		printf("mmc=0x%x\n", (u32)mmc);

		bd = mmc_get_blk_desc(mmc);
		printf("bd=0x%x\n", (u32)bd);

		ret = blk_dselect_hwpart(bd, 1);
		if (ret)
			printf("bd selet part fail\n");

		blk = dwc_train[type][train2D].dmem_base / 512;
		dwc_train[type][train2D].dmem_base %= 512;
		blks = dwc_train[type][train2D].dmem_len / 512;

		printf("blk read blk=0x%x, blks=0x%x\n", blk, blks);
		ret = blk_dread(bd, blk, blks + 1, (void *)target_base);

		printf("blk read cnt=%d\n", ret);
	} else {
		printf("Unsupported Device!\n");
	}

	src = (u32 *)(dwc_train[type][train2D].dmem_base + target_base);
	printf("dmem target src = 0x%x\n", (u32)src);
	printf("dmem target 1st dword = 0x%x\n", (u32)*src);
	printf("dmem target len = 0x%x\n", dwc_train[type][train2D].dmem_len);

	for (i = 0; i < dwc_train[type][train2D].dmem_len / 4; i++)
		writel(*(src + i), (void *)DRAMC_PHY_BASE + 2 * (dmem_base + 2 * i));

	return ret;
}

void dwc_phy_init(struct sdramc_ac_timing *ac)
{
	u32 ctx_start, imem_start, dmem_start, imem_2d_start, dmem_2d_start;
	u32 imem_len, dmem_len, imem_2d_len, dmem_2d_len;
	u32 ddr5_imem, ddr5_imem_len, ddr5_dmem, ddr5_dmem_len;
	u32 base = CONFIG_SPL_TEXT_BASE - 0x80;

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

	printf("SPL context base = 0x%x\n", ctx_start);

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

	printf("ddr4 1d imem base 0x%x\n", dwc_train[1][0].imem_base);
	printf("ddr4 1d imem size 0x%x\n", dwc_train[1][0].imem_len);
	printf("ddr4 1d dmem base 0x%x\n", dwc_train[1][0].dmem_base);
	printf("ddr4 1d dmem size 0x%x\n", dwc_train[1][0].dmem_len);
	printf("ddr4 2d imem base 0x%x\n", dwc_train[1][1].imem_base);
	printf("ddr4 2d imem size 0x%x\n", dwc_train[1][1].imem_len);
	printf("ddr4 2d dmem base 0x%x\n", dwc_train[1][1].dmem_base);
	printf("ddr4 2d dmem size 0x%x\n", dwc_train[1][1].dmem_len);
	printf("ddr5 imem base 0x%x\n", dwc_train[0][0].imem_base);
	printf("ddr5 imem size 0x%x\n", dwc_train[0][0].imem_len);
	printf("ddr5 dmem base 0x%x\n", dwc_train[0][0].dmem_base);
	printf("ddr5 dmem size 0x%x\n", dwc_train[0][0].dmem_len);

	if (IS_ENABLED(CONFIG_ASPEED_DDR_PHY_TRAINING)) {
		if (ac->type == DRAM_TYPE_4) {
			printf("%s: Starting ddr4 training\n", __func__);
			#include "dwc_ddrphy_phyinit_ddr4-3200-nodimm-train2D.c"
		} else if (ac->type == DRAM_TYPE_5) {
			printf("%s: Starting ddr5 training\n", __func__);
			#include "dwc_ddrphy_phyinit_ddr5-3200-nodimm-train2D.c"
		}
	}
//	else {
//printf("Skip ddr phy training procedure\n");
//		if (ac->type == DRAM_TYPE_4) {
//#include "dwc_ddrphy_phyinit_ddr4-3200-nodimm-skiptrain.c"
//		} else if (ac->type == DRAM_TYPE_5) {
//#include "dwc_ddrphy_phyinit_ddr5-3200-nodimm-skiptrain.c"
//		}
//	}
}
