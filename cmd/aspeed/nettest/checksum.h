/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef _NETDIAG_CHECKSUM_H
#define _NETDIAG_CHECKSUM_H

#define NETDIAG_CKS_NONE	0	/* no checkcum test */
#define NETDIAG_CKS_TX_IP	1	/* TX IP checksum test */
#define NETDIAG_CKS_TX_TCP4	2	/* TX TCP IPv4 checksum test */
#define NETDIAG_CKS_TX_TCP6	3	/* TX TCP IPv6 checksum test */
#define NETDIAG_CKS_TX_UDP4	4	/* TX UDP IPv4 checksum test */
#define NETDIAG_CKS_TX_UDP6	5	/* TX UDP IPv6 checksum test */
#define NETDIAG_CKS_RX_IP	6	/* RX IP checksum test */
#define NETDIAG_CKS_RX_TCP4	7	/* RX TCP IPv4 checksum test */
#define NETDIAG_CKS_RX_TCP6	8	/* RX TCP IPv6 checksum test */
#define NETDIAG_CKS_RX_UDP4	9	/* RX UDP IPv4 checksum test */
#define NETDIAG_CKS_RX_UDP6	10	/* RX UDP IPv6 checksum test */
#define NETDIAG_CKS_TX_ALL	11	/* TX ALL checksum test */
#define NETDIAG_CKS_RX_ALL	12	/* RX ALL checksum test */
#define NETDIAG_CKS_ALL		13	/* ALL checksum test */
#define TX_DESC_IP_EN		BIT(19)
#define TX_DESC_UDP_EN		BIT(18)
#define TX_DESC_TCP_EN		BIT(17)

int checksum_offload_test(struct test_s *test_obj);

#endif	/* _NETDIAG_CHECKSUM_H */
