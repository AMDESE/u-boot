// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */
#include "platform.h"
#include "internal.h"
#include "pktgen.h"
#include "checksum.h"

u8 pkt_bufs[PKT_PER_TEST * 2][ROUNDUP_DMA_SIZE(MAX_PACKET_SIZE)] DMA_ALIGNED;
u8 broadcast_address[ETH_SIZE_DA] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

void dump(void *buf, size_t len)
{
	u8 *data = (u8 *)buf;
	size_t i;

	printf("\n");

	for (i = 0; i < len; i++) {
		printf("%02X ", data[i]);
		if ((i % 8) == 7)
			printf("\n");
	}
	if (((i - 1) % 8) != 7)
		printf("\n");
}

void dump_le16(void *buf, size_t len)
{
	u16 *data = (u16 *)buf;
	size_t i;

	len = (len + 1) / 2;
	for (i = 0; i < len; i++) {
		printf("%04X ", data[i]);
		if ((i % 8) == 7)
			printf("\n");
	}
	if ((i % 8) != 7)
		printf("\n");
}

u16 checksum(void *buf, u32 size, u16 checksum)
{
	u32 cks = checksum;
	u32 i;
	u16 *data = (u16 *)buf;
	u32 odd = size % 2;

	size /= 2;
	for (i = 0; i < size; i++)
		cks += data[i];
	/* Add the last byte */
	if (odd)
		cks += data[i] & 0xFF;

	while (cks > 0xFFFF)
		cks = (cks & 0xFFFF) + ((cks >> 16) & 0xFFFF);

	return ~cks;
}

u16 checksum_ipv4_pseudo(struct ip_hdr *iphdr)
{
	struct ipc4_pseudo_hdr phdr = { 0 };
	u32 total_len = ntohs(iphdr->total_len);

	MEMCPY(&phdr.sa, &iphdr->sa, 4);
	MEMCPY(&phdr.da, &iphdr->da, 4);
	phdr.protocol = iphdr->protocol;
	total_len -= (iphdr->ihl * 4);
	phdr.total_len = htons(total_len);

	return ~checksum(&phdr, sizeof(phdr), 0);
}

u16 checksum_ipv6_pseudo(struct ipv6_hdr *ipv6hdr)
{
	struct ipc6_pseudo_hdr phdr = { 0 };

	MEMCPY(phdr.sa, ipv6hdr->sa, 16);
	MEMCPY(phdr.da, ipv6hdr->da, 16);
	phdr.protocol = ipv6hdr->next_hdr;
	phdr.total_len = ipv6hdr->payload_len;

	return ~checksum(&phdr, sizeof(phdr), 0);
}

u32 _ethernet_header(u8 *packet, u8 *da, u8 *sa, u16 ethtype)
{
	struct eth_hdr *hdr = (struct eth_hdr *)packet;

	MEMCPY(hdr->da, da, ETH_SIZE_DA);
	MEMCPY(hdr->sa, sa, ETH_SIZE_SA);
	hdr->ethtype = htons(ethtype);

	return ETH_SIZE_HEADER;
}

u32 _ethernet_vlan_header(u8 *packet, u8 *da, u8 *sa, u16 tci)
{
	struct vlan_ethhdr *hdr = (struct vlan_ethhdr *)packet;
	u16 eth_type = ETHTYPE_VLAN;//get_ticks() & 0xFFFF;

	MEMCPY(hdr->da, da, ETH_SIZE_DA);
	MEMCPY(hdr->sa, sa, ETH_SIZE_SA);
	hdr->vlan_proto = htons(ETHTYPE_VLAN);
	hdr->vlan_TCI = htons(tci);
	hdr->vlan_encapsulated_proto = htons(eth_type);

	return ETH_VLAN_SIZE_HEADER;
}

u32 _ip_header(u8 *packet, u32 ip_sa, u32 ip_da, u8 protocol, u16 packet_size, u16 header_size)
{
	struct ip_hdr *hdr = (struct ip_hdr *)packet;

	packet_size = packet_size - ETH_SIZE_HEADER;

	hdr->version = 0x4;
	hdr->ihl = header_size / 4; // ihl * 32-increments = header_size Bytes
	hdr->tos = 0;
	hdr->total_len = htons(packet_size);
	hdr->id = 0x55AA;
	hdr->fragment_offset = htons(0x4000); // Don't fragment
	hdr->ttl = 64;
	hdr->protocol = protocol;
	hdr->checksum = 0;
	ip_sa = htonl(ip_sa);
	MEMCPY(&hdr->sa, &ip_sa, 4);
	ip_da = htonl(ip_da);
	MEMCPY(&hdr->da, &ip_da, 4);

	return sizeof(*hdr);
}

u32 _ipv6_header(u8 *packet, u8 next_hdr, u16 packet_size)
{
	struct ipv6_hdr *hdr = (struct ipv6_hdr *)packet;

	packet_size = packet_size - ETH_SIZE_HEADER - sizeof(*hdr);

	hdr->version = 0x6;
	hdr->priority = 0;
	hdr->flow_lbl[0] = 0;
	hdr->flow_lbl[1] = 0;
	hdr->flow_lbl[2] = 0;
	hdr->payload_len = htons(packet_size);
	hdr->next_hdr = next_hdr;
	hdr->hop_limit = 0;
	memset(hdr->sa, get_ticks(), 16);
	memset(hdr->da, get_ticks(), 16);

	return sizeof(*hdr);
}

u32 _tcp_header(u8 *packet, u16 sp, u16 dp, u16 header_size)
{
	struct tcp_hdr *hdr = (struct tcp_hdr *)packet;

	hdr->sp = htons(sp);
	hdr->dp = htons(dp);
	hdr->seq = 0x1234;
	hdr->ack_seq = 0xABCD;
	hdr->res1 = 0;
	hdr->doff = header_size / 4; // doff * 32-increments = header_size Bytes
	hdr->flag = 0x55;
	hdr->window = 0x5678;
	hdr->checksum = 0;
	hdr->urg_ptr = 0;

	return sizeof(*hdr);
}

u32 _udp_header(u8 *packet, u16 sp, u16 dp, u16 total_len)
{
	struct udp_hdr *hdr = (struct udp_hdr *)packet;

	hdr->sp = htons(sp);
	hdr->dp = htons(dp);
	hdr->total_len = htons(total_len);
	hdr->checksum = 0;

	return sizeof(*hdr);
}

void _payload(u8 *packet, u32 payload_size, u8 mode)
{
	u32 i;

	mode = mode % 5;

	switch (mode) {
	case PAYLOAD_INC:
		for (i = 0; i < payload_size; i++)
			*packet++ = i;
		break;
	case PAYLOAD_RANDOM:
		for (i = 0; i < payload_size; i++)
			*packet++ = (get_ticks() & 0xFF) + 1;
		break;
	case PAYLOAD_55:
		memset(packet, 0x55, payload_size);
		break;
	case PAYLOAD_AA:
		memset(packet, 0xAA, payload_size);
		break;
	case PAYLOAD_00:
		memset(packet, 0, payload_size);
		break;
	}
}

void generate_vlan_pakcet(struct test_s *test_obj, u32 packet_size, bool include_vlan)
{
	struct mac_s *mac_obj = test_obj->mac_obj;
	u32 i;

	for (i = 0; i < mac_obj->n_txdes; i++) {
		u8 *packet = test_obj->tx_pkt_buf[i];
		u32 payload_size;

		test_obj->vlan.tci[i] = get_ticks() & 0xFFFF;

		if (include_vlan)
			packet += _ethernet_vlan_header(packet, broadcast_address,
							mac_obj->mac_addr, test_obj->vlan.tci[i]);
		else
			packet += _ethernet_header(packet, broadcast_address, mac_obj->mac_addr,
						   get_ticks() & 0xFFFF);
		payload_size = packet_size - (packet - test_obj->tx_pkt_buf[i]);
		_payload(packet, payload_size, i);
	}
}

void generate_ip_pakcet(struct test_s *test_obj, u32 packet_size, bool need_cks)
{
	struct mac_s *mac_obj = test_obj->mac_obj;
	u16 header_size = (get_ticks() % 40) + 20; /* MAX/MIN IP hdr size = 60/20 */
	u32 i;

	if ((header_size + ETH_SIZE_HEADER) > packet_size)
		header_size = packet_size - ETH_SIZE_HEADER;

	for (i = 0; i < mac_obj->n_txdes; i++) {
		struct ip_hdr *ip;
		u8 *packet = test_obj->tx_pkt_buf[i];
		u32 ip_sa = get_ticks() & 0xFFFFFFFF;
		u32 ip_da = get_ticks() & 0xFFFFFFFF;
		u32 payload_size;

		packet += _ethernet_header(packet, broadcast_address, mac_obj->mac_addr,
					   ETHTYPE_IPV4);
		ip = (struct ip_hdr *)packet;
		packet += _ip_header(packet, ip_sa, ip_da, 0, packet_size, header_size);
		payload_size = packet_size - (packet - test_obj->tx_pkt_buf[i]);
		_payload(packet, payload_size, PAYLOAD_00);

		ip->checksum = checksum(ip, (ip->ihl * 4), 0);
		if (!need_cks)
			ip->checksum += 1;
	}
}

u16 tcp_udp_ipv4_checksum(u8 *buf, u32 packet_size)
{
	struct ip_hdr *ip;
	u16 ip_hdr_size;
	u32 checksum_len;
	u16 cks;

	/* IPv4 Pesudo Header Checksum*/
	ip = (struct ip_hdr *)&buf[IP_HDR_OFFSET];
	cks = checksum_ipv4_pseudo(ip);

	/* TCP/UDP + payload len*/
	ip_hdr_size = (buf[IP_HDR_OFFSET] & 0x0F) * 4;
	checksum_len = packet_size - ETH_SIZE_HEADER - ip_hdr_size;
	/* TCP/UDP + payload checksum*/
	cks = checksum(&buf[IP_HDR_OFFSET + ip_hdr_size], checksum_len, cks);

	return cks;
}

u16 tcp_udp_ipv6_checksum(u8 *buf, u32 packet_size)
{
	struct ipv6_hdr *ipv6;
	u32 checksum_len;
	u16 cks;

	/* IPv6 Pesudo Header Checksum*/
	ipv6 = (struct ipv6_hdr *)&buf[IP_HDR_OFFSET];
	cks = checksum_ipv6_pseudo(ipv6);

	/* TCP/UDP + payload len*/
	checksum_len = ntohs(ipv6->payload_len);
	/* TCP/UDP + payload checksum*/
	cks = checksum(&buf[IP_HDR_OFFSET + sizeof(struct ipv6_hdr)], checksum_len, cks);

	return cks;
}

void generate_tcp_ipv4_pakcet(struct test_s *test_obj, u32 packet_size, bool need_cks)
{
	struct mac_s *mac_obj = test_obj->mac_obj;
	u32 i;

	debug("%s - need_cks: %d\n", __func__, need_cks);

	for (i = 0; i < mac_obj->n_txdes; i++) {
		struct tcp_hdr *tcp;
		u8 *packet = test_obj->tx_pkt_buf[i];
		u32 ip_sa = get_ticks() & 0xFFFFFFFF;
		u32 ip_da = get_ticks() & 0xFFFFFFFF;
		u16 sp = get_ticks() & 0xFFFF;
		u16 dp = get_ticks() & 0xFFFF;
		u32 payload_size;

		packet += _ethernet_header(packet, broadcast_address, (u8 *)&packet_size, ETHTYPE_IPV4);
		packet += _ip_header(packet, ip_sa, ip_da, IP_PROTOCOL_TCP, packet_size,
				     sizeof(struct ip_hdr));
		tcp = (struct tcp_hdr *)packet;

		packet += _tcp_header(packet, sp, dp, sizeof(struct tcp_hdr));
		payload_size = packet_size - (packet - test_obj->tx_pkt_buf[i]);
		_payload(packet, payload_size, i);

		tcp->checksum = tcp_udp_ipv4_checksum(test_obj->tx_pkt_buf[i], packet_size);
		/* For Error case */
		if (!need_cks)
			tcp->checksum += 1;
	}
}

void generate_tcp_ipv6_pakcet(struct test_s *test_obj, u32 packet_size, bool need_cks)
{
	struct mac_s *mac_obj = test_obj->mac_obj;
	u32 i;

	debug("%s - need_cks: %d\n", __func__, need_cks);

	for (i = 0; i < mac_obj->n_txdes; i++) {
		struct tcp_hdr *tcp;
		u8 *packet = test_obj->tx_pkt_buf[i];
		u16 sp = get_ticks() & 0xFFFF;
		u16 dp = get_ticks() & 0xFFFF;
		u32 payload_size;

		packet += _ethernet_header(packet, broadcast_address, mac_obj->mac_addr,
					   ETHTYPE_IPV6);
		packet += _ipv6_header(packet, IP_PROTOCOL_TCP, packet_size);
		tcp = (struct tcp_hdr *)packet;
		packet += _tcp_header(packet, sp, dp, sizeof(struct tcp_hdr));
		payload_size = packet_size - (packet - test_obj->tx_pkt_buf[i]);
		_payload(packet, payload_size, i);

		tcp->checksum = tcp_udp_ipv6_checksum(test_obj->tx_pkt_buf[i], packet_size);
		/* For Error case */
		if (!need_cks)
			tcp->checksum += 1;
	}
}

void generate_udp_ipv4_pakcet(struct test_s *test_obj, u32 packet_size, bool need_cks)
{
	struct mac_s *mac_obj = test_obj->mac_obj;
	u32 i;

	debug("%s - need_cks: %d\n", __func__, need_cks);

	for (i = 0; i < mac_obj->n_txdes; i++) {
		struct udp_hdr *udp;
		u8 *packet = test_obj->tx_pkt_buf[i];
		u32 ip_sa = get_ticks() & 0xFFFFFFFF;
		u32 ip_da = get_ticks() & 0xFFFFFFFF;
		u16 sp = get_ticks() & 0xFFFF;
		u16 dp = get_ticks() & 0xFFFF;
		u32 payload_size;

		packet += _ethernet_header(packet, broadcast_address, mac_obj->mac_addr,
					   ETHTYPE_IPV4);

		packet += _ip_header(packet, ip_sa, ip_da, IP_PROTOCOL_UDP, packet_size,
				     sizeof(struct ip_hdr));
		udp = (struct udp_hdr *)packet;

		/* total_len = udp hdr size + payload size */
		payload_size = packet_size - ETH_SIZE_HEADER - sizeof(struct ip_hdr);
		packet += _udp_header(packet, sp, dp, payload_size);
		payload_size = packet_size - (packet - test_obj->tx_pkt_buf[i]);
		_payload(packet, payload_size, i);

		udp->checksum = tcp_udp_ipv4_checksum(test_obj->tx_pkt_buf[i], packet_size);
		/* For Error case */
		if (!need_cks)
			udp->checksum += 1;
	}
}

void generate_udp_ipv6_pakcet(struct test_s *test_obj, u32 packet_size, bool need_cks)
{
	struct mac_s *mac_obj = test_obj->mac_obj;
	u32 i;

	debug("%s - need_cks: %d\n", __func__, need_cks);

	for (i = 0; i < mac_obj->n_txdes; i++) {
		struct udp_hdr *udp;
		u8 *packet = test_obj->tx_pkt_buf[i];
		u16 sp = get_ticks() & 0xFFFF;
		u16 dp = get_ticks() & 0xFFFF;
		u32 payload_size;

		packet += _ethernet_header(packet, broadcast_address, mac_obj->mac_addr,
					   ETHTYPE_IPV6);
		packet += _ipv6_header(packet, IP_PROTOCOL_UDP, packet_size);
		udp = (struct udp_hdr *)packet;

		/* total_len = udp hdr size + payload size */
		payload_size = packet_size - ETH_SIZE_HEADER - sizeof(struct ipv6_hdr);
		packet += _udp_header(packet, sp, dp, payload_size);
		payload_size = packet_size - (packet - test_obj->tx_pkt_buf[i]);
		_payload(packet, payload_size, i);

		udp->checksum = tcp_udp_ipv6_checksum(test_obj->tx_pkt_buf[i], packet_size);
		/* For Error case */
		if (!need_cks)
			udp->checksum += 1;
	}
}

void generate_ethernet_pakcet(struct test_s *test_obj, u32 packet_size, bool need_vlan)
{
	struct mac_s *mac_obj = test_obj->mac_obj;
	u32 i;

	debug("%s - need_vlan: %d\n", __func__, need_vlan);

	for (i = 0; i < mac_obj->n_txdes; i++) {
		u16 ethertype;
		u8 *packet = test_obj->tx_pkt_buf[i];

		if (need_vlan) {
			ethertype = ETHTYPE_VLAN;
		} else {
			ethertype = get_ticks() & 0xFFFF; // random ethertype
			if (ethertype == ETHTYPE_VLAN)
				ethertype++;
		}
		packet += _ethernet_header(packet, broadcast_address, mac_obj->mac_addr, ethertype);

		_payload(packet, packet_size - ETH_SIZE_HEADER, PAYLOAD_RANDOM);
	}
}
