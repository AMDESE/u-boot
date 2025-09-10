// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2025 ASPEED Technology Inc.
 */
#include <config.h>
#include <common.h>
#include <dm.h>
#include <misc.h>
#include <u-boot/cptra_ipc.h>

enum misc_op {
	MISC_OP_READ,
	MISC_OP_WRITE
};

enum mbox_ioctl {
	MBOX_IOCTL_CAPS,
	MBOX_IOCTL_SEND,
	MBOX_IOCTL_RECV,
};

static char *misc_op_str[] = {
	"read",
	"write"
};

static char *mbox_ioctl_str[] = {
	"caps",
	"send",
	"recv"
};

static int do_misc_op(enum misc_op op, void *buff, int size)
{
	struct udevice *dev;
	int offset = 0;
	int ret;

	ret = uclass_get_device_by_name(UCLASS_MISC, MBOX_BOOTMCU_DEVICE_NAME, &dev);
	if (ret) {
		pr_warn("Unable to find device %s\n", MBOX_BOOTMCU_DEVICE_NAME);
		return ret;
	}

	if (op == MISC_OP_READ)
		ret = misc_read(dev, offset, buff, size);
	else
		ret = misc_write(dev, offset, buff, size);

	if (ret < 0) {
		if (ret == -ENOSYS) {
			pr_warn("The device does not support %s\n",
				misc_op_str[op]);
			ret = 0;
		}
	} else {
		if (ret == size)
			ret = 0;
		else
			pr_warn("Partially %s %d bytes\n", misc_op_str[op], ret);
	}

	return ret;
}

static int do_mbox_ioctl(int cmd, void *buff)
{
	struct udevice *dev;
	int ret;

	ret = uclass_get_device_by_name(UCLASS_MISC, MBOX_BOOTMCU_DEVICE_NAME, &dev);
	if (ret) {
		pr_warn("Unable to find device %s\n", MBOX_BOOTMCU_DEVICE_NAME);
		return ret;
	}

	ret = misc_ioctl(dev, cmd, buff);
	if (ret < 0) {
		if (ret == -ENOSYS) {
			pr_warn("The device does not support %s\n",
				mbox_ioctl_str[cmd]);
			ret = 0;
		}

		pr_warn("ioctl(%s) failed: %d\n", mbox_ioctl_str[cmd], ret);
	}

	return ret;
}

int cptra_ipc_trigger(enum cptra_ipc_cmd cmd)
{
	uint32_t data[8] = {0};
	int ret;

	data[0] = cmd;
	data[1] = (uint32_t)IPC_CHANNEL_1_BOOTMCU_IN_ADDR;
	data[2] = (uint32_t)IPC_CHANNEL_1_BOOTMCU_OUT_ADDR;

	ret = do_mbox_ioctl(MBOX_IOCTL_SEND, data);
	if (ret < 0) {
		pr_warn("error: ioctl (MBOX_IOCTL_SEND)");
		return 1;
	}

	return 0;
}

int cptra_ipc_receive(enum cptra_ipc_rx_type type, void *output, int output_size)
{
	uint32_t msg[8];
	int ret = 0;

	/* polling to check rx ready */
	while (1) {
		ret = do_mbox_ioctl(MBOX_IOCTL_RECV, msg);
		if (ret == 0)
			break;
	}

	if (type == CPTRA_IPC_RX_TYPE_INTERNAL) {
		memcpy(output, msg, output_size);
		pr_debug("Message received: 0x%x 0x%x\n", msg[0], msg[1]);

	} else {
		ret = do_misc_op(MISC_OP_READ, output, output_size);
		if (ret < 0) {
			pr_warn("error: misc_op (MISC_OP_READ)");
			return 1;
		}

		ret = msg[0];
	}

	return ret;
}
