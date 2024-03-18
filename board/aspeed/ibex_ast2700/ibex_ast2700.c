// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */
#include <asm/io.h>
#include <dm.h>
#include <ram.h>
#include <spl.h>
#include <image.h>
#include <binman_sym.h>
#include <common.h>
#include <asm/csr.h>
#include <asm/arch-aspeed/platform.h>
#include <asm/arch-aspeed/dp_ast2700.h>
#include <asm/arch-aspeed/e2m_ast2700.h>
#include <asm/arch-aspeed/recovery.h>
#include <asm/arch-aspeed/scu_ast2700.h>
#include <asm/arch-aspeed/sli_ast2700.h>
#include <asm/arch-aspeed/ltpi_ast2700.h>
#include <asm/arch-aspeed/sdram_ast2700.h>
#include <asm/arch-aspeed/stor_ast2700.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/delay.h>

DECLARE_GLOBAL_DATA_PTR;

static bool ibex_boot2fw;

int misc_init(void)
{
	struct udevice *dev;
	int i = 0;
	int ret;

	/* Loop over all MISC uclass drivers */
	while (1) {
		ret = uclass_get_device(UCLASS_MISC, i++, &dev);
		if (ret)
			break;
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

u32 spl_boot_device(void)
{
	if (is_recovery())
		return BOOT_DEVICE_UART;
	else if ((readl((void *)ASPEED_IO_HW_STRAP1) & SCU_IO_HWSTRAP_EMMC)) {
		if (readl((void *)ASPEED_IO_HW_STRAP1) & SCU_IO_HWSTRAP_UFS)
			return BOOT_DEVICE_SATA;

		return BOOT_DEVICE_MMC1;
	}
	else
		return BOOT_DEVICE_RAM;
}

struct init_callback {
	char name[10];
	int (*init_cb)(void);
};

struct init_callback board_init_seq[] = {
	{"SLI",		sli_init},
	{"LTPI",	ltpi_init},
	{"STOR",	stor_init},
	{"DRAM",	dram_init},
	{"VGA",		pci_vga_init},
};

int spl_board_init_f(void)
{
	int err;
	int i;

	for (i = 0; i < ARRAY_SIZE(board_init_seq); i++) {

		err = board_init_seq[i].init_cb();
		if (err)
			printf("%s: %s init failed.\n", __func__, board_init_seq[i].name);
	}

	if (CONFIG_IS_ENABLED(MISC))
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

void board_fit_image_post_process(const void *fit, int node, void **p_image, size_t *p_size)
{
	uint64_t ep_arm;
	uint8_t arch;
	uint8_t type;
	uint8_t os;
	ulong ep;

	fit_image_get_arch(fit, node, &arch);
	fit_image_get_type(fit, node, &type);
	fit_image_get_os(fit, node, &os);
	fit_image_get_entry(fit, node, &ep);

	/* ibex firmware recognized */
	if (arch == IH_ARCH_RISCV && type == IH_TYPE_FIRMWARE) {
		ibex_boot2fw = true;
		return;
	}

	/* convert to Arm view */
	ep_arm = ((uint64_t)ep - 0x80000000) | 0x400000000ULL;

	switch (os) {
	case IH_OS_ARM_TRUSTED_FIRMWARE:
		writel(ep_arm >> 4, (void *)ASPEED_CPU_CA35_RVBAR0);
		writel(ep_arm >> 4, (void *)ASPEED_CPU_CA35_RVBAR1);
		writel(ep_arm >> 4, (void *)ASPEED_CPU_CA35_RVBAR2);
		writel(ep_arm >> 4, (void *)ASPEED_CPU_CA35_RVBAR3);
		break;
	case IH_OS_U_BOOT:
		writeq(ep_arm, (void *)ASPEED_CPU_SMP_EP0);
		break;
	default:
		break;
	}
}

void spl_board_prepare_for_boot(void)
{
	/* for v7 FPGA only to switch to uart12. */
	if (IS_ENABLED(CONFIG_ASPEED_FPGA))
		writel(SCU_CPU_HWSTRAP_DIS_CPU, (void *)ASPEED_CPU_HW_STRAP1_CLR);

	/* clean up secondary entries */
	writeq(0x0, (void *)ASPEED_CPU_SMP_EP1);
	writeq(0x0, (void *)ASPEED_CPU_SMP_EP2);
	writeq(0x0, (void *)ASPEED_CPU_SMP_EP3);

	/* release CA35 reset */
	writel(0x1, (void *)ASPEED_CPU_CA35_REL);

	/* keep going if ibex has further FW to run */
	if (ibex_boot2fw) {
		/* A0 workaround: delay for CA35 ATF to fine-tune SLI */
		mdelay(100);

		return;
	}

	/* sleep well otherwise */
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

int spl_parse_board_header(struct spl_image_info *spl_image,
			   const struct spl_boot_device *bootdev,
			   const void *image_header, size_t size)
{
	return 0;
}
