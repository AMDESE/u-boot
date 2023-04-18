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

#define UFS_MPHY_RST_REG		0x0
#define UFS_MPHY_RST_N			BIT(0)
#define UFS_MPHY_RST_N_PCS		BIT(4)

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
