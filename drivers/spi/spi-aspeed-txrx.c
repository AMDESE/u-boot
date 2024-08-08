// SPDX-License-Identifier: GPL-2.0+
/*
 * ASPEED pure SPI Controller driver
 * which supports both SPI half-duplex
 * and full-duplex modes.
 *
 * Copyright (c) 2024 ASPEED Corporation.
 *
 * Author:
 *     Chin-Ting Kuo <chin-ting_kuo@aspeedtech.com>
 */

#include <asm/io.h>
#include <clk.h>
#include <common.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <linux/bitops.h>
#include <linux/bug.h>
#include <linux/err.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/sizes.h>
#include <malloc.h>
#include <reset.h>
#include <spi.h>
#include <inttypes.h>

#define ASPEED_SPI_MAX_CS	5
#define CTRL_IO_MODE_USER	GENMASK(1, 0)
#define CTRL_STOP_ACTIVE	BIT(2)

#define SPI_FULL_DUPLEX_SUPPORT	0x00000001

struct aspeed_spi_regs {
	u32 conf;                       /* 0x00 CE Type Setting */
	u32 ctrl;                       /* 0x04 CE Control */
	u32 _reserved0[2];              /* 0x08 - 0x0c */
	u32 ce_ctrl[ASPEED_SPI_MAX_CS]; /* 0x10 .. 0x20 CEx Control */
	u32 _reserved1[3];              /* .. */
	u32 segment_addr[ASPEED_SPI_MAX_CS]; /* 0x30 .. 0x40 Segment Address */
	u32 _reserved2[104];            /* 0x44 - 0x1e0 */
	u32 full_duplex_rx_data;	/* 0x1e4 */
	u32 _reserved3[3];		/* 0x1e8 - 0x1f0 */
};

struct aspeed_spi_plat {
	u8 max_cs;
	uintptr_t ahb_base; /* AHB address base for all devices. */
	fdt_size_t ahb_sz; /* Overall AHB window size for all device. */
	u32 hclk_rate; /* AHB clock rate */
	struct reset_ctl rst_ctl;
	u32 flag;
};

struct aspeed_spi_dev {
	uintptr_t ahb_base;
	size_t ahb_decoded_sz;
	u32 ctrl_val;
	u32 max_freq;
};

struct aspeed_spi_priv {
	u32 num_cs;
	struct aspeed_spi_regs *regs;
	struct aspeed_spi_info *info;
	struct aspeed_spi_dev spi_devs[ASPEED_SPI_MAX_CS];
};

struct aspeed_spi_info {
	size_t min_decoded_sz;
	u32 clk_ctrl_mask;
	uintptr_t (*segment_start)(struct udevice *bus, u32 reg);
	uintptr_t (*segment_end)(struct udevice *bus, u32 reg);
	u32 (*segment_reg)(uintptr_t start, uintptr_t end);
	u32 (*get_clk_setting)(struct udevice *dev, uint hz);
};

static uintptr_t ast2700_spi_segment_start(struct udevice *bus, u32 reg)
{
	struct aspeed_spi_plat *plat = dev_get_plat(bus);
	uintptr_t start_offset = (((reg) & 0x0000ffff) << 16);

	if (start_offset == 0)
		return plat->ahb_base;

	return plat->ahb_base + start_offset;
}

static uintptr_t ast2700_spi_segment_end(struct udevice *bus, u32 reg)
{
	struct aspeed_spi_plat *plat = dev_get_plat(bus);
	uintptr_t end_offset = reg & 0xffff0000;

	/* Meaningless end_offset, set to physical ahb base. */
	if (end_offset == 0)
		return plat->ahb_base;

	return plat->ahb_base + end_offset;
}

static u32 ast2700_spi_segment_reg(uintptr_t start, uintptr_t end)
{
	if (start == end)
		return 0;

	return ((((start) >> 16) & 0x7fff) | ((end + 1) & 0x7fff0000));
}

static const u32 aspeed_spi_hclk_divs[] = {
	/* HCLK, HCLK/2, HCLK/3, HCLK/4, HCLK/5, ..., HCLK/16 */
	0xf, 0x7, 0xe, 0x6, 0xd,
	0x5, 0xc, 0x4, 0xb, 0x3,
	0xa, 0x2, 0x9, 0x1, 0x8,
	0x0
};

#define ASPEED_SPI_HCLK_DIV(i)	(aspeed_spi_hclk_divs[(i)] << 8)

static u32 ast2700_get_clk_setting(struct udevice *dev, uint max_hz)
{
	struct aspeed_spi_plat *plat = dev_get_plat(dev->parent);
	struct aspeed_spi_priv *priv = dev_get_priv(dev->parent);
	struct dm_spi_slave_plat *slave_plat = dev_get_parent_plat(dev);
	u32 hclk_clk = plat->hclk_rate;
	u32 hclk_div = 0x0400; /* default value */
	u32 i, j;
	bool found = false;

	/* FMC/SPIR10[27:24] */
	for (j = 0; j < 0xf; j++) {
		/* FMC/SPIR10[11:8] */
		for (i = 0; i < ARRAY_SIZE(aspeed_spi_hclk_divs); i++) {
			if (i == 0 && j == 0)
				continue;

			if (hclk_clk / (i + 1 + (j * 16)) <= max_hz) {
				found = true;
				break;
			}
		}

		if (found) {
			hclk_div = ((j << 24) | ASPEED_SPI_HCLK_DIV(i));
			priv->spi_devs[slave_plat->cs].max_freq =
						hclk_clk / (i + 1 + j * 16);
			break;
		}
	}

	dev_dbg(dev, "found: %s, hclk: %d, max_clk: %d\n", found ? "yes" : "no",
		hclk_clk, max_hz);

	if (found) {
		dev_dbg(dev, "base_clk: %d, h_div: %d (mask %x), speed: %d\n",
			j, i + 1, aspeed_spi_hclk_divs[i],
			priv->spi_devs[slave_plat->cs].max_freq);
	}

	return hclk_div;
}

static void aspeed_spi_cs_activate(struct aspeed_spi_priv *priv, u32 cs)
{
	struct aspeed_spi_dev *spi_dev = &priv->spi_devs[cs];
	uintptr_t ctrl_reg = (uintptr_t)&priv->regs->ce_ctrl[cs];

	/* start user mode */
	writel(spi_dev->ctrl_val, ctrl_reg);
	writel((spi_dev->ctrl_val & (~CTRL_STOP_ACTIVE)), ctrl_reg);
}

static void aspeed_spi_cs_deactivate(struct aspeed_spi_priv *priv, u32 cs)
{
	struct aspeed_spi_dev *spi_dev = &priv->spi_devs[cs];
	uintptr_t ctrl_reg = (uintptr_t)&priv->regs->ce_ctrl[cs];

	/* stop user mode */
	writel((spi_dev->ctrl_val | CTRL_STOP_ACTIVE), ctrl_reg);
}

static void aspeed_spi_tx(struct udevice *bus, u32 cs,
			  const void *dout, void *din,
			  const size_t len, bool *full_duplex_rx)
{
	struct aspeed_spi_plat *plat = dev_get_plat(bus);
	struct aspeed_spi_priv *priv = dev_get_priv(bus);
	struct aspeed_spi_dev *spi_dev = &priv->spi_devs[cs];
	uintptr_t ahb_base = spi_dev->ahb_base;
	const u8 *tx_buf = (u8 *)dout;
	u8 *rx_buf = (u8 *)din;
	u32 i;

	dev_dbg(bus, "tx: ");

	for (i = 0; i < len; i++) {
		dev_dbg(bus, "%02x ", tx_buf[i]);
		writeb(tx_buf[i], ahb_base);
		if (rx_buf && (plat->flag & SPI_FULL_DUPLEX_SUPPORT)) {
			rx_buf[i] = readb(&priv->regs->full_duplex_rx_data);
			*full_duplex_rx = true;
		}
	}

	dev_dbg(bus, "\n");
}

static int aspeed_spi_rx(struct udevice *bus, u32 cs,
			 void *din, size_t len)
{
	struct aspeed_spi_priv *priv = dev_get_priv(bus);
	struct aspeed_spi_dev *spi_dev = &priv->spi_devs[cs];
	uintptr_t ahb_base = spi_dev->ahb_base;
	u8 *rx_buf = (u8 *)din;
	u32 i;

	readsb(ahb_base, rx_buf, len);

	dev_dbg(bus, "rx: ");
	for (i = 0; i < len; i++)
		dev_dbg(bus, "%02x ", rx_buf[i]);

	dev_dbg(bus, "\n");

	return 0;
}

static int aspeed_spi_transfer(struct udevice *dev, unsigned int bitlen,
			       const void *dout, void *din, unsigned long flags)
{
	struct udevice *bus = dev->parent;
	struct aspeed_spi_priv *priv = dev_get_priv(bus);
	struct dm_spi_slave_plat *slave_plat = dev_get_parent_plat(dev);
	u32 cs = slave_plat->cs;
	u32 bytes = bitlen / 8;
	bool full_duplex_rx = false;

	dev_dbg(bus, "len 0x%x, tx %p, rx %p\n", bytes, dout, din);

	if (flags & SPI_XFER_BEGIN)
		aspeed_spi_cs_activate(priv, cs);

	if (dout) {
		aspeed_spi_tx(bus, cs, dout, din,
			      bytes, &full_duplex_rx);
	}

	if (din && !full_duplex_rx)
		aspeed_spi_rx(bus, cs, din, bytes);

	if (flags & SPI_XFER_END) {
		aspeed_spi_cs_deactivate(priv, cs);
		writel(0x00130601, &priv->regs->ce_ctrl[cs]);
	}

	return 0;
}

static void aspeed_spi_decoded_base_calculate(struct udevice *bus)
{
	struct aspeed_spi_plat *plat = dev_get_plat(bus);
	struct aspeed_spi_priv *priv = dev_get_priv(bus);
	u32 cs;

	priv->spi_devs[0].ahb_base = plat->ahb_base;

	for (cs = 1; cs < plat->max_cs; cs++) {
		priv->spi_devs[cs].ahb_base =
				priv->spi_devs[cs - 1].ahb_base +
				priv->spi_devs[cs - 1].ahb_decoded_sz;
	}
}

static int aspeed_spi_decoded_ranges_sanity(struct udevice *bus)
{
	struct aspeed_spi_plat *plat = dev_get_plat(bus);
	struct aspeed_spi_priv *priv = dev_get_priv(bus);
	u32 cs;
	size_t total_sz = 0;

	/* Check overall size. */
	for (cs = 0; cs < plat->max_cs; cs++)
		total_sz += priv->spi_devs[cs].ahb_decoded_sz;

	if (total_sz > plat->ahb_sz) {
		dev_err(bus, "invalid total size 0x%08x\n", (u32)total_sz);
		return -EINVAL;
	}

	/*
	 * Check overlay. Here, we assume the deccded ranges and
	 * address base	are monotonic increasing with CE#.
	 */
	for (cs = plat->max_cs - 1; cs > 0; cs--) {
		if (priv->spi_devs[cs].ahb_base != 0 &&
		    priv->spi_devs[cs].ahb_base <
		    priv->spi_devs[cs - 1].ahb_base +
		    priv->spi_devs[cs - 1].ahb_decoded_sz) {
			dev_err(bus, "decoded range overlay 0x%" PRIxPTR " 0x%" PRIxPTR "\n",
				priv->spi_devs[cs].ahb_base,
				priv->spi_devs[cs - 1].ahb_base);

			return -EINVAL;
		}
	}

	return 0;
}

static void aspeed_spi_decoded_range_set(struct udevice *bus)
{
	struct aspeed_spi_plat *plat = dev_get_plat(bus);
	struct aspeed_spi_priv *priv = dev_get_priv(bus);
	u32 decoded_reg_val;
	uintptr_t start_addr, end_addr;
	u32 cs;

	for (cs = 0; cs < plat->max_cs; cs++) {
		start_addr = priv->spi_devs[cs].ahb_base;
		end_addr = priv->spi_devs[cs].ahb_base +
			   priv->spi_devs[cs].ahb_decoded_sz;

		decoded_reg_val = priv->info->segment_reg(start_addr, end_addr);

		writel(decoded_reg_val, &priv->regs->segment_addr[cs]);

		dev_dbg(bus, "cs: %d, decoded_reg: 0x%x\n", cs, decoded_reg_val);
		dev_dbg(bus, "start: 0x%" PRIxPTR ", end: 0x%" PRIxPTR "\n",
			start_addr, end_addr);
	}
}

static int aspeed_spi_decoded_range_config(struct udevice *bus)
{
	int ret = 0;
	struct aspeed_spi_priv *priv = dev_get_priv(bus);
	u32 cs;
	u32 reg_val;
	size_t decoded_sz;

	/* Assign basic AHB decoded size for each CS. */
	for (cs = 0; cs < priv->num_cs; cs++) {
		reg_val = readl(&priv->regs->segment_addr[cs]);
		decoded_sz = priv->info->segment_end(bus, reg_val) -
			     priv->info->segment_start(bus, reg_val);

		if (decoded_sz < priv->info->min_decoded_sz)
			decoded_sz = priv->info->min_decoded_sz;

		priv->spi_devs[cs].ahb_decoded_sz = decoded_sz;
	}

	aspeed_spi_decoded_base_calculate(bus);
	ret = aspeed_spi_decoded_ranges_sanity(bus);
	if (ret)
		return ret;

	aspeed_spi_decoded_range_set(bus);

	return ret;
}

static int aspeed_spi_ctrl_init(struct udevice *bus)
{
	int ret;
	struct aspeed_spi_plat *plat = dev_get_plat(bus);
	struct aspeed_spi_priv *priv = dev_get_priv(bus);
	u32 cs;
	u32 reg_val;

	/* Enable write capability for all CS. */
	reg_val = readl(&priv->regs->conf);
	writel(reg_val | (GENMASK(plat->max_cs - 1, 0) << 16),
	       &priv->regs->conf);

	memset(priv->spi_devs, 0x0,
	       sizeof(struct aspeed_spi_dev) * ASPEED_SPI_MAX_CS);

	/* Initial user mode. */
	for (cs = 0; cs < priv->num_cs; cs++)
		priv->spi_devs[cs].ctrl_val = (CTRL_STOP_ACTIVE | CTRL_IO_MODE_USER);

	ret = aspeed_spi_decoded_range_config(bus);

	return ret;
}

static const struct aspeed_spi_info ast2700_fmc_info = {
	.min_decoded_sz = 0x10000,
	.clk_ctrl_mask = 0x0f000f00,
	.segment_start = ast2700_spi_segment_start,
	.segment_end = ast2700_spi_segment_end,
	.segment_reg = ast2700_spi_segment_reg,
	.get_clk_setting = ast2700_get_clk_setting,
};

static const struct aspeed_spi_info ast2700_spi_info = {
	.min_decoded_sz = 0x10000,
	.clk_ctrl_mask = 0x0f000f00,
	.segment_start = ast2700_spi_segment_start,
	.segment_end = ast2700_spi_segment_end,
	.segment_reg = ast2700_spi_segment_reg,
	.get_clk_setting = ast2700_get_clk_setting,
};

static int aspeed_spi_claim_bus(struct udevice *dev)
{
	struct udevice *bus = dev->parent;
	struct dm_spi_slave_plat *slave_plat = dev_get_parent_plat(dev);
	struct aspeed_spi_priv *priv = dev_get_priv(dev->parent);
	struct aspeed_spi_dev *spi_dev = &priv->spi_devs[slave_plat->cs];
	u32 clk_setting;

	dev_dbg(bus, "%s: claim bus CS%u\n", bus->name, slave_plat->cs);

	clk_setting = priv->info->get_clk_setting(dev, slave_plat->max_hz);
	spi_dev->ctrl_val &= ~(priv->info->clk_ctrl_mask);
	spi_dev->ctrl_val |= clk_setting;

	return 0;
}

static int aspeed_spi_release_bus(struct udevice *dev)
{
	struct udevice *bus = dev->parent;
	struct dm_spi_slave_plat *slave_plat = dev_get_parent_plat(dev);

	dev_dbg(bus, "%s: release bus CS%u\n", bus->name, slave_plat->cs);

	return 0;
}

static int aspeed_spi_set_mode(struct udevice *bus, uint mode)
{
	dev_dbg(bus, "%s: setting mode to %x\n", bus->name, mode);

	return 0;
}

static int aspeed_spi_set_speed(struct udevice *bus, uint hz)
{
	dev_dbg(bus, "%s: setting speed to %u\n", bus->name, hz);
	/*
	 * ASPEED SPI controller supports multiple CS with different
	 * clock frequency. We cannot distinguish which CS here.
	 * Thus, the related implementation is postponed to claim_bus.
	 */

	return 0;
}

static int apseed_spi_txrx_of_to_plat(struct udevice *bus)
{
	struct aspeed_spi_plat *plat = dev_get_plat(bus);
	struct aspeed_spi_priv *priv = dev_get_priv(bus);
	int ret;
	struct clk hclk;

	priv->regs = (void __iomem *)devfdt_get_addr_index(bus, 0);
	if ((uintptr_t)priv->regs == FDT_ADDR_T_NONE) {
		dev_err(bus, "wrong ctrl base\n");
		ret = -ENODEV;
		goto end;
	}

	plat->ahb_base = devfdt_get_addr_size_index(bus, 1, &plat->ahb_sz);
	if (plat->ahb_base == FDT_ADDR_T_NONE) {
		dev_err(bus, "wrong AHB base\n");
		ret = -ENODEV;
		goto end;
	}

	plat->max_cs = dev_read_u32_default(bus, "num-cs", ASPEED_SPI_MAX_CS);
	if (plat->max_cs > ASPEED_SPI_MAX_CS) {
		ret = -EINVAL;
		goto end;
	}

	ret = clk_get_by_index(bus, 0, &hclk);
	if (ret < 0) {
		dev_err(bus, "%s could not get clock: %d\n", bus->name, ret);
		goto end;
	}

	ret = reset_get_by_index(bus, 0, &plat->rst_ctl);
	if (ret) {
		plat->rst_ctl.dev = NULL;
		ret = 0;
	}

	plat->hclk_rate = clk_get_rate(&hclk);
	clk_free(&hclk);

	plat->flag = 0;
	if (dev_read_bool(bus, "spi-aspeed-full-duplex"))
		plat->flag |= SPI_FULL_DUPLEX_SUPPORT;

	dev_dbg(bus, "ctrl_base = 0x%" PRIxPTR ", ahb_base = 0x%" PRIxPTR "\n",
		(uintptr_t)priv->regs, plat->ahb_base);
	dev_dbg(bus, "ahb_size = 0x%llx\n", (u64)plat->ahb_sz);
	dev_dbg(bus, "hclk = %dMHz, max_cs = %d\n",
		plat->hclk_rate / 1000000, plat->max_cs);

end:
	return ret;
}

static int aspeed_spi_txrx_probe(struct udevice *bus)
{
	int ret;
	struct aspeed_spi_plat *plat = dev_get_plat(bus);
	struct aspeed_spi_priv *priv = dev_get_priv(bus);
	struct udevice *dev;

	priv->info = (struct aspeed_spi_info *)dev_get_driver_data(bus);

	priv->num_cs = 0;
	for (device_find_first_child(bus, &dev); dev;
	     device_find_next_child(&dev)) {
		priv->num_cs++;
	}

	if (priv->num_cs > ASPEED_SPI_MAX_CS)
		return -EINVAL;

	if (plat->rst_ctl.dev)
		reset_deassert(&plat->rst_ctl);

	ret = aspeed_spi_ctrl_init(bus);

	return ret;
}

static const struct dm_spi_ops aspeed_spi_txrx_ops = {
	.claim_bus = aspeed_spi_claim_bus,
	.release_bus = aspeed_spi_release_bus,
	.set_speed = aspeed_spi_set_speed,
	.set_mode = aspeed_spi_set_mode,
	.xfer = aspeed_spi_transfer,
};

static const struct udevice_id aspeed_spi_txrx_ids[] = {
	{ .compatible = "aspeed,ast2700-fmc-txrx", .data = (ulong)&ast2700_fmc_info, },
	{ .compatible = "aspeed,ast2700-spi-txrx", .data = (ulong)&ast2700_spi_info, },
	{ }
};

U_BOOT_DRIVER(aspeed_spi_txrx) = {
	.name = "aspeed_spi_txrx",
	.id = UCLASS_SPI,
	.of_match = aspeed_spi_txrx_ids,
	.ops = &aspeed_spi_txrx_ops,
	.of_to_plat = apseed_spi_txrx_of_to_plat,
	.plat_auto = sizeof(struct aspeed_spi_plat),
	.priv_auto = sizeof(struct aspeed_spi_priv),
	.probe = aspeed_spi_txrx_probe,
};
