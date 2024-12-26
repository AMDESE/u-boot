// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2021 ASPEED Technology Inc.
 */
#include <hexdump.h>
#include <asm/io.h>
#include <dm/device.h>
#include <dm/fdtaddr.h>
#include <linux/iopoll.h>
#include <u-boot/rsa-mod-exp.h>

#include "aspeed_rsss.h"

static void aspeed_sram_read(void *dest, void __iomem *from, uint32_t len);

#ifdef ASPEED_RSSS_DEBUG
static void aspeed_debug_hexdump(const char *str, void __iomem *sram,
				 size_t len)
{
	uint8_t sram_buf[SRAM_BLOCK_SIZE] = { 0 };

	/* Read sram content */
	aspeed_sram_read(sram_buf, sram, len);

	/* Print sram content */
	print_hex_dump_bytes(str, DUMP_PREFIX_NONE, sram_buf, len);
}
#else
static void aspeed_debug_hexdump(const char *str, void __iomem *sram,
				 size_t len)
{
}
#endif

static inline void aspeed_reverse_buf(uint8_t *buf, uint32_t len)
{
	int i = 0;

	if (!buf || len > SRAM_BLOCK_SIZE)
		return;

	for (i = 0; i < (len / 2); i++)
		SWAP(buf[i], buf[len - 1 - i]);
}

static void aspeed_sram_write(void __iomem *dest, const void *from,
			      uint32_t len)
{
	uint8_t sram_buf[SRAM_BLOCK_SIZE] = { 0 };

	if (len > SRAM_BLOCK_SIZE || !dest || !from)
		return;

	memcpy(sram_buf, from, len);
	aspeed_reverse_buf(sram_buf, len);
	memcpy_toio(dest, sram_buf, ROUND(len, 8));
}

static void aspeed_sram_read(void *dest, void __iomem *from, uint32_t len)
{
	uint8_t sram_buf[SRAM_BLOCK_SIZE] = { 0 };

	if (len > SRAM_BLOCK_SIZE || !dest || !from)
		return;

	memcpy_fromio(sram_buf, from, ROUND(len, 8));
	aspeed_reverse_buf(sram_buf, len);
	memcpy(dest, sram_buf, len);
}

static void aspeed_rsa_mode_switch(struct aspeed_rsss *rsss,
				   enum aspeed_rsa_mode mode)
{
	struct aspeed_engine_rsa *rsa_engine = &rsss->rsa_engine;

	if (rsa_engine->mode != ASPEED_RSSS_RSA_AHB_CPU_MODE &&
	    rsa_engine->mode != ASPEED_RSSS_RSA_AHB_ENGINE_MODE)
		return;

	if (rsa_engine->mode == mode)
		return;

	switch (mode) {
	case ASPEED_RSSS_RSA_AHB_CPU_MODE:
		setbits_le32(rsss->base + ASPEED_RSSS_CTRL, SRAM_AHB_MODE_CPU);
		rsa_engine->mode = mode;
		break;
	case ASPEED_RSSS_RSA_AHB_ENGINE_MODE:
		clrbits_le32(rsss->base + ASPEED_RSSS_CTRL, SRAM_AHB_MODE_CPU);
		rsa_engine->mode = mode;
		break;
	default:
		break;
	}
}

static int aspeed_rsa_self_test(struct aspeed_rsss *rsss)
{
	struct aspeed_engine_rsa *rsa_engine = &rsss->rsa_engine;
	uint32_t pattern = 0xbeef;

	/* Set sram access control - cpu */
	aspeed_rsa_mode_switch(rsss, ASPEED_RSSS_RSA_AHB_CPU_MODE);

	/* Write rsa sram test - 1 */
	writel(pattern, rsa_engine->sram_exp);
	if (readl(rsa_engine->sram_exp) != pattern)
		return -ENXIO;

	/* Write rsa sram test - 2 */
	writel(0x0, rsa_engine->sram_exp);
	if (readl(rsa_engine->sram_exp))
		return -ENXIO;

	return 0;
}

static int aspeed_rsa_init(struct udevice *dev, struct aspeed_rsss *rsss)
{
	if (!dev || !rsss)
		return -EINVAL;

	/* Get rsa engine address */
	rsss->base = devfdt_get_addr_index(dev, 0);

	/* Init rsa engine sw structure */
	rsss->rsa_engine.mode = ASPEED_RSSS_RSA_AHB_CPU_MODE;
	rsss->rsa_engine.sram_exp = (void *)rsss->base + SRAM_OFFSET_EXP;
	rsss->rsa_engine.sram_mod = (void *)rsss->base + SRAM_OFFSET_MOD;
	rsss->rsa_engine.sram_data = (void *)rsss->base + SRAM_OFFSET_DATA;

	/* Enable RSSS RSA interrupt */
	setbits_le32(rsss->base + ASPEED_RSSS_INT_EN, RSA_INT_EN);

	if (aspeed_rsa_self_test(rsss))
		return -ENXIO;

	return 0;
}

static int aspeed_rsa_wait_complete(struct aspeed_rsss *rsss)
{
	uint32_t sts = 0;
	int ret = 0;

	ret = readl_poll_timeout(rsss->base + ASPEED_RSA_ENG_STS, sts,
				 ((sts & RSA_STS) == 0x0), ASPEED_RSSS_TIMEOUT);
	if (ret)
		return -ETIME;

	ret = readl_poll_timeout(rsss->base + ASPEED_RSSS_INT_STS, sts,
				 ((sts & RSA_INT_DONE) == RSA_INT_DONE),
				 ASPEED_RSSS_TIMEOUT);
	if (ret)
		return -ETIME;

	/* Clear rsss rsa interrupt status */
	setbits_le32(rsss->base + ASPEED_RSSS_INT_STS, RSA_INT_DONE);

	/* Stop rsss rsa engine */
	clrbits_le32(rsss->base + ASPEED_RSA_TRIGGER, RSA_TRIGGER);

	return 0;
}

static void aspeed_rsa_trigger(struct aspeed_rsss *rsss)
{
	/* Set sram access control - engine */
	aspeed_rsa_mode_switch(rsss, ASPEED_RSSS_RSA_AHB_ENGINE_MODE);

	/* Trigger rsa engine */
	setbits_le32(rsss->base + ASPEED_RSA_TRIGGER, RSA_TRIGGER);
}

static void aspeed_rsa_set_key(struct aspeed_rsss *rsss, const void *key_exp,
			       uint32_t keybits_e, const void *key_n,
			       uint32_t keybits_n)
{
	struct aspeed_engine_rsa *rsa_engine = &rsss->rsa_engine;

	/* Set sram access control - cpu */
	aspeed_rsa_mode_switch(rsss, ASPEED_RSSS_RSA_AHB_CPU_MODE);

	/* Set rsa key */
	aspeed_sram_write(rsa_engine->sram_exp, key_exp, TO_BYTES(keybits_e));
	aspeed_sram_write(rsa_engine->sram_mod, key_n, TO_BYTES(keybits_n));

	/* Set key information */
	writel(((keybits_e << 16) | keybits_n),
	       rsss->base + ASPEED_RSA_KEY_INFO);

	/* Debug dump */
	aspeed_debug_hexdump("exp ", rsa_engine->sram_exp, TO_BYTES(keybits_e));
	aspeed_debug_hexdump("mod ", rsa_engine->sram_mod, TO_BYTES(keybits_n));
}

static void aspeed_rsa_set_data(struct aspeed_rsss *rsss, void *data,
				uint32_t data_len)
{
	struct aspeed_engine_rsa *rsa_engine = &rsss->rsa_engine;

	/* Set sram access control - cpu */
	aspeed_rsa_mode_switch(rsss, ASPEED_RSSS_RSA_AHB_CPU_MODE);

	/* Set rsa data */
	aspeed_sram_write(rsa_engine->sram_data, data, data_len);

	/* Debug dump */
	aspeed_debug_hexdump("data ", rsa_engine->sram_data, data_len);
}

static void aspeed_rsa_get_data(struct aspeed_rsss *rsss, void *buf,
				uint32_t buf_size)
{
	struct aspeed_engine_rsa *rsa_engine = &rsss->rsa_engine;

	/* Set sram access control - cpu */
	aspeed_rsa_mode_switch(rsss, ASPEED_RSSS_RSA_AHB_CPU_MODE);

	aspeed_sram_read(buf, rsa_engine->sram_data, buf_size);
}

static int aspeed_rsss_mod_exp(struct udevice *dev, const uint8_t *sig,
			       uint32_t sig_len, struct key_prop *prop,
			       uint8_t *out)
{
	struct aspeed_rsss *rsss = dev_get_priv(dev);

	if (!rsss || !sig || !prop || !out)
		return -EINVAL;

	if (sig_len != RSA2048_BYTES && sig_len != RSA3072_BYTES &&
	    sig_len != RSA4096_BYTES)
		return -EINVAL;

	/* Send input to rsss-rsa engine */
	aspeed_rsa_set_key(rsss, prop->public_exponent, TO_BITS(prop->exp_len),
			   prop->modulus, prop->num_bits);
	aspeed_rsa_set_data(rsss, (void *)sig, sig_len);
	aspeed_rsa_trigger(rsss);

	/* Wait for rsss-rsa engine finished */
	if (aspeed_rsa_wait_complete(rsss))
		return -EIO;

	/* Collect the rsss rsa engine output */
	aspeed_rsa_get_data(rsss, out, sig_len);

	return 0;
}

static int aspeed_rsss_probe(struct udevice *dev)
{
	struct aspeed_rsss *rsss = dev_get_priv(dev);
	int ret = 0;

	/* Get rsss clock information */
	ret = clk_get_by_index(dev, 0, &rsss->clk);
	if (ret < 0)
		return ret;

	/* Get rsss reset information */
	ret = reset_get_by_index(dev, 0, &rsss->rst);
	if (ret)
		return ret;

	/* Enable rsss rsa clock */
	ret = clk_enable(&rsss->clk);
	if (ret)
		return ret;

	/* Release rsss rsa reset */
	ret = reset_deassert(&rsss->rst);
	if (ret)
		return ret;

	/* Init rsss rsa engine */
	ret = aspeed_rsa_init(dev, rsss);
	if (ret)
		return ret;

	return ret;
}

static int aspeed_rsss_remove(struct udevice *dev)
{
	return 0;
}

static const struct mod_exp_ops aspeed_rsss_ops = {
	.mod_exp = aspeed_rsss_mod_exp,
};

static const struct udevice_id aspeed_rsss_ids[] = {
	{ .compatible = "aspeed,ast2700-rsss" },
	{}
};

U_BOOT_DRIVER(aspeed_rsss) = {
	.name = "aspeed_rsss",
	.id = UCLASS_MOD_EXP,
	.of_match = aspeed_rsss_ids,
	.probe = aspeed_rsss_probe,
	.remove = aspeed_rsss_remove,
	.priv_auto = sizeof(struct aspeed_rsss),
	.ops = &aspeed_rsss_ops,
	.flags = DM_FLAG_PRE_RELOC,
};
