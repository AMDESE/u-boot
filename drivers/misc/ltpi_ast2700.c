// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */
#include <asm/io.h>
#include <asm/arch/platform.h>
#include <asm/arch/scu_ast2700.h>
#include <common.h>
#include <dm.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/iopoll.h>

#define LTPI_LINK_MANAGE_ST		0x108
#define   LTPI_LINK_PARTNER_AST1700	BIT(24)
#define   LTPI_LINK_MNG_ST		GENMASK(3, 0)
#define     LTPI_LINK_MNG_ST_OP		7

#define LTPI_ADDR_REMAP_REG0		0x124
#define   REMAP_ENTRY1			GENMASK(25, 16)
#define   REMAP_ENTRY0			GENMASK(9, 0)
#define LTPI_ADDR_REMAP_REG1		0x128
#define   REMAP_ENTRY3			GENMASK(25, 16)
#define   REMAP_ENTRY2			GENMASK(9, 0)

#define LTPI_REMOTE_AST1700_IOD_SPACE	(0x14000000 >> 26)
#define LTPI_REMOTE_AST1700_SPI2_SPACE	(0x280000000U >> 26)

static int get_phandle_dev_regs(ofnode node, const char *propname, uint32_t *regs)
{
	ofnode prop_node;
	uint32_t phandle, value;
	int rc;

	rc = ofnode_read_u32(node, propname, &phandle);
	if (rc) {
		debug("cannot get %s phandle\n", propname);
		return -ENODEV;
	}

	prop_node = ofnode_get_by_phandle(phandle);
	if (!ofnode_valid(prop_node)) {
		debug("cannot get %s device node\n", propname);
		return -ENODEV;
	}

	value = (uint32_t)ofnode_get_addr(prop_node);
	if (value == (uint32_t)FDT_ADDR_T_NONE) {
		debug("cannot map %s registers\n", propname);
		return -ENODEV;
	}

	*regs = value;

	return 0;
}

int ast2700_ltpi_probe(struct udevice *dev)
{
	struct ast2700_scu1 *scu;
	ofnode node;
	uint32_t ltpi_regs, scu1_regs, reg_val, remap;
	int ret, index, hwstrap_en, ltpi_sio_en;

	node = dev_ofnode(dev);

	ltpi_regs = (uint32_t)ofnode_get_addr(node);
	if (ltpi_regs == (uint32_t)FDT_ADDR_T_NONE) {
		debug("cannot get LTPI base\n");
		return -ENODEV;
	};

	ret = ofnode_read_u32(node, "index", &index);
	if (ret) {
		debug("cannot get index\n");
		return -ENOENT;
	}

	if (index) {
		hwstrap_en = SCU_IO_HWSTRAP_LTPI1_EN;
		ltpi_sio_en = ASPEED_IO_MISC_SIO_LTPI1_EN;
	} else {
		hwstrap_en = SCU_IO_HWSTRAP_LTPI0_EN;
		ltpi_sio_en = ASPEED_IO_MISC_SIO_LTPI_EN;
	}

	ret = get_phandle_dev_regs(node, "aspeed,scu1", &scu1_regs);
	if (ret < 0)
		return ret;

	scu = (struct ast2700_scu1 *)scu1_regs;
	reg_val = readl((void *)&scu->hwstrap1);
	if ((reg_val & hwstrap_en) == 0) {
		debug("LTPI%d not enabled\n", index);
		return 0;
	}

	reg_val = readl((void *)ltpi_regs + LTPI_LINK_MANAGE_ST);
	if (FIELD_GET(LTPI_LINK_MNG_ST, reg_val) != LTPI_LINK_MNG_ST_OP) {
		debug("LTPI%d not linked\n", index);
		return 0;
	}

	/* OEM Data frame: AHB address remap */
	if (reg_val & LTPI_LINK_PARTNER_AST1700)
		remap = FIELD_PREP(REMAP_ENTRY0, LTPI_REMOTE_AST1700_IOD_SPACE) |
			FIELD_PREP(REMAP_ENTRY1, LTPI_REMOTE_AST1700_SPI2_SPACE);
	else
		remap = 0;

	writel(remap, (void *)ltpi_regs + LTPI_ADDR_REMAP_REG0);

	/* Route SIO to LTPI IO frame */
	reg_val = readl((void *)&scu->misc);
	reg_val |= ltpi_sio_en;
	writel(reg_val, (void *)&scu->misc);

	return 0;
}

static const struct udevice_id aspeed_ltpi_match[] = {
	{ .compatible = "aspeed,ast2700-ltpi" },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(aspeed_ltpi_driver) = {
	.name = "ast2700-ltpi",
	.id = UCLASS_MISC,
	.of_match = aspeed_ltpi_match,
	.probe = ast2700_ltpi_probe,
};
