// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 Aspeed Technology Inc.
 */

#include <stdlib.h>
#include <common.h>
#include <console.h>
#include <bootretry.h>
#include <cli.h>
#include <command.h>
#include <console.h>
#include <malloc.h>
#include <inttypes.h>
#include <mapmem.h>
#include <asm/io.h>
#include <linux/compiler.h>
#include <linux/iopoll.h>
#include <u-boot/sha256.h>
#include <u-boot/sha512.h>
#include <u-boot/rsa.h>
#include <u-boot/rsa-mod-exp.h>
#include <dm.h>
#include <misc.h>
#include <clk.h>

DECLARE_GLOBAL_DATA_PTR;

#define OTP_VER				"1.0.0"

/***********************
 *                     *
 * OTP regs definition *
 *                     *
 ***********************/
#define OTP_REG_SIZE		0x200

#define OTP_PASSWD		0x349fe38a
#define OTP_CMD_READ		0x23b1e361
#define OTP_CMD_PROG		0x23b1e364
#define OTP_CMD_CMP		0x23b1e363
#define OTP_CMD_BIST		0x23b1e368

#define OTP_CMD_OFFSET		0x20
#define OTP_MASTER		OTP_M0

#define OTP_KEY				0x0
#define OTP_CMD				(OTP_MASTER * OTP_CMD_OFFSET + 0x4)
#define OTP_WDATA_0			(OTP_MASTER * OTP_CMD_OFFSET + 0x8)
#define OTP_WDATA_1			(OTP_MASTER * OTP_CMD_OFFSET + 0xc)
#define OTP_WDATA_2			(OTP_MASTER * OTP_CMD_OFFSET + 0x10)
#define OTP_WDATA_3			(OTP_MASTER * OTP_CMD_OFFSET + 0x14)
#define OTP_STATUS			(OTP_MASTER * OTP_CMD_OFFSET + 0x18)
#define OTP_ADDR			(OTP_MASTER * OTP_CMD_OFFSET + 0x1c)
#define OTP_RDATA			(OTP_MASTER * OTP_CMD_OFFSET + 0x20)

#define OTP_DBG00			0x0C4
#define OTP_MASTER_PID			0x0D0
#define OTP_ECC_EN			0x0D4
#define OTP_CMD_LOCK			0x0D8
#define OTP_SW_RST			0x0DC
#define OTP_SLV_ID			0x0E0
#define OTP_PMC_CQ			0x0E4
#define OTP_FPGA			0x0EC
#define OTP_REGION_ROM_PATCH		0x100
#define OTP_REGION_OTPCFG		0x104
#define OTP_REGION_OTPSTRAP		0x108
#define OTP_REGION_OTP_FLASHSTRAP	0x10C
#define OTP_REGION_SECURE		0x120
#define OTP_REGION_SECURE_RANGE		0x124
#define OTP_REGION_SECURE1		0x128
#define OTP_REGION_SECURE1_RANGE	0x12C
#define OTP_REGION_USR0			0x140
#define OTP_REGION_USR0_RANGE		0x144
#define OTP_REGION_USR1			0x148
#define OTP_REGION_USR1_RANGE		0x14C
#define OTP_REGION_USR2			0x150
#define OTP_REGION_USR2_RANGE		0x154
#define OTP_REGION_USR3			0x158
#define OTP_REGION_USR3_RANGE		0x15C
#define OTP_PUF				0x160
#define OTP_MASTER_ID			0x170
#define OTP_MASTER_ID_EXT		0x174
#define OTP_R_MASTER_ID			0x178
#define OTP_R_MASTER_ID_EXT		0x17C
#define OTP_R_LOCK			0x180

#define OTP_PMC				0x200
#define OTP_DAP				0x300

/* OTP status: [0] */
#define OTP_STS_IDLE		0x0
#define OTP_STS_BUSY		0x1

/* OTP cmd status: [7:4] */
#define OTP_GET_CMD_STS(x)	(((x) & 0xF0) >> 4)
#define OTP_STS_PASS		0x0
#define OTP_STS_FAIL		0x1
#define OTP_STS_CMP_FAIL	0x2
#define OTP_STS_REGION_FAIL	0x3
#define OTP_STS_MASTER_FAIL	0x4

/* OTP ECC EN */
#define ECC_ENABLE		0x1
#define ECC_DISABLE		0x0
#define ECCBRP_EN		BIT(0)
#define CFG_ECCBRP_EN		BIT(2)

/* OTP PUF */
#define OTP_SW_PUF		BIT(0)
#define OTP_HW_PUF		BIT(1)

/********************************************************************/

/* OTP memory address from 0x0~0x2000. (unit: Single Word 16-bits) */
/* ----  0x0  -----
 *     OTP ROM
 * ---- 0x400 -----
 *     OTPCFG
 * ---- 0x420 -----
 *     HW STRAP
 * ---- 0x430 -----
 *   Flash STRAP
 * ---- 0x440 -----
 *   User Region
 * ---- 0x1000 ----
 *  Secure Region
 * ---- 0x1f80 ----
 *      SW PUF
 * ---- 0x1fc0 ----
 *      HW PUF
 * ---- 0x2000 ----
 */
#define OTPROM_START_ADDR		0x0
#define OTPROM_END_ADDR			0x400
#define OTPCFG_START_ADDR		0x400
#define OTPCFG_END_ADDR			0x420
#define OTPSTRAP_START_ADDR		0x420
#define OTPSTRAP_END_ADDR		0x430
#define OTPFLASHSTRAP_START_ADDR	0x430
#define OTPFLASHSTRAP_END_ADDR		0x440
#define USER_REGION_START_ADDR		0x440
#define USER_REGION_END_ADDR		0x1000
#define SEC_REGION_START_ADDR		0x1000
#define SEC_REGION_END_ADDR		0x1f80
#define SW_PUF_START_ADDR		0x1f80
#define SW_PUF_END_ADDR			0x1fc0
#define HW_PUF_START_ADDR		0x1fc0
#define HW_PUF_END_ADDR			0x2000

#define OTP_MEM_ADDR_MAX		HW_PUF_START_ADDR
#define OTP_USER_REGION_SIZE	(USER_REGION_END_ADDR - USER_REGION_START_ADDR)
#define OTP_SEC_REGION_SIZE	(SEC_REGION_END_ADDR - SEC_REGION_START_ADDR)

/* OTPCFG */
#define OTPCFG0_ADDR			OTPCFG_START_ADDR
#define OTPCFG1_ADDR			(OTPCFG0_ADDR + 0x1)
#define OTPCFG2_ADDR			(OTPCFG0_ADDR + 0x2)
#define OTPCFG3_ADDR			(OTPCFG0_ADDR + 0x3)
#define OTPCFG4_ADDR			(OTPCFG0_ADDR + 0x4)
#define OTPCFG5_ADDR			(OTPCFG0_ADDR + 0x5)
#define OTPCFG6_ADDR			(OTPCFG0_ADDR + 0x6)
#define OTPCFG7_ADDR			(OTPCFG0_ADDR + 0x7)
#define OTPCFG8_ADDR			(OTPCFG0_ADDR + 0x8)
#define OTPCFG9_ADDR			(OTPCFG0_ADDR + 0x9)
#define OTPCFG10_ADDR			(OTPCFG0_ADDR + 0xa)
#define OTPCFG11_ADDR			(OTPCFG0_ADDR + 0xb)
#define OTPCFG12_ADDR			(OTPCFG0_ADDR + 0xc)
#define OTPCFG13_ADDR			(OTPCFG0_ADDR + 0xd)
#define OTPCFG14_ADDR			(OTPCFG0_ADDR + 0xe)
#define OTPCFG15_ADDR			(OTPCFG0_ADDR + 0xf)
#define OTPCFG16_ADDR			(OTPCFG0_ADDR + 0x10)

/* OTPCFG0 */
enum otpcfg0_desc {
	RD_PROT_OTP_FLASH_STRAP = 0,
	RD_PROT_OTP_ROM,
	RD_PROT_OTP_CFG,
	RD_PROT_OTP_STRAP,
	WR_PROT_OTP_FLASH_STRAP,
	WR_PROT_OTP_ROM,
	WR_PROT_OTP_CFG,
	WR_PROT_OTP_STRAP,
	OTPCFG0_8_RESERVED,
	WR_PROT_OTPKEY_RETIRE,
	OTPCFG0_10_RESERVED,
	OTPCFG0_11_RESERVED,
	OTPCFG0_12_RESERVED,
	OTPCFG0_13_RESERVED,
	OTP_MEM_ECC_ENABLE,
	OTP_MEM_LOCK_ENABLE,
};

#define OTP_TIMEOUT_US		10000
#define OTP_PATTERN		0x1

/* SCU IO */
#define SCU_RESET_LOG		0x50
#define SYS_PWR_RESET		BIT(11)
#define SYS_EXT_RESET		BIT(1)
#define SYS_SYS_RESET		BIT(0)

/* OTPSTRAP */
#define OTPSTRAP0_ADDR		OTPSTRAP_START_ADDR
#define OTPSTRAP1_ADDR		(OTPSTRAP0_ADDR + 1)
#define OTPSTRAP2_ADDR		(OTPSTRAP0_ADDR + 2)
#define OTPSTRAP3_ADDR		(OTPSTRAP0_ADDR + 3)
#define OTPSTRAP4_ADDR		(OTPSTRAP0_ADDR + 4)
#define OTPSTRAP5_ADDR		(OTPSTRAP0_ADDR + 5)
#define OTPSTRAP6_ADDR		(OTPSTRAP0_ADDR + 6)
#define OTPSTRAP7_ADDR		(OTPSTRAP0_ADDR + 7)
#define OTPSTRAP8_ADDR		(OTPSTRAP0_ADDR + 8)
#define OTPSTRAP9_ADDR		(OTPSTRAP0_ADDR + 9)
#define OTPSTRAP10_ADDR		(OTPSTRAP0_ADDR + 0xa)
#define OTPSTRAP11_ADDR		(OTPSTRAP0_ADDR + 0xb)
#define OTPSTRAP12_ADDR		(OTPSTRAP0_ADDR + 0xc)
#define OTPSTRAP13_ADDR		(OTPSTRAP0_ADDR + 0xd)
#define OTPSTRAP14_ADDR		(OTPSTRAP0_ADDR + 0xe)

#define OTPTOOL_VERSION(a, b, c) (((a) << 24) + ((b) << 12) + (c))
#define OTPTOOL_VERSION_MAJOR(x) (((x) >> 24) & 0xff)
#define OTPTOOL_VERSION_PATCHLEVEL(x) (((x) >> 12) & 0xfff)
#define OTPTOOL_VERSION_SUBLEVEL(x) ((x) & 0xfff)
#define OTPTOOL_COMPT_VERSION 2

enum otp_error_code {
	OTP_READ_FAIL,
	OTP_PROG_FAIL,
	OTP_CMP_FAIL,
};

enum aspeed_otp_master_id {
	OTP_M0 = 0,
	OTP_M1,
	OTP_M2,
	OTP_M3,
	OTP_M4,
	OTP_M5,
	OTP_MID_MAX,
};

struct aspeed_otp {
	phys_addr_t base;
	struct clk clk;
	int cfg_ecc_en;
	int mem_ecc_en;
};

static void otp_unlock(struct udevice *dev)
{
	struct aspeed_otp *otp = dev_get_priv(dev);

	writel(OTP_PASSWD, otp->base + OTP_KEY);
}

static void otp_lock(struct udevice *dev)
{
	struct aspeed_otp *otp = dev_get_priv(dev);

	writel(0x1, otp->base + OTP_KEY);
}

static int wait_complete(struct udevice *dev)
{
	struct aspeed_otp *otp = dev_get_priv(dev);
	int ret;
	u32 val;

	ret = readl_poll_timeout(otp->base + OTP_STATUS, val, (val == 0x0),
				 OTP_TIMEOUT_US);
	if (ret)
		printf("\n%s: timeout. sts:0x%x\n", __func__, val);

	return ret;
}

static int otp_read_data(struct udevice *dev, u32 offset, u16 *data)
{
	struct aspeed_otp *otp = dev_get_priv(dev);
	int ret;

	/* Set cfg/non-cfg ecc */
	if ((offset >= OTPCFG_START_ADDR && offset < OTPCFG_END_ADDR) &&
	    otp->cfg_ecc_en)
		writel(CFG_ECCBRP_EN, otp->base + OTP_ECC_EN);
	else if (otp->mem_ecc_en)
		writel(ECCBRP_EN, otp->base + OTP_ECC_EN);
	else
		writel(0, otp->base + OTP_ECC_EN);

	writel(offset, otp->base + OTP_ADDR);
	writel(OTP_CMD_READ, otp->base + OTP_CMD);
	ret = wait_complete(dev);
	if (ret)
		return OTP_READ_FAIL;

	data[0] = readl(otp->base + OTP_RDATA);

	return 0;
}

int otp_prog_data(struct udevice *dev, u32 offset, u16 data)
{
	struct aspeed_otp *otp = dev_get_priv(dev);
	int ret;

	/* cfg/non-cfg ecc */
	if ((offset >= OTPCFG_START_ADDR && offset < OTPCFG_END_ADDR) &&
	    otp->cfg_ecc_en)
		writel(CFG_ECCBRP_EN, otp->base + OTP_ECC_EN);
	else if (otp->mem_ecc_en)
		writel(ECCBRP_EN, otp->base + OTP_ECC_EN);
	else
		writel(0, otp->base + OTP_ECC_EN);

	writel(offset, otp->base + OTP_ADDR);
	writel(data, otp->base + OTP_WDATA_0);
	writel(OTP_CMD_PROG, otp->base + OTP_CMD);
	ret = wait_complete(dev);
	if (ret)
		return OTP_PROG_FAIL;

	return 0;
}

static int aspeed_otp_read(struct udevice *dev, int offset,
			   void *buf, int size)
{
	int ret;
	u16 *data = buf;

	otp_unlock(dev);
	for (int i = 0; i < size; i++) {
		ret = otp_read_data(dev, offset + i, data + i);
		if (ret) {
			printf("%s: read failed\n", __func__);
			break;
		}
	}

	otp_lock(dev);
	return ret;
}

static int aspeed_otp_write(struct udevice *dev, int offset,
			    const void *buf, int size)
{
	u16 *data = (u16 *)buf;
	int ret;

	otp_unlock(dev);

	for (int i = 0; i < size; i++) {
//		printf("%s: prog 0x%x=0x%x\n", __func__, offset + i, data[i]);
		ret = otp_prog_data(dev, offset + i, data[i]);
		if (ret) {
			printf("%s: prog failed\n", __func__);
			break;
		}
	}

	otp_lock(dev);
	return ret;
}

static int aspeed_otp_ioctl(struct udevice *dev, unsigned long request,
			    void *buf)
{
	return 0;
}

static int aspeed_otp_ecc_init(struct udevice *dev)
{
	struct aspeed_otp *otp = dev_get_priv(dev);
	int ret;
	u32 val;

	/* Check cfg_ecc_en */
	writel(0, otp->base + OTP_ECC_EN);
	writel(OTPSTRAP14_ADDR, otp->base + OTP_ADDR);
	writel(OTP_CMD_READ, otp->base + OTP_CMD);
	ret = wait_complete(dev);
	if (ret)
		return OTP_READ_FAIL;

	val = readl(otp->base + OTP_RDATA);
	if (val & 0x1)
		otp->cfg_ecc_en = 0x1;
	else
		otp->cfg_ecc_en = 0x0;

	/* Check mem_ecc_en */
	if (otp->cfg_ecc_en)
		writel(CFG_ECCBRP_EN, otp->base + OTP_ECC_EN);
	else
		writel(0, otp->base + OTP_ECC_EN);

	writel(OTPCFG0_ADDR, otp->base + OTP_ADDR);
	writel(OTP_CMD_READ, otp->base + OTP_CMD);
	ret = wait_complete(dev);
	if (ret)
		return OTP_READ_FAIL;

	val = readl(otp->base + OTP_RDATA);
	if (val & BIT(OTP_MEM_ECC_ENABLE))
		otp->mem_ecc_en = 0x1;
	else
		otp->mem_ecc_en = 0x0;

	return 0;
}

static int aspeed_otp_probe(struct udevice *dev)
{
	struct aspeed_otp *otp = dev_get_priv(dev);
	int rc;

	otp->base = devfdt_get_addr(dev);
	printf("%s: otp base: 0x%llx\n", __func__, otp->base);

	/* OTP ECC init */
	rc = aspeed_otp_ecc_init(dev);
	if (rc) {
		debug("OTP ECC init failed, rc:%d\n", rc);
		return rc;
	}

	return rc;
}

static int aspeed_otp_remove(struct udevice *dev)
{
	struct aspeed_otp *otp = dev_get_priv(dev);

	clk_disable(&otp->clk);

	return 0;
}

static const struct misc_ops aspeed_otp_ops = {
	.read = aspeed_otp_read,
	.write = aspeed_otp_write,
	.ioctl = aspeed_otp_ioctl,
};

static const struct udevice_id aspeed_otp_ids[] = {
	{ .compatible = "aspeed,ast2700-otp" },
	{ }
};

U_BOOT_DRIVER(aspeed_otp) = {
	.name = "aspeed_otp",
	.id = UCLASS_MISC,
	.of_match = aspeed_otp_ids,
	.ops = &aspeed_otp_ops,
	.probe = aspeed_otp_probe,
	.remove	= aspeed_otp_remove,
	.priv_auto = sizeof(struct aspeed_otp),
	.flags = DM_FLAG_PRE_RELOC,
};
