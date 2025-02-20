/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 ASPEED Technology Inc.
 */
#include <linux/bitops.h>
//#include <debug.h>
#define BOOTSTAGE_DCSCM				"M "
#define BOOTSTAGE_LTPI_MASTER			"Lm"
#define BOOTSTAGE_LTPI_SLAVE			"Ls"
#define BOOTSTAGE_LTPI_INIT			"Li"
#define BOOTSTAGE_LTPI_SP_CAP			"Lc"
#define BOOTSTAGE_LTPI_WAIT_OP			"Lo"
#define BOOTSTAGE_STATUS_SUCCESS		0x00

/* OCP_DC-SCM_2.0_LTPI_ver_1.0, Table 21 LTPI speed capability encoding */
#define LTPI_SP_CAP_25M				BIT(0)
#define LTPI_SP_CAP_50M				BIT(1)
#define LTPI_SP_CAP_75M				BIT(2)
#define LTPI_SP_CAP_100M			BIT(3)
#define LTPI_SP_CAP_150M			BIT(4)
#define LTPI_SP_CAP_200M			BIT(5)
#define LTPI_SP_CAP_250M			BIT(6)
#define LTPI_SP_CAP_300M			BIT(7)
#define LTPI_SP_CAP_400M			BIT(8)
#define LTPI_SP_CAP_600M			BIT(9)
#define LTPI_SP_CAP_800M			BIT(10)
#define LTPI_SP_CAP_1G				BIT(11)
/* --gap-- */
#define LTPI_SP_CAP_DDR				BIT(15)
#define LTPI_SP_CAP_ASPEED_SUPPORTED                                               \
	(LTPI_SP_CAP_25M | LTPI_SP_CAP_50M | LTPI_SP_CAP_100M | LTPI_SP_CAP_200M | \
	 LTPI_SP_CAP_250M | LTPI_SP_CAP_400M | LTPI_SP_CAP_800M | LTPI_SP_CAP_1G)

#define LTPI_SP_CAP_MASK_25M			GENMASK(0, 0)
#define LTPI_SP_CAP_MASK_50M			GENMASK(1, 0)
#define LTPI_SP_CAP_MASK_75M			GENMASK(2, 0)
#define LTPI_SP_CAP_MASK_100M			GENMASK(3, 0)
#define LTPI_SP_CAP_MASK_150M			GENMASK(4, 0)
#define LTPI_SP_CAP_MASK_200M			GENMASK(5, 0)
#define LTPI_SP_CAP_MASK_250M			GENMASK(6, 0)
#define LTPI_SP_CAP_MASK_300M			GENMASK(7, 0)
#define LTPI_SP_CAP_MASK_400M			GENMASK(8, 0)
#define LTPI_SP_CAP_MASK_600M			GENMASK(9, 0)
#define LTPI_SP_CAP_MASK_800M			GENMASK(10, 0)
#define LTPI_SP_CAP_MASK_1G			GENMASK(11, 0)
