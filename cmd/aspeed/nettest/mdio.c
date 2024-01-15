// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */
#include "platform.h"
#include "internal.h"

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

#define ASPEED_MDIO_INTERVAL_US		100
#define ASPEED_MDIO_TIMEOUT_US		(ASPEED_MDIO_INTERVAL_US * 10)

#define MII_ADDR_C45			BIT(30)
#define MII_DEVADDR_C45_SHIFT		16
#define MII_REGADDR_C45_MASK		GENMASK(15, 0)

static int aspeed_mdio_set_mdc_period(struct mdio_s *ctx, u32 mdc_period)
{
	u32 data;

	data = readl(ctx->device->base + ASPEED_MDIO_DATA);
	data &= ~ASPEED_MDIO_DATA_MDC_THRES;
	data |= FIELD_PREP(ASPEED_MDIO_DATA_MDC_THRES, mdc_period);
	writel(data, ctx->device->base + ASPEED_MDIO_DATA);

	return 0;
}

static int aspeed_mdio_op(struct mdio_s *ctx, u8 st, u8 op, u8 phyad, u8 regad, u16 data)
{
	u32 ctrl;

	debug("%s: st: %u op: %u, phyad: %u, regad: %u, data: %u\n", __func__, st, op, phyad, regad,
	      data);

	ctrl = ASPEED_MDIO_CTRL_FIRE
		| FIELD_PREP(ASPEED_MDIO_CTRL_ST, st)
		| FIELD_PREP(ASPEED_MDIO_CTRL_OP, op)
		| FIELD_PREP(ASPEED_MDIO_CTRL_PHYAD, phyad)
		| FIELD_PREP(ASPEED_MDIO_CTRL_REGAD, regad)
		| FIELD_PREP(ASPEED_MDIO_DATA_MIIRDATA, data);

	iowrite32(ctrl, ctx->device->base + ASPEED_MDIO_CTRL);
	return readl_poll_sleep_timeout(ctx->device->base + ASPEED_MDIO_CTRL, ctrl,
					!(ctrl & ASPEED_MDIO_CTRL_FIRE), ASPEED_MDIO_INTERVAL_US,
					ASPEED_MDIO_TIMEOUT_US);
}

static int aspeed_mdio_get_data(struct mdio_s *ctx)
{
	u32 data;
	int rc;

	rc = readl_poll_sleep_timeout(ctx->device->base + ASPEED_MDIO_DATA, data,
				      data & ASPEED_MDIO_DATA_IDLE, ASPEED_MDIO_INTERVAL_US,
				      ASPEED_MDIO_TIMEOUT_US);
	if (rc < 0)
		return rc;

	return FIELD_GET(ASPEED_MDIO_DATA_MIIRDATA, data);
}

static int aspeed_mdio_read_c22(struct mdio_s *mdio, int addr, int regnum)
{
	int rc;

	rc = aspeed_mdio_op(mdio, ASPEED_MDIO_CTRL_ST_C22, MDIO_C22_OP_READ,
			    addr, regnum, 0);
	if (rc < 0)
		return rc;

	return aspeed_mdio_get_data(mdio);
}

static int aspeed_mdio_write_c22(struct mdio_s *mdio, int addr, int regnum, u16 val)
{
	return aspeed_mdio_op(mdio, ASPEED_MDIO_CTRL_ST_C22, MDIO_C22_OP_WRITE, addr, regnum, val);
}

static int aspeed_mdio_read_c45(struct mdio_s *mdio, int addr, int regnum)
{
	u8 c45_dev = (regnum >> 16) & 0x1F;
	u16 c45_addr = regnum & 0xFFFF;
	int rc;

	rc = aspeed_mdio_op(mdio, ASPEED_MDIO_CTRL_ST_C45, MDIO_C45_OP_ADDR,
			    addr, c45_dev, c45_addr);
	if (rc < 0)
		return rc;

	rc = aspeed_mdio_op(mdio, ASPEED_MDIO_CTRL_ST_C45, MDIO_C45_OP_READ,
			    addr, c45_dev, 0);
	if (rc < 0)
		return rc;

	return aspeed_mdio_get_data(mdio);
}

static int aspeed_mdio_write_c45(struct mdio_s *mdio, int addr, int regnum,
				 u16 val)
{
	u8 c45_dev = (regnum >> 16) & 0x1F;
	u16 c45_addr = regnum & 0xFFFF;
	int rc;

	rc = aspeed_mdio_op(mdio, ASPEED_MDIO_CTRL_ST_C45, MDIO_C45_OP_ADDR,
			    addr, c45_dev, c45_addr);
	if (rc < 0)
		return rc;

	return aspeed_mdio_op(mdio, ASPEED_MDIO_CTRL_ST_C45, MDIO_C45_OP_WRITE,
			      addr, c45_dev, val);
}

int aspeed_mdio_read(struct mdio_s *mdio, int addr, int regnum)
{
	debug("%s: addr: %d, regnum: %d\n", __func__, addr, regnum);

	if (regnum & MII_ADDR_C45)
		return aspeed_mdio_read_c45(mdio, addr, regnum);

	return aspeed_mdio_read_c22(mdio, addr, regnum);
}

int aspeed_mdio_write(struct mdio_s *mdio, int addr, int regnum, u16 val)
{
	debug("%s: addr: %d, regnum: %d, val: 0x%x\n", __func__, addr, regnum, val);

	if (regnum & MII_ADDR_C45)
		return aspeed_mdio_write_c45(mdio, addr, regnum, val);

	return aspeed_mdio_write_c22(mdio, addr, regnum, val);
}

void aspeed_mdio_init(struct mdio_s *mdio)
{
	aspeed_reset_deassert(mdio->device);
	aspeed_mdio_set_mdc_period(mdio, 0x30);
}

/* expored interface */
extern struct mdio_s mdio_data[NUM_OF_MDIO_DEVICES];
void mdio_init(int bus_id)
{
	if (bus_id >= NUM_OF_MDIO_DEVICES)
		return;

	aspeed_mdio_init(&mdio_data[bus_id]);
	net_enable_mdio_pin(bus_id);
}

int mdio_write(int bus_id, int addr, int regnum, u16 val)
{
	if (bus_id >= NUM_OF_MDIO_DEVICES)
		return -1;

	return aspeed_mdio_write(&mdio_data[bus_id], addr, regnum, val);
}

int mdio_read(int bus_id, int addr, int regnum)
{
	if (bus_id >= NUM_OF_MDIO_DEVICES)
		return -1;

	return aspeed_mdio_read(&mdio_data[bus_id], addr, regnum);
}

int mdio_set_period(int bus_id, int mdc_period)
{
	if (bus_id >= NUM_OF_MDIO_DEVICES)
		return -1;

	return aspeed_mdio_set_mdc_period(&mdio_data[bus_id], (u32)mdc_period);
}
