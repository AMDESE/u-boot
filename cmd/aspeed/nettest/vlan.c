// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */
#include "platform.h"
#include "internal.h"
#include "vlan.h"
#include "pktgen.h"

int __memcmp(const void *s1, const void *s2, size_t n)
{
	const u8 *pos1 = s1;
	const u8 *pos2 = s2;
	size_t size = n;

	for (; n; --n) {
		if (*pos1 != *pos2) {
			printf("Failed at %ld\n", size - n);
			return *pos1 - *pos2;
		}
		++pos1;
		++pos2;
	}
	return 0;
}

int vlan_rx_check(struct test_s *test_obj, u32 packet_size, u32 item)
{
	struct mac_s *mac_obj = test_obj->mac_obj;
	u8 *tx_packet = test_obj->tx_pkt_buf[item];
	u8 *rx_packet = test_obj->rx_pkt_buf[item];
	int ret;

	// RX will remove VLAN
	packet_size -= ETH_SIZE_VLAN;

	/* Compare Ethernet header */
	ret = memcmp(tx_packet, rx_packet, ETH_SIZE_HEADER - ETH_SIZE_TYPE_LENG);
	if (ret) {
		printf("\nCompare eth header failed (0x%02X).\n", ret);
		return FAIL_ETH_HDR_COMPARE;
	}

	tx_packet += (ETH_SIZE_HEADER - ETH_SIZE_TYPE_LENG);
	rx_packet += (ETH_SIZE_HEADER - ETH_SIZE_TYPE_LENG);
	packet_size -= (ETH_SIZE_HEADER - ETH_SIZE_TYPE_LENG);

	if ((mac_obj->rxdes[item].rxdes1 & 0xFFFF) != test_obj->vlan.tci[item]) {
		printf("\nThe VLAN TCI is wrong. (0x%04X != 0x%04X).\n",
		       mac_obj->rxdes[item].rxdes1 & 0xFFFF, test_obj->vlan.tci[item]);
		return FAIL_ERR_VLAN_TCI;
	}
	tx_packet += ETH_SIZE_VLAN;

	ret = __memcmp(tx_packet, rx_packet, packet_size);
	if (ret) {
		printf("\nCompare payload failed (0x%02X).\n", ret);
		return FAIL_DATA_COMPARE;
	}

	return 0;
}

int vlan_tx_check(struct test_s *test_obj, u32 packet_size, u32 item)
{
	u8 *tx_packet = test_obj->tx_pkt_buf[item];
	u8 *rx_packet = test_obj->rx_pkt_buf[item];
	int ret;

	/* Compare Ethernet header */
	ret = memcmp(tx_packet, rx_packet, ETH_SIZE_HEADER - ETH_SIZE_TYPE_LENG);
	if (ret) {
		printf("\nCompare eth header failed (0x%02X).\n", ret);
		return FAIL_ETH_HDR_COMPARE;
	}

	tx_packet += (ETH_SIZE_HEADER - ETH_SIZE_TYPE_LENG);
	rx_packet += (ETH_SIZE_HEADER - ETH_SIZE_TYPE_LENG);
	packet_size -= (ETH_SIZE_HEADER - ETH_SIZE_TYPE_LENG);

	if (*((u16 *)rx_packet) != htons(ETHTYPE_VLAN)) {
		printf("\nThe VLAN protocol is wrong. (0x%04X).\n", *((u16 *)rx_packet));
		return FAIL_ERR_VLAN_PROTOCOL;
	}
	rx_packet += 2;
	if (*((u16 *)rx_packet) != htons(test_obj->vlan.tci[item])) {
		printf("\nThe VLAN TCI is wrong. (0x%04X != 0x%04X).\n", *((u16 *)rx_packet),
		       htons(test_obj->vlan.tci[item]));
		return FAIL_ERR_VLAN_TCI;
	}
	rx_packet += 2;
	ret = __memcmp(tx_packet, rx_packet, packet_size);
	if (ret) {
		printf("\nCompare payload failed (0x%02X).\n", ret);
		return FAIL_DATA_COMPARE;
	}

	return 0;
}

int vlan_check(struct test_s *test_obj, u32 packet_size, u32 item)
{
	if (test_obj->vlan.mode == NETDIAG_VLAN_TX)
		return vlan_tx_check(test_obj, packet_size, item);
	return vlan_rx_check(test_obj, packet_size, item);
}

void vlan_mac_txpkt_add(struct test_s *test_obj, int pakcet_size)
{
	int i;

	for (i = 0; i < test_obj->pkt_per_test; i++)
		aspeed_mac_txpkt_add(test_obj->mac_obj, test_obj->tx_pkt_buf[i], pakcet_size,
				     test_obj);
}

int vlan_run_test(struct test_s *test_obj, int packet_size)
{
	struct mac_s *mac_obj = test_obj->mac_obj;
	u32 rxlen;
	int ret, i;

	aspeed_mac_init_tx_desc(mac_obj);
	aspeed_mac_init_rx_desc(mac_obj);
	generate_vlan_pakcet(test_obj, packet_size, test_obj->vlan.mode == NETDIAG_VLAN_RX);
	vlan_mac_txpkt_add(test_obj, packet_size);

	DSB;
	aspeed_mac_xmit(mac_obj);
	for (i = 0; i < test_obj->pkt_per_test; i++) {
		ret = net_get_packet(mac_obj, (void **)&test_obj->rx_pkt_buf[i], &rxlen, 100);
		if (ret)
			goto fail;
		debug("RX pkt#%02x: length=%d addr=%p\n", i, rxlen, test_obj->rx_pkt_buf[i]);
		ret = vlan_check(test_obj, packet_size, i);
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

int vlan_run_tests(struct test_s *test_obj, int min_packet_size, int max_packet_size)
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
		ret = vlan_run_test(test_obj, packet_size);
		if (ret && test_obj->fail_stop)
			break;
		aspeed_reset_assert(mac_obj->device);
		printf("\b\b\b\b");
	}
	printf("\r");
	return ret;
}

void tx_vlan_enable(struct test_s *test_obj, bool enable)
{
	struct mac_s *mac_obj = test_obj->mac_obj;

	if (enable)
		mac_obj->txdes1 = TXDESC1_INS_VLAN;
	else
		mac_obj->txdes1 = 0;
}

void rx_vlan_enable(struct test_s *test_obj, bool enable)
{
	test_obj->mac_obj->rx_vlan_remove = enable;
}

int vlan_tx_test(struct test_s *test_obj)
{
	struct mac_s *mac_obj = test_obj->mac_obj;
	int ret;
	int min_packet_size = 60;
	int max_packet_size = mac_obj->mtu + ETH_SIZE_HEADER;

	printf("= Start TX VLAN Test\n");

	tx_vlan_enable(test_obj, true);
	rx_vlan_enable(test_obj, false);

	ret = vlan_run_tests(test_obj, min_packet_size, max_packet_size);
	if (ret)
		return ret;

	printf("PASS              ");
	printf("\n");
	return 0;
}

int vlan_rx_test(struct test_s *test_obj)
{
	struct mac_s *mac_obj = test_obj->mac_obj;
	int ret;
	int min_packet_size = 60 + ETH_SIZE_VLAN;
	int max_packet_size = mac_obj->mtu + ETH_SIZE_HEADER + ETH_SIZE_VLAN;

	printf("= Start RX VLAN Test\n");

	tx_vlan_enable(test_obj, false);
	rx_vlan_enable(test_obj, true);

	ret = vlan_run_tests(test_obj, min_packet_size, max_packet_size);
	if (ret)
		return ret;

	printf("PASS              ");
	printf("\n");
	return 0;
}

int vlan_offload_test(struct test_s *test_obj)
{
	struct mac_s *mac_obj = test_obj->mac_obj;
	int first_mode, last_mode;
	int result[NETDIAG_VLAN_ALL] = { 0xFF };
	int i, ret = 0;

	if (test_obj->vlan.mode == NETDIAG_VLAN_ALL) {
		first_mode = NETDIAG_VLAN_TX;
		last_mode = NETDIAG_VLAN_RX;
	} else {
		first_mode = test_obj->vlan.mode;
		last_mode = test_obj->vlan.mode;
	}

	printf("Test %d ~ %d\n", first_mode, last_mode);

	for (i = first_mode; i <= last_mode; i++) {
		test_obj->vlan.mode = i;
		printf("= Test mode: %d\n", test_obj->vlan.mode);
		if (i == NETDIAG_VLAN_TX)
			result[i] = vlan_tx_test(test_obj);
		else
			result[i] = vlan_rx_test(test_obj);
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
