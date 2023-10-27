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
#include <reset.h>
#include <regmap.h>
#include <syscon.h>
#include <fdtdec.h>
#include <asm/io.h>
#include "ast_vbios.h"

struct aspeed_vbios_priv {
	void *e2m0_ctl_base;
	void *scu_ctl_base;
	void *vbios0_base;
};

static int aspeed_vbios_probe(struct udevice *dev)
{
	struct aspeed_vbios_priv *vbios = dev_get_priv(dev);
	u32 vbios0_e2m_value;
	u64 vbios0_mem_base;
	u32 vbios0_mem_length;

	/* Get the controller base address */
	vbios0_mem_base = (uintptr_t)(vbios->vbios0_base);
	vbios0_mem_length = sizeof(uefi);

	memcpy((u32 *)vbios->vbios0_base, uefi, vbios0_mem_length);
	invalidate_dcache_range(vbios0_mem_base, vbios0_mem_base
	+ vbios0_mem_length);

	/* Set VBIOS 32KB into reserved buffer */
	vbios0_e2m_value = (vbios0_mem_base >> 4) | 0x04;

	/* Set VBIOS setting into e2m */
	writel(vbios0_e2m_value, vbios->e2m0_ctl_base + 0x4);

	/* Set VBIOS setting into scu */
	writel(vbios0_e2m_value, vbios->scu_ctl_base + 0x02c);

	return 0;
}

static int aspeed_vbios_of_to_plat(struct udevice *dev)
{
	struct aspeed_vbios_priv *vbios = dev_get_priv(dev);
	struct fdt_resource res;
	u32 nodeoff;
	/* Get the controller base address */

	vbios->e2m0_ctl_base = (void *)devfdt_get_addr_index(dev, 0);
	if (IS_ERR(vbios->e2m0_ctl_base)) {
		dev_err(dev, "can't allocate e2m0_ctl\n");
		return PTR_ERR(vbios->e2m0_ctl_base);
	}
	vbios->scu_ctl_base = (void *)devfdt_get_addr_index(dev, 1);
	if (IS_ERR(vbios->scu_ctl_base)) {
		dev_err(dev, "can't allocate scu_ctl\n");
		return PTR_ERR(vbios->scu_ctl_base);
	}
	nodeoff = fdt_path_offset(gd->fdt_blob, "/reserved-memory/pcie_vbios0");
	fdt_get_resource(gd->fdt_blob, nodeoff, "reg", 0, &res);
	vbios->vbios0_base = (void *)res.start;
	if (IS_ERR(vbios->vbios0_base)) {
		dev_err(dev, "can't obtain pcie_vbios0\n");
		return PTR_ERR(vbios->vbios0_base);
	}
	return 0;
}

static const struct udevice_id aspeed_vbios_ids[] = {
	{ .compatible = "aspeed,ast2700-vbios" },
	{ }
};

U_BOOT_DRIVER(aspeed_vbios) = {
	.name		= "aspeed_vbios",
	.id			= UCLASS_MISC,
	.of_match	= aspeed_vbios_ids,
	.probe		= aspeed_vbios_probe,
	.of_to_plat = aspeed_vbios_of_to_plat,
	.priv_auto  = sizeof(struct aspeed_vbios_priv),
};
