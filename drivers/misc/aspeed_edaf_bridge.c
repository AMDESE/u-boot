// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <errno.h>
#include <reset.h>
#include <fdtdec.h>
#include <asm/io.h>

#define EDAF_BDGE_CMD_READ		0x40
#define EDAF_BDGE_CMD_WRITE		0x48
#define EDAF_BDGE_CMD_ERASE_4K	0x50
#define EDAF_BDGE_CMD_ERASE_32K	0x54
#define EDAF_BDGE_CMD_ERASE_64K	0x58

static int aspeed_edaf_bridge_probe(struct udevice *dev)
{
	void *edaf_bridge_base;
	uint32_t cfg;
	ofnode node;
	int rc;

	edaf_bridge_base = (void *)devfdt_get_addr_index(dev, 0);
	if (edaf_bridge_base == (void *)FDT_ADDR_T_NONE) {
		printf("cannot get eDAF MCU base\n");
		return -ENODEV;
	};

	node = dev_ofnode(dev);
	if (!ofnode_valid(node)) {
		printf("cannot get eDAF device node\n");
		return -ENODEV;
	}

	rc = ofnode_read_u32(node, "cmd-read", &cfg);
	if (!rc)
		writel(cfg, edaf_bridge_base + EDAF_BDGE_CMD_READ);

	rc = ofnode_read_u32(node, "cmd-write", &cfg);
	if (!rc)
		writel(cfg, edaf_bridge_base + EDAF_BDGE_CMD_WRITE);

	rc = ofnode_read_u32(node, "cmd-erase-4k", &cfg);
	if (!rc)
		writel(cfg, edaf_bridge_base + EDAF_BDGE_CMD_ERASE_4K);

	rc = ofnode_read_u32(node, "cmd-erase-32k", &cfg);
	if (!rc)
		writel(cfg, edaf_bridge_base + EDAF_BDGE_CMD_ERASE_32K);

	rc = ofnode_read_u32(node, "cmd-erase-64k", &cfg);
	if (!rc)
		writel(cfg, edaf_bridge_base + EDAF_BDGE_CMD_ERASE_64K);

	return 0;
}

static const struct udevice_id aspeed_edaf_bridge_ids[] = {
	{ .compatible = "aspeed,ast2700-edaf-bridge" },
	{ }
};

U_BOOT_DRIVER(aspeed_edaf_bridge) = {
	.name = "aspeed_edaf_bridge",
	.id = UCLASS_MISC,
	.of_match = aspeed_edaf_bridge_ids,
	.probe = aspeed_edaf_bridge_probe,
	.flags = DM_FLAG_PRE_RELOC,
};
