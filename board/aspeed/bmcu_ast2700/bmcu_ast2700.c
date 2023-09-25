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

DECLARE_GLOBAL_DATA_PTR;

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

int board_init(void)
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

int board_late_init(void)
{
	return 0;
}

int spl_board_init_f(void)
{
	dram_init();

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
	return BOOT_DEVICE_RAM;
}

void board_fit_image_post_process(const void *fit, int node, void **p_image, size_t *p_size)
{
	uint64_t ep64;
	uint8_t os;
	ulong ep;

	fit_image_get_os(fit, node, &os);

	if (os != IH_OS_U_BOOT)
		return;

	fit_image_get_entry(fit, node, &ep);

	/* convert to AArch64 view */
	ep64 = ((uint64_t)ep - 0x80000000) | 0x400000000ULL;
	writeq(ep64, (void *)0x12c02780);
}

void spl_board_prepare_for_boot(void)
{
	uint32_t rvbar;

	writel(0x1, (void *)0x12c02014);

	rvbar = (readq((void *)0x12c02780) >> 4) & 0xffffffff;
	writel(rvbar, (void *)0x12c02110);
	writel(rvbar, (void *)0x12c02114);
	writel(rvbar, (void *)0x12c02118);
	writel(rvbar, (void *)0x12c0211c);

	writel(0x1, (void *)0x12c0210c);

	while (1)
		__asm__("wfi");
}
