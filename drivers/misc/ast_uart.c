// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <errno.h>
#include <regmap.h>
#include <syscon.h>
#include <reset.h>
#include <fdtdec.h>
#include <asm/io.h>
#include <linux/bitfield.h>
#include <linux/delay.h>

#include <xyzModem.h>
#include <asm/arch/platform.h>
#include <ast_loader.h>

#define BUF_SIZE	(1024)

struct bootuart_priv {
	u32 temp;
};

static int getcymodem(void)
{
	if (tstc())
		return (getchar());

	return -1;
}

static int bootuart_ymodem_load(u32 addr)
{
	ulong size = 0;
	int err;
	int res;
	int ret;
	connection_info_t info;
	uchar *buf = (uchar *)(addr);

	info.mode = xyzModem_ymodem;
	ret = xyzModem_stream_open(&info, &err);
	if (ret) {
		printf("spl: ymodem err - %s\n", xyzModem_error(err));
		return -1;
	}

	while ((res = xyzModem_stream_read(buf, BUF_SIZE, &err)) > 0) {
		size += res;
		buf += res;
	}

	xyzModem_stream_close(&err);
	xyzModem_stream_terminate(false, &getcymodem);

	debug("Loaded %lu bytes\n", size);

	return (int)size;
}

static int uart_init(struct udevice *dev)
{
	return 0;
}

static int uart_load(struct udevice *dev, u32 *dst, u32 *len)
{
	u32 received;

	debug("dst=0x%x\n", (u32)dst);

	received = bootuart_ymodem_load((u32)dst);
	if (len < 0)
		return received;

	*len = received;

	return 0;
}

static const struct udevice_id bootuart_ids[] = {
	{ .compatible = "aspeed,bootuart" },
	{ }
};

static struct ast_loader_ops bootuart_ops = {
	.init = uart_init,
	.load = uart_load,
};

U_BOOT_DRIVER(bootuart) = {
	.name		= "bootuart",
	.id		= UCLASS_MISC,
	.of_match	= bootuart_ids,
	.priv_auto	= sizeof(struct bootuart_priv),
	.ops		= &bootuart_ops,
};
