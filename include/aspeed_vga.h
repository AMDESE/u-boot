/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) Aspeed Technology Inc.
 */

#ifndef _ASPEED_VGA_H_
#define _ASPEED_VGA_H_

/* This function is used to tell how many VGA nodes are available
 * in the system. The number of VGA nodes is determined by
 * the efuse value and the hardware strap settings.
 *
 * Parameters:
 *  is_pcie0_enable: Pointer to store the status of PCIE0 VGA node
 *  is_pcie1_enable: Pointer to store the status of PCIE1 VGA node
 *
 * returns:
 *  0: No VGA nodes available
 *  1: One VGA node available
 *  2: Two VGA nodes available
 */
int ast_vga_get_nodes(u8 *is_pcie0_enable, u8 *is_pcie1_enable);

/* Get control mask for graphics memory
 *
 * Parameters:
 *  @node: 0 or 1, to specify which node to query
 *
 * return: The mask for graphics memory control which enable VRAM address
 *	   offset.
 */
u32 ast_vga_get_gfm_ctrl(u8 node);

#endif
