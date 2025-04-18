// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */

#include <stdbool.h>

#include <hexdump.h>
#include <asm/io.h>
#include <dm/read.h>
#include <dm/uclass-id.h>
#include <dt-bindings/clock/ast2700-clock.h>
#include <dt-bindings/reset/ast2700-reset.h>
#include <linux/errno.h>

#define SYS_POLICY_SEC_VAL(_sec, _grp) !!(((_grp) + 1) & BIT(_sec))
#define SYS_POLICY_MAX_NUM (128)
#define SYS_POLICY_SEC_BANK_MAX_NUM (2)
#define SYS_POLICY_SEC_REG_MAX_NUM (3)

#define SYS_POLICY_RESET_LOCK (GENMASK(15, 13) | GENMASK(7, 5))
#define SYS_POLICY_CLK0_LOCK (GENMASK(23, 21))
#define SYS_POLICY_CLK1_LOCK (GENMASK(23, 21) | GENMASK(31, 29))

enum {
	SCU_SEC_PSP_GROUP = 0,
	SCU_SSP_GROUP,
	SCU_PSP_GROUP,
	SCU_TSP_GROUP,
	SCU_PSP_SSP_GROUP,
	SCU_SSP_TSP_GROUP,
	SCU_BOOTMCU_GROUP,
};

enum {
	SYS_POLICY_SOC0_RST = 0,
	SYS_POLICY_SOC1_RST,
	SYS_POLICY_SOC0_CLK,
	SYS_POLICY_SOC1_CLK,
	SYS_POLICY_SOC0_CLK_SEL,
	SYS_POLICY_SOC1_CLK_SEL,
};

struct sys_policy {
	const char *name;
	struct udevice *dev;
	const enum uclass_id uclass_id;
	void __iomem *base;
	int num;
	const uint32_t max_id;
	uint32_t *list;
	uint32_t (*reg_bank)[SYS_POLICY_SEC_REG_MAX_NUM];
	int (*init_policy)(struct sys_policy *ctrl, int grp);
	int (*load_policy)(struct sys_policy *ctrl, int grp);
	int (*apply_policy)(struct sys_policy *ctrl);
};

/******************************************************************************
 *                       Aspeed Clock Policy Callback                         *
 ******************************************************************************/
static int sys_policy_apply_clk0_policy(struct sys_policy *clk_ctrl)
{
	uint32_t(*reg)[3] = ((uint32_t(*)[3])clk_ctrl->reg_bank);

	if (!reg)
		return -EINVAL;

	/* Set clock 0 security policy*/
	writel(reg[0][0], clk_ctrl->base + 0x14);
	writel(reg[0][1], clk_ctrl->base + 0x18);
	writel(reg[0][2], clk_ctrl->base + 0x1C);

	/* Set clock 0 write protection */
	writel(SYS_POLICY_CLK0_LOCK, clk_ctrl->base + 0xBD0);

#ifdef SYS_POLICY_DEBUG
	printf("Dbg: 0x%08x 0x%08x 0x%08x\n", reg[0][0], reg[0][1], reg[0][2]);
#endif

	return 0;
}

static int sys_policy_apply_clk1_policy(struct sys_policy *clk_ctrl)
{
	uint32_t(*reg)[3] = ((uint32_t(*)[3])clk_ctrl->reg_bank);

	if (!reg)
		return -EINVAL;

	/* Set clock 1 ctrl 0 security policy */
	writel(reg[0][0], clk_ctrl->base + 0x14);
	writel(reg[0][1], clk_ctrl->base + 0x18);
	writel(reg[0][2], clk_ctrl->base + 0x1C);

	/* Set clock 1 ctrl 1 security policy */
	writel(reg[1][0], clk_ctrl->base + 0x34);
	writel(reg[1][1], clk_ctrl->base + 0x38);
	writel(reg[1][2], clk_ctrl->base + 0x3C);

	/* Set clock 1 write protection */
	writel(SYS_POLICY_CLK1_LOCK, clk_ctrl->base + 0xBD0);

#ifdef SYS_POLICY_DEBUG
	printf("Dbg: 0x%08x 0x%08x 0x%08x\n", reg[0][0], reg[0][1], reg[0][2]);
	printf("Dbg: 0x%08x 0x%08x 0x%08x\n", reg[1][0], reg[1][1], reg[1][2]);
#endif

	return 0;
}

/******************************************************************************
 *                       Aspeed Reset Policy Callback                         *
 ******************************************************************************/
static int sys_policy_apply_rst_policy(struct sys_policy *rst_ctrl)
{
	uint32_t(*reg)[3] = ((uint32_t(*)[3])rst_ctrl->reg_bank);

	if (!reg)
		return -EINVAL;

	/* Set reset 0 security policy */
	writel(reg[0][0], rst_ctrl->base + 0x14);
	writel(reg[0][1], rst_ctrl->base + 0x18);
	writel(reg[0][2], rst_ctrl->base + 0x1C);

	/* Set reset 1 security policy */
	writel(reg[1][0], rst_ctrl->base + 0x34);
	writel(reg[1][1], rst_ctrl->base + 0x38);
	writel(reg[1][2], rst_ctrl->base + 0x3C);

	/* Set reset 0/1 write protection */
	writel(SYS_POLICY_RESET_LOCK, rst_ctrl->base + 0xC10);

#ifdef SYS_POLICY_DEBUG
	printf("Dbg: 0x%08x 0x%08x 0x%08x\n", reg[0][0], reg[0][1], reg[0][2]);
	printf("Dbg: 0x%08x 0x%08x 0x%08x\n", reg[1][0], reg[1][1], reg[1][2]);
#endif

	return 0;
}

/******************************************************************************
 *                          Aspeed System Policy                              *
 ******************************************************************************/
static int sys_policy_init_policy(struct sys_policy *ctrl, int grp)
{
	static const char *const prop[] = { "sec-psp", "ssp", "psp", "tsp",
						"psp-ssp", "ssp-tsp", "bmcu" };

	if (!ctrl->list)
		return -EINVAL;

	ctrl->num = dev_read_size(ctrl->dev, prop[grp]) / sizeof(uint32_t);
	if (ctrl->num < 0 || ctrl->num >= SYS_POLICY_MAX_NUM)
		return -EINVAL;

	if (dev_read_u32_array(ctrl->dev, prop[grp], ctrl->list, ctrl->num))
		return -EINVAL;

#ifdef SYS_POLICY_DEBUG
	print_hex_dump(prop[grp], DUMP_PREFIX_NONE, 16, 1, ctrl->list,
		       ctrl->num * sizeof(uint32_t), false);
#endif

	return 0;
}

static int sys_policy_load_policy(struct sys_policy *ctrl, int grp)
{
	uint32_t(*reg)[3] = ((uint32_t(*)[3])ctrl->reg_bank);
	uint32_t rst_bank = 0;
	uint32_t rst_bit = 0;
	uint32_t i = 0;

	if (!reg)
		return -EINVAL;

	for (i = 0; i < ctrl->num; i++) {
		rst_bank = ctrl->list[i] / 32;
		rst_bit = ctrl->list[i] % 32;

		if (ctrl->list[i] > ctrl->max_id ||
		    rst_bank >= SYS_POLICY_SEC_BANK_MAX_NUM) {
			/* Check if the reset number is bigger than max number */
			printf("Err: invalid policy (%d, %d)\n", ctrl->list[i], grp);
		} else if ((reg[rst_bank][0] & BIT(rst_bit)) != 0 ||
			   (reg[rst_bank][1] & BIT(rst_bit)) != 0 ||
			   (reg[rst_bank][2] & BIT(rst_bit)) != 0) {
			/* Check if the reset policy is set */
			printf("Err: duplicated policy (%d, %d)\n", ctrl->list[i], grp);
		} else {
			/* Set reset security policy */
			reg[rst_bank][0] |= SYS_POLICY_SEC_VAL(0, grp) ? BIT(rst_bit) : 0;
			reg[rst_bank][1] |= SYS_POLICY_SEC_VAL(1, grp) ? BIT(rst_bit) : 0;
			reg[rst_bank][2] |= SYS_POLICY_SEC_VAL(2, grp) ? BIT(rst_bit) : 0;
		}
	}

#ifdef SYS_POLICY_DEBUG
	printf("Dbg: 0x%08x 0x%08x 0x%08x\n", reg[0][0], reg[0][1], reg[0][2]);
	printf("Dbg: 0x%08x 0x%08x 0x%08x\n", reg[1][0], reg[1][1], reg[1][2]);
#endif

	return 0;
}

static int sys_policy_alloc_dev(struct sys_policy *ctrl)
{
	int ret = 0;

	/* Get udevice node and return the udevice */
	ret = uclass_get_device_by_name(ctrl->uclass_id, ctrl->name,
					&ctrl->dev);
	if (ret)
		goto exit;

	/* Get the system policy control base address */
	ctrl->base = dev_read_addr_ptr(ctrl->dev);

exit:
	return ret;
}

static int sys_policy_config_dev(struct sys_policy *ctrl)
{
	int policy_list[SYS_POLICY_MAX_NUM] = { 0 };
	int reg[SYS_POLICY_SEC_BANK_MAX_NUM][SYS_POLICY_SEC_REG_MAX_NUM] = { 0 };
	int grp = 0;
	int ret = 0;

	/* Prepare the register value should be set */
	for (grp = SCU_SEC_PSP_GROUP; grp <= SCU_BOOTMCU_GROUP; grp++) {
		/* To reduce the memory usage, use a temporary buffer */
		ctrl->list = policy_list;
		ctrl->reg_bank = (uint32_t(*)[SYS_POLICY_SEC_REG_MAX_NUM])reg;

		ret = ctrl->init_policy(ctrl, grp);
		if (ret) {
			printf("Err: init %s:%d policy failed.\n", ctrl->name, grp);
			return ret;
		}

		ret = ctrl->load_policy(ctrl, grp);
		if (ret) {
			printf("Err: load %s:%d policy failed.\n", ctrl->name, grp);
			return ret;
		}
	}

	return ctrl->apply_policy(ctrl);
}

static int sys_policy_enable(struct sys_policy *ctrl)
{
	int ret = 0;

	ret = sys_policy_alloc_dev(ctrl);
	if (ret) {
		printf("Err: alloc %s policy failed.\n", ctrl->name);
		goto exit;
	}

	ret = sys_policy_config_dev(ctrl);
	if (ret) {
		printf("Err: config %s policy failed.\n", ctrl->name);
		goto exit;
	}

exit:
	return ret;
}

static struct sys_policy policy_ctrl[] = {
	[SYS_POLICY_SOC0_RST] = {
		.uclass_id = UCLASS_RESET,
		.max_id = SCU0_RESET_VLINK,
		.name = "reset-controller@12c02200",
		.init_policy = sys_policy_init_policy,
		.load_policy = sys_policy_load_policy,
		.apply_policy = sys_policy_apply_rst_policy,
	},
	[SYS_POLICY_SOC1_RST] = {
		.uclass_id = UCLASS_RESET,
		.max_id = SCU1_RESET_I3CDMA,
		.name = "reset-controller@14c02200",
		.init_policy = sys_policy_init_policy,
		.load_policy = sys_policy_load_policy,
		.apply_policy = sys_policy_apply_rst_policy,
	},
	[SYS_POLICY_SOC0_CLK] = {
		.uclass_id = UCLASS_CLK,
		.max_id = SCU0_CLK_GATE_RVAS1CLK,
		.name = "clock-controller@12c02200",
		.init_policy = sys_policy_init_policy,
		.load_policy = sys_policy_load_policy,
		.apply_policy = sys_policy_apply_clk0_policy,
	},
	[SYS_POLICY_SOC1_CLK] = {
		.uclass_id = UCLASS_CLK,
		.max_id = SCU1_CLK_GATE_LTPI1TXCLK,
		.name = "clock-controller@14c02200",
		.init_policy = sys_policy_init_policy,
		.load_policy = sys_policy_load_policy,
		.apply_policy = sys_policy_apply_clk1_policy,
	},
};

int sys_policy_init(void)
{
	int ret = 0;

	ret |= sys_policy_enable(&policy_ctrl[SYS_POLICY_SOC0_RST]);
	ret |= sys_policy_enable(&policy_ctrl[SYS_POLICY_SOC1_RST]);
	ret |= sys_policy_enable(&policy_ctrl[SYS_POLICY_SOC0_CLK]);
	ret |= sys_policy_enable(&policy_ctrl[SYS_POLICY_SOC1_CLK]);

	return ret;
}
