// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 IBM Corp.
 * Eddie James <eajames@linux.ibm.com>
 */

#include <common.h>
#include <clk.h>
#include <reset.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <malloc.h>
#include <mmc.h>
#include <sdhci.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <dm/lists.h>
#include <power/regulator.h>

#define ASPEED_SDC_PHASE		0xf4
#define   ASPEED_SDC_S1_PHASE_IN	GENMASK(25, 21)
#define   ASPEED_SDC_S0_PHASE_IN	GENMASK(20, 16)
#define   ASPEED_SDC_S0_PHASE_IN_SHIFT	16
#define   ASPEED_SDC_S0_PHASE_OUT_SHIFT 3
#define   ASPEED_SDC_S1_PHASE_OUT	GENMASK(15, 11)
#define   ASPEED_SDC_S1_PHASE_IN_EN	BIT(10)
#define   ASPEED_SDC_S1_PHASE_OUT_EN	GENMASK(9, 8)
#define   ASPEED_SDC_S0_PHASE_OUT	GENMASK(7, 3)
#define   ASPEED_SDC_S0_PHASE_IN_EN	BIT(2)
#define   ASPEED_SDC_S0_PHASE_OUT_EN	GENMASK(1, 0)
#define   ASPEED_SDC_PHASE_MAX		31

#define ASPEED_SDHCI_TAP_PARAM_INVERT_CLK	BIT(4)
#define ASPEED_SDHCI_NR_TAPS		15

struct aspeed_sdhci_plat {
	struct mmc_config cfg;
	struct mmc mmc;
};

#ifdef MMC_SUPPORTS_TUNING
static int aspeed_execute_tuning(struct mmc *mmc, u8 opcode)
{
	struct sdhci_host *host = mmc->priv;
	u32 val, left, right, edge;
	u32 window, oldwindow = 0, center;
	u32 in_phase, out_phase, enable_mask, inverted = 0;

	out_phase = sdhci_readl(host, ASPEED_SDC_PHASE) & ASPEED_SDC_S0_PHASE_OUT;

	enable_mask = ASPEED_SDC_S0_PHASE_OUT_EN | ASPEED_SDC_S0_PHASE_IN_EN;

	/*
	 * There are two window upon clock rising and falling edge.
	 * Iterate each tap delay to find the valid window and choose the
	 * bigger one, set the tap delay at the middle of window.
	 */
	for (edge = 0; edge < 2; edge++) {
		if (edge == 1)
			inverted = ASPEED_SDHCI_TAP_PARAM_INVERT_CLK;

		val = (out_phase | enable_mask | (inverted << ASPEED_SDC_S0_PHASE_IN_SHIFT));

		/* find the left boundary */
		for (left = 0; left < ASPEED_SDHCI_NR_TAPS + 1; left++) {
			in_phase = val | (left << ASPEED_SDC_S0_PHASE_IN_SHIFT);
			sdhci_writel(host, in_phase, ASPEED_SDC_PHASE);
			if (!mmc_send_tuning(mmc, opcode, NULL))
				break;
		}

		/* find the right boundary */
		for (right = left + 1; right < ASPEED_SDHCI_NR_TAPS + 1; right++) {
			in_phase = val | (right << ASPEED_SDC_S0_PHASE_IN_SHIFT);
			sdhci_writel(host, in_phase, ASPEED_SDC_PHASE);
			if (mmc_send_tuning(mmc, opcode, NULL))
				break;
		}

		window = right - left;
		pr_debug("tuning window[%d][%d~%d] = %d\n", edge, left, right, window);

		if (window > oldwindow) {
			oldwindow = window;
			center = (((right - 1) + left) / 2) | inverted;
		}
	}

	val = (out_phase | enable_mask | (center << ASPEED_SDC_S0_PHASE_IN_SHIFT));
	sdhci_writel(host, val, ASPEED_SDC_PHASE);

	pr_debug("input tuning result=%x\n", val);

	inverted = 0;
	out_phase = val & ~ASPEED_SDC_S0_PHASE_OUT;
	in_phase = out_phase;
	oldwindow = 0;

	for (edge = 0; edge < 2; edge++) {
		if (edge == 1)
			inverted = ASPEED_SDHCI_TAP_PARAM_INVERT_CLK;

		val = (in_phase | enable_mask | (inverted << ASPEED_SDC_S0_PHASE_OUT_SHIFT));

		/* find the left boundary */
		for (left = 0; left < ASPEED_SDHCI_NR_TAPS + 1; left++) {
			out_phase = val | (left << ASPEED_SDC_S0_PHASE_OUT_SHIFT);
			sdhci_writel(host, out_phase, ASPEED_SDC_PHASE);

			if (!mmc_send_tuning(mmc, opcode, NULL))
				break;
		}

		/* find the right boundary */
		for (right = left + 1; right < ASPEED_SDHCI_NR_TAPS + 1; right++) {
			out_phase = val | (right << ASPEED_SDC_S0_PHASE_OUT_SHIFT);
			sdhci_writel(host, out_phase, ASPEED_SDC_PHASE);

			if (mmc_send_tuning(mmc, opcode, NULL))
				break;
		}

		window = right - left;
		pr_debug("tuning window[%d][%d~%d] = %d\n", edge, left, right, window);

		if (window > oldwindow) {
			oldwindow = window;
			center = (((right - 1) + left) / 2) | inverted;
		}
	}

	val = (in_phase | enable_mask | (center << ASPEED_SDC_S0_PHASE_OUT_SHIFT));
	sdhci_writel(host, val, ASPEED_SDC_PHASE);

	pr_debug("output tuning result=%x\n", val);

	return mmc_send_tuning(mmc, opcode, NULL);
}

static int aspeed_set_ios_post(struct sdhci_host *host)
{
	struct mmc *mmc = host->mmc;
	u32 reg;
	u32 drv;

	drv = dev_read_u32_default(mmc->dev, "sdhci-drive-type", 0);

	reg = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	reg &= ~SDHCI_CTRL_DRV_TYPE_MASK;

	switch (mmc->selected_mode) {
	case MMC_LEGACY:
	case SD_HS:
	case UHS_SDR50:
	case UHS_DDR50:
	case UHS_SDR104:
		reg |= ((drv & 0x3) << 4);
		break;
	default:
		break;
	}

	sdhci_writew(host, reg, SDHCI_HOST_CONTROL2);

	return 0;
}

static struct sdhci_ops aspeed_sdhci_ops = {
	.set_control_reg = sdhci_set_control_reg,
	.set_ios_post = aspeed_set_ios_post,
	.platform_execute_tuning = aspeed_execute_tuning,
};
#endif

static int aspeed_sdhci_probe(struct udevice *dev)
{
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct aspeed_sdhci_plat *plat = dev_get_plat(dev);
	struct sdhci_host *host = dev_get_priv(dev);
	u32 max_clk;
	u32 max_freq;
	struct clk clk;
	int ret;

	max_freq = dev_read_u32_default(dev, "max-frequency", 4);

	ret = clk_get_by_index(dev, 0, &clk);
	if (ret) {
		dev_err(dev, "clock get failed\n");
		return ret;
	}

	ret = clk_enable(&clk);
	if (ret) {
		dev_err(dev, "clock enable failed");
		goto free;
	}

	host->name = dev->name;
	host->ioaddr = dev_read_addr_ptr(dev);

	max_clk = clk_get_rate(&clk);
	if (IS_ERR_VALUE(max_clk)) {
		ret = max_clk;
		dev_err(dev, "clock rate get failed\n");
		goto err;
	}

#ifdef MMC_SUPPORTS_TUNING
	host->ops = &aspeed_sdhci_ops;
#endif
	host->max_clk = max_clk;
	host->mmc = &plat->mmc;
	host->mmc->dev = dev;
	host->mmc->priv = host;
	upriv->mmc = host->mmc;

	host->bus_width = dev_read_u32_default(dev, "bus-width", 4);

	if (host->bus_width == 8)
		host->host_caps |= MMC_MODE_8BIT;

	ret = sdhci_setup_cfg(&plat->cfg, host, max_freq, 400000);
	if (ret)
		goto err;

	ret = sdhci_probe(dev);
	if (ret)
		goto err;

	return 0;

err:
	clk_disable(&clk);
free:
	clk_free(&clk);
	return ret;
}

static int aspeed_sdhci_bind(struct udevice *dev)
{
	struct aspeed_sdhci_plat *plat = dev_get_plat(dev);

	return sdhci_bind(dev, &plat->mmc, &plat->cfg);
}

static const struct udevice_id aspeed_sdhci_ids[] = {
	{ .compatible = "aspeed,ast2400-sdhci" },
	{ .compatible = "aspeed,ast2500-sdhci" },
	{ .compatible = "aspeed,ast2600-sdhci" },
	{ .compatible = "aspeed,ast2700-sdhci" },
	{ }
};

U_BOOT_DRIVER(aspeed_sdhci_drv) = {
	.name		= "aspeed_sdhci",
	.id		= UCLASS_MMC,
	.of_match	= aspeed_sdhci_ids,
	.ops		= &sdhci_ops,
	.bind		= aspeed_sdhci_bind,
	.probe		= aspeed_sdhci_probe,
	.priv_auto	= sizeof(struct sdhci_host),
	.plat_auto	= sizeof(struct aspeed_sdhci_plat),
};

#define TIMING_PHASE_OFFSET 0xf4
#define SDHCI140_SLOT_0_MIRROR_OFFSET 0x10
#define SDHCI240_SLOT_0_MIRROR_OFFSET 0x20
#define SDHCI140_SLOT_0_CAP_REG_1_OFFSET 0x140
#define SDHCI240_SLOT_0_CAP_REG_1_OFFSET 0x240

static int aspeed_sdc_probe(struct udevice *dev)
{
	struct reset_ctl rst_ctl;
	void *sdhci_ctrl_base;
	struct clk clk;
	u32 timing_phase;
	u32 reg_val;
	int ret;

	ret = clk_get_by_index(dev, 0, &clk);
	if (ret) {
		dev_err(dev, "clock get failed\n");
		return ret;
	}

	ret = clk_enable(&clk);
	if (ret) {
		dev_err(dev, "clock enable failed");
		goto free;
	}

	ret = reset_get_by_index(dev, 0, &rst_ctl);
	if (!ret) {
		reset_assert(&rst_ctl);
		udelay(2);
		reset_deassert(&rst_ctl);
	}

	sdhci_ctrl_base = dev_read_addr_ptr(dev);
	if (!sdhci_ctrl_base)
		return -EINVAL;

	timing_phase = dev_read_u32_default(dev, "timing-phase", 0);
	writel(timing_phase, sdhci_ctrl_base + TIMING_PHASE_OFFSET);

	if (dev_read_bool(dev, "sdhci_hs200")) {
		reg_val = readl(sdhci_ctrl_base + SDHCI140_SLOT_0_CAP_REG_1_OFFSET);
		/* support 1.8V */
		reg_val |= BIT(26);
		writel(reg_val, sdhci_ctrl_base + SDHCI140_SLOT_0_MIRROR_OFFSET);
		reg_val = readl(sdhci_ctrl_base + SDHCI240_SLOT_0_CAP_REG_1_OFFSET);
		/* support 1.8V */
		reg_val |= BIT(26);
		writel(reg_val, sdhci_ctrl_base + SDHCI240_SLOT_0_MIRROR_OFFSET);
	}

	return 0;

free:
	clk_free(&clk);

	return ret;
}

static const struct udevice_id aspeed_sdc_ids[] = {
	{ .compatible = "aspeed,ast2400-sd-controller" },
	{ .compatible = "aspeed,ast2500-sd-controller" },
	{ .compatible = "aspeed,ast2600-sd-controller" },
	{ .compatible = "aspeed,ast2700-sd-controller" },
	{ }
};

U_BOOT_DRIVER(aspeed_sdc_drv) = {
	.name		= "aspeed_sdc",
	.id		= UCLASS_MISC,
	.of_match	= aspeed_sdc_ids,
	.probe		= aspeed_sdc_probe,
};
