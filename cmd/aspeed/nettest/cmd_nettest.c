// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */
#include <common.h>
#include <command.h>

extern int netdiag_func(int argc, char *const argv[]);

int do_netdiag(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	return netdiag_func(argc, argv);
}

U_BOOT_CMD(netdiag, 32, 0, do_netdiag,
	   "Dedicated LAN & Share LAN (NC-SI) test program", "run netdiag -h");
