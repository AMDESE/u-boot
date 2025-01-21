// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */

#include <asm/io.h>
#include <asm/sections.h>
#include <asm/arch/stor_ast2700.h>
#include <aspeed/fmc_hdr.h>
#include <errno.h>
#include <spl.h>
#include <string.h>

static int fmc_hdr_v1_get_prebuilt(struct fmc_hdr_v1 *hdr, uint32_t type, uint32_t *ofst, uint32_t *size)
{
	struct fmc_hdr_preamble_v1 *preamble;
	struct fmc_hdr_body_v1 *body;
	uint32_t pb_max, t, s, o;
	int i;

	preamble = &hdr->preamble;
	body = &hdr->body;
	pb_max = sizeof(body->raz) / sizeof(body->pbs[0]);

	for (i = 0, o = sizeof(*hdr) + body->fmc_size; i < pb_max; ++i) {
		t = body->pbs[i].type;
		s = body->pbs[i].size;

		/* skip if unrecognized, yet */
		if (t >= PBT_NUM) {
			o += s;
			continue;
		}

		/* prebuilt end mark */
		if (t == 0 && s == 0)
			break;

		/* return the prebuilt info if found */
		if (t == type) {
			*ofst = o;
			*size = s;

			goto found;
		}

		/* update offset for next prebuilt */
		o += s;
	}

	return -ENODATA;

found:
	return 0;
}

static int fmc_hdr_v2_get_prebuilt(struct fmc_hdr_v2 *hdr, uint32_t type, uint32_t *ofst, uint32_t *size, uint8_t *dgst)
{
	struct fmc_hdr_preamble_v2 *preamble;
	struct fmc_hdr_body_v2 *body;
	uint32_t pb_max, t, s, o;
	uint8_t *d;
	int i;

	preamble = &hdr->preamble;
	body = &hdr->body;
	pb_max = sizeof(body->raz) / sizeof(body->pbs[0]);

	for (i = 0, o = sizeof(*hdr) + body->fmc_size; i < pb_max; ++i) {
		t = body->pbs[i].type;
		s = body->pbs[i].size;
		d = body->pbs[i].dgst;

		/* skip if unrecognized, yet */
		if (t >= PBT_NUM) {
			o += s;
			continue;
		}

		/* prebuilt end mark */
		if (t == 0 && s == 0)
			break;

		/* return the prebuilt info if found */
		if (t == type) {
			*ofst = o;
			*size = s;

			if (dgst)
				memcpy(dgst, d, HDR_DGST_LEN);

			goto found;
		}

		/* update offset for next prebuilt */
		o += s;
	}

	return -ENODATA;

found:
	return 0;
}

int fmc_hdr_get_prebuilt(uint32_t type, uint32_t *ofst, uint32_t *size, uint8_t *dgst)
{
	struct fmc_hdr_v1 *hdr_v1;
	struct fmc_hdr_v2 *hdr_v2;

	if (type >= PBT_NUM)
		return -EINVAL;

	if (!ofst || !size)
		return -EINVAL;

	/* try version 1 */
	hdr_v1 = (struct fmc_hdr_v1 *)(_start - sizeof(*hdr_v1));
	if (hdr_v1->preamble.magic == HDR_MAGIC && hdr_v1->preamble.version == 1)
		return fmc_hdr_v1_get_prebuilt(hdr_v1, type, ofst, size);

	/* try version 2 */
	hdr_v2 = (struct fmc_hdr_v2 *)(_start - sizeof(*hdr_v2));
	if (hdr_v2->preamble.magic == HDR_MAGIC && hdr_v2->preamble.version == 2)
		return fmc_hdr_v2_get_prebuilt(hdr_v2, type, ofst, size, dgst);

	return -ENOENT;
}

int fmc_load_image(uint32_t type, u32 *src)
{
	int ret = 0;
	u32 fw_ofst;
	u32 fw_size;

	ret = fmc_hdr_get_prebuilt(type, &fw_ofst, &fw_size, NULL);
	if (ret)
		return ret;
	ret = stor_copy((u32 *)fw_ofst, src, fw_size);

	return ret;
}

