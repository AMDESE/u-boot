// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */
#include <common.h>
#include <debug_uart.h>
#include <dm.h>
#include <spl.h>
#include <init.h>
#include <hang.h>
#include <image.h>
#include <linux/err.h>
#include <asm/io.h>
#include <asm/arch/scu_ast2600.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

void board_init_f(ulong dummy)
{
	spl_early_init();
	preloader_console_init();
	timer_init();
	dram_init();
}

/*
 * Try to detect the boot mode. Fallback to the default,
 * memory mapped SPI XIP booting if detection failed.
 */
u32 spl_boot_device(void)
{
	int rc;
	struct udevice *scu_dev;
	struct ast2600_scu *scu;

	rc = uclass_get_device_by_driver(UCLASS_CLK,
					 DM_DRIVER_GET(aspeed_ast2600_scu), &scu_dev);
	if (rc) {
		debug("%s: failed to get SCU driver\n", __func__);
		goto out;
	}

	scu = devfdt_get_addr_ptr(scu_dev);
	if (IS_ERR_OR_NULL(scu)) {
		debug("%s: failed to get SCU base\n", __func__);
		goto out;
	}

	/* boot from UART has higher priority */
	if (scu->hwstrap2 & SCU_HWSTRAP2_BOOT_UART)
		return BOOT_DEVICE_UART;

	if (scu->hwstrap1 & SCU_HWSTRAP1_BOOT_EMMC)
		return BOOT_DEVICE_MMC1;

out:
	return BOOT_DEVICE_RAM;
}

struct legacy_img_hdr *spl_get_load_buffer(ssize_t offset, size_t size)
{
	return (struct legacy_img_hdr *)(CONFIG_SYS_LOAD_ADDR);
}

#ifdef CONFIG_SPL_OS_BOOT
int spl_start_uboot(void)
{
	/* boot linux */
	return 0;
}
#endif

int spl_fit_images_get_uboot_entry(void *blob, ulong *entry)
{
	int parent, node, ndepth;
	const void *data;
	int rc;

	if (!blob)
		return -FDT_ERR_BADMAGIC;

	parent = fdt_path_offset(blob, "/fit-images");
	if (parent < 0)
		return -FDT_ERR_NOTFOUND;

	for (node = fdt_next_node(blob, parent, &ndepth);
	     (node >= 0) && (ndepth > 0);
	     node = fdt_next_node(blob, node, &ndepth)) {
		if (ndepth != 1)
			continue;

		data = fdt_getprop(blob, node, FIT_OS_PROP, NULL);
		if (!data)
			continue;

		if (genimg_get_os_id(data) == IH_OS_U_BOOT)
			break;
	};

	if (node == -FDT_ERR_NOTFOUND)
		return -FDT_ERR_NOTFOUND;

	rc = fit_image_get_entry(blob, node, entry);
	if (rc)
		rc = fit_image_get_load(blob, node, entry);

	return rc;
}

void spl_optee_entry(void *arg0, void *arg1, void *arg2, void *arg3)
{
	ulong optee_ns_ep;
	int rc;

	rc = spl_fit_images_get_uboot_entry(arg2, &optee_ns_ep);
	if (rc)
		goto err_hang;

	asm volatile(
		"mov lr, %[ns_ep]\n\t"
		"mov r0, %[a0]\n\t"
		"mov r1, %[a1]\n\t"
		"mov r2, %[a2]\n\t"
		"bx %[a3]\n\t"
		:
		: [ns_ep]"r" (optee_ns_ep), [a0]"r" (arg0), [a1]"r" (arg1), [a2]"r" (arg2), [a3]"r" (arg3)
		: "lr", "r0", "r1", "r2"
	);

err_hang:
	debug("cannot find NS image entry for OPTEE\n");
	hang();
}

void board_fit_image_post_process(const void *fit, int node, void **p_image, size_t *p_size)
{
	struct udevice *scu_dev;
	struct ast2600_scu *scu;
	ulong s_ep;
	uint8_t os;
	int rc;

	fit_image_get_os(fit, node, &os);

	/* skip if no TEE */
	if (os != IH_OS_TEE)
		return;

	rc = uclass_get_device_by_driver(UCLASS_CLK,
					 DM_DRIVER_GET(aspeed_ast2600_scu), &scu_dev);
	if (rc) {
		debug("%s: failed to get SCU driver\n", __func__);
		return;
	}

	scu = devfdt_get_addr_ptr(scu_dev);
	if (IS_ERR_OR_NULL(scu)) {
		debug("%s: failed to get SCU base\n", __func__);
		return;
	}

	rc = fit_image_get_entry(fit, node, &s_ep);
	if (rc) {
		debug("%s: failed to get secure entrypoint\n", __func__);
		return;
	}

	/* set & lock secure entrypoint for secondary cores */
	writel(s_ep, &scu->smp_boot[15]);
	writel(BIT(17) | BIT(18) | BIT(19), &scu->wr_prot2);
}

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	/* just empty function now - can't decide what to choose */
	debug("%s: %s\n", __func__, name);
	return 0;
}
#endif
