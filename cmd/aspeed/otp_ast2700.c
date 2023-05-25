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

#define OTP_VER				"1.0.0"

#define OTP_USAGE			-1
#define OTP_FAILURE			-2
#define OTP_SUCCESS			0

#define OTP_AST2700_A0			0

#define ID0_AST2700A0			0x06000003
#define ID1_AST2700A0			0x06000003

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
#define OTP_STRAP_REGION_SIZE		(OTPSTRAP_END_ADDR - OTPSTRAP_START_ADDR)
#define OTP_FLASH_STRAP_REGION_SIZE	(OTPFLASHSTRAP_END_ADDR - OTPFLASHSTRAP_START_ADDR)
#define OTP_USER_REGION_SIZE		(USER_REGION_END_ADDR - USER_REGION_START_ADDR)
#define OTP_SEC_REGION_SIZE		(SEC_REGION_END_ADDR - SEC_REGION_START_ADDR)

struct udevice *otp_dev;

static u32 chip_version(void)
{
	u32 revid0, revid1;

	revid0 = readl(ASPEED_REVISION_ID0);
	revid1 = readl(ASPEED_REVISION_ID1);

	if (revid0 == ID0_AST2700A0 && revid1 == ID1_AST2700A0) {
		/* AST2700-A0 */
		return OTP_AST2700_A0;
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
	int range = OTPSTRAP_END_ADDR - OTPSTRAP_START_ADDR;
	u16 ret[1];
	int i;

	if (offset + w_count > range)
		return OTP_USAGE;

	for (i = offset; i < offset + w_count; i++) {
		otp_read(OTPSTRAP_START_ADDR + i, ret);
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

static int do_otpprogpatch(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	phys_addr_t addr;
	u32 offset;
	size_t size;
	int ret;
	u16 val;

	printf("%s: argc:%d\n", __func__, argc);

	if (argc == 4) {
		addr = simple_strtoul(argv[1], NULL, 16);
		offset = simple_strtoul(argv[2], NULL, 16);
		size = simple_strtoul(argv[3], NULL, 16);
	} else {
		return CMD_RET_USAGE;
	}

	printf("%s: addr:0x%llx, offset:0x%x, size:0x%lx\n", __func__, addr, offset, size);
	for (int i = 0; i < size; i++) {
		val = readw((u16 *)addr + i);
		printf("read 0x%lx = 0x%x..., prog into OTP addr 0x%x\n",
		       (uintptr_t)addr + i, val, offset + i);
		ret = otp_prog(offset + i, val);
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
	int mode = 0;
	int nconfirm = 0;
	int otp_addr = 0;
	int bit_offset;
	int value;
	int ret;

	if (argc != 5 && argc != 6) {
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

	otp_addr = simple_strtoul(argv[0], NULL, 16);
	bit_offset = simple_strtoul(argv[1], NULL, 16);
	value = simple_strtoul(argv[2], NULL, 16);
	if (bit_offset >= 16)
		return CMD_RET_USAGE;

	/* Check param */
	if (mode == OTP_REGION_CONF) {
		if (otp_addr >= OTP_CFG_REGION_SIZE)
			return CMD_RET_USAGE;

	} else if (mode == OTP_REGION_STRAP) {
		if (otp_addr >= OTP_STRAP_REGION_SIZE)
			return CMD_RET_USAGE;

	} else if (mode == OTP_REGION_FLASH_STRAP || mode == OTP_REGION_FLASH_STRAP_VLD) {
		if (otp_addr >= OTP_FLASH_STRAP_REGION_SIZE / 2)
			return CMD_RET_USAGE;

	} else {
		/* user data + secure data region */
		if (otp_addr >= OTP_USER_REGION_SIZE + OTP_SEC_REGION_SIZE)
			return CMD_RET_USAGE;
	}

	ret = otp_prog_data(mode, otp_addr, bit_offset, value, nconfirm);

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
	U_BOOT_CMD_MKENT(progpatch, 4, 0, do_otpprogpatch, "", ""),
	U_BOOT_CMD_MKENT(ecc, 2, 0, do_otpecc, "", ""),
//	U_BOOT_CMD_MKENT(info, 3, 0, do_otpinfo, "", ""),
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
	   "otp read conf|strap|f-strap|f-strap-vld|u-data|s-data|puf <otp_w_offset> <w_count>\n"
	   "otp pb conf|strap|f-strap|f-strap-vld|data [o] <otp_w_offset> <bit_offset> <value>\n"
	   "otp progpatch <dram_addr> <otp_w_offset> <w_count>\n"
	   "otp ecc status|enable\n"
	  );
