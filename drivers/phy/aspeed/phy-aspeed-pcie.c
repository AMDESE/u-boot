// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025 Aspeed Technology Inc.
 */

#include <asm/io.h>
#include <dm.h>
#include <dt-bindings/phy/phy.h>
#include <generic-phy.h>
#include <linux/bitfield.h>

/* AST2600 PCIe Host Controller Registers */
/* reg30 pehr_global */
#define AST2600_PORT_TYPE_MASK		GENMASK(5, 4)
#define AST2600_PORT_TYPE(x)		FIELD_PREP(AST2600_PORT_TYPE_MASK, x)
/* reg70 pehr_lock */
#define PCIE_UNLOCK			0xa8

/* AST2700 PEHR */
/* reg58 */
#define LOCAL_SCALE_SUP			BIT(0)
/* reg5c */
#define PCIE_RC_DEVICE			BIT(30)
/* reg60 */
#define AST2700_PORT_TYPE_MASK		GENMASK(7, 4)
#define PORT_TYPE_ROOT			0x4
/* reg70 */
#define POSTED_DATA_CREDITS(x)		FIELD_PREP(GENMASK(15, 0), x)
#define POSTED_HEADER_CREDITS(x)	FIELD_PREP(GENMASK(27, 16), x)
/* reg78 */
#define COMPLETION_DATA_CREDITS(x)	FIELD_PREP(GENMASK(15, 0), x)
#define COMPLETION_HEADER_CREDITS(x)	FIELD_PREP(GENMASK(27, 16), x)
/* reg268 pehr_clk_freq */
#define CLK_SEL_INTERNAL_25M		BIT(8)
#define CLK_FREQ_MULTI_MASK		GENMASK(7, 0)
#define CLK_FREQ_MULTI(x)		FIELD_PREP(CLK_FREQ_MULTI_MASK, (x))

struct ast2600_pehr_reg {
	u32 pehr_reserved0[12];		// 0x00 ~ 0x2c
	u32 pehr_global;		// 0x30
	u32 pehr_reserved1[18];		// 0x34 ~ 0x78
	u32 pehr_lock;			// 0x7c
};

struct ast2700_pehr_reg {
	u32 pehr_reserved0[22];		// 0x00 ~ 0x54
	u32 pehr_misc_58;		// 0x58
	u32 pehr_misc_5c;		// 0x5c
	u32 pehr_misc_60;		// 0x60
	u32 pehr_reserved1[3];		// 0x64 ~ 0x6c
	u32 pehr_misc_70;		// 0x70
	u32 pehr_reserved2;		// 0x74
	u32 pehr_misc_78;		// 0x78
	u32 pehr_reserved3[123];	// 0x7c ~ 0x264
	u32 pehr_clk_freq;		// 0x268
};

/*
 * Aspeed PCIe PHY variants
 */
enum aspeed_pcie_phy_model {
	ASTEED_AST2600,
	ASTEED_AST2700,
};

struct aspeed_pcie_phy {
	const struct aspeed_pcie_phy_platform *platform;

	struct ast2600_pehr_reg *pehr_reg_26;
	struct ast2700_pehr_reg *pehr_reg_27;

	u32 type;
};

struct aspeed_pcie_phy_platform {
	int (*init)(struct phy *phy);
};

static int aspeed_ast2600_init(struct phy *phy)
{
	struct aspeed_pcie_phy *pcie_phy = dev_get_priv(phy->dev);
	struct ast2600_pehr_reg *pehr_reg = pcie_phy->pehr_reg_26;

	if (pcie_phy->type != PHY_TYPE_PCIE)
		return -EINVAL;

	writel(PCIE_UNLOCK, &pehr_reg->pehr_lock);
	writel(AST2600_PORT_TYPE(0x3), &pehr_reg->pehr_global);

	return 0;
}

static int aspeed_ast2700_init(struct phy *phy)
{
	struct aspeed_pcie_phy *pcie_phy = dev_get_priv(phy->dev);
	struct ast2700_pehr_reg *pehr_reg = pcie_phy->pehr_reg_27;
	u32 cfg_val;

	if (pcie_phy->type == PHY_TYPE_PCIE) {
		writel(POSTED_DATA_CREDITS(0xc0) | POSTED_HEADER_CREDITS(0xa),
		       &pehr_reg->pehr_misc_70);
		writel(COMPLETION_DATA_CREDITS(0x30) | COMPLETION_HEADER_CREDITS(0x8),
		       &pehr_reg->pehr_misc_78);
		writel(LOCAL_SCALE_SUP, &pehr_reg->pehr_misc_58);
		writel(PCIE_RC_DEVICE, &pehr_reg->pehr_misc_5c);
		cfg_val = readl(&pehr_reg->pehr_misc_60);
		cfg_val &= ~AST2700_PORT_TYPE_MASK;
		cfg_val |= FIELD_PREP(AST2700_PORT_TYPE_MASK, PORT_TYPE_ROOT);
		writel(cfg_val, &pehr_reg->pehr_misc_60);
	} else if (pcie_phy->type == PHY_TYPE_SGMII) {
		/*
		 * The PLDA frequency multiplication is X xor 0x19.
		 * (X xor 0x19) * clock source = data rate.
		 * SGMII data rate is 1.25G, so (0x2b xor 0x19) * 25MHz is equal 1.25G.
		 */
		cfg_val = CLK_SEL_INTERNAL_25M | CLK_FREQ_MULTI(0x2b);
		writel(cfg_val, &pehr_reg->pehr_clk_freq);
	} else {
		return -EINVAL;
	}

	return 0;
}

static struct aspeed_pcie_phy_platform pcie_phy_ast2600 = {
	.init = aspeed_ast2600_init,
};

static struct aspeed_pcie_phy_platform pcie_phy_ast2700 = {
	.init = aspeed_ast2700_init,
};

static int aspeed_pcie_phy_init(struct phy *phy)
{
	struct aspeed_pcie_phy *pcie_phy = dev_get_priv(phy->dev);

	return pcie_phy->platform->init(phy);
}

static int aspeed_pcie_phy_xlate(struct phy *phy,
				 struct ofnode_phandle_args *args)
{
	struct aspeed_pcie_phy *pcie_phy = dev_get_priv(phy->dev);

	pcie_phy->type = args->args[0];

	return 0;
}

int aspeed_pcie_phy_probe(struct udevice *dev)
{
	return 0;
}

static int aspeed_pcie_phy_of_to_plat(struct udevice *dev)
{
	struct aspeed_pcie_phy *pcie_phy = dev_get_priv(dev);

	/* Get the controller base address */
	switch (dev_get_driver_data(dev)) {
	case ASTEED_AST2700:
		pcie_phy->pehr_reg_27 = (void *)devfdt_get_addr_index(dev, 0);
		pcie_phy->platform = &pcie_phy_ast2700;
		break;
	case ASTEED_AST2600:
		pcie_phy->pehr_reg_26 = (void *)devfdt_get_addr_index(dev, 0);
		pcie_phy->platform = &pcie_phy_ast2600;
		break;
	default:
		pr_err("%s(): invalid data from udevce_id\n", __func__);
		return -EINVAL;
	}

	return 0;
}

struct phy_ops aspeed_pcie_phy_ops = {
	.init = aspeed_pcie_phy_init,
	.of_xlate = aspeed_pcie_phy_xlate,
	.set_mode = NULL,
};

static const struct udevice_id aspeed_pcie_ids[] = {
	{ .compatible = "aspeed,ast2600-pcie-phy", .data = ASTEED_AST2600 },
	{ .compatible = "aspeed,ast2700-pcie-phy", .data = ASTEED_AST2700 },
	{ }
};

U_BOOT_DRIVER(phy_aspeed_pcie) = {
	.name		= "phy_aspeed_pcie",
	.id		= UCLASS_PHY,
	.of_match	= aspeed_pcie_ids,
	.ops		= &aspeed_pcie_phy_ops,
	.of_to_plat	= aspeed_pcie_phy_of_to_plat,
	.probe		= aspeed_pcie_phy_probe,
	.priv_auto	= sizeof(struct aspeed_pcie_phy),
};
