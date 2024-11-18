// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */
#include <common.h>
#include <env.h>
#include <i2c.h>
#include <dm/uclass.h>
#include <net.h>
#include <hexdump.h>
#include <errno.h>
#include <asm/io.h>

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

#define FRU_MRC_HDR_OFFSET    (5)
#define MRC_HDR_AREA_START    (8)
#define HPM_BRD_ID_OFFSET    (18)
#define HPM_BRD_REV_OFFSET   (19)
#define STR_BUF_LEN         (128)

#define ENV_BOOTARGS           "bootargs"
#define ENV_BOARD_FIT_CONF   "board_conf"
#define ENV_BOARD_ID           "board_id"
#define ENV_BOARD_REV         "board_rev"
#define ENV_ETH_ADDR            "ethaddr"
#define ENV_ETH1_ADDR          "eth1addr"
#define ENV_ETH2_ADDR          "eth2addr"

/* Sys Scratch reg that holds sys_rst info, refer cpu-info.c */
#define ASPEED_SYS_SCRATCH_7FC 0x12C027FC
#define SYS_SRST		BIT(0)

/* SP7 HPM boards */
#define MARLEY_1        0x79
#define MARLEY_2        0x7A
#define MARLEY_3        0x7B
#define MOJANDA_1       0x7C
#define MOJANDA_2       0x7E
#define MOJANDA_3       0x7F
#define CONGO_1         0x80
#define CONGO_2         0x81
#define CONGO_3         0x86
#define MOROCCO_1       0x82
#define MOROCCO_2       0x83
#define MOROCCO_3       0x87
#define KENYA           0x84
#define NIGERIA         0x85
#define GHANA           0x8E
#define SENEGAL_SLT     0x88
#define SAHARA          0x89
#define MALAWI          0x8A
#define ZAMBIA          0x8B
#define ZIMBABWE        0x8C
#define ZANZIBAR        0x8D

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

int get_platform_name( const u8 board_id, char* platname)
{
	int ret = 0;
	switch (board_id) {
		case CONGO_1:
		case CONGO_2:
		case CONGO_3:
			strcpy(platname, "congo");
			break;
		case MOROCCO_1:
		case MOROCCO_2:
		case MOROCCO_3:
			strcpy(platname, "morocco");
			break;
		case MARLEY_1:
		case MARLEY_2:
		case MARLEY_3:
			strcpy(platname, "marley");
			break;
		case MOJANDA_1:
		case MOJANDA_2:
		case MOJANDA_3:
			strcpy(platname, "mojanda");
			break;
		case KENYA:
			strcpy(platname, "kenya");
			break;
		case NIGERIA:
			strcpy(platname, "nigeria");
			break;
		case GHANA:
			strcpy(platname, "ghana");
			break;
		case SENEGAL_SLT:
			strcpy(platname, "senegal");
			break;
		case SAHARA:
			strcpy(platname, "sahara");
			break;
		case MALAWI:
			strcpy(platname, "malawi");
			break;
		case ZAMBIA:
			strcpy(platname, "zambia");
			break;
		case ZIMBABWE:
			strcpy(platname, "zimbabwe");
			break;
		case ZANZIBAR:
			strcpy(platname, "zanzibar");
			break;
		default:
			strcpy(platname, "sp7");
			ret = -1;
	}
	return ret;
}
int set_board_info(const u8* scm_eeprom_buf, const u8* hpm_eeprom_buf)
{
	char mac_str[MAC_ADDR_LEN] = {0};
	uchar enetaddr[MAC_ADDR_LEN] = {0};
	char new_hostname[STR_BUF_LEN] = {0};
	char *old_bootargs = NULL;
	char new_bootargs [STR_BUF_LEN] = {0};
	char plat_name [STR_BUF_LEN] = {0};
	char board_conf_name[STR_BUF_LEN] = {0};
	char board_id_str[STR_BUF_LEN] = {0};
	char board_rev_str[STR_BUF_LEN] = {0};
	u8 board_id = 0;
	u8 board_rev = 0;
	u8 hpm_mrc = 0;

	/* calculate HPM Multi Rec Area offsets */
	hpm_mrc = (*(hpm_eeprom_buf + FRU_MRC_HDR_OFFSET)) * MRC_HDR_AREA_START;
	board_id = *(hpm_eeprom_buf + hpm_mrc + HPM_BRD_ID_OFFSET);
	board_rev = *(hpm_eeprom_buf + hpm_mrc + HPM_BRD_REV_OFFSET);

	/* HPM board name */
	if (get_platform_name(board_id, &plat_name) == 0) {
		/* HPM board FDT config */
		if(!env_get(ENV_BOARD_FIT_CONF)) {
			snprintf(board_conf_name, sizeof(board_conf_name),"#conf-aspeed-bmc-amd-%s.dtb", plat_name);
			env_set(ENV_BOARD_FIT_CONF, board_conf_name);
			printf("Saving Board FDT config: %s\n", board_conf_name);
			env_save();
		}
		else
			printf("HPM EEPROM not programmed\nLoading first DTB config\n");
	}

	/* HPM board env variables for linux apps */
	if(!env_get(ENV_BOARD_ID)) {
		if ((board_id != 0xff)) {
			bin2hex(board_id_str, &board_id, sizeof board_id);
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
		bin2hex(mac_str, scm_eeprom_buf + LAST_2B_MAC0_ADDR, LAST_2B_MAC0_LEN);
		old_bootargs = env_get(ENV_BOOTARGS);
		if  ( !(strstr(old_bootargs, "systemd.hostname")) ) {
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

void aspeed_errata_app_note_6_pcie_ep()
{
       printf("applying Aspeed app note 6 pcie ep ...\n");
       run_command("mw 12c02a60 0", 0);
       run_command("mw 12c02ae0 0", 0);
       printf("applying Aspeed app note 6 pcie ep ... Done\n");
}

void update_por_env(void)
{
	const char *s;
	u32 por_rst = readl(ASPEED_SYS_SCRATCH_7FC);

	/* set reset reason env */
	printf("Scratch register value: 0x%08x\n", por_rst);
	if (por_rst & SYS_SRST)
		env_set("por_rst", "true");
	else
		env_set("por_rst", "false");
	env_save();
}

int misc_init_r(void)
{
	/* Read the FRU EEPROM and store in buffer */
	u8 scm_eeprom_buf [EEPROM_BUF_LEN] = {0};
	u8 hpm_eeprom_buf [EEPROM_BUF_LEN] = {0};

	struct udevice *idev, *ibus;
	int ret;
	uchar enetaddr[MAC_ADDR_LEN] = {0};

	ret = uclass_get_device_by_seq(UCLASS_I2C, SCM_EEPROM_I2C_BUS, &ibus);
	if (ret)
		goto err;
	ret = dm_i2c_probe(ibus, EEPROM_DEV_ADDR, 0, &idev);
	if (ret)
		goto err;

	ret = i2c_set_chip_offset_len(idev, SCM_EEPROM_OFF_LEN);
	if (ret)
		goto err;

	if (dm_i2c_read(idev, 0, &scm_eeprom_buf, sizeof scm_eeprom_buf)) {
		printf("\nSCM EEPROM read failed!\n");
		goto err;
	}

	ret = uclass_get_device_by_seq(UCLASS_I2C, HPM_EEPROM_I2C_BUS, &ibus);
	if (ret)
		goto err;

	ret = dm_i2c_probe(ibus, EEPROM_DEV_ADDR, 0, &idev);
	if (ret)
		goto err;

	ret = i2c_set_chip_offset_len(idev, HPM_EEPROM_OFF_LEN);
	if (ret)
		goto err;

	if (dm_i2c_read(idev, 0, &hpm_eeprom_buf, sizeof hpm_eeprom_buf)) {
		printf("\nHPM EEPROM read failed!\n");
		goto err;
	}

	/* set MAC addresses from SCM EEPROM */
	if (set_mac_addresses(&scm_eeprom_buf) == 0)
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

	/* set Hostname, board id,rev and fdt config from HPM EEPROM */
	if (set_board_info(&scm_eeprom_buf, &hpm_eeprom_buf) == 0)
	{
		printf("Loading %s\n", env_get(ENV_BOARD_FIT_CONF));
	}
	/* set power-on reset variable */
	update_por_env();

        /* apply aspeed errata */
        aspeed_errata_app_note_6_pcie_ep();

	return 0;
err:
	printf("EEPROM i2c error in %s\n", __func__);
	return 0; // non-zero return code will halt u-boot
}
