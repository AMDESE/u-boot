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

#include <image.h>
#include <u-boot/hash-checksum.h>

#include <asm/arch/scu_ast2700.h>
#include <asm/arch/platform.h>
#include <asm/sections.h>
#include <ast_loader.h>

#define SCU1_OTPCFG				(ASPEED_IO_SCU_BASE + 0x880)
#define SCU1_OTPCFG_03_02			(SCU1_OTPCFG + 0x4)
#define   OTPCFG2_DIS_RECOVERY_MODE		BIT(3)

#define   SCU1_HWSTRAP1_EN_RECOVERY_BOOT	BIT(4)
#define   SCU1_HWSTRAP1_RECOVERY_INTERFACE	GENMASK(27, 26)
#define   SCU1_HWSTRAP1_RECOVERY_I3C		(BIT(26) | BIT(27))
#define   SCU1_HWSTRAP1_RECOVERY_I2C		BIT(27)
#define   SCU1_HWSTRAP1_RECOVERY_USB		BIT(26)

static enum boot_mode_t get_boot_mode(void)
{
	u32 dis, strap;

	dis = readl((void *)SCU1_OTPCFG_03_02);
	strap = readl((void *)ASPEED_IO_HW_STRAP1);

	/* check if recovery is disabled by OTP */
	if (!(dis & OTPCFG2_DIS_RECOVERY_MODE)) {
		/* check if recovery is enabled by hwstrap */
		if (strap & SCU1_HWSTRAP1_EN_RECOVERY_BOOT) {
			if ((strap & SCU1_HWSTRAP1_RECOVERY_INTERFACE) == SCU1_HWSTRAP1_RECOVERY_USB)
				return BOOT_USB;
			else if ((strap & SCU1_HWSTRAP1_RECOVERY_INTERFACE) == SCU1_HWSTRAP1_RECOVERY_I2C)
				return BOOT_I2C;
			else if ((strap & SCU1_HWSTRAP1_RECOVERY_INTERFACE) == SCU1_HWSTRAP1_RECOVERY_I3C)
				return BOOT_I3C;
			else
				return BOOT_UART;
		}
	}

	if (strap & SCU_IO_HWSTRAP_EMMC) {
		if (strap & SCU_IO_HWSTRAP_UFS)
			return BOOT_UFS;
		else
			return BOOT_EMMC;
	} else {
		return BOOT_SPI;
	}
}

static int ast_loader_verify(u32 type, u32 *message, u32 len)
{
	struct fmc_hdr_v2 *hdr = (struct fmc_hdr_v2 *)(_start - sizeof(struct fmc_hdr_v2));
	struct image_region region[1];
	u8 hash[HDR_DGST_LEN];
	int err;

	region[0].data = message;
	region[0].size = len;

	err = hash_calculate("sha384", region, 1, hash);
	if (err) {
		printf("%s Hash calculate err=%d\n", __func__, err);
		return err;
	}

	err = memcmp(hash, hdr->body.pbs[type - 1].dgst, sizeof(hash));

	return err;
}

static int ast_loader_probe(struct udevice *dev)
{
	struct ast_loader *ast = dev_get_priv(dev);
	int err;

	ast->bootmode = get_boot_mode();

	err = stor_init(dev);
	if (err == -ENODEV)
		err = recovery_init(dev);

	if (err)
		return err;

	ast->dev = dev;
	ast->rev_id = !!(readl((void *)ASPEED_IO_REVISION_ID) & CHIP_AST2700A1_ID_MASK);
	ast->verify = (ast->rev_id) ? ast_loader_verify : NULL;

	return err;
}

int ast_loader_load_image(u32 type, u32 *dst)
{
	struct ast_loader *ast;
	struct udevice *dev;
	u32 len;
	int err;
	u32 *tmp_buf = (u32 *)ASPEED_SRAM_BASE;

	err = uclass_get_device_by_name(UCLASS_MISC, "ast_loader", &dev);
	if (err && err != -ENODEV) {
		printf("Get ast_loader udevice Failed %d.\n", err);
		return err;
	}

	ast = dev_get_priv(dev);

	if (ast->load) {
		err = ast->load(dev, type, tmp_buf, &len);
		if (err)
			return err;
	}

	if (ast->verify) {
		err = ast->verify(type, tmp_buf, len);
		if (err) {
			printf("Digest verify fail, err=%d\n", err);
			return err;
		}
	}

	memcpy(dst, tmp_buf, len);

	return err;
}

int ast_loader_init(void)
{
	struct udevice *dev;
	int err;

	err = uclass_get_device_by_name(UCLASS_MISC, "ast_loader", &dev);
	if (err && err != -ENODEV)
		printf("Get ast_loader udevice Failed %d.\n", err);

	return err;
}

static int ast_loader_of_to_plat(struct udevice *dev)
{
	return 0;
}

static const struct udevice_id ast_loader_ids[] = {
	{ .compatible = "aspeed,ast_loader" },
	{ }
};

U_BOOT_DRIVER(ast_loader) = {
	.name		= "ast-loader",
	.id		= UCLASS_MISC,
	.of_match	= ast_loader_ids,
	.probe		= ast_loader_probe,
	.of_to_plat	= ast_loader_of_to_plat,
	.priv_auto	= sizeof(struct ast_loader),
};
