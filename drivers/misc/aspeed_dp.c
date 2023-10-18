// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include <linux/bitfield.h>
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

#define MCU_CTRL                        0x00e0
#define  MCU_CTRL_AHBS_IMEM_EN          BIT(0)
#define  MCU_CTRL_AHBS_SW_RST           BIT(4)
#define  MCU_CTRL_AHBM_SW_RST           BIT(8)
#define  MCU_CTRL_CORE_SW_RST           BIT(12)
#define  MCU_CTRL_DMEM_SHUT_DOWN        BIT(16)
#define  MCU_CTRL_DMEM_SLEEP            BIT(17)
#define  MCU_CTRL_DMEM_CLK_OFF          BIT(18)
#define  MCU_CTRL_IMEM_SHUT_DOWN        BIT(20)
#define  MCU_CTRL_IMEM_SLEEP            BIT(21)
#define  MCU_CTRL_IMEM_CLK_OFF          BIT(22)
#define  MCU_CTRL_IMEM_SEL              BIT(24)
#define  MCU_CTRL_CONFIG                BIT(28)

#define MCU_INTR_CTRL                   0x00e8
#define  MCU_INTR_CTRL_CLR              GENMASK(7, 0)
#define  MCU_INTR_CTRL_MASK             GENMASK(15, 8)
#define  MCU_INTR_CTRL_EN               GENMASK(23, 16)

struct astdp_data {
	u16 scratch0;
	u16 scratch1;
	u16 dp_pin_mux;
};

static const struct astdp_data ast2600_data = {
	.scratch0 = 0x100,
	.scratch1 = 0,
	.dp_pin_mux = 0,
};

static const struct astdp_data ast2700_data = {
	.scratch0 = 0x900,
	.scratch1 = 0x910,
	.dp_pin_mux = 0x414,
};

struct aspeed_dp_priv {
	void *ctrl_base;
	void *mcud_base;	// mcu data
	void *mcuc_base;	// mcu ctrl regs
	void *mcui_base;	// mcu instruction mem
	struct regmap *scu;
};

static void _redriver_cfg(struct udevice *dev)
{
	struct aspeed_dp_priv *dp = dev_get_priv(dev);
	const u32 *cell;
	int i, len;
	u32 tmp;

	// update configs to dmem for re-driver
	writel(0x0000dead, dp->mcud_base + 0x0e00);	// mark re-driver cfg not ready
	cell = dev_read_prop(dev, "eq-table", &len);
	if (cell) {
		for (i = 0; i < len / sizeof(u32); ++i)
			writel(fdt32_to_cpu(cell[i]), dp->mcud_base + 0x0e04 + i * 4);
	} else {
		dev_dbg(dev, "%s(): Failed to get eq-table for re-driver\n", __func__);
		return;
	}

	tmp = dev_read_s32_default(dev, "i2c-base-addr", -1);
	if (tmp == -1) {
		dev_dbg(dev, "%s(): Failed to get i2c port's base address\n", __func__);
		return;
	}
	writel(tmp, dp->mcud_base + 0x0e28);

	tmp = dev_read_s32_default(dev, "i2c-buf-addr", -1);
	if (tmp == -1) {
		dev_dbg(dev, "%s(): Failed to get i2c port's buf address\n", __func__);
		return;
	}
	writel(tmp, dp->mcud_base + 0x0e2c);

	tmp = dev_read_s32_default(dev, "dev-addr", -1);
	if (tmp == -1)
		tmp = 0x70;
	writel(tmp, dp->mcud_base + 0x0e30);
	writel(0x0000cafe, dp->mcud_base + 0x0e00);	// mark re-driver cfg ready
}

// Decide the offset of scu scratch register.
static u32 _get_scu_offset(struct udevice *dev)
{
	struct astdp_data *data = (struct astdp_data *)dev_get_driver_data(dev);

	if (data->scratch1) {
		struct aspeed_dp_priv *priv = dev_get_priv(dev);
		u32 val;

		// There is 2 node in AST2700.
		// Use DP_output mux to decide which scu
		regmap_read(priv->scu, data->dp_pin_mux, &val);
		return (((val >> 8) & 0x3) == 1) ?
			data->scratch1 : data->scratch0;
	}

	return data->scratch0;
}

static int aspeed_dp_probe(struct udevice *dev)
{
	struct aspeed_dp_priv *dp = dev_get_priv(dev);
	struct reset_ctl dp_reset_ctl, dpmcu_reset_ctrl;
	struct clk clk;
	int i, ret = 0;
	u32 mcu_ctrl, val, scu_offset;
	bool is_mcu_stop = false;
	u32 fw[0x1000];

	scu_offset = _get_scu_offset(dev);
	regmap_read(dp->scu, scu_offset, &val);
	is_mcu_stop = ((val & BIT(13)) == 0);

	dev_dbg(dev, "%s(dev=%p) scu offset(%#x)\n", __func__, dev, scu_offset);

	ret = dev_read_u32_array(dev, "aspeed,dp-fw", fw, ARRAY_SIZE(fw));
	if (ret) {
		dev_err(dev, "Can't get dp-firmware, err(%d)\n", ret);
		return ret;
	}

	ret = reset_get_by_index(dev, 0, &dp_reset_ctl);
	if (ret) {
		dev_err(dev, "%s(): Failed to get dp reset signal\n", __func__);
		return ret;
	}

	ret = reset_get_by_index(dev, 1, &dpmcu_reset_ctrl);
	if (ret) {
		dev_err(dev, "%s(): Failed to get dp mcu reset signal\n", __func__);
		return ret;
	}

	ret = clk_get_by_index(dev, 0, &clk);
	if (ret) {
		debug("cannot get clock for %s: %d\n", dev->name, ret);
	} else {
		ret = clk_enable(&clk);
		if (ret) {
			dev_err(dev, "%s(): Failed to enable dp clk\n", __func__);
			return ret;
		}
	}

	/* reset for DPTX and DPMCU if MCU isn't running */
	if (is_mcu_stop) {
		reset_assert(&dp_reset_ctl);
		reset_assert(&dpmcu_reset_ctrl);
		reset_deassert(&dp_reset_ctl);
		reset_deassert(&dpmcu_reset_ctrl);
	}

	/* select HOST or BMC as display control master
	 * enable or disable sending EDID to Host
	 */
	writel(readl(dp->ctrl_base + 0xB8) & ~(BIT(24) | BIT(28)), dp->ctrl_base + 0xB8);

	/* DPMCU */
	/* clear display format and enable region */
	writel(0, dp->mcud_base + 0x0de0);

	_redriver_cfg(dev);

	/* load DPMCU firmware to internal instruction memory */
	if (is_mcu_stop) {
		mcu_ctrl = MCU_CTRL_CONFIG | MCU_CTRL_IMEM_CLK_OFF | MCU_CTRL_IMEM_SHUT_DOWN |
		      MCU_CTRL_DMEM_CLK_OFF | MCU_CTRL_DMEM_SHUT_DOWN | MCU_CTRL_AHBS_SW_RST;
		writel(mcu_ctrl, dp->mcuc_base + MCU_CTRL);

		mcu_ctrl &= ~(MCU_CTRL_IMEM_SHUT_DOWN | MCU_CTRL_DMEM_SHUT_DOWN);
		writel(mcu_ctrl, dp->mcuc_base + MCU_CTRL);

		mcu_ctrl &= ~(MCU_CTRL_IMEM_CLK_OFF | MCU_CTRL_DMEM_CLK_OFF);
		writel(mcu_ctrl, dp->mcuc_base + MCU_CTRL);

		mcu_ctrl |= MCU_CTRL_AHBS_IMEM_EN;
		writel(mcu_ctrl, dp->mcuc_base + MCU_CTRL);

		for (i = 0; i < ARRAY_SIZE(fw); i++)
			writel(fw[i], dp->mcui_base + (i * 4));

		/* release DPMCU internal reset */
		mcu_ctrl &= ~MCU_CTRL_AHBS_IMEM_EN;
		writel(mcu_ctrl, dp->mcuc_base + MCU_CTRL);
		mcu_ctrl |= MCU_CTRL_CORE_SW_RST | MCU_CTRL_AHBM_SW_RST;
		writel(mcu_ctrl, dp->mcuc_base + MCU_CTRL);
		//disable dp interrupt
		writel(FIELD_PREP(MCU_INTR_CTRL_EN, 0xff), dp->mcuc_base + MCU_INTR_CTRL);
	}

	//set vga ASTDP with DPMCU FW handling scratch
	regmap_update_bits(dp->scu, scu_offset, 0x7 << 9, 0x7 << 9);

	return 0;
}

static int aspeed_dp_of_to_plat(struct udevice *dev)
{
	struct aspeed_dp_priv *dp = dev_get_priv(dev);

	/* Get the controller base address */
	dp->ctrl_base = (void *)devfdt_get_addr_index(dev, 0);
	if (IS_ERR(dp->ctrl_base))
		return PTR_ERR(dp->ctrl_base);
	dp->mcud_base = (void *)devfdt_get_addr_index(dev, 1);
	if (IS_ERR(dp->mcud_base))
		return PTR_ERR(dp->mcud_base);
	dp->mcuc_base = (void *)devfdt_get_addr_index(dev, 2);
	if (IS_ERR(dp->mcuc_base))
		return PTR_ERR(dp->mcuc_base);
	dp->mcui_base = (void *)devfdt_get_addr_index(dev, 3);
	if (IS_ERR(dp->mcui_base))
		return PTR_ERR(dp->mcui_base);
	dp->scu = syscon_regmap_lookup_by_phandle(dev, "aspeed,scu");
	if (IS_ERR(dp->scu)) {
		dev_err(dev, "%s can't get regmap for scu!\n", dev->name);
		return PTR_ERR(dp->scu);
	}

	return 0;
}

static const struct udevice_id aspeed_dp_ids[] = {
	{ .compatible = "aspeed,ast2600-displayport",
	  .data = (ulong)&ast2600_data,	},
	{ .compatible = "aspeed,ast2700-displayport",
	  .data = (ulong)&ast2700_data,	},
	{ }
};

U_BOOT_DRIVER(aspeed_dp) = {
	.name		= "aspeed_dp",
	.id		= UCLASS_MISC,
	.of_match	= aspeed_dp_ids,
	.probe		= aspeed_dp_probe,
	.of_to_plat   = aspeed_dp_of_to_plat,
	.priv_auto = sizeof(struct aspeed_dp_priv),
};
