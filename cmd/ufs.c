// SPDX-License-Identifier: GPL-2.0+
/**
 * ufs.c - UFS specific U-Boot commands
 *
 * Copyright (C) 2019 Texas Instruments Incorporated - http://www.ti.com
 *
 */
#include <common.h>
#include <command.h>
#include <ufs.h>
#include <dm.h>
#include "../drivers/ufs/ufs.h"

int ufshcd_map_desc_id_to_length(struct ufs_hba *hba, enum desc_idn desc_id,
				 int *desc_len);

int ufshcd_query_descriptor_retry(struct ufs_hba *hba, enum query_opcode opcode,
				  enum desc_idn idn, u8 index, u8 selector,
				  u8 *desc_buf, int *buf_len);

int ufshcd_read_desc_param(struct ufs_hba *hba, enum desc_idn desc_id,
			   int desc_index, u8 param_offset, u8 *param_read_buf,
			   u8 param_size);

static int do_ufs_create(struct cmd_tbl *cmdtp, int flag,
			 int argc, char *const argv[])
{
	struct udevice *dev;
	struct ufs_hba *hba;
	int ret;
	int buff_len;
	u8 desc_buf[0x16 + 8 * 0x1A], i;
	u32 tmp;
	int en, lun, boot;
	u32 size;

	en = ((!strcmp(argv[0], "create")) ? 1 : 0);
	boot = ((!strcmp(argv[1], "boot")) ? 1 : 0);
	lun = dectoul(argv[2], NULL);
	size = hextoul(argv[3], NULL);

	ret = uclass_get_device(UCLASS_SCSI, 0, &dev);
	if (ret == -ENODEV) {
		printf("No udevice get!\n");
		return CMD_RET_FAILURE;
	}

	hba = dev_get_uclass_priv(dev->parent);

	ret = ufshcd_map_desc_id_to_length(hba, QUERY_DESC_IDN_CONFIGURATION, &buff_len);
	ret = ufshcd_query_descriptor_retry(hba, UPIU_QUERY_OPCODE_READ_DESC,
					    QUERY_DESC_IDN_CONFIGURATION, 0, 0, desc_buf,
					    &buff_len);
	if (ret)
		printf("read desc fail!!!\n");

	if (!strcmp(argv[0], "list"))
		goto dump;

	if (desc_buf[0x16 + lun * 0x1A] == en)
		goto dump;

	desc_buf[0x3] = boot ? 1 : 0; // bBootEnable
	desc_buf[0x16 + lun * 0x1A] = en; // bLUEnable
	if (boot)
		desc_buf[0x16 + lun * 0x1A + 1] = ((lun == 1) ? 1 : 2); // bBootLunID: 01h: Boot LU A, 02h: Boot LU B.
	else
		desc_buf[0x16 + lun * 0x1A + 1] = 0;
	desc_buf[0x16 + lun * 0x1A + 4] = size & 0xff;
	desc_buf[0x16 + lun * 0x1A + 5] = (size & 0xff) >> 8;
	desc_buf[0x16 + lun * 0x1A + 6] = (size & 0xff0000) >> 16;
	desc_buf[0x16 + lun * 0x1A + 7] = (size & 0xff000000) >> 24;

	ret = ufshcd_query_descriptor_retry(hba, UPIU_QUERY_OPCODE_WRITE_DESC,
					    QUERY_DESC_IDN_CONFIGURATION, 0, 0, desc_buf,
					    &buff_len);
	if (ret)
		printf("write desc fail!!!\n");

dump:
	for (i = 0; i < 8; i++) {
		tmp = 0;
		ret =  ufshcd_read_desc_param(hba, QUERY_DESC_IDN_UNIT, i,
					      3, (u8 *)&tmp, sizeof(tmp));

		printf("LUN %d, lu_enable=%x\n", i, tmp);
	}

	return 0;
}

static int do_ufs_init(struct cmd_tbl *cmdtp, int flag,
		       int argc, char *const argv[])
{
	int dev, ret;

	if (argc == 2) {
		dev = dectoul(argv[2], NULL);
		ret = ufs_probe_dev(dev);
		if (ret)
			return CMD_RET_FAILURE;
	} else {
		ufs_probe();
	}

	return CMD_RET_SUCCESS;
}

static struct cmd_tbl cmd_ufs[] = {
	U_BOOT_CMD_MKENT(init, 2, 0, do_ufs_init, "", ""),
	U_BOOT_CMD_MKENT(create, 4, 0, do_ufs_create, "", ""),
	U_BOOT_CMD_MKENT(delete, 3, 0, do_ufs_create, "", ""),
	U_BOOT_CMD_MKENT(list, 1, 0, do_ufs_create, "", ""),
};

static int do_ufs(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	struct cmd_tbl *cp;

	cp = find_cmd_tbl(argv[1], cmd_ufs, ARRAY_SIZE(cmd_ufs));

	argc--;
	argv++;

	if (!cp || argc > cp->maxargs)
		return CMD_RET_USAGE;
	if (flag == CMD_FLAG_REPEAT && !cmd_is_repeatable(cp))
		return CMD_RET_SUCCESS;

	return cp->cmd(cmdtp, flag, argc, argv);
}

U_BOOT_CMD(ufs, 5, 1, do_ufs,
	   "UFS  sub system",
	   "init [dev] - init UFS subsystem\n"
	   "ufs create [boot|lun] [lun] [size] - create UFS LUN\n"
	   "ufs delete [boot|lun] [lun] - Delete UFS LUN\n"
	   "ufs list - List UFS LUN\n"
	   "ex. ufs create lun 0 10000 : Create lun 0 with size 1GB.\n"
	   "	ufs create boot 1 1000000 : Create boot lun 1 with size 4MB.\n"
	   "				    LUN 1 is default set to Boot LU A.\n"
	   "	ufs create boot 2 100000 : Create boot lun 2 with size 16GB.\n"
	   "				   Others are set to Boot LU B.\n"
	   "	ufs create lun 3 10000000 : Create lun 3 with size 64MB.\n"
	   "	ufs delete boot 2 : Delete boot lun 2.\n"
	   "	ufs delete lun 0 : Delete lun 0.\n"
	   "	ufs list.\n"
);
