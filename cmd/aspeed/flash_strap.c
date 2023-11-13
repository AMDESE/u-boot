// SPDX-License-Identifier: GPL-2.0+
/*
 * Command for access flash strap
 *
 * Copyright (C) 2023 ASPEED Tech.
 */

#include <asm/cache.h>
#include <asm/io.h>
#include <common.h>
#include <command.h>
#include <console.h>
#include <display_options.h>
#include <dm/device-internal.h>
#include <div64.h>
#include <dm.h>
#include <linux/mtd/mtd.h>
#include <log.h>
#include <malloc.h>
#include <mapmem.h>
#include <mmc.h>
#include <spi.h>
#include <spi_flash.h>

static struct spi_flash *flash;

#define FLASH_STRAP_MAGIC	(0x50415254)
#define FLASH_STRAP_SZ_ALG	((0x01 << 16) | (0x18))

static uint32_t g_flash_strap_content[8] __aligned(8) = {0};
static uint8_t *g_buf;

bool g_strap_flash_read;

static int curr_mmc_device = -1;

struct flash_strap {
	char *idx_mark;
	char *name;
	uint32_t dword_idx;
	uint32_t bit_offset;
	uint32_t len;
	uint32_t value;
	uint32_t desc_max_len;
	char (*description)[32];
};

#define STRAP_INFO(_idx_mark_, _name_, _dword_idx_, _bit_offset_, _len_, _desc_max_len_, _des_)	\
	{	\
		.idx_mark = _idx_mark_,	\
		.name = _name_,	\
		.dword_idx = _dword_idx_,	\
		.bit_offset = _bit_offset_,	\
		.len = _len_,	\
		.value = 0,	\
		.desc_max_len = _desc_max_len_,	\
		.description = _des_,	\
	}

char arm_debug_info[2][32] = {
	"ARM Debug is Enabled",
	"ARM Debug is Disabled",
};

char vga_class_code_info[2][32] = {
	"VGA Device",
	"VEDIO Device",
};

char emmc_boot_speed_info[2][32] = {
	"Low Speed(25MHz)",
	"50MHz",
};

char xhci_info[2][32] = {
	"XHCI is Enabled",
	"XHCI is Disabled",
};

char arm_debug_in_tz_info[2][32] = {
	"ARM Debug in TZ is Enabled",
	"ARM Debug in TZ is Disabled",
};

char wdt_full_rst_info[2][32] = {
	"WDT Full Reset is Enabled",
	"WDT Full Reset is Disabled",
};

char rvas_info[2][32] = {
	"RVAS is Enabled",
	"RVAS is Disabled",
};

char abr_info[2][32] = {
	"ABR is Disabled",
	"ABR is Enabled",
};

char rom_clear_sram_info[2][32] = {
	"BROM will not Clear SRAM",
	"BROM will Clear SRAM",
};

char recovery_mode_info[2][32] = {
	"Recovery Mode is Enabled",
	"Recovery Mode is Disabled",
};

char spi_flash_sz_info[8][32] = {
	"Disabled",
	"8MB", "16MB", "32MB", "64MB", "128MB", "256MB", "512MB",
};

char spi_aux_pin_info[2][32] = {
	"AUX pins are Disabled",
	"AUX pins are Enabled",
};

char ltpi_max_freq_info[3][32] = {
	"1GHz",
	"800MHz",
	"Others",
};

char ltpi_io_type_info[2][32] = {
	"LVDS",
	"Single-End",
};

char fwspi_init_freq_info[4][32] = {
	"12.5MHz",
	"25MHz",
	"40MHz",
	"50MHz",
};

char abr_cs_swap_info[2][32] = {
	"CS Swap is Enabled",
	"CS Swap is Disabled",
};

char fwspi_abr_mode_info[2][32] = {
	"Dual Flash ABR",
	"Single Flash ABR",
};

char sio_decode_addr_info[2][32] = {
	"deocde addr: 0x2E",
	"deocde addr: 0x4E",
};

char sio_decoding_info[2][32] = {
	"SIO Decoding is Enabled",
	"SIO Decoding is Disabled",
};

char acpi_info[2][32] = {
	"ACPI is Disabled",
	"ACPI is Enabled",
};

char lpc_info[2][32] = {
	"LPC is Disabled",
	"LPC is Enabled",
};

char edaf_info[2][32] = {
	"eDAF is Disabled",
	"eDAF is Enabled",
};

char espi_rtc_info[2][32] = {
	"eSPI RTC is Disabled",
	"eSPI RTC is Enabled",
};

char ssp_info[2][32] = {
	"SSP is Enabled",
	"SSP is Disabled",
};

char tsp_info[2][32] = {
	"TSP is Enabled",
	"TSP is Disabled",
};

char option_rom_info[2][32] = {
	"System BIOS",
	"DRAM",
};

char vga_ram_sz_info[2][32] = {
	"32MB",
	"64MB",
};

char dp_info[2][32] = {
	"DP is Disabled",
	"DP is Enabled",
};

char pcie_vga_info[2][32] = {
	"PCIe VGA is Enabled",
	"PCIe VGA is Disabled",
};

char pcie_ehci_info[2][32] = {
	"PCIe EHCI is Enabled",
	"PCIe EHCI is Disabled",
};

char pcie_xhci_info[2][32] = {
	"PCIe XHCI is Enabled",
	"PCIe XHCI is Disabled",
};

struct flash_strap g_strap_info[] = {
	STRAP_INFO("[0F : 00]",	"reserved",
		   0,	0,	16,	0,	NULL),
	STRAP_INFO("[     10]",	"Disable ARM Debug",
		   0,	16,	1,	2,	arm_debug_info),
	STRAP_INFO("[     11]",	"reserved",
		   0,	17,	1,	0,	NULL),
	STRAP_INFO("[     12]",	"VGA Class Code",
		   0,	18,	1,	2,	vga_class_code_info),
	STRAP_INFO("[     13]",	"reserved",
		   0,	19,	1,	0,	NULL),
	STRAP_INFO("[     14]",	"eMMC Boot Speed",
		   0,	20,	1,	2,	emmc_boot_speed_info),
	STRAP_INFO("[     15]",	"Disable XHCI",
		   0,	21,	1,	2,	xhci_info),
	STRAP_INFO("[     16]",	"Disable ARM Debug in TrustZone",
		   0,	22,	1,	2,	arm_debug_in_tz_info),
	STRAP_INFO("[18 : 17]",	"reserved",
		   0,	23,	2,	0,	NULL),
	STRAP_INFO("[     19]",	"Disable WDT Full Reset",
		   0,	25,	1,	2,	wdt_full_rst_info),
	STRAP_INFO("[1D : 1A]",	"reserved",
		   0,	26,	4,	0,	NULL),
	STRAP_INFO("[     1E]",	"Disable RVAS",
		   0,	30,	1,	2,	rvas_info),
	STRAP_INFO("[     1F]",	"reserved",
		   0,	31,	1,	0,	NULL),

	STRAP_INFO("[     20]",	"Flash/Storage ABR",
		   1,	0,	1,	2,	abr_info),
	STRAP_INFO("[2B : 21]",	"reserved",
		   1,	1,	11,	0,	NULL),
	STRAP_INFO("[     2C]",	"Disable Recovery Mode",
		   1,	12,	1,	2,	recovery_mode_info),
	STRAP_INFO("[2F : 2D]",	"FWSPI Flash Size",
		   1,	13,	3,	8,	spi_flash_sz_info),
	STRAP_INFO("[     30]",	"Enable FWSPI AUX pin",
		   1,	16,	1,	2,	spi_aux_pin_info),
	STRAP_INFO("[32 : 31]",	"reserved",
		   1,	17,	2,	0,	NULL),
	STRAP_INFO("[35 : 33]",	"LTPI Maximum Frequency",
		   1,	19,	3,	3,	ltpi_max_freq_info),
	STRAP_INFO("[     36]",	"LTPI IO Type",
		   1,	22,	1,	2,	ltpi_io_type_info),
	STRAP_INFO("[38 : 37]",	"FWSPI Initial Frequency",
		   1,	23,	2,	4,	fwspi_init_freq_info),
	STRAP_INFO("[3B : 39]",	"reserved",
		   1,	25,	3,	0,	NULL),
	STRAP_INFO("[     3C]",	"Disabled FWSPI ABR CS SWAP",
		   1,	28,	1,	2,	abr_cs_swap_info),
	STRAP_INFO("[     3D]",	"FWSPI ABR Mode",
		   1,	29,	1,	2,	fwspi_abr_mode_info),
	STRAP_INFO("[3F : 3E]",	"reserved",
		   1,	30,	2,	0,	NULL),

	STRAP_INFO("[42 : 40]",	"Host 0 SPI Flash Size",
		   2,	0,	3,	8,	spi_flash_sz_info),
	STRAP_INFO("[45 : 43]",	"reserved",
		   2,	3,	3,	0,	NULL),
	STRAP_INFO("[     46]",	"Host 0 SIO Decode Address",
		   2,	6,	1,	2,	sio_decode_addr_info),
	STRAP_INFO("[     47]",	"Host 0 Disable SIO Decoding",
		   2,	7,	1,	2,	sio_decoding_info),
	STRAP_INFO("[     48]",	"Host 0 Enable ACPI",
		   2,	8,	1,	2,	acpi_info),
	STRAP_INFO("[     49]",	"Host 0 Enable LPC",
		   2,	9,	1,	2,	lpc_info),
	STRAP_INFO("[     4A]",	"Host 0 Enable eDAF",
		   2,	10,	1,	2,	edaf_info),
	STRAP_INFO("[     4B]",	"Host 0 Enable eSPI RTC",
		   2,	11,	1,	2,	espi_rtc_info),
	STRAP_INFO("[4F : 4C]",	"reserved",
		   2,	12,	4,	0,	NULL),
	STRAP_INFO("[52 : 50]",	"Host 1 SPI Flash Size",
		   2,	16,	3,	8,	spi_flash_sz_info),
	STRAP_INFO("[55 : 53]",	"reserved",
		   2,	19,	3,	0,	NULL),
	STRAP_INFO("[     56]",	"Host 1 SIO Decode Address",
		   2,	22,	1,	2,	sio_decode_addr_info),
	STRAP_INFO("[     57]",	"Host 1 Disable SIO Decoding",
		   2,	23,	1,	2,	sio_decoding_info),
	STRAP_INFO("[     58]",	"Host 1 Enable ACPI",
		   2,	24,	1,	2,	acpi_info),
	STRAP_INFO("[     59]",	"Host 1 Enable LPC",
		   2,	25,	1,	2,	lpc_info),
	STRAP_INFO("[     5A]",	"Host 1 Enable eDAF",
		   2,	26,	1,	2,	edaf_info),
	STRAP_INFO("[     5B]",	"Host 1 Enable eSPI RTC",
		   2,	27,	1,	2,	espi_rtc_info),
	STRAP_INFO("[5F : 5C]",	"reserved",
		   2,	28,	4,	0,	NULL),

	STRAP_INFO("[     60]",	"Disable SSP",
		   3,	0,	1,	2,	ssp_info),
	STRAP_INFO("[     61]",	"Disable TSP",
		   3,	1,	1,	2,	tsp_info),
	STRAP_INFO("[     62]",	"Host 0 Option ROM Config",
		   3,	2,	1,	2,	option_rom_info),
	STRAP_INFO("[     63]",	"Host 1 Option ROM Config",
		   3,	3,	1,	2,	option_rom_info),
	STRAP_INFO("[     64]",	"Host 0 VGA RAM Size",
		   3,	4,	1,	2,	vga_ram_sz_info),
	STRAP_INFO("[     65]",	"Host 1 VGA RAM Size",
		   3,	5,	1,	2,	vga_ram_sz_info),
	STRAP_INFO("[     66]",	"Enable DP",
		   3,	6,	1,	2,	dp_info),
	STRAP_INFO("[     67]",	"Host 0 Disable PCIe VGA",
		   3,	7,	1,	2,	pcie_vga_info),
	STRAP_INFO("[     68]",	"reserved",
		   3,	8,	1,	0,	NULL),
	STRAP_INFO("[     69]",	"Host 0 Disable PCIe EHCI",
		   3,	9,	1,	2,	pcie_ehci_info),
	STRAP_INFO("[     6A]",	"Host 0 Disable PCIe XHCI",
		   3,	10,	1,	2,	pcie_xhci_info),
	STRAP_INFO("[     6B]",	"Host 1 Disable PCIe VGA",
		   3,	11,	1,	2,	pcie_xhci_info),
	STRAP_INFO("[     6C]",	"reserved",
		   3,	12,	1,	0,	NULL),
	STRAP_INFO("[     6D]",	"Host 1 Disable PCIe EHCI",
		   3,	13,	1,	2,	pcie_ehci_info),
	STRAP_INFO("[     6E]",	"Host 1 Disable PCIe XHCI",
		   3,	14,	1,	2,	pcie_xhci_info),
	STRAP_INFO("[7F : 6F]",	"reserved",
		   3,	15,	17,	0,	NULL),
};

static int strap_spi_flash_probe(void)
{
	unsigned int bus = CONFIG_SF_DEFAULT_BUS;
	unsigned int cs = CONFIG_SF_DEFAULT_CS;
	struct udevice *new, *bus_dev;
	int ret;

	if (!IS_ENABLED(CONFIG_DM_SPI_FLASH)) {
		printf("please enable DM_SPI_FLASH config\n");
		return 1;
	}

	/* Remove the old device, otherwise probe will just be a nop */
	ret = spi_find_bus_and_cs(bus, cs, &bus_dev, &new);
	if (!ret)
		device_remove(new, DM_REMOVE_NORMAL);

	flash = NULL;

	spi_flash_probe_bus_cs(bus, cs, &new);
	flash = dev_get_uclass_priv(new);

	if (!flash) {
		printf("Failed to initialize SPI flash at %u:%u (error %d)\n",
		       bus, cs, ret);
		return 1;
	}

	return 0;
}

static const char *strap_spi_flash_update_block(struct spi_flash *flash, u32 offset,
						size_t len, const char *buf,
						char *cmp_buf, size_t *skipped)
{
	char *ptr = (char *)buf;

	debug("offset=%#x, sector_size=%#x, len=%#zx\n",
	      offset, flash->sector_size, len);
	/* Read the entire sector so to allow for rewriting */
	if (spi_flash_read(flash, offset, flash->sector_size, cmp_buf))
		return "read";
	/* Compare only what is meaningful (len) */
	if (memcmp(cmp_buf, buf, len) == 0) {
		debug("Skip region %x size %zx: no change\n",
		      offset, len);
		*skipped += len;
		return NULL;
	}
	/* Erase the entire sector */
	if (spi_flash_erase(flash, offset, flash->sector_size))
		return "erase";
	/* If it's a partial sector, copy the data into the temp-buffer */
	if (len != flash->sector_size) {
		memcpy(cmp_buf, buf, len);
		ptr = cmp_buf;
	}
	/* Write one complete sector */
	if (spi_flash_write(flash, offset, flash->sector_size, ptr))
		return "write";

	return NULL;
}

static int strap_spi_flash_update(struct spi_flash *flash, u32 offset,
				  size_t len, const char *buf)
{
	const char *err_oper = NULL;
	char *cmp_buf;
	const char *end = buf + len;
	size_t todo;		/* number of bytes to do in this pass */
	size_t skipped = 0;	/* statistics */

	cmp_buf = memalign(ARCH_DMA_MINALIGN, flash->sector_size);
	if (cmp_buf) {
		for (; buf < end && !err_oper; buf += todo, offset += todo) {
			todo = min_t(size_t, end - buf, flash->sector_size);
			err_oper = strap_spi_flash_update_block(flash, offset, todo,
								buf, cmp_buf,
								&skipped);
		}
	} else {
		err_oper = "malloc";
	}

	free(cmp_buf);

	if (err_oper) {
		printf("SPI flash failed in %s step\n", err_oper);
		return 1;
	}

	return 0;
}

static int do_strap_sf_update(uint8_t *buf)
{
	int ret;
	uint32_t i;

	for (i = 0; i < 8; i++)
		*(uint32_t *)(buf + 0x1F000 + i * 4) = g_flash_strap_content[i];

	printf("update strap into SPI flsah offset 0x1F000...");
	ret = strap_spi_flash_update(flash, 0x0, 0x20000, buf);
	if (ret != 0)
		printf("failed.\n");
	else
		printf("done.\n");

	return ret;
}

static uint32_t strap_checksum_calc(uint32_t *buf, uint32_t len_dwd)
{
	uint32_t i;
	uint32_t checksum = 0;

	for (i = 0; i < len_dwd; i++)
		checksum += buf[i];

	return checksum;
}

static void strap_value_calc(void)
{
	uint32_t idx;
	uint32_t dword_idx;
	uint32_t bit_offset;
	uint32_t bit_len;
	uint32_t mask;

	for (idx = 0; idx < ARRAY_SIZE(g_strap_info); idx++) {
		dword_idx = g_strap_info[idx].dword_idx;
		bit_offset = g_strap_info[idx].bit_offset;
		bit_len = g_strap_info[idx].len;
		mask = (BIT(bit_len) - 1) << bit_offset;
		g_strap_info[idx].value =
			(g_flash_strap_content[dword_idx + 2] & mask) >> bit_offset;
	}
}

static int do_strap_info_conf(uint32_t strap_num, uint32_t value)
{
	uint32_t dword_idx;
	uint32_t bit_offset;
	uint32_t idx;
	uint32_t mask;

	if (strap_num >= 128)
		return CMD_RET_FAILURE;

	dword_idx = strap_num / 32;
	bit_offset = strap_num % 32;

	if (dword_idx > 3)
		return CMD_RET_FAILURE;

	for (idx = 0; idx < ARRAY_SIZE(g_strap_info); idx++) {
		if (g_strap_info[idx].dword_idx != dword_idx)
			continue;
		if (bit_offset >= g_strap_info[idx].bit_offset &&
		    bit_offset < g_strap_info[idx].bit_offset + g_strap_info[idx].len) {
			bit_offset = g_strap_info[idx].bit_offset;
			mask = BIT(g_strap_info[idx].len) - 1;
			value &= mask;
			printf("modify %s: %s to value: 0x%x...",
			       g_strap_info[idx].idx_mark,
			       g_strap_info[idx].name, value);

			mask <<= bit_offset;
			g_flash_strap_content[dword_idx + 2] &= ~(mask);
			g_flash_strap_content[dword_idx + 2] |= (value << bit_offset);
			g_strap_info[idx].value = value;
			printf("done\n");
			break;
		}
	}

	g_flash_strap_content[6] = strap_checksum_calc((g_flash_strap_content + 2), 4);

	return 0;
}

static void do_strap_dump_info(void)
{
	uint32_t idx;
	uint32_t value;

	printf("------------------------------------------------------------------------------\n");
	printf("BIT(hex)         Name                      Value(hex)    Status\n");
	printf("------------------------------------------------------------------------------\n");
	for (idx = 0; idx < ARRAY_SIZE(g_strap_info); idx++) {
		printf("%-14s", g_strap_info[idx].idx_mark);
		printf("%-32s", g_strap_info[idx].name);
		value = g_strap_info[idx].value;
		if (value > g_strap_info[idx].desc_max_len)
			value = g_strap_info[idx].desc_max_len - 1;
		g_strap_info[idx].value = value;
		printf("0x%-5x", value);

		if (g_strap_info[idx].description)
			printf("%-32s", g_strap_info[idx].description[value]);

		printf("\n");
	}
	printf("------------------------------------------------------------------------------\n");
}

static int do_strap_sf_read(void)
{
	int ret;
	enum command_ret_t cmd_ret = CMD_RET_SUCCESS;
	int i;
	bool re_init = false;
	uint32_t checksum;

	if (g_strap_flash_read)
		return 0;

	ret = strap_spi_flash_probe();
	if (ret != 0) {
		cmd_ret = CMD_RET_FAILURE;
		goto end;
	}

	if (g_buf)
		free(g_buf);

	/* allocate 128kB */
	g_buf = memalign(ARCH_DMA_MINALIGN, 0x20000);
	if (!g_buf) {
		cmd_ret = CMD_RET_FAILURE;
		goto end;
	}

	ret = spi_flash_read(flash, 0, 0x20000, g_buf);
	if (ret != 0) {
		printf("SF Read failed (ret = %d)\n", ret);
		cmd_ret = CMD_RET_FAILURE;
		goto end;
	}

	for (i = 0; i < 8; i++)
		g_flash_strap_content[i] = *(uint32_t *)(g_buf + 0x1F000 + i * 4);

	checksum = strap_checksum_calc((g_flash_strap_content + 2), 4);

	if (g_flash_strap_content[0] != FLASH_STRAP_MAGIC) {
		printf("invalid strap magic 0x%08x\n",
		       g_flash_strap_content[0]);
		re_init = true;
	} else if ((g_flash_strap_content[1] & 0xffff) != 0x18) {
		printf("invalid strap image size 0x%04x\n",
		       g_flash_strap_content[1] & 0xffff);
		re_init = true;
	} else if ((g_flash_strap_content[1] & 0xffff0000) != 0x00010000) {
		printf("non-checksum algorithm 0x%08x\n",
		       g_flash_strap_content[1] & 0xffff0000);
		re_init = true;
	} else if (checksum != g_flash_strap_content[6]) {
		printf("unexpected checksum val: 0x%08x (0x%08x)\n",
		       g_flash_strap_content[6], checksum);
		re_init = true;
	}

	if (re_init) {
		printf("reinitialize the flash strap.\n");
		g_flash_strap_content[0] = FLASH_STRAP_MAGIC;
		g_flash_strap_content[1] = FLASH_STRAP_SZ_ALG;
		g_flash_strap_content[2] = 0x0;
		g_flash_strap_content[3] = 0x0;
		g_flash_strap_content[4] = 0x0;
		g_flash_strap_content[5] = 0x0;
		g_flash_strap_content[6] =
			strap_checksum_calc((g_flash_strap_content + 2), 4);
	}

	strap_value_calc();
	g_strap_flash_read = true;

end:
	if (!g_strap_flash_read && g_buf)
		free(g_buf);

	return cmd_ret;
}

/*
 * ASPEED flash strap command
 * strap sf read
 * strap sf config <idx> <value>
 * strap sf update
 * strap mmc read
 * strap mmc config <idx> <value>
 * strap mmc update
 */
static int do_flash_strap_sf(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	int ret;
	enum command_ret_t cmd_ret = CMD_RET_SUCCESS;
	uint32_t strap_num;
	uint32_t strap_val;

	if (argc != 2 && argc != 4) {
		cmd_ret = CMD_RET_USAGE;
		goto end;
	}

	ret = do_strap_sf_read();
	if (ret != 0) {
		cmd_ret = CMD_RET_USAGE;
		goto end;
	}

	if (!strcmp(argv[1], "read")) {
		do_strap_dump_info();
	} else if (!strcmp(argv[1], "conf")) {
		strap_num = simple_strtoul(argv[2], NULL, 16);
		strap_val = simple_strtoul(argv[3], NULL, 16);
		ret = do_strap_info_conf(strap_num, strap_val);
		if (ret != 0) {
			cmd_ret = CMD_RET_FAILURE;
			goto end;
		}
	} else if (!strcmp(argv[1], "update")) {
		if (!g_buf) {
			cmd_ret = CMD_RET_FAILURE;
			goto end;
		}

		ret = do_strap_sf_update(g_buf);
		if (ret != 0) {
			cmd_ret = CMD_RET_FAILURE;
			goto end;
		}
	}

end:
	return cmd_ret;
}

static struct mmc *init_mmc_device(int dev, bool force_init)
{
	struct mmc *mmc;

	mmc = find_mmc_device(dev);
	if (!mmc) {
		printf("no mmc device at slot %x\n", dev);
		return NULL;
	}

	if (!mmc_getcd(mmc))
		force_init = true;

	if (force_init)
		mmc->has_init = 0;

	if (mmc_init(mmc))
		return NULL;

	if (IS_ENABLED(CONFIG_BLOCK_CACHE)) {
		struct blk_desc *bd = mmc_get_blk_desc(mmc);

		blkcache_invalidate(bd->uclass_id, bd->devnum);
	}

	return mmc;
}

static int do_mmc_write(uint32_t mmc_blk, uint32_t mmc_cnt, void *buf)
{
	struct mmc *mmc;
	uint32_t n;

	mmc = init_mmc_device(curr_mmc_device, false);
	if (!mmc)
		return CMD_RET_FAILURE;

	printf("\nMMC write: dev # %d, block # %d, count %d ... ",
	       curr_mmc_device, mmc_blk, mmc_cnt);

	if (mmc_getwp(mmc) == 1) {
		printf("Error: eMMC is write protected!\n");
		return CMD_RET_FAILURE;
	}

	n = blk_dwrite(mmc_get_blk_desc(mmc), mmc_blk, mmc_cnt, buf);
	printf("%d blocks written: %s\n", n, (n == mmc_cnt) ? "OK" : "ERROR");

	return (n == mmc_cnt) ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}

static int do_mmc_dev(void)
{
	int dev = 0, part = 1, ret;
	struct mmc *mmc;

	mmc = init_mmc_device(dev, true);
	if (!mmc)
		return CMD_RET_FAILURE;

	ret = blk_select_hwpart_devnum(UCLASS_MMC, dev, part);
	printf("switch to partitions #%d, %s\n",
	       part, (!ret) ? "OK" : "ERROR");
	if (ret != 0)
		return 1;

	curr_mmc_device = dev;
	if (mmc->part_config == MMCPART_NOAVAILABLE)
		printf("mmc%d is current device\n", curr_mmc_device);
	else
		printf("mmc%d(part %d) is current device\n",
		       curr_mmc_device, mmc_get_blk_desc(mmc)->hwpart);

	return 0;
}

static int do_strap_mmc_read(void)
{
	int ret;
	struct mmc *mmc;
	enum command_ret_t cmd_ret = CMD_RET_SUCCESS;
	int i;
	bool re_init = false;
	uint32_t checksum;
	uint32_t mmc_blk = 0;
	uint32_t mmc_cnt = (128 * 1024) / 512;
	uint32_t n;

	if (g_strap_flash_read)
		return 0;

	ret = do_mmc_dev();
	if (ret != 0) {
		cmd_ret = CMD_RET_FAILURE;
		goto end;
	}

	mmc = init_mmc_device(curr_mmc_device, false);
	if (!mmc) {
		cmd_ret = CMD_RET_FAILURE;
		goto end;
	}

	if (g_buf)
		free(g_buf);

	/* allocate 128kB */
	g_buf = memalign(ARCH_DMA_MINALIGN, 0x20000);
	if (!g_buf) {
		cmd_ret = CMD_RET_FAILURE;
		goto end;
	}

	printf("\nMMC read: dev # %d, block # %d, count %d ... ",
	       curr_mmc_device, mmc_blk, mmc_cnt);
	n = blk_dread(mmc_get_blk_desc(mmc), mmc_blk, mmc_cnt, (void *)g_buf);
	printf("%d blocks read:", n);
	if (n == mmc_cnt) {
		printf("OK\n");
	} else {
		printf("ERROR\n");
		cmd_ret = CMD_RET_FAILURE;
		goto end;
	}

	for (i = 0; i < 8; i++)
		g_flash_strap_content[i] = *(uint32_t *)(g_buf + 0x1F000 + i * 4);

	checksum = strap_checksum_calc((g_flash_strap_content + 2), 4);

	if (g_flash_strap_content[0] != FLASH_STRAP_MAGIC) {
		printf("invalid strap magic 0x%08x\n",
		       g_flash_strap_content[0]);
		re_init = true;
	} else if ((g_flash_strap_content[1] & 0xffff) != 0x18) {
		printf("invalid strap image size 0x%04x\n",
		       g_flash_strap_content[1] & 0xffff);
		re_init = true;
	} else if ((g_flash_strap_content[1] & 0xffff0000) != 0x00010000) {
		printf("non-checksum algorithm 0x%08x\n",
		       g_flash_strap_content[1] & 0xffff0000);
		re_init = true;
	} else if (checksum != g_flash_strap_content[6]) {
		printf("unexpected checksum val: 0x%08x (0x%08x)\n",
		       g_flash_strap_content[6], checksum);
		re_init = true;
	}

	if (re_init) {
		printf("reinitialize the flash strap.\n");
		g_flash_strap_content[0] = FLASH_STRAP_MAGIC;
		g_flash_strap_content[1] = FLASH_STRAP_SZ_ALG;
		g_flash_strap_content[2] = 0x0;
		g_flash_strap_content[3] = 0x0;
		g_flash_strap_content[4] = 0x0;
		g_flash_strap_content[5] = 0x0;
		g_flash_strap_content[6] =
			strap_checksum_calc((g_flash_strap_content + 2), 4);
	}

	strap_value_calc();
	g_strap_flash_read = true;

end:
	if (!g_strap_flash_read && g_buf)
		free(g_buf);

	return cmd_ret;
}

static int do_strap_mmc_update(uint8_t *buf)
{
	int ret;
	uint32_t i;

	for (i = 0; i < 8; i++)
		*(uint32_t *)(buf + 0x1F000 + i * 4) = g_flash_strap_content[i];

	printf("update strap into eMMC offset 0x1F000...");
	ret = do_mmc_write(0x0, (128 * 1024) / 512, buf);
	if (ret != 0)
		printf("failed.\n");
	else
		printf("done.\n");

	return ret;
}

static int do_flash_strap_mmc(struct cmd_tbl *cmdtp, int flag,
			      int argc, char *const argv[])
{
	int ret;
	enum command_ret_t cmd_ret = CMD_RET_SUCCESS;
	uint32_t strap_num;
	uint32_t strap_val;

	if (argc != 2 && argc != 4) {
		cmd_ret = CMD_RET_USAGE;
		goto end;
	}

	ret = do_strap_mmc_read();
	if (ret != 0) {
		cmd_ret = CMD_RET_USAGE;
		goto end;
	}

	if (!strcmp(argv[1], "read")) {
		do_strap_dump_info();
	} else if (!strcmp(argv[1], "conf")) {
		strap_num = simple_strtoul(argv[2], NULL, 16);
		strap_val = simple_strtoul(argv[3], NULL, 16);
		ret = do_strap_info_conf(strap_num, strap_val);
		if (ret != 0) {
			cmd_ret = CMD_RET_FAILURE;
			goto end;
		}
	} else if (!strcmp(argv[1], "update")) {
		if (!g_buf) {
			cmd_ret = CMD_RET_FAILURE;
			goto end;
		}

		ret = do_strap_mmc_update(g_buf);
		if (ret != 0) {
			cmd_ret = CMD_RET_FAILURE;
			goto end;
		}
	}

end:
	return cmd_ret;
}

static struct cmd_tbl cmd_flash_strap[] = {
	U_BOOT_CMD_MKENT(sf, 4, 0, do_flash_strap_sf, "", ""),
	U_BOOT_CMD_MKENT(mmc, 4, 0, do_flash_strap_mmc, "", ""),
};

static int do_flash_strap(struct cmd_tbl *cmdtp, int flag, int argc,
			  char *const argv[])
{
	int ret;
	struct cmd_tbl *cb;

	cb = find_cmd_tbl(argv[1], cmd_flash_strap, ARRAY_SIZE(cmd_flash_strap));
	/* Drop the flash strap command */
	argc--;
	argv++;

	if (!cb || argc > cb->maxargs)
		return CMD_RET_USAGE;
	if (flag == CMD_FLAG_REPEAT && !cmd_is_repeatable(cb))
		return CMD_RET_SUCCESS;

	ret = cb->cmd(cmdtp, flag, argc, argv);
	return ret;
}

U_BOOT_CMD(strap,	5,	1,	do_flash_strap,
	   "ASPEED Flash Strap sub-system",
	   "sf read\n"
	   "strap sf config <idx> <value>\n"
	   "strap sf update\n"
	   "strap mmc read\n"
	   "strap mmc config <idx> <value>\n"
	   "strap mmc update\n"
);
