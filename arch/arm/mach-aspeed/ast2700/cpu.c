// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include <common.h>
#include <asm/armv8/mmu.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <env.h>
#include <env_internal.h>
#include <linux/bitfield.h>

#define SCU_CPU_VGA0_SAR0	(0x12c02a0c)
#define SCU_PCI_MISC70		(0x12c02a70)
#define SCU_CPU_VGA1_SAR0	(0x12c02a8c)
#define SCU_PCI_MISCF0		(0x12c02af0)

DECLARE_GLOBAL_DATA_PTR;

static struct mm_region aspeed2700_mem_map[] = {
		{
				.virt =  0x0UL,
				.phys =  0x0UL,
				.size =  0x40000000UL,
				.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
					 PTE_BLOCK_NON_SHARE |
					 PTE_BLOCK_PXN | PTE_BLOCK_UXN
		},
		{
				.virt =  0x40000000UL,
				.phys =  0x40000000UL,
				.size = 0x2C0000000UL,
				.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
					 PTE_BLOCK_NON_SHARE
		},
		{
				.virt = 0x400000000UL,
				.phys = 0x400000000UL,
				.size = 0x200000000UL,
				.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
					 PTE_BLOCK_INNER_SHARE
		},
		{
				/* List terminator */
				0,
		}
};

struct mm_region *mem_map = aspeed2700_mem_map;

u64 get_page_table_size(void)
{
	return 0x80000;
}

enum env_location env_get_location(enum env_operation op, int prio)
{
	enum env_location env_loc = ENVL_UNKNOWN;

	if (prio)
		return env_loc;

	if ((readl(ASPEED_IO_HW_STRAP1) & STRAP_BOOTMODE_BIT))
		env_loc =  ENVL_MMC;
	else
		env_loc =  ENVL_SPI_FLASH;

	return env_loc;
}

static void vram_patch(u64 scu_addr, u64 e2m_addr)
{
	u32 hi_byte, val;

	// Decide high byte, [31:24] of vram address
	val = readl(0x12c00010) >> 2 & 0x07;
	hi_byte = ((1 << val) - 1) << 24;

	val = readl(scu_addr);
	val = (val & 0x00ffffff) | hi_byte;
	writel(val, scu_addr);
	writel(val, e2m_addr);
}

static int pci_vga_patch(void)
{
	u32 val, dac_src;

	val = readl(ASPEED_CPU_REVISION_ID);
	// VGA patches for A0
	if (FIELD_GET(GENMASK(23, 16), val) == 0) {
		bool is_pcie0_enable = (readl(SCU_PCI_MISC70) & BIT(0));
		bool is_pcie1_enable = (readl(SCU_PCI_MISCF0) & BIT(0));

		if (is_pcie0_enable)
			vram_patch(SCU_CPU_VGA0_SAR0, 0x12c21100);
		if (is_pcie1_enable)
			vram_patch(SCU_CPU_VGA1_SAR0, 0x12c22100);

		// vga link init
		val = readl(0x12c02414);
		dac_src = val >> 10 & 0x3;
		val = 0x10000000 | dac_src;
		writel(val, 0x12c1d050);
		writel(0x00010002, 0x12c1d044);
		writel(0x00030009, 0x12c1d010);
		writel(0x00030009, 0x12c1d110);
		writel(0x00030009, 0x14c3a010);
		writel(0x00030009, 0x14c3a110);
	}

	return 0;
}

int arch_misc_init(void)
{
	if (IS_ENABLED(CONFIG_ARCH_MISC_INIT)) {
		if ((readl(ASPEED_IO_HW_STRAP1) & STRAP_BOOTMODE_BIT))
			env_set("bootcmd", EMMC_BOOTCOMMAND);
		else
			env_set("bootcmd", SPI_BOOTCOMMAND);

		pci_vga_patch();
	}

	return 0;
}
