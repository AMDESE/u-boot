/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#ifndef __FMC_HDR_H__
#define __FMC_HDR_H__

#include <linux/types.h>

#define HDR_MAGIC		0x48545341	/* ASTH */
#define HDR_ECC_SIGN_LEN	96		/* ECDSA384 r and s */
#define HDR_LMS_SIGN_LEN	1620		/* LMS N24/H15/W4 */
#define HDR_DGST_LEN		48		/* SHA384 */
#define HDR_PB_MAX		13

enum prebuilt_type {
	PBT_END_MARK = 0x0,

	PBT_DDR4_PMU_TRAIN_IMEM,
	PBT_DDR4_PMU_TRAIN_DMEM,
	PBT_DDR4_2D_PMU_TRAIN_IMEM,
	PBT_DDR4_2D_PMU_TRAIN_DMEM,
	PBT_DDR5_PMU_TRAIN_IMEM,
	PBT_DDR5_PMU_TRAIN_DMEM,
	PBT_DP_FW,
	PBT_UEFI_X64_AST2700,

	PBT_NUM
};

struct fmc_hdr_preamble {
	uint32_t magic;
	uint16_t ecc_key_idx;
	uint16_t lms_key_idx;
	uint8_t ecc_sign[HDR_ECC_SIGN_LEN];
	uint8_t lms_sign[HDR_LMS_SIGN_LEN];
	uint32_t raz[17];
};

struct fmc_hdr_body {
	uint32_t size : 24;
	uint32_t svn : 8;
	uint8_t dgst[HDR_DGST_LEN];
	union {
		struct {
			uint32_t size : 24;
			uint32_t type : 8;
			uint8_t dgst[HDR_DGST_LEN];
		} pbs[0];
		uint32_t raz[179];
	};
};

struct fmc_hdr {
	struct fmc_hdr_preamble preamble;
	struct fmc_hdr_body body;
} __packed;

int fmc_hdr_get_prebuilt(uint32_t type, uint32_t *ofst, uint32_t *size, uint8_t *dgst);

#endif
