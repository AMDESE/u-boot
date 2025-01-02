/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) Aspeed Technology Inc.
 */
#ifndef _ASM_ARCH_STOR_AST2700_H

int stor_init(void);
int stor_copy(u32 *src, u32 *dest, u32 len);

#endif
