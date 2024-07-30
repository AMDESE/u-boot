// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */

#include <asm/arch-aspeed/fmc_hdr.h>
#include <asm/io.h>
#include <asm/sections.h>
#include <errno.h>
#include <spl.h>
#include <string.h>

struct prebuilt_cache {
	uint32_t cached;
	uint32_t ofst;
	uint32_t size;
	uint8_t *dgst;
};

static struct prebuilt_cache pb_cache[PBT_NUM] = { 0 };

int fmc_hdr_get_prebuilt(uint32_t type, uint32_t *ofst, uint32_t *size, uint8_t *dgst)
{
	struct fmc_hdr_preamble *preamble;
	struct fmc_hdr_body *body;
	struct fmc_hdr *hdr;
	uint32_t t, s, o;
	uint8_t *d;
	int i;

	if (type >= PBT_NUM)
		return -EINVAL;

	if (!ofst || !size)
		return -EINVAL;

	if (pb_cache[type].cached) {
		*ofst = pb_cache[type].ofst;
		*size = pb_cache[type].size;

		if (dgst)
			memcpy(dgst, pb_cache[type].dgst, HDR_DGST_LEN);

		goto found;
	}

	hdr = (struct fmc_hdr *)(_start - sizeof(*hdr));
	preamble = &hdr->preamble;
	body = &hdr->body;

	if (preamble->magic != HDR_MAGIC)
		return -EIO;

	for (i = 0, o = sizeof(*hdr) + body->size; i < HDR_PB_MAX; ++i) {
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

		/* cache the prebuilt info. scanned */
		if (!pb_cache[t].cached) {
			pb_cache[t].ofst = o;
			pb_cache[t].size = s;
			pb_cache[t].dgst = d;
			pb_cache[t].cached = 1;
		}

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
