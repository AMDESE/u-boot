// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include <command.h>
#include <common.h>
#include <env.h>
#include <i2c.h>
#include <dm/uclass.h>
#include <net.h>
#include <hexdump.h>
#include <errno.h>
#include <asm/io.h>
#include <stdlib.h>
#include <asm/gpio.h>
#include <linux/delay.h>

// HPM power sequence gpio(s)
#define HPM_RST_GPIO   19  // RST_L
#define HPM_EN_GPIO    20  // EN
#define HPM_RDY_GPIO   21  // RDY

#define SCM_EEPROM_I2C_BUS    (7)
#define HPM_EEPROM_I2C_BUS    (8)
#define SCM_EEPROM_OFF_LEN    (1) // AT24C08C
#define HPM_EEPROM_OFF_LEN    (2) // AT24C32E
#define EEPROM_DEV_ADDR    (0x50)
#define EEPROM_BUF_LEN    (0x400)
#define MAC_ADDR_LEN          (6)
#define MAC0_ADDR_OFFSET     (16)
#define MAC1_ADDR_OFFSET     (24)
#define MAC2_ADDR_OFFSET     (32)
#define LAST_2B_MAC0_ADDR    (20)
#define LAST_2B_MAC0_LEN      (2)
#define SCM_BOM_VARIANT_OFF  (79) // QSPI or eSPI variant
#define ESPI_VARIANT_BIT      (1)

#define FRU_MRC_HDR_OFFSET    (5)
#define MRC_HDR_AREA_START    (8)
#define HPM_BRD_ID_OFFSET    (18)
#define HPM_BRD_REV_OFFSET   (19)
#define STR_BUF_LEN         (128)
#define FRU_HDR_OFF_MULTIPLIER  8
#define CIA_START_OFFSET        2
#define CIA_HDR_SIZE            3
#define FRU_FIELD_TYPE_LEN_SIZE 1
#define FRU_FIELD_TYPE_LEN_MASK 0x3F
#define MIN_CSN_UNIQ_STR_LEN    4
#define HYPHEN_DELIM_SIZE       1
#define HYPHEN_DELIM            '-'

#define ENV_BOOTARGS           "bootargs"
#define ENV_BOARD_FIT_CONF   "board_conf"
#define ENV_BOARD_ID           "board_id"
#define ENV_BOARD_REV         "board_rev"
#define ENV_DT_NAME              "dtname"
#define ENV_ETH_ADDR            "ethaddr"
#define ENV_ETH1_ADDR          "eth1addr"
#define ENV_ETH2_ADDR          "eth2addr"
#define ENV_SOC_ID               "soc_id"

/* Sys Scratch reg that holds sys_rst info, refer cpu-info.c */
#define ASPEED_SYS_SCRATCH_7FC 0x12C027FC
#define SYS_SRST               BIT(0)

/* HPM Power-on Retry */
#define HPM_STBY_EN_RETRY      (50)
#define HPM_RDY_RTRY_INTRVL    (100000)
/* LTPI */
#define LTPI_TRAIN_RETRY       (50)
// 100ms for LTPI spec 1.1
#define ADVRT_TIMEOUT_US_1_1   "100000"
// 1ms for LTPI spec 1.0
#define ADVRT_TIMEOUT_US_1_0   "1000"
// 3x advt timeout for FPGA to move to LDFA state
#define OP_TIMEOUT_US          "300000"

/* SoC mapping Table */
#define SOC_ID(str, rev) { .name = str, .rev_id = rev, }

/* SP7 Circuit Type */
typedef enum {
	HCC_TYPE_1 = 0x01,
	HCC_TYPE_2 = 0x02,
	AMD_TYPE_SLT_1P = 0x03,
	AMD_TYPE_SLT_2P = 0x04,
	AMD_TYPE_2x1_P0 = 0x05,
	AMD_TYPE_2x1_P1 = 0x06,
	AMD_SP8_SLT_1P = 0x07,
	AMD_SP8_SLT_2P = 0x08,
	AMD_SB1_SLT_1P = 0x09,
} BoardType;

enum {
	ONE_LINK = 0,
	TWO_LINK = 1
};

/* SP7 Board info */
typedef struct {
	const char *name;
	int id;
	BoardType type;
	int ltpi_type;
} BoardInfo;

/* SP7 HPM boards (ordered by id)*/
static u8 board_id = 0x80; // Default - Congo

const BoardInfo boards[] = {
	/*  Name        ID      Type                LTPI Type */
	{ "marley",     0x79,   HCC_TYPE_2,         ONE_LINK },
	{ "marley",     0x7A,   HCC_TYPE_2,         ONE_LINK },
	{ "marley",     0x7B,   HCC_TYPE_2,         ONE_LINK },
	{ "mojanda",    0x7C,   HCC_TYPE_2,         ONE_LINK },
	{ "mojanda",    0x7D,   HCC_TYPE_2,         ONE_LINK },
	{ "mojanda",    0x7E,   HCC_TYPE_2,         ONE_LINK },
	{ "congo",      0x80,   HCC_TYPE_1,         ONE_LINK },
	{ "congo",      0x81,   HCC_TYPE_1,         ONE_LINK },
	{ "morocco",    0x82,   HCC_TYPE_2,         ONE_LINK },
	{ "morocco",    0x83,   HCC_TYPE_2,         ONE_LINK },
	{ "kenya",      0x84,   HCC_TYPE_1,         ONE_LINK },
	{ "nigeria",    0x85,   HCC_TYPE_2,         ONE_LINK },
	{ "congo",      0x86,   HCC_TYPE_1,         ONE_LINK },
	{ "morocco",    0x87,   HCC_TYPE_2,         ONE_LINK },
	{ "senegal",    0x88,   AMD_TYPE_SLT_1P,    ONE_LINK },
	{ "sahara",     0x89,   AMD_TYPE_SLT_1P,    ONE_LINK },
	{ "malawi",     0x8A,   AMD_TYPE_SLT_2P,    ONE_LINK },
	{ "zambia",     0x8B,   AMD_TYPE_SLT_1P,    ONE_LINK },
	{ "zimbabwe",   0x8C,   AMD_TYPE_SLT_1P,    ONE_LINK },
	{ "zanzibar",   0x8D,   AMD_TYPE_SLT_1P,    ONE_LINK },
	{ "ghana",      0x8E,   HCC_TYPE_2,         ONE_LINK },
	{ "morocco",    0x8F,   HCC_TYPE_2,         ONE_LINK },
	{ "morocco",    0x90,   HCC_TYPE_2,         ONE_LINK },
	{ "nigeria",    0x91,   HCC_TYPE_2,         ONE_LINK },
	{ "nigeria",    0x92,   HCC_TYPE_2,         ONE_LINK },
	{ "ghana",      0x93,   HCC_TYPE_2,         ONE_LINK },
	{ "ghana",      0x94,   HCC_TYPE_2,         ONE_LINK },
	{ "malawi",     0x95,   AMD_TYPE_SLT_2P,    ONE_LINK },
	{ "malawi",     0x96,   AMD_TYPE_SLT_2P,    ONE_LINK },
	{ "nigerias3",  0x97,   HCC_TYPE_2,         TWO_LINK },
	{ "nigerias3",  0x98,   HCC_TYPE_2,         TWO_LINK },
	{ "nigerias3",  0x99,   HCC_TYPE_2,         TWO_LINK },
	{ "morocco",    0x9A,   HCC_TYPE_2,         ONE_LINK },
	{ "morocco",    0x9B,   HCC_TYPE_2,         ONE_LINK },
	{ "morocco",    0x9C,   HCC_TYPE_2,         ONE_LINK },
	{ "morocco",    0x9D,   HCC_TYPE_2,         ONE_LINK },
	{ "zaire",      0x9E,   AMD_TYPE_SLT_1P,    ONE_LINK },
	{ "eagle",      0x9F,   HCC_TYPE_1,         ONE_LINK },
	{ "eagle",      0xA0,   HCC_TYPE_1,         ONE_LINK },
	{ "eagle",      0xA1,   HCC_TYPE_1,         ONE_LINK },
	{ "duck",       0xA2,   AMD_SP8_SLT_2P,     ONE_LINK },
	{ "duck",       0xA3,   AMD_SP8_SLT_2P,     ONE_LINK },
	{ "duck",       0xA4,   AMD_SP8_SLT_2P,     ONE_LINK },
	{ "hornbill",   0xA5,   HCC_TYPE_2,         ONE_LINK },
	{ "hornbill",   0xA6,   HCC_TYPE_2,         ONE_LINK },
	{ "hornbill",   0xA7,   HCC_TYPE_2,         ONE_LINK },
	{ "hornbill",   0xA8,   HCC_TYPE_2,         ONE_LINK },
	{ "hornbill",   0xA9,   HCC_TYPE_2,         ONE_LINK },
	{ "hornbill",   0xAA,   HCC_TYPE_2,         ONE_LINK },
	{ "hornbill",   0xAB,   HCC_TYPE_2,         ONE_LINK },
	{ "hornbill",   0xAC,   HCC_TYPE_2,         ONE_LINK },
	{ "hornbill",   0xAD,   HCC_TYPE_2,         ONE_LINK },
	{ "robin",      0xAE,   AMD_SP8_SLT_1P,     ONE_LINK },
	{ "sandpiper",  0xAF,   AMD_SP8_SLT_1P,     ONE_LINK },
	{ "marrakesh",  0xB0,   AMD_TYPE_SLT_1P,    ONE_LINK },
	{ "falcon",     0xB1,   HCC_TYPE_1,         ONE_LINK },
	{ "falcon",     0xB2,   HCC_TYPE_1,         ONE_LINK },
	{ "falcon",     0xB3,   HCC_TYPE_1,         ONE_LINK },
	{ "falcon",     0xB4,   HCC_TYPE_1,         TWO_LINK },
	{ "seagull",    0xB5,   HCC_TYPE_2,         ONE_LINK },
	{ "seagull",    0xB6,   HCC_TYPE_2,         ONE_LINK },
	{ "seagull",    0xB7,   HCC_TYPE_2,         TWO_LINK },
	{ "peacock",    0xB8,   AMD_SP8_SLT_1P,     ONE_LINK },
	{ "pelican",    0xB9,   AMD_SP8_SLT_1P,     ONE_LINK },
	{ "penguin",    0xBA,   AMD_SP8_SLT_1P,     ONE_LINK },
	{ "arthur",     0xBB,   HCC_TYPE_1,         ONE_LINK },
	{ "lancelot",   0xBC,   AMD_SB1_SLT_1P,     ONE_LINK },
	{ "galhad",     0xBD,   AMD_SB1_SLT_1P,     ONE_LINK },
	{ "merlin",     0xBE,   AMD_SB1_SLT_1P,     ONE_LINK },
	{ "mordred",    0xBF,   AMD_SB1_SLT_1P,     ONE_LINK },
	{ "sb1charz",   0xC0,   AMD_SB1_SLT_1P,     ONE_LINK },
};

// mach aspeed cpu info
struct soc_id {
	const char *name;
	u64 rev_id;
};

static struct soc_id soc_map_table[] = {
	SOC_ID("AST2750-A0", 0x0600000306000003),
	SOC_ID("AST2700-A0", 0x0600010306000103),
	SOC_ID("AST2720-A0", 0x0600020306000203),
	SOC_ID("AST2750-A1", 0x0601000306010003),
	SOC_ID("AST2700-A1", 0x0601010306010103),
	SOC_ID("AST2720-A1", 0x0601020306010203),
	SOC_ID("AST2750-A2", 0x0601000306020003),
	SOC_ID("AST2700-A2", 0x0601010306020103),
	SOC_ID("AST2720-A2", 0x0601020306020203),
	SOC_ID("AST2750-A3", 0x0601000306030003),
	SOC_ID("AST2700-A3", 0x0601010306030103),
	SOC_ID("AST2720-A3", 0x0601020306030203),
};


void disable_fru_bus_muxes(void)
{
	struct udevice *bus;
	struct udevice *dev;
	int ret;
	uint8_t zero = 0x00;
	int addrs[] = { 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77 };
	int i;

	// Get the I2C bus
	ret = uclass_get_device_by_seq(UCLASS_I2C, HPM_EEPROM_I2C_BUS, &bus);
	if (ret) {
		printf("INFO: Cannot find I2C bus %d, err=%d\n", HPM_EEPROM_I2C_BUS, ret);
		return;
	}

	// Write 0x00 to register 0x00 of each target address
	for (i = 0; i < ARRAY_SIZE(addrs); i++) {
		ret = i2c_get_chip(bus, addrs[i], 1, &dev);
		if (ret) {
			printf("INFO: Failed to get device at 0x%02x\n", addrs[i]);
			continue;
		}
		ret = dm_i2c_write(dev, 0x00, &zero, 1);
		if (ret) {
			printf("INFO: Write to 0x%02x failed (err=%d)\n", addrs[i], ret);
		}
	}
}

void env_soc_id(void)
{
	int i;
	u64 rev_id;

	rev_id = readl(ASPEED_CPU_REVISION_ID);
	rev_id = ((u64)readl(ASPEED_IO_REVISION_ID) << 32) | rev_id;

	for (i = 0; i < ARRAY_SIZE(soc_map_table); i++) {
		if (rev_id == soc_map_table[i].rev_id)
			break;
	}
	if (i == ARRAY_SIZE(soc_map_table))
		printf("Unknown-SOC: %llx\n", rev_id);
	else {
		env_set(ENV_SOC_ID, soc_map_table[i].name);
		printf("Saving SOC config: %s\n", soc_map_table[i].name);
		env_save();
	}
}

int set_mac_addresses(const u8 *eeprom_buf)
{
	uchar enetaddr[MAC_ADDR_LEN] = {0};
	if(NULL == eeprom_buf)
		return -1;

	if (env_get(ENV_ETH_ADDR) && env_get(ENV_ETH1_ADDR)) {
		printf("ethaddr already set !!\n");
		return 0;
	}

	memcpy(enetaddr, eeprom_buf + MAC0_ADDR_OFFSET, sizeof enetaddr);
	if (!is_valid_ethaddr(enetaddr))
		printf("Error: not valid mac0 address\n");
	else
		eth_env_set_enetaddr(ENV_ETH_ADDR, enetaddr);

	memcpy(enetaddr, eeprom_buf + MAC1_ADDR_OFFSET, sizeof enetaddr);
	if (!is_valid_ethaddr(enetaddr))
		printf("Error: not valid mac1 address\n");
	else
		eth_env_set_enetaddr(ENV_ETH1_ADDR, enetaddr);

	memcpy(enetaddr, eeprom_buf + MAC2_ADDR_OFFSET, sizeof enetaddr);
	if (!is_valid_ethaddr(enetaddr))
		printf("Error: not valid mac2 address\n");
	else
		eth_env_set_enetaddr(ENV_ETH2_ADDR, enetaddr);

	return 0;
}

int get_platform_name( const u8 board_id, char* platname, char* dtsname, size_t buf_len)
{
	int ret = -1;
	int i = 0;
	int brd_count;

	if ((dtsname == NULL || platname == NULL))
		return ret;

	/* Get name string from BoardInfo structure based on id */
	brd_count = sizeof(boards) / sizeof(boards[0]);
	for (i = 0; i < brd_count; i++) {
		if (boards[i].id == board_id) {
			/* fill board name */
			strlcpy(platname, boards[i].name, buf_len);
			/* use 1P/2P default devicetrees for SLT boards */
			if (boards[i].type == AMD_TYPE_SLT_1P) {
				strlcpy(dtsname, "congo", buf_len);
			} else if (boards[i].type == AMD_TYPE_SLT_2P) {
				strlcpy(dtsname, "morocco", buf_len);
			} else if (boards[i].type == AMD_SP8_SLT_1P) {
				strlcpy(dtsname, "eagle", buf_len);
			} else if (boards[i].type == AMD_SP8_SLT_2P) {
				strlcpy(dtsname, "hornbill", buf_len);
			} else if (boards[i].type == AMD_SB1_SLT_1P) {
				strlcpy(dtsname, "arthur", buf_len);
			} else {
				strlcpy(dtsname, boards[i].name, buf_len);
			}
			ret = 0;
			break;
		}
	}
	if (ret == -1)
	{	/* Default device tree */
		strlcpy(platname, "sp7", sizeof(platname));
		strlcpy(dtsname, "congo", buf_len);
	}
	return ret;
}
/* extract chassis serial number as per FRU spec v1.0 */
int get_cia_ser_num(const uint8_t* fru_buf, char* chassis_ser_num)
{
	size_t cia_start_offset, cpn_start_offset, cpn_end_offset, csn_start_offset;
	int cpn_len = 0;
	int csn_len = 0;

	if (chassis_ser_num == NULL)
		return -1;

	cia_start_offset = fru_buf[CIA_START_OFFSET] * FRU_HDR_OFF_MULTIPLIER;
	cpn_start_offset = cia_start_offset + CIA_HDR_SIZE;
	cpn_len = (fru_buf[cpn_start_offset] & FRU_FIELD_TYPE_LEN_MASK);
	cpn_end_offset = cpn_start_offset + cpn_len;
	csn_start_offset = cpn_end_offset + FRU_FIELD_TYPE_LEN_SIZE;
	csn_len = (fru_buf[csn_start_offset] & FRU_FIELD_TYPE_LEN_MASK);

	if(csn_len > 0)
		memcpy(chassis_ser_num, &fru_buf[csn_start_offset+FRU_FIELD_TYPE_LEN_SIZE], csn_len);
	else
		printf("invalid chassis serial number\n");
	return csn_len;
}


int get_csn_last4(const char* str, char* buf, size_t buf_len)
{
	int pos_min = 0;
	int pos_max = 0;
	int cur_pos = 0;
	int c=0;
	pos_max=strlen(str);

	/* Trim leading, trailing whitespaces */
	while(isspace(*(str+pos_min))) pos_min++;
	while(isspace(*(str+pos_max-1))) pos_max--;
	/* extract from end */
	cur_pos = pos_max - MIN_CSN_UNIQ_STR_LEN;
	while (cur_pos <= pos_max)
	{
		buf[c] = *(str+cur_pos);
		c++;
		cur_pos++;
	}
	buf[MIN_CSN_UNIQ_STR_LEN]='\0';
	if (strlen(buf) >= MIN_CSN_UNIQ_STR_LEN)
		return cur_pos;
	else
		return -1;
}

int get_csn_uniq_str(const char* str, char delim, char* buf, size_t buf_len)
{
	int pos = 0;
	int c;
	int s_len = strlen(str);
	for (c = 0; c < s_len; c++)
	{
		if (*(str+c) == delim)
			pos = c;
	}
	for (c = 0; (*(str+pos) !='\0') && (c < (buf_len - 1)); c++)
	{
		buf[c]=*(str+pos);
		pos++;
	}
	if (strlen(buf) > MIN_CSN_UNIQ_STR_LEN)
		return pos;
	else
		return -1;
}

int set_board_info(const u8* scm_eeprom_buf, const u8* hpm_eeprom_buf)
{
	char mac_str[MAC_ADDR_LEN] = {0};
	uchar enetaddr[MAC_ADDR_LEN] = {0};
	char new_hostname[STR_BUF_LEN] = {0};
	char *old_bootargs = NULL;
	char new_bootargs [STR_BUF_LEN] = {0};
	char plat_name [STR_BUF_LEN] = {0};
	char dts_name [STR_BUF_LEN] = {0};
	char board_conf_name[STR_BUF_LEN] = {0};
	char board_id_buf[STR_BUF_LEN] = {0};
	char board_id_str[STR_BUF_LEN] = {0};
	char board_rev_str[STR_BUF_LEN] = {0};
	char chassis_ser_num[STR_BUF_LEN] = {0};
	char hpm_csn_uniq_str[STR_BUF_LEN] = {0};
	u8 board_rev = 0;
	u8 hpm_mrc = 0;

	/* calculate HPM Multi Rec Area offsets */
	hpm_mrc = (*(hpm_eeprom_buf + FRU_MRC_HDR_OFFSET)) * MRC_HDR_AREA_START;
	board_id = *(hpm_eeprom_buf + hpm_mrc + HPM_BRD_ID_OFFSET);
	board_rev = *(hpm_eeprom_buf + hpm_mrc + HPM_BRD_REV_OFFSET);

	/* HPM board name */
	if (get_platform_name(board_id, plat_name, dts_name, sizeof(dts_name)) == 0) {
		/* HPM board FDT config */
		if(!env_get(ENV_BOARD_FIT_CONF)) {
			snprintf(board_conf_name, sizeof(board_conf_name),"#conf-aspeed-bmc-amd-%s.dtb", dts_name);
			env_set(ENV_BOARD_FIT_CONF, board_conf_name);
			printf("Saving Board FDT config: %s\n", board_conf_name);
			env_save();
		}
		else
			printf("HPM EEPROM not programmed\nLoading first DTB config\n");

		/* Export dt name, defaults to congo */
		if(!env_get(ENV_DT_NAME)) {
			env_set(ENV_DT_NAME, dts_name);
			printf("Saving DT name: %s\n", dts_name);
			env_save();
		}
	}

	/* HPM board env variables for linux apps */
	if(!env_get(ENV_BOARD_ID)) {
		if ((board_id != 0xff)) {
			bin2hex(board_id_buf, &board_id, sizeof board_id);
			str_to_upper(board_id_buf, board_id_str, strlen(board_id_buf)+1);
			env_set(ENV_BOARD_ID, board_id_str);
			printf("Saving board_id: %s\n", board_id_str);
			env_save();
		}
		else
			printf("Invalid board_id in HPM EEPROM\n");
	}
	if(!env_get(ENV_BOARD_REV)) {
		if ((board_rev != 0xff)) {
			bin2hex(board_rev_str, &board_rev, sizeof board_rev);
			env_set(ENV_BOARD_REV, board_rev_str);
			printf("Saving board_rev: %s\n", board_rev_str);
			env_save();
		}
		else
			printf("Invalid board_rev in HPM EEPROM\n");
	}

	/* check if scm has valid data */
	memcpy(enetaddr, scm_eeprom_buf + MAC0_ADDR_OFFSET, sizeof enetaddr);
	if (!is_valid_ethaddr(enetaddr)) {
		printf("Error: not valid mac0 address\n");
		printf("Please program SCM EEPROM\n");
		return -1;
	}
	else {
		/* try reading chassis info area serial number from HPM eeprom */
		/* Get chassis Info area, read chassis serial num */
		/* strip out last uniq string and convert to hostname */
		if (get_cia_ser_num(hpm_eeprom_buf, chassis_ser_num) > 0) {
			printf("Chassis Serial Number: %s\n", chassis_ser_num);
			if(get_csn_last4(chassis_ser_num, hpm_csn_uniq_str, sizeof hpm_csn_uniq_str) > 0)
				printf("Unique ID: %s\n",hpm_csn_uniq_str);
			if (strlen(hpm_csn_uniq_str) > 0)
				memcpy(mac_str, hpm_csn_uniq_str, strlen(hpm_csn_uniq_str));
		}
		else /* read mac address from SCM eeprom */
			bin2hex(mac_str, scm_eeprom_buf + LAST_2B_MAC0_ADDR, LAST_2B_MAC0_LEN);
		/* update bootargs with new hostname */
		old_bootargs = env_get(ENV_BOOTARGS);
		if  (old_bootargs && !(strstr(old_bootargs, "systemd.hostname")) ) {
			/* Hostname to pass to systemd-networking */
			snprintf(new_hostname, sizeof(new_hostname), "systemd.hostname=%s-%s", plat_name, mac_str);
			snprintf(new_bootargs, sizeof(new_bootargs),"%s %s",old_bootargs,new_hostname);
			env_set(ENV_BOOTARGS, new_bootargs);
			printf("Setting new hostname %s\n", new_hostname);
			env_save();
		}
	}
	return 0;
}

void configure_edaf_spi(const u8 *eeprom_buf)
{
	char *edaf_flag = NULL;

	// Reconfigure pin from SCM_GPO to GPIO mode.
	// pin mode changes to SCM_GPO on power-on/reset of BMC.
	run_command("mw 14c02404 55000055", 0);

	if ( *(eeprom_buf + SCM_BOM_VARIANT_OFF) & ESPI_VARIANT_BIT) {
		printf("eSPI variant SCM board detected\n");

		printf("configuring for eDAF ...\n");
		edaf_flag = env_get("edaf");
		if ( edaf_flag == NULL ) {
			/* used by remote BIOS SPI updates */
			run_command("setenv edaf true", 0);
			run_command("saveenv", 0);
		}
		/* Set eSPI strap to high for IO/Alert pin mode */
		run_command("gpio set 10", 0); //eSPI0
		run_command("gpio set 11", 0); //eSPI1
		/* Set Erase block size - underlying SPI uses 4K blocks */
		run_command("mw 14c05028 401", 0); // eSPI0
		run_command("mw 14c06028 401", 0); // eSPI1
		printf("configuring for eDAF ...Done\n");
	}
	else {
		printf("QSPI variant SCM board detected\n");

		// set espi GPIO strap to low for dedicated alert pin mode
		run_command("gpio clear 10", 0); //eSPI0
		run_command("gpio clear 11", 0); //eSPI1
	}
}

void power_on_hpm(int retry)
{
	int i;
	// TODO: Enable by default on RevB SCM boards
	const char *env_val = env_get("EN_HPM_PWR_SEQ");
	if (!env_val) {
		printf("EN_HPM_PWR_SEQ is not set. Skipping...\n");
		return;
	}
	// Init GPIO
	if (gpio_request(HPM_RST_GPIO, "HPM_RST_GPIO")) {
		printf("[ERR] Failed to request RST_L GPIO (%d)\n", HPM_RST_GPIO);
		return;
	}
	if (gpio_request(HPM_EN_GPIO, "HPM_EN_GPIO")) {
		printf("[ERR] Failed to request EN GPIO (%d)\n", HPM_EN_GPIO);
		gpio_free(HPM_RST_GPIO);
		return;
	}
	if (gpio_request(HPM_RDY_GPIO, "HPM_RDY_GPIO")) {
		printf("[ERR] Failed to request RDY GPIO (%d)\n", HPM_RDY_GPIO);
		gpio_free(HPM_RST_GPIO);
		gpio_free(HPM_EN_GPIO);
		return;
	}

	gpio_direction_output(HPM_RST_GPIO, 0);
	gpio_direction_output(HPM_EN_GPIO, 0);
	gpio_direction_input(HPM_RDY_GPIO);

	// HPM STBY EN
	gpio_set_value(HPM_EN_GPIO, 1);

	for (i = 0; i < retry; i++) {
		if (gpio_get_value(HPM_RDY_GPIO) == 0) {
			printf("HPM FPGA not ready, attempt %d/%d\n", i+1, retry);
			udelay(HPM_RDY_RTRY_INTRVL); // 100ms
		} else {
			printf("HPM FPGA ready on attempt %d\n", i+1);
			break;
		}
	}
	if (retry == i)
		printf("[ERR] FPGA did not become ready after %d attempts\n", retry);


	// RST_L
	gpio_set_value(HPM_RST_GPIO, 1);
	printf("HPM devices out of reset\n");

}

void train_ltpi(int retry, int mode)
{
	int i=0;
	char buf[8];
	char mode_flag[8] = "";
	char command[256];

	/* Set mode flag based on link type */
	if (mode == ONE_LINK) {
		snprintf(mode_flag, sizeof(mode_flag), "-m 0 ");
	} else if (mode == TWO_LINK) {
		snprintf(mode_flag, sizeof(mode_flag), "-m 1 ");
	}
	/* start LTPI with operational and advertise timeouts */
	snprintf(command, sizeof(command), "ltpi -T " OP_TIMEOUT_US " -t " ADVRT_TIMEOUT_US_1_1 " -p 1 %s", mode_flag);
	if(run_command(command, 0) != 0)
	{
		for (i=0; i<retry; i++)
		{
			if(run_command(command,0) == 0)
			{
				printf("LTPI link %d configured, proceeding to boot...\n", mode);
				break;
			}
			else
			{
				printf("Retrying Link training...(%d)\n",i);
				snprintf(buf, sizeof(buf), "%d", i);
				env_set("ltpi_rt_cnt", buf);
				env_save();
			}
		}
		if (i >= retry)
			{
				printf("LTPI failed to train, collect register dump!!!\n");
			}

	}
}
void update_por_env(void)
{
	u32 por_rst = readl(ASPEED_SYS_SCRATCH_7FC);

	/* set reset reason env */
	printf("Scratch register value: 0x%08x\n", por_rst);
	if (por_rst & SYS_SRST)
		env_set("por_rst", "true");
	else
		env_set("por_rst", "false");
	env_save();
}

int read_eeprom_buffers(u8 *scm_eeprom_buf, u8 *hpm_eeprom_buf)
{
	struct udevice *idev, *ibus;
	int ret;

	ret = uclass_get_device_by_seq(UCLASS_I2C, SCM_EEPROM_I2C_BUS, &ibus);
	if (ret) {
		printf("\nSCM i2c bus acquisition failed!\n");
		return ret;
	}

	ret = dm_i2c_probe(ibus, EEPROM_DEV_ADDR, 0, &idev);
	if (ret) {
		printf("\n SCM slave probe failed\n");
		return ret;
	}

	ret = i2c_set_chip_offset_len(idev, SCM_EEPROM_OFF_LEN);
	if (ret) {
		printf("\n SCM slave len failed\n");
		return ret;
	}

	if (dm_i2c_read(idev, 0, scm_eeprom_buf, EEPROM_BUF_LEN)) {
		printf("\nSCM EEPROM read failed!\n");
		return -1;
	}

	/* Disable muxes on FRU bus to access direct slaves only */
	disable_fru_bus_muxes();

	ret = uclass_get_device_by_seq(UCLASS_I2C, HPM_EEPROM_I2C_BUS, &ibus);
	if (ret) {
		printf("\n HPM i2c bus acquisition failed\n");
		return ret;
	}

	ret = dm_i2c_probe(ibus, EEPROM_DEV_ADDR, 0, &idev);
	if (ret) {
		printf("\n HPM slave probe failed\n");
		return ret;
	}

	ret = i2c_set_chip_offset_len(idev, HPM_EEPROM_OFF_LEN);
	if (ret) {
		printf("\n HPM slave len failed\n");
		return ret;
	}

	if (dm_i2c_read(idev, 0, hpm_eeprom_buf, EEPROM_BUF_LEN)) {
		printf("\nHPM EEPROM read failed!\n");
		return -1;
	}

	return 0;
}

int misc_init_r(void)
{
	int ret;
	int i, ltpi_type = ONE_LINK;

	/* Identify SoC of DC-SCM card */
	env_soc_id();

	/* Read the FRU EEPROM and store in buffer */
	u8 scm_eeprom_buf [EEPROM_BUF_LEN] = {0};
	u8 hpm_eeprom_buf [EEPROM_BUF_LEN] = {0};

	uchar enetaddr[MAC_ADDR_LEN] = {0};

	/* Read the SCM and HPM EEPROMs */
	ret = read_eeprom_buffers(scm_eeprom_buf, hpm_eeprom_buf);
	if (ret) {
		printf("EEPROM read failed!\n");
		goto err;
	}

	/* check if SCM EEPROM is programmed */
	if (scm_eeprom_buf[0] == 0xFF) {
		printf("EEPROM not programmed\n");
		goto err;
	}

	/* set MAC addresses from SCM EEPROM */
	if (set_mac_addresses(scm_eeprom_buf) == 0)
	{
		if (eth_env_get_enetaddr(ENV_ETH_ADDR, enetaddr)) {
			printf("SCM MAC0 : %pM\n", enetaddr);
		}
		if (eth_env_get_enetaddr(ENV_ETH1_ADDR, enetaddr)) {
			printf("SCM MAC1 : %pM\n", enetaddr);
		}
		if (eth_env_get_enetaddr(ENV_ETH2_ADDR, enetaddr)) {
			printf("SCM MAC2 : %pM\n", enetaddr);
		}
	}

	/* check if HPM EEPROM is programmed */
	if (hpm_eeprom_buf[0] == 0xFF) {
		printf("HPM EEPROM not programmed\n");
		goto err;
	}

	/* set Hostname, board id,rev and fdt config from HPM EEPROM */
	if (set_board_info(scm_eeprom_buf, hpm_eeprom_buf) == 0)
	{
		printf("Loading %s\n", env_get(ENV_BOARD_FIT_CONF));
	}
	/* set power-on reset variable */
	update_por_env();

	/* HPM Power-on sequence */
	power_on_hpm(HPM_STBY_EN_RETRY);

	/* enable ltpi strap and train link */
	for (i = 0; i < ARRAY_SIZE(boards); i++) {
		if (board_id == boards[i].id)  {
			ltpi_type = boards[i].ltpi_type;
			break;
		}
	}

	train_ltpi(LTPI_TRAIN_RETRY, ltpi_type);

	/* configure spi mux for edaf
           NOTE: do after running 'ltpi' as it reconfigures SCM GPIOs
        */
	configure_edaf_spi(scm_eeprom_buf);

	return 0;
err:
	printf("EEPROM i2c error in %s\n", __func__);

	return 0; // non-zero return code will halt u-boot
}
