// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include <common.h>
#include <command.h>
#include <dm.h>
#include <errno.h>
#include <misc.h>
#include <malloc.h>
#include <memalign.h>

#define I2CGLOBAL		0x14c0f000
#define LTPISHEADER		0xf0000000
#define LTPIMISCREAD	0x0
#define LTPIMISCWRITE	0x1

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

struct ltpi_ipc_data {
	u32 misc;
	u32 port;
	u32 value;
};

static int do_mbox_stress(struct cmd_tbl *cmdtp, int flag,
			  int argc, char *const argv[])
{
	struct udevice *dev;
	int ret;
	u32 msg[8];
	u32 *buf;
	int size, counter = 0;
	u32 test_pattern = 0;
	bool correct = true;

	ret = uclass_get_device_by_name(UCLASS_MISC, argv[0], &dev);
	if (ret) {
		printf("Unable to find device %s\n", argv[0]);
		return ret;
	}

	buf = malloc_cache_aligned(0x1000);
	if (!buf) {
		printf("NO MEM\n");
		return -ENOMEM;
	}

	while (1) {
		while (1) {
			ret = misc_ioctl(dev, MBOX_IOCTL_RECV, msg);
			if (ret == 0)
				break;
		}

		// check message
		if ((msg[0] & 0xff) != 0xF0) {
			if ((msg[0] & 0xff) == 0xF1)
				printf("recv stop message, 0x%x\n", msg[0]);
			else
				printf("message not expected, 0x%x 0x%x\n", msg[0], msg[1]);
			break;
		}
		size = msg[0] >> 8;
		test_pattern = msg[1];
		debug("recv message , msg (0x%x 0x%x)\n",
		      msg[0], msg[1]);

		ret = misc_read(dev, 0, buf, size);
		if (ret != size) {
			printf("buffer read not enough, %d\n", ret);
			break;
		}

		// check buffer
		for (int i = 0; i < size / 4; i++, test_pattern++) {
			if (buf[i] != test_pattern) {
				printf("buffer not expected, %d: 0x%x 0x%x\n",
				       i, buf[i], test_pattern);
				correct = false;
				break;
			}
		}
		if (!correct)
			break;

		printf("stress round-%d pass\n", ++counter);

		// prepare message & buffer
		msg[1] = test_pattern;
		for (int i = 0; i < size / 4; i++, test_pattern++)
			buf[i] = test_pattern;
		ret = misc_write(dev, 0, buf, size);
		if (ret != size) {
			printf("buffer write not enough, %d\n", ret);
			break;
		}

		debug("send message , msg (0x%x 0x%x)\n",
		      msg[0], msg[1]);
		// send
		ret = misc_ioctl(dev, MBOX_IOCTL_SEND, msg);
		if (ret != 0) {
			printf("send message failed, msg (0x%x 0x%x), %d\n",
			       msg[0], msg[1], ret);
			break;
		}
	}

	free(buf);
	return 0;
}

static int do_misc_list(struct cmd_tbl *cmdtp, int flag,
			int argc, char *const argv[])
{
	struct udevice *dev;
	u32 buf[4];

	printf("Device           Driver      TX-SHMEM              RX-SHMEM\n");
	printf("-------------------------------------\n");
	for (uclass_first_device(UCLASS_MISC, &dev);
	     dev;
	     uclass_next_device(&dev)) {
		if (strcmp(dev->driver->name, "aspeed-mbox") != 0)
			continue;
		misc_ioctl(dev, MBOX_IOCTL_CAPS, buf);
		printf("%-16s %10s 0x%09llx, 0x%6x 0x%09llx, 0x%06x\n",
		       dev->name, dev->driver->name,
		       ((u64)buf[0]) << 8, buf[1], ((u64)buf[2]) << 8, buf[3]);
	}

	return 0;
}

static int do_misc_op(struct cmd_tbl *cmdtp, int flag,
		      int argc, char *const argv[], enum misc_op op)
{
	struct udevice *dev;
	int offset;
	void *buf;
	int size;
	int ret;

	ret = uclass_get_device_by_name(UCLASS_MISC, argv[0], &dev);
	if (ret) {
		printf("Unable to find device %s\n", argv[0]);
		return ret;
	}

	offset = hextoul(argv[1], NULL);
	buf = (void *)hextoul(argv[2], NULL);
	size = hextoul(argv[3], NULL);

	if (op == MISC_OP_READ)
		ret = misc_read(dev, offset, buf, size);
	else
		ret = misc_write(dev, offset, buf, size);

	if (ret < 0) {
		if (ret == -ENOSYS) {
			printf("The device does not support %s\n",
			       misc_op_str[op]);
			ret = 0;
		}
	} else {
		if (ret == size)
			ret = 0;
		else
			printf("Partially %s %d bytes\n", misc_op_str[op], ret);
	}

	return ret;
}

static int do_mbox_ioctl(struct cmd_tbl *cmdtp, int flag,
			 int argc, char *const argv[], unsigned long cmd)
{
	struct udevice *dev;
	void *buf;
	int ret;

	ret = uclass_get_device_by_name(UCLASS_MISC, argv[0], &dev);
	if (ret) {
		printf("Unable to find device %s\n", argv[0]);
		return ret;
	}

	buf = (void *)hextoul(argv[1], NULL);

	ret = misc_ioctl(dev, cmd, buf);
	if (ret < 0) {
		if (ret == -ENOSYS) {
			printf("The device does not support %s\n",
			       mbox_ioctl_str[cmd]);
			ret = 0;
		}
	}

	return ret;
}

static int do_mbox_send(struct cmd_tbl *cmdtp, int flag,
			int argc, char *const argv[])
{
	return do_mbox_ioctl(cmdtp, flag, argc, argv, MBOX_IOCTL_SEND);
}

static int do_mbox_recv(struct cmd_tbl *cmdtp, int flag,
			int argc, char *const argv[])
{
	return do_mbox_ioctl(cmdtp, flag, argc, argv, MBOX_IOCTL_RECV);
}

static int do_misc_read(struct cmd_tbl *cmdtp, int flag,
			int argc, char *const argv[])
{
	return do_misc_op(cmdtp, flag, argc, argv, MISC_OP_READ);
}

static int do_misc_write(struct cmd_tbl *cmdtp, int flag,
			 int argc, char *const argv[])
{
	return do_misc_op(cmdtp, flag, argc, argv, MISC_OP_WRITE);
}

static int do_mbox_ltpi_read(struct cmd_tbl *cmdtp, int flag,
			     int argc, char *const argv[])
{
	struct udevice *dev;
	u32 *buf = NULL;
	struct ltpi_ipc_data *data = NULL;
	u32 msg[8];
	int ret = 0, i;
	u32 channel = 0, offset, size, data_size;

	ret = uclass_get_device_by_name(UCLASS_MISC, argv[0], &dev);
	if (ret) {
		printf("Unable to find device %s\n", argv[0]);
		return ret;
	}

	channel = hextoul(argv[1], NULL);
	offset = hextoul(argv[2], NULL);
	size = hextoul(argv[3], NULL);

	data_size = sizeof(struct ltpi_ipc_data) * size;

	/* prepare local buffer */
	buf = malloc_cache_aligned(data_size);
	if (!buf) {
		printf("NO MEM\n");
		return -ENOMEM;
	}

	data = (struct ltpi_ipc_data *)buf;

	/* fill read data */
	for (i = 0; i < size ; i++) {
		/* read the port data and save into value */
		data->misc = LTPIMISCREAD;
		data->port = offset + (i << 2);
		data->value = 0;

		data++;
	}

	/* copy the communication data into tx share buffer */
	ret = misc_write(dev, 0, buf, data_size);
	if (ret != data_size) {
		printf("buffer write not enough, %d\n", ret);
		return ret;
	}

	/* fill the header with struture count in the low byte */
	msg[0] = LTPISHEADER | size;

	/* send messgae with mailbox */
	ret = misc_ioctl(dev, MBOX_IOCTL_SEND, msg);
	if (ret != 0) {
		printf("send message failed, msg (0x%x 0x%x), %d\n",
		       msg[0], msg[1], ret);
		return ret;
	}

	/* copy the communication data from rx share buffer back */
	ret = misc_read(dev, 0, buf, data_size);
	if (ret != data_size) {
		printf("buffer read not enough, %d\n", ret);
		return ret;
	}

	data = (struct ltpi_ipc_data *)buf;

	/* print out the result from received buffer */
	for (i = 0; i < size ; i++) {
		printf("R<-[0x%08x]: [0x%08x]\n", data->port, data->value);
		data++;
	}

	free(buf);
	return 0;
}

static int do_mbox_ltpi_write(struct cmd_tbl *cmdtp, int flag,
			      int argc, char *const argv[])
{
	struct udevice *dev;
	u32 *buf = NULL;
	struct ltpi_ipc_data *data = NULL;
	u32 msg[8];
	int ret = 0, i;
	u32 channel = 0, offset, value, data_size;

	ret = uclass_get_device_by_name(UCLASS_MISC, argv[0], &dev);
	if (ret) {
		printf("Unable to find device %s\n", argv[0]);
		return ret;
	}

	channel = hextoul(argv[1], NULL);
	offset = hextoul(argv[2], NULL);
	value = hextoul(argv[3], NULL);

	data_size = sizeof(struct ltpi_ipc_data);

	/* prepare local buffer */
	buf = malloc_cache_aligned(data_size);
	if (!buf) {
		printf("NO MEM\n");
		return -ENOMEM;
	}

	data = (struct ltpi_ipc_data *)buf;

	/* fill read data */
	for (i = 0; i < 1 ; i++) {
		/* read the port data and save into value */
		data->misc = LTPIMISCWRITE;
		data->port = offset + (i << 2);
		data->value = value;

		data++;
	}

	/* copy the communication data into tx share buffer */
	ret = misc_write(dev, 0, buf, data_size);
	if (ret != data_size) {
		printf("buffer write not enough, %d\n", ret);
		return ret;
	}

	/* fill the header with struture count in the low byte */
	msg[0] = LTPISHEADER | 1;

	/* send messgae with mailbox */
	ret = misc_ioctl(dev, MBOX_IOCTL_SEND, msg);
	if (ret != 0) {
		printf("send message failed, msg (0x%x 0x%x), %d\n",
		       msg[0], msg[1], ret);
		return ret;
	}

	/* copy the communication data from rx share buffer back */
	ret = misc_read(dev, 0, buf, data_size);
	if (ret != data_size) {
		printf("buffer read not enough, %d\n", ret);
		return ret;
	}

	data = (struct ltpi_ipc_data *)buf;

	/* print out the result from received buffer */
	printf("W->[0x%08x]: [0x%08x]\n", data->port, data->value);

	free(buf);
	return 0;
}

static struct cmd_tbl mbox_commands[] = {
	U_BOOT_CMD_MKENT(list, 0, 1, do_misc_list, "", ""),
	U_BOOT_CMD_MKENT(stress, 1, 1, do_mbox_stress, "", ""),
	U_BOOT_CMD_MKENT(send, 2, 1, do_mbox_send, "", ""),
	U_BOOT_CMD_MKENT(recv, 2, 1, do_mbox_recv, "", ""),
	U_BOOT_CMD_MKENT(read, 4, 1, do_misc_read, "", ""),
	U_BOOT_CMD_MKENT(write, 4, 1, do_misc_write, "", ""),
	U_BOOT_CMD_MKENT(ltpi_r, 4, 1, do_mbox_ltpi_read, "", ""),
	U_BOOT_CMD_MKENT(ltpi_w, 4, 1, do_mbox_ltpi_write, "", ""),
};

static int do_mbox(struct cmd_tbl *cmdtp, int flag,
		   int argc, char *const argv[])
{
	struct cmd_tbl *mbox_cmd;
	int ret;

	if (argc < 2)
		return CMD_RET_USAGE;
	mbox_cmd = find_cmd_tbl(argv[1], mbox_commands,
				ARRAY_SIZE(mbox_commands));
	argc -= 2;
	argv += 2;
	if (!mbox_cmd || argc != mbox_cmd->maxargs)
		return CMD_RET_USAGE;

	ret = mbox_cmd->cmd(mbox_cmd, flag, argc, argv);

	return cmd_process_error(mbox_cmd, ret);
}

U_BOOT_CMD(
	aspeed_mbox,	7,	0,	do_mbox,
	"Access ASPEED maiilbox devices. All number parameters in hex format.",
	"list                       - list all devices related aspeed mbox\n"
	"aspeed_mbox stress name                - stress test. recv & send\n"
	"aspeed_mbox send name addr             - send message by device\n"
	"					 `name' from memory at `addr'\n"
	"aspeed_mbox recv name addr             - recv message by device\n"
	"					 `name' to memory at `addr'\n"
	"aspeed_mbox read  name offset addr len - read `len' bytes starting at\n"
	"					 `offset' of device `name'\n"
	"					 to memory at `addr'\n"
	"aspeed_mbox write name offset addr len - write `len' bytes starting at\n"
	"					 `offset' of device `name'\n"
	"					 from memory at `addr'\n"
	"aspeed_mbox ltpi_r name ltpi_channel offset len - read from ltpi channel\n"
	"aspeed_mbox ltpi_w name ltpi_channel offset value - write into ltpi channel\n"
);
