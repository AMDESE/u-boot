// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */
#include "platform.h"
#include "internal.h"
#include "checksum.h"
#include "pktgen.h"

void checksum_enable(struct test_s *test_obj, bool enable)
{
	struct mac_s *mac_obj = test_obj->mac_obj;
	u32 *txdes1 = &mac_obj->txdes1;

	*txdes1 = 0;
	if (!enable)
		return;

	switch (test_obj->checksum.mode) {
	case NETDIAG_CKS_TX_IP:
		*txdes1 |= TX_DESC_IP_EN;
		break;
	case NETDIAG_CKS_TX_TCP4:
	case NETDIAG_CKS_TX_TCP6:
		*txdes1 |= TX_DESC_TCP_EN;
		break;
	case NETDIAG_CKS_TX_UDP4:
	case NETDIAG_CKS_TX_UDP6:
		*txdes1 |= TX_DESC_UDP_EN;
		break;
	default:
		break;
	}
}

unsigned short checksum_get_tcp_cks_from_buf(u8 *buf)
{
	return ((struct tcp_hdr *)buf)->checksum;
}

unsigned short checksum_get_udp_cks_from_buf(u8 *buf)
{
	return ((struct udp_hdr *)buf)->checksum;
}

int checksum_check_tx_append(struct test_s *test_obj, u32 item)
{
	struct ip_hdr *ip_tx, *ip_rx;
	u16 tx_cks, rx_cks;
	u32 offset;

	ip_tx = (struct ip_hdr *)&test_obj->tx_pkt_buf[item][IP_HDR_OFFSET];
	ip_rx = (struct ip_hdr *)&test_obj->rx_pkt_buf[item][IP_HDR_OFFSET];
	switch (test_obj->checksum.mode) {
	case NETDIAG_CKS_TX_IP:
		if (ip_tx->checksum == ip_rx->checksum) {
			printf("TX checksum: 0x%04x, RX checksum: 0x%04x", ip_tx->checksum,
			       ip_rx->checksum);
			return FAIL_TX_NOT_APPEND_CHK;
		}
		return 0;
	case NETDIAG_CKS_TX_TCP4:
		offset = IP_HDR_OFFSET + ip_tx->ihl * 4;
		tx_cks = checksum_get_tcp_cks_from_buf(&test_obj->tx_pkt_buf[item][offset]);
		rx_cks = checksum_get_tcp_cks_from_buf(&test_obj->rx_pkt_buf[item][offset]);
		break;
	case NETDIAG_CKS_TX_TCP6:
		offset = IP_HDR_OFFSET + sizeof(struct ipv6_hdr);
		tx_cks = checksum_get_tcp_cks_from_buf(&test_obj->tx_pkt_buf[item][offset]);
		rx_cks = checksum_get_tcp_cks_from_buf(&test_obj->rx_pkt_buf[item][offset]);
		break;
	case NETDIAG_CKS_TX_UDP4:
		offset = IP_HDR_OFFSET + ip_tx->ihl * 4;
		tx_cks = checksum_get_udp_cks_from_buf(&test_obj->tx_pkt_buf[item][offset]);
		rx_cks = checksum_get_udp_cks_from_buf(&test_obj->rx_pkt_buf[item][offset]);
		break;
	case NETDIAG_CKS_TX_UDP6:
		offset = IP_HDR_OFFSET + sizeof(struct ipv6_hdr);
		tx_cks = checksum_get_udp_cks_from_buf(&test_obj->tx_pkt_buf[item][offset]);
		rx_cks = checksum_get_udp_cks_from_buf(&test_obj->rx_pkt_buf[item][offset]);
		break;
	}

	if (tx_cks == rx_cks)
		return FAIL_TX_NOT_APPEND_CHK;
	return 0;
}

void _debug_check_tx_append(struct test_s *test_obj, u32 item)
{
	struct ip_hdr *ip_tx, *ip_rx;
	struct tcp_hdr *tcp_tx, *tcp_rx;
	struct udp_hdr *udp_tx, *udp_rx;
	u32 offset;

	ip_tx = (struct ip_hdr *)&test_obj->tx_pkt_buf[item][IP_HDR_OFFSET];
	ip_rx = (struct ip_hdr *)&test_obj->rx_pkt_buf[item][IP_HDR_OFFSET];
	switch (test_obj->checksum.mode) {
	case NETDIAG_CKS_TX_IP:
		printf("TX C: 0x%04x, RX C: 0x%04X\n", ip_tx->checksum, ip_rx->checksum);
		break;
	case NETDIAG_CKS_TX_TCP4:
		offset = IP_HDR_OFFSET + ip_tx->ihl * 4;
		printf("offset: %d\n", offset);
		tcp_tx = (struct tcp_hdr *)&test_obj->tx_pkt_buf[item][offset];
		tcp_rx = (struct tcp_hdr *)&test_obj->rx_pkt_buf[item][offset];
		printf("TX C: 0x%04x, RX C: 0x%04X\n", tcp_tx->checksum, tcp_rx->checksum);
		break;
	case NETDIAG_CKS_TX_TCP6:
		offset = IP_HDR_OFFSET + sizeof(struct ipv6_hdr);
		printf("offset: %d\n", offset);
		tcp_tx = (struct tcp_hdr *)&test_obj->tx_pkt_buf[item][offset];
		tcp_rx = (struct tcp_hdr *)&test_obj->rx_pkt_buf[item][offset];
		printf("TX C: 0x%04x, RX C: 0x%04X\n", tcp_tx->checksum, tcp_rx->checksum);
		break;
	case NETDIAG_CKS_TX_UDP4:
		offset = IP_HDR_OFFSET + ip_tx->ihl * 4;
		printf("offset: %d\n", offset);
		udp_tx = (struct udp_hdr *)&test_obj->tx_pkt_buf[item][offset];
		udp_rx = (struct udp_hdr *)&test_obj->rx_pkt_buf[item][offset];
		printf("TX C: 0x%04x, RX C: 0x%04X\n", udp_tx->checksum, udp_rx->checksum);
		break;
	case NETDIAG_CKS_TX_UDP6:
		offset = IP_HDR_OFFSET + sizeof(struct ipv6_hdr);
		printf("offset: %d\n", offset);
		udp_tx = (struct udp_hdr *)&test_obj->tx_pkt_buf[item][offset];
		udp_rx = (struct udp_hdr *)&test_obj->rx_pkt_buf[item][offset];
		printf("TX C: 0x%04x, RX C: 0x%04X\n", udp_tx->checksum, udp_rx->checksum);
		break;
	}
}

int checksum_tx_check(struct test_s *test_obj, u32 packet_size, u32 item)
{
	u32 header_size;
	u16 cks;
	int ret;

	switch (test_obj->checksum.mode) {
	case NETDIAG_CKS_TX_IP:
		ret = checksum_check_tx_append(test_obj, item);
		if (ret)
			return FAIL_TX_NOT_APPEND_CHK;
		header_size = (test_obj->rx_pkt_buf[item][IP_HDR_OFFSET] & 0x0F) * 4;
		cks = checksum((u16 *)&test_obj->rx_pkt_buf[item][IP_HDR_OFFSET], header_size, 0);
		if (cks)
			return FAIL_CHECKSUM;
		break;
	case NETDIAG_CKS_TX_TCP4:
	case NETDIAG_CKS_TX_UDP4:
		ret = checksum_check_tx_append(test_obj, item);
		if (ret)
			return FAIL_TX_NOT_APPEND_CHK;
		cks = tcp_udp_ipv4_checksum(test_obj->rx_pkt_buf[item], packet_size);
		if (cks)
			return FAIL_CHECKSUM;
		break;
	case NETDIAG_CKS_TX_TCP6:
	case NETDIAG_CKS_TX_UDP6:
		ret = checksum_check_tx_append(test_obj, item);
		if (ret)
			return FAIL_TX_NOT_APPEND_CHK;
		cks = tcp_udp_ipv6_checksum(test_obj->rx_pkt_buf[item], packet_size);
		if (cks)
			return FAIL_CHECKSUM;
		break;
	default:
		return FAIL_GENERAL;
	}
	return 0;
}

int checksum_check_rx_desc(struct test_s *test_obj, u32 item, bool correct_case)
{
	struct mac_s *mac_obj = test_obj->mac_obj;
	u32 check_cks, check_protl;
	bool check_ipv6;

	switch (test_obj->checksum.mode) {
	case NETDIAG_CKS_RX_IP:
		check_cks = RXDESC1_IPCS_FAIL;
		check_protl = PROTOCOL_IP;
		check_ipv6 = false;
		break;
	case NETDIAG_CKS_RX_TCP4:
		check_cks = RXDESC1_TCPCS_FAIL;
		check_protl = PROTOCOL_TCP;
		check_ipv6 = false;
		break;
	case NETDIAG_CKS_RX_TCP6:
		check_cks = RXDESC1_TCPCS_FAIL;
		check_protl = PROTOCOL_TCP;
		check_ipv6 = true;
		break;
	case NETDIAG_CKS_RX_UDP4:
		check_cks = RXDESC1_UDPCS_FAIL;
		check_protl = PROTOCOL_UDP;
		check_ipv6 = false;
		break;
	case NETDIAG_CKS_RX_UDP6:
		check_cks = RXDESC1_UDPCS_FAIL;
		check_protl = PROTOCOL_UDP;
		check_ipv6 = true;
		break;
	default:
		return FAIL_GENERAL;
	}

	if (!(mac_obj->rxdes[item].rxdes1 & check_cks) != correct_case)
		return FAIL_CHECKSUM;
	if (FIELD_GET(RXDESC1_PROTL_TYPE, mac_obj->rxdes[item].rxdes1) != check_protl)
		return FAIL_PROTOCOL;
	if (!!(mac_obj->rxdes[item].rxdes1 & RXDESC1_IPV6) != check_ipv6)
		return FAIL_FLAG_IPV6;

	return 0;
}

int checksum_compare_packets(struct test_s *test_obj, u32 packet_size, u32 item, bool test_case)
{
	u32 header_size;
	u8 *tx_packet = test_obj->tx_pkt_buf[item];
	u8 *rx_packet = test_obj->rx_pkt_buf[item];
	int ret;

	if (test_obj->checksum.mode <= NETDIAG_CKS_TX_UDP6 && test_case) {
		/* Compare Ethernet header */
		ret = memcmp(tx_packet, rx_packet, ETH_SIZE_HEADER);
		if (ret) {
			printf("\nCompare eth header failed (0x%02X).\n", ret);
			return FAIL_ETH_HDR_COMPARE;
		}
		tx_packet += ETH_SIZE_HEADER;
		rx_packet += ETH_SIZE_HEADER;
		packet_size -= ETH_SIZE_HEADER;

		/* IP header size */
		if (test_obj->checksum.mode == NETDIAG_CKS_TX_IP ||
		    test_obj->checksum.mode == NETDIAG_CKS_TX_TCP4 ||
		    test_obj->checksum.mode == NETDIAG_CKS_TX_UDP4)
			header_size = (*tx_packet & 0x0F) * 4; // IPv4
		else
			header_size = sizeof(struct ipv6_hdr); // IPv6
		tx_packet += header_size;
		rx_packet += header_size;
		packet_size -= header_size;

		if (test_obj->checksum.mode == NETDIAG_CKS_TX_TCP4 ||
		    test_obj->checksum.mode == NETDIAG_CKS_TX_TCP6) {
			tx_packet += sizeof(struct tcp_hdr);
			rx_packet += sizeof(struct tcp_hdr);
			packet_size -= sizeof(struct tcp_hdr);
		} else if (test_obj->checksum.mode == NETDIAG_CKS_TX_UDP4 ||
			   test_obj->checksum.mode == NETDIAG_CKS_TX_UDP6) {
			tx_packet += sizeof(struct udp_hdr);
			rx_packet += sizeof(struct udp_hdr);
			packet_size -= sizeof(struct udp_hdr);
		}
	}
	ret = memcmp(tx_packet, rx_packet, packet_size);
	if (ret) {
		printf("\nCompare payload failed (0x%02X).\n", ret);
		return FAIL_DATA_COMPARE;
	}

	return 0;
}

int checksum_check(struct test_s *test_obj, u32 packet_size, u32 item, bool test_case)
{
	int ret;

	ret = checksum_compare_packets(test_obj, packet_size, item, test_case);
	if (ret)
		return ret;

	/* TX Test */
	if (test_obj->checksum.mode <= NETDIAG_CKS_TX_UDP6) {
		if (test_case)
			return checksum_tx_check(test_obj, packet_size, item);
		return 0;
	}

	/* RX Test */
	return checksum_check_rx_desc(test_obj, item, test_case);
}

void checksum_mac_txpkt_add(struct test_s *test_obj, int pakcet_size)
{
	int i;

	for (i = 0; i < test_obj->pkt_per_test; i++)
		aspeed_mac_txpkt_add(test_obj->mac_obj, test_obj->tx_pkt_buf[i], pakcet_size);
}

void checksum_generate_packet(struct test_s *test_obj, u32 packet_size, bool need_cks)
{
	switch (test_obj->checksum.mode) {
	case NETDIAG_CKS_TX_IP:
	case NETDIAG_CKS_RX_IP:
		generate_ip_pakcet(test_obj, packet_size, need_cks);
		break;
	case NETDIAG_CKS_TX_TCP4:
	case NETDIAG_CKS_RX_TCP4:
		generate_tcp_ipv4_pakcet(test_obj, packet_size, need_cks);
		break;
	case NETDIAG_CKS_TX_TCP6:
	case NETDIAG_CKS_RX_TCP6:
		generate_tcp_ipv6_pakcet(test_obj, packet_size, need_cks);
		break;
	case NETDIAG_CKS_TX_UDP4:
	case NETDIAG_CKS_RX_UDP4:
		generate_udp_ipv4_pakcet(test_obj, packet_size, need_cks);
		break;
	case NETDIAG_CKS_TX_UDP6:
	case NETDIAG_CKS_RX_UDP6:
		generate_udp_ipv6_pakcet(test_obj, packet_size, need_cks);
		break;
	}
}

int checksum_run_test(struct test_s *test_obj, int packet_size, bool test_case)
{
	struct mac_s *mac_obj = test_obj->mac_obj;
	u32 rxlen;
	int ret, i;

	aspeed_mac_init_tx_desc(mac_obj);
	aspeed_mac_init_rx_desc(mac_obj);
	if (test_obj->checksum.mode <= NETDIAG_CKS_TX_UDP6)
		checksum_generate_packet(test_obj, packet_size, false);
	else
		checksum_generate_packet(test_obj, packet_size, test_case);
	checksum_mac_txpkt_add(test_obj, packet_size);

	DSB;
	aspeed_mac_xmit(mac_obj);
	for (i = 0; i < test_obj->pkt_per_test; i++) {
		ret = net_get_packet(mac_obj, (void **)&test_obj->rx_pkt_buf[i], &rxlen, 100);
		if (ret)
			goto fail;
		debug("RX pkt#%02x: length=%d addr=%p\n", i, rxlen, test_obj->rx_pkt_buf[i]);
		ret = checksum_check(test_obj, packet_size, i, test_case);
		if (ret)
			if (test_obj->fail_stop)
				goto fail;
	}
	return 0;
fail:
	printf("\r");
	printf("FAIL              ");
	printf("\n");
	printf("(%d) pkt#%02x, length=%d\n", ret, i, packet_size);
	aspeed_mac_reg_dump(mac_obj);

	return ret;
}

int checksum_run_tests(struct test_s *test_obj, int min_packet_size, int max_packet_size,
		       bool test_case)
{
	struct mac_s *mac_obj = test_obj->mac_obj;
	int packet_size, ret = 0;

	printf("== Min/Max Packet Size: %d / %d\n", min_packet_size, max_packet_size);
	printf("Packet Size: ");
	for (packet_size = min_packet_size; packet_size <= max_packet_size; packet_size++) {
#if defined(U_BOOT)
		if (ctrlc()) {
			clear_ctrlc();
			return FAIL_CTRL_C;
		}
#endif
		printf("%04d", packet_size);
		aspeed_mac_init(mac_obj);
		aspeed_mac_set_loopback(mac_obj,
					test_obj->parm.control == NETDIAG_CTRL_LOOPBACK_MAC);
		aspeed_mac_set_sgmii_loopback(mac_obj,
					      test_obj->parm.control == NETDIAG_CTRL_LOOPBACK_MII);
		ret = checksum_run_test(test_obj, packet_size, test_case);
		if (ret && test_obj->fail_stop)
			break;
		aspeed_reset_assert(mac_obj->device);
		printf("\b\b\b\b");
	}
	printf("\r");
	return ret;
}

int checksum_tx_test(struct test_s *test_obj)
{
	struct mac_s *mac_obj = test_obj->mac_obj;
	int enable, ret;
	int min_packet_size = 60;
	int max_packet_size = mac_obj->mtu + ETH_SIZE_HEADER;

	printf("= Start TX Checksum Test\n");

	if (test_obj->checksum.mode == NETDIAG_CKS_TX_TCP6)
		/* 14(eth) + 40(ipv6) + 20(tcp) */
		min_packet_size = 74;
	else if (test_obj->checksum.mode == NETDIAG_CKS_TX_UDP6)
		/* 14(eth) + 40(ipv6) + 8(udp) */
		min_packet_size = 62;

	for (enable = 0; enable < 2; enable++) {
		printf("== %s Checksum Offload Test\n", (enable) ? "Enable" : "Disable");

		/* Enable/Disable TX Checksum offload */
		checksum_enable(test_obj, enable);

		if (enable)
			ret = checksum_run_tests(test_obj, min_packet_size, max_packet_size,
						 enable);
		else
			ret = checksum_run_tests(test_obj, min_packet_size, 512, enable);
		if (ret)
			break;
		printf("PASS              ");
		printf("\n");
	}

	return ret;
}

int checksum_rx_test(struct test_s *test_obj)
{
	struct mac_s *mac_obj = test_obj->mac_obj;
	int correct_case, ret;
	int min_packet_size = 60;
	int max_packet_size = mac_obj->mtu + ETH_SIZE_HEADER;

	printf("= Start RX Checksum Test\n");

	if (test_obj->checksum.mode == NETDIAG_CKS_RX_TCP6)
		/* 14(eth) + 40(ipv6) + 20(tcp) */
		min_packet_size = 74;
	else if (test_obj->checksum.mode == NETDIAG_CKS_RX_UDP6)
		/* 14(eth) + 40(ipv6) + 8(udp) */
		min_packet_size = 62;

	/* Disable TX Checksum offload. Let SW calculate the checksum */
	checksum_enable(test_obj, false);
	for (correct_case = 0; correct_case < 2; correct_case++) {
		printf("== %s Case Checksum Offload Test\n", (correct_case) ? "Correct" : "Fail");
		ret = checksum_run_tests(test_obj, min_packet_size, max_packet_size, correct_case);
		if (ret)
			break;
		printf("PASS              ");
		printf("\n");
	}

	return ret;
}

int checksum_offload_test(struct test_s *test_obj)
{
	struct mac_s *mac_obj = test_obj->mac_obj;
	int first_mode, last_mode;
	int result[NETDIAG_CKS_ALL] = { 0xFF };
	int i, ret = 0;

	if (test_obj->checksum.mode == NETDIAG_CKS_TX_ALL) {
		first_mode = NETDIAG_CKS_TX_IP;
		last_mode = NETDIAG_CKS_TX_UDP6;
	} else if (test_obj->checksum.mode == NETDIAG_CKS_RX_ALL) {
		first_mode = NETDIAG_CKS_RX_IP;
		last_mode = NETDIAG_CKS_RX_UDP6;
	} else if (test_obj->checksum.mode == NETDIAG_CKS_ALL) {
		first_mode = NETDIAG_CKS_TX_IP;
		last_mode = NETDIAG_CKS_RX_UDP6;
	} else if (test_obj->checksum.mode <= NETDIAG_CKS_TX_UDP6) {
		first_mode = test_obj->checksum.mode;
		last_mode = test_obj->checksum.mode;
	} else {
		first_mode = test_obj->checksum.mode;
		last_mode = test_obj->checksum.mode;
	}

	printf("Test %d ~ %d\n", first_mode, last_mode);

	for (i = first_mode; i <= last_mode; i++) {
		test_obj->checksum.mode = i;
		printf("= Test mode: %d\n", test_obj->checksum.mode);
		if (i >= NETDIAG_CKS_RX_IP)
			result[i] = checksum_rx_test(test_obj);
		else
			result[i] = checksum_tx_test(test_obj);
		if (result[i] == FAIL_CTRL_C) {
			ret = FAIL_CTRL_C;
			goto out;
		}
	}

	printf("Result:\n");
	for (i = first_mode; i <= last_mode; i++) {
		printf("Mode %02d: ", i);
		if (result[i]) {
			printf("X (%d)\n", result[i]);
			ret = result[i];
		} else {
			printf("O\n");
		}
	}
out:
	aspeed_reset_deassert(mac_obj->device);
	aspeed_mac_set_loopback(mac_obj, false);
	aspeed_mac_set_sgmii_loopback(mac_obj, false);

	return ret;
}
