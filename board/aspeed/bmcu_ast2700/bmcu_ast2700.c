// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */
#include <asm/io.h>
#include <dm.h>
#include <ram.h>
#include <spl.h>
#include <image.h>
#include <common.h>
#include <asm/csr.h>
#include <asm/arch-aspeed/platform.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>

DECLARE_GLOBAL_DATA_PTR;


#define SCU_CPU_HWSTRAP1	((void *)0x12c02010)
#define SCU_CPU_HWSTRAP1_CLR	((void *)0x12c02014)
#define SCU_CPU_CLK_CLR		((void *)0x12c02244)
#define SCU_CPU_VGA_FUNC	((void *)0x12c02414)
#define   VGA_DAC_OUTPUT	GENMASK(11, 10)
#define   VGA_DP_OUTPUT		GENMASK(9, 8)
#define   VGA_DAC_DISABLE	BIT(7)
#define SCU_CPU_VGA0_SAR0	((void *)0x12c02a0c)
#define SCU_PCI_MISC70		((void *)0x12c02a70)
#define SCU_CPU_VGA1_SAR0	((void *)0x12c02a8c)
#define SCU_PCI_MISCF0		((void *)0x12c02af0)

// for SCU_CPU_VGAx_SAR0
#define   VGA_FB_SIZE		GENMASK(4, 0)

int dram_init(void)
{
	int ret;
	struct udevice *dev;
	struct ram_info ram;

	ret = uclass_get_device(UCLASS_RAM, 0, &dev);
	if (ret) {
		printf("cannot get DRAM driver\n");
		return ret;
	}

	ret = ram_get_info(dev, &ram);
	if (ret) {
		printf("cannot get DRAM information\n");
		return ret;
	}

	gd->ram_size = ram.size;

	return 0;
}

int misc_init(void)
{
	struct udevice *dev;
	int i = 0;
	int ret;

	/*
	 * Loop over all MISC uclass drivers to call the comphy code
	 * and init all CP110 devices enabled in the DT
	 */
	while (1) {
		/* Call the comphy code via the MISC uclass driver */
		ret = uclass_get_device(UCLASS_MISC, i++, &dev);

		/* We're done, once no further CP110 device is found */
		if (ret)
			break;
	}

	return 0;
}

// To support 64bit address calculation for e2m
static u32 _ast_get_e2m_addr(u32 addr)
{
	u32 val;

	val = readl((void *)0x12c00010) >> 2 & 0x07;

	debug("%s: DRAMC val(%x)\n", __func__, val);
	return (((1 << val) - 1) << 24) | (addr >> 4);
}

int pci_vga_init(void)
{
	u32 val, vram_size, vram_addr;
	u8 vram_size_cfg;
	bool is_pcie0_enable = readl(SCU_PCI_MISC70) & BIT(0);
	bool is_pcie1_enable = readl(SCU_PCI_MISCF0) & BIT(0);
	bool is_64vram = readl((void *)0x12c00100) & BIT(0);
	u8 dac_src = readl(SCU_CPU_HWSTRAP1) & BIT(28);
	u8 dp_src = readl(SCU_CPU_HWSTRAP1) & BIT(29);

	debug("%s: ENABLE 0(%d) 1(%d)\n", __func__, is_pcie0_enable, is_pcie1_enable);

	// for CRAA[1:0]
	setbits_le32(SCU_CPU_HWSTRAP1, BIT(11));
	if (is_64vram)
		setbits_le32(SCU_CPU_HWSTRAP1, BIT(10));
	else
		setbits_le32(SCU_CPU_HWSTRAP1_CLR, BIT(10));

	vram_addr = 0x10000000;

	vram_size_cfg = is_64vram ? 0xf : 0xe;
	vram_size = 2 << (vram_size_cfg + 10);
	debug("%s: VRAM size(%x) cfg(%x)\n", __func__, vram_size, vram_size_cfg);

	if (is_pcie0_enable) {
		// enable clk
		setbits_le32(SCU_CPU_CLK_CLR, BIT(5));

		vram_addr -= vram_size;
		debug("pcie0 e2m addr(%x)\n", _ast_get_e2m_addr(vram_addr));
		val = _ast_get_e2m_addr(vram_addr) | FIELD_PREP(VGA_FB_SIZE, vram_size_cfg);
		debug("pcie0 debug reg(%x)\n", val);
		writel(val, (void *)0x12c21100);
		writel(val, SCU_CPU_VGA0_SAR0);

		// Enable VRAM address offset: cursor, rvas, 2d
		writel(BIT(10) | BIT(24) | BIT(27), (void *)0x12c00104);
	}

	if (is_pcie1_enable) {
		// enable clk
		setbits_le32(SCU_CPU_CLK_CLR, BIT(10));

		vram_addr -= vram_size;
		debug("pcie1 e2m addr(%x)\n", _ast_get_e2m_addr(vram_addr));
		val = _ast_get_e2m_addr(vram_addr) | FIELD_PREP(VGA_FB_SIZE, vram_size_cfg);
		debug("pcie1 debug reg(%x)\n", val);
		writel(val, (void *)0x12c22100);
		writel(val, SCU_CPU_VGA1_SAR0);

		// Enable VRAM address offset: cursor, rvas, 2d
		writel(BIT(19) | BIT(25) | BIT(28), (void *)0x12c00108);
	}

	if (is_pcie0_enable || is_pcie1_enable) {
		// enable dac clk
		setbits_le32(SCU_CPU_CLK_CLR, BIT(17));

		val = readl(SCU_CPU_VGA_FUNC);
		val &= ~(VGA_DAC_OUTPUT | VGA_DP_OUTPUT | VGA_DAC_DISABLE);
		val |= FIELD_PREP(VGA_DAC_OUTPUT, dac_src)
		     | FIELD_PREP(VGA_DP_OUTPUT, dp_src)
		     | FIELD_PREP(VGA_DAC_DISABLE, 0);
		writel(val, SCU_CPU_VGA_FUNC);

		// vga link init
		writel(0x00030009, (void *)0x12c1d010);
		val = 0x10000000 | dac_src;
		writel(val, (void *)0x12c1d050);
		writel(0x00010002, (void *)0x12c1d044);
		writel(0x00030009, (void *)0x12c1d110);
		writel(0x00030009, (void *)0x14c3a010);
		writel(0x00030009, (void *)0x14c3a110);
	}

	return 0;
}

int board_init(void)
{
	misc_init();

	return 0;
}

int board_late_init(void)
{
	return 0;
}

int spl_mmc_init(void)
{
	int ret;
	struct udevice *dev;

	/* release emmc pin from emmc boot */
	writel(0, (void *)0x12c0b00c);

	printf("spl probe blk\n");

	ret = uclass_get_device(UCLASS_BLK, 0, &dev);
	if (ret)
		printf("cannot get BLK driver\n");

	return ret;
}

int spl_board_init_f(void)
{
	/* Probe block driver to bring up mmc */
	if (spl_boot_device() == BOOT_DEVICE_MMC1)
		spl_mmc_init();

	dram_init();

	pci_vga_init();

	misc_init();

	return 0;
}

struct legacy_img_hdr *spl_get_load_buffer(ssize_t offset, size_t size)
{
	return (struct legacy_img_hdr *)CONFIG_SYS_LOAD_ADDR;
}

void *board_spl_fit_buffer_addr(ulong fit_size, int sectors, int bl_len)
{
	return (void *)spl_get_load_buffer(sectors, bl_len);
}

u32 spl_boot_device(void)
{
	if ((readl((void *)ASPEED_IO_HW_STRAP1) & STRAP_BOOTMODE_BIT))
		return BOOT_DEVICE_MMC1;
	else
		return BOOT_DEVICE_RAM;
}

void board_fit_image_post_process(const void *fit, int node, void **p_image, size_t *p_size)
{
	uint64_t ep_arm;
	uint8_t os;
	ulong ep;

	fit_image_get_os(fit, node, &os);
	fit_image_get_entry(fit, node, &ep);

	/* convert to Arm view */
	ep_arm = ((uint64_t)ep - 0x80000000) | 0x400000000ULL;

	switch (os) {
	case IH_OS_ARM_TRUSTED_FIRMWARE:
		writel(ep_arm >> 4, (void *)0x12c02110);
		writel(ep_arm >> 4, (void *)0x12c02114);
		writel(ep_arm >> 4, (void *)0x12c02118);
		writel(ep_arm >> 4, (void *)0x12c0211c);
		break;
	case IH_OS_U_BOOT:
		writeq(ep_arm, (void *)0x12c02780);
		break;
	default:
		break;
	}
}

void spl_board_prepare_for_boot(void)
{
	/* for v7 FPGA only */
	writel(0x1, (void *)0x12c02014);

	/* clean up secondary entries */
	writeq(0x0, (void *)0x12c02788);
	writeq(0x0, (void *)0x12c02790);
	writeq(0x0, (void *)0x12c02798);

	/* release CA35 reset */
	writel(0x1, (void *)0x12c0210c);

	/* sleep well */
	while (1)
		__asm__("wfi");
}

#define CSR_MCYCLE 0xb00
#define CSR_MCYCLEH 0xb80
unsigned long timer_read_counter(void)
{
	uint32_t tl, th;

	tl = csr_read(CSR_MCYCLE);
	th = csr_read(CSR_MCYCLEH);

	return (((uint64_t)th) << 32 | tl);
}
