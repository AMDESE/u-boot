/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef _NETDIAG_PKTGEN_H
#define _NETDIAG_PKTGEN_H

#define PAYLOAD_INC		0
#define PAYLOAD_RANDOM		1
#define PAYLOAD_55		2
#define PAYLOAD_AA		3
#define PAYLOAD_00		4

void generate_ip_pakcet(struct test_s *test_obj, u32 packet_size, bool need_cks);
void generate_tcp_ipv4_pakcet(struct test_s *test_obj, u32 packet_size, bool need_cks);
void generate_tcp_ipv6_pakcet(struct test_s *test_obj, u32 packet_size, bool need_cks);
void generate_udp_ipv4_pakcet(struct test_s *test_obj, u32 packet_size, bool need_cks);
void generate_udp_ipv6_pakcet(struct test_s *test_obj, u32 packet_size, bool need_cks);
void generate_ethernet_pakcet(struct test_s *test_obj, u32 packet_size, bool need_vlan);
u16 checksum(void *buf, u32 size, u16 checksum);
u16 tcp_udp_ipv4_checksum(u8 *buf, u32 packet_size);
u16 tcp_udp_ipv6_checksum(u8 *buf, u32 packet_size);
void dump(void *buf, size_t len);
void dump_le16(void *buf, size_t len);

#endif	/* _NETDIAG_PKTGEN_H */
