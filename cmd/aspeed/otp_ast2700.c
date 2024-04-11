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
	OTP_REGION_STRAP_EXT,
	OTP_REGION_STRAP_EXT_VLD,
	OTP_REGION_USER_DATA,
	OTP_REGION_SECURE,
	OTP_REGION_CALIPTRA,
	OTP_REGION_PUF,
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

#define SOC_AST2700A0			8
#define SOC_AST2700A1			9

/* OTP memory address from 0x0~0x2000. (unit: Single Word 16-bits) */
/* ----  0x0  -----
 *       ROM
 * ---- 0x400 -----
 *      CONF
 * ---- 0x420 -----
 *      STRAP
 * ---- 0x430 -----
 *    STRAP EXT
 * ---- 0x440 -----
 *   User Region
 * ---- 0x1000 ----
 *  Secure Region
 * ---- 0x1c00 ----
 *     Caliptra
 * ---- 0x1f80 ----
 *      SW PUF
 * ---- 0x1fc0 ----
 *      HW PUF
 * ---- 0x2000 ----
 */
#define ROM_REGION_START_ADDR		0x0
#define ROM_REGION_END_ADDR		0x400
#define CONF_REGION_START_ADDR		ROM_REGION_END_ADDR
#define CONF_REGION_END_ADDR		0x420
#define STRAP_REGION_START_ADDR		CONF_REGION_END_ADDR
#define STRAP_REGION_END_ADDR		0x430
#define STRAPEXT_REGION_START_ADDR	STRAP_REGION_END_ADDR
#define STRAPEXT_REGION_END_ADDR	0x440
#define USER_REGION_START_ADDR		STRAPEXT_REGION_END_ADDR
#define USER_REGION_END_ADDR		0x1000
#define SEC_REGION_START_ADDR		USER_REGION_END_ADDR
#define SEC_REGION_END_ADDR		0x1c00
#define CAL_REGION_START_ADDR		SEC_REGION_END_ADDR
#define CAL_REGION_END_ADDR		0x1f80
#define SW_PUF_REGION_START_ADDR	CAL_REGION_END_ADDR
#define SW_PUF_REGION_END_ADDR		0x1fc0
#define HW_PUF_REGION_START_ADDR	SW_PUF_REGION_END_ADDR
#define HW_PUF_REGION_END_ADDR		0x2000

#define OTP_MEM_ADDR_MAX		HW_PUF_REGION_START_ADDR
#define OTP_ROM_REGION_SIZE		(ROM_REGION_END_ADDR - ROM_REGION_START_ADDR)
#define OTP_CONF_REGION_SIZE		(CONF_REGION_END_ADDR - CONF_REGION_START_ADDR)
#define OTP_STRAP_REGION_SIZE		(STRAP_REGION_END_ADDR - STRAP_REGION_START_ADDR - 4)
#define OTP_STRAP_EXT_REGION_SIZE	(STRAPEXT_REGION_END_ADDR - STRAPEXT_REGION_START_ADDR)
#define OTP_USER_REGION_SIZE		(USER_REGION_END_ADDR - USER_REGION_START_ADDR)
#define OTP_SEC_REGION_SIZE		(SEC_REGION_END_ADDR - SEC_REGION_START_ADDR)
#define OTP_CAL_REGION_SIZE		(CAL_REGION_END_ADDR - CAL_REGION_START_ADDR)
#define OTP_PUF_REGION_SIZE		(HW_PUF_REGION_END_ADDR - SW_PUF_REGION_START_ADDR)

#define OTP_DEVICE_NAME_0		"otp@14c07000"
#define OTP_DEVICE_NAME_1		"otp@30c07000"

#define OTP_MAGIC		"SOCOTP"
#define CHECKSUM_LEN		48
#define OTP_INC_ROM		BIT(31)
#define OTP_INC_CONFIG		BIT(30)
#define OTP_INC_STRAP		BIT(29)
#define OTP_INC_STRAP_EXT	BIT(28)
#define OTP_INC_SECURE		BIT(27)
#define OTP_INC_CALIPTRA	BIT(26)
#define OTP_REGION_SIZE(info)	(((info) >> 16) & 0xffff)
#define OTP_REGION_OFFSET(info)	((info) & 0xffff)
#define OTP_IMAGE_SIZE(info)	((info) & 0xffff)

/* OTP key header format */
#define OTP_KH_NUM		64
#define OTP_KH_KEY_ID(kh)	((kh) & 0xf)
#define OTP_KH_KEY_TYPE(kh)	(((kh) >> 4) & 0x7)
#define OTP_KH_LAST(kh)		(((kh) >> 15) & 0x1)
#define OTP_KH_OFFSET(kh)	(((kh) >> 16) & 0xfff)

struct otp_header {
	u8	otp_magic[8];
	u32	soc_ver;
	u32	otptool_ver;
	u32	image_info;
	u32	rom_info;
	u32	config_info;
	u32	strap_info;
	u32	strap_ext_info;
	u32	secure_info;
	u32	caliptra_info;
	u32	checksum_offset;
} __packed;

struct otpstrap_status {
	int value;
	int option_value[6];
	int remain_times;
	int writeable_option;
	int protected;
};

union otp_pro_sts {
	u32 value;
	struct {
		char r_prot_strap_ext : 1;
		char r_prot_rom : 1;
		char r_prot_conf : 1;
		char r_prot_strap : 1;
		char w_prot_strap_ext : 1;
		char w_prot_rom : 1;
		char w_prot_conf : 1;
		char w_prot_strap : 1;
		char retire_option : 1;
		char w_prot_key_ret : 1;
		char w_prot_rid : 1;
		char r_prot_cal : 1;
		char w_prot_cal : 1;
		char reserved : 1;
		char ecc_enable : 1;
		char mem_lock : 1;
	} fields;
};

struct otp_info_cb {
	int version;
	char ver_name[3];
	const struct otpconf_info *conf_info;
	int conf_info_len;
	const struct otpstrap_info *strap_info;
	int strap_info_len;
	const struct otpstrap_ext_info *strap_ext_info;
	int strap_ext_info_len;
	const struct otpcal_info *cal_info;
	int cal_info_len;
	const struct otpkey_type *key_info;
	int key_info_len;
	union otp_pro_sts pro_sts;
};

static struct otp_info_cb info_cb;

struct otp_image_layout {
	int rom_length;
	int conf_length;
	int strap_length;
	int strap_ext_length;
	int secure_length;
	int caliptra_length;
	u8 *rom;
	u8 *conf;
	u8 *strap;
	u8 *strap_ext;
	u8 *secure;
	u8 *caliptra;
};

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

static void buf_print(u8 *buf, int len)
{
	int i;

	printf("      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
	for (i = 0; i < len; i++) {
		if (i % 16 == 0)
			printf("%04X: ", i);
		printf("%02X ", buf[i]);
		if ((i + 1) % 16 == 0)
			printf("\n");
	}
	printf("\n");
}

static int otp_read(u32 offset, u16 *data)
{
	return misc_read(otp_dev, offset, data, 1);
}

static int otp_prog(u32 offset, u16 data)
{
	return misc_write(otp_dev, offset, &data, 1);
}

static int otp_read_rom(u32 offset, u16 *data)
{
	return otp_read(offset + ROM_REGION_START_ADDR, data);
}

static int otp_read_conf(u32 offset, u16 *data)
{
	return otp_read(offset + CONF_REGION_START_ADDR, data);
}

static int otp_read_strap(u32 offset, u16 *data)
{
	return otp_read(offset + STRAP_REGION_START_ADDR, data);
}

static int otp_read_strap_ext(u32 offset, u16 *data)
{
	return otp_read(offset + STRAPEXT_REGION_START_ADDR, data);
}

static int otp_read_strap_ext_vld(u32 offset, u16 *data)
{
	return otp_read(offset + STRAPEXT_REGION_START_ADDR + 0x8, data);
}

static int otp_read_data(u32 offset, u16 *data)
{
	return otp_read(offset + USER_REGION_START_ADDR, data);
}

static int otp_read_secure(u32 offset, u16 *data)
{
	return otp_read(offset + SEC_REGION_START_ADDR, data);
}

static int otp_read_secure_multi(u32 offset, u16 *data, int num)
{
	return misc_read(otp_dev, offset + SEC_REGION_START_ADDR, data, num);
}

static int otp_read_caliptra(u32 offset, u16 *data)
{
	return otp_read(offset + CAL_REGION_START_ADDR, data);
}

static int otp_print_rom(u32 offset, int w_count)
{
	int range = OTP_ROM_REGION_SIZE;
	u16 ret[1];
	int i;

	if (offset + w_count > range)
		return OTP_USAGE;

	printf("ROM_REGION: 0x%x~0x%x\n", offset, offset + w_count);
	for (i = offset; i < offset + w_count; i++) {
		otp_read(ROM_REGION_START_ADDR + i, &ret[0]);
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
	int range = OTP_CONF_REGION_SIZE;
	u16 ret[1];
	int i;

	if (offset + w_count > range)
		return OTP_USAGE;

	for (i = offset; i < offset + w_count; i++) {
		otp_read(CONF_REGION_START_ADDR + i, ret);
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
		otp_read(STRAP_REGION_START_ADDR + 2 + i, ret);
		printf("OTPSTRAP0x%X: 0x%04X\n", i, ret[0]);
	}
	printf("\n");

	return OTP_SUCCESS;
}

static int otp_print_strap_pro(u32 offset, int w_count)
{
	int range = 2;	/* 32-bit / 16 (per word) */
	u16 ret[1];
	int i;

	if (offset + w_count > range)
		return OTP_USAGE;

	for (i = offset; i < offset + w_count; i++) {
		otp_read(STRAP_REGION_START_ADDR + i, ret);
		printf("OTPSTRAP_PRO0x%X: 0x%04X\n", i, ret[0]);
	}
	printf("\n");

	return OTP_SUCCESS;
}

static int otp_print_strap_ext(u32 offset, int w_count)
{
	int range = (OTP_STRAP_EXT_REGION_SIZE) / 2;
	u16 ret[1];
	int i;

	if (offset + w_count > range)
		return OTP_USAGE;

	for (i = offset; i < offset + w_count; i++) {
		otp_read(STRAPEXT_REGION_START_ADDR + i, ret);
		printf("OTPSTRAPEXT0x%X: 0x%04X\n", i, ret[0]);
	}
	printf("\n");

	return OTP_SUCCESS;
}

static int otp_print_strap_ext_valid(u32 offset, int w_count)
{
	int range = (OTP_STRAP_EXT_REGION_SIZE) / 2;
	u16 ret[1];
	int i;

	if (offset + w_count > range)
		return OTP_USAGE;

	for (i = offset; i < offset + w_count; i++) {
		otp_read(STRAPEXT_REGION_START_ADDR + 0x8 + i, ret);
		printf("OTPSTRAPEXT_VLD0x%X: 0x%04X\n", i, ret[0]);
	}
	printf("\n");

	return OTP_SUCCESS;
}

static int otp_print_user_data(u32 offset, int w_count)
{
	int range = OTP_USER_REGION_SIZE;
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
	int range = OTP_SEC_REGION_SIZE;
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

static int otp_print_caliptra(u32 offset, int w_count)
{
	int range = OTP_CAL_REGION_SIZE;
	u16 ret[1];
	int i;

	if (offset + w_count > range)
		return OTP_USAGE;

	printf("Caliptra Region: 0x%x~0x%x\n", offset, offset + w_count);
	for (i = offset; i < offset + w_count; i++) {
		otp_read(CAL_REGION_START_ADDR + i, &ret[0]);
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
	int range = OTP_PUF_REGION_SIZE;
	u16 ret[1];
	int i;

	if (offset + w_count > range)
		return OTP_USAGE;

	printf("PUF: 0x%x~0x%x\n", offset, offset + w_count);
	for (i = offset; i < offset + w_count; i++) {
		otp_read(SW_PUF_REGION_START_ADDR + i, &ret[0]);
		if (i % 8 == 0)
			printf("\n%03X: %04X ", i * 2, ret[0]);
		else
			printf("%04X ", ret[0]);
	}
	printf("\n");

	return OTP_SUCCESS;
}

static int otp_prog_data(int mode, int otp_w_offset, int bit_offset,
			 int value, int nconfirm, bool debug)
{
	u32 prog_address;
	u16 read[1];
	int ret = 0;

	switch (mode) {
	case OTP_REGION_ROM:
		otp_read_rom(otp_w_offset, read);
		prog_address = ROM_REGION_START_ADDR + otp_w_offset;
		if (debug)
			printf("Program OTPROM%d[0x%X] = 0x%x...\n", otp_w_offset,
			       bit_offset, value);
		break;
	case OTP_REGION_CONF:
		otp_read_conf(otp_w_offset, read);
		prog_address = CONF_REGION_START_ADDR + otp_w_offset;
		if (debug)
			printf("Program OTPCFG%d[0x%X] = 0x%x...\n", otp_w_offset,
			       bit_offset, value);
		break;
	case OTP_REGION_STRAP:
		otp_read_strap(otp_w_offset, read);
		prog_address = STRAP_REGION_START_ADDR + otp_w_offset;
		if (debug)
			printf("Program OTPSTRAP%d[0x%X] = 0x%x...\n", otp_w_offset,
			       bit_offset, value);
		break;
	case OTP_REGION_STRAP_EXT:
		otp_read_strap_ext(otp_w_offset, read);
		prog_address = STRAPEXT_REGION_START_ADDR + otp_w_offset;
		if (debug)
			printf("Program OTPSTRAPEXT%d[0x%X] = 0x%x...\n", otp_w_offset,
			       bit_offset, value);
		break;
	case OTP_REGION_STRAP_EXT_VLD:
		otp_read_strap_ext_vld(otp_w_offset, read);
		prog_address = STRAPEXT_REGION_START_ADDR + 0x8 + otp_w_offset;
		if (debug)
			printf("Program OTPSTRAPEXT_VLD%d[0x%X] = 0x%x...\n", otp_w_offset,
			       bit_offset, value);
		break;
	case OTP_REGION_USER_DATA:
		otp_read_data(otp_w_offset, read);
		prog_address = USER_REGION_START_ADDR + otp_w_offset;
		if (debug)
			printf("Program OTPDATA%d[0x%X] = 0x%x...\n", otp_w_offset,
			       bit_offset, value);
		break;
	case OTP_REGION_SECURE:
		otp_read_secure(otp_w_offset, read);
		prog_address = SEC_REGION_START_ADDR + otp_w_offset;
		if (debug)
			printf("Program OTPSEC%d[0x%X] = 0x%x...\n", otp_w_offset,
			       bit_offset, value);
		break;
	case OTP_REGION_CALIPTRA:
		otp_read_caliptra(otp_w_offset, read);
		prog_address = CAL_REGION_START_ADDR + otp_w_offset;
		if (debug)
			printf("Program OTPCAL%d[0x%X] = 0x%x...\n", otp_w_offset,
			       bit_offset, value);
		break;
	default:
		printf("mode 0x%x is not supported\n", mode);
		return OTP_FAILURE;
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

	if (debug)
		printf("SUCCESS\n");

	return OTP_SUCCESS;
}

static void otp_strap_status(struct otpstrap_status *otpstrap)
{
	int strap_start, strap_end;
	u16 data[2];
	int ret;

	/* Initial otpstrap */
	for (int i = 0; i < 32; i++) {
		otpstrap[i].value = 0;
		otpstrap[i].remain_times = 6;
		otpstrap[i].writeable_option = -1;
		otpstrap[i].protected = 0;
	}

	/* Check OTP strap value */
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

	/* Check OTP strap write protect */
	ret = otp_read_strap(0, &data[0]);
	ret += otp_read_strap(1, &data[1]);
	if (ret)
		printf("OTP read strap failed, ret=0x%x\n", ret);

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

static int otp_strap_bit_confirm(struct otpstrap_status *otpstrap, int offset, int value, int pbit)
{
	int prog_flag = 0;

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

static void otp_print_strap_ext_info(void)
{
	const struct otpstrap_ext_info *strap_ext_info = info_cb.strap_ext_info;
	u32 bit_offset;
	u32 otp_value, otp_vld;
	u32 length;
	u16 data[8];
	u16 vld[8];

	/* Read Flash strap */
	for (int i = 0; i < 8; i++)
		otp_read_strap_ext(i, &data[i]);

	/* Read Flash strap valid */
	for (int i = 0; i < 8; i++)
		otp_read_strap_ext_vld(i, &vld[i]);

	printf("BIT(hex) Value  Valid   Description\n");
	printf("___________________________________________________________________________________________________\n");

	for (int i = 0; i < info_cb.strap_ext_info_len; i++) {
		otp_value = 0;
		otp_vld = 0;
		bit_offset = strap_ext_info[i].bit_offset;
		length = strap_ext_info[i].length;

		int w_offset = bit_offset / 16;
		int b_offset = bit_offset % 16;

		otp_value = (data[w_offset] >> b_offset) &
			    GENMASK(length - 1, 0);
		otp_vld = (vld[w_offset] >> b_offset) &
			  GENMASK(length - 1, 0);

		if (otp_value != strap_ext_info[i].value)
			continue;

		for (int j = 0; j < length; j++) {
			printf("0x%-7X", strap_ext_info[i].bit_offset + j);
			printf("0x%-5lX", (otp_value & BIT(j)) >> j);
			printf("0x%-5lX", (otp_vld & BIT(j)) >> j);

			if (length == 1) {
				printf(" %s\n", strap_ext_info[i].information);
				continue;
			}

			if (j == 0)
				printf("/%s\n", strap_ext_info[i].information);
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
	ret = otp_prog_data(OTP_REGION_CONF, 4, 1, offset, 1, true);
	if (ret) {
		printf("%s: Prog location Failed, ret:0x%x\n", __func__, ret);
		return ret;
	}

	/* Set Size - OTPCFG5[9:0] */
	ret = otp_prog_data(OTP_REGION_CONF, 5, 0, size, 1, true);
	if (ret) {
		printf("%s: Prog size Failed, ret:0x%x\n", __func__, ret);
		return ret;
	}

	/* enable pre_otp_patch_vld - OTPCFG4[0] */
	ret = otp_prog_data(OTP_REGION_CONF, 4, 0, 1, 1, true);
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
	ret = otp_prog_data(OTP_REGION_CONF, 6, 1, offset, 1, true);
	if (ret) {
		printf("%s: Prog location Failed, ret:0x%x\n", __func__, ret);
		return ret;
	}

	/* Set Size - OTPCFG7[9:0] */
	ret = otp_prog_data(OTP_REGION_CONF, 7, 0, size, 1, true);
	if (ret) {
		printf("%s: Prog size Failed, ret:0x%x\n", __func__, ret);
		return ret;
	}

	/* enable pre_otp_patch_vld - OTPCFG6[0] */
	ret = otp_prog_data(OTP_REGION_CONF, 6, 0, 1, 1, true);
	if (ret) {
		printf("%s: Enable post_otp_patch_vld Failed, ret:0x%x\n", __func__, ret);
		return ret;
	}

	return 0;
}

static void do_hash(const void *data, int data_len, const char *algo_name, uint8_t *value)
{
	struct hash_algo *algo;

	if (hash_lookup_algo(algo_name, &algo)) {
		printf("Unsupported hash algorithm\n");
		return;
	}

	algo->hash_func_ws(data, data_len, value, algo->chunk_size);
}

static int otp_verify_image(u8 *src_buf, u32 length, u8 *digest_buf)
{
	u8 digest_ret[48];
	int digest_len;

	do_hash(src_buf, length, "sha384", digest_ret);
	digest_len = 48;

	if (!memcmp(digest_buf, digest_ret, digest_len))
		return OTP_SUCCESS;

	printf("%s: digest should be:\n", __func__);
	buf_print(digest_ret, 48);
	return OTP_FAILURE;
}

static int otp_check_strap_image(struct otp_image_layout *image_layout,
				 struct otpstrap_status *otpstrap)
{
	int bit, pbit, ret;
	int fail = 0;
	u16 *strap;

	strap = (u16 *)image_layout->strap;

	for (int i = 0; i < 32; i++) {
		if (i < 16) {
			bit = (strap[0] >> i) & 0x1;
			pbit = (strap[2] >> i) & 0x1;
		} else {
			bit = (strap[1] >> (i - 16)) & 0x1;
			pbit = (strap[3] >> (i - 16)) & 0x1;
		}

		ret = otp_strap_bit_confirm(&otpstrap[i], i, bit, pbit);

		if (ret == OTP_FAILURE)
			fail = 1;
	}

	if (fail == 1) {
		printf("Input image can't program into OTP, please check.\n");
		return OTP_FAILURE;
	}

	return OTP_SUCCESS;
}

static int otp_print_rom_image(struct otp_image_layout *image_layout)
{
	u32 *buf;
	int size;

	buf = (u32 *)image_layout->rom;
	size = image_layout->rom_length;

	for (int i = 0; i < size / 4; i++) {
		if (i % 4 == 0)
			printf("\n%04x:", i * 4);
		printf(" %08x", buf[i]);
	}
	printf("\n");

	return 0;
}

static int _otp_print_key(u32 header, u32 offset, u8 *data)
{
	const struct otpkey_type *key_info_array = info_cb.key_info;
	struct otpkey_type key_info = { .value = -1 };
	int key_id, key_w_offset, key_offset, key_type;
	int last;
	int i;

	key_id = OTP_KH_KEY_ID(header);
	key_w_offset = OTP_KH_OFFSET(header);
	key_offset = key_w_offset * 2;
	key_type = OTP_KH_KEY_TYPE(header);
	last = OTP_KH_LAST(header);

	printf("\nKey[%d]:\n", offset);
	printf("Header: %x\n", header);

	for (i = 0; i < info_cb.key_info_len; i++) {
		if (key_type == key_info_array[i].value) {
			key_info = key_info_array[i];
			break;
		}
	}

	if (i == info_cb.key_info_len) {
		printf("Error: Cannot find the key type\n");
		return -1;
	}

	printf("Key Type: ");
	printf("%s\n", key_info.information);
	printf("Key Number ID: %d\n", key_id);
	printf("Key Word Offset: 0x%x\n", key_w_offset);
	if (last)
		printf("This is the last key\n");

	if (!data)
		return -1;

	printf("Key Value:\n");
	if (key_info.key_type == SOC_ECDSA_PUB) {
		printf("Q.x:\n");
		buf_print(&data[key_offset], 0x30);
		printf("Q.y:\n");
		buf_print(&data[key_offset + 0x30], 0x30);

	} else if (key_info.key_type == SOC_LMS_PUB) {
		printf("LMS (todo):\n");

	} else if (key_info.key_type == CAL_MANU_PUB_HASH) {
		buf_print(&data[key_offset], 0x30);
		printf("Manufacture ECC Key Mask: 0x%x\n", data[key_offset + 0x30]);
		printf("Manufacture LMS Key Mask: 0x%x\n", data[key_offset + 0x34]);

	} else if (key_info.key_type == CAL_OWN_PUB_HASH) {
		buf_print(&data[key_offset], 0x30);

	} else if (key_info.key_type == SOC_VAULT || key_info.key_type == SOC_VAULT_SEED) {
		buf_print(&data[key_offset], 0x20);
	}

	return 0;
}

static void otp_print_key(u32 *data)
{
	u8 *byte_buf;
	int empty;
	int ret;

	byte_buf = (u8 *)data;
	empty = 1;

	for (int i = 0; i < OTP_KH_NUM; i++) {
		if (data[i] != 0)
			empty = 0;
	}

	if (empty) {
		printf("OTP data header is empty\n");
		return;
	}

	for (int i = 0; i < OTP_KH_NUM; i++) {
		ret = _otp_print_key(data[i], i, byte_buf);
		if (ret)
			break;
	}
}

static void otp_print_key_info(void)
{
	u16 buf[OTP_SEC_REGION_SIZE];

	otp_read_secure_multi(0, buf, OTP_SEC_REGION_SIZE);
	otp_print_key((u32 *)buf);
}

static int otp_print_secure_image(struct otp_image_layout *image_layout)
{
	u32 *buf;

	buf = (u32 *)image_layout->secure;
	otp_print_key(buf);

	return OTP_SUCCESS;
}

static int otp_print_conf_image(struct otp_image_layout *image_layout)
{
	const struct otpconf_info *conf_info = info_cb.conf_info;
	u16 *OTPCFG = (u16 *)image_layout->conf;
	u32 w_offset;
	u32 bit_offset;
	u32 otp_value;
	u32 mask;

	printf("Word    Bit        Value       Description\n");
	printf("__________________________________________________________________________\n");
	for (int i = 0; i < info_cb.conf_info_len; i++) {
		w_offset = conf_info[i].w_offset;
		bit_offset = conf_info[i].bit_offset;
		mask = BIT(conf_info[i].length) - 1;
		otp_value = (OTPCFG[w_offset] >> bit_offset) & mask;

		if (!otp_value)
			continue;

		if (conf_info[i].value != OTP_REG_VALUE && otp_value != conf_info[i].value)
			continue;

		printf("0x%-4X", w_offset);

		if (conf_info[i].length == 1) {
			printf("0x%-9X", conf_info[i].bit_offset);
		} else {
			printf("0x%-2X:0x%-4X",
			       conf_info[i].bit_offset + conf_info[i].length - 1,
			       conf_info[i].bit_offset);
		}
		printf("0x%-10x", otp_value);

		if (conf_info[i].value == OTP_REG_RESERVED) {
			printf("Reserved\n");

		} else if (conf_info[i].value == OTP_REG_VALUE) {
			printf(conf_info[i].information, otp_value);
			printf("\n");

		} else {
			printf("%s\n", conf_info[i].information);
		}
	}

	return OTP_SUCCESS;
}

static int otp_print_strap_image(struct otp_image_layout *image_layout)
{
	const struct otpstrap_info *strap_info = info_cb.strap_info;
	u16 *OTPSTRAP;
	u16 *OTPSTRAP_PRO;
	u32 w_offset;
	u32 bit_offset;
	u32 otp_value;
	u32 otp_protect;
	u32 mask;

	OTPSTRAP = (u16 *)image_layout->strap;
	OTPSTRAP_PRO = OTPSTRAP + 2;

	printf("Bit(hex)   Value       Protect     Description\n");
	printf("__________________________________________________________________________________________\n");

	for (int i = 0; i < info_cb.strap_info_len; i++) {
		if (strap_info[i].bit_offset > 15) {
			w_offset = 1;
			bit_offset = strap_info[i].bit_offset - 16;
		} else {
			w_offset = 0;
			bit_offset = strap_info[i].bit_offset;
		}

		mask = BIT(strap_info[i].length) - 1;
		otp_value = (OTPSTRAP[w_offset] >> bit_offset) & mask;
		otp_protect = (OTPSTRAP_PRO[w_offset] >> bit_offset) & mask;

		if (otp_value != strap_info[i].value)
			continue;

		if (strap_info[i].length == 1) {
			printf("0x%-9X", strap_info[i].bit_offset);
		} else {
			printf("0x%-2X:0x%-4X",
			       strap_info[i].bit_offset + strap_info[i].length - 1,
			       strap_info[i].bit_offset);
		}
		printf("0x%-10x", otp_value);
		printf("0x%-10x", otp_protect);
		printf("%s\n", strap_info[i].information);
	}

	return OTP_SUCCESS;
}

static int otp_print_strap_ext_image(struct otp_image_layout *image_layout)
{
	const struct otpstrap_ext_info *strap_ext_info = info_cb.strap_ext_info;
	u16 *OTPSTRAP_EXT;
	u16 *OTPSTRAP_EXT_VLD;
	u32 w_offset;
	u32 bit_offset;
	u32 otp_value;
	u32 otp_valid;
	u32 mask;

	OTPSTRAP_EXT = (u16 *)image_layout->strap_ext;
	OTPSTRAP_EXT_VLD = OTPSTRAP_EXT + 8;

	printf("Bit(hex)   Value       Valid     Description\n");
	printf("__________________________________________________________________________________________\n");

	for (int i = 0; i < info_cb.strap_ext_info_len; i++) {
		w_offset = strap_ext_info[i].bit_offset / 16;
		bit_offset = strap_ext_info[i].bit_offset % 16;

		mask = BIT(strap_ext_info[i].length) - 1;
		otp_value = (OTPSTRAP_EXT[w_offset] >> bit_offset) & mask;
		otp_valid = (OTPSTRAP_EXT_VLD[w_offset] >> bit_offset) & mask;

		if (!otp_value && !otp_valid)
			continue;

		if (otp_value != strap_ext_info[i].value)
			continue;

		if (strap_ext_info[i].length == 1) {
			printf("0x%-9X", strap_ext_info[i].bit_offset);
		} else {
			printf("0x%-2X:0x%-4X",
			       strap_ext_info[i].bit_offset + strap_ext_info[i].length - 1,
			       strap_ext_info[i].bit_offset);
		}
		printf("0x%-10x", otp_value);
		printf("0x%-10x", otp_valid);
		printf("%s\n", strap_ext_info[i].information);
	}

	return OTP_SUCCESS;
}

static int otp_print_caliptra_image(struct otp_image_layout *image_layout)
{
	const struct otpcal_info *cal_info = info_cb.cal_info;
	u16 *OTPCAL = (u16 *)image_layout->caliptra;
	u32 w_offset;
	u32 bit_offset;
	u32 otp_value;
	u32 mask;

	printf("Word    Bit        Value       Description\n");
	printf("__________________________________________________________________________\n");
	for (int i = 0; i < info_cb.cal_info_len; i++) {
		w_offset = cal_info[i].w_offset;
		bit_offset = cal_info[i].bit_offset;
		mask = BIT(cal_info[i].length) - 1;
		otp_value = (OTPCAL[w_offset] >> bit_offset) & mask;

		if (!otp_value)
			continue;

		printf("0x%-4X", w_offset);

		if (cal_info[i].length == 1) {
			printf("0x%-9X", cal_info[i].bit_offset);
		} else {
			printf("0x%-2X:0x%-4X",
			       cal_info[i].bit_offset + cal_info[i].length - 1,
			       cal_info[i].bit_offset);
		}
		printf("0x%-10x", otp_value);

		if (cal_info[i].value == OTP_REG_RESERVED) {
			printf("Reserved\n");

		} else if (cal_info[i].value == OTP_REG_VALUE) {
			printf(cal_info[i].information, otp_value);
			printf("\n");

		} else {
			printf("%s\n", cal_info[i].information);
		}
	}

	return OTP_SUCCESS;
}

static int otp_prog_image_region(struct otp_image_layout *image_layout, enum otp_region region_type)
{
	int (*otp_read_func)(u32 offset, u16 *data);
	u16 otp_value;
	u16 *buf;
	int size, w_region_size;
	int ret;

	switch (region_type) {
	case OTP_REGION_ROM:
		buf = (u16 *)image_layout->rom;
		size = image_layout->rom_length;
		w_region_size = OTP_ROM_REGION_SIZE;
		otp_read_func = otp_read_rom;
		break;
	case OTP_REGION_CONF:
		buf = (u16 *)image_layout->conf;
		size = image_layout->conf_length;
		w_region_size = OTP_CONF_REGION_SIZE;
		otp_read_func = otp_read_conf;
		break;
	case OTP_REGION_SECURE:
		buf = (u16 *)image_layout->secure;
		size = image_layout->secure_length;
		w_region_size = OTP_SEC_REGION_SIZE;
		otp_read_func = otp_read_secure;
		break;
	case OTP_REGION_CALIPTRA:
		buf = (u16 *)image_layout->caliptra;
		size = image_layout->caliptra_length;
		w_region_size = OTP_CAL_REGION_SIZE;
		otp_read_func = otp_read_caliptra;
		break;
	default:
		printf("%s: region type 0x%x is not supported\n", __func__, region_type);
		return OTP_FAILURE;
	}

	if (size != w_region_size * 2) {
		printf("image size is mismatch, size:0x%x, should be:0x%x\n",
		       size, w_region_size * 2);
		return OTP_FAILURE;
	}

	printf("Start Programing...\n");
	for (int i = 0; i < size / 2; i++) {
		otp_read_func(i, &otp_value);
		if (otp_value) {
			if (otp_value != buf[i])
				printf("Warning: OTP region w_offset [0x%x]=0x%x prog to 0x%x\n",
				       i, otp_value, buf[i]);
			continue;
		} else {
			ret = otp_prog_data(region_type, i, 0, buf[i], 1, false);
			if (ret) {
				printf("%s: Prog Failed, ret:0x%x\n", __func__, ret);
				return ret;
			}
		}
	}
	printf("Done\n");

	return OTP_SUCCESS;
}

static int otp_prog_strap_image(struct otp_image_layout *image_layout,
				struct otpstrap_status *otpstrap)
{
	u32 *strap;
	u32 *strap_pro;
	u32 w_offset;
	int bit, pbit, offset;
	int fail = 0;
	int prog_flag = 0;
	int bit_offset;
	int ret;

	strap = (u32 *)image_layout->strap;
	strap_pro = strap + 1;

	printf("Start Programing...\n");
	for (int i = 0; i < 32; i++) {
		offset = i;
		bit = (strap[0] >> offset) & 0x1;
		bit_offset = i % 16;
		pbit = (strap_pro[0] >> offset) & 0x1;
		if (i < 16)
			w_offset = otpstrap[i].writeable_option * 2 + 2;
		else
			w_offset = otpstrap[i].writeable_option * 2 + 3;

		if (bit == otpstrap[i].value)
			prog_flag = 0;
		else
			prog_flag = 1;

		if (otpstrap[i].protected == 1 && prog_flag) {
			fail = 1;
			printf("Warning: OTPSTRAP[0x%x] is protected, cannot be programmed\n", i);
			continue;
		}
		if (otpstrap[i].remain_times == 0 && prog_flag) {
			fail = 1;
			printf("Warning: OTPSTRAP[0x%x] no remain times\n", i);
			continue;
		}

		if (prog_flag) {
			ret = otp_prog_data(OTP_REGION_STRAP, w_offset, bit_offset, bit, 1, false);
			if (ret)
				return OTP_FAILURE;
		}

		if (pbit) {
			if (i < 16)
				w_offset = 0;
			else
				w_offset = 1;

			ret = otp_prog_data(OTP_REGION_STRAP, w_offset, bit_offset, pbit, 1, false);
			if (ret)
				return OTP_FAILURE;
		}
	}

	if (fail == 1)
		return OTP_FAILURE;

	printf("Done\n");
	return OTP_SUCCESS;
}

static int otp_prog_strap_ext_image(struct otp_image_layout *image_layout)
{
	u16 *strap_ext;
	u16 *strap_ext_vld;
	int w_offset, bit_offset;
	int bit, vbit;
	int fail = 0;
	int ret;

	strap_ext = (u16 *)image_layout->strap_ext;
	strap_ext_vld = strap_ext + 8;

	printf("Start Programing...\n");
	for (int i = 0; i < 128; i++) {
		w_offset = i / 16;
		bit_offset = i % 16;
		bit = (strap_ext[w_offset] >> bit_offset) & 0x1;
		vbit = (strap_ext_vld[w_offset] >> bit_offset) & 0x1;

		if (bit) {
			ret = otp_prog_data(OTP_REGION_STRAP_EXT, w_offset, bit_offset, 1, 1,
					    false);
			if (ret)
				return OTP_FAILURE;
		}

		if (vbit) {
			ret = otp_prog_data(OTP_REGION_STRAP_EXT_VLD, w_offset, bit_offset, 1, 1,
					    false);
			if (ret)
				return OTP_FAILURE;
		}
	}

	if (fail == 1)
		return OTP_FAILURE;

	printf("Done\n");
	return OTP_SUCCESS;
}

static int otp_prog_image(phys_addr_t addr, int nconfirm)
{
	struct otp_image_layout image_layout;
	struct otpstrap_status otpstrap[32];
	struct otp_header *otp_header;
	int image_soc_ver = 0;
	int image_size;
	int ret;
	u8 *checksum;
	u8 *buf;

	otp_header = map_physmem(addr, sizeof(struct otp_header), MAP_WRBACK);
	if (!otp_header) {
		printf("Failed to map physical memory\n");
		return OTP_FAILURE;
	}

	image_size = OTP_IMAGE_SIZE(otp_header->image_info);
	// printf("image_size: 0x%x\n", image_size);
	unmap_physmem(otp_header, MAP_WRBACK);

	buf = map_physmem(addr, image_size + CHECKSUM_LEN, MAP_WRBACK);

	if (!buf) {
		printf("Failed to map physical memory\n");
		return OTP_FAILURE;
	}
	otp_header = (struct otp_header *)buf;
	checksum = buf + otp_header->checksum_offset;

	/* Check Image Magic */
	if (strcmp(OTP_MAGIC, (char *)otp_header->otp_magic) != 0) {
		printf("Image is invalid\n");
		return OTP_FAILURE;
	}

	image_layout.rom_length = OTP_REGION_SIZE(otp_header->rom_info);
	image_layout.rom = buf + OTP_REGION_OFFSET(otp_header->rom_info);

	image_layout.conf_length = OTP_REGION_SIZE(otp_header->config_info);
	image_layout.conf = buf + OTP_REGION_OFFSET(otp_header->config_info);

	image_layout.strap_length = OTP_REGION_SIZE(otp_header->strap_info);
	image_layout.strap = buf + OTP_REGION_OFFSET(otp_header->strap_info);

	image_layout.strap_ext_length = OTP_REGION_SIZE(otp_header->strap_ext_info);
	image_layout.strap_ext = buf + OTP_REGION_OFFSET(otp_header->strap_ext_info);

	image_layout.secure_length = OTP_REGION_SIZE(otp_header->secure_info);
	image_layout.secure = buf + OTP_REGION_OFFSET(otp_header->secure_info);

	image_layout.caliptra_length = OTP_REGION_SIZE(otp_header->caliptra_info);
	image_layout.caliptra = buf + OTP_REGION_OFFSET(otp_header->caliptra_info);

	if (otp_header->soc_ver == SOC_AST2700A0) {
		image_soc_ver = OTP_AST2700_A0;
	} else if (otp_header->soc_ver == SOC_AST2700A1) {
		image_soc_ver = OTP_AST2700_A1;
	} else {
		printf("Image SOC Version is not supported\n");
		return OTP_FAILURE;
	}

	if (image_soc_ver != info_cb.version) {
		printf("Image SOC version is not match to HW SOC version\n");
		return OTP_FAILURE;
	}

	// printf("buf addr:0x%llx, checksum addr: 0x%llx\n", buf, checksum);
	ret = otp_verify_image(buf, image_size, checksum);
	if (ret) {
		printf("checksum is invalid\n");
		return OTP_FAILURE;
	}

	if (info_cb.pro_sts.fields.mem_lock) {
		printf("OTP memory is locked\n");
		return OTP_FAILURE;
	}

	ret = 0;
	if (otp_header->image_info & OTP_INC_ROM) {
		if (info_cb.pro_sts.fields.w_prot_rom) {
			printf("OTP rom region is write protected\n");
			ret = -1;
		}
	}

	if (otp_header->image_info & OTP_INC_CONFIG) {
		if (info_cb.pro_sts.fields.w_prot_conf) {
			printf("OTP config region is write protected\n");
			ret = -1;
		}
	}

	if (otp_header->image_info & OTP_INC_STRAP) {
		if (info_cb.pro_sts.fields.w_prot_strap) {
			printf("OTP strap region is write protected\n");
			ret = -1;
		}
		printf("Read OTP Strap Region:\n");
		otp_strap_status(otpstrap);

		printf("Check writable...\n");
		if (otp_check_strap_image(&image_layout, otpstrap) == OTP_FAILURE)
			ret = -1;
	}

	if (otp_header->image_info & OTP_INC_STRAP_EXT) {
		if (info_cb.pro_sts.fields.w_prot_strap_ext) {
			printf("OTP strap extension region is write protected\n");
			ret = -1;
		}
	}

	if (otp_header->image_info & OTP_INC_CALIPTRA) {
		if (info_cb.pro_sts.fields.w_prot_cal) {
			printf("OTP caliptra region is write protected\n");
			ret = -1;
		}
	}

	if (ret == -1)
		return OTP_FAILURE;

	if (!nconfirm) {
		if (otp_header->image_info & OTP_INC_ROM) {
			printf("\nOTP ROM region:\n");
			if (otp_print_rom_image(&image_layout) < 0) {
				printf("OTP print rom error, please check.\n");
				return OTP_FAILURE;
			}
		}
		if (otp_header->image_info & OTP_INC_SECURE) {
			printf("\nOTP secure region :\n");
			if (otp_print_secure_image(&image_layout) < 0) {
				printf("OTP print secure error, please check.\n");
				return OTP_FAILURE;
			}
		}
		if (otp_header->image_info & OTP_INC_CONFIG) {
			printf("\nOTP configuration region :\n");
			if (otp_print_conf_image(&image_layout) < 0) {
				printf("OTP config error, please check.\n");
				return OTP_FAILURE;
			}
		}
		if (otp_header->image_info & OTP_INC_STRAP) {
			printf("\nOTP strap region :\n");
			if (otp_print_strap_image(&image_layout) < 0) {
				printf("OTP strap error, please check.\n");
				return OTP_FAILURE;
			}
		}
		if (otp_header->image_info & OTP_INC_STRAP_EXT) {
			printf("\nOTP strap extension region :\n");
			if (otp_print_strap_ext_image(&image_layout) < 0) {
				printf("OTP strap_ext error, please check.\n");
				return OTP_FAILURE;
			}
		}
		if (otp_header->image_info & OTP_INC_CALIPTRA) {
			printf("\nOTP caliptra region :\n");
			if (otp_print_caliptra_image(&image_layout) < 0) {
				printf("OTP caliptra error, please check.\n");
				return OTP_FAILURE;
			}
		}

		printf("type \"YES\" (no quotes) to continue:\n");
		if (!confirm_yesno()) {
			printf(" Aborting\n");
			return OTP_FAILURE;
		}
	}

	if (otp_header->image_info & OTP_INC_ROM) {
		printf("programing rom region ...\n");
		ret = otp_prog_image_region(&image_layout, OTP_REGION_ROM);
		if (ret) {
			printf("Error\n");
			return ret;
		}
		// printf("Done\n");
	}
	if (otp_header->image_info & OTP_INC_SECURE) {
		printf("programing secure region ...\n");
		ret = otp_prog_image_region(&image_layout, OTP_REGION_SECURE);
		if (ret) {
			printf("Error\n");
			return ret;
		}
		//printf("Done\n");
	}
	if (otp_header->image_info & OTP_INC_CONFIG) {
		printf("programing configuration region ...\n");
		ret = otp_prog_image_region(&image_layout, OTP_REGION_CONF);
		if (ret != 0) {
			printf("Error\n");
			return ret;
		}
		//printf("Done\n");
	}
	if (otp_header->image_info & OTP_INC_STRAP) {
		printf("programing strap region ...\n");
		ret = otp_prog_strap_image(&image_layout, otpstrap);
		if (ret != 0) {
			printf("Error\n");
			return ret;
		}
		//printf("Done\n");
	}
	if (otp_header->image_info & OTP_INC_STRAP_EXT) {
		printf("programing strap extension region ...\n");
		ret = otp_prog_strap_ext_image(&image_layout);
		if (ret != 0) {
			printf("Error\n");
			return ret;
		}
		//printf("Done\n");
	}
	if (otp_header->image_info & OTP_INC_CALIPTRA) {
		printf("programing caliptra region ...\n");
		ret = otp_prog_image_region(&image_layout, OTP_REGION_CALIPTRA);
		if (ret != 0) {
			printf("Error\n");
			return ret;
		}
		//printf("Done\n");
	}

	return OTP_SUCCESS;
}

static int do_otpinfo(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	if (argc != 2 && argc != 3)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1], "strap"))
		otp_print_strap_info();
	else if (!strcmp(argv[1], "strap-ext"))
		otp_print_strap_ext_info();
	else if (!strcmp(argv[1], "key"))
		otp_print_key_info();
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
	else if (!strcmp(argv[1], "strap-pro"))
		ret = otp_print_strap_pro(offset, count);
	else if (!strcmp(argv[1], "strap-ext"))
		ret = otp_print_strap_ext(offset, count);
	else if (!strcmp(argv[1], "strap-ext-vld"))
		ret = otp_print_strap_ext_valid(offset, count);
	else if (!strcmp(argv[1], "u-data"))
		ret = otp_print_user_data(offset, count);
	else if (!strcmp(argv[1], "s-data"))
		ret = otp_print_sec_data(offset, count);
	else if (!strcmp(argv[1], "caliptra"))
		ret = otp_print_caliptra(offset, count);
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

static int do_otpprog(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	phys_addr_t addr;
	int ret;

	if (argc == 3) {
		if (strcmp(argv[1], "o"))
			return CMD_RET_USAGE;

		addr = simple_strtoul(argv[2], NULL, 16);
		ret = otp_prog_image(addr, 1);

	} else if (argc == 2) {
		addr = simple_strtoul(argv[1], NULL, 16);
		ret = otp_prog_image(addr, 0);

	} else {
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
	else if (!strcmp(argv[0], "strap-ext"))
		mode = OTP_REGION_STRAP_EXT;
	else if (!strcmp(argv[0], "strap-ext-vld"))
		mode = OTP_REGION_STRAP_EXT_VLD;
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

	} else if (mode == OTP_REGION_STRAP_EXT) {
		bit_offset = simple_strtoul(argv[0], NULL, 16);
		value = simple_strtoul(argv[1], NULL, 16);
		if (bit_offset >= 128)
			return CMD_RET_USAGE;
		if (value != 1)
			return CMD_RET_USAGE;

	} else if (mode == OTP_REGION_STRAP_EXT_VLD) {
		if (!strcmp(argv[0], "all")) {
			for (int i = 0; i < 8; i++) {
				ret = otp_prog_data(mode, i, 0, GENMASK(15, 0), 1, true);
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
		if (otp_addr >= OTP_CONF_REGION_SIZE)
			return CMD_RET_USAGE;

	} else if (mode == OTP_REGION_STRAP) {
		// get otpstrap status
		otp_strap_status(otpstrap);

		ret = otp_strap_bit_confirm(&otpstrap[bit_offset], bit_offset, value, 0);
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

	} else if (mode == OTP_REGION_STRAP_EXT || mode == OTP_REGION_STRAP_EXT_VLD) {
		otp_addr = bit_offset / 16;
		bit_offset = bit_offset % 16;

	} else {
		/* user data + secure data region */
		if (otp_addr >= OTP_USER_REGION_SIZE + OTP_SEC_REGION_SIZE)
			return CMD_RET_USAGE;
	}

	ret = otp_prog_data(mode, otp_addr, bit_offset, value, nconfirm, true);

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
	U_BOOT_CMD_MKENT(prog, 3, 0, do_otpprog, "", ""),
	U_BOOT_CMD_MKENT(pb, 6, 0, do_otppb, "", ""),
	U_BOOT_CMD_MKENT(patch, 5, 0, do_otppatch, "", ""),
	U_BOOT_CMD_MKENT(ecc, 2, 0, do_otpecc, "", ""),
	U_BOOT_CMD_MKENT(info, 3, 0, do_otpinfo, "", ""),
};

static int do_driver_init(int argc, char *const argv[])
{
	int index;
	int ret;
	char *name;

	index = simple_strtoul(argv[1], NULL, 10);

	switch (index) {
	case 0:
		name = OTP_DEVICE_NAME_0;
		break;
	case 1:
		name = OTP_DEVICE_NAME_1;
		break;
	default:
		printf("Unknown OTP device index %d\n", index);
		return CMD_RET_USAGE;
	}

	ret = uclass_get_device_by_name(UCLASS_MISC, name, &otp_dev);
	if (ret)
		printf("%s: get Aspeed otp driver failed\n", __func__);

	return ret;
}

static int do_ast_otp(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	union otp_pro_sts *pro_sts;
	struct cmd_tbl *cp;
	u32 ver;
	int ret;
	u16 otp_conf0;

	ret = do_driver_init(argc, argv);
	if (ret)
		return ret;

	/* Drop the otp command */
	argc--;
	argv++;

	cp = find_cmd_tbl(argv[1], cmd_otp, ARRAY_SIZE(cmd_otp));

	/* Drop the otp command */
	argc--;
	argv++;

	if (!cp || argc > cp->maxargs)
		return CMD_RET_USAGE;
	if (flag == CMD_FLAG_REPEAT && !cmd_is_repeatable(cp))
		return CMD_RET_SUCCESS;

	ver = chip_version();
	switch (ver) {
	case OTP_AST2700_A0:
		printf("Chip: AST2700-A0\n");
		info_cb.version = OTP_AST2700_A0;
		info_cb.strap_info = a0_strap_info;
		info_cb.strap_info_len = ARRAY_SIZE(a0_strap_info);
		info_cb.strap_ext_info = a0_strap_ext_info;
		info_cb.strap_ext_info_len = ARRAY_SIZE(a0_strap_ext_info);
		break;
	case OTP_AST2700_A1:
		printf("Chip: AST2700-A1\n");
		info_cb.version = OTP_AST2700_A1;
		info_cb.conf_info = a1_conf_info;
		info_cb.conf_info_len = ARRAY_SIZE(a1_conf_info);
		info_cb.strap_info = a1_strap_info;
		info_cb.strap_info_len = ARRAY_SIZE(a1_strap_info);
		info_cb.strap_ext_info = a1_strap_ext_info;
		info_cb.strap_ext_info_len = ARRAY_SIZE(a1_strap_ext_info);
		info_cb.cal_info = a1_cal_info;
		info_cb.cal_info_len = ARRAY_SIZE(a1_cal_info);
		info_cb.key_info = a1_key_type;
		info_cb.key_info_len = ARRAY_SIZE(a1_key_type);
		break;
	default:
		printf("SOC is not supported\n");
		return CMD_RET_FAILURE;
	}

	otp_read_conf(0, &otp_conf0);
	pro_sts = &info_cb.pro_sts;
	pro_sts->value = otp_conf0;

	ret = cp->cmd(cmdtp, flag, argc, argv);

	return ret;
}

U_BOOT_CMD(otp, 7, 0,  do_ast_otp,
	   "ASPEED One-Time-Programmable sub-system",
	   "<dev> version\n"
	   "otp <dev> read rom|conf|strap|strap-pro|strap-ext|strap-ext-vld|u-data|s-data|caliptra|puf <otp_w_offset> <w_count>\n"
	   "otp <dev> pb conf|data [o] <otp_w_offset> <bit_offset> <value>\n"
	   "otp <dev> pb strap|strap-ext|strap-ext-vld [o] <bit_offset> <value>\n"
	   "otp <dev> pb strap-ext-vld all\n"
	   "otp <dev> prog <addr>\n"
	   "otp <dev> info key\n"
	   "otp <dev> info strap|strap-ext\n"
	   "otp <dev> patch prog <dram_addr> <otp_w_offset> <w_count>\n"
	   "otp <dev> patch enable pre|post <otp_start_w_offset> <w_count>\n"
	   "otp <dev> ecc status|enable\n"
	  );
