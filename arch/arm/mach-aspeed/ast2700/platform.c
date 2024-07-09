// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include <dm.h>
#include <asm/arch-aspeed/pci_ast2700.h>
#include <asm/arch-aspeed/scu_ast2700.h>
#include <asm/arch-aspeed/spi.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <env.h>
#include <env_internal.h>

DECLARE_GLOBAL_DATA_PTR;

enum env_location env_get_location(enum env_operation op, int prio)
{
	enum env_location env_loc = ENVL_UNKNOWN;

	if (prio)
		return env_loc;

	if ((readl(ASPEED_IO_HW_STRAP1) & SCU_IO_HWSTRAP_EMMC))
		env_loc =  ENVL_MMC;
	else
		env_loc =  ENVL_SPI_FLASH;

	return env_loc;
}

int arch_misc_init(void)
{
	int nodeoffset;
	ofnode node;

	if (IS_ENABLED(CONFIG_ARCH_MISC_INIT)) {
		if ((readl(ASPEED_IO_HW_STRAP1) & SCU_IO_HWSTRAP_EMMC)) {
			env_set("boot_device", "mmc");
		} else {
			env_set("boot_device", "spi");
			spi_bootarg_config();
		}
	}

	/* find the offset of compatible node */
	nodeoffset = fdt_node_offset_by_compatible(gd->fdt_blob, -1,
						   "aspeed,ast2700-scu0");
	if (nodeoffset < 0) {
		printf("%s: failed to get aspeed,ast2700-scu0\n", __func__);
		return -ENODEV;
	}

	/* get ast2700-scu0 node */
	node = offset_to_ofnode(nodeoffset);

	pci_config((struct ast2700_soc0_scu *)ofnode_get_addr(node));

	return 0;
}
