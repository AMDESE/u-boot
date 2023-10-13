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
#include <init.h>

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

	if (IS_ENABLED(CONFIG_PCI_ENDPOINT))
		pci_ep_init();

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
