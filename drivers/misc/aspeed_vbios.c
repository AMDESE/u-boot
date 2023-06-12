// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include <linux/bitfield.h>
#include <common.h>
#include <clk.h>
#include <dm.h>
#include <errno.h>
#include <reset.h>
#include <fdtdec.h>
#include <asm/io.h>
#include "ast_vbios.h"

struct aspeed_vbios_priv {
	void *pcie_ctl_base;
	void *e2m0_ctl_base;
	void *scu_ctl_base;
	void *mem_ctl_base;
};

static int aspeed_vbios_probe(struct udevice *dev)
{
	struct aspeed_vbios_priv *vbios = dev_get_priv(dev);
	u32 value = (uintptr_t)(&uefi);

	/* Get the controller base address */
	vbios->pcie_ctl_base = (void *)devfdt_get_addr_index(dev, 0);
	vbios->e2m0_ctl_base = (void *)devfdt_get_addr_index(dev, 1);
	vbios->scu_ctl_base = (void *)devfdt_get_addr_index(dev, 2);
	vbios->mem_ctl_base = (void *)devfdt_get_addr_index(dev, 3);

	/* Set PCIE controller gen 1. Ref PLDM document. Need to filed every bit.*/
	writel(0x800254, vbios->pcie_ctl_base + 0x60);
	/* Set Frame buffer 32MB on 0x41e000000 */
	writel(0x01e0000e, vbios->e2m0_ctl_base);
	/* SCU framebuffer setting */
	writel(0x01e0000e, vbios->scu_ctl_base + 0x0c);
	/* (FPGA only) FPGA timing. */
	writel(0x11501a02, vbios->pcie_ctl_base);
	/* MEM ctrl */
	writel(0x400, vbios->mem_ctl_base + 0x4);

	/*Load UEFI*/
	value = (value >> 4) | 0x04;

	writel(value, vbios->e2m0_ctl_base + 0x4);
	writel(value, vbios->scu_ctl_base + 0x02c);

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
	//.ofdata_to_platdata   = vbios_aspeed_ofdata_to_platdata,
	//.priv_auto_alloc_size = sizeof(struct aspeed_vbios_priv),
};
