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
	struct sprot_cfg_ast2700 sprot_cfg = {0};
	struct sprot_sid_ast2700 sprot_sid_ctrl = {0};
	struct sprot_region_enable_ast2700 sprot_ctrl = {0};
	struct sprot_addr_ast2700 sprot_region = {0};
	uint32_t i;
	uint32_t ret = 0;
	uint32_t sprot_unit;

	sprot_unit = 1;
	sprot_cfg.raw = readl((void *)(uintptr_t)(ctrl_base + ESRAM_SPROT_CFG));
	debug("esram reg(cfg) \t\t\taddr:0x%lx,\tvalue:%x\n", ctrl_base + ESRAM_SPROT_CFG, sprot_cfg.raw);
	for (i = 0; i < sprot_cfg.b.unit; i++)
		sprot_unit = 2 * sprot_unit;

	sprot_sid_ctrl.sidg0.b.sid0 = 0x0; /* ARM_W */
	sprot_sid_ctrl.sidg0.b.sid1 = 0x1; /* ARM_R */
	sprot_sid_ctrl.sidg0.b.sid2 = 0x80; /* secure ARM_W */
	sprot_sid_ctrl.sidg0.b.sid3 = 0x81; /* secure ARM_R */
	debug("esram reg(sidg0) \t\taddr:0x%lx,\tvalue:%x\n", ctrl_base + ESRAM_SPROT_SIDG0, sprot_sid_ctrl.sidg0.raw);
	writel(sprot_sid_ctrl.sidg0.raw, (void *)(uintptr_t)(ctrl_base + ESRAM_SPROT_SIDG0));

	sprot_sid_ctrl.sidg1.b.sid4 = 0x4; /* SSP_S */
	sprot_sid_ctrl.sidg1.b.sid5 = 0xb; /* TSP_S */
	sprot_sid_ctrl.sidg1.b.sid6 = 0x21; /* MCU0D */
	debug("esram reg(sidg1) \t\taddr:0x%lx,\tvalue:%x\n", ctrl_base + ESRAM_SPROT_SIDG1, sprot_sid_ctrl.sidg1.raw);
	writel(sprot_sid_ctrl.sidg1.raw, (void *)(uintptr_t)(ctrl_base + ESRAM_SPROT_SIDG1));

	/*
	 * (0x12C0E1c0)region0: start_address = 0x0, size = 0x400
	 * Read/Write permission = SSP and secure ARM_R
	 * Start address = 0x1000_0000 : 0x1000_0000(SRAM base) + 0x0(start address) * 4(unit size)
	 * End address   = 0x1000_1000 : 0x1000_0000(Start address) + 0x400(size) * 4(unit size)
	 */
	sprot_ctrl.region0_enable.ctrl.b.w_enable_sid2 = 1; /* secure ARM_W */
	sprot_ctrl.region0_enable.ctrl.b.r_enable_sid3 = 1; /* secure ARM_R */
	sprot_ctrl.region0_enable.ctrl.b.w_enable_sid4 = 1; /* SSP_S */
	sprot_ctrl.region0_enable.ctrl.b.r_enable_sid4 = 1; /* SSP_S */
	debug("esram reg(region0_enable) \taddr:0x%lx,\tvalue:%x\n", ctrl_base + ESRAM_SPROT_CTL00, sprot_ctrl.region0_enable.ctrl.raw);
	writel(sprot_ctrl.region0_enable.ctrl.raw, (void *)(uintptr_t)(ctrl_base + ESRAM_SPROT_CTL00));

	sprot_region.region0.b.start_address = 0x0;
	sprot_region.region0.b.size = 0x400;
	debug("esram reg(region0) \t\taddr:0x%lx,\tvalue:%x\n", ctrl_base + ESRAM_SPROT_ADR00, sprot_region.region0.raw);
	writel(sprot_region.region0.raw, (void *)(uintptr_t)(ctrl_base + ESRAM_SPROT_ADR00));

	/*
	 * (0x12C0E1c4)region1: start_address = 0x400, size = 0x400
	 * Read/Write permission = SSP and ARM_R
	 * Start address = 0x1000_1000 : 0x1000_0000(SRAM base) + 0x400(start address) * 4(unit size)
	 * End address   = 0x1000_2000 : 0x1000_2000(Start address) + 0x400 * 4(unit size)
	 */
	sprot_ctrl.region1_enable.ctrl.b.w_enable_sid0 = 1; /* ARM_W */
	sprot_ctrl.region1_enable.ctrl.b.r_enable_sid1 = 1; /* ARM_R */
	sprot_ctrl.region1_enable.ctrl.b.w_enable_sid4 = 1; /* SSP_S */
	sprot_ctrl.region1_enable.ctrl.b.r_enable_sid4 = 1; /* SSP_S */
	debug("esram reg(region1_enable) \taddr:0x%lx,\tvalue:%x\n", ctrl_base + ESRAM_SPROT_CTL01,
	      sprot_ctrl.region1_enable.ctrl.raw);
	writel(sprot_ctrl.region1_enable.ctrl.raw, (void *)(uintptr_t)(ctrl_base + ESRAM_SPROT_CTL01));

	sprot_region.region1.b.start_address = 0x400;
	sprot_region.region1.b.size = 0x400;
	debug("esram reg(region1) \t\taddr:0x%lx,\tvalue:%x\n", ctrl_base + ESRAM_SPROT_ADR01,
	      sprot_region.region1.raw);
	writel(sprot_region.region1.raw, (void *)(uintptr_t)(ctrl_base + ESRAM_SPROT_ADR01));

	/*
	 * (0x12C0E1c8)region2: start_address = 0x800, size = 0x800
	 * Read/Write permission = MCU0D and SSP
	 * Start address = 0x1000_2000 : 0x1000_0000(SRAM base) + 0x800(start address) * 4(unit size)
	 * End address   = 0x1000_4000 : 0x1000_4000(Start address) + 0x800 * 4(unit size)
	 */
	sprot_ctrl.region2_enable.ctrl.b.w_enable_sid6 = 1; /* MCU0D */
	sprot_ctrl.region2_enable.ctrl.b.r_enable_sid6 = 1; /* MCU0D */
	sprot_ctrl.region2_enable.ctrl.b.w_enable_sid4 = 1; /* SSP */
	sprot_ctrl.region2_enable.ctrl.b.r_enable_sid4 = 1; /* SSP */
	debug("esram reg(region2_enable) \taddr:0x%lx,\tvalue:%x\n", ctrl_base + ESRAM_SPROT_CTL02,
	      sprot_ctrl.region2_enable.ctrl.raw);
	writel(sprot_ctrl.region2_enable.ctrl.raw, (void *)(uintptr_t)(ctrl_base + ESRAM_SPROT_CTL02));

	sprot_region.region2.b.start_address = 0x800;
	sprot_region.region2.b.size = 0x800;
	debug("esram reg(region2) \t\taddr:0x%lx,\tvalue:%x\n", ctrl_base + ESRAM_SPROT_ADR02,
	      sprot_region.region2.raw);
	writel(sprot_region.region2.raw, (void *)(uintptr_t)(ctrl_base + ESRAM_SPROT_ADR02));

	/*
	 * (0x12C0E1cc)region3: start_address = 0x1000, size = 0x800
	 * Read/Write permission = MCU0D and SSP
	 * Start address = 0x1000_4000 : 0x1000_0000(SRAM base) + 0x1000(start address) * 4(unit size)
	 * End address   = 0x1000_6000 : 0x1000_6000(Start address) + 0x800 * 4(unit size)
	 */
	sprot_ctrl.region3_enable.ctrl.b.w_enable_sid6 = 1; /* MCU0D */
	sprot_ctrl.region3_enable.ctrl.b.r_enable_sid6 = 1; /* MCU0D */
	sprot_ctrl.region3_enable.ctrl.b.w_enable_sid4 = 1; /* SSP */
	sprot_ctrl.region3_enable.ctrl.b.r_enable_sid4 = 1; /* SSP */
	debug("esram reg(region3_enable) \taddr:0x%lx,\tvalue:%x\n", ctrl_base + ESRAM_SPROT_CTL03,
	      sprot_ctrl.region3_enable.ctrl.raw);
	writel(sprot_ctrl.region3_enable.ctrl.raw, (void *)(uintptr_t)(ctrl_base + ESRAM_SPROT_CTL03));

	sprot_region.region3.b.start_address = 0x1000;
	sprot_region.region3.b.size = 0x800;
	debug("esram reg(region3) \t\taddr:0x%lx,\tvalue:%x\n", ctrl_base + ESRAM_SPROT_ADR03,
	      sprot_region.region3.raw);
	writel(sprot_region.region3.raw, (void *)(uintptr_t)(ctrl_base + ESRAM_SPROT_ADR03));

	debug("  sprot_unit \tvalue:%x\n", sprot_unit);
	debug("  Region0 protect range \taddr:0x%lx, \tsize:%x\n",
	      sprot_addr + sprot_region.region0.b.start_address * sprot_unit,
	      sprot_region.region0.b.size * sprot_unit);
	debug("  Region1 protect range \taddr:0x%lx, \tsize:%x\n",
	      sprot_addr + sprot_region.region1.b.start_address * sprot_unit,
	      sprot_region.region1.b.size * sprot_unit);
	debug("  Region2 protect range \taddr:0x%lx, \tsize:%x\n",
	      sprot_addr + sprot_region.region2.b.start_address * sprot_unit,
	      sprot_region.region2.b.size * sprot_unit);
	debug("  Region3 protect range \taddr:0x%lx, \tsize:%x\n",
	      sprot_addr + sprot_region.region3.b.start_address * sprot_unit,
	      sprot_region.region3.b.size * sprot_unit);

	return ret;
}

static int gsram_prictrl_init(uintptr_t ctrl_base, uintptr_t sprot_addr, uint32_t sprot_size)
{
	struct sprot_cfg_ast2700 sprot_cfg = {0};
	struct sprot_sid_ast2700 sprot_sid_ctrl = {0};
	struct sprot_region_enable_ast2700 sprot_ctrl = {0};
	struct sprot_addr_ast2700 sprot_region = {0};
	uint32_t i;
	uint32_t ret = 0;
	uint32_t sprot_unit;

	sprot_unit = 1;
	sprot_cfg.raw = readl((void *)(uintptr_t)(ctrl_base + GSRAM_SPROT_CFG));
	debug("gsram reg(cfg) \t\t\taddr:0x%lx,\tvalue:%x\n", ctrl_base + GSRAM_SPROT_CFG, sprot_cfg.raw);
	for (i = 0; i < sprot_cfg.b.unit; i++)
		sprot_unit = 2 * sprot_unit;

	sprot_sid_ctrl.sidg0.b.sid0 = 0x20; /* BootMCU I */
	sprot_sid_ctrl.sidg0.b.sid1 = 0x21; /* BootMCU D */
	sprot_sid_ctrl.sidg0.b.sid2 = 0x0C; /* EB(eMMC Boot) */
	sprot_sid_ctrl.sidg0.b.sid3 = 0x32; /* I2C */
	debug("gsram reg(sidg0) \t\taddr:0x%lx,\tvalue:%x\n", ctrl_base + GSRAM_SPROT_SIDG0,
	      sprot_sid_ctrl.sidg0.raw);
	writel(sprot_sid_ctrl.sidg0.raw, (void *)(uintptr_t)(ctrl_base + GSRAM_SPROT_SIDG0));

	sprot_sid_ctrl.sidg1.b.sid4 = 0xF; /* U3 */
	sprot_sid_ctrl.sidg1.b.sid5 = 0x3A; /* UUARTA */
	sprot_sid_ctrl.sidg1.b.sid6 = 0x3B; /* UUARTB */
	debug("gsram reg(sidg1) \t\taddr:0x%lx,\tvalue:%x\n", ctrl_base + GSRAM_SPROT_SIDG1,
	      sprot_sid_ctrl.sidg1.raw);
	writel(sprot_sid_ctrl.sidg1.raw, (void *)(uintptr_t)(ctrl_base + GSRAM_SPROT_SIDG1));

	/* region0 read and write permission. */
	sprot_ctrl.region0_enable.ctrl.b.w_enable_sid0 = 1; /* Enable write for BootMCU I */
	sprot_ctrl.region0_enable.ctrl.b.r_enable_sid0 = 1; /* Enable read for BootMCU I */
	sprot_ctrl.region0_enable.ctrl.b.w_enable_sid1 = 1; /* Enable write for BootMCU D */
	sprot_ctrl.region0_enable.ctrl.b.r_enable_sid1 = 1; /* Enable read for BootMCU D */
	sprot_ctrl.region0_enable.ctrl.b.w_enable_sid2 = 1; /* Enable write for eMMC Boot */
	sprot_ctrl.region0_enable.ctrl.b.r_enable_sid2 = 1; /* Enable read for eMMC Boot */
	sprot_ctrl.region0_enable.ctrl.b.w_enable_sid3 = 1; /* Enable write for I2C */
	sprot_ctrl.region0_enable.ctrl.b.r_enable_sid3 = 1; /* Enable read for I2C */
	sprot_ctrl.region0_enable.ctrl.b.w_enable_sid4 = 1; /* Enable write for U3 */
	sprot_ctrl.region0_enable.ctrl.b.r_enable_sid4 = 1; /* Enable read for U3 */
	sprot_ctrl.region0_enable.ctrl.b.w_enable_sid5 = 1; /* Enable write for UUARTA */
	sprot_ctrl.region0_enable.ctrl.b.r_enable_sid5 = 1; /* Enable read for UUARTA */
	sprot_ctrl.region0_enable.ctrl.b.w_enable_sid6 = 1; /* Enable write for UUARTB */
	sprot_ctrl.region0_enable.ctrl.b.r_enable_sid6 = 1; /* Enable read for UUARTB */
	sprot_ctrl.region0_enable.ctrl.b.r_enable_sid7 = 1; /* Enable read for Others */
	debug("gsram reg(region0_enable) \taddr:0x%lx,\tvalue:%x\n", ctrl_base + GSRAM_SPROT_CTL00,
	      sprot_ctrl.region0_enable.ctrl.raw);
	writel(sprot_ctrl.region0_enable.ctrl.raw, (void *)(uintptr_t)(ctrl_base + GSRAM_SPROT_CTL00));

	/*
	 * (0x14c0a3c0)region0:
	 * Start address : 0x14b8_0000(SRAM base) + 0x0(start address) * 4(unit size)
	 * End address   : 0x14bc_0000(Start address) + 0xffff(sprot_size) * 4(unit size)
	 */
	sprot_region.region0.b.start_address = 0x0;
	if (sprot_size >= 0x40000)
		sprot_region.region0.b.size = 0xffff; /* 0x14b8_0000 + 0x3fffc in max setting */
	else
		sprot_region.region0.b.size = sprot_size >> 2; /* 0x14b8_0000 + sprot_size */
	debug("gsram reg(region0) \t\taddr:0x%lx,\tvalue:%x\n", ctrl_base + GSRAM_SPROT_ADR00,
	      sprot_region.region0.raw);
	writel(sprot_region.region0.raw, (void *)(uintptr_t)(ctrl_base + GSRAM_SPROT_ADR00));

	debug("  sprot_unit \tvalue:%x\n", sprot_unit);
	debug("  Region0 protect range \taddr:0x%lx, \tsize:%x\n",
	      sprot_addr + sprot_region.region0.b.start_address * sprot_unit,
	      sprot_region.region0.b.size * sprot_unit);

	return ret;
}

static int sram_prictrl_hw_init(struct udevice *dev)
{
	/*
	 * Magic value used to check SRAM privilege control readiness.
	 * 0x7F7F7F7E is chosen as a rarely-occurring pattern to avoid accidental matches
	 * with normal SRAM contents. This ensures the memory region is writable and accessible.
	 */
	const uint32_t magic = 0x7F7F7F7E;
	struct sram_prictrl_aspeed_config *cfg = dev_get_priv(dev);

	/* Check whether esram privilege control is ready */
	writel(magic, (void *)(uintptr_t)cfg->esram_base);
	if (readl((void *)(uintptr_t)cfg->esram_base) != magic)
		return -EAGAIN;
	esram_prictrl_init(cfg->esram_ctrl_base, cfg->esram_base, cfg->esram_size);

	/* Check whether gsram privilege control is ready */
	writel(magic, (void *)(uintptr_t)cfg->gsram_base);
	if (readl((void *)(uintptr_t)cfg->gsram_base) != magic)
		return -EAGAIN;
	gsram_prictrl_init(cfg->gsram_ctrl_base, cfg->gsram_base, cfg->gsram_size);

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
	debug("esram buffer base \taddr:0x%lx,\tsize:%x\n", (uintptr_t)cfg->esram_base, cfg->esram_size);
	debug("esram ctrl base \taddr:0x%lx\n", (uintptr_t)cfg->esram_ctrl_base);

	cfg->gsram_base = dev_read_addr_size_index(dev, 2, &cfg->gsram_size);
	cfg->gsram_ctrl_base = dev_read_addr_index(dev, 3);
	debug("gsram buffer base \taddr:0x%lx,\tsize:%x\n", (uintptr_t)cfg->gsram_base, cfg->gsram_size);
	debug("gsram ctrl base \taddr:0x%lx\n", (uintptr_t)cfg->gsram_ctrl_base);

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
