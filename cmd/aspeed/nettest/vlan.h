/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef _NETDIAG_VLAN_H
#define _NETDIAG_VLAN_H

#define NETDIAG_VLAN_NONE	0	/* no checkcum test */
#define NETDIAG_VLAN_TX		1	/* TX VLAN test */
#define NETDIAG_VLAN_RX		2	/* RX VLAN test (Random ethertype) */
#define NETDIAG_VLAN_RX_QINQ	3	/* RX VLAN test (QinQ: 0x8100)*/
#define NETDIAG_VLAN_TX_ALL	4
#define NETDIAG_VLAN_RX_ALL	5
#define NETDIAG_VLAN_ALL	6	/* ALL VLAN test */

int vlan_offload_test(struct test_s *test_obj);

#endif	/* _NETDIAG_CHECKSUM_H */
