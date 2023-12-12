// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) ASPEED Technology Inc.
 */
#include <common.h>
#include <clk.h>
#include <dm.h>
#include <errno.h>
#include <ram.h>
#include <regmap.h>
#include <reset.h>
#include <asm/io.h>
#include <asm/global_data.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/sizes.h>
#include <dt-bindings/clock/ast2700-clock.h>
#include <asm/arch-aspeed/scu_ast2700.h>

struct dramc_port {
	u32 configuration;
	u32 timeout;
	u32 read_qos;
	u32 write_qos;
	u32 monitor_config;
	u32 monitor_limit;
	u32 monitor_timer;
	u32 monitor_status;
	u32 bandwidth_log;
	u32 intf_monitor[3];
};

struct dramc_protect {
	u32 control;
	u32 err_status;
	u32 lo_addr;
	u32 hi_addr;
	u32 wr_master_0;
	u32 wr_master_1;
	u32 rd_master_0;
	u32 rd_master_1;
	u32 wr_secure_0;
	u32 wr_secure_1;
	u32 rd_secure_0;
	u32 rd_secure_1;
};

struct dramc_regs {
	u32 protection_key;		/* offset 0x00 */
	u32 intr_status;		/* offset 0x04 */
	u32 intr_clear;			/* offset 0x08 */
	u32 intr_mask;			/* offset 0x0C */
	u32 main_configuration;		/* offset 0x10 */
	u32 main_control;
	u32 main_status;
	u32 error_status;
	u32 ac_timing[7];
	u32 dfi_timing;
	u32 dfi_configuration;
	u32 dfi_control_msg;
	u32 mode_reg_control;
	u32 mode_reg_wr_op;
	u32 mode_reg_rd_op;
	u32 mr01_setting;
	u32 mr23_setting;
	u32 mr45_setting;
	u32 mr67_setting;
	u32 refresh_control;
	u32 refresh_mng_control;
	u32 refresh_status;
	u32 zqc_control;		/* offset 0x70 */
	u32 ecc_addr_range;		/* offset 0x74 */
	u32 ecc_failure_status;		/* offset 0x78 */
	u32 ecc_failure_addr;		/* offset 0x7C */
	u32 ecc_test_control;		/* offset 0x80 */
	u32 ecc_test_status;		/* offset 0x84 */
	u32 arbitration_control;	/* offset 0x88 */
	u32 enc_configuration;		/* offset 0x8c */
	u32 protect_lock_set;		/* offset 0x90 */
	u32 protect_lock_status;	/* offset 0x94 */
	u32 protect_lock_reset;		/* offset 0x98 */
	u32 enc_min_address;		/* offset 0x9c */
	u32 enc_max_address;		/* offset 0xa0 */
	u32 enc_key[4];			/* offset 0xa4~0xb0 */
	u32 enc_iv[3];			/* offset 0xb4~0xbc */
	u32 built_in_test_config;	/* offset 0xc0 */
	u32 built_in_test_addr;
	u32 built_in_test_size;
	u32 built_in_test_pattern;
	u32 built_in_test_result;	/* offset 0xd0 */
	u32 built_in_fail_addr;
	u32 built_in_fail_data[4];
	u32 reserved2[2];
	u32 debug_control;		/* offset 0xf0 */
	u32 debug_status;
	u32 phy_intf_status;
	u32 test_configuration;
	u32 graphic_memory_config;	/* 0x100 */
	u32 graphic_memory_0_ctrl;
	u32 graphic_memory_1_ctrl;
	u32 reserved3[0xf8];
	struct dramc_port port[6];	/* 0x200 */
	struct dramc_protect region[16];/* 0x600 */
};

struct dram_priv {
	struct ram_info info;
	struct dramc_regs *regs;

	void __iomem *phy_setting;
	void __iomem *phy_status;
	ulong clock_rate;
};

enum {
	SDRAM_SIZE_256MB = 0,
	SDRAM_SIZE_512MB,
	SDRAM_SIZE_1GB,
	SDRAM_SIZE_2GB,
	SDRAM_SIZE_4GB,
	SDRAM_SIZE_8GB,
	SDRAM_SIZE_MAX,
};

enum {
	SDRAM_VGA_RSVD_32MB = 0,
	SDRAM_VGA_RSVD_64MB,
};

static size_t ast2700_sdrammc_get_vga_mem_size(struct dram_priv *priv)
{
	struct ast2700_soc0_scu *scu;
	int nodeoffset;
	size_t vga_ram_size[] = {
		0x2000000, // 32MB
		0x4000000, // 64MB
		};
	int vga_sz_sel;
	ofnode node;
	int dual = 0;

	vga_sz_sel = readl(&priv->regs->graphic_memory_config) & 0x1;

	/* find the offset of compatible node */
	nodeoffset = fdt_node_offset_by_compatible(gd->fdt_blob, -1,
						   "aspeed,ast2700-scu0");
	if (nodeoffset < 0) {
		printf("%s: failed to get aspeed,ast2700-scu0\n", __func__);
		return -ENODEV;
	}

	/* get ast2700-scu0 node */
	node = offset_to_ofnode(nodeoffset);

	scu = (struct ast2700_soc0_scu *)ofnode_get_addr(node);

	if (scu->pci0_misc[28] & BIT(0)) {
		printf("VGA0:%ldMiB, ", vga_ram_size[vga_sz_sel] / SZ_1M);
		dual++;
	}

	if (scu->pci1_misc[28] & BIT(0)) {
		printf("VGA1:%ldMiB, ", vga_ram_size[vga_sz_sel] / SZ_1M);
		dual++;
	}

	return vga_ram_size[vga_sz_sel] * dual;
}

struct ddr_capacity {
	size_t size;
	int rfc[2];
};

#define SCU_IO_REG			0x14c02000
#define SCU_IO_HWSTRAP1			(SCU_IO_REG + 0x010)
#define IO_HWSTRAP1_DRAM_TYPE		BIT(10)

static int ast2700_sdrammc_calc_size(struct dram_priv *priv)
{
	struct ddr_capacity ram_size[] = {
		{0x10000000,	{208, 256}}, // 256MB
		{0x20000000,	{208, 416}}, // 512MB
		{0x40000000,	{208, 560}}, // 1GB
		{0x80000000,	{472, 880}}, // 2GB
		{0x100000000,	{656, 880}}, // 4GB
		{0x200000000,	{880, 880}}, // 8GB
		};
	u32 test_pattern = 0xdeadbeef;
	u32 val;
	int sz, ddr4;

	/* Configure ram size to max to enable whole area */
	val = readl(&priv->regs->main_configuration);
	val &= ~(0x7 << 2);
	writel(val | (SDRAM_SIZE_8GB << 2), &priv->regs->main_configuration);

	/* Clear basement. */
	writel(0, CFG_SYS_SDRAM_BASE);

	for (sz = SDRAM_SIZE_8GB - 1; sz > SDRAM_SIZE_256MB; sz--) {
		test_pattern = (test_pattern << 4) + sz;
		writel(test_pattern, CFG_SYS_SDRAM_BASE + ram_size[sz].size);

		if (readl(CFG_SYS_SDRAM_BASE) != test_pattern)
			break;
	}

	/* re-configure ram size to dramc. */
	val = readl(&priv->regs->main_configuration);
	val &= ~(0x7 << 2);
	writel(val | ((sz + 1) << 2), &priv->regs->main_configuration);

	ddr4 = ((readl(SCU_IO_HWSTRAP1) & IO_HWSTRAP1_DRAM_TYPE) ? 1 : 0);

	/* update rfc in ac_timing5 register. */
	val = readl(&priv->regs->ac_timing[4]);
	val &= ~(0x3ff);
	val |= (ram_size[sz + 1].rfc[ddr4] >> 1);
	writel(val, &priv->regs->ac_timing[4]);

	/* report actual ram base and size to kernel */
	priv->info.base = CFG_SYS_SDRAM_BASE;
	priv->info.size = ram_size[sz + 1].size - ast2700_sdrammc_get_vga_mem_size(priv);

	return 0;
}

static int ast2700_sdrammc_probe(struct udevice *dev)
{
	struct dram_priv *priv = (struct dram_priv *)dev_get_priv(dev);

	ast2700_sdrammc_calc_size(priv);

	return 0;
}

static int ast2700_sdrammc_of_to_plat(struct udevice *dev)
{
	struct dram_priv *priv = dev_get_priv(dev);

	priv->regs = (void *)(uintptr_t)devfdt_get_addr_index(dev, 0);
	priv->phy_setting = (void *)(uintptr_t)devfdt_get_addr_index(dev, 1);

	return 0;
}

static int ast2700_sdrammc_get_info(struct udevice *dev, struct ram_info *info)
{
	struct dram_priv *priv = dev_get_priv(dev);

	*info = priv->info;

	return 0;
}

static struct ram_ops ast2700_sdrammc_ops = {
	.get_info = ast2700_sdrammc_get_info,
};

static const struct udevice_id ast2700_sdrammc_ids[] = {
	{ .compatible = "aspeed,ast2700-sdrammc" },
	{ }
};

U_BOOT_DRIVER(sdrammc_ast2700) = {
	.name = "aspeed_ast2700_sdrammc",
	.id = UCLASS_RAM,
	.of_match = ast2700_sdrammc_ids,
	.ops = &ast2700_sdrammc_ops,
	.of_to_plat = ast2700_sdrammc_of_to_plat,
	.probe = ast2700_sdrammc_probe,
	.priv_auto = sizeof(struct dram_priv),
};
