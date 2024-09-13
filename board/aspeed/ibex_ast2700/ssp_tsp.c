// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */
#include <asm/io.h>
#include <asm/arch-aspeed/scu_ast2700.h>
#include <asm/arch-aspeed/platform.h>
#include <linux/delay.h>
#include <stdint.h>

int ssp_init(ulong load_addr)
{
	struct ast2700_soc0_scu *scu;
	uint32_t reg_val;
	uint64_t phy_addr;

	scu = (struct ast2700_soc0_scu *)ASPEED_CPU_SCU_BASE;

	reg_val = readl((void *)&scu->ssp_ctrl_1);
	if (!(reg_val & SCU_CPU_SSP_TSP_RESET_STS))
		return 0;

	writel(SCU_CPU_RST_SSP, (void *)&scu->modrst1_ctrl);
	writel(SCU_CPU_RST_SSP, (void *)&scu->modrst1_clr);

	reg_val = SCU_CPU_SSP_TSP_NIDEN | SCU_CPU_SSP_TSP_DBGEN |
		  SCU_CPU_SSP_TSP_DBG_ENABLE | SCU_CPU_SSP_TSP_RESET;
	writel(reg_val, (void *)&scu->ssp_ctrl_1);

	/* SSP 0x0000_0000 - 0x0200_0000 -> DRAM */
	writel(0, (void *)&scu->ssp_remap2_base);
	writel(0x2000000, (void *)&scu->ssp_remap2_size);

	/* SSP 0x0200_0000 - 0x0200_2000 -> AHB */
	writel(0x2000000, (void *)&scu->ssp_remap1_base);
	writel(0x2000, (void *)&scu->ssp_remap1_size);

	/* SSP 0x0200_2000 - 0x0200_4000 -> TCM */
	writel(0x2002000, (void *)&scu->ssp_remap0_base);
	writel(0x2000, (void *)&scu->ssp_remap0_size);

	/* Configure physical AHB remap */
	writel(0x10001000, (void *)&scu->ssp_ctrl_2);

	/* Configure physical DRAM remap */
	phy_addr = ((uint64_t)load_addr - 0x80000000) | 0x400000000ULL;
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
	struct ast2700_soc0_scu *scu;

	scu = (struct ast2700_soc0_scu *)ASPEED_CPU_SCU_BASE;
	setbits_le32((void *)&scu->ssp_ctrl_1, SCU_CPU_SSP_TSP_ENABLE);

	/* HW auto de-asserts SSP reset when WDT timeout reset occurs */
	clrbits_le32((void *)&scu->ssp_ctrl_1, SCU_CPU_SSP_TSP_RESET);

	return 0;
}

int tsp_init(ulong load_addr)
{
	struct ast2700_soc0_scu *scu;
	uint32_t reg_val;
	uint64_t phy_addr;

	scu = (struct ast2700_soc0_scu *)ASPEED_CPU_SCU_BASE;

	reg_val = readl((void *)&scu->tsp_ctrl_1);
	if (!(reg_val & SCU_CPU_SSP_TSP_RESET_STS))
		return 0;

	writel(SCU_CPU_RST2_TSP, (void *)&scu->modrst2_ctrl);
	writel(SCU_CPU_RST2_TSP, (void *)&scu->modrst2_clr);

	reg_val = SCU_CPU_SSP_TSP_NIDEN | SCU_CPU_SSP_TSP_DBGEN |
		  SCU_CPU_SSP_TSP_DBG_ENABLE | SCU_CPU_SSP_TSP_RESET;
	writel(reg_val, (void *)&scu->tsp_ctrl_1);

	/* TSP 0x0000_0000 - 0x0200_0000 -> DRAM */
	writel(0x2000000, (void *)&scu->tsp_remap_size);

	/* Configure physical DRAM remap */
	phy_addr = ((uint64_t)load_addr - 0x80000000) | 0x400000000ULL;
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
	struct ast2700_soc0_scu *scu;

	scu = (struct ast2700_soc0_scu *)ASPEED_CPU_SCU_BASE;
	setbits_le32((void *)&scu->tsp_ctrl_1, SCU_CPU_SSP_TSP_ENABLE);

	/* HW auto de-asserts TSP reset when WDT timeout reset occurs */
	clrbits_le32((void *)&scu->tsp_ctrl_1, SCU_CPU_SSP_TSP_RESET);

	return 0;
}
