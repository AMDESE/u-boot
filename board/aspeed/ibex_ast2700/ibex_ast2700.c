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
#include <asm/arch/extrst.h>
#include <asm/arch/platform.h>
#include <asm/arch/recovery.h>
#include <asm/arch/pci_ast2700.h>
#include <asm/arch/scu_ast2700.h>
#include <asm/arch/sli_ast2700.h>
#include <asm/arch/sdram_ast2700.h>
#include <asm/arch/spi.h>
#include <asm/arch/sys_policy.h>
#include <asm/arch/usb_ast2700.h>
#include <asm/arch/wdt.h>
#include <asm/arch/ssp_tsp_ast2700.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <ast_loader.h>

DECLARE_GLOBAL_DATA_PTR;

static bool ibex_boot2fw;
static bool has_pspfw;
static bool has_sspfw;
static bool has_tspfw;

int misc_init(void)
{
	struct udevice *dev;
	int i = 0;
	int ret;

	/* Loop over all MISC uclass drivers */
	while (1) {
		ret = uclass_get_device(UCLASS_MISC, i++, &dev);
		if (ret == -ENODEV)
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
	return ast_get_boot_mode();
}

struct init_callback {
	char name[10];
	int (*init_cb)(void);
};

static int sli1_init(void)
{
	struct udevice *dev;
	int err;

	/* sli1 */
	err = uclass_get_device_by_name(UCLASS_MISC, "sli@14c1e000", &dev);
	if (err && err != -ENODEV)
		printf("Get sli1 udevice Failed %d.\n", err);

	return err;
}

static int sli0_init(void)
{
	struct udevice *dev;
	int err;

	/* sli0 */
	err = uclass_get_device_by_name(UCLASS_MISC, "sli@12c17000", &dev);
	if (err && err != -ENODEV)
		printf("Get sli0 udevice Failed %d.\n", err);

	return err;
}

static int dp_init(void)
{
	struct udevice *dev;
	int err;
	struct ast2700_scu0 *scu = (struct ast2700_scu0 *)ASPEED_CPU_SCU_BASE;

	// leave works to u-boot
	if (FIELD_GET(SCU_CPU_REVISION_ID_HW, scu->chip_id1) == 0) {
		debug("%s: Do nothing in A0\n", __func__);
		return 0;
	}

	/* dp */
	err = uclass_get_device_by_name(UCLASS_MISC, "dp@12c0a000", &dev);
	if (err && err != -ENODEV)
		printf("Get dp udevice Failed %d.\n", err);

	return err;
}

static void pci_check_prop(int node, void *reg, const char *prop, int bit)
{
	const char *val;
	int len;

	val = fdt_getprop(gd->fdt_blob, node, prop, &len);
	if (!val) {
		clrbits_le32(reg, BIT(bit) | BIT(bit + 8) | BIT(bit + 16) | BIT(bit + 24));
		return;
	}

	if (!strcmp(val, "MSI"))
		setbits_le32(reg, BIT(bit) | BIT(bit + 8) | BIT(bit + 16) | BIT(bit + 24));
	else
		setbits_le32(reg, BIT(bit) | BIT(bit + 8) | BIT(bit + 16));
}

static void pci_config_ep(void *msi_cap, const char *name, bool alt_pcie_node)
{
	int nodeoffset;
	ofnode node;
	const char *str;

	nodeoffset = fdt_node_offset_by_compatible(gd->fdt_blob, -1,
						   "aspeed,ast27xx-pcie");
	if (nodeoffset < 0) {
		printf("%s: failed to get aspeed,ast27xx-pcie\n", __func__);
		return;
	}

	while (nodeoffset >= 0) {
		node = offset_to_ofnode(nodeoffset);
		str = ofnode_get_name(node);
		if (str && !strcmp(str, name))
			break;

		nodeoffset = fdt_node_offset_by_compatible(gd->fdt_blob, nodeoffset,
							   "aspeed,ast27xx-pcie");
	}

	if (nodeoffset < 0) {
		printf("%s: failed to get %s\n", __func__, name);
		return;
	}

	pci_check_prop(nodeoffset, msi_cap, "vga", 0);
	pci_check_prop(nodeoffset, msi_cap, "bmc-device", 1);
	pci_check_prop(nodeoffset, msi_cap, "ehci", 2);
	pci_check_prop(nodeoffset, msi_cap, "xhci", 3);

	if (alt_pcie_node &&
	    fdt_getprop(gd->fdt_blob, nodeoffset, "alt-pcie-node", NULL)) {
		// disable vga & bmc-dev if alt_pcie_node is set
		clrbits_le32(msi_cap, 0x03030303);
		clrbits_le32(msi_cap + 8, BIT(31));
	}
}

static int pci_init(void)
{
	struct ast2700_scu0 *scu = (struct ast2700_scu0 *)ASPEED_CPU_SCU_BASE;

	// leave works to u-boot
	if (FIELD_GET(SCU_CPU_REVISION_ID_HW, scu->chip_id1) == 0) {
		debug("%s: Do nothing in A0\n", __func__);
		return 0;
	}

	/* cpu-die pcie node 1 */
	// setup preset for plda2
	writel(0x12600000, (void *)ASPEED_PLDA2_PRESET0);
	writel(0x00012600, (void *)ASPEED_PLDA2_PRESET1);

	// Enable Bridge2 MSI
	clrbits_le32((void *)ASPEED_PLDA2_MSI_CAP, BIT(3));
	// Set Bridge2 INTA
	clrsetbits_le32((void *)ASPEED_PLDA2_MSI_CAP, GENMASK(2, 0), 0x1);

	pci_config_ep(&scu->pci1_misc[28], "pcie@12c15800", true);

	// clk/reset for e2m
	setbits_le32(&scu->clkgate_clr, SCU_CPU_CLKGATE1_E2M1);
	mdelay(10);
	setbits_le32(&scu->modrst2_clr, SCU_CPU_RST2_E2M1);

	/* cpu-die pcie node 0 */
	// setup preset for plda1
	writel(0x12600000, (void *)ASPEED_PLDA1_PRESET0);
	writel(0x00012600, (void *)ASPEED_PLDA1_PRESET1);

	// Enable Bridge1 MSI
	clrbits_le32((void *)ASPEED_PLDA1_MSI_CAP, BIT(3));
	// Set Bridge1 INTA
	clrsetbits_le32((void *)ASPEED_PLDA1_MSI_CAP, GENMASK(2, 0), 0x1);

	pci_config_ep(&scu->pci0_misc[28], "pcie@12c15000", false);

	// clk/reset for e2m
	setbits_le32(&scu->clkgate_clr, SCU_CPU_CLKGATE1_E2M0);
	mdelay(10);
	setbits_le32(&scu->modrst2_clr, SCU_CPU_RST2_E2M0);

	/* io-die pcie node */
	// Enable Bridge3 MSI
	clrbits_le32((void *)ASPEED_PLDA3_MSI_CAP, BIT(3));
	// Set Bridge3 INTA
	clrsetbits_le32((void *)ASPEED_PLDA3_MSI_CAP, GENMASK(2, 0), 0x1);

	return 0;
}

static int sram_prictrl_init(void)
{
	struct udevice *dev;
	int err = 0;

	/* privilege control init */
	err = uclass_get_device_by_name(UCLASS_MISC, "sram_prictrl@12c0e000", &dev);
	if (err && err != -ENODEV)
		printf("Get SRAM privilege control udevice Failed %d.\n", err);

	return err;
}

static int prictrl_init(void)
{
	struct udevice *dev;
	int err = 0;

	if (!has_pspfw)
		return err;

	/* privilege control init */
	err = uclass_get_device_by_name(UCLASS_MISC, "prictrl@12140000", &dev);
	if (err && err != -ENODEV)
		printf("Get privilege control udevice Failed %d.\n", err);

	return err;
}

int dram_init(void)
{
	struct udevice *dev;
	int err;

	err = uclass_get_device_by_name(UCLASS_RAM, "sdrammc@12c00000", &dev);
	if (err && err != -ENODEV)
		printf("Get sdrammc udevice Failed %d.\n", err);

	return err;
}

static int vbios_init(void)
{
	struct udevice *dev;
	int err = 0;

	/* load vbios init */
	err = uclass_get_device_by_name(UCLASS_MISC, "vbios@0", &dev);
	if (err && err != -ENODEV)
		printf("Get VBIOS udevice Failed %d.\n", err);

	return err;
}

static int hspi_init(void)
{
	u32 val;

	val = readl((void *)(ASPEED_SPI0_REG_BASE + MISC_CTRL_REG));
	val |= SPI_USER_CMD_MODE | SPI_UNALGNED_ACCESS;
	val &= ~SPI_CS_CONTINUOUS;
	writel(val, (void *)(ASPEED_SPI0_REG_BASE + MISC_CTRL_REG));

	val = readl((void *)(ASPEED_SPI1_REG_BASE + MISC_CTRL_REG));
	val |= SPI_USER_CMD_MODE | SPI_UNALGNED_ACCESS;
	val &= ~SPI_CS_CONTINUOUS;
	writel(val, (void *)(ASPEED_SPI1_REG_BASE + MISC_CTRL_REG));

	return 0;
}

static int usb_init(void)
{
	int nodeoffset;
	ofnode node;
	struct ast2700_scu0 *scu;

	/* find the offset of compatible node */
	nodeoffset = fdt_node_offset_by_compatible(gd->fdt_blob, -1,
						   "aspeed,ast2700-scu0");
	if (nodeoffset < 0)
		printf("%s: failed to get aspeed,ast2700-scu0\n", __func__);

	/* get ast2700-scu0 node */
	node = offset_to_ofnode(nodeoffset);

	scu = (struct ast2700_scu0 *)ofnode_get_addr(node);
	if (IS_ERR_OR_NULL(scu))
		printf("%s: cannot get SYSCON address pointer\n", __func__);

	if (FIELD_GET(SCU_CPU_REVISION_ID_HW, scu->chip_id1) == 0) {
		debug("%s: Do nothing in A0\n", __func__);
		return 0;
	}

	/* clk/reset for vhuba1 (including PortA 2.0 PHY) */
	setbits_le32(&scu->clkgate_clr, SCU_CPU_CLKGATE1_USBA);
	mdelay(10);
	setbits_le32(&scu->modrst2_clr, SCU_CPU_RST2_USBA_VHUB);
	mdelay(1);

	/* Set PortA PHY2 Pre-emphasis current {PHYA_B08} [22:21] = b'10 */
	clrsetbits_le32((void *)ASPEED_VHUBA1_PHY_CTL_3, GENMASK(22, 21), 2 << 21);

	/* clk/reset for vhubb1 (including PortB 2.0 PHY) */
	setbits_le32(&scu->clkgate_clr, SCU_CPU_CLKGATE1_USBB);
	mdelay(10);
	setbits_le32(&scu->modrst2_clr, SCU_CPU_RST2_USBB_VHUB);
	mdelay(1);

	/* Set PortB PHY2 Pre-emphasis current {PHYA_B08} [22:21] = b'10 */
	clrsetbits_le32((void *)ASPEED_VHUBB1_PHY_CTL_3, GENMASK(22, 21), 2 << 21);

	return 0;
}

static int otp_init(void)
{
	struct udevice *dev;
	int err;

	err = uclass_get_device_by_name(UCLASS_MISC, "otp@14c07000", &dev);
	if (err && err != -ENODEV)
		printf("Get OTP udevice Failed %d.\n", err);

	return err;
}

struct init_callback board_init_seq[] = {
	{"POLICY",	sys_policy_init},
	{"WDT",		wdt_init},
	{"OTP",		otp_init},
	{"EXTRST",	extrst_mask_init},
	{"STOR",	ast_loader_init},
	{"SLI1",	sli1_init},
	{"DP",		dp_init},
	{"SLI0",	sli0_init},
	{"HSPI",	hspi_init},
	{"DRAM",	dram_init},
	{"PCI",		pci_init},
	{"VBIOS",	vbios_init},
	{"USB",		usb_init}
};

int spl_board_init_f(void)
{
	struct udevice *dev;
	int err;
	int i;

	/* scu1 initial for driving and clk*/
	err = uclass_get_device_by_name(UCLASS_CLK, "clock-controller@14c02200", &dev);
	if (err && err != -ENODEV)
		printf("Get soc1 clk udevice Failed %d.\n", err);

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
	ulong load_addr;
	const char *name;
	int len;

	fit_image_get_arch(fit, node, &arch);
	fit_image_get_type(fit, node, &type);
	fit_image_get_os(fit, node, &os);
	fit_image_get_entry(fit, node, &ep);
	name = fdt_getprop(fit, node, "description", &len);

	/* ibex firmware recognized */
	if (arch == IH_ARCH_RISCV && type == IH_TYPE_FIRMWARE) {
		ibex_boot2fw = true;
		return;
	}

	if (strncmp(name, "SSP", 3) == 0) {
		fit_image_get_load(fit, node, &load_addr);
		ssp_init(load_addr);
		has_sspfw = true;
		return;
	}

	if (strncmp(name, "TSP", 3) == 0) {
		fit_image_get_load(fit, node, &load_addr);
		tsp_init(load_addr);
		has_tspfw = true;
		return;
	}

	/* convert to Arm view */
	ep_arm = ((uint64_t)ep - 0x80000000) | 0x400000000ULL;

	switch (os) {
	case IH_OS_ARM_TRUSTED_FIRMWARE:
		has_pspfw = true;
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

#define ASPEED_UFS_PATH_AXI	(0x12c080e4)

void spl_board_prepare_for_boot(void)
{
	struct udevice *dev;

	if (!uclass_get_device_by_name(UCLASS_RAM, "sdrammc@12c00000", &dev))
		sdramc_mpu_enable(dev);

	/* for v7 FPGA only to switch to uart12. */
	if (IS_ENABLED(CONFIG_ASPEED_FPGA))
		writel(SCU_CPU_HWSTRAP_DIS_CPU, (void *)ASPEED_CPU_HW_STRAP1_CLR);

	if (IS_ENABLED(CONFIG_ASPEED_UFS))
		writel(1, (void *)ASPEED_UFS_PATH_AXI);

	if (has_pspfw) {
		/* clean up secondary entries */
		writeq(0x0, (void *)ASPEED_CPU_SMP_EP1);
		writeq(0x0, (void *)ASPEED_CPU_SMP_EP2);
		writeq(0x0, (void *)ASPEED_CPU_SMP_EP3);

		/* release CA35 reset */
		writel(0x1, (void *)ASPEED_CPU_CA35_REL);
	}

	/* Enable privilege control */
	prictrl_init();

	/* Enable SRAM privilege control */
	sram_prictrl_init();

	/* release SSP reset */
	if (has_sspfw)
		ssp_enable();

	/* release TSP reset */
	if (has_tspfw)
		tsp_enable();

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
