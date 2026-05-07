// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) AMD Inc.
 *
 * AMD SP7 board-local UDP syslog console replay.
 *
 * UDP syslog replay of the U-Boot console log (broadcast or unicast).
 *
 * Destination selection (env-driven, resolved once per invocation):
 *  - bmc_syslog_target=<ipv4> : unicast to this host. Needs 'ipaddr' set; the
 *                               first packet triggers ARP and we pump eth_rx()
 *                               inline until resolved (<=2 s). Subsequent
 *                               packets skip ARP.
 *  - bmc_syslog_port=<n>      : UDP dest port override. Default 514.
 *  - (neither set)            : UDP broadcast to 255.255.255.255.
 *
 * APIs:
 *  - bmc_syslog_net_up()         : one-shot net_init() + eth_halt/set_current/
 *                                  eth_init with best-effort error path.
 *  - bmc_syslog_configure_dest() : parse env and seed bmc_syslog_dst_{ip,mac,
 *                                  port}; falls back to broadcast on any
 *                                  issue so autoboot is never blocked.
 *  - bmc_syslog_send_line()      : frames one "<14>uboot-<host>: <line>"
 *                                  packet into net_tx_packet, transmits it,
 *                                  and (for unicast) waits for ARP on the
 *                                  first call.
 *  - do_bmc_syslog()             : 'bmc_syslog [message]' U-Boot command. With
 *                                  no argument, drains console_record into
 *                                  one packet per line. With an argument,
 *                                  sends that one line.
 *  - board_late_init()           : sets preboot="bmc_syslog" so the drain
 *                                  runs automatically after initr_net().
 *
 * Wire format is a minimal RFC5424-ish line:
 *     "<14>uboot-<host>: <line>"
 * (facility=user, severity=info => priority 14)
 */

#include <command.h>
#include <common.h>
#include <console.h>
#include <env.h>
#include <errno.h>
#include <net.h>
#include <stdlib.h>
#include <linux/delay.h>
#include <linux/kconfig.h>

#include "sp7_syslog.h"

#define BMC_SYSLOG_PAYLOAD_MAX	480U	/* fits inside PKTSIZE minus headers */
#define BMC_SYSLOG_PORT		514
#define BMC_SYSLOG_PRIORITY	14	/* facility=1 (user), severity=6 (info) */
#define ENV_BMC_SYSLOG_CMD	"bmc_syslog"
#define ENV_BMC_SYSLOG_EN	"bmc_syslog_en"	   /* bool: enable auto-broadcast */
#define ENV_BMC_SYSLOG_TARGET	"bmc_syslog_target" /* optional unicast IPv4 dest */
#define ENV_BMC_SYSLOG_PORT	"bmc_syslog_port"  /* optional UDP port override */
#define BMC_SYSLOG_LINE_MAX	256	/* per-line drain buffer */
#define BMC_SYSLOG_INTER_PKT_US	2000	/* 2 ms between packets */
#define BMC_SYSLOG_LINK_TRIES	3	/* eth_init attempts before giving up */
#define BMC_SYSLOG_LINK_GAP_US	500000	/* 0.5 s between eth_init retries */
#define BMC_SYSLOG_ARP_TIMEOUT_MS 2000	/* ARP resolution wait for unicast */

#define ENV_BOOTARGS		"bootargs"

#if CONFIG_IS_ENABLED(CONSOLE_RECORD)
void sp7_syslog_enable_console_record(void)
{
	/*
	 * CONFIG_CONSOLE_RECORD=y only *allocates* the ring; recording itself
	 * is gated by GD_FLG_RECORD, which U-Boot only flips on when
	 * console_record_reset_enable() is called.
	 */
	console_record_reset_enable();
}
#endif

#if defined(CONFIG_CMD_NET)
extern struct in_addr net_arp_wait_packet_ip;
extern uchar *arp_wait_packet_ethaddr;
extern int arp_wait_tx_packet_size;

static uchar bmc_syslog_dst_mac[6];
static struct in_addr bmc_syslog_dst_ip;
static int bmc_syslog_dst_port;

/*
 * Parse `bmc_syslog_en` as a boolean. Empty/unset/"0"/"false"/"no"/"off"
 * are treated as disabled; anything else enables the feature.
 */
static bool bmc_syslog_enabled(void)
{
	const char *v = env_get(ENV_BMC_SYSLOG_EN);

	if (!v || !*v)
		return false;
	if (!strcmp(v, "0") || !strcmp(v, "false") ||
	    !strcmp(v, "no") || !strcmp(v, "off"))
		return false;
	return true;
}

static int bmc_syslog_parse_port(void)
{
	const char *v = env_get(ENV_BMC_SYSLOG_PORT);
	long p;

	if (!v || !*v)
		return BMC_SYSLOG_PORT;
	p = simple_strtol(v, NULL, 10);
	if (p < 1 || p > 65535) {
		printf("bmc_syslog: invalid %s='%s', using %d\n",
		       ENV_BMC_SYSLOG_PORT, v, BMC_SYSLOG_PORT);
		return BMC_SYSLOG_PORT;
	}
	return (int)p;
}

static void bmc_syslog_use_broadcast(void)
{
	bmc_syslog_dst_ip.s_addr = 0xFFFFFFFFUL;
	memcpy(bmc_syslog_dst_mac, net_bcast_ethaddr, 6);
}

/*
 * Resolve the destination (IP + MAC seed) from env. Always succeeds; falls
 * back to broadcast if the env is unset/invalid or if unicast is impossible
 * (no local ipaddr for bmc).
 */
static void bmc_syslog_configure_dest(void)
{
	const char *v;
	struct in_addr tgt;

	bmc_syslog_dst_port = bmc_syslog_parse_port();

	v = env_get(ENV_BMC_SYSLOG_TARGET);
	if (!v || !*v) {
		bmc_syslog_use_broadcast();
		printf("bmc_syslog: dest=broadcast:%d\n", bmc_syslog_dst_port);
		return;
	}

	tgt = string_to_ip(v);
	if (tgt.s_addr == 0 || tgt.s_addr == 0xFFFFFFFFUL) {
		printf("bmc_syslog: invalid %s='%s'; using broadcast\n",
		       ENV_BMC_SYSLOG_TARGET, v);
		bmc_syslog_use_broadcast();
		printf("bmc_syslog: dest=broadcast:%d\n", bmc_syslog_dst_port);
		return;
	}

	if (net_ip.s_addr == 0) {
		printf("bmc_syslog: local ipaddr unset (needed for unicast); using broadcast\n");
		bmc_syslog_use_broadcast();
		printf("bmc_syslog: dest=broadcast:%d\n", bmc_syslog_dst_port);
		return;
	}

	bmc_syslog_dst_ip = tgt;
	memset(bmc_syslog_dst_mac, 0, 6);
	printf("bmc_syslog: dest=%pI4:%d (unicast; ARP on first send)\n",
	       &bmc_syslog_dst_ip, bmc_syslog_dst_port);
}

/* Resolve a reasonable hostname string for the syslog tag. */
static const char *bmc_syslog_hostname(void)
{
	const char *h = env_get("hostname");
	const char *ba, *hp;

	if (h && *h)
		return h;

	ba = env_get(ENV_BOOTARGS);
	hp = ba ? strstr(ba, "systemd.hostname=") : NULL;
	return hp ? hp + strlen("systemd.hostname=") : "bmc";
}

/*
 * We use the SP7 BMC dedicated port (eth0 = ftgmac@14050000) and retry
 * eth_init() up to BMC_SYSLOG_LINK_TRIES times if PHY auto-neg / link-up
 * fails. If it still cannot come up we return the error and the caller
 * skips TX so boot continues.
 *
 * Must be called from a context where network init is safe (e.g. preboot /
 * command line after initr_net), NOT from misc_init_r.
 */
static int bmc_syslog_net_up(void)
{
	int ret;
	int tries;

	/* Ensure net_tx_packet and other buffers exist. */
	ret = net_init();
	if (ret) {
		printf("bmc_syslog: net_init failed (%d)\n", ret);
		return ret;
	}

	if (!eth_is_on_demand_init())
		return 0;

	eth_set_current();

	for (tries = 1; tries <= BMC_SYSLOG_LINK_TRIES; tries++) {
		eth_halt();
		ret = eth_init();
		if (ret >= 0)
			return 0;
		printf("bmc_syslog: eth_init try %d/%d failed (%d)\n",
		       tries, BMC_SYSLOG_LINK_TRIES, ret);
		if (tries < BMC_SYSLOG_LINK_TRIES)
			udelay(BMC_SYSLOG_LINK_GAP_US);
	}

	eth_halt();
	printf("bmc_syslog: link not up after %d tries; skipping broadcast\n",
	       BMC_SYSLOG_LINK_TRIES);
	return ret;
}

/*
 * Build and send one UDP packet with `line` as payload. The destination
 * (broadcast vs unicast) and UDP port are picked from env by
 * bmc_syslog_configure_dest(), which must have run first.
 */
static int bmc_syslog_send_line(const char *line)
{
	char *payload;
	const char *host;
	int payload_len;
	int eth_hdr_size;
	int ret;

	if (!line || !*line)
		return 0;

	eth_hdr_size = net_set_ether(net_tx_packet, bmc_syslog_dst_mac, PROT_IP);
	payload = (char *)net_tx_packet + eth_hdr_size + IP_UDP_HDR_SIZE;

	host = bmc_syslog_hostname();
	payload_len = snprintf(payload, BMC_SYSLOG_PAYLOAD_MAX,
			       "<%u>uboot-%s: %s",
			       BMC_SYSLOG_PRIORITY, host, line);
	if (payload_len <= 0)
		return -EINVAL;
	if (payload_len > (int)BMC_SYSLOG_PAYLOAD_MAX)
		payload_len = BMC_SYSLOG_PAYLOAD_MAX;
	/* Include trailing NUL so receivers logging as C strings see full line. */
	payload_len += 1;

	ret = net_send_udp_packet(bmc_syslog_dst_mac, bmc_syslog_dst_ip,
				  bmc_syslog_dst_port, bmc_syslog_dst_port,
				  payload_len);
	if (ret == 1) {
		ulong start = get_timer(0);

		while (net_arp_wait_packet_ip.s_addr &&
		       get_timer(start) < BMC_SYSLOG_ARP_TIMEOUT_MS)
			eth_rx();

		if (net_arp_wait_packet_ip.s_addr) {
			net_arp_wait_packet_ip.s_addr = 0;
			arp_wait_packet_ethaddr = NULL;
			arp_wait_tx_packet_size = 0;
			printf("bmc_syslog: ARP timeout for %pI4; switching to broadcast\n",
			       &bmc_syslog_dst_ip);
			bmc_syslog_use_broadcast();
			return -ETIMEDOUT;
		}
		ret = 0;
	} else if (ret < 0) {
		printf("bmc_syslog: TX failed (%d)\n", ret);
	}

	return ret;
}

static int do_bmc_syslog(struct cmd_tbl *cmdtp, int flag, int argc,
			 char *const argv[])
{
	int ret;

	/* Manual single-line mode: `bmc_syslog "msg"` always works, so an
	 * operator can debug from the prompt without toggling the env var.
	 */
	if (argc > 1) {
		ret = bmc_syslog_net_up();
		if (ret)
			return CMD_RET_SUCCESS; /* never block autoboot */
		bmc_syslog_configure_dest();
		(void)bmc_syslog_send_line(argv[1]);
		return CMD_RET_SUCCESS;
	}

	if (!bmc_syslog_enabled())
		return CMD_RET_SUCCESS;

	ret = bmc_syslog_net_up();
	if (ret)
		return CMD_RET_SUCCESS; /* never block autoboot */

	bmc_syslog_configure_dest();

#if CONFIG_IS_ENABLED(CONSOLE_RECORD)
	{
		char line[BMC_SYSLOG_LINE_MAX];
		int drained = 0;

		int budget = console_record_avail();

		while (budget > 0) {
			int n = console_record_readline(line, sizeof(line));

			if (n <= 0)
				break;
			/* readline returns strlen; account against budget
			 * using n + 1 for the consumed newline.
			 */
			budget -= (n + 1);
			(void)bmc_syslog_send_line(line);
			drained++;
			udelay(BMC_SYSLOG_INTER_PKT_US);
		}
		if (!drained)
			(void)bmc_syslog_send_line("boot");
	}
#else
	(void)bmc_syslog_send_line("boot");
#endif

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(bmc_syslog, 2, 0, do_bmc_syslog,
	   "replay U-Boot console over UDP syslog",
	   "[message]\n"
	   "    - With no argument, drains the CONSOLE_RECORD ring and sends one\n"
	   "      '<14>uboot-<host>: <line>' UDP packet per captured line. The\n"
	   "      auto/no-arg path is GATED on the env variable 'bmc_syslog_en'\n"
	   "      (set to 1/true/yes/on to enable).\n"
	   "      Default: disabled so normal boots are not delayed by PHY wait.\n"
	   "    - With an argument, sends that single line (always enabled).\n"
	   "\n"
	   "  Destination (optional env overrides):\n"
	   "    bmc_syslog_target=<ipv4>  unicast to this host (requires\n"
	   "                              'ipaddr' to be set; first packet\n"
	   "                              triggers ARP). Default: 255.255.255.255\n"
	   "                              (broadcast) if unset or invalid.\n"
	   "    bmc_syslog_port=<n>       UDP dest port (1..65535). Default: 514.\n"
	   "\n"
	   "  On ARP timeout or missing local ipaddr the code falls back to\n"
	   "  broadcast and continues; it never blocks autoboot.");
#endif /* CONFIG_CMD_NET */

/*
 * Late board init runs before initr_net, so we cannot transmit here.
 * Instead we wire up the `preboot` env var to run our bmc_syslog command;
 * preboot runs from main_loop() AFTER initr_net(), so the MAC is ready.
 */
int board_late_init(void)
{
#if defined(CONFIG_CMD_NET) && defined(CONFIG_USE_PREBOOT)
	const char *pre = env_get("preboot");

	if (bmc_syslog_enabled()) {
		if (!pre || !*pre)
			env_set("preboot", ENV_BMC_SYSLOG_CMD);
	} else if (pre && !strcmp(pre, ENV_BMC_SYSLOG_CMD)) {
		/* We previously set it; clear so disabling takes effect
		 * without needing a manual `setenv preboot`.
		 */
		env_set("preboot", NULL);
	}
	/* Do not env_save here; avoid extra SPI writes per boot. */
#endif
	return 0;
}
