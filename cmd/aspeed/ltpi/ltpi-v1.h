/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 ASPEED Technology Inc.
 */
#ifndef __LTPI_V1_H__
#define __LTPI_V1_H__

#define BOOTSTAGE_DCSCM				"M "
#define BOOTSTAGE_LTPI_MASTER			"Lm"
#define BOOTSTAGE_LTPI_SLAVE			"Ls"
#define BOOTSTAGE_LTPI_INIT			"Li"
#define BOOTSTAGE_LTPI_SP_CAP			"Lc"
#define BOOTSTAGE_LTPI_WAIT_OP			"Lo"
#define BOOTSTAGE_STATUS_SUCCESS		0x00

/* general codes for all BOOTSTAGE_LTPI message */
#define BOOTSTAGE_LTPI_MODE_NONE		0
#define BOOTSTAGE_LTPI_MODE_SDR			1
#define BOOTSTAGE_LTPI_MODE_CDR			2

/* end status of BOOTSTAGE_LTPI_INIT */
#define BOOTSTAGE_LTPI_INIT_SKIP		0x00
#define BOOTSTAGE_LTPI_INIT_REQUIRE		0x10

/* end status of BOOTSTAGE_LTPI_SP_CAP */
#define BOOTSTAGE_LTPI_SP_CAP_E_NS		0x40	/* Not Same */
#define BOOTSTAGE_LTPI_SP_CAP_E_NC		0x80	/* No Common speed */

#define LTPI_SP_CAP_ASPEED_SUPPORTED                                               \
	(LTPI_SP_CAP_25M | LTPI_SP_CAP_50M | LTPI_SP_CAP_100M | LTPI_SP_CAP_200M | \
	 LTPI_SP_CAP_250M | LTPI_SP_CAP_400M | LTPI_SP_CAP_800M | LTPI_SP_CAP_1G)

/* LTPI status code */
#define LTPI_OK					0x00
#define LTPI_ERR_TIMEOUT			0x10
#define LTPI_ERR_REMOTE_DISCON			0x20
/* --gap-- 0x40 is reserved for further use */
#define LTPI_ERR_SEVERE				0x80

/**
 * @brief initialize the LTPI bus according to straps
 */
void ltpi_init(void);

/**
 * @brief Get LTPI link status
 * @param[IN] index: index of the LTPI port
 * @return true: LTPI linked. false: LTPI not linked
 */
bool ltpi_query_link_status(int index);
#endif	/* #ifndef __LTPI_H__ */
