// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include <dm.h>
#include <misc.h>
#include <syscon.h>
#include <asm/io.h>
#include <linux/bitfield.h>

#include "aspeed_mhmac.h"

#define MHMAC_IRQ_ENABLE (0)

static struct aspeed_mhmac_master_info region_master[] = {
	[0] = { "ssp", { CM4_SSP_I, CM4_SSP_D, CM4_SSP_S, MCU0_D } },
	[1] = { "tsp", { CM4_TSP, NO_MASTER, NO_MASTER, MCU0_D } },
	[2] = { "ns-ca35", { CA35_W_NS, CA35_R_NS, NO_MASTER, MCU0_D } },
	[3] = { "s-ca35", { CA35_W_S, CA35_R_S, NO_MASTER, MCU0_D } },
};

/********************************************************************
 *                       MHMAC debug feature                        *
 ********************************************************************/
static void aspeed_mhmac_debug_cfg(struct udevice *dev)
{
#if MHMAC_DEBUG
	struct aspeed_mhmac_config *cfg = dev_get_priv(dev);
	struct aspeed_mhmac_region *region = NULL;

	/* Pre-Init mhmac configuration dump*/
	aspeed_mhmac_debug("mhmac base addr: %p\n", cfg->base);
	aspeed_mhmac_debug("scu base addr: %p\n", cfg->scu_base);
	aspeed_mhmac_debug("region number: %d\n", cfg->region_sel);

	/* Pre-Init hmac region configuration dump */
	for (region = &cfg->region[0]; region <= &cfg->region[3]; region++) {
		aspeed_mhmac_debug("region %d configuration\n",
				   region->region_num);
		aspeed_mhmac_debug("dram address range: 0x%08x 0x%08x\n",
				   region->dram_base, region->dram_size);
		aspeed_mhmac_debug("master 0x%x 0x%x 0x%x 0x%x\n",
				   region->master[0], region->master[1],
				   region->master[2], region->master[3]);
		aspeed_mhmac_debug_hexdump("hmac key", region->hmac_key,
					   MHMAC_KEY_LEN_BYTES);
	}
#endif
}

static void aspeed_mhmac_debug_reg(struct udevice *dev)
{
#if MHMAC_DEBUG
	struct aspeed_mhmac_config *cfg = dev_get_priv(dev);

	aspeed_mhmac_debug("0x%08x 0x%08x 0x%08x 0x%08x\n",
			   readl(cfg->base + MHMAC_CTRL_OFFSET),
			   readl(cfg->base + MHMAC_REG_PROT_OFFSET),
			   readl(cfg->base + MHMAC_RESERVE_OFFSET),
			   readl(cfg->base + MHMAC_REG_MID_OFFSET));

	aspeed_mhmac_debug("0x%08x 0x%08x 0x%08x 0x%08x\n",
			   readl(cfg->base + MHMAC_ZONE0_RANGE_OFFSET),
			   readl(cfg->base + MHMAC_ZONE1_RANGE_OFFSET),
			   readl(cfg->base + MHMAC_ZONE2_RANGE_OFFSET),
			   readl(cfg->base + MHMAC_ZONE3_RANGE_OFFSET));

	aspeed_mhmac_debug("0x%08x 0x%08x 0x%08x 0x%08x\n",
			   readl(cfg->base + MHMAC_ZONE0_MIDS_OFFSET),
			   readl(cfg->base + MHMAC_ZONE1_MIDS_OFFSET),
			   readl(cfg->base + MHMAC_ZONE2_MIDS_OFFSET),
			   readl(cfg->base + MHMAC_ZONE3_MIDS_OFFSET));

	aspeed_mhmac_debug("0x%08x 0x%08x\n",
			   readl(cfg->base + MHMAC_INT_EN_OFFSET),
			   readl(cfg->base + MHMAC_INT_ST_OFFSET));
#endif
}

/********************************************************************
 *                   MHMAC software util function                   *
 ********************************************************************/
static bool aspeed_mhmac_secmid(uint8_t mid)
{
	if (mid == CA35_W_S || mid == CA35_R_S)
		return true;
	else
		return false;
}

static int aspeed_mhmac_gen_key(struct udevice *dev, uint8_t *hmac_key,
				uint32_t key_len)
{
	uint32_t *rng_hmac_key = NULL;
	struct aspeed_mhmac_config *cfg = dev_get_priv(dev);

	if (key_len != MHMAC_KEY_LEN_BYTES)
		return -1;

	for (rng_hmac_key = (uint32_t *)hmac_key;
	     rng_hmac_key < (uint32_t *)(hmac_key + key_len); rng_hmac_key++)
		*rng_hmac_key = readl(cfg->scu_base + MHMAC_SCU_RNG_OFFSET);

	return 0;
}

static uint8_t *aspeed_mhmac_get_master_info(const char *name)
{
	int i = 0;

	if (!name)
		return NULL;

	for (i = 0; i < ARRAY_SIZE(region_master); i++) {
		if (strcmp(region_master[i].name, name) == 0)
			return region_master[i].master_id;
	}

	return NULL;
}

static struct aspeed_mhmac_region *aspeed_mhmac_get_region(struct udevice *dev,
							   uint8_t num)
{
	uint8_t max_region = 0;
	struct aspeed_mhmac_config *cfg = dev_get_priv(dev);

	if (cfg->region_sel < 1 || cfg->region_sel > 3)
		return NULL;

	/* max_region number = the total region amount - 1 */
	max_region = MHMAX_MAX_ZONE_NUMBER(cfg->region_sel) - 1;

	if (num > max_region)
		return NULL;

	return &cfg->region[num];
}

/********************************************************************
 *                  MHMAC device tree util function                 *
 ********************************************************************/
static int aspeed_mhmac_get_dts_node(struct udevice *dev, ofnode *node,
				     char *subname, char *phname)
{
	uint32_t phandle = 0;
	ofnode tmp_node = { 0 };

	/* Get the mhmac node information */
	tmp_node = !subname ? dev_ofnode(dev) : dev_read_subnode(dev, subname);

	/* Get the phandle node */
	if (phname) {
		if (ofnode_read_u32(tmp_node, phname, &phandle))
			return -ENODEV;

		/* Get the phandle node */
		tmp_node = ofnode_get_by_phandle(phandle);
		if (!ofnode_valid(tmp_node))
			return -ENODEV;
	}

	*node = tmp_node;
	return 0;
}

static uint32_t aspeed_mhmac_dtb_read_addr(struct udevice *dev, char *subname,
					   char *property)
{
	uint32_t addr = 0;
	ofnode node = { 0 };

	if (!aspeed_mhmac_get_dts_node(dev, &node, subname, property))
		addr = ofnode_get_addr(node);

	return addr != FDT_ADDR_T_NONE ? addr : 0;
}

static int aspeed_mhmac_dtb_read_size(struct udevice *dev, char *subname,
				      char *property)
{
	int size = 0;
	ofnode node = { 0 };

	if (!aspeed_mhmac_get_dts_node(dev, &node, subname, property))
		size = ofnode_get_size(node);

	return size;
}

static const char *aspeed_mhmac_dtb_read_str(struct udevice *dev, char *subname,
					     char *property)
{
	const char *str = NULL;
	ofnode node = { 0 };

	if (!aspeed_mhmac_get_dts_node(dev, &node, subname, NULL))
		str = ofnode_read_string(node, property);

	return str;
}

static int aspeed_mhmac_config_dtb_region(struct udevice *dev, int index)
{
#define ASPEED_MHMAC_DRAM_REMAP(_addr) ((_addr) - 0x80000000)
	const char *master = NULL;
	char propname[16] = { 0 };
	struct aspeed_mhmac_config *cfg = dev_get_priv(dev);
	struct aspeed_mhmac_region *region = &cfg->region[index];

	snprintf(propname, sizeof(propname), "mhmac-region-%d", index);

	/* Denote the region number */
	region->region_num = index;

	/* Get the dram base address and size from dts */
	region->dram_base = aspeed_mhmac_dtb_read_addr(dev, propname, "region");
	region->dram_base = ASPEED_MHMAC_DRAM_REMAP(region->dram_base);
	region->dram_size = aspeed_mhmac_dtb_read_size(dev, propname, "region");

	/* Get the master configuration */
	master = aspeed_mhmac_dtb_read_str(dev, propname, "master");
	region->master = aspeed_mhmac_get_master_info(master);

	/* Generate the hmac key randomly */
	aspeed_mhmac_gen_key(dev, region->hmac_key, MHMAC_KEY_LEN_BYTES);

	return 0;
}

/********************************************************************
 *                  MHMAC hardware util function                    *
 ********************************************************************/
static void aspeed_mhmac_invalid_cache(struct aspeed_mhmac_config *cfg)
{
	uint32_t reg_val = 0;

	/* Datasheet descripts the invalid operation as "clean" */
	reg_val = readl(cfg->base + MHMAC_CTRL_OFFSET);
	writel(reg_val | MHMAC_CTRL_CLEAN_CACHE, cfg->base + MHMAC_CTRL_OFFSET);

	/* Restore the mhmac configuration */
	writel(reg_val, cfg->base + MHMAC_CTRL_OFFSET);
}

static void aspeed_mhmac_flush_cache(struct aspeed_mhmac_config *cfg)
{
	uint32_t *mhmac_start = (uint32_t *)MHMAC_ZONE_0_REMAP_ADDR;
	uint32_t *mhmac_end =
		(uint32_t *)(mhmac_start + MHMAC_INTERNAL_CACHE_SIZE);

	/* MHMAC hardware does not support mhmac clean, re-write it */
	for (mhmac_start = (uint32_t *)MHMAC_ZONE_0_REMAP_ADDR;
	     mhmac_start < mhmac_end; mhmac_start++)
		writel(readl(mhmac_start), mhmac_start);

	aspeed_mhmac_invalid_cache(cfg);
}

static void aspeed_mhmac_bootmcu_reamp(struct aspeed_mhmac_config *cfg)
{
	uint32_t reg_val = 0;

	reg_val = readl(cfg->scu_base + MHMAC_SCU_REMAP_OFFSET);
	reg_val &= ~MHMAC_BOOTMCU_REMAP_MASK;
	reg_val |= FIELD_PREP(MHMAC_BOOTMCU_REMAP_MASK,
			      MHMAC_ZONE_0_REMAP_ADDR >> MHMAC_MOST_BYTE_SHIFT);
	writel(reg_val, cfg->scu_base + MHMAC_SCU_REMAP_OFFSET);
}

static int aspeed_mhmac_memset(uint32_t mhmac_addr, int val, uint32_t len)
{
	uint32_t *mhmac_start = (uint32_t *)mhmac_addr;
	uint32_t *mhmac_end = (uint32_t *)(mhmac_addr + len);

	if (len % 4 != 0)
		return -EINVAL;

	/* Mhmac mapping memory set */
	aspeed_mhmac_debug("Write %x ~ %p\n", mhmac_addr, mhmac_end);
	for (mhmac_start = (uint32_t *)mhmac_addr; mhmac_start < mhmac_end;
	     mhmac_start++) {
		writel(val, mhmac_start);

		/* Check memory init success or not */
		if (readl(mhmac_start) != val)
			return -EBUSY;
	}

	return 0;
}

static int aspeed_mhmac_memcpy(uint32_t mhmac_addr, const uint32_t *buf,
			       uint32_t len)
{
	uint32_t *mhmac_start = (uint32_t *)mhmac_addr;
	uint32_t *mhmac_end = (uint32_t *)(mhmac_addr + len);

	if (len % 4 != 0)
		return -EINVAL;

	/* Mhmac mapping memory copy */
	aspeed_mhmac_debug("Write %x ~ %p\n", mhmac_addr, mhmac_end);
	for (mhmac_start = (uint32_t *)mhmac_addr; mhmac_start < mhmac_end;
	     mhmac_start++, buf++) {
		writel(*buf, mhmac_start);

		/* Check memory init success or not */
		if (readl(mhmac_start) != *buf)
			return -EBUSY;
	}

	return 0;
}

/********************************************************************
 *               MHMAC programing sequence interface                *
 ********************************************************************/
static int aspeed_mhmac_region_setup_range(struct aspeed_mhmac_config *cfg,
					   struct aspeed_mhmac_region *region)
{
	uint32_t zone_addr = 0;
	uint32_t zone_size = 0;
	uint32_t reg_val = 0;
	uint32_t offset = 0;

	/* Check region base address 64K alignment */
	if ((region->dram_base & MHMAC_ZONE_ALIGN_MASK) != 0)
		return -ENOMEM;

	/* Check whether region size in dram enough */
	if (region->dram_size < MHMAC_SIZE_TO_DRAM_SIZE(region->mhmac_size))
		return -ENOMEM;

	/* Check region size 64K alignment */
	if ((region->mhmac_size & MHMAC_ZONE_ALIGN_MASK) != 0)
		return -ENOMEM;

	/* Check whether mhmac is overflow */
	if (region->mhmac_size > MHMAC_MAX_ZONE_SIZE(cfg->region_sel))
		return -ENOMEM;

	/* Get zone base address and size configuration register offset */
	offset = MHMAC_ZONE0_RANGE_OFFSET + region->region_num * 4;

	/* Configure mhmac zone range */
	zone_addr = region->dram_base / MHMAC_ZONE_ALIGN_SIZE;
	zone_size = (region->mhmac_size / MHMAC_ZONE_ALIGN_SIZE) - 1;
	reg_val = FIELD_PREP(MHMAC_ZONE_RANGE_SIZE_MASK, zone_size) |
		  FIELD_PREP(MHMAC_ZONE_RANGE_ADDR_MASK, zone_addr);
	writel(reg_val, cfg->base + offset);

	return 0;
}

static int aspeed_mhmac_region_setup_perm(struct aspeed_mhmac_config *cfg,
					  struct aspeed_mhmac_region *region)
{
	uint32_t reg_val = 0;
	uint32_t offset = 0;

	/* Get zone permission configuration register offset */
	offset = MHMAC_ZONE0_MIDS_OFFSET + region->region_num * 4;

	/* Setup master ID */
	reg_val = FIELD_PREP(MHMAC_ZONE_MIDS_MID0_MASK, region->master[0]);
	reg_val |= FIELD_PREP(MHMAC_ZONE_MIDS_MID1_MASK, region->master[1]);
	reg_val |= FIELD_PREP(MHMAC_ZONE_MIDS_MID2_MASK, region->master[2]);
	reg_val |= FIELD_PREP(MHMAC_ZONE_MIDS_MID3_MASK, region->master[3]);

	/* Setup master ID secure bit */
	reg_val |= aspeed_mhmac_secmid(region->master[0]) ?
			   MHMAC_ZONE_MIDS_MID0_SEC :
			   0;
	reg_val |= aspeed_mhmac_secmid(region->master[1]) ?
			   MHMAC_ZONE_MIDS_MID1_SEC :
			   0;
	reg_val |= aspeed_mhmac_secmid(region->master[2]) ?
			   MHMAC_ZONE_MIDS_MID2_SEC :
			   0;
	reg_val |= aspeed_mhmac_secmid(region->master[3]) ?
			   MHMAC_ZONE_MIDS_MID3_SEC :
			   0;

	/* Congiure mhmac permission */
	writel(reg_val, cfg->base + offset);

	return 0;
}

static int
aspeed_mhmac_region_setup_hmac_key(struct aspeed_mhmac_config *cfg,
				   struct aspeed_mhmac_region *region)
{
	uint32_t offset = 0;

	/* Get zone hmac key configuration register offset */
	offset = MHMAC_ZONE0_KEY_OFFSET +
		 region->region_num * MHMAC_KEY_LEN_BYTES;

	/* Set region hmac key */
	memcpy_toio(cfg->base + offset, region->hmac_key, MHMAC_KEY_LEN_BYTES);

	return 0;
}

static int aspeed_mhmac_region_init_mem(struct aspeed_mhmac_config *cfg,
					struct aspeed_mhmac_region *region)
{
	int ret = 0;
	uint32_t remap_addr[] = {
		MHMAC_ZONE_0_REMAP_ADDR,
		MHMAC_ZONE_1_REMAP_ADDR,
		MHMAC_ZONE_2_REMAP_ADDR,
		MHMAC_ZONE_3_REMAP_ADDR,
	};

	if (region->region_num >= MHMAX_MAX_ZONE_NUMBER(cfg->region_sel))
		return -EINVAL;

	/* Copy input data to mhmac ram */
	aspeed_mhmac_bootmcu_reamp(cfg);
	ret = aspeed_mhmac_memset(remap_addr[region->region_num], 0,
				  region->mhmac_size);
	if (ret)
		return ret;

	/* Clean and invalid cache */
	aspeed_mhmac_flush_cache(cfg);

	region->mhmac_base = remap_addr[region->region_num];

	return ret;
}

static int aspeed_mhmac_region_enable(struct aspeed_mhmac_config *cfg,
				      struct aspeed_mhmac_region *region)
{
	/* Enable mhmac check fail interrupt */
#if MHMAC_IRQ_ENABLE
	setbits_le32(cfg->base + MHMAC_INT_EN_OFFSET,
		     MHMAC_CTRL_ZONE_INT_EN(region->region_num));
#endif

	/* Enable mhmac region */
	setbits_le32(cfg->base + MHMAC_CTRL_OFFSET,
		     MHMAC_CTRL_ZONE_ENABLE(region->region_num));

	return 0;
}

static int aspeed_mhmac_region_setting(struct udevice *dev,
				       struct aspeed_mhmac_region *region)
{
	int ret = 0;
	struct aspeed_mhmac_config *cfg = dev_get_priv(dev);

	/* Check the base and size should not be zero, if == zero, do nothing */
	if (!region->dram_base || !region->dram_size)
		return 0;

	/* Setup zone-n range */
	ret = aspeed_mhmac_region_setup_range(cfg, region);
	if (ret)
		goto fail;

	/* Setup zone-n permission */
	ret = aspeed_mhmac_region_setup_perm(cfg, region);
	if (ret)
		goto fail;

	/* Setup zone-n hmac key */
	ret = aspeed_mhmac_region_setup_hmac_key(cfg, region);
	if (ret)
		goto fail;

	/* Initialize zone-n memory */
	ret = aspeed_mhmac_region_init_mem(cfg, region);
	if (ret)
		goto fail;

	/* Enable zone-n monitor */
	ret = aspeed_mhmac_region_enable(cfg, region);
	if (ret)
		goto fail;

	aspeed_mhmac_debug_reg(dev);

	return 0;
fail:
	printf("Mhmac region %d setup fail (%d).\n", region->region_num, ret);
	return ret;
}

static int aspeed_mhmac_lock(struct udevice *dev)
{
	struct aspeed_mhmac_config *cfg = dev_get_priv(dev);

	writel(MHMAC_MID_PROT_EN, cfg->base + MHMAC_REG_MID_OFFSET);
	writel(MHMAC_REG_MID_PROT_EN, cfg->base + MHMAC_REG_MID_OFFSET);

	return 0;
}

/********************************************************************
 *                      MHMAC export callback                       *
 ********************************************************************/
static int aspeed_mhmac_probe(struct udevice *dev)
{
	int i = 0;
	int ret = 0;
	uint32_t reg_val = 0;
	struct aspeed_mhmac_region *region = NULL;
	struct aspeed_mhmac_config *cfg = dev_get_priv(dev);

	/* SBE30C will block if hace clock disable */
	if (clk_get_by_index(dev, 0, &cfg->hace_clk) < 0 ||
	    clk_enable(&cfg->hace_clk))
		return -ENODEV;

	/* SBE30C will block if hace reset disable */
	if (reset_get_by_index(dev, 0, &cfg->hace_rst) ||
	    reset_deassert(&cfg->hace_rst))
		return -ENODEV;

	/* Init mhmac configure register */
	for (i = MHMAC_CTRL_OFFSET; i < MHMAC_ZONE0_KEY_OFFSET; i += 4)
		writel(0x0, cfg->base + i);

	/* Configure the region number */
	reg_val =
		FIELD_PREP(MHMAC_CTRL_REGION_NUMBER_SEL_MASK, cfg->region_sel);
	writel(reg_val, cfg->base + MHMAC_CTRL_OFFSET);

	/* Restrict configuration of MHMAC to bootMCU only */
	aspeed_mhmac_lock(dev);

	/* Initialize mhmac 0 ~ 3 region */
	for (i = 0; i < MHMAX_MAX_ZONE_NUMBER(cfg->region_sel); i++) {
		region = aspeed_mhmac_get_region(dev, i);
		if (region) {
			region->mhmac_size =
				DRAM_SIZE_TO_MHMAC_SIZE(region->dram_size);
			region->mhmac_size = rounddown(region->mhmac_size,
						       MHMAC_ZONE_ALIGN_SIZE);
			ret |= aspeed_mhmac_region_setting(dev, region);
		}
	}

	return ret;
}

static int aspeed_mhmac_write(struct udevice *dev, int offset, const void *buf,
			      int size)
{
	uint8_t region_num = offset >> 28;
	uint32_t region_offset = offset & GENMASK(27, 0);
	struct aspeed_mhmac_config *cfg = dev_get_priv(dev);
	struct aspeed_mhmac_region *region = NULL;

	/*
	 * offset 0x0000_0000 ~ 0x0FFF_FFFF is the range of region 0
	 * offset 0x1000_0000 ~ 0x1FFF_FFFF is the range of region 1
	 * offset 0x2000_0000 ~ 0x2FFF_FFFF is the range of region 2
	 * offset 0x3000_0000 ~ 0x3FFF_FFFF is the range of region 3
	 */

	region = aspeed_mhmac_get_region(dev, region_num);
	if (!region)
		return -ENODEV;

	if (!region->mhmac_base || !region->mhmac_size)
		return -ENOMEM;

	aspeed_mhmac_memcpy(region->mhmac_base + region_offset, buf, size);
	aspeed_mhmac_flush_cache(cfg);

	return 0;
}

static int aspeed_mhmac_of_to_plat(struct udevice *dev)
{
	struct aspeed_mhmac_config *cfg = dev_get_priv(dev);

	/* Get mhmac base address */
	cfg->base = (void *)dev_read_addr_index(dev, 0);
	cfg->scu_base =
		(void *)aspeed_mhmac_dtb_read_addr(dev, NULL, "aspeed,scu1");

	/* If get fail region_num = 0 for disabling mhmac*/
	cfg->region_sel = dev_read_u32_default(dev, "region-sel", 0);

	/* Get the mhmac regions configuration from dts */
	aspeed_mhmac_config_dtb_region(dev, 0);
	aspeed_mhmac_config_dtb_region(dev, 1);
	aspeed_mhmac_config_dtb_region(dev, 2);
	aspeed_mhmac_config_dtb_region(dev, 3);

	aspeed_mhmac_debug_cfg(dev);

	return 0;
}

static const struct udevice_id aspeed_mhmac_ids[] = {
	{ .compatible = "aspeed,ast2700-mhmac" },
	{}
};

static const struct misc_ops aspeed_mhmac_ops = {
	.write = aspeed_mhmac_write,
};

U_BOOT_DRIVER(aspeed_mhmac) = {
	.name = "aspeed_mhmac",
	.id = UCLASS_MISC,
	.of_match = aspeed_mhmac_ids,
	.probe = aspeed_mhmac_probe,
	.of_to_plat = aspeed_mhmac_of_to_plat,
	.priv_auto = sizeof(struct aspeed_mhmac_config),
	.ops = &aspeed_mhmac_ops,
};
