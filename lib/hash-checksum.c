// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2013, Andreas Oetken.
 */

#ifndef USE_HOSTCC
#include <common.h>
#include <fdtdec.h>
#include <asm/byteorder.h>
#include <linux/errno.h>
#include <asm/unaligned.h>
#ifndef CONFIG_DM_HASH
#include <hash.h>
#else
#include <dm.h>
#include <u-boot/hash.h>
#endif /* !CONFIG_DM_HASH */
#else
#include "fdt_host.h"
#include <hash.h>
#endif	/* !USE_HOSTCC */
#include <image.h>

int hash_calculate(const char *name,
		    const struct image_region *region,
		    int region_count, uint8_t *checksum)
{
	int ret = 0;
	void *ctx;
	int i;

#if !defined(USE_HOSTCC) && defined(CONFIG_DM_HASH)
	enum HASH_ALGO algo;
	struct udevice *dev;

	if (region_count < 1)
		return -EINVAL;

	ret = uclass_get_device(UCLASS_HASH, 0, &dev);
	if (ret) {
		debug("failed to get hash device, rc=%d\n", ret);
		return -ENODEV;
	}

	algo = hash_algo_lookup_by_name(name);
	if (algo == HASH_ALGO_INVALID) {
		debug("unsupported hash algorithm\n");
		return -EINVAL;
	}

	ret = hash_init(dev, algo, &ctx);
	if (ret)
		return ret;

	for (i = 0; i < region_count; i++) {
		ret = hash_update(dev, ctx, region[i].data, region[i].size);
		if (ret)
			return ret;
	}

	ret = hash_finish(dev, ctx, checksum);
	if (ret)
		return ret;
#else
	struct hash_algo *algo;

	if (region_count < 1)
		return -EINVAL;

	ret = hash_progressive_lookup_algo(name, &algo);
	if (ret)
		return ret;

	ret = algo->hash_init(algo, &ctx);
	if (ret)
		return ret;

	for (i = 0; i < region_count - 1; i++) {
		ret = algo->hash_update(algo, ctx, region[i].data,
					region[i].size, 0);
		if (ret)
			return ret;
	}

	ret = algo->hash_update(algo, ctx, region[i].data, region[i].size, 1);
	if (ret)
		return ret;
	ret = algo->hash_finish(algo, ctx, checksum, algo->digest_size);
	if (ret)
		return ret;
#endif

	return 0;
}
