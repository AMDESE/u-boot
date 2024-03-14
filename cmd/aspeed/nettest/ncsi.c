// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */
#include "platform.h"
#include "internal.h"
#include "ncsi.h"

struct ncsi_pkt_hdr {
	unsigned char mc_id;        /* Management controller ID */
	unsigned char revision;     /* NCSI version - 0x01      */
	unsigned char reserved;     /* Reserved                 */
	unsigned char id;           /* Packet sequence number   */
	unsigned char type;         /* Packet type              */
	unsigned char channel;      /* Network controller ID    */
	__be16        length;       /* Payload length           */
	__be32        reserved1[2]; /* Reserved                 */
};

struct ncsi_cmd_pkt_hdr {
	struct ncsi_pkt_hdr common; /* Common NCSI packet header */
};

struct ncsi_rsp_pkt_hdr {
	struct ncsi_pkt_hdr common; /* Common NCSI packet header */
	__be16              code;   /* Response code             */
	__be16              reason; /* Response reason           */
};

struct ncsi_rsp_pkt {
	struct ncsi_rsp_pkt_hdr rsp;      /* Response header */
	__be32                  checksum; /* Checksum        */
	unsigned char           pad[22];
};

/* Get Version ID */
struct ncsi_rsp_gvi_pkt {
	struct ncsi_rsp_pkt_hdr rsp;          /* Response header */
	__be32                  ncsi_version; /* NCSI version    */
	unsigned char           reserved[3];  /* Reserved        */
	unsigned char           alpha2;       /* NCSI version    */
	unsigned char           fw_name[12];  /* f/w name string */
	__be32                  fw_version;   /* f/w version     */
	__be16                  pci_ids[4];   /* PCI IDs         */
	__be32                  mf_id;        /* Manufacture ID  */
	__be32                  checksum;
};

/* Set MAC Address */
struct ncsi_cmd_sma_pkt {
	struct ncsi_cmd_pkt_hdr cmd;      /* Command header          */
	unsigned char           mac[6];   /* MAC address             */
	unsigned char           index;    /* MAC table index         */
	unsigned char           at_e;     /* Addr type and operation */
	__be32                  checksum; /* Checksum                */
	unsigned char           pad[18];
};

struct {
	u16 vid;
	u16 did;
	char description[32];
} cards[] = {
	{ PCI_VID_INTEL_8086, 0x10d3, "Intel 82574L" },
	{ PCI_VID_INTEL_8086, 0x10d6, "Intel 82566" },
	{ PCI_VID_INTEL_8086, 0x10a7, "Intel 82575EB" },
	{ PCI_VID_INTEL_8086, 0x10a9, "Intel 82575EB" },
	{ PCI_VID_INTEL_8086, 0x10C9, "Intel 82576" },
	{ PCI_VID_INTEL_8086, 0x10E6, "Intel 82576" },
	{ PCI_VID_INTEL_8086, 0x10E7, "Intel 82576" },
	{ PCI_VID_INTEL_8086, 0x10E8, "Intel 82576" },
	{ PCI_VID_INTEL_8086, 0x150D, "Intel 82576" },
	{ PCI_VID_INTEL_8086, 0x1518, "Intel 82576NS" },
	{ PCI_VID_INTEL_8086, 0x150A, "Intel 82576NS" },
	{ PCI_VID_INTEL_8086, 0x1526, "Intel 82576 ET2" },
	{ PCI_VID_INTEL_8086, 0x10FB, "Intel 82599" },
	{ PCI_VID_INTEL_8086, 0x1557, "Intel 82599EN" },
	{ PCI_VID_INTEL_8086, 0x1533, "Intel I210" },
	{ PCI_VID_INTEL_8086, 0x1537, "Intel I210" },
	{ PCI_VID_INTEL_8086, 0x1521, "Intel I350" },
	{ PCI_VID_INTEL_8086, 0x1523, "Intel I350" },
	{ PCI_VID_INTEL_8086, 0x1528, "Intel X540" },
	{ PCI_VID_INTEL_8086, 0x1563, "Intel X550" },
	{ PCI_VID_INTEL_8086, 0x15AB, "Intel Broadwell-DE" },
	{ PCI_VID_INTEL_8086, 0x15FF, "Intel X710" },
	{ PCI_VID_INTEL_8086, 0x37D0, "Intel X722" },
	{ PCI_VID_BROADCOM, 0x1656, "Broadcom BCM5718" },
	{ PCI_VID_BROADCOM, 0x1657, "Broadcom BCM5719" },
	{ PCI_VID_BROADCOM, 0x165F, "Broadcom BCM5720" },
	{ PCI_VID_BROADCOM, 0x1643, "Broadcom BCM5725" },
	{ PCI_VID_BROADCOM, 0x168E, "Broadcom BCM57810S" },
	{ PCI_VID_BROADCOM, 0x16CA, "Broadcom Cumulus" },
	{ PCI_VID_BROADCOM, 0x16C9, "Broadcom BCM57302" },
	{ PCI_VID_BROADCOM, 0x16D8, "Broadcom BCM57416" },
	{ PCI_VID_BROADCOM, 0x16F1, "Broadcom BCM957452" },
	{ PCI_VID_MELLANOX, 0x1003, "Mellanox ConnectX-3" },
	{ PCI_VID_MELLANOX, 0x1007, "Mellanox ConnectX-3" },
	{ PCI_VID_MELLANOX, 0x1015, "Mellanox ConnectX-4" },
	{ PCI_VID_EMULEX, 0x0720, "Emulex 40G" },
};

static unsigned int cmd_payload(int cmd)
{
	switch (cmd) {
	case NCSI_PKT_CMD_CIS:
		return 0;
	case NCSI_PKT_CMD_SP:
		return 4;
	case NCSI_PKT_CMD_DP:
		return 0;
	case NCSI_PKT_CMD_EC:
		return 0;
	case NCSI_PKT_CMD_DC:
		return 4;
	case NCSI_PKT_CMD_RC:
		return 4;
	case NCSI_PKT_CMD_ECNT:
		return 0;
	case NCSI_PKT_CMD_DCNT:
		return 0;
	case NCSI_PKT_CMD_AE:
		return 8;
	case NCSI_PKT_CMD_SL:
		return 8;
	case NCSI_PKT_CMD_GLS:
		return 0;
	case NCSI_PKT_CMD_SVF:
		return 8;
	case NCSI_PKT_CMD_EV:
		return 4;
	case NCSI_PKT_CMD_DV:
		return 0;
	case NCSI_PKT_CMD_SMA:
		return 8;
	case NCSI_PKT_CMD_EBF:
		return 4;
	case NCSI_PKT_CMD_DBF:
		return 0;
	case NCSI_PKT_CMD_EGMF:
		return 4;
	case NCSI_PKT_CMD_DGMF:
		return 0;
	case NCSI_PKT_CMD_SNFC:
		return 4;
	case NCSI_PKT_CMD_GVI:
		return 0;
	case NCSI_PKT_CMD_GC:
		return 0;
	case NCSI_PKT_CMD_GP:
		return 0;
	case NCSI_PKT_CMD_GCPS:
		return 0;
	case NCSI_PKT_CMD_GNS:
		return 0;
	case NCSI_PKT_CMD_GNPTS:
		return 0;
	case NCSI_PKT_CMD_GPS:
		return 0;
	default:
		printf("NCSI: Unknown command 0x%02x\n", cmd);
		return 0;
	}
}

static void ncsi_update_channel_id(struct test_s *test_obj)
{
	test_obj->ncsi.current_channel++;
}

static u32 ncsi_calculate_checksum(unsigned char *data, int len)
{
	u32 checksum = 0;
	int i;

	for (i = 0; i < len; i += 2)
		checksum += (((u32)data[i] << 8) | data[i + 1]);

	checksum = (~checksum + 1);
	return checksum;
}

static int ncsi_send_command(struct test_s *test_obj, unsigned int np, unsigned int nc,
			     unsigned int cmd, u8 *payload)
{
	struct mac_s *mac_obj = test_obj->mac_obj;
	struct ncsi_pkt_hdr *hdr;
	u32 checksum;
	u8 *pkt;
	int final_len;
	int len = cmd_payload(cmd);

	pkt = test_obj->tx_pkt_buf[0];

	/* Ethernet Header */
	memset(pkt, 0xFF, ETH_SIZE_DA);
	pkt += ETH_SIZE_DA;
	MEMCPY(pkt, mac_obj->mac_addr, ETH_SIZE_SA);
	pkt += ETH_SIZE_SA;
	*((u16 *)pkt) = htons(ETHTYPE_NCSI);
	pkt += ETH_SIZE_TYPE_LENG;

	/* Set NCSI command header fields */
	hdr = (struct ncsi_pkt_hdr *)pkt;
	hdr->mc_id = 0;
	hdr->revision = NCSI_PKT_REVISION;
	if ((test_obj->ncsi.last_request & 0xFF) == 0xFF)
		test_obj->ncsi.last_request = 0;
	hdr->id = ++test_obj->ncsi.last_request;
	hdr->type = cmd;
	hdr->channel = NCSI_TO_CHANNEL(np, nc);
	hdr->length = htons(len);

	if (payload && len)
		MEMCPY(pkt + sizeof(struct ncsi_pkt_hdr), payload, len);

	/* Calculate checksum */
	checksum = ncsi_calculate_checksum((unsigned char *)hdr, sizeof(*hdr) + len);
	checksum = htonl(checksum);
	MEMCPY((void *)(hdr + 1) + len, &checksum, sizeof(checksum));

	if (len < 26)
		len = 26;
	/* frame header, packet header, payload, checksum */
	final_len = ETH_SIZE_HEADER + sizeof(struct ncsi_cmd_pkt_hdr) + len + 4;

	/* Set DESC 0 as the last desc */
	mac_obj->txdes[0].txdes0 = BIT(30);
	aspeed_mac_txpkt_add(mac_obj, test_obj->tx_pkt_buf[0], final_len, test_obj);

	DSB;
	aspeed_mac_xmit(mac_obj);

	return 0;
}

static void ncsi_send_sp(struct test_s *test_obj)
{
	u8 payload[4] = { SP_DISABLE_HW_ARB, 0, 0, 0 };

	ncsi_send_command(test_obj, test_obj->ncsi.current_package, NCSI_RESERVED_CHANNEL,
			  NCSI_PKT_CMD_SP, payload);
}

static void ncsi_send_dp(struct test_s *test_obj)
{
	ncsi_send_command(test_obj, test_obj->ncsi.current_package, NCSI_RESERVED_CHANNEL,
			  NCSI_PKT_CMD_DP, NULL);
}

static void ncsi_send_cis(struct test_s *test_obj)
{
	ncsi_send_command(test_obj, test_obj->ncsi.current_package, test_obj->ncsi.current_channel,
			  NCSI_PKT_CMD_CIS, NULL);
}

static void ncsi_send_gvi(struct test_s *test_obj)
{
	ncsi_send_command(test_obj, test_obj->ncsi.current_package,
			  test_obj->ncsi.current_channel, NCSI_PKT_CMD_GVI, NULL);
}

static void ncsi_probe_packages(struct test_s *test_obj)
{
	switch (test_obj->ncsi.state) {
	case NCSI_PROBE_PACKAGE_SP:
		ncsi_send_sp(test_obj);
		break;
	case NCSI_PROBE_PACKAGE_DP:
		ncsi_send_dp(test_obj);
		break;
	case NCSI_PROBE_CHANNEL:
		ncsi_send_cis(test_obj);
		break;
	case NCSI_GET_VERSION_ID:
		ncsi_send_gvi(test_obj);
		break;
	default:
		printf("NCSI: unknown state 0x%x\n", test_obj->ncsi.state);
	}
}

static int ncsi_add_list(struct test_s *test_obj)
{
	int i;

	for (i = 0; i < NCSI_LIST_MAX; i++) {
		if (test_obj->ncsi.info[i].package == test_obj->ncsi.current_package &&
		    test_obj->ncsi.info[i].channel == test_obj->ncsi.current_channel)
			break;
		if (test_obj->ncsi.info[i].package == 0xFF) {
			test_obj->ncsi.info[i].package = test_obj->ncsi.current_package;
			test_obj->ncsi.info[i].channel = test_obj->ncsi.current_channel;
			MEMCPY(test_obj->ncsi.info[i].pci_ids, test_obj->ncsi.temp_pci_ids,
			       sizeof(test_obj->ncsi.temp_pci_ids));
			test_obj->ncsi.info[i].mf_id = test_obj->ncsi.temp_mf_id;
			break;
		}
	}

	return 0;
}

static void ncsi_rsp_gvi(struct test_s *test_obj, struct ncsi_rsp_pkt *pkt)
{
	struct ncsi_rsp_gvi_pkt *gvi = (struct ncsi_rsp_gvi_pkt *)pkt;
	int i;

	for (i = 0; i < 4; i++)
		test_obj->ncsi.temp_pci_ids[i] = ntohs(gvi->pci_ids[i]);
	test_obj->ncsi.temp_mf_id = ntohl(gvi->mf_id);

	ncsi_add_list(test_obj);
	if (test_obj->ncsi.mode == NCSI_MODE_MULTI) {
		test_obj->ncsi.channel_count++;
		ncsi_update_channel_id(test_obj);
	}
}

static int ncsi_validate_rsp(struct ncsi_rsp_pkt *pkt, int payload)
{
	struct ncsi_rsp_pkt_hdr *hdr = &pkt->rsp;
	u32 checksum, c_offset;
	u32 pchecksum;

	if (hdr->common.revision != 1) {
		debug("NCSI: 0x%02x response has unsupported revision 0x%x\n", hdr->common.type,
		      hdr->common.revision);
		return -1;
	}

	if (hdr->code != 0) {
		debug("NCSI: 0x%02x response returns error %d\n", hdr->common.type,
		      __be16_to_cpu(hdr->code));
		if (ntohs(hdr->reason) == 0x05)
			printf("(Invalid command length)\n");
		return -1;
	}

	if (ntohs(hdr->common.length) != payload) {
		debug("NCSI: 0x%02x response has incorrect length %d\n", hdr->common.type,
		      hdr->common.length);
		return -1;
	}

	c_offset = sizeof(struct ncsi_rsp_pkt_hdr) + payload - sizeof(checksum);
	MEMCPY(&pchecksum, (void *)hdr + c_offset, sizeof(pchecksum));
	if (pchecksum != 0) {
		pchecksum = htonl(pchecksum);
		checksum = ncsi_calculate_checksum((unsigned char *)hdr, c_offset);
		if (pchecksum != checksum) {
			printf("NCSI: 0x%02x response has invalid checksum\n", hdr->common.type);
			return -1;
		}
	}

	return 0;
}

int ncsi_recv(struct test_s *test_obj)
{
	struct mac_s *mac_obj = test_obj->mac_obj;
	struct ncsi_rsp_pkt *pkt;
	struct ncsi_rsp_pkt_hdr *nh;
	void (*handler)(struct test_s *, struct ncsi_rsp_pkt *) = NULL;
	unsigned short payload;
	u32 rxptr;
	u32 rxlen;
	int status;

	rxptr = mac_obj->rxptr;
	/* Command response timeout = 50 ms */
	status = net_get_packet(mac_obj, (void **)&test_obj->rx_pkt_buf[rxptr], &rxlen, 50);
	if (status == 0) {
		debug("RX packet %d: length=%d addr=%p\n", rxptr, rxlen,
		      &test_obj->rx_pkt_buf[rxptr]);
	} else {
		debug("pkt#%02x: error: %d\n", rxptr, status);
		return status;
	}
	rxlen -= ETH_SIZE_HEADER;
	pkt = (struct ncsi_rsp_pkt *)(test_obj->rx_pkt_buf[rxptr] + ETH_SIZE_HEADER);
	nh = (struct ncsi_rsp_pkt_hdr *)&pkt->rsp;

	if (rxlen < sizeof(struct ncsi_rsp_pkt_hdr)) {
		printf("NCSI: undersized packet: %u bytes\n", rxlen);
		return FAIL_NCSI;
	}

	switch (nh->common.type) {
	case NCSI_PKT_RSP_GVI:
		payload = 40;
		handler = ncsi_rsp_gvi;
		break;
	case NCSI_PKT_RSP_SP:
	case NCSI_PKT_RSP_DP:
	case NCSI_PKT_RSP_CIS:
		payload = 4;
		break;
	default:
		debug("NCSI: unsupported packet type 0x%02x\n", nh->common.type);
		return FAIL_NCSI;
	}

	if (ncsi_validate_rsp(pkt, payload) != 0) {
		printf("NCSI: discarding invalid packet of type 0x%02x\n", nh->common.type);
		return FAIL_NCSI;
	}

	if (handler)
		handler(test_obj, pkt);

	return 0;
}

static int ncsi_update_state(struct test_s *test_obj)
{
	int ret;

	ret = ncsi_recv(test_obj);
	if (ret)
		return ret;

	if (test_obj->mac_obj->rxptr == 0)
		aspeed_mac_init_rx_desc(test_obj->mac_obj);

	switch (test_obj->ncsi.state) {
	case NCSI_PROBE_PACKAGE_SP:
		test_obj->ncsi.state = NCSI_PROBE_CHANNEL;
		break;
	case NCSI_PROBE_PACKAGE_DP:
		test_obj->ncsi.complete = 1;
		break;
	case NCSI_PROBE_CHANNEL:
		test_obj->ncsi.state = NCSI_GET_VERSION_ID;
		break;
	case NCSI_GET_VERSION_ID:
		if (test_obj->ncsi.mode != NCSI_MODE_MULTI) {
			test_obj->ncsi.state = NCSI_PROBE_PACKAGE_DP;
			break;
		}
		if (test_obj->ncsi.current_channel == 0x1F)
			test_obj->ncsi.state = NCSI_PROBE_PACKAGE_DP;
		else
			test_obj->ncsi.state = NCSI_PROBE_CHANNEL;
		break;
	default:
		printf("NCSI: something went very wrong, nevermind\n");
		break;
	}

	return 0;
}

static int ncsi_per_test(struct test_s *test_obj)
{
	struct mac_s *mac_obj = test_obj->mac_obj;
	int i = 0, ret;

	for (i = 0; i < test_obj->parm.loop; i++) {
		aspeed_mac_init(mac_obj);
		aspeed_mac_set_loopback(mac_obj, false);

		test_obj->ncsi.state = NCSI_PROBE_PACKAGE_SP;
		test_obj->ncsi.complete = 0;
		while (1) {
			aspeed_mac_init_tx_desc(mac_obj);
			ncsi_probe_packages(test_obj);
			ret = ncsi_update_state(test_obj);
			if (ret && test_obj->ncsi.state == NCSI_PROBE_PACKAGE_SP)
				goto out;
			if (ret && test_obj->ncsi.state == NCSI_PROBE_CHANNEL)
				ncsi_update_channel_id(test_obj);
			if (test_obj->ncsi.state == NCSI_PROBE_CHANNEL &&
			    test_obj->ncsi.current_channel == 0x1F)
				test_obj->ncsi.state = NCSI_PROBE_PACKAGE_DP;
			if (test_obj->ncsi.complete)
				break;
		}
	}
out:
	debug("%d %d %d %d\n", test_obj->ncsi.current_package, test_obj->ncsi.current_channel, i,
	      ret);
	return ret;
}

void ncsi_print_card_info(u16 vid, u16 did)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(cards); i++) {
		if (cards[i].vid == vid && cards[i].did == did) {
			printf("%s\n", cards[i].description);
			return;
		}
	}
	switch (vid) {
	case PCI_VID_INTEL_8086:
	case PCI_VID_INTEL_8087:
	case PCI_VID_INTEL_163C:
		printf("Intel");
		break;
	case PCI_VID_BROADCOM:
		printf("Boardcom");
		break;
	case PCI_VID_MELLANOX:
		printf("Mellanox");
		break;
	case PCI_VID_EMULEX:
		printf("Emulex");
		break;
	}
	printf("\n");
}

void ncsi_print_info(struct test_s *test_obj)
{
	int i;

	printf("\n[NC-SI] List scanned devices\n");
	printf("(pk, ch):(MFR. ID )(VID :DID )(SVID:SDID)\n");
	printf("=========================================\n");

	for (i = 0; i < NCSI_LIST_MAX; i++) {
		if (test_obj->ncsi.info[i].package == 0xFF)
			break;
		printf("(%02d, %02d):(%08X)(%04X:%04X)(%04X:%04X) ",
		       test_obj->ncsi.info[i].package, test_obj->ncsi.info[i].channel,
		       test_obj->ncsi.info[i].mf_id, test_obj->ncsi.info[i].pci_ids[1],
		       test_obj->ncsi.info[i].pci_ids[0], test_obj->ncsi.info[i].pci_ids[3],
		       test_obj->ncsi.info[i].pci_ids[2]);
		ncsi_print_card_info(test_obj->ncsi.info[i].pci_ids[1],
				     test_obj->ncsi.info[i].pci_ids[0]);
	}
}

int ncsi_multi_test(struct test_s *test_obj)
{
	test_obj->ncsi.current_package = 0;
	test_obj->ncsi.current_channel = 0;

	for (test_obj->ncsi.current_package = 0;
	     test_obj->ncsi.current_package < NCSI_PACKAGE_MAX;
	     test_obj->ncsi.current_package++) {
		if (ncsi_per_test(test_obj) == 0)
			test_obj->ncsi.package_count++;
		test_obj->ncsi.current_channel = 0;
	}

	if (test_obj->ncsi.package_number != 0 &&
	    test_obj->ncsi.package_count == test_obj->ncsi.package_number &&
	    test_obj->ncsi.channel_count == test_obj->ncsi.channel_number)
		return 0;

	return FAIL_NCSI_MULTI_SCAN;
}

int ncsi_single_test(struct test_s *test_obj)
{
	int ret;

	ret = ncsi_per_test(test_obj);
	if (ret)
		return ret;

	ncsi_add_list(test_obj);

	return 0;
}

int ncsitest(struct test_s *test_obj)
{
	int (*func_test)(struct test_s *test_obj);

	test_obj->ncsi.package_count = 0;
	test_obj->ncsi.channel_count = 0;

	if (test_obj->ncsi.mode == NCSI_MODE_MULTI)
		func_test = ncsi_multi_test;
	else
		func_test = ncsi_single_test;

	return func_test(test_obj);
}
