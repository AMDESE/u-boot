// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2024 Aspeed Technology Inc.
 */

#include <asm/arch/platform.h>
#include <asm/arch/sdram_ast2700.h>
#include <asm/io.h>
#include <asm/u-boot.h>
#include <common.h>
#include <log.h>
#include <spl.h>
#include <xyzModem.h>

#define BUF_SIZE	(1024)
#define IMAGE_MAX_SZ	(64 * 1024)
#define SCU1_HWSTRAP1	(ASPEED_IO_SCU_BASE + 0x010)
#define   RECOVERY_INTERFACE	GENMASK(27, 26)
#define   RECOVERY_BOOT_EN	BIT(4)

bool is_recovery(void)
{
	u32 recovery_pin = readl((void *)(SCU1_HWSTRAP1));

	return (recovery_pin & RECOVERY_BOOT_EN) ? true : false;
}

bool recovery_from_uart(void)
{
	u32 recovery_mode = readl((void *)(SCU1_HWSTRAP1));

	if (!(recovery_mode & RECOVERY_INTERFACE))
		return true;

	return false;
}

static int getcymodem(void)
{
	if (tstc())
		return (getchar());
	return -1;
}

int aspeed_spl_ymodem_image_load(u32 addr)
{
	ulong size = 0;
	int err;
	int res;
	int ret;
	connection_info_t info;
	uchar *buf = (uchar *)((u32)ASPEED_SRAM_BASE + addr);

	info.mode = xyzModem_ymodem;
	ret = xyzModem_stream_open(&info, &err);
	if (ret) {
		printf("spl: ymodem err - %s\n", xyzModem_error(err));
		return -1;
	}

	while ((res = xyzModem_stream_read(buf, BUF_SIZE, &err)) > 0) {
		size += res;
		buf += res;
	}

	xyzModem_stream_close(&err);
	xyzModem_stream_terminate(false, &getcymodem);

	printf("Loaded %lu bytes\n", size);

	return (int)size;
}

void aspeed_spl_ddr_image_ymodem_load(struct train_bin dwc_train[][2], int ddr4,
				      int i_mem, const int train2D)
{
	if (ddr4 == 1 && i_mem == 1 && train2D == 0) {
		printf("Please send \"ddr4_pmu_train_imem.bin\" through Ymodem.\n");
		aspeed_spl_ymodem_image_load(0);
		dwc_train[ddr4][train2D].imem_base = ASPEED_SRAM_BASE;
	} else if (ddr4 == 1 && i_mem == 0 && train2D == 0) {
		printf("Please send \"ddr4_pmu_train_dmem.bin\" through Ymodem.\n");
		aspeed_spl_ymodem_image_load(IMAGE_MAX_SZ);
		dwc_train[ddr4][train2D].dmem_base = ASPEED_SRAM_BASE + IMAGE_MAX_SZ;
	} else if (ddr4 == 1 && i_mem == 1 && train2D == 1) {
		printf("Please send \"ddr4_2d_pmu_train_imem.bin\" through Ymodem.\n");
		aspeed_spl_ymodem_image_load(0);
		dwc_train[ddr4][train2D].imem_base = ASPEED_SRAM_BASE;
	} else if (ddr4 == 1 && i_mem == 0 && train2D == 1) {
		printf("Please send \"ddr4_2d_pmu_train_dmem.bin\" through Ymodem.\n");
		aspeed_spl_ymodem_image_load(IMAGE_MAX_SZ);
		dwc_train[ddr4][train2D].dmem_base = ASPEED_SRAM_BASE + IMAGE_MAX_SZ;
	} else if (ddr4 == 0 && i_mem == 1) {
		printf("Please send \"ddr5_pmu_train_imem.bin\" through Ymodem.\n");
		aspeed_spl_ymodem_image_load(0);
		dwc_train[ddr4][0].imem_base = ASPEED_SRAM_BASE;
	} else if (ddr4 == 0 && i_mem == 0) {
		printf("Please send \"ddr5_pmu_train_dmem.bin\" through Ymodem.\n");
		aspeed_spl_ymodem_image_load(IMAGE_MAX_SZ);
		dwc_train[ddr4][0].dmem_base = ASPEED_SRAM_BASE + IMAGE_MAX_SZ;
	}
}

int aspeed_spl_dp_image_ymodem_load(u32 addr)
{
	int image_size;

	printf("Please send \"dp_fw.bin\" through Ymodem.\n");

	image_size = aspeed_spl_ymodem_image_load(0);
	if (image_size > 0) {
		memcpy((void *)addr, (void *)ASPEED_SRAM_BASE, image_size);
		return 0;
	}

	return -1;
}

int aspeed_spl_recovery_load_dp(u32 addr)
{
	int ret = -1;

	if (recovery_from_uart())
		ret = aspeed_spl_dp_image_ymodem_load(addr);

	return ret;
}

