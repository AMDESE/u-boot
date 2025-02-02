// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2024 Aspeed Technology Inc.
 */

#include <asm/arch/platform.h>
#include <asm/arch/recovery.h>
#include <asm/arch/sdram_ast2700.h>
#include <asm/io.h>
#include <asm/u-boot.h>
#include <common.h>
#include <log.h>
#include <spl.h>
#include <xyzModem.h>

#define BUF_SIZE	(1024)

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
	uchar *buf = (uchar *)(addr);

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

int aspeed_spl_dp_image_ymodem_load(u32 addr)
{
	printf("Please send \"dp_fw.bin\" through Ymodem.\n");

	return aspeed_spl_ymodem_image_load(addr);
}

int aspeed_spl_ddr_image_ymodem_load(u32 dest, enum recovery_ddr_type ddr_type,
				     enum recovery_ddr_mem_type mem_type,
				     const int train2D)
{
	if (ddr_type == TYPE_DDR4 && mem_type == DDR_I_MEM && train2D == 0)
		printf("Please send \"ddr4_pmu_train_imem.bin\" through Ymodem.\n");
	else if (ddr_type == TYPE_DDR4 && mem_type == DDR_D_MEM && train2D == 0)
		printf("Please send \"ddr4_pmu_train_dmem.bin\" through Ymodem.\n");
	else if (ddr_type == TYPE_DDR4 && mem_type == DDR_I_MEM && train2D == 1)
		printf("Please send \"ddr4_2d_pmu_train_imem.bin\" through Ymodem.\n");
	else if (ddr_type == TYPE_DDR4 && mem_type == DDR_D_MEM && train2D == 1)
		printf("Please send \"ddr4_2d_pmu_train_dmem.bin\" through Ymodem.\n");
	else if (ddr_type == TYPE_DDR5 && mem_type == DDR_I_MEM)
		printf("Please send \"ddr5_pmu_train_imem.bin\" through Ymodem.\n");
	else if (ddr_type == TYPE_DDR5 && mem_type == DDR_D_MEM)
		printf("Please send \"ddr5_pmu_train_dmem.bin\" through Ymodem.\n");

	return aspeed_spl_ymodem_image_load(dest);
}
