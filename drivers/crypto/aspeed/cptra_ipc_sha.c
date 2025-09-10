// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2025 ASPEED Technology Inc.
 */
#include <common.h>
#include <dm.h>
#include <u-boot/cptra_ipc.h>
#include <u-boot/hash.h>

#define CPTRA_SHA_ACC_BSIZE			0xc0000		/* 768KB */

enum cptra_sha_modes {
	CPTRA_SHA384_STREAM,
	CPTRA_SHA512_STREAM,
};

struct cptra_sha_ctx {
	enum HASH_ALGO algo;
	uint32_t dgst_len;
};

enum hash_algo {
	CRYPTO_HASH_ALGO_SHA1 = 0,
	CRYPTO_HASH_ALGO_SHA224 = 1,
	CRYPTO_HASH_ALGO_SHA256 = 2,
	CRYPTO_HASH_ALGO_SHA384 = 3,
	CRYPTO_HASH_ALGO_SHA512 = 4,
	CRYPTO_HASH_ALGO_SHA512_224 = 5,
	CRYPTO_HASH_ALGO_SHA512_256 = 6,
};

struct cptra_hash_ctx {
	uint32_t algo;
	int in_len;
	uint32_t in_buf;
	int out_len;
	uint32_t out_buf;
};

static int cptra_sha384_init(void)
{
	uint8_t *p8 = (uint8_t *)IPC_CHANNEL_1_NS_CA35_IN_ADDR;
	enum cptra_ipc_cmd ipccmd = CPTRA_IPCCMD_SHA384_INIT;
	struct cptra_hash_ctx ctx;
	int ret, rc;

	ctx.algo = CRYPTO_HASH_ALGO_SHA384;

	/* Copy input data structure into shared memory */
	memcpy((void *)p8, &ctx, sizeof(struct cptra_hash_ctx));

	ret = cptra_ipc_trigger(ipccmd);
	if (ret) {
		pr_warn("cptra_ipc_trigger:0x%x is failure, ret:0x%x\n", ipccmd, ret);
		goto end;
	}

	rc = cptra_ipc_receive(CPTRA_IPC_RX_TYPE_INTERNAL, &ret, sizeof(ret));
	if (rc)
		return rc;

	if (ret) {
		pr_warn("cptra_ipc_receive:0x%x is failure, ret:0x%x\n", ipccmd, ret);
		goto end;
	}

end:
	return ret;
}

static int cptra_sha384_update(uint8_t *msg, int msg_size)
{
	uint8_t *p8_bmcu_in = (uint8_t *)IPC_CHANNEL_1_BOOTMCU_IN_ADDR;
	uint8_t *p8 = (uint8_t *)IPC_CHANNEL_1_NS_CA35_IN_ADDR;
	enum cptra_ipc_cmd ipccmd = CPTRA_IPCCMD_SHA384_UPDATE;
	struct cptra_hash_ctx ctx;
	int ret, rc;

	pr_debug("%s: msg: 0x%lx, msg_size: 0x%x\n", __func__, (uintptr_t)msg, msg_size);

	ctx.algo = CRYPTO_HASH_ALGO_SHA384;
	ctx.in_len = msg_size;
	ctx.in_buf = (uint32_t)(uintptr_t)p8_bmcu_in + sizeof(struct cptra_hash_ctx);

	/* Copy input data structure into shared memory */
	memcpy((void *)p8, &ctx, sizeof(struct cptra_hash_ctx));
	p8 += sizeof(struct cptra_hash_ctx);

	/* Copy input data into shared memory */
	memcpy(p8, msg, msg_size);

	ret = cptra_ipc_trigger(ipccmd);
	if (ret) {
		pr_warn("cptra_ipc_trigger:0x%x is failure, ret:0x%x\n", ipccmd, ret);
		goto end;
	}

	rc = cptra_ipc_receive(CPTRA_IPC_RX_TYPE_INTERNAL, &ret, sizeof(ret));
	if (rc)
		return rc;

	if (ret) {
		pr_warn("cptra_ipc_receive:0x%x is failure, ret:0x%x\n", ipccmd, ret);
		goto end;
	}

end:
	return ret;
}

static int cptra_sha384_final(uint8_t *output, int output_size)
{
	uint8_t *p8_bmcu_out = (uint8_t *)IPC_CHANNEL_1_BOOTMCU_OUT_ADDR;
	uint8_t *p8 = (uint8_t *)IPC_CHANNEL_1_NS_CA35_IN_ADDR;
	enum cptra_ipc_cmd ipccmd = CPTRA_IPCCMD_SHA384_FINAL;
	struct cptra_hash_ctx ctx;
	int ret, rc;

	ctx.algo = CRYPTO_HASH_ALGO_SHA384;
	ctx.out_len = output_size;
	ctx.out_buf = (uint32_t)(uintptr_t)p8_bmcu_out;

	/* Copy input data structure into shared memory */
	memcpy((void *)p8, &ctx, sizeof(struct cptra_hash_ctx));

	ret = cptra_ipc_trigger(ipccmd);
	if (ret) {
		pr_warn("cptra_ipc_trigger:0x%x is failure, ret:0x%x\n", ipccmd, ret);
		goto end;
	}

	rc = cptra_ipc_receive(CPTRA_IPC_RX_TYPE_EXTERNAL, output, output_size);
	if (rc)
		return rc;

	if (ret) {
		pr_warn("cptra_ipc_receive:0x%x is failure, ret:0x%x\n", ipccmd, ret);
		goto end;
	}

end:
	return ret;
}

static int cptra_ipc_sha_init(struct udevice *dev, enum HASH_ALGO algo, void **ctxp)
{
	int ret;

	switch (algo) {
	case HASH_ALGO_SHA384:
		break;
	default:
		ret = -EINVAL;
		goto end;
	};

	ret = cptra_sha384_init();

end:
	return ret;
}

static int cptra_ipc_sha_update(struct udevice *dev, void *ctx, const void *ibuf, uint32_t ilen)
{
	int bsize = CPTRA_SHA_ACC_BSIZE;
	int size;
	int ret;

	for (int i = 0; i < ilen; i += bsize) {
		if (ilen - i > bsize)
			size = bsize;
		else
			size = ilen - i;

		ret = cptra_sha384_update((uint8_t *)ibuf + i, size);
		if (ret)
			return ret;
	}

	return 0;
}

static int cptra_ipc_sha_finish(struct udevice *dev, void *ctx, void *obuf)
{
	return cptra_sha384_final((uint8_t *)obuf, 48);
}

static int cptra_ipc_sha_digest_wd(struct udevice *dev, enum HASH_ALGO algo,
				   const void *ibuf, const uint32_t ilen,
				   void *obuf, uint32_t chunk_sz)
{
	const void *cur, *end;
	uint32_t chunk;
	void *ctx;
	int rc;

	rc = cptra_ipc_sha_init(dev, algo, &ctx);
	if (rc)
		return rc;

	if (IS_ENABLED(CONFIG_HW_WATCHDOG) || CONFIG_IS_ENABLED(WATCHDOG)) {
		cur = ibuf;
		end = ibuf + ilen;

		while (cur < end) {
			chunk = end - cur;
			if (chunk > chunk_sz)
				chunk = chunk_sz;

			rc = cptra_ipc_sha_update(dev, ctx, cur, chunk);
			if (rc)
				return rc;

			cur += chunk;
			schedule();
		}
	} else {
		rc = cptra_ipc_sha_update(dev, ctx, ibuf, ilen);
		if (rc)
			return rc;
	}

	rc = cptra_ipc_sha_finish(dev, ctx, obuf);
	if (rc)
		return rc;

	return 0;
}

static int cptra_ipc_sha_digest(struct udevice *dev, enum HASH_ALGO algo,
				const void *ibuf, const uint32_t ilen,
				void *obuf)
{
	/* re-use the watchdog version with input length as the chunk_sz */
	return cptra_ipc_sha_digest_wd(dev, algo, ibuf, ilen, obuf, ilen);
}

static int cptra_ipc_sha_probe(struct udevice *dev)
{
	return 0;
}

static int cptra_ipc_sha_remove(struct udevice *dev)
{
	return 0;
}

static const struct hash_ops cptra_sha_ops = {
	.hash_init = cptra_ipc_sha_init,
	.hash_update = cptra_ipc_sha_update,
	.hash_finish = cptra_ipc_sha_finish,
	.hash_digest_wd = cptra_ipc_sha_digest_wd,
	.hash_digest = cptra_ipc_sha_digest,
};

static const struct udevice_id cptra_sha_ids[] = {
	{ .compatible = "aspeed,cptra-ipc-sha" },
	{ }
};

U_BOOT_DRIVER(aspeed_cptra_ipc_sha) = {
	.name = "aspeed_cptra_ipc_sha",
	.id = UCLASS_HASH,
	.of_match = cptra_sha_ids,
	.ops = &cptra_sha_ops,
	.probe = cptra_ipc_sha_probe,
	.remove	= cptra_ipc_sha_remove,
	.flags = DM_FLAG_PRE_RELOC,
};
