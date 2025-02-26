// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) ASPEED Technology Inc.
 */

#include <common.h>
#include <clk.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <errno.h>
#include <regmap.h>
#include <syscon.h>
#include <reset.h>
#include <fdtdec.h>
#include <asm/io.h>
#include <linux/bitfield.h>
#include <linux/delay.h>
#include <asm/arch/scu_ast2700.h>
#include <asm/arch/platform.h>
#include <asm/arch/sdram_ast2700.h>
#include <binman_sym.h>
#include <ast_loader.h>

binman_sym_declare(u32, u_boot_spl_ddr, image_pos);

binman_sym_declare(u32, ddr4_1d_imem_fw, image_pos);
binman_sym_declare(u32, ddr4_1d_imem_fw, size);
binman_sym_declare(u32, ddr4_1d_dmem_fw, image_pos);
binman_sym_declare(u32, ddr4_1d_dmem_fw, size);
binman_sym_declare(u32, ddr4_2d_imem_fw, image_pos);
binman_sym_declare(u32, ddr4_2d_imem_fw, size);
binman_sym_declare(u32, ddr4_2d_dmem_fw, image_pos);
binman_sym_declare(u32, ddr4_2d_dmem_fw, size);
binman_sym_declare(u32, ddr5_imem_fw, image_pos);
binman_sym_declare(u32, ddr5_imem_fw, size);
binman_sym_declare(u32, ddr5_dmem_fw, image_pos);
binman_sym_declare(u32, ddr5_dmem_fw, size);

static int stor_load(struct udevice *dev, u32 type, u32 *dst, u32 *len)
{
	struct ast_loader *ast = dev_get_priv(dev);
	struct ast_loader_ops *ops;
	int err = 0;
	u32 *src, ofst, sz;
	u32 rev_id = ast->rev_id;
	u32 hdr_ofst = 0x0;

	if (rev_id) {
		hdr_ofst = 0x20000;

		err = fmc_hdr_get_prebuilt(type, &ofst, &sz, NULL);
		if (err)
			return err;

	} else {
		struct train_bin dwc_train[DRAM_TYPE_MAX][2] = {
			{{0, 0, 0, 0}, {0, 0, 0, 0}},
			{{0, 0, 0, 0}, {0, 0, 0, 0}},
		};

		u32 ctx_start, imem_start, dmem_start, imem_2d_start, dmem_2d_start;
		u32 imem_len, dmem_len, imem_2d_len, dmem_2d_len;
		u32 ddr5_imem, ddr5_imem_len, ddr5_dmem, ddr5_dmem_len;
		u32 base = CONFIG_SPL_TEXT_BASE;

		imem_start = binman_sym(u32, ddr4_1d_imem_fw, image_pos);
		imem_len = binman_sym(u32, ddr4_1d_imem_fw, size);
		dmem_start = binman_sym(u32, ddr4_1d_dmem_fw, image_pos);
		dmem_len = binman_sym(u32, ddr4_1d_dmem_fw, size);
		imem_2d_start = binman_sym(u32, ddr4_2d_imem_fw, image_pos);
		imem_2d_len = binman_sym(u32, ddr4_2d_imem_fw, size);
		dmem_2d_start = binman_sym(u32, ddr4_2d_dmem_fw, image_pos);
		dmem_2d_len = binman_sym(u32, ddr4_2d_dmem_fw, size);
		ddr5_imem = binman_sym(u32, ddr5_imem_fw, image_pos);
		ddr5_imem_len = binman_sym(u32, ddr5_imem_fw, size);
		ddr5_dmem = binman_sym(u32, ddr5_dmem_fw, image_pos);
		ddr5_dmem_len = binman_sym(u32, ddr5_dmem_fw, size);

		if (BINMAN_SYMS_OK) {
			ctx_start = binman_sym(u32, u_boot_spl_ddr, image_pos);
			imem_start = binman_sym(u32, ddr4_1d_imem_fw, image_pos);
			imem_len = binman_sym(u32, ddr4_1d_imem_fw, size);
			dmem_start = binman_sym(u32, ddr4_1d_dmem_fw, image_pos);
			dmem_len = binman_sym(u32, ddr4_1d_dmem_fw, size);
			imem_2d_start = binman_sym(u32, ddr4_2d_imem_fw, image_pos);
			imem_2d_len = binman_sym(u32, ddr4_2d_imem_fw, size);
			dmem_2d_start = binman_sym(u32, ddr4_2d_dmem_fw, image_pos);
			dmem_2d_len = binman_sym(u32, ddr4_2d_dmem_fw, size);
			ddr5_imem = binman_sym(u32, ddr5_imem_fw, image_pos);
			ddr5_imem_len = binman_sym(u32, ddr5_imem_fw, size);
			ddr5_dmem = binman_sym(u32, ddr5_dmem_fw, image_pos);
			ddr5_dmem_len = binman_sym(u32, ddr5_dmem_fw, size);

			// ddr5
			dwc_train[0][0].imem_base = ddr5_imem - base;
			dwc_train[0][0].imem_len = ddr5_imem_len;
			dwc_train[0][0].dmem_base = ddr5_dmem - base;
			dwc_train[0][0].dmem_len = ddr5_dmem_len;

			// ddr4 1d
			dwc_train[1][0].imem_base = imem_start - base;
			dwc_train[1][0].imem_len = imem_len;
			dwc_train[1][0].dmem_base = dmem_start - base;
			dwc_train[1][0].dmem_len = dmem_len;

			// ddr4 2d
			dwc_train[1][1].imem_base = imem_2d_start - base;
			dwc_train[1][1].imem_len = imem_2d_len;
			dwc_train[1][1].dmem_base = dmem_2d_start - base;
			dwc_train[1][1].dmem_len = dmem_2d_len;
		}

		switch (type) {
		case PBT_DDR4_PMU_TRAIN_IMEM:
			ofst = dwc_train[1][0].imem_base;
			sz = dwc_train[1][0].imem_len;
		break;
		case PBT_DDR4_PMU_TRAIN_DMEM:
			ofst = dwc_train[1][0].dmem_base;
			sz = dwc_train[1][0].dmem_len;
		break;
		case PBT_DDR4_2D_PMU_TRAIN_IMEM:
			ofst = dwc_train[1][1].imem_base;
			sz = dwc_train[1][1].imem_len;
		break;
		case PBT_DDR4_2D_PMU_TRAIN_DMEM:
			ofst = dwc_train[1][1].dmem_base;
			sz = dwc_train[1][1].dmem_len;
		break;
		case PBT_DDR5_PMU_TRAIN_IMEM:
			ofst = dwc_train[0][0].imem_base;
			sz = dwc_train[0][0].imem_len;
		break;
		case PBT_DDR5_PMU_TRAIN_DMEM:
			ofst = dwc_train[0][0].dmem_base;
			sz = dwc_train[0][0].dmem_len;
		break;
		default:
			printf("Unsupport type!!!!!!\n");
			break;
		};
	}

	src = (u32 *)(ofst + hdr_ofst);

	ops = ast_loader_get_ops(ast->boot_dev);
	if (ops && ops->copy)
		err = ops->copy(ast->boot_dev, dst, src, sz);

	*len = sz;

	return err;
}

int stor_init(struct udevice *dev)
{
	struct ast_loader *ast = dev_get_priv(dev);
	struct ast_loader_ops *ops;
	struct udevice *boot_dev = NULL;
	enum boot_mode_t bootmode;
	int err = -ENODEV;

	bootmode = ast->bootmode;

	if (bootmode == BOOT_SPI) {
		err = bootspi_init(&boot_dev);
	} else if (bootmode == BOOT_EMMC) {
		err = bootmmc_init(&boot_dev);
	} else if (bootmode == BOOT_UFS) {
		err = uclass_get_device_by_name(UCLASS_MISC, "bootufs", &boot_dev);
	} else {
		return -ENODEV;
	}

	if (err && err != -ENODEV) {
		printf("Get stor udevice Failed %d.\n", err);
		return err;
	}

	ast->boot_dev = boot_dev;
	ast->load = stor_load;

	ops = ast_loader_get_ops(boot_dev);
	if (ops && ops->init)
		err = ops->init(boot_dev);

	return err;
}
