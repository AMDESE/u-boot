// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright Aspeed Technology Inc. (C) 2025. All rights reserved
 */

#include <common.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <errno.h>
#include <mailbox.h>
#include <misc.h>
#include <asm/cache.h>
#include <linux/ioport.h>
#include <linux/io.h>

#define ASPEED_MBOX_RECV_TIMEOUT	1000

enum _ioctl {
	ASPEED_MBOX_IOCTL_CAPS = 0,
	ASPEED_MBOX_IOCTL_SEND,
	ASPEED_MBOX_IOCTL_RECV,
};

/**
 * struct aspeed_mem - Description of a ASPEED memory buffer
 * @buf:	Shared memory base address
 * @size:	Shared memory byte size
 */
struct aspeed_mem {
	u8 *buf;
	fdt_size_t size;
};

struct aspeed_mbox {
	struct mbox_chan mbox;
	struct aspeed_mem tx_base;
	struct aspeed_mem rx_base;
};

static int aspeed_mbox_read(struct udevice *dev, int offset,
			    void *buf, int size)
{
	struct aspeed_mbox *mb = dev_get_priv(dev);

	if (offset + size > mb->rx_base.size) {
		printf("%s: read out of range\n", __func__);
		return -EINVAL;
	}

	memcpy(buf, mb->rx_base.buf + offset, size);
	return size;
}

static int aspeed_mbox_write(struct udevice *dev, int offset,
			     const void *buf, int size)
{
	struct aspeed_mbox *mb = dev_get_priv(dev);

	if (offset + size > mb->tx_base.size) {
		printf("%s: write out of range\n", __func__);
		return -EINVAL;
	}

	memcpy(mb->tx_base.buf + offset, buf, size);
	return size;
}

static int aspeed_mbox_ioctl(struct udevice *dev, unsigned long request,
			     void *buf)
{
	struct aspeed_mbox *mb = dev_get_priv(dev);
	int *data = (int *)buf;
	int ret = 0;

	switch (request) {
	case ASPEED_MBOX_IOCTL_CAPS:
		data[0] = ((uintptr_t)mb->tx_base.buf) >> 8;
		data[1] = mb->tx_base.size;
		data[2] = ((uintptr_t)mb->rx_base.buf) >> 8;
		data[3] = mb->rx_base.size;
		break;
	case ASPEED_MBOX_IOCTL_SEND:
		flush_dcache_range((ulong)mb->tx_base.buf,
				   (ulong)mb->tx_base.buf + mb->tx_base.size);
		ret = mbox_send(&mb->mbox, data);
		if (ret)
			dev_err(dev, "mbox_send() failed: %d\n", ret);
		break;
	case ASPEED_MBOX_IOCTL_RECV:
		ret = mbox_recv(&mb->mbox, data, ASPEED_MBOX_RECV_TIMEOUT);
		if (ret)
			debug("mbox_recv() failed: %d\n", ret);
		else
			invalidate_dcache_range((ulong)mb->rx_base.buf,
						(ulong)mb->rx_base.buf + mb->rx_base.size);
		break;
	default:
		break;
	}

	return ret;
}

/**
 * Get shared memory configuration defined by the referred DT phandle
 * Return with a errno compliant value.
 */
static int aspeed_mbox_get_shmem(struct udevice *dev, int index, struct aspeed_mem *mem)
{
	fdt_addr_t addr;

	addr = dev_read_addr_size_index(dev, index, &mem->size);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	mem->buf = map_physmem(addr, mem->size, MAP_NOCACHE);
	if (!mem->buf)
		return -ENOMEM;

	return 0;
}

static int aspeed_mbox_probe(struct udevice *dev)
{
	struct aspeed_mbox *priv = dev_get_priv(dev);
	int ret;

	ret = mbox_get_by_index(dev, 0, &priv->mbox);
	if (ret) {
		dev_err(dev, "mbox_get_by_index() failed: %d\n", ret);
		return ret;
	}

	ret = aspeed_mbox_get_shmem(dev, 0, &priv->tx_base);
	if (ret) {
		dev_err(dev, "Failed to get tx shm resources: %d\n", ret);
		goto err_free_mbox;
	}
	ret = aspeed_mbox_get_shmem(dev, 1, &priv->rx_base);
	if (ret) {
		priv->rx_base.buf = priv->tx_base.buf;
		priv->rx_base.size = priv->tx_base.size;
	}
	debug("shmem: rx=(%p, %llu), tx=(%p, %llu)\n",
	      priv->rx_base.buf, priv->rx_base.size,
	      priv->tx_base.buf, priv->tx_base.size);

	return 0;

err_free_mbox:
	mbox_free(&priv->mbox);

	return ret;
}

static int aspeed_mbox_remove(struct udevice *dev)
{
	struct aspeed_mbox *priv = dev_get_priv(dev);

	mbox_free(&priv->mbox);

	return 0;
}

static struct misc_ops aspeed_mbox_ops = {
	.read = aspeed_mbox_read,
	.write = aspeed_mbox_write,
	.ioctl = aspeed_mbox_ioctl,
};

static const struct udevice_id aspeed_mbox_ids[] = {
	{ .compatible = "aspeed,aspeed-mbox" },
	{ }
};

U_BOOT_DRIVER(aspeed_mbox) = {
	.name		= "aspeed-mbox",
	.id		= UCLASS_MISC,
	.of_match	= aspeed_mbox_ids,
	.probe		= aspeed_mbox_probe,
	.remove		= aspeed_mbox_remove,
	.ops		= &aspeed_mbox_ops,
	.priv_auto	= sizeof(struct aspeed_mbox),
};
