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
#include <linux/ioport.h>
#include <reset.h>

#define SCU1_HOST_CONF_1		0xa00
#define SCU1_HOST_CONF_2		0xa20
#define   SCU1_HOST_CONF_EDAF_EN	BIT(10)
#define   SCU1_HOST_CONF_LPC_EN		BIT(9)

#define ESPI_CHN3_CTL			0x400
#define   ESPI_CHN3_SW_READY		BIT(5)

#define EDAF_BDGE_CFG			0x0
#define   EDAF_BDGE_CFG_CMD_EN		BIT(0)
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

uint64_t ast27xx_soc_virt_addr_to_phy_addr(uintptr_t addr)
{
	if (addr < 0x80000000)
		return addr;

	return (uint64_t)(addr & ~0x80000000) | 0x400000000ULL;
}

uintptr_t ast27xx_soc_phy_addr_to_virt_addr(uint64_t addr)
{
	if (addr < 0x400000000)
		return addr;

	return (uintptr_t)(addr - 0x400000000ULL) | 0x80000000UL;
}

static int aspeed_edaf_bridge_probe(struct udevice *dev)
{
	void *edaf_bridge_regs;
	void *scu_regs, *espi_regs, *virt;
	ofnode node, scu1_node, mem_node;
	uint64_t cbase, mbase, espibase;
	uint32_t scu, edaf, espi, phandle;
	struct resource res;
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

	scu = readl(scu_regs + SCU1_HOST_CONF_1);
	if (scu & SCU1_HOST_CONF_LPC_EN) {
		printf("Host Configuration1: eSPI0 eDAF is not valid\n");
		return -EINVAL;
	}
	scu |= SCU1_HOST_CONF_EDAF_EN;
	writel(scu, scu_regs + SCU1_HOST_CONF_1);

	scu = readl(scu_regs + SCU1_HOST_CONF_2);
	if (scu & SCU1_HOST_CONF_LPC_EN) {
		printf("Host Configuration2: eSPI1 eDAF is not valid\n");
		return -EINVAL;
	}
	scu |= SCU1_HOST_CONF_EDAF_EN;
	writel(scu, scu_regs + SCU1_HOST_CONF_2);

	edaf = readl(edaf_bridge_regs + EDAF_BDGE_CFG);
	if (ofnode_read_bool(node, "edaf-ddr-mode")) {
		rc = ofnode_read_u32(node, "mem-base", &phandle);
		if (rc) {
			printf("cannot get mem-base phandle\n");
			return -ENODEV;
		}

		mem_node = ofnode_get_by_phandle(phandle);
		if (!ofnode_valid(mem_node)) {
			printf("cannot get mem-base device node\n");
			return -ENODEV;
		}

		rc = ofnode_read_resource(mem_node, 0, &res);
		if (rc) {
			printf("cannot get eDAF resource.\n");
			return -ENODEV;
		}

		mbase = ast27xx_soc_virt_addr_to_phy_addr(res.start);
		virt = (void *)(uintptr_t)mbase;
		if (!virt) {
			printf("cannot map eDAF physical address\n");
			return -ENOMEM;
		}

		edaf &= ~EDAF_BDGE_CFG_CMD_EN;
	} else {
		rc = ofnode_read_u64(node, "mem-base", &mbase);
		if (rc)
			printf("cannot get eDAF memory region\n");

		edaf |= EDAF_BDGE_CFG_CMD_EN;
	}
	writel(mbase, edaf_bridge_regs + EDAF_BDGE_MBASE);
	writel(edaf, edaf_bridge_regs + EDAF_BDGE_CFG);

	edaf = readl(edaf_bridge_regs + EDAF_BDGE_MISC);
	edaf &= ~EDAF_BDGE_MISC_MBASE_H;
	edaf |= FIELD_PREP(EDAF_BDGE_MISC_MBASE_H, mbase >> 32);
	writel(edaf, edaf_bridge_regs + EDAF_BDGE_MISC);

	rc = ofnode_read_u64(node, "ctl-base", &cbase);
	if (!rc) {
		writel(cbase, edaf_bridge_regs + EDAF_BDGE_CBASE);

		edaf = readl(edaf_bridge_regs + EDAF_BDGE_MISC);
		edaf &= ~EDAF_BDGE_MISC_CBASE_H;
		edaf |= FIELD_PREP(EDAF_BDGE_MISC_CBASE_H, cbase >> 32);
		writel(edaf, edaf_bridge_regs + EDAF_BDGE_MISC);
	}

	rc = ofnode_read_u32(node, "cmd-read", &edaf);
	if (!rc)
		writel(edaf, edaf_bridge_regs + EDAF_BDGE_CMD_READ);

	rc = ofnode_read_u32(node, "cmd-write-enable", &edaf);
	if (!rc)
		writel(edaf, edaf_bridge_regs + EDAF_BDGE_CMD_WRITE_EN);

	rc = ofnode_read_u32(node, "cmd-write", &edaf);
	if (!rc)
		writel(edaf, edaf_bridge_regs + EDAF_BDGE_CMD_WRITE);

	rc = ofnode_read_u32(node, "cmd-read-status", &edaf);
	if (!rc)
		writel(edaf, edaf_bridge_regs + EDAF_BDGE_CMD_READ_STS);

	rc = ofnode_read_u32(node, "cmd-erase-4k", &edaf);
	if (!rc)
		writel(edaf, edaf_bridge_regs + EDAF_BDGE_CMD_ERASE_4K);

	rc = ofnode_read_u32(node, "cmd-erase-32k", &edaf);
	if (!rc)
		writel(edaf, edaf_bridge_regs + EDAF_BDGE_CMD_ERASE_32K);

	rc = ofnode_read_u32(node, "cmd-erase-64k", &edaf);
	if (!rc)
		writel(edaf, edaf_bridge_regs + EDAF_BDGE_CMD_ERASE_64K);

	rc = ofnode_read_u64(node, "espi-base", &espibase);
	if (!rc) {
		espi_regs = (void *)(uintptr_t)espibase;
		espi = readl(espi_regs + ESPI_CHN3_CTL);
		espi |= ESPI_CHN3_SW_READY;
		writel(espi, espi_regs + ESPI_CHN3_CTL);
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
