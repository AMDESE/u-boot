// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 Aspeed Technology Inc.
 */

#include <common.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <errno.h>
#include <generic-phy.h>
#include <regmap.h>
#include <soc.h>
#include <syscon.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/bitfield.h>

#define SGMII_CFG		0x00
#define SGMII_PHY_PIPE_CTL	0x20
#define SGMII_MODE		0x30

#define SGMII_CFG_FIFO_MODE		BIT(0)
#define SGMII_CFG_SPEED_10M		0
#define SGMII_CFG_SPEED_100M		BIT(4)
#define SGMII_CFG_SPEED_1G		BIT(5)
#define SGMII_CFG_PWR_DOWN		BIT(11)
#define SGMII_CFG_AN_ENABLE		BIT(12)
#define SGMII_CFG_SW_RESET		BIT(15)
#define SGMII_PCTL_TX_DEEMPH_3_5DB	BIT(6)
#define SGMII_MODE_ENABLE		BIT(0)
#define SGMII_MODE_USE_LOCAL_CONFIG	BIT(2)

#define PLDA_CLK		0x268

#define PLDA_CLK_SEL_INTERNAL_25M	BIT(8)
#define PLDA_CLK_FREQ_MULTI		GENMASK(7, 0)

struct aspeed_sgmii {
	phys_addr_t regs;
	struct regmap *plda_regmap;
};

static int aspeed_sgmii_phy_init(struct phy *phy)
{
	struct udevice *dev = phy->dev;
	struct aspeed_sgmii *sgmii = dev_get_priv(dev);
	u32 reg;

	/*
	 * The PLDA frequency multiplication is X xor 0x19.
	 * (X xor 0x19) * clock source = data rate.
	 * SGMII data rate is 1.25G, so (0x2b xor 0x19) * 25MHz is equal 1.25G.
	 */
	reg = PLDA_CLK_SEL_INTERNAL_25M | FIELD_PREP(PLDA_CLK_FREQ_MULTI, 0x2b);
	regmap_write(sgmii->plda_regmap, PLDA_CLK, reg);

	writel(0, sgmii->regs + SGMII_MODE);

	writel(0, sgmii->regs + SGMII_CFG);
	reg = SGMII_CFG_SW_RESET | SGMII_CFG_PWR_DOWN;
	writel(reg, sgmii->regs + SGMII_CFG);

	reg = SGMII_CFG_AN_ENABLE;
	writel(reg, sgmii->regs + SGMII_CFG);

	writel(SGMII_PCTL_TX_DEEMPH_3_5DB, sgmii->regs + SGMII_PHY_PIPE_CTL);
	reg = SGMII_MODE_USE_LOCAL_CONFIG | SGMII_MODE_ENABLE;
	writel(reg, sgmii->regs + SGMII_MODE);

	return 0;
}

static int aspeed_sgmii_phy_set_speed(struct phy *phy, int speed)
{
	struct udevice *dev = phy->dev;
	struct aspeed_sgmii *sgmii = dev_get_priv(dev);
	u32 reg = 0;

	if (speed == 10)
		reg |= SGMII_CFG_SPEED_10M;
	else if (speed == 100)
		reg |= SGMII_CFG_SPEED_100M;
	else
		reg |= SGMII_CFG_SPEED_1G;
	writel(reg, sgmii->regs + SGMII_CFG);

	return 0;
}

struct phy_ops aspeed_sgmii_phy_ops = {
	.init = aspeed_sgmii_phy_init,
	.set_speed = aspeed_sgmii_phy_set_speed,
};

int aspeed_sgmii_probe(struct udevice *dev)
{
	struct aspeed_sgmii *sgmii = dev_get_priv(dev);

	sgmii->regs = dev_read_addr(dev);
	if (!sgmii->regs)
		return -EINVAL;

	sgmii->plda_regmap = syscon_regmap_lookup_by_phandle(dev, "aspeed,plda");
	if (IS_ERR(sgmii->plda_regmap)) {
		dev_err(dev, "Unable to find plda regmap (%ld)\n", PTR_ERR(sgmii->plda_regmap));
		return PTR_ERR(sgmii->plda_regmap);
	}

	return 0;
}

static const struct udevice_id aspeed_sgmii_ids[] = {
	{ .compatible = "aspeed,ast2700-sgmii" },
	{ }
};

U_BOOT_DRIVER(aspeed_sgmii) = {
	.name = "aspeed_sgmii",
	.id = UCLASS_PHY,
	.of_match = aspeed_sgmii_ids,
	.probe = aspeed_sgmii_probe,
	.ops = &aspeed_sgmii_phy_ops,
	.priv_auto = sizeof(struct aspeed_sgmii),
};
