/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2024 ASPEED Technology Inc.
 */
#ifndef __LTPI_V2_H__
#define __LTPI_V2_H__

#define BOOTSTAGE_LTPI_INIT			"L "

#define LTPI_SP_CAP_ASPEED_SUPPORTED                              \
	(LTPI_SP_CAP_25M | LTPI_SP_CAP_50M | LTPI_SP_CAP_75M |    \
	 LTPI_SP_CAP_100M | LTPI_SP_CAP_150M | LTPI_SP_CAP_200M | \
	 LTPI_SP_CAP_250M | LTPI_SP_CAP_300M | LTPI_SP_CAP_400M | \
	 LTPI_SP_CAP_600M | LTPI_SP_CAP_500M | LTPI_SP_CAP_DDR)

/* bootstage_t->errno */
#define LTPI_STATUS_EXIT			BIT(7)	/* 1: exit due to errors */
#define LTPI_STATUS_RESTART			BIT(6)	/* 1: restart LTPI initialization */

#define LTPI_STATUS_HAS_CRC_ERR			BIT(4)
#define LTPI_STATUS_MODE			GENMASK(3, 2)	/* PHY mode */
#define LTPI_STATUS_IDX				BIT(1)	/* 1: LTPI1 */
#define LTPI_STATUS_HPM				BIT(0)	/* 1: LTPI controller is on HPM */

/* bootstage_t->syndrome */
#define LTPI_SYND_OK				0
#define LTPI_SYND_OK_ALREADY_INIT		1
#define LTPI_SYND_NO_COMMOM_SPEED		2
#define LTPI_SYND_WAIT_OP_TO			3
#define LTPI_SYND_EXTRST_LINK_TRAINING		4	/* EXTRST deasserted during link training */
#define LTPI_SYND_EXTRST_LINK_CONFIG		5	/* EXTRST deasserted during link configuration */
#define LTPI_SYND_SOC_RECOVERY			6	/* Exit due to SOC recovery */

/* LTPI wait state return code */
#define LTPI_ERR_NONE				0x00
#define LTPI_ERR_TIMEOUT			0x10
#define LTPI_ERR_DISCON				0x20

#endif /* #ifndef __LTPI_V2_H__ */
