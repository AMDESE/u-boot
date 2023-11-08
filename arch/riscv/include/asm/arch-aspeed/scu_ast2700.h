/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) Aspeed Technology Inc.
 */
#ifndef _ASM_ARCH_SCU_AST2700_H
#define _ASM_ARCH_SCU_AST2700_H

/* SoC0 SCU Register */
#define SCU_CPU_REVISION_ID_HW			GENMASK(23, 16)
#define SCU_CPU_REVISION_ID_EFUSE		GENMASK(15, 8)

#define SCU_CPU_HWSTRAP_DIS_RVAS		BIT(30)
#define SCU_CPU_HWSTRAP_DP_SRC			BIT(29)
#define SCU_CPU_HWSTRAP_DAC_SRC			BIT(28)
#define SCU_CPU_HWSTRAP_VRAM_SIZE		GENMASK(11, 10)

#define SCU_CPU_RST_DPMCU			BIT(29)
#define SCU_CPU_RST_DP				BIT(28)
#define SCU_CPU_RST_XDMA1			BIT(26)
#define SCU_CPU_RST_XDMA0			BIT(25)
#define SCU_CPU_RST_EMMC			BIT(17)
#define SCU_CPU_RST_EN_DP_PCI			BIT(15)
#define SCU_CPU_RST_CRT				BIT(13)
#define SCU_CPU_RST_RVAS1			BIT(10)
#define SCU_CPU_RST_RVAS0			BIT(9)
#define SCU_CPU_RST_2D				BIT(7)
#define SCU_CPU_RST_VIDEO			BIT(6)
#define SCU_CPU_RST_SOC				BIT(5)
#define SCU_CPU_RST_DDRPHY			BIT(1)

#define SCU_CPU_VGA_FUNC_DAC_OUTPUT		GENMASK(11, 10)
#define SCU_CPU_VGA_FUNC_DP_OUTPUT		GENMASK(9, 8)
#define SCU_CPU_VGA_FUNC_DAC_DISABLE		BIT(7)

#define SCU_CPU_PCI_MISC0C_FB_SIZE		GENMASK(4, 0)

#define SCU_CPU_PCI_MISC70_EN_XHCI		BIT(3)
#define SCU_CPU_PCI_MISC70_EN_EHCI		BIT(2)
#define SCU_CPU_PCI_MISC70_EN_IPMI		BIT(1)
#define SCU_CPU_PCI_MISC70_EN_VGA		BIT(0)

/* SoC1 SCU Register */
#define SCU_IO_HWSTRAP_EMMC			BIT(11)
#define SCU_IO_HWSTRAP_SCM			BIT(3)

#ifndef __ASSEMBLY__
struct ast2700_soc0_scu {
	uint32_t chip_id1;		/* 0x000 */
	uint32_t rsv_0x04[3];		/* 0x004 ~ 0x00C */
	uint32_t hwstrap1;		/* 0x010 */
	uint32_t hwstrap1_clr;		/* 0x014 */
	uint32_t rsv_0x18[2];		/* 0x018 ~ 0x01C */
	uint32_t hwstrap1_lock;		/* 0x020 */
	uint32_t hwstrap1_prot1;	/* 0x024 */
	uint32_t hwstrap1_prot2;	/* 0x028 */
	uint32_t hwstrap1_prot3;	/* 0x02C */
	uint32_t rsv_0x30[116];		/* 0x030 ~ 0x1FC */
	uint32_t modrst1_ctrl;		/* 0x200 */
	uint32_t modrst1_clr;		/* 0x204 */
	uint32_t rsv_0x208[2];		/* 0x208 ~ 0x20C */
	uint32_t modrst1_lock;		/* 0x210 */
	uint32_t modrst1_prot1;		/* 0x214 */
	uint32_t modrst1_prot2;		/* 0x218 */
	uint32_t modrst1_prot3;		/* 0x21C */
	uint32_t modrst2_ctrl;		/* 0x220 */
	uint32_t modrst2_clr;		/* 0x224 */
	uint32_t rsv_0x228[2];		/* 0x228 ~ 0x22C */
	uint32_t modrst2_lock;		/* 0x230 */
	uint32_t modrst2_prot1;		/* 0x234 */
	uint32_t modrst2_prot2;		/* 0x238 */
	uint32_t modrst2_prot3;		/* 0x23C */
	uint32_t rsv_0x240[112];	/* 0x240 ~ 0x3FC clk */
	uint32_t pinmux1;		/* 0x400 */
	uint32_t pinmux2;		/* 0x404 */
	uint32_t pinmux3;		/* 0x408 */
	uint32_t rsv_0x40c;		/* 0x40C */
	uint32_t pinmux4;		/* 0x410 */
	uint32_t vga_func_ctrl;		/* 0x414 */
	uint32_t rsv_0x418[314];	/* 0x418 ~ 0x8FC */
	uint32_t vga0_scratch1[4];	/* 0x900 ~ 0x90C */
	uint32_t vga1_scratch1[4];	/* 0x910 ~ 0x91C */
	uint32_t vga0_scratch2[8];	/* 0x920 ~ 0x93C */
	uint32_t vga1_scratch2[8];	/* 0x940 ~ 0x95C */
	uint32_t pci_cfg1[3];		/* 0x960 ~ 0x968 */
	uint32_t rsv_0x96c;		/* 0x96C */
	uint32_t pcie_cfg1;		/* 0x970 */
	uint32_t mmio_decode1;		/* 0x974 */
	uint32_t reloc_ctrl_decode1[2];	/* 0x978 ~ 0x97C */
	uint32_t rsv_0x980[4];		/* 0x980 ~ 0x98C */
	uint32_t mbox_decode1;		/* 0x990 */
	uint32_t shared_sram_decode1[2];/* 0x994 ~ 0x998 */
	uint32_t rsv_0x99c;		/* 0x99C */
	uint32_t pci_cfg2[3];		/* 0x9A0 ~ 0x9A8 */
	uint32_t rsv_0x9ac;		/* 0x9AC */
	uint32_t pcie_cfg2;		/* 0x9B0 */
	uint32_t mmio_decode2;		/* 0x9B4 */
	uint32_t reloc_ctrl_decode2[2];	/* 0x9B8 ~ 0x9BC */
	uint32_t rsv_0x9c0[4];		/* 0x9C0 ~ 0x9CC */
	uint32_t mbox_decode2;		/* 0x9D0 */
	uint32_t shared_sram_decode2[2];/* 0x9D4 ~ 0x9D8 */
	uint32_t rsv_0x9dc[9];		/* 0x9DC ~ 0x9FC */
	uint32_t pci0_misc[32];		/* 0xA00 ~ 0xA7C */
	uint32_t pci1_misc[32];		/* 0xA80 ~ 0xAFC */
};
#endif
#endif
