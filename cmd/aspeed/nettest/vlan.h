/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef _NETDIAG_VLAN_H
#define _NETDIAG_VLAN_H

#define NETDIAG_VLAN_NONE	0	/* no checkcum test */
#define NETDIAG_VLAN_TX		1	/* TX VLAN test */
#define NETDIAG_VLAN_RX		2	/* RX VLAN test */
#define NETDIAG_VLAN_ALL	3	/* ALL VLAN test */

int vlan_offload_test(struct test_s *test_obj);

#endif	/* _NETDIAG_CHECKSUM_H */
