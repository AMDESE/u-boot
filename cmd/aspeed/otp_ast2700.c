// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 Aspeed Technology Inc.
 */

#include <stdlib.h>
#include <common.h>
#include <console.h>
#include <bootretry.h>
#include <cli.h>
#include <command.h>
#include <console.h>
#include <malloc.h>
#include <inttypes.h>
#include <mapmem.h>
#include <asm/io.h>
#include <linux/compiler.h>
#include <linux/iopoll.h>
#include <u-boot/sha256.h>
#include <u-boot/sha512.h>
#include <u-boot/rsa.h>
#include <u-boot/rsa-mod-exp.h>
#include <dm.h>
#include <misc.h>
#include <asm/arch/otp_ast2700.h>
#include "otp_info_ast2700.h"

DECLARE_GLOBAL_DATA_PTR;

enum otp_region {
	OTP_REGION_ROM,
	OTP_REGION_CONF,
	OTP_REGION_STRAP,
	OTP_REGION_FLASH_STRAP,
	OTP_REGION_FLASH_STRAP_VLD,
	OTP_REGION_USER_DATA,
	OTP_REGION_SEC_DATA,
};

enum otp_status {
	OTP_FAILURE = -2,
	OTP_USAGE = -1,
	OTP_SUCCESS = 0,
	OTP_PROG_SKIP,
};

#define OTP_VER				"1.0.0"

#define OTP_AST2700_A0			0
#define OTP_AST2700_A1			1

#define ID0_AST2700A0			0x06000003
#define ID1_AST2700A0			0x06000003
#define ID0_AST2700A1			0x06010003
#define ID1_AST2700A1			0x06010003

/* OTP memory address from 0x0~0x2000. (unit: Single Word 16-bits) */
/* ----  0x0  -----
 *     OTP ROM
 * ---- 0x400 -----
 *     OTPCFG
 * ---- 0x420 -----
 *     HW STRAP
 * ---- 0x430 -----
 *   Flash STRAP
 * ---- 0x440 -----
 *   User Region
 * ---- 0x1000 ----
 *  Secure Region
 * ---- 0x1f80 ----
 *      SW PUF
 * ---- 0x1fc0 ----
 *      HW PUF
 * ---- 0x2000 ----
 */
#define OTPROM_START_ADDR		0x0
#define OTPROM_END_ADDR			0x400
#define OTPCFG_START_ADDR		0x400
#define OTPCFG_END_ADDR			0x420
#define OTPSTRAP_START_ADDR		0x420
#define OTPSTRAP_END_ADDR		0x430
#define OTPFLASHSTRAP_START_ADDR	0x430
#define OTPFLASHSTRAP_END_ADDR		0x440
#define USER_REGION_START_ADDR		0x440
#define USER_REGION_END_ADDR		0x1000
#define SEC_REGION_START_ADDR		0x1000
#define SEC_REGION_END_ADDR		0x1f80
#define SW_PUF_START_ADDR		0x1f80
#define SW_PUF_END_ADDR			0x1fc0
#define HW_PUF_START_ADDR		0x1fc0
#define HW_PUF_END_ADDR			0x2000

#define OTP_MEM_ADDR_MAX		HW_PUF_START_ADDR
#define OTP_ROM_REGION_SIZE		(OTPROM_END_ADDR - OTPROM_START_ADDR)
#define OTP_CFG_REGION_SIZE		(OTPCFG_END_ADDR - OTPCFG_START_ADDR)
#define OTP_STRAP_REGION_SIZE		(OTPSTRAP_END_ADDR - OTPSTRAP_START_ADDR - 4)
#define OTP_FLASH_STRAP_REGION_SIZE	(OTPFLASHSTRAP_END_ADDR - OTPFLASHSTRAP_START_ADDR)
#define OTP_USER_REGION_SIZE		(USER_REGION_END_ADDR - USER_REGION_START_ADDR)
#define OTP_SEC_REGION_SIZE		(SEC_REGION_END_ADDR - SEC_REGION_START_ADDR)

struct otpstrap_status {
	int value;
	int option_value[6];
	int remain_times;
	int writeable_option;
	int protected;
};

struct otp_info_cb {
	int version;
	char ver_name[3];
	const struct otpstrap_info *strap_info;
	int strap_info_len;
	const struct otp_f_strap_info *f_strap_info;
	int f_strap_info_len;
};

static struct otp_info_cb info_cb;
struct udevice *otp_dev;

static u32 chip_version(void)
{
	u32 revid0, revid1;

	revid0 = readl(ASPEED_CPU_REVISION_ID);
	revid1 = readl(ASPEED_IO_REVISION_ID);

	if (revid0 == ID0_AST2700A0 && revid1 == ID1_AST2700A0) {
		/* AST2700-A0 */
		return OTP_AST2700_A0;
	} else if (revid0 == ID0_AST2700A1 && revid1 == ID1_AST2700A1) {
		/* AST2700-A1 */
		return OTP_AST2700_A1;
	}

	return OTP_FAILURE;
}

static int otp_read(u32 offset, u16 *data)
{
	return misc_read(otp_dev, offset, data, 1);
}

static int otp_prog(u32 offset, u16 data)
{
	return misc_write(otp_dev, offset, &data, 1);
}

static int otp_read_conf(u32 offset, u16 *data)
{
	return otp_read(offset + OTPCFG_START_ADDR, data);
}

static int otp_read_strap(u32 offset, u16 *data)
{
	return otp_read(offset + OTPSTRAP_START_ADDR, data);
}

static int otp_read_flash_strap(u32 offset, u16 *data)
{
	return otp_read(offset + OTPFLASHSTRAP_START_ADDR, data);
}

static int otp_read_flash_strap_vld(u32 offset, u16 *data)
{
	return otp_read(offset + OTPFLASHSTRAP_START_ADDR + 0x8, data);
}

static int otp_read_data(u32 offset, u16 *data)
{
	return otp_read(offset + USER_REGION_START_ADDR, data);
}

static int otp_print_rom(u32 offset, int w_count)
{
	int range = OTPROM_END_ADDR - OTPROM_START_ADDR;
	u16 ret[1];
	int i;

	if (offset + w_count > range)
		return OTP_USAGE;

	printf("OTPROM: 0x%x~0x%x\n", offset, offset + w_count);
	for (i = offset; i < offset + w_count; i++) {
		otp_read(OTPROM_START_ADDR + i, &ret[0]);
		if (i % 8 == 0)
			printf("\n%03X: %04X ", i * 2, ret[0]);
		else
			printf("%04X ", ret[0]);
	}
	printf("\n");

	return OTP_SUCCESS;
}

static int otp_print_conf(u32 offset, int w_count)
{
	int range = OTPCFG_END_ADDR - OTPCFG_START_ADDR;
	u16 ret[1];
	int i;

	if (offset + w_count > range)
		return OTP_USAGE;

	for (i = offset; i < offset + w_count; i++) {
		otp_read(OTPCFG_START_ADDR + i, ret);
		printf("OTPCFG0x%X: 0x%04X\n", i, ret[0]);
	}
	printf("\n");

	return OTP_SUCCESS;
}

static int otp_print_strap(u32 offset, int w_count)
{
	int range = 12;	/* 32-bit * 6 / 16 (per word) */
	u16 ret[1];
	int i;

	if (offset + w_count > range)
		return OTP_USAGE;

	for (i = offset; i < offset + w_count; i++) {
		otp_read(OTPSTRAP_START_ADDR + 2 + i, ret);
		printf("OTPSTRAP0x%X: 0x%04X\n", i, ret[0]);
	}
	printf("\n");

	return OTP_SUCCESS;
}

static int otp_print_flash_strap(u32 offset, int w_count)
{
	int range = (OTPFLASHSTRAP_END_ADDR - OTPFLASHSTRAP_START_ADDR) / 2;
	u16 ret[1];
	int i;

	if (offset + w_count > range)
		return OTP_USAGE;

	for (i = offset; i < offset + w_count; i++) {
		otp_read(OTPFLASHSTRAP_START_ADDR + i, ret);
		printf("OTPFLASHSTRAP0x%X: 0x%04X\n", i, ret[0]);
	}
	printf("\n");

	return OTP_SUCCESS;
}

static int otp_print_flash_strap_valid(u32 offset, int w_count)
{
	int range = (OTPFLASHSTRAP_END_ADDR - OTPFLASHSTRAP_START_ADDR) / 2;
	u16 ret[1];
	int i;

	if (offset + w_count > range)
		return OTP_USAGE;

	for (i = offset; i < offset + w_count; i++) {
		otp_read(OTPFLASHSTRAP_START_ADDR + 0x8 + i, ret);
		printf("OTPFLASHSTRAP_VLD0x%X: 0x%04X\n", i, ret[0]);
	}
	printf("\n");

	return OTP_SUCCESS;
}

static int otp_print_user_data(u32 offset, int w_count)
{
	int range = USER_REGION_END_ADDR - USER_REGION_START_ADDR;
	u16 ret[1];
	int i;

	if (offset + w_count > range)
		return OTP_USAGE;

	printf("User Region: 0x%x~0x%x\n", offset, offset + w_count);
	for (i = offset; i < offset + w_count; i++) {
		otp_read(USER_REGION_START_ADDR + i, &ret[0]);
		if (i % 8 == 0)
			printf("\n%03X: %04X ", i * 2, ret[0]);
		else
			printf("%04X ", ret[0]);
	}
	printf("\n");

	return OTP_SUCCESS;
}

static int otp_print_sec_data(u32 offset, int w_count)
{
	int range = SEC_REGION_END_ADDR - SEC_REGION_START_ADDR;
	u16 ret[1];
	int i;

	if (offset + w_count > range)
		return OTP_USAGE;

	printf("Secure Region: 0x%x~0x%x\n", offset, offset + w_count);
	for (i = offset; i < offset + w_count; i++) {
		otp_read(SEC_REGION_START_ADDR + i, &ret[0]);
		if (i % 8 == 0)
			printf("\n%03X: %04X ", i * 2, ret[0]);
		else
			printf("%04X ", ret[0]);
	}
	printf("\n");

	return OTP_SUCCESS;
}

static int otp_print_puf(u32 offset, int w_count)
{
	int range = SW_PUF_END_ADDR - SW_PUF_START_ADDR;
	u16 ret[1];
	int i;

	if (offset + w_count > range)
		return OTP_USAGE;

	printf("SW PUF: 0x%x~0x%x\n", offset, offset + w_count);
	for (i = offset; i < offset + w_count; i++) {
		otp_read(SW_PUF_START_ADDR + i, &ret[0]);
		if (i % 8 == 0)
			printf("\n%03X: %04X ", i * 2, ret[0]);
		else
			printf("%04X ", ret[0]);
	}
	printf("\n");

	return OTP_SUCCESS;
}

static int otp_prog_data(int mode, int otp_w_offset, int bit_offset,
			 int value, int nconfirm)
{
	u32 prog_address;
	u16 read[1];
	int ret = 0;

	switch (mode) {
	case OTP_REGION_CONF:
		otp_read_conf(otp_w_offset, read);
		prog_address = OTPCFG_START_ADDR + otp_w_offset;
		printf("Program OTPCFG%d[0x%X] = 0x%x...\n", otp_w_offset,
		       bit_offset, value);
		break;
	case OTP_REGION_STRAP:
		otp_read_strap(otp_w_offset, read);
		prog_address = OTPSTRAP_START_ADDR + otp_w_offset;
		printf("Program OTPSTRAP%d[0x%X] = 0x%x...\n", otp_w_offset,
		       bit_offset, value);
		break;
	case OTP_REGION_FLASH_STRAP:
		otp_read_flash_strap(otp_w_offset, read);
		prog_address = OTPFLASHSTRAP_START_ADDR + otp_w_offset;
		printf("Program OTPFLASHSTRAP%d[0x%X] = 0x%x...\n", otp_w_offset,
		       bit_offset, value);
		break;
	case OTP_REGION_FLASH_STRAP_VLD:
		otp_read_flash_strap_vld(otp_w_offset, read);
		prog_address = OTPFLASHSTRAP_START_ADDR + 0x8 + otp_w_offset;
		printf("Program OTPFLASHSTRAP_VLD%d[0x%X] = 0x%x...\n", otp_w_offset,
		       bit_offset, value);
		break;
	case OTP_REGION_USER_DATA:
		otp_read_data(otp_w_offset, read);
		prog_address = USER_REGION_START_ADDR + otp_w_offset;
		printf("Program OTPDATA%d[0x%X] = 0x%x...\n", otp_w_offset,
		       bit_offset, value);
		break;
	}

	if (!nconfirm) {
		printf("type \"YES\" (no quotes) to continue:\n");
		if (!confirm_yesno()) {
			printf(" Aborting\n");
			return OTP_FAILURE;
		}
	}

	value = value << bit_offset;
	ret = otp_prog(prog_address, value);
	if (ret) {
		printf("OTP cannot be programmed\n");
		printf("FAILURE\n");
		return OTP_FAILURE;
	}

	printf("SUCCESS\n");

	return OTP_SUCCESS;
}

static void otp_strap_status(struct otpstrap_status *otpstrap)
{
	int strap_start, strap_end;
	u16 data[2];

	for (int i = 0; i < 32; i++) {
		otpstrap[i].value = 0;
		otpstrap[i].remain_times = 6;
		otpstrap[i].writeable_option = -1;
		otpstrap[i].protected = 0;
	}

	strap_start = 2;
	strap_end = 2 + 12;

	for (int i = strap_start; i < strap_end; i += 2) {
		int option = (i - strap_start) / 2;

		otp_read_strap(i, &data[0]);
		otp_read_strap(i + 1, &data[1]);

		for (int j = 0; j < 16; j++) {
			char bit_value = ((data[0] >> j) & 0x1);

			if (bit_value == 0 && otpstrap[j].writeable_option == -1)
				otpstrap[j].writeable_option = option;
			if (bit_value == 1)
				otpstrap[j].remain_times--;
			otpstrap[j].value ^= bit_value;
			otpstrap[j].option_value[option] = bit_value;
		}

		for (int j = 16; j < 32; j++) {
			char bit_value = ((data[1] >> (j - 16)) & 0x1);

			if (bit_value == 0 && otpstrap[j].writeable_option == -1)
				otpstrap[j].writeable_option = option;
			if (bit_value == 1)
				otpstrap[j].remain_times--;
			otpstrap[j].value ^= bit_value;
			otpstrap[j].option_value[option] = bit_value;
		}
	}

	otp_read_strap(0, &data[0]);
	otp_read_strap(1, &data[1]);

	for (int j = 0; j < 16; j++) {
		if (((data[0] >> j) & 0x1) == 1)
			otpstrap[j].protected = 1;
	}
	for (int j = 16; j < 32; j++) {
		if (((data[1] >> (j - 16)) & 0x1) == 1)
			otpstrap[j].protected = 1;
	}

#ifdef DEBUG
	for (int i = 0; i < 32; i++) {
		printf("otpstrap[%d]: value:%d, remain_times:%d, writeable_option:%d, protected:%d\n",
		       i, otpstrap[i].value, otpstrap[i].remain_times,
		       otpstrap[i].writeable_option, otpstrap[i].protected);
		printf("option_value: ");
		for (int j = 0; j < 6; j++)
			printf("%d ", otpstrap[i].option_value[j]);
		printf("\n");
	}
#endif
}

static void otp_print_strap_info(void)
{
	const struct otpstrap_info *strap_info = info_cb.strap_info;
	struct otpstrap_status strap_status[32];
	u32 bit_offset;
	u32 length;
	u32 otp_value;
	u32 otp_protect;

	otp_strap_status(strap_status);

	printf("BIT(hex) Value  Remains  Protect   Description\n");
	printf("___________________________________________________________________________________________________\n");

	for (int i = 0; i < info_cb.strap_info_len; i++) {
		otp_value = 0;
		otp_protect = 0;
		bit_offset = strap_info[i].bit_offset;
		length = strap_info[i].length;
		for (int j = 0; j < length; j++) {
			otp_value |= strap_status[bit_offset + j].value << j;
			otp_protect |= strap_status[bit_offset + j].protected << j;
		}

		if (otp_value != strap_info[i].value &&
		    strap_info[i].value != OTP_REG_RESERVED)
			continue;

		for (int j = 0; j < length; j++) {
			printf("0x%-7X", strap_info[i].bit_offset + j);
			printf("0x%-5X", strap_status[bit_offset + j].value);
			printf("%-9d", strap_status[bit_offset + j].remain_times);
			printf("0x%-7X", strap_status[bit_offset + j].protected);
			if (strap_info[i].value == OTP_REG_RESERVED) {
				printf(" Reserved\n");
				continue;
			}

			if (length == 1) {
				printf(" %s\n", strap_info[i].information);
				continue;
			}

			if (j == 0)
				printf("/%s\n", strap_info[i].information);
			else if (j == length - 1)
				printf("\\ \"\n");
			else
				printf("| \"\n");
		}
	}
}

static int otp_strap_bit_confirm(struct otpstrap_status *otpstrap, int offset, int ibit, int value, int pbit)
{
	int prog_flag = 0;

	// ignore this bit
	if (ibit == 1)
		return OTP_SUCCESS;

	printf("OTPSTRAP[0x%X]:\n", offset);

	if (value == otpstrap->value) {
		if (!pbit) {
			printf("\tThe value is same as before, skip it.\n");
			return OTP_PROG_SKIP;
		}
		printf("\tThe value is same as before.\n");

	} else {
		prog_flag = 1;
	}

	if (otpstrap->protected == 1 && prog_flag) {
		printf("\tThis bit is protected and is not writable\n");
		return OTP_FAILURE;
	}

	if (otpstrap->remain_times == 0 && prog_flag) {
		printf("\tThis bit has no remaining chance to write.\n");
		return OTP_FAILURE;
	}

	if (pbit == 1)
		printf("\tThis bit will be protected and become non-writable.\n");

	if (prog_flag)
		printf("\tWrite 1 to OTPSTRAP[0x%X] OPTION[0x%X], that value becomes from 0x%X to 0x%X.\n",
		       offset, otpstrap->writeable_option, otpstrap->value, otpstrap->value ^ 1);

	return OTP_SUCCESS;
}

static void otp_print_flash_strap_info(void)
{
	const struct otp_f_strap_info *f_strap_info = info_cb.f_strap_info;
	u32 bit_offset;
	u32 otp_value, otp_vld;
	u32 length;
	u16 data[8];
	u16 vld[8];

	/* Read Flash strap */
	for (int i = 0; i < 8; i++)
		otp_read_flash_strap(i, &data[i]);

	/* Read Flash strap valid */
	for (int i = 0; i < 8; i++)
		otp_read_flash_strap_vld(i, &vld[i]);

	printf("BIT(hex) Value  Valid   Description\n");
	printf("___________________________________________________________________________________________________\n");

	for (int i = 0; i < info_cb.f_strap_info_len; i++) {
		otp_value = 0;
		otp_vld = 0;
		bit_offset = f_strap_info[i].bit_offset;
		length = f_strap_info[i].length;

		int w_offset = bit_offset / 16;
		int b_offset = bit_offset % 16;

		otp_value = (data[w_offset] >> b_offset) &
			    GENMASK(length - 1, 0);
		otp_vld = (vld[w_offset] >> b_offset) &
			  GENMASK(length - 1, 0);

		if (otp_value != f_strap_info[i].value)
			continue;

		for (int j = 0; j < length; j++) {
			printf("0x%-7X", f_strap_info[i].bit_offset + j);
			printf("0x%-5lX", (otp_value & BIT(j)) >> j);
			printf("0x%-5lX", (otp_vld & BIT(j)) >> j);

			if (length == 1) {
				printf(" %s\n", f_strap_info[i].information);
				continue;
			}

			if (j == 0)
				printf("/%s\n", f_strap_info[i].information);
			else if (j == length - 1)
				printf("\\ \"\n");
			else
				printf("| \"\n");
		}
	}
}

static int otp_patch_prog(phys_addr_t addr, u32 offset, size_t size)
{
	int ret = 0;
	u16 val;

	printf("%s: addr:0x%llx, offset:0x%x, size:0x%lx\n", __func__,
	       addr, offset, size);

	for (int i = 0; i < size; i++) {
		val = readw((u16 *)addr + i);
		printf("read 0x%lx = 0x%x..., prog into OTP addr 0x%x\n",
		       (uintptr_t)addr + i, val, offset + i);
		ret += otp_prog(offset + i, val);
	}

	return ret;
}

static int otp_patch_enable_pre(u16 offset, size_t size)
{
	int ret;

	/* Set location - OTPCFG4[10:1] */
	ret = otp_prog_data(OTP_REGION_CONF, 4, 1, offset, 1);
	if (ret) {
		printf("%s: Prog location Failed, ret:0x%x\n", __func__, ret);
		return ret;
	}

	/* Set Size - OTPCFG5[9:0] */
	ret = otp_prog_data(OTP_REGION_CONF, 5, 0, size, 1);
	if (ret) {
		printf("%s: Prog size Failed, ret:0x%x\n", __func__, ret);
		return ret;
	}

	/* enable pre_otp_patch_vld - OTPCFG4[0] */
	ret = otp_prog_data(OTP_REGION_CONF, 4, 0, 1, 1);
	if (ret) {
		printf("%s: Enable pre_otp_patch_vld Failed, ret:0x%x\n", __func__, ret);
		return ret;
	}

	return 0;
}

static int otp_patch_enable_post(u16 offset, size_t size)
{
	int ret;

	/* Set location - OTPCFG6[10:1] */
	ret = otp_prog_data(OTP_REGION_CONF, 6, 1, offset, 1);
	if (ret) {
		printf("%s: Prog location Failed, ret:0x%x\n", __func__, ret);
		return ret;
	}

	/* Set Size - OTPCFG7[9:0] */
	ret = otp_prog_data(OTP_REGION_CONF, 7, 0, size, 1);
	if (ret) {
		printf("%s: Prog size Failed, ret:0x%x\n", __func__, ret);
		return ret;
	}

	/* enable pre_otp_patch_vld - OTPCFG6[0] */
	ret = otp_prog_data(OTP_REGION_CONF, 6, 0, 1, 1);
	if (ret) {
		printf("%s: Enable post_otp_patch_vld Failed, ret:0x%x\n", __func__, ret);
		return ret;
	}

	return 0;
}

static int do_otpinfo(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	if (argc != 2 && argc != 3)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1], "strap"))
		otp_print_strap_info();
	else if (!strcmp(argv[1], "f-strap"))
		otp_print_flash_strap_info();
	else
		return CMD_RET_USAGE;

	return CMD_RET_SUCCESS;
}

static int do_otpread(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	u32 offset, count;
	int ret;

	printf("%s: %d: %s %s %s\n", __func__, argc, argv[1], argv[2], argv[3]);

	if (argc == 4) {
		offset = simple_strtoul(argv[2], NULL, 16);
		count = simple_strtoul(argv[3], NULL, 16);
	} else if (argc == 3) {
		offset = simple_strtoul(argv[2], NULL, 16);
		count = 1;
	} else {
		return CMD_RET_USAGE;
	}

	if (!strcmp(argv[1], "rom"))
		ret = otp_print_rom(offset, count);
	else if (!strcmp(argv[1], "conf"))
		ret = otp_print_conf(offset, count);
	else if (!strcmp(argv[1], "strap"))
		ret = otp_print_strap(offset, count);
	else if (!strcmp(argv[1], "f-strap"))
		ret = otp_print_flash_strap(offset, count);
	else if (!strcmp(argv[1], "f-strap-vld"))
		ret = otp_print_flash_strap_valid(offset, count);
	else if (!strcmp(argv[1], "u-data"))
		ret = otp_print_user_data(offset, count);
	else if (!strcmp(argv[1], "s-data"))
		ret = otp_print_sec_data(offset, count);
	else if (!strcmp(argv[1], "puf"))
		ret = otp_print_puf(offset, count);
	else
		return CMD_RET_USAGE;

	if (ret == OTP_SUCCESS)
		return CMD_RET_SUCCESS;
	return CMD_RET_USAGE;
}

static int do_otppatch(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	phys_addr_t addr;
	u32 offset;
	size_t size;
	int ret;

	printf("%s: argc:%d\n", __func__, argc);

	if (argc != 5)
		return CMD_RET_USAGE;

	/* Drop the cmd */
	argc--;
	argv++;

	if (!strcmp(argv[0], "prog")) {
		addr = simple_strtoul(argv[1], NULL, 16);
		offset = simple_strtoul(argv[2], NULL, 16);
		size = simple_strtoul(argv[3], NULL, 16);

		ret = otp_patch_prog(addr, offset, size);

	} else if (!strcmp(argv[0], "enable")) {
		offset = simple_strtoul(argv[2], NULL, 16);
		size = simple_strtoul(argv[3], NULL, 16);

		if (!strcmp(argv[1], "pre"))
			ret = otp_patch_enable_pre(offset, size);

		else if (!strcmp(argv[1], "post"))
			ret = otp_patch_enable_post(offset, size);

		else
			return CMD_RET_USAGE;
	}

	if (ret == OTP_SUCCESS)
		return CMD_RET_SUCCESS;
	else if (ret == OTP_FAILURE)
		return CMD_RET_FAILURE;
	else
		return CMD_RET_USAGE;
}

static int do_otppb(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	struct otpstrap_status otpstrap[32];
	int mode = 0;
	int nconfirm = 0;
	int otp_addr = 0;
	int bit_offset;
	int value;
	int ret;

	if (argc != 3 && argc != 4 && argc != 5 && argc != 6) {
		printf("%s: argc:%d\n", __func__, argc);
		return CMD_RET_USAGE;
	}

	/* Drop the pb cmd */
	argc--;
	argv++;

	if (!strcmp(argv[0], "conf"))
		mode = OTP_REGION_CONF;
	else if (!strcmp(argv[0], "strap"))
		mode = OTP_REGION_STRAP;
	else if (!strcmp(argv[0], "f-strap"))
		mode = OTP_REGION_FLASH_STRAP;
	else if (!strcmp(argv[0], "f-strap-vld"))
		mode = OTP_REGION_FLASH_STRAP_VLD;
	else if (!strcmp(argv[0], "data"))
		mode = OTP_REGION_USER_DATA;
	else
		return CMD_RET_USAGE;

	/* Drop the region cmd */
	argc--;
	argv++;

	if (!strcmp(argv[0], "o")) {
		nconfirm = 1;
		/* Drop the force option */
		argc--;
		argv++;
	}

	if (mode == OTP_REGION_STRAP) {
		bit_offset = simple_strtoul(argv[0], NULL, 16);
		value = simple_strtoul(argv[1], NULL, 16);
		if (bit_offset >= 32)
			return CMD_RET_USAGE;
		if (value != 0 && value != 1)
			return CMD_RET_USAGE;

	} else if (mode == OTP_REGION_FLASH_STRAP) {
		bit_offset = simple_strtoul(argv[0], NULL, 16);
		value = simple_strtoul(argv[1], NULL, 16);
		if (bit_offset >= 128)
			return CMD_RET_USAGE;
		if (value != 1)
			return CMD_RET_USAGE;

	} else if (mode == OTP_REGION_FLASH_STRAP_VLD) {
		if (!strcmp(argv[0], "all")) {
			for (int i = 0; i < 8; i++) {
				ret = otp_prog_data(mode, i, 0, GENMASK(15, 0), 1);
				if (ret)
					goto end;
			}

			goto end;
		}

		bit_offset = simple_strtoul(argv[0], NULL, 16);
		value = simple_strtoul(argv[1], NULL, 16);
		if (bit_offset >= 128)
			return CMD_RET_USAGE;
		if (value != 1)
			return CMD_RET_USAGE;

	} else {
		otp_addr = simple_strtoul(argv[0], NULL, 16);
		bit_offset = simple_strtoul(argv[1], NULL, 16);
		value = simple_strtoul(argv[2], NULL, 16);
		if (bit_offset >= 16)
			return CMD_RET_USAGE;
	}

	/* Check param */
	if (mode == OTP_REGION_CONF) {
		if (otp_addr >= OTP_CFG_REGION_SIZE)
			return CMD_RET_USAGE;

	} else if (mode == OTP_REGION_STRAP) {
		// get otpstrap status
		otp_strap_status(otpstrap);

		ret = otp_strap_bit_confirm(&otpstrap[bit_offset], bit_offset, 0, value, 0);
		if (ret != OTP_SUCCESS)
			return ret;

		// assign writable otp address
		if (bit_offset < 16) {
			otp_addr = 2 + otpstrap[bit_offset].writeable_option * 2;
		} else {
			otp_addr = 3 + otpstrap[bit_offset].writeable_option * 2;
			bit_offset -= 16;
		}

		value = 1;

	} else if (mode == OTP_REGION_FLASH_STRAP || mode == OTP_REGION_FLASH_STRAP_VLD) {
		otp_addr = bit_offset / 16;
		bit_offset = bit_offset % 16;

	} else {
		/* user data + secure data region */
		if (otp_addr >= OTP_USER_REGION_SIZE + OTP_SEC_REGION_SIZE)
			return CMD_RET_USAGE;
	}

	ret = otp_prog_data(mode, otp_addr, bit_offset, value, nconfirm);

end:
	if (ret == OTP_SUCCESS)
		return CMD_RET_SUCCESS;
	else if (ret == OTP_FAILURE)
		return CMD_RET_FAILURE;
	else
		return CMD_RET_USAGE;
}

static int do_otpecc(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	int ret;
	int ecc_en = 0;

	/* Get ECC status */
	ret = misc_ioctl(otp_dev, GET_ECC_STATUS, &ecc_en);
	if (ret)
		return CMD_RET_FAILURE;

	/* Drop the ecc cmd */
	argc--;
	argv++;

	if (!strcmp(argv[0], "status")) {
		if (ecc_en == OTP_ECC_ENABLE)
			printf("OTP ECC is enabled\n");
		else
			printf("OTP ECC is disabled\n");

		return CMD_RET_SUCCESS;

	} else if (!strcmp(argv[0], "enable")) {
		if (ecc_en == OTP_ECC_ENABLE) {
			printf("OTP ECC is already enabled\n");
			return CMD_RET_SUCCESS;
		}

		/* Set ECC enable */
		ret = misc_ioctl(otp_dev, SET_ECC_ENABLE, NULL);
		if (ret)
			return CMD_RET_FAILURE;

		printf("OTP ECC is enabled\n");

	} else {
		return CMD_RET_USAGE;
	}

	return CMD_RET_SUCCESS;
}

static int do_otpver(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	printf("OTP tool version: %s\n", OTP_VER);
	printf("OTP info version: %s\n", OTP_INFO_VER);

	return CMD_RET_SUCCESS;
}

static struct cmd_tbl cmd_otp[] = {
	U_BOOT_CMD_MKENT(version, 1, 0, do_otpver, "", ""),
	U_BOOT_CMD_MKENT(read, 4, 0, do_otpread, "", ""),
	U_BOOT_CMD_MKENT(pb, 6, 0, do_otppb, "", ""),
	U_BOOT_CMD_MKENT(patch, 5, 0, do_otppatch, "", ""),
	U_BOOT_CMD_MKENT(ecc, 2, 0, do_otpecc, "", ""),
	U_BOOT_CMD_MKENT(info, 3, 0, do_otpinfo, "", ""),
};

static void do_driver_init(void)
{
	int ret;

	ret = uclass_get_device_by_driver(UCLASS_MISC,
					  DM_DRIVER_GET(aspeed_otp), &otp_dev);
	if (ret) {
		printf("%s: get Aspeed otp driver failed\n", __func__);
		return;
	}
}

static int do_ast_otp(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	struct cmd_tbl *cp;
	u32 ver;
	int ret;

	cp = find_cmd_tbl(argv[1], cmd_otp, ARRAY_SIZE(cmd_otp));

	/* Drop the otp command */
	argc--;
	argv++;

	if (!cp || argc > cp->maxargs)
		return CMD_RET_USAGE;
	if (flag == CMD_FLAG_REPEAT && !cmd_is_repeatable(cp))
		return CMD_RET_SUCCESS;

	do_driver_init();

	ver = chip_version();
	switch (ver) {
	case OTP_AST2700_A0:
	case OTP_AST2700_A1:
		info_cb.version = OTP_AST2700_A0;
		info_cb.strap_info = a0_strap_info;
		info_cb.strap_info_len = ARRAY_SIZE(a0_strap_info);
		info_cb.f_strap_info = a0_f_strap_info;
		info_cb.f_strap_info_len = ARRAY_SIZE(a0_f_strap_info);
		break;
	default:
		printf("SOC is not supported\n");
		return CMD_RET_FAILURE;
	}

	ret = cp->cmd(cmdtp, flag, argc, argv);

	return ret;
}

U_BOOT_CMD(otp, 7, 0,  do_ast_otp,
	   "ASPEED One-Time-Programmable sub-system",
	   "version\n"
	   "otp read rom|conf|strap|f-strap|f-strap-vld|u-data|s-data|puf <otp_w_offset> <w_count>\n"
	   "otp pb conf|data [o] <otp_w_offset> <bit_offset> <value>\n"
	   "otp pb strap|f-strap|f-strap-vld [o] <bit_offset> <value>\n"
	   "otp pb f-strap-vld all\n"
	   "otp info strap|f-strap\n"
	   "otp patch prog <dram_addr> <otp_w_offset> <w_count>\n"
	   "otp patch enable pre|post <otp_start_w_offset> <w_count>\n"
	   "otp ecc status|enable\n"
	  );
