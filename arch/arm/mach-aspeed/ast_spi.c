// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */

#include <common.h>
#include <linux/types.h>

__weak u32 aspeed_spi_abr_offset(void)
{
	return 0;
}
