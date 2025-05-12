// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include <dm.h>
#include <errno.h>
#include <log.h>
#include <malloc.h>
#include <asm/io.h>
#include <linux/bitfield.h>
#include <linux/delay.h>

#include "aspeed_sram_prictrl.h"

static int esram_prictrl_init(uintptr_t ctrl_base, uintptr_t sprot_addr, uint32_t sprot_size)
{
	struct sprot_cfg_ast2700 *sprot_cfg;		/* config */
	struct sprot_sid_ast2700 *sprot_sid_ctrl;	/* ID definition */
	struct sprot_region_enable_ast2700 *sprot_ctrl; /* SID Enable ctrl */
	struct sprot_addr_ast2700 *sprot_region;	/* region definition */
	uint32_t i;
	uint32_t ret = 0;
	uint32_t sprot_unit;

	sprot_cfg = (struct sprot_cfg_ast2700 *)(ctrl_base + ESRAM_SPROT_CFG);
	sprot_unit = 1;
	for (i = 0; i < sprot_cfg->b.unit; i++)
		sprot_unit = 2 * sprot_unit;

	debug("reg(cfg) \t\taddr:0x%p,\tvalue:%x\n", &sprot_cfg->raw, sprot_cfg->raw);
	debug("\tsprot_unit \tvalue:%x\n", sprot_unit);

	sprot_sid_ctrl = (struct sprot_sid_ast2700 *)(ctrl_base + ESRAM_SPROT_SIDG);
	sprot_ctrl = (struct sprot_region_enable_ast2700 *)(ctrl_base + ESRAM_SPROT_CTL);
	sprot_region = (struct sprot_addr_ast2700 *)(ctrl_base + ESRAM_SPROT_ADR);

	/* (0x12C0E100)SPROT_SIDG0 = 0x0b04; */
	sprot_sid_ctrl->sidg0.b.sid0 = 0x4; /* SSP */
	sprot_sid_ctrl->sidg0.b.sid1 = 0xb; /* TSP */
	debug("reg(sidg0) \t\taddr:0x%p,\tvalue:%x\n", &sprot_sid_ctrl->sidg0.raw,
	      sprot_sid_ctrl->sidg0.raw);

	/* region0 for SSP/TSP read write but others read only . */
	sprot_ctrl->region0_enable.ctrl.b.w_enable_sid0 = 1; /* region0 enable write for SSP */
	sprot_ctrl->region0_enable.ctrl.b.r_enable_sid0 = 1; /* region0 enable read for SSP */
	sprot_ctrl->region0_enable.ctrl.b.w_enable_sid1 = 1; /* region0 enable write for TSP */
	sprot_ctrl->region0_enable.ctrl.b.r_enable_sid1 = 1; /* region0 enable read for TSP */
	sprot_ctrl->region0_enable.ctrl.b.r_enable_sid7 = 1; /* region0 enable read for Others */
	debug("reg(region0_enable) \taddr:0x%p,\tvalue:%x\n", &sprot_ctrl->region0_enable.ctrl.raw,
	      sprot_ctrl->region0_enable.ctrl.raw);

	/*
	 * (0x12C0E1c0)region0:
	 * Start address : 0x1000_0000(SRAM base) + 0x0(start address) * 4(unit size)
	 * End address   : 0x1000_0000(Start address) + sprot_size
	 */
	sprot_region->region0.b.start_address = 0x0;
	if (sprot_size >= 0x40000)
		sprot_region->region0.b.size = 0xffff; /* 0x1000_0000 + 0x3fffc in max setting */
	else
		sprot_region->region0.b.size = sprot_size >> 2; /* 0x1000_0000 + sprot_size */
	debug("reg(region0) \t\taddr:0x%p,\tvalue:%x\n", &sprot_region->region0.raw,
	      sprot_region->region0.raw);
	debug("\tProtect range \taddr:0x%lx, \tsize:%x\n",
	      sprot_addr + sprot_region->region0.b.start_address * sprot_unit,
	      sprot_region->region0.b.size * sprot_unit);

	return ret;
}

static int sram_prictrl_hw_init(struct udevice *dev)
{
	const uint32_t magic = 0x7F7F7F7E;
	struct sram_prictrl_aspeed_config *cfg = dev_get_priv(dev);

	/* Check whether esram privilege control is ready */
	writel(magic, (void *)cfg->esram_base);
	if (readl((void *)cfg->esram_base) != magic)
		return -EAGAIN;
	esram_prictrl_init(cfg->esram_ctrl_base, cfg->esram_base, cfg->esram_size);

	return 0;
}

static int aspeed_sram_prictrl_probe(struct udevice *dev)
{
	int ret = 0;

	if (!dev)
		return -EINVAL;

	ret = sram_prictrl_hw_init(dev);
	if (ret)
		return -EAGAIN;

	return ret;
}

static int aspeed_sram_prictrl_of_to_plat(struct udevice *dev)
{
	struct sram_prictrl_aspeed_config *cfg = dev_get_priv(dev);

	/* Get the base address from device tree */
	cfg->esram_base = dev_read_addr_size_index(dev, 0, &cfg->esram_size);
	cfg->esram_ctrl_base = dev_read_addr_index(dev, 1);
	debug("reg(esram base) \t\taddr:0x%p,\tsize:%x\n", (void *)cfg->esram_base, cfg->esram_size);
	debug("reg(esram ctrl base) \taddr:0x%p\n", (void *)cfg->esram_ctrl_base);

	return 0;
}

static const struct udevice_id aspeed_sram_prictrl_ids[] = {
	{.compatible = "aspeed,ast2700-sram-prictrl"},
	{}
};

U_BOOT_DRIVER(aspeed_sram_prictrl) = {
	.name = "aspeed_sram_prictrl",
	.id = UCLASS_MISC,
	.of_match = aspeed_sram_prictrl_ids,
	.probe = aspeed_sram_prictrl_probe,
	.of_to_plat = aspeed_sram_prictrl_of_to_plat,
	.priv_auto = sizeof(struct sram_prictrl_aspeed_config),
};
