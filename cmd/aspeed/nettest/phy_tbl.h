/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) ASPEED Technology Inc.
 */
void phy_rtl8211f_config(struct phy_s *obj);
void phy_bcm54616_config(struct phy_s *obj);
void phy_bcm54616_clear(struct phy_s *obj);
void phy_bcm5221_config(struct phy_s *obj);
void phy_an8801_config(struct phy_s *obj);

struct phy_desc {
	u16 id1;
	u16 id2;
	u16 id2_mask;
	u8 name[64];
	void (*config)(struct phy_s *obj);
	void (*clear)(struct phy_s *obj);
};

static struct phy_desc phy_lookup_tbl[] = {
	{ .id1 = 0x001c,
	  .id2 = 0xc916,
	  .id2_mask = 0xffff,
	  .name = "RTL8211F",
	  .config = phy_rtl8211f_config,
	  .clear = NULL },
	{ .id1 = 0x001c,
	  .id2 = 0xc859,
	  .id2_mask = 0xffff,
	  .name = "RTL8211FD-VX",
	  .config = phy_rtl8211f_config,
	  .clear = NULL },
	{ .id1 = 0x001c,
	  .id2 = 0xc870,
	  .id2_mask = 0xfff0,
	  .name = "RTL8211FD-VD",
	  .config = phy_rtl8211f_config,
	  .clear = NULL },
	{ .id1 = 0x0362,
	  .id2 = 0x5d10,
	  .id2_mask = 0xfff0,
	  .name = "BCM54616S",
	  .config = phy_bcm54616_config,
	  .clear = phy_bcm54616_clear },
	{ .id1 = 0x600d,
	  .id2 = 0x84a2,
	  .id2_mask = 0xfff0,
	  .name = "BCM5421X",
	  .config = phy_bcm54616_config,
	  .clear = phy_bcm54616_clear },
	{ .id1 = 0x0040,
	  .id2 = 0x61e0,
	  .id2_mask = 0xfc00,
	  .name = "BCM5221",
	  .config = phy_bcm5221_config,
	  .clear = NULL },
	{ .id1 = 0xC0ff,
	  .id2 = 0x0421,
	  .id2_mask = 0xFFF0,
	  .name = "Airoha AN8801R",
	  .config = phy_an8801_config,
	  .clear = NULL },
};
