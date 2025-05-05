// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright Aspeed Technology Inc. (C) 2025. All rights reserved
 */

#include <common.h>
#include <asm/io.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <mailbox-uclass.h>
#include <linux/bitfield.h>
#include <linux/bug.h>

/* Each bit in the register represents an IPC ID */
#define IPCR_TX_TRIG		0x00
#define IPCR_TX_ENABLE		0x04
#define IPCR_RX_ENABLE		0x104
#define IPCR_TX_STATUS		0x08
#define IPCR_RX_STATUS		0x108
#define  RX_IRQ(n)		BIT(0 + 1 * (n))
#define  RX_IRQ_MASK		0xf
#define IPCR_TX_DATA		0x10
#define IPCR_RX_DATA		0x110

struct ast2700_mbox_data {
	u8 num_chans;
	u8 msg_size;
};

struct ast2700_mbox_priv {
	void __iomem *regs;
	const struct ast2700_mbox_data *drv_data;
};

static inline int ast2700_mbox_tx_done(struct ast2700_mbox_priv *mb, int idx)
{
	return !(readl(mb->regs + IPCR_TX_STATUS) & BIT(idx));
}

static int ast2700_mbox_send(struct mbox_chan *chan, const void *data)
{
	struct ast2700_mbox_priv *mb = dev_get_priv(chan->dev);
	int idx = chan->id;
	void *data_reg;
	uint32_t *word_data;
	int num_words;

	if (!(readl(mb->regs + IPCR_TX_ENABLE) & BIT(idx))) {
		dev_warn(chan->dev, "%s: Ch-%d not enabled yet\n", __func__, idx);
		return -EBUSY;
	}

	if (!(ast2700_mbox_tx_done(mb, idx))) {
		dev_warn(chan->dev, "%s: Ch-%d last data has not finished\n", __func__, idx);
		return -EBUSY;
	}

	for (data_reg = mb->regs + IPCR_TX_DATA + mb->drv_data->msg_size * idx,
	     num_words = (mb->drv_data->msg_size / sizeof(uint32_t)),
	     word_data = (uint32_t *)data;
	     num_words; num_words--, data_reg += sizeof(uint32_t), word_data++)
		writel(*word_data, data_reg);

	writel(BIT(idx), mb->regs + IPCR_TX_TRIG);
	debug("%s: Ch-%d sent\n", __func__, idx);

	return 0;
}

static int ast2700_mbox_recv(struct mbox_chan *chan, void *data)
{
	struct ast2700_mbox_priv *mb = dev_get_priv(chan->dev);
	int idx = chan->id;
	void __iomem *data_reg;
	int num_words;
	uint32_t *word_data;
	uint32_t status;

	status = readl(mb->regs + IPCR_RX_STATUS);
	if (!(status & BIT(idx)))
		return -ENODATA;

	for (data_reg = mb->regs + IPCR_RX_DATA + mb->drv_data->msg_size * idx,
	     word_data = data,
	     num_words = (mb->drv_data->msg_size / sizeof(uint32_t));
	     num_words; num_words--, data_reg += sizeof(uint32_t), word_data++)
		*word_data = readl(data_reg);

	writel(RX_IRQ(idx), mb->regs + IPCR_RX_STATUS);
	debug("%s: Ch-%d)\n", __func__, idx);

	return 0;
}

static int ast2700_mbox_request(struct mbox_chan *chan)
{
	struct ast2700_mbox_priv *mb = dev_get_priv(chan->dev);
	int idx = chan->id;

	debug("%s: Ch-%d\n", __func__, idx);
	setbits_le32(mb->regs + IPCR_RX_ENABLE, BIT(idx));

	return 0;
}

static int ast2700_mbox_free(struct mbox_chan *chan)
{
	struct ast2700_mbox_priv *mb = dev_get_priv(chan->dev);
	int idx = chan->id;

	debug("%s: Ch-%d\n", __func__, idx);
	clrbits_le32(mb->regs + IPCR_RX_ENABLE, BIT(idx));

	return 0;
}

static int ast2700_mbox_probe(struct udevice *dev)
{
	struct ast2700_mbox_priv *priv = dev_get_priv(dev);

	priv->regs = dev_read_addr_ptr(dev);
	if (!priv->regs)
		return -EINVAL;

	priv->drv_data = (void *)dev_get_driver_data(dev);

	return 0;
}

static const struct ast2700_mbox_data ast2700_drv_data = {
	.num_chans = 4,
	.msg_size = 0x20,
};

static const struct udevice_id ast2700_mbox_of_match[] = {
	{ .compatible = "aspeed,ast2700-mailbox", .data = (ulong)&ast2700_drv_data },
	{ }
};

struct mbox_ops ast2700_mbox_ops = {
	.request = ast2700_mbox_request,
	.rfree = ast2700_mbox_free,
	.send = ast2700_mbox_send,
	.recv = ast2700_mbox_recv,
};

U_BOOT_DRIVER(ast2700_mbox) = {
	.name = "ast2700-mbox",
	.id = UCLASS_MAILBOX,
	.of_match = ast2700_mbox_of_match,
	.probe = ast2700_mbox_probe,
	.priv_auto = sizeof(struct ast2700_mbox_priv),
	.ops = &ast2700_mbox_ops,
};
