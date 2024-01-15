// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */
#include "platform.h"
#include "internal.h"

struct aspeed_mac_priv_s mac_priv_data[NUM_OF_MAC_DEVICES] = {
	{ .max_rx_packet_size = 9600 },
	{ .max_rx_packet_size = 2048 },
	{ .max_rx_packet_size = 2048 },
};

DECLARE_DEV_CLK(mdio0, 0, 0, 0);
DECLARE_DEV_RESET(mdio0, AST_IO_SCU_BASE + 0x200, AST_IO_SCU_BASE + 0x204, BIT(2));
DECLARE_DEV(mdio0, ASPEED_DEV_MDIO0, AST_MDIO0_BASE, NULL);

DECLARE_DEV_CLK(mdio1, 0, 0, 0);
DECLARE_DEV_RESET(mdio1, AST_IO_SCU_BASE + 0x200, AST_IO_SCU_BASE + 0x204, BIT(2));
DECLARE_DEV(mdio1, ASPEED_DEV_MDIO1, AST_MDIO1_BASE, NULL);

DECLARE_DEV_CLK(mdio2, 0, 0, 0);
DECLARE_DEV_RESET(mdio2, AST_IO_SCU_BASE + 0x200, AST_IO_SCU_BASE + 0x204, BIT(2));
DECLARE_DEV(mdio2, ASPEED_DEV_MDIO2, AST_MDIO2_BASE, NULL);

DECLARE_DEV_CLK(mac0, AST_IO_SCU_BASE + 0x244, AST_IO_SCU_BASE + 0x240, BIT(8));
DECLARE_DEV_RESET(mac0, AST_IO_SCU_BASE + 0x200, AST_IO_SCU_BASE + 0x204, BIT(5));
DECLARE_DEV(mac0, ASPEED_DEV_MAC0, (void *)AST_MAC0_BASE, &mac_priv_data[0]);

DECLARE_DEV_CLK(mac1, AST_IO_SCU_BASE + 0x244, AST_IO_SCU_BASE + 0x240, BIT(9));
DECLARE_DEV_RESET(mac1, AST_IO_SCU_BASE + 0x200, AST_IO_SCU_BASE + 0x204, BIT(6));
DECLARE_DEV(mac1, ASPEED_DEV_MAC1, (void *)AST_MAC1_BASE, &mac_priv_data[1]);

DECLARE_DEV_CLK(mac2, AST_IO_SCU_BASE + 0x244, AST_IO_SCU_BASE + 0x240, BIT(10));
DECLARE_DEV_RESET(mac2, AST_IO_SCU_BASE + 0x200, AST_IO_SCU_BASE + 0x204, BIT(7));
DECLARE_DEV(mac2, ASPEED_DEV_MAC2, (void *)AST_MAC2_BASE, &mac_priv_data[2]);

struct mdio_s mdio_data[NUM_OF_MDIO_DEVICES] = {
	{ .device = &mdio0, .phy_addr = 0 },
	{ .device = &mdio1, .phy_addr = 0 },
	{ .device = &mdio2, .phy_addr = 0 },
};

struct phy_s phy_data[NUM_OF_MDIO_DEVICES] = {
	{ .link = 0, .mdio = &mdio_data[0], .loopback = 0 },
	{ .link = 0, .mdio = &mdio_data[1], .loopback = 0 },
	{ .link = 0, .mdio = &mdio_data[2], .loopback = 0 }
};

struct mac_s mac_data[NUM_OF_MAC_DEVICES] = {
	{ .device = &mac0, .phy = &phy_data[0] },
	{ .device = &mac1, .phy = &phy_data[1] },
	{ .device = &mac2, .phy = &phy_data[2] }
};

static struct aspeed_sig_desc_s rgmii1[] = {
	{ 0x444, GENMASK(31, 0), 1 },
	{ 0x444, BIT(0) | BIT(4) | BIT(8) | BIT(12) | BIT(16) | BIT(20) | BIT(24) | BIT(28), 0 },
	{ 0x448, GENMASK(14, 0), 1 },
	{ 0x448, BIT(0) | BIT(4) | BIT(8) | BIT(12), 0 },
	/* IO Driving */
	{ 0x4D4, GENMASK(23, 0), 1 },
	{ 0x4D4,
	  BIT(0) | BIT(2) | BIT(4) | BIT(6) | BIT(8) | BIT(10) | BIT(12) | BIT(14) | BIT(16) |
		  BIT(18) | BIT(20) | BIT(22),
	  0 },
};

static struct aspeed_sig_desc_s rgmii2[] = {
	{ 0x44c, GENMASK(31, 0), 1 },
	{ 0x44c, BIT(0) | BIT(4) | BIT(8) | BIT(12) | BIT(16) | BIT(20) | BIT(24) | BIT(28), 0 },
	{ 0x450, GENMASK(14, 0), 1 },
	{ 0x450, BIT(0) | BIT(4) | BIT(8) | BIT(12), 0 },
	/* IO Driving */
	{ 0x4D8, GENMASK(23, 0), 1 },
	{ 0x4D8,
	  BIT(0) | BIT(2) | BIT(4) | BIT(6) | BIT(8) | BIT(10) | BIT(12) | BIT(14) | BIT(16) |
		  BIT(18) | BIT(20) | BIT(22),
	  0 },
};

static struct aspeed_sig_desc_s rmii1[] = {
	{ 0x444, GENMASK(31, 0), 1 },
	{ 0x444, BIT(1) | BIT(9) | BIT(13) | BIT(17) | BIT(21) | BIT(25) | BIT(29), 0 },
	{ 0x448, GENMASK(6, 0), 1 },
	{ 0x448, BIT(1) | BIT(5), 0 },
	/* IO Driving */
	{ 0x4D4, GENMASK(23, 0), 1 },
	{ 0x4D4,
	  BIT(0) | BIT(2) | BIT(4) | BIT(6) | BIT(8) | BIT(10) | BIT(12) | BIT(14) | BIT(16) |
		  BIT(18) | BIT(20) | BIT(22),
	  0 },
};

static struct aspeed_sig_desc_s rmii2[] = {
	{ 0x44c, GENMASK(31, 0), 1 },
	{ 0x44c, BIT(1) | BIT(9) | BIT(13) | BIT(17) | BIT(21) | BIT(25) | BIT(29), 0 },
	{ 0x450, GENMASK(6, 0), 1 },
	{ 0x450, BIT(1) | BIT(5), 0 },
	/* IO Driving */
	{ 0x4D8, GENMASK(23, 0), 1 },
	{ 0x4D8,
	  BIT(0) | BIT(2) | BIT(4) | BIT(6) | BIT(8) | BIT(10) | BIT(12) | BIT(14) | BIT(16) |
		  BIT(18) | BIT(20) | BIT(22),
	  0 },
};

struct aspeed_group_config_s rgmii_pinctrl[NUM_OF_MAC_DEVICES] = {
	{ "RGMII1", ARRAY_SIZE(rgmii1), rgmii1 },
	{ "RGMII2", ARRAY_SIZE(rgmii2), rgmii2 },
	{0}
};

struct aspeed_group_config_s rmii_pinctrl[NUM_OF_MAC_DEVICES] = {
	{ "RMII1", ARRAY_SIZE(rmii1), rmii1 },
	{ "RMII2", ARRAY_SIZE(rmii2), rmii2 },
	{0}
};

static struct aspeed_sig_desc_s mdio1_link[] = {
	{ 0x448, GENMASK(22, 16), 1 },
	{ 0x448, BIT(16) | BIT(20), 0 },
};

static struct aspeed_sig_desc_s mdio2_link[] = {
	{ 0x450, GENMASK(22, 16), 1 },
	{ 0x450, BIT(16) | BIT(20), 0 },
};

static struct aspeed_sig_desc_s mdio3_link[] = {
	{ 0x440, GENMASK(6, 0), 1 },
	{ 0x440, BIT(0) | BIT(4), 0 },
};

struct aspeed_group_config_s mdio_pinctrl[NUM_OF_MAC_DEVICES] = {
	{ "MDIO1", ARRAY_SIZE(mdio1_link), mdio1_link },
	{ "MDIO2", ARRAY_SIZE(mdio2_link), mdio2_link },
	{ "MDIO3", ARRAY_SIZE(mdio3_link), mdio3_link },
};
