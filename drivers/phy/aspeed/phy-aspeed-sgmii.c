// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 Aspeed Technology Inc.
 */

#include <common.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <generic-phy.h>
#include <regmap.h>
#include <soc.h>
#include <syscon.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/bitfield.h>

#define SGMII_CFG			0x00
#define   SGMII_CFG_FIFO_MODE			BIT(0)
#define   SGMII_CFG_SPEED_SEL_MASK		GENMASK(5, 4)
#define   SGMII_CFG_SPEED_SEL(x)		FIELD_PREP(SGMII_CFG_SPEED_SEL_MASK, (x))
#define   SGMII_CFG_PWR_DOWN			BIT(11)
#define   SGMII_CFG_AN_ENABLE			BIT(12)
#define  SGMII_CFG_SW_RESET			BIT(15)
#define SGMII_LINK_TIMER		0x08
#define SGMII_NWAY_ACK			0x0c
#define SGMII_PHY_CFG1			0x18
#define   SGMII_PHY_SPEED_MASK			GENMASK(3, 2)
#define   SGMII_PHY_SPEED(x)			FIELD_PREP(SGMII_PHY_SPEED_MASK, (x))
#define SGMII_PHY_PIPE_CTL		0x20
#define   SGMII_PCTL_TX_NO_DEEMPH		BIT(7)
#define SGMII_FIFO_DELAY_THREHOLD	0x28
#define SGMII_MODE			0x30
#define   SGMII_MODE_ENABLE			BIT(0)
#define   SGMII_MODE_USE_LOCAL_CONFIG		BIT(2)

#define PCIEPHY_CLK			0x268
#define   PCIEPHY_CLK_FREQ_MULTI_MASK		GENMASK(7, 0)
#define   PCIEPHY_CLK_FREQ_MULTI(x)		FIELD_PREP(PCIEPHY_CLK_FREQ_MULTI_MASK, (x))
#define   PCIEPHY_CLK_SEL_INTERNAL_25M		BIT(8)

#define SGMII_SPEED_10M		0x00
#define SGMII_SPEED_100M	0x01
#define SGMII_SPEED_1G		0x02

struct aspeed_sgmii {
	phys_addr_t regs;
	struct regmap *plda_regmap;

	struct phy pcie_phy;
};

static int aspeed_sgmii_conf(struct phy *phy, bool nway, int speed)
{
	struct udevice *dev = phy->dev;
	struct aspeed_sgmii *sgmii = dev_get_priv(dev);
	u32 cfg;

	writel(0, sgmii->regs + SGMII_MODE);

	writel(0, sgmii->regs + SGMII_CFG);
	writel(SGMII_CFG_SW_RESET | SGMII_CFG_PWR_DOWN, sgmii->regs + SGMII_CFG);
	if (nway) {
		writel(SGMII_CFG_AN_ENABLE, sgmii->regs + SGMII_CFG);
	} else {
		switch (speed) {
		case 10:
			cfg = SGMII_SPEED_10M;
			break;
		case 100:
			cfg = SGMII_SPEED_100M;
			break;
		case 1000:
			cfg = SGMII_SPEED_1G;
			break;
		default:
			return -EINVAL;
		}
		writel(SGMII_PHY_SPEED(cfg), sgmii->regs + SGMII_PHY_CFG1);
		writel(SGMII_CFG_SPEED_SEL(cfg), sgmii->regs + SGMII_CFG);
	}

	writel(0x0c, sgmii->regs + SGMII_FIFO_DELAY_THREHOLD);
	writel(SGMII_PCTL_TX_NO_DEEMPH, sgmii->regs + SGMII_PHY_PIPE_CTL);

	/* Set link timer for state change */
	writel(0x100, sgmii->regs + SGMII_LINK_TIMER);

	/* Bit 0 always sets to 1 in ACK message */
	writel(0x1, sgmii->regs + SGMII_NWAY_ACK);

	cfg = SGMII_MODE_ENABLE;
	if (!nway)
		cfg |= SGMII_MODE_USE_LOCAL_CONFIG;
	writel(cfg, sgmii->regs + SGMII_MODE);

	return 0;
}

static int aspeed_sgmii_phy_init(struct phy *phy)
{
	/* Default to enable Nway, not need configure speed */
	return aspeed_sgmii_conf(phy, true, 0);
}

int aspeed_sgmii_phy_set_speed(struct phy *phy, int speed)
{
	return aspeed_sgmii_conf(phy, false, speed);
}

int aspeed_sgmii_phy_exit(struct phy *phy)
{
	struct udevice *dev = phy->dev;
	struct aspeed_sgmii *sgmii = dev_get_priv(dev);

	/* Disable SGMII controller */
	writel(0, sgmii->regs + SGMII_MODE);

	return 0;
}

struct phy_ops aspeed_sgmii_phy_ops = {
	.init = aspeed_sgmii_phy_init,
	.set_speed = aspeed_sgmii_phy_set_speed,
	.exit = aspeed_sgmii_phy_exit,
};

int aspeed_sgmii_probe(struct udevice *dev)
{
	struct aspeed_sgmii *sgmii = dev_get_priv(dev);
	int ret;

	sgmii->regs = dev_read_addr(dev);
	if (!sgmii->regs)
		return -EINVAL;

	ret = generic_phy_get_by_index(dev, 0, &sgmii->pcie_phy);
	if (ret)
		return ret;

	ret = generic_phy_init(&sgmii->pcie_phy);
	if (ret)
		return ret;

	return 0;
}

static const struct udevice_id aspeed_sgmii_ids[] = {
	{ .compatible = "aspeed,ast2700-sgmii" },
	{ }
};

U_BOOT_DRIVER(phy_aspeed_sgmii) = {
	.name = "aspeed-sgmii",
	.id = UCLASS_PHY,
	.of_match = aspeed_sgmii_ids,
	.probe = aspeed_sgmii_probe,
	.ops = &aspeed_sgmii_phy_ops,
	.priv_auto = sizeof(struct aspeed_sgmii),
};
