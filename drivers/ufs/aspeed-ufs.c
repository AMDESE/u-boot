// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2020 ASPEED Technology Inc.
 */

#include <asm/io.h>
#include <clk.h>
#include <reset.h>
#include <common.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <ufs.h>
#include "ufs.h"

#define UFS_MPHY_RST_REG		0x0
#define UFS_MPHY_RST_N			BIT(0)
#define UFS_MPHY_RST_N_PCS		BIT(4)

#define UFS_MPHY_VCONTROL		0x98
#define UFS_MPHY_CALI_IN_1		0x90
#define UFS_MPHY_CALI_IN_0		0x8c

#define USEC_PER_SEC			1000000L
#define ASPEED_UFS_REG_HCLKDIV		0xFC

static int aspeed_ufs_link_startup_notify(struct ufs_hba *hba,
					  enum ufs_notify_change_status status)
{
	struct ufs_pa_layer_attr pwr_info;
	u32 max_gear;
	int ret;

	if (IS_ENABLED(CONFIG_ASPEED_FPGA))
		max_gear = UFS_HS_G1;
	else
		max_gear = UFS_HS_G3;

	hba->quirks |= UFSHCD_QUIRK_BROKEN_LCC;
	switch (status) {
	case PRE_CHANGE:
		return ufshcd_dme_set(hba,
				      UIC_ARG_MIB(PA_LOCAL_TX_LCC_ENABLE),
				      0);
	case POST_CHANGE:
		ufshcd_writel(hba, 0, REG_AUTO_HIBERNATE_IDLE_TIMER);

		ufshcd_dme_set(hba, UIC_ARG_MIB(PA_MAXRXHSGEAR), max_gear);
		ufshcd_dme_peer_set(hba, UIC_ARG_MIB(PA_MAXRXHSGEAR), max_gear);

		pwr_info.lane_rx = 2;
		pwr_info.lane_tx = 2;
		pwr_info.pwr_rx = SLOWAUTO_MODE;
		pwr_info.pwr_tx = SLOWAUTO_MODE;
		pwr_info.gear_rx = UFS_PWM_G1;
		pwr_info.gear_tx = UFS_PWM_G1;
		pwr_info.hs_rate = PA_HS_MODE_A;
		pwr_info.hs_rate = PA_HS_MODE_A;

		ret = ufshcd_change_power_mode(hba, &pwr_info);
		if (ret) {
			dev_err(hba->dev, "%s: Failed setting power mode, err = %d\n",
				__func__, ret);

			return ret;
		}
	}

	return 0;
}

static int aspeed_ufs_set_hclkdiv(struct ufs_hba *hba)
{
	struct clk clk;
	unsigned long core_clk_rate = 0;
	u32 core_clk_div = 0;
	int ret;

	ret = clk_get_by_name(hba->dev, "core_clk", &clk);
	if (ret) {
		dev_err(hba->dev, "failed to get core_clk clock\n");
		return ret;
	}

	core_clk_rate = clk_get_rate(&clk);
	if (IS_ERR_VALUE(core_clk_rate)) {
		dev_err(hba->dev, "%s: unable to find core_clk rate\n",
			__func__);
		return core_clk_rate;
	}

	if (IS_ENABLED(CONFIG_ASPEED_FPGA))
		core_clk_div = 24;
	else
		core_clk_div = core_clk_rate / USEC_PER_SEC;

	ufshcd_writel(hba, core_clk_div, ASPEED_UFS_REG_HCLKDIV);

	return 0;
}

static int aspeed_ufs_hce_enable_notify(struct ufs_hba *hba,
					enum ufs_notify_change_status status)
{
	switch (status) {
	case PRE_CHANGE:
		return aspeed_ufs_set_hclkdiv(hba);
	case POST_CHANGE:
	;
	}

	return 0;
}

static struct ufs_hba_ops aspeed_pltfm_hba_ops = {
	.hce_enable_notify = aspeed_ufs_hce_enable_notify,
	.link_startup_notify = aspeed_ufs_link_startup_notify,
};

static int aspeed_ufs_pltfm_probe(struct udevice *dev)
{
	int err;

	err = ufshcd_probe(dev, &aspeed_pltfm_hba_ops);
	if (err)
		dev_err(dev, "ufshcd_probe() failed %d\n", err);

	return err;
}

static int aspeed_ufs_pltfm_bind(struct udevice *dev)
{
	struct udevice *scsi_dev;

	return ufs_scsi_bind(dev, &scsi_dev);
}

static const struct udevice_id aspeed_ufs_pltfm_ids[] = {
	{
		.compatible = "aspeed,ufshc-m31-16nm",
	},
	{},
};

U_BOOT_DRIVER(aspeed_ufs_pltfm) = {
	.name		= "aspeed-ufs-pltfm",
	.id		=  UCLASS_UFS,
	.of_match	= aspeed_ufs_pltfm_ids,
	.probe		= aspeed_ufs_pltfm_probe,
	.bind		= aspeed_ufs_pltfm_bind,
};

static int aspeed_ufs_probe(struct udevice *dev)
{
	void __iomem *base;
	struct clk clk;
	struct reset_ctl rst_ctl;
	u32 reg;
	int ret;

	ret = clk_get_by_index(dev, 0, &clk);
	if (ret) {
		dev_err(dev, "failed to get UFS clock\n");
		return ret;
	}

	ret = reset_get_by_index(dev, 0, &rst_ctl);
	if (ret) {
		dev_err(dev, "failed to get UFS reset\n");
		return ret;
	}

	ret = clk_enable(&clk);
	if (ret) {
		debug("%s: clock enable failed %d\n", __func__, ret);
		return ret;
	}

	/* Reset UFS Host Controller */
	reset_assert(&rst_ctl);
	udelay(2);
	reset_deassert(&rst_ctl);

	base = dev_remap_addr_index(dev, 0);

	/* reduce signal swing */
	writel(0xd0707, base + UFS_MPHY_CALI_IN_1);
	writel(0x5ffff00, base + UFS_MPHY_CALI_IN_0);

	/* Reset MPHY */
	reg = readl(base + UFS_MPHY_RST_REG);
	reg &= ~(UFS_MPHY_RST_N | UFS_MPHY_RST_N_PCS);

	writel(reg, base + UFS_MPHY_RST_REG);
	udelay(1);
	writel(reg | UFS_MPHY_RST_N, base + UFS_MPHY_RST_REG);
	udelay(1);
	writel(reg | UFS_MPHY_RST_N | UFS_MPHY_RST_N_PCS, base + UFS_MPHY_RST_REG);

	return 0;
}

static int aspeed_ufs_remove(struct udevice *dev)
{
	void __iomem *base = dev_remap_addr_index(dev, 0);
	u32 reg = readl(base + UFS_MPHY_RST_REG);

	reg &= ~(UFS_MPHY_RST_N | UFS_MPHY_RST_N_PCS);
	writel(reg, base + UFS_MPHY_RST_REG);

	return 0;
}

static const struct udevice_id aspeed_ufs_ids[] = {
	{
		.compatible = "aspeed,ast2700-ufs",
	},
	{},
};

U_BOOT_DRIVER(aspeed_ufs) = {
	.name			= "ast2700-ufs",
	.id			= UCLASS_MISC,
	.of_match		= aspeed_ufs_ids,
	.probe			= aspeed_ufs_probe,
	.remove			= aspeed_ufs_remove,
	.flags			= DM_FLAG_OS_PREPARE,
};
