// SPDX-License-Identifier: GPL-2.0+
/*
 * Aspeed MDIO driver
 *
 * (C) Copyright 2021 Aspeed Technology Inc.
 *
 * This file is inspired from the Linux kernel driver drivers/net/phy/mdio-aspeed.c
 */

#include <common.h>
#include <dm.h>
#include <log.h>
#include <miiphy.h>
#include <net.h>
#include <reset.h>
#include <linux/bitops.h>
#include <linux/bitfield.h>
#include <linux/io.h>
#include <linux/iopoll.h>

#define ASPEED_MDIO_CTRL		0x0
#define   ASPEED_MDIO_CTRL_FIRE		BIT(31)
#define   ASPEED_MDIO_CTRL_ST		BIT(28)
#define     ASPEED_MDIO_CTRL_ST_C45	0
#define     ASPEED_MDIO_CTRL_ST_C22	1
#define   ASPEED_MDIO_CTRL_OP		GENMASK(27, 26)
#define     MDIO_C22_OP_WRITE		0b01
#define     MDIO_C22_OP_READ		0b10
#define     MDIO_C45_OP_ADDR		0b00
#define     MDIO_C45_OP_WRITE		0b01
#define     MDIO_C45_OP_PREAD		0b10
#define     MDIO_C45_OP_READ		0b11
#define   ASPEED_MDIO_CTRL_PHYAD	GENMASK(25, 21)
#define   ASPEED_MDIO_CTRL_REGAD	GENMASK(20, 16)
#define   ASPEED_MDIO_CTRL_MIIWDATA	GENMASK(15, 0)

#define ASPEED_MDIO_DATA		0x4
#define   ASPEED_MDIO_DATA_MDC_THRES	GENMASK(31, 24)
#define   ASPEED_MDIO_DATA_MDIO_EDGE	BIT(23)
#define   ASPEED_MDIO_DATA_MDIO_LATCH	GENMASK(22, 20)
#define   ASPEED_MDIO_DATA_IDLE		BIT(16)
#define   ASPEED_MDIO_DATA_MIIRDATA	GENMASK(15, 0)

#define ASPEED_MDIO_TIMEOUT_US		1000

struct aspeed_mdio_priv {
	void *base;
};

static int aspeed_mdio_op(struct udevice *mdio_dev, u8 st, u8 op, u8 phyad, u8 regad, u16 data)
{
	struct aspeed_mdio_priv *priv = dev_get_priv(mdio_dev);
	u32 ctrl;

	ctrl = ASPEED_MDIO_CTRL_FIRE
		| FIELD_PREP(ASPEED_MDIO_CTRL_ST, st)
		| FIELD_PREP(ASPEED_MDIO_CTRL_OP, op)
		| FIELD_PREP(ASPEED_MDIO_CTRL_PHYAD, phyad)
		| FIELD_PREP(ASPEED_MDIO_CTRL_REGAD, regad)
		| FIELD_PREP(ASPEED_MDIO_CTRL_MIIWDATA, data);

	writel(ctrl, priv->base + ASPEED_MDIO_CTRL);
	/* Add dummy read to ensure triggering mdio controller */
	(void)readl(priv->base + ASPEED_MDIO_CTRL);

	return readl_poll_timeout(priv->base + ASPEED_MDIO_CTRL, ctrl,
				  !(ctrl & ASPEED_MDIO_CTRL_FIRE),
				  ASPEED_MDIO_TIMEOUT_US);
}

static int aspeed_mdio_get_data(struct udevice *mdio_dev)
{
	struct aspeed_mdio_priv *priv = dev_get_priv(mdio_dev);
	u32 data;
	int rc;

	rc = readl_poll_timeout(priv->base + ASPEED_MDIO_DATA, data,
				data & ASPEED_MDIO_DATA_IDLE,
				ASPEED_MDIO_TIMEOUT_US);
	if (rc < 0)
		return rc;

	return FIELD_GET(ASPEED_MDIO_DATA_MIIRDATA, data);
}

static int aspeed_mdio_read_c22(struct udevice *mdio_dev, int addr, int regnum)
{
	int rc;

	rc = aspeed_mdio_op(mdio_dev, ASPEED_MDIO_CTRL_ST_C22, MDIO_C22_OP_READ,
			    addr, regnum, 0);
	if (rc < 0)
		return rc;

	return aspeed_mdio_get_data(mdio_dev);
}

static int aspeed_mdio_write_c22(struct udevice *mdio_dev, int addr, int regnum,
				 u16 val)
{
	return aspeed_mdio_op(mdio_dev, ASPEED_MDIO_CTRL_ST_C22, MDIO_C22_OP_WRITE,
			      addr, regnum, val);
}

static int aspeed_mdio_read_c45(struct udevice *mdio_dev, int addr, int devad,
				int regnum)
{
	int rc;

	rc = aspeed_mdio_op(mdio_dev, ASPEED_MDIO_CTRL_ST_C45, MDIO_C45_OP_ADDR,
			    addr, devad, regnum);
	if (rc < 0)
		return rc;

	rc = aspeed_mdio_op(mdio_dev, ASPEED_MDIO_CTRL_ST_C45, MDIO_C45_OP_READ,
			    addr, devad, 0);
	if (rc < 0)
		return rc;

	return aspeed_mdio_get_data(mdio_dev);
}

static int aspeed_mdio_write_c45(struct udevice *mdio_dev, int addr, int devad,
				 int regnum, u16 val)
{
	int rc;

	rc = aspeed_mdio_op(mdio_dev, ASPEED_MDIO_CTRL_ST_C45, MDIO_C45_OP_ADDR,
			    addr, devad, regnum);
	if (rc < 0)
		return rc;

	return aspeed_mdio_op(mdio_dev, ASPEED_MDIO_CTRL_ST_C45, MDIO_C45_OP_WRITE,
			      addr, devad, val);
}

static int aspeed_mdio_read(struct udevice *mdio_dev, int addr, int devad, int reg)
{
	if (devad == MDIO_DEVAD_NONE)
		return aspeed_mdio_read_c22(mdio_dev, addr, reg);

	return aspeed_mdio_read_c45(mdio_dev, addr, devad, reg);
}

static int aspeed_mdio_write(struct udevice *mdio_dev, int addr, int devad, int reg, u16 val)
{
	if (devad == MDIO_DEVAD_NONE)
		return aspeed_mdio_write_c22(mdio_dev, addr, reg, val);

	return aspeed_mdio_write_c45(mdio_dev, addr, devad, reg, val);
}

static const struct mdio_ops aspeed_mdio_ops = {
	.read = aspeed_mdio_read,
	.write = aspeed_mdio_write,
};

static int aspeed_mdio_probe(struct udevice *dev)
{
	struct aspeed_mdio_priv *priv = dev_get_priv(dev);
	struct reset_ctl reset_ctl;
	int ret = 0;
	u32 reg;

	priv->base = dev_read_addr_ptr(dev);

	ret = reset_get_by_index(dev, 0, &reset_ctl);
	reset_deassert(&reset_ctl);

	/*
	 * The maximum permitted frequency of MDC is set at 2.5 MHz
	 *
	 * In Aspeed SOC:
	 * MDC period = (ASPEED_MDIO_DATA_MDC_THRES + 1) * 2 * HCLK period >= (1 / 2.5MHz)
	 *
	 * Given HCLK period = 5ns
	 * We can derive
	 *   (ASPEED_MDIO_DATA_MDC_THRES + 1) * 2 * 5ns >= 400ns
	 *   ASPEED_MDIO_DATA_MDC_THRES >= 39
	 *
	 * Choose ASPEED_MDIO_DATA_MDC_THRES = 48 > 39
	 */
	reg = readl(priv->base + ASPEED_MDIO_DATA);
	reg &= ~ASPEED_MDIO_DATA_MDC_THRES;
	reg |= FIELD_PREP(ASPEED_MDIO_DATA_MDC_THRES, 48);
	writel(reg, priv->base + ASPEED_MDIO_DATA);

	return 0;
}

static const struct udevice_id aspeed_mdio_ids[] = {
	{ .compatible = "aspeed,ast2600-mdio" },
	{ .compatible = "aspeed,ast2700-mdio" },
	{ }
};

U_BOOT_DRIVER(aspeed_mdio) = {
	.name = "aspeed_mdio",
	.id = UCLASS_MDIO,
	.of_match = aspeed_mdio_ids,
	.probe = aspeed_mdio_probe,
	.ops = &aspeed_mdio_ops,
	.plat_auto = sizeof(struct mdio_perdev_priv),
	.priv_auto = sizeof(struct aspeed_mdio_priv),
};
