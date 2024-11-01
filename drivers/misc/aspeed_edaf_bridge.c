// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include <asm/io.h>
#include <common.h>
#include <clk.h>
#include <dm.h>
#include <errno.h>
#include <fdtdec.h>
#include <linux/bitfield.h>
#include <reset.h>

#define SCU1_HOST_CONF_1		0xa00
#define SCU1_HOST_CONF_2		0xa20
#define   SCU1_HOST_CONF_EDAF_EN	BIT(10)
#define   SCU1_HOST_CONF_LPC_EN		BIT(9)

#define ESPI_CHN3_CTL			0x400
#define   ESPI_CHN3_SW_READY		BIT(5)

#define EDAF_BDGE_CBASE			0x20
#define EDAF_BDGE_MBASE			0x24
#define EDAF_BDGE_CMD_READ		0x40
#define EDAF_BDGE_CMD_WRITE_EN		0x44
#define EDAF_BDGE_CMD_WRITE		0x48
#define EDAF_BDGE_CMD_READ_STS		0x4c
#define EDAF_BDGE_CMD_ERASE_4K		0x50
#define EDAF_BDGE_CMD_ERASE_32K		0x54
#define EDAF_BDGE_CMD_ERASE_64K		0x58
#define EDAF_BDGE_MISC			0x70
#define   EDAF_BDGE_MISC_MBASE_H	GENMASK(15, 8)
#define   EDAF_BDGE_MISC_CBASE_H	GENMASK(7, 0)

static int aspeed_edaf_bridge_probe(struct udevice *dev)
{
	void *edaf_bridge_regs;
	uint64_t cbase, mbase, espibase;
	uint32_t cfg;
	uint32_t phandle;
	ofnode node, scu1_node;
	void *scu_regs, *espi_regs;
	int rc;

	edaf_bridge_regs = (void *)devfdt_get_addr_index(dev, 0);
	if (edaf_bridge_regs == (void *)FDT_ADDR_T_NONE) {
		printf("cannot get eDAF MCU base\n");
		return -ENODEV;
	};

	node = dev_ofnode(dev);
	if (!ofnode_valid(node)) {
		printf("cannot get eDAF device node\n");
		return -ENODEV;
	}

	rc = ofnode_read_u32(node, "aspeed,scu1", &phandle);
	if (rc) {
		printf("cannot get SCU1 phandle\n");
		return -ENODEV;
	}

	scu1_node = ofnode_get_by_phandle(phandle);
	if (!ofnode_valid(scu1_node)) {
		printf("cannot get SCU1 device node\n");
		return -ENODEV;
	}

	scu_regs = (void *)ofnode_get_addr(scu1_node);
	if (scu_regs == (void *)FDT_ADDR_T_NONE) {
		printf("cannot map SCU1 registers\n");
		return -ENODEV;
	}

	cfg = readl(scu_regs + SCU1_HOST_CONF_1);
	if (cfg & SCU1_HOST_CONF_LPC_EN) {
		printf("Host Configuration1: eSPI0 eDAF is not valid\n");
		return -EINVAL;
	}
	cfg |= SCU1_HOST_CONF_EDAF_EN;
	writel(cfg, scu_regs + SCU1_HOST_CONF_1);

	cfg = readl(scu_regs + SCU1_HOST_CONF_2);
	if (cfg & SCU1_HOST_CONF_LPC_EN) {
		printf("Host Configuration2: eSPI1 eDAF is not valid\n");
		return -EINVAL;
	}
	cfg |= SCU1_HOST_CONF_EDAF_EN;
	writel(cfg, scu_regs + SCU1_HOST_CONF_2);

	rc = ofnode_read_u64(node, "ctl-base", &cbase);
	if (!rc) {
		writel(cbase, edaf_bridge_regs + EDAF_BDGE_CBASE);

		cfg = readl(edaf_bridge_regs + EDAF_BDGE_MISC);
		cfg &= ~EDAF_BDGE_MISC_CBASE_H;
		cfg |= FIELD_PREP(EDAF_BDGE_MISC_CBASE_H, cbase >> 32);
		writel(cfg, edaf_bridge_regs + EDAF_BDGE_MISC);
	}

	rc = ofnode_read_u64(node, "mem-base", &mbase);
	if (!rc) {
		writel(mbase, edaf_bridge_regs + EDAF_BDGE_MBASE);

		cfg = readl(edaf_bridge_regs + EDAF_BDGE_MISC);
		cfg &= ~EDAF_BDGE_MISC_MBASE_H;
		cfg |= FIELD_PREP(EDAF_BDGE_MISC_MBASE_H, mbase >> 32);
		writel(cfg, edaf_bridge_regs + EDAF_BDGE_MISC);
	}

	rc = ofnode_read_u32(node, "cmd-read", &cfg);
	if (!rc)
		writel(cfg, edaf_bridge_regs + EDAF_BDGE_CMD_READ);

	rc = ofnode_read_u32(node, "cmd-write-enable", &cfg);
	if (!rc)
		writel(cfg, edaf_bridge_regs + EDAF_BDGE_CMD_WRITE_EN);

	rc = ofnode_read_u32(node, "cmd-write", &cfg);
	if (!rc)
		writel(cfg, edaf_bridge_regs + EDAF_BDGE_CMD_WRITE);

	rc = ofnode_read_u32(node, "cmd-read-status", &cfg);
	if (!rc)
		writel(cfg, edaf_bridge_regs + EDAF_BDGE_CMD_READ_STS);

	rc = ofnode_read_u32(node, "cmd-erase-4k", &cfg);
	if (!rc)
		writel(cfg, edaf_bridge_regs + EDAF_BDGE_CMD_ERASE_4K);

	rc = ofnode_read_u32(node, "cmd-erase-32k", &cfg);
	if (!rc)
		writel(cfg, edaf_bridge_regs + EDAF_BDGE_CMD_ERASE_32K);

	rc = ofnode_read_u32(node, "cmd-erase-64k", &cfg);
	if (!rc)
		writel(cfg, edaf_bridge_regs + EDAF_BDGE_CMD_ERASE_64K);

	rc = ofnode_read_u64(node, "espi-base", &espibase);
	if (!rc) {
		espi_regs = (void *)(uintptr_t)espibase;
		cfg = readl(espi_regs + ESPI_CHN3_CTL);
		cfg |= ESPI_CHN3_SW_READY;
		writel(cfg, espi_regs + ESPI_CHN3_CTL);
	}

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
