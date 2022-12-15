// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright ASPEED Technology Inc.
 */
#include <linux/iopoll.h>
#include <common.h>
#include <clk.h>
#include <dm.h>
#include <errno.h>
#include <fdtdec.h>
#include <i2c.h>
#include <asm/io.h>
#include <regmap.h>
#include <reset.h>

struct ast2600_i2c_regs {
	u32 fun_ctrl;
	u32 ac_timing;
	u32 trx_buff;
	u32 icr;
	u32 ier;
	u32 isr;
	u32 cmd_sts;
};

/* 0x00 : I2CC Master/Slave Function Control Register  */
#define AST2600_I2CC_SLAVE_ADDR_RX_EN	BIT(20)
#define AST2600_I2CC_MASTER_RETRY_MASK	GENMASK(19, 18)
#define AST2600_I2CC_MASTER_RETRY(x)	(((x) & GENMASK(1, 0)) << 18)
#define AST2600_I2CC_BUS_AUTO_RELEASE	BIT(17)
#define AST2600_I2CC_M_SDA_LOCK_EN		BIT(16)
#define AST2600_I2CC_MULTI_MASTER_DIS	BIT(15)
#define AST2600_I2CC_M_SCL_DRIVE_EN		BIT(14)
#define AST2600_I2CC_MSB_STS			BIT(9)
#define AST2600_I2CC_SDA_DRIVE_1T_EN	BIT(8)
#define AST2600_I2CC_M_SDA_DRIVE_1T_EN	BIT(7)
#define AST2600_I2CC_M_HIGH_SPEED_EN	BIT(6)
/* reserver 5 : 2 */
#define AST2600_I2CC_SLAVE_EN			BIT(1)
#define AST2600_I2CC_MASTER_EN			BIT(0)

/* 0x04 : I2CD Clock and AC Timing Control Register #1 */
/* Base register value. These bits are always set by the driver. */
#define I2CD_CACTC_BASE		0xfff00300
#define I2CD_TCKHIGH_SHIFT	16
#define I2CD_TCKLOW_SHIFT	12
#define I2CD_THDDAT_SHIFT	10
#define I2CD_TO_DIV_SHIFT	8
#define I2CD_BASE_DIV_SHIFT	0

/* 0x08 : I2CC Master/Slave Transmit/Receive Byte Buffer Register */
#define AST2600_I2CC_TX_DIR_MASK		GENMASK(31, 29)
#define AST2600_I2CC_SDA_OE				BIT(28)
#define AST2600_I2CC_SDA_O				BIT(27)
#define AST2600_I2CC_SCL_OE				BIT(26)
#define AST2600_I2CC_SCL_O				BIT(25)

#define AST2600_I2CC_SCL_LINE_STS		BIT(18)
#define AST2600_I2CC_SDA_LINE_STS		BIT(17)
#define AST2600_I2CC_BUS_BUSY_STS		BIT(16)
#define AST2600_I2CC_GET_RX_BUFF(x)		(((x) >> 8) & GENMASK(7, 0))

/* 0x10 : I2CM Master Interrupt Control Register */
/* 0x14 : I2CM Master Interrupt Status Register  */
#define AST2600_I2CM_PKT_TIMEOUT		BIT(18)
#define AST2600_I2CM_PKT_ERROR			BIT(17)
#define AST2600_I2CM_PKT_DONE			BIT(16)

#define AST2600_I2CM_BUS_RECOVER_FAIL	BIT(15)
#define AST2600_I2CM_SDA_DL_TO			BIT(14)
#define AST2600_I2CM_BUS_RECOVER		BIT(13)
#define AST2600_I2CM_SMBUS_ALT			BIT(12)

#define AST2600_I2CM_SCL_LOW_TO			BIT(6)
#define AST2600_I2CM_ABNORMAL			BIT(5)
#define AST2600_I2CM_NORMAL_STOP		BIT(4)
#define AST2600_I2CM_ARBIT_LOSS			BIT(3)
#define AST2600_I2CM_RX_DONE			BIT(2)
#define AST2600_I2CM_TX_NAK				BIT(1)
#define AST2600_I2CM_TX_ACK				BIT(0)

/* 0x18 : I2CM Master Command/Status Register   */
#define AST2600_I2CM_PKT_ADDR(x)		(((x) & GENMASK(6, 0)) << 24)
#define AST2600_I2CM_PKT_EN				BIT(16)
#define AST2600_I2CM_SDA_OE_OUT_DIR		BIT(15)
#define AST2600_I2CM_SDA_O_OUT_DIR		BIT(14)
#define AST2600_I2CM_SCL_OE_OUT_DIR		BIT(13)
#define AST2600_I2CM_SCL_O_OUT_DIR		BIT(12)
#define AST2600_I2CM_RECOVER_CMD_EN		BIT(11)

#define AST2600_I2CM_RX_DMA_EN			BIT(9)
#define AST2600_I2CM_TX_DMA_EN			BIT(8)
/* Command Bit */
#define AST2600_I2CM_RX_BUFF_EN			BIT(7)
#define AST2600_I2CM_TX_BUFF_EN			BIT(6)
#define AST2600_I2CM_STOP_CMD			BIT(5)
#define AST2600_I2CM_RX_CMD_LAST		BIT(4)
#define AST2600_I2CM_RX_CMD				BIT(3)

#define AST2600_I2CM_TX_CMD				BIT(1)
#define AST2600_I2CM_START_CMD			BIT(0)

#define I2C_TIMEOUT_US 100000
/*
 * Device private data
 */
struct ast2600_i2c_priv {
	struct clk clk;
	struct ast2600_i2c_regs *regs;
	void __iomem *global;
};

static int ast2600_i2c_read_data(struct ast2600_i2c_priv *priv, u8 chip_addr,
				 u8 *buffer, size_t len, bool send_stop)
{
	int rx_cnt, ret = 0;
	u32 cmd, isr;

	for (rx_cnt = 0; rx_cnt < len; rx_cnt++, buffer++) {
		cmd = AST2600_I2CM_PKT_EN | AST2600_I2CM_PKT_ADDR(chip_addr) |
		      AST2600_I2CM_RX_CMD;
		if (!rx_cnt)
			cmd |= AST2600_I2CM_START_CMD;

		if ((len - 1) == rx_cnt)
			cmd |= AST2600_I2CM_RX_CMD_LAST;

		if (send_stop && ((len - 1) == rx_cnt))
			cmd |= AST2600_I2CM_STOP_CMD;

		writel(cmd, &priv->regs->cmd_sts);

		ret = readl_poll_timeout(&priv->regs->isr, isr,
					 isr & AST2600_I2CM_PKT_DONE,
					 I2C_TIMEOUT_US);
		if (ret)
			return -ETIMEDOUT;

		*buffer =
			AST2600_I2CC_GET_RX_BUFF(readl(&priv->regs->trx_buff));

		writel(AST2600_I2CM_PKT_DONE, &priv->regs->isr);

		if (isr & AST2600_I2CM_TX_NAK)
			return -EREMOTEIO;
	}

	return 0;
}

static int ast2600_i2c_write_data(struct ast2600_i2c_priv *priv, u8 chip_addr,
				  u8 *buffer, size_t len, bool send_stop)
{
	int tx_cnt, ret = 0;
	u32 cmd, isr;

	if (!len) {
		cmd = AST2600_I2CM_PKT_EN | AST2600_I2CM_PKT_ADDR(chip_addr) |
		      AST2600_I2CM_START_CMD;
		writel(cmd, &priv->regs->cmd_sts);
		ret = readl_poll_timeout(&priv->regs->isr, isr,
					 isr & AST2600_I2CM_PKT_DONE,
					 I2C_TIMEOUT_US);
		if (ret)
			return -ETIMEDOUT;

		writel(AST2600_I2CM_PKT_DONE, &priv->regs->isr);

		if (isr & AST2600_I2CM_TX_NAK)
			return -EREMOTEIO;
	}

	for (tx_cnt = 0; tx_cnt < len; tx_cnt++, buffer++) {
		cmd = AST2600_I2CM_PKT_EN | AST2600_I2CM_PKT_ADDR(chip_addr);
		cmd |= AST2600_I2CM_TX_CMD;

		if (!tx_cnt)
			cmd |= AST2600_I2CM_START_CMD;

		if (send_stop && ((len - 1) == tx_cnt))
			cmd |= AST2600_I2CM_STOP_CMD;

		writel(*buffer, &priv->regs->trx_buff);
		writel(cmd, &priv->regs->cmd_sts);
		ret = readl_poll_timeout(&priv->regs->isr, isr,
					 isr & AST2600_I2CM_PKT_DONE,
					 I2C_TIMEOUT_US);
		if (ret)
			return -ETIMEDOUT;

		writel(AST2600_I2CM_PKT_DONE, &priv->regs->isr);

		if (isr & AST2600_I2CM_TX_NAK)
			return -EREMOTEIO;
	}

	return 0;
}

static int ast2600_i2c_deblock(struct udevice *dev)
{
	struct ast2600_i2c_priv *priv = dev_get_priv(dev);
	u32 csr = readl(&priv->regs->cmd_sts);
	u32 isr;
	int ret;

	//reinit
	writel(0, &priv->regs->fun_ctrl);
	/* Enable Master Mode. Assuming single-master */
	writel(AST2600_I2CC_BUS_AUTO_RELEASE | AST2600_I2CC_MASTER_EN |
		       AST2600_I2CC_MULTI_MASTER_DIS,
	       &priv->regs->fun_ctrl);

	csr = readl(&priv->regs->cmd_sts);

	if (!(csr & AST2600_I2CC_SDA_LINE_STS) &&
	    (csr & AST2600_I2CC_SCL_LINE_STS)) {
		debug("Bus stuck (%x), attempting recovery\n", csr);
		writel(AST2600_I2CM_RECOVER_CMD_EN, &priv->regs->cmd_sts);
		ret = readl_poll_timeout(&priv->regs->isr, isr,
					 isr & (AST2600_I2CM_BUS_RECOVER_FAIL |
						AST2600_I2CM_BUS_RECOVER),
					 I2C_TIMEOUT_US);
		writel(~0, &priv->regs->isr);
		if (ret)
			return -EREMOTEIO;
	}

	return 0;
}

static int ast2600_i2c_xfer(struct udevice *dev, struct i2c_msg *msg, int nmsgs)
{
	struct ast2600_i2c_priv *priv = dev_get_priv(dev);
	int ret;

	if (readl(&priv->regs->trx_buff) & AST2600_I2CC_BUS_BUSY_STS)
		return -EREMOTEIO;

	for (; nmsgs > 0; nmsgs--, msg++) {
		if (msg->flags & I2C_M_RD) {
			debug("i2c_read: chip=0x%x, len=0x%x, flags=0x%x\n",
			      msg->addr, msg->len, msg->flags);
			ret = ast2600_i2c_read_data(priv, msg->addr, msg->buf,
						    msg->len, (nmsgs == 1));
		} else {
			debug("i2c_write: chip=0x%x, len=0x%x, flags=0x%x\n",
			      msg->addr, msg->len, msg->flags);
			ret = ast2600_i2c_write_data(priv, msg->addr, msg->buf,
						     msg->len, (nmsgs == 1));
		}
		if (ret) {
			debug("%s: error (%d)\n", __func__, ret);
			return -EREMOTEIO;
		}
	}

	return 0;
}

static int ast2600_i2c_set_speed(struct udevice *dev, unsigned int speed)
{
	struct ast2600_i2c_priv *priv = dev_get_priv(dev);
	unsigned long base_clk1, base_clk2, base_clk3, base_clk4;
	int baseclk_idx;
	u32 clk_div_reg;
	u32 apb_clk;
	u32 scl_low;
	u32 scl_high;
	int divisor;
	int inc = 0;
	u32 data;

	debug("Setting speed for I2C%d to <%u>\n", dev->seq_, speed);
	if (!speed) {
		debug("No valid speed specified\n");
		return -EINVAL;
	}

	apb_clk = clk_get_rate(&priv->clk);

	clk_div_reg = readl(priv->global + 0x10);
	base_clk1 = (apb_clk * 10) / ((((clk_div_reg & 0xff) + 2) * 10) / 2);
	base_clk2 =
		(apb_clk * 10) / (((((clk_div_reg >> 8) & 0xff) + 2) * 10) / 2);
	base_clk3 = (apb_clk * 10) /
		    (((((clk_div_reg >> 16) & 0xff) + 2) * 10) / 2);
	base_clk4 = (apb_clk * 10) /
		    (((((clk_div_reg >> 24) & 0xff) + 2) * 10) / 2);

	if ((apb_clk / speed) <= 32) {
		baseclk_idx = 0;
		divisor = DIV_ROUND_UP(apb_clk, speed);
	} else if ((base_clk1 / speed) <= 32) {
		baseclk_idx = 1;
		divisor = DIV_ROUND_UP(base_clk1, speed);
	} else if ((base_clk2 / speed) <= 32) {
		baseclk_idx = 2;
		divisor = DIV_ROUND_UP(base_clk2, speed);
	} else if ((base_clk3 / speed) <= 32) {
		baseclk_idx = 3;
		divisor = DIV_ROUND_UP(base_clk3, speed);
	} else {
		baseclk_idx = 4;
		divisor = DIV_ROUND_UP(base_clk4, speed);
		inc = 0;
		while ((divisor + inc) > 32) {
			inc |= divisor & 0x1;
			divisor >>= 1;
			baseclk_idx++;
		}
		divisor += inc;
	}
	divisor = min_t(int, divisor, 32);
	baseclk_idx &= 0xf;
	scl_low = ((divisor * 9) / 16) - 1;
	scl_low = min_t(u32, scl_low, 0xf);
	scl_high = (divisor - scl_low - 2) & 0xf;
	/* Divisor : Base Clock : tCKHighMin : tCK High : tCK Low  */
	data = ((scl_high - 1) << 20) | (scl_high << 16) | (scl_low << 12) |
	       (baseclk_idx);
	/* Set AC Timing */
	writel(data, &priv->regs->ac_timing);

	return 0;
}

static int ast2600_i2c_probe(struct udevice *dev)
{
	struct ast2600_i2c_priv *priv = dev_get_priv(dev);
	struct udevice *misc_dev;
	int ret;

	/* find global base address */
	ret = uclass_get_device_by_driver(UCLASS_MISC,
					  DM_DRIVER_GET(aspeed_i2c_global),
					  &misc_dev);
	if (ret) {
		debug("i2c global not defined\n");
		return ret;
	}

	priv->global = devfdt_get_addr_ptr(misc_dev);
	if (IS_ERR(priv->global)) {
		debug("%s(): can't get global\n", __func__);
		return PTR_ERR(priv->global);
	}

	/* Reset device */
	writel(0, &priv->regs->fun_ctrl);
	/* Enable Master Mode. Assuming single-master */
	writel(AST2600_I2CC_BUS_AUTO_RELEASE | AST2600_I2CC_MASTER_EN |
		       AST2600_I2CC_MULTI_MASTER_DIS,
	       &priv->regs->fun_ctrl);

	writel(0, &priv->regs->ier);
	/* Clear Interrupt */
	writel(~0, &priv->regs->isr);

	return 0;
}

static int ast2600_i2c_of_to_plat(struct udevice *dev)
{
	struct ast2600_i2c_priv *priv = dev_get_priv(dev);
	int ret;

	priv->regs = dev_read_addr_ptr(dev);
	if (!(priv->regs))
		return -EINVAL;

	ret = clk_get_by_index(dev, 0, &priv->clk);
	if (ret < 0) {
		debug("%s: Can't get clock for %s: %d\n", __func__, dev->name,
		      ret);
		return ret;
	}

	return 0;
}

static const struct dm_i2c_ops ast2600_i2c_ops = {
	.xfer = ast2600_i2c_xfer,
	.deblock = ast2600_i2c_deblock,
	.set_bus_speed = ast2600_i2c_set_speed,
};

static const struct udevice_id ast2600_i2c_ids[] = {
	{ .compatible = "aspeed,ast2600-i2c-bus" },
	{ .compatible = "aspeed,ast2700-i2c-bus" },
	{},
};

U_BOOT_DRIVER(ast2600_i2c) = {
	.name = "ast2600_i2c",
	.id = UCLASS_I2C,
	.of_match = ast2600_i2c_ids,
	.probe = ast2600_i2c_probe,
	.of_to_plat = ast2600_i2c_of_to_plat,
	.priv_auto = sizeof(struct ast2600_i2c_priv),
	.ops = &ast2600_i2c_ops,
};

#define AST2600_I2CG_ISR			0x00
#define AST2600_I2CG_SLAVE_ISR		0x04
#define AST2600_I2CG_OWNER		    0x08
#define AST2600_I2CG_CTRL		    0x0C
#define AST2600_I2CG_CLK_DIV_CTRL	0x10

#define AST2600_I2CG_SLAVE_PKT_NAK	    BIT(4)
#define AST2600_I2CG_M_S_SEPARATE_INTR	BIT(3)
#define AST2600_I2CG_CTRL_NEW_REG	    BIT(2)
#define AST2600_I2CG_CTRL_NEW_CLK_DIV	BIT(1)

#define AST2600_GLOBAL_INIT					\
			(AST2600_I2CG_SLAVE_PKT_NAK |	\
			AST2600_I2CG_CTRL_NEW_REG |		\
			AST2600_I2CG_CTRL_NEW_CLK_DIV)
#define I2CCG_DIV_CTRL 0xC6411208

struct ast2600_i2c_global_priv {
	void __iomem *regs;
	struct reset_ctl reset;
};

/*
 * APB clk : 100Mhz
 * div  : scl       : baseclk [APB/((div/2) + 1)] : tBuf [1/bclk * 16]
 * I2CG10[31:24] base clk4 for i2c auto recovery timeout counter (0xC6)
 * I2CG10[23:16] base clk3 for Standard-mode (100Khz) min tBuf 4.7us
 * 0x3c : 100.8Khz  : 3.225Mhz                    : 4.96us
 * 0x3d : 99.2Khz   : 3.174Mhz                    : 5.04us
 * 0x3e : 97.65Khz  : 3.125Mhz                    : 5.12us
 * 0x40 : 97.75Khz  : 3.03Mhz                     : 5.28us
 * 0x41 : 99.5Khz   : 2.98Mhz                     : 5.36us (default)
 * I2CG10[15:8] base clk2 for Fast-mode (400Khz) min tBuf 1.3us
 * 0x12 : 400Khz    : 10Mhz                       : 1.6us
 * I2CG10[7:0] base clk1 for Fast-mode Plus (1Mhz) min tBuf 0.5us
 * 0x08 : 1Mhz      : 20Mhz                       : 0.8us
 */

static int aspeed_i2c_global_probe(struct udevice *dev)
{
	struct ast2600_i2c_global_priv *i2c_global = dev_get_priv(dev);
	int ret = 0;

	i2c_global->regs = dev_read_addr_ptr(dev);
	if (!(i2c_global->regs))
		return -EINVAL;

	debug("%s(dev=%p)\n", __func__, dev);

	ret = reset_get_by_index(dev, 0, &i2c_global->reset);
	if (ret) {
		printf("%s(): Failed to get reset signal\n", __func__);
		return ret;
	}

	reset_deassert(&i2c_global->reset);

	writel(AST2600_GLOBAL_INIT, i2c_global->regs +
		AST2600_I2CG_CTRL);
	writel(I2CCG_DIV_CTRL, i2c_global->regs +
		AST2600_I2CG_CLK_DIV_CTRL);

	return 0;
}

static const struct udevice_id aspeed_i2c_global_ids[] = {
	{	.compatible = "aspeed,ast2600-i2c-global",	},
	{	.compatible = "aspeed,ast2700-i2c-global",	},
	{ }
};

U_BOOT_DRIVER(aspeed_i2c_global) = {
	.name		= "aspeed_i2c_global",
	.id			= UCLASS_MISC,
	.of_match	= aspeed_i2c_global_ids,
	.probe		= aspeed_i2c_global_probe,
	.priv_auto  = sizeof(struct ast2600_i2c_global_priv),
};

