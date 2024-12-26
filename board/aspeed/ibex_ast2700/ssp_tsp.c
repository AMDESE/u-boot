// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */
#include <asm/io.h>
#include <asm/arch-aspeed/scu_ast2700.h>
#include <asm/arch-aspeed/platform.h>
#include <dm.h>
#include <fdt_support.h>
#include <linux/delay.h>
#include <linux/libfdt.h>

/* MAX visible range is 512M for SSP and TSP */
#define MAX_I_D_ADDRESS		(512 * 1024 * 1024)

#define SSP_MEMORY_NODE		"/reserved-memory/ssp-memory"
#define TSP_MEMORY_NODE		"/reserved-memory/tsp-memory"
#define ATF_MEMORY_NODE		"/reserved-memory/trusted-firmware-a"
#define OPTEE_MEMORY_NODE	"/reserved-memory/optee-core"

struct mem_info {
	ulong base;
	size_t size;
};

static struct mem_info get_reserved_memory(const char *path)
{
	struct mem_info info = { -1, -1 };
	const fdt32_t *reg;
	const void *fdt = gd->fdt_blob;
	int offset;

	/* Find the node in the device tree */
	offset = fdt_path_offset(fdt, path);
	if (offset < 0) {
		debug("Cannot find node %s in the device tree.\n", path);
		return info;
	}

	reg = fdt_getprop(fdt, offset, "reg", NULL);
	info.base = (ulong)fdt32_to_cpu(reg[0]);
	info.size = (size_t)fdt32_to_cpu(reg[1]);

	return info;
}

int ssp_init(ulong load_addr)
{
	struct ast2700_scu0 *scu;
	struct mem_info info;
	uint32_t reg_val;
	uint64_t phy_addr;

	info = get_reserved_memory(SSP_MEMORY_NODE);
	if (info.base != load_addr) {
		debug("FIT load address %08lx doesn't match SSP reserved memory %08lx\n",
		      load_addr, info.base);
		return -1;
	}

	scu = (struct ast2700_scu0 *)ASPEED_CPU_SCU_BASE;

	reg_val = readl((void *)&scu->ssp_ctrl_1);
	if (!(reg_val & SCU_CPU_SSP_TSP_RESET_STS))
		return 0;

	writel(SCU_CPU_RST_SSP, (void *)&scu->modrst1_ctrl);
	writel(SCU_CPU_RST_SSP, (void *)&scu->modrst1_clr);

	reg_val = SCU_CPU_SSP_TSP_NIDEN | SCU_CPU_SSP_TSP_DBGEN |
		  SCU_CPU_SSP_TSP_DBG_ENABLE | SCU_CPU_SSP_TSP_RESET;
	writel(reg_val, (void *)&scu->ssp_ctrl_1);

	/*
	 * SSP Memory Map:
	 * - 0x0000_0000 - 0x0507_FFFF: ssp_remap2 -> DRAM[load_addr]
	 * - 0x0508_0000 - 0x1FFF_FFFF: ssp_remap1 -> AHB -> DRAM[0]
	 * - 0x2000_0000 - 0x2000_2000: ssp_remap0 -> TCM (Not used)
	 *
	 * The SSP serves as the secure loader for TSP, ATF, OP-TEE, and U-Boot.
	 * Therefore, their load buffers must be visible to the SSP.
	 *
	 * - SSP remap entry #2 (ssp_remap2_base/size) maps the load buffers
	 *   for SSP, TSP, ATF, and OP-TEE. Ensure these buffers are contiguous.
	 * - SSP remap entry #1 (ssp_remap1_base/size) maps the load buffer
	 *   for U-Boot at DRAM offset 0x0.
	 * - SSP remap entry #0 (ssp_remap0_base/size) maps TCM, which is not used.
	 */
	writel(0, (void *)&scu->ssp_remap2_base);
	reg_val = info.size;
	info = get_reserved_memory(TSP_MEMORY_NODE);
	reg_val += info.size;
	info = get_reserved_memory(ATF_MEMORY_NODE);
	reg_val += info.size;
	info = get_reserved_memory(OPTEE_MEMORY_NODE);
	reg_val += info.size;
	writel(reg_val, (void *)&scu->ssp_remap2_size);

	writel(reg_val, (void *)&scu->ssp_remap1_base);
	writel(MAX_I_D_ADDRESS - reg_val, (void *)&scu->ssp_remap1_size);

	writel(MAX_I_D_ADDRESS, (void *)&scu->ssp_remap0_base);
	writel(0x0, (void *)&scu->ssp_remap0_size);

	/* Configure physical AHB remap: through H2M, mapped to SYS_DRAM_BASE */
	writel((uint32_t)(0x400000000ULL >> 4), (void *)&scu->ssp_ctrl_2);

	/* Configure physical DRAM remap */
	phy_addr = ((uint64_t)load_addr - ASPEED_DRAM_BASE) | 0x400000000ULL;
	reg_val = (uint32_t)(phy_addr >> 4);
	writel(reg_val, (void *)&scu->ssp_ctrl_3);

	/* Enable 1st i-cache area */
	writel(BIT(0), (void *)&scu->ssp_ctrl_4);

	/* Enable 1st d-cache area */
	writel(BIT(0), (void *)&scu->ssp_ctrl_5);

	/* Disable i & d cache by default */
	writel(0x0, (void *)&scu->ssp_ctrl_7);

	return 0;
}

int ssp_enable(void)
{
	struct ast2700_scu0 *scu;

	scu = (struct ast2700_scu0 *)ASPEED_CPU_SCU_BASE;
	setbits_le32((void *)&scu->ssp_ctrl_1, SCU_CPU_SSP_TSP_ENABLE);

	/* HW auto de-asserts SSP reset when WDT timeout reset occurs */
	clrbits_le32((void *)&scu->ssp_ctrl_1, SCU_CPU_SSP_TSP_RESET);

	return 0;
}

int tsp_init(ulong load_addr)
{
	struct ast2700_scu0 *scu;
	struct mem_info info = { -1, -1 };
	uint32_t reg_val;
	uint64_t phy_addr;

	info = get_reserved_memory(TSP_MEMORY_NODE);
	if (info.base != load_addr) {
		debug("FIT load address %08lx doesn't match TSP reserved memory %08lx\n",
		      load_addr, info.base);
		return -1;
	}

	scu = (struct ast2700_scu0 *)ASPEED_CPU_SCU_BASE;

	reg_val = readl((void *)&scu->tsp_ctrl_1);
	if (!(reg_val & SCU_CPU_SSP_TSP_RESET_STS))
		return 0;

	writel(SCU_CPU_RST2_TSP, (void *)&scu->modrst2_ctrl);
	writel(SCU_CPU_RST2_TSP, (void *)&scu->modrst2_clr);

	reg_val = SCU_CPU_SSP_TSP_NIDEN | SCU_CPU_SSP_TSP_DBGEN |
		  SCU_CPU_SSP_TSP_DBG_ENABLE | SCU_CPU_SSP_TSP_RESET;
	writel(reg_val, (void *)&scu->tsp_ctrl_1);

	/* TSP 0x0000_0000 - 0x0200_0000 -> DRAM */
	writel(info.size, (void *)&scu->tsp_remap_size);

	/* Configure physical DRAM remap */
	phy_addr = ((uint64_t)info.base - 0x80000000) | 0x400000000ULL;
	reg_val = (uint32_t)(phy_addr >> 4);
	writel(reg_val, (void *)&scu->tsp_ctrl_3);

	/* Enable 1st i-cache area */
	writel(BIT(0), (void *)&scu->tsp_ctrl_4);

	/* Enable 1st d-cache area */
	writel(BIT(0), (void *)&scu->tsp_ctrl_5);

	/* Disable i & d cache by default */
	writel(0x0, (void *)&scu->tsp_ctrl_7);

	return 0;
}

int tsp_enable(void)
{
	struct ast2700_scu0 *scu;

	scu = (struct ast2700_scu0 *)ASPEED_CPU_SCU_BASE;
	setbits_le32((void *)&scu->tsp_ctrl_1, SCU_CPU_SSP_TSP_ENABLE);

	/* HW auto de-asserts TSP reset when WDT timeout reset occurs */
	clrbits_le32((void *)&scu->tsp_ctrl_1, SCU_CPU_SSP_TSP_RESET);

	return 0;
}
