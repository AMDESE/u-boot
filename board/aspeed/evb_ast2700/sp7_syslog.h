/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) AMD Inc.
 *
 * AMD SP7 board-local UDP syslog console replay.
 *
 */

#ifndef __SP7_SYSLOG_H
#define __SP7_SYSLOG_H

#include <linux/kconfig.h>

#if CONFIG_IS_ENABLED(CONSOLE_RECORD)
void sp7_syslog_enable_console_record(void);
#else
static inline void sp7_syslog_enable_console_record(void) { }
#endif

#endif /* __SP7_SYSLOG_H */
