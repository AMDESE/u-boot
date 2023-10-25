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
	void *vbios_base;
};

static int aspeed_vbios_probe(struct udevice *dev)
{
	struct aspeed_vbios_priv *vbios = dev_get_priv(dev);
	u32 value = (uintptr_t)(&uefi);
	u64 temp;

	/* Get the controller base address */
	temp = (uintptr_t)(vbios->vbios_base);

	memcpy((u32 *)vbios->vbios_base, uefi, sizeof(uefi));

	/* Set VBIOS 32KB into reserved buffer */
	value = (temp >> 4) | 0x04;

	/* Set VBIOS setting into e2m */
	writel(value, vbios->e2m0_ctl_base + 0x4);

	/* Set VBIOS setting into scu */
	writel(value, vbios->scu_ctl_base + 0x02c);

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
	nodeoff = fdt_path_offset(gd->fdt_blob, "/reserved-memory/vbios_base0");
	fdt_get_resource(gd->fdt_blob, nodeoff, "reg", 0, &res);
	vbios->vbios_base = (void *)res.start;
	if (IS_ERR(vbios->vbios_base)) {
		dev_err(dev, "can't obtain vbios_base\n");
		return PTR_ERR(vbios->vbios_base);
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
