/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) Aspeed Technology Inc.
 */
#ifndef _ASM_ARCH_UFS_AST2700_H
#define _ASM_ARCH_UFS_AST2700_H

int ufs_init(void);
int ufs_load_image(u32 *src, u32 *dest, u32 len);

#endif
