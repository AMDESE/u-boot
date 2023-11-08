/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) Aspeed Technology Inc.
 */
#ifndef _ASM_ARCH_LTPI_AST2700_H
#define _ASM_ARCH_LTPI_AST2700_H

#define LTPI_LINK_MANAGE_ST		0x108
#define   LTPI_LINK_PARTNER_AST1700	BIT(24)

#define LTPI_ADDR_REMAP_REG0		0x124
#define   REMAP_ENTRY1			GENMASK(25, 16)
#define   REMAP_ENTRY0			GENMASK(9, 0)
#define LTPI_ADDR_REMAP_REG1		0x128
#define   REMAP_ENTRY3			GENMASK(25, 16)
#define   REMAP_ENTRY2			GENMASK(9, 0)

#define LTPI_REMOTE_AST1700_IOD_SPACE	(0x14000000 >> 26)
#endif
