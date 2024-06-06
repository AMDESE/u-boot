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
#include <sdhci.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <dm/lists.h>

struct aspeed_sdhci_plat {
	struct mmc_config cfg;
	struct mmc mmc;
};

static int aspeed_sdhci_probe(struct udevice *dev)
{
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct aspeed_sdhci_plat *plat = dev_get_plat(dev);
	struct sdhci_host *host = dev_get_priv(dev);
	u32 max_clk;
	struct clk clk;
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

	host->name = dev->name;
	host->ioaddr = dev_read_addr_ptr(dev);

	max_clk = clk_get_rate(&clk);
	if (IS_ERR_VALUE(max_clk)) {
		ret = max_clk;
		dev_err(dev, "clock rate get failed\n");
		goto err;
	}

	host->max_clk = max_clk;
	host->mmc = &plat->mmc;
	host->mmc->dev = dev;
	host->mmc->priv = host;
	upriv->mmc = host->mmc;

	host->bus_width = dev_read_u32_default(dev, "bus-width", 4);

	if (host->bus_width == 8)
		host->host_caps |= MMC_MODE_8BIT;

	ret = sdhci_setup_cfg(&plat->cfg, host, 0, 0);
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
