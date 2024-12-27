// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright ASPEED Technology Inc.
 */
#include <common.h>
#include <clk.h>
#include <dm.h>
#include <errno.h>
#include <fdtdec.h>
#include <i2c.h>
#include <log.h>
#include <regmap.h>
#include <reset.h>
#include <syscon.h>
#include <asm/io.h>
#include <linux/iopoll.h>
#include "ast2600_i2c.h"

/* Device private data */
struct ast2600_i2c_priv {
	int version;
	struct clk clk;
	struct ast2600_i2c_regs *regs;
	struct ast2600_i2c_global_regs *global_regs;
};

static int ast2700_i2c_read_data(struct ast2600_i2c_priv *priv, u8 chip_addr,
				 u8 *buffer, size_t len, bool send_stop)
{
	int rx_cnt, ret = 0;
	u32 cmd, isr;
	u8 tx_data[32];
	u8 rx_data[32];

	/* Set DMA buffer */
	writel(I2CM_SET_DMA_BASE_H((uintptr_t)&tx_data), &priv->regs->m_dma_tx_hi);
	writel(I2CM_SET_DMA_BASE_H((uintptr_t)&rx_data), &priv->regs->m_dma_rx_hi);
	writel(I2CM_SET_DMA_BASE_L((uintptr_t)&tx_data), &priv->regs->m_dma_txa);
	writel(I2CM_SET_DMA_BASE_L((uintptr_t)&rx_data), &priv->regs->m_dma_rxa);

	for (rx_cnt = 0; rx_cnt < len; rx_cnt++, buffer++) {
		cmd = I2CM_PKT_EN | I2CM_PKT_ADDR(chip_addr) | I2CM_RX_DMA_EN |
		      I2CM_RX_CMD;

		if (!rx_cnt)
			cmd |= I2CM_START_CMD;

		if ((len - 1) == rx_cnt)
			cmd |= I2CM_RX_CMD_LAST;

		if (send_stop && ((len - 1) == rx_cnt))
			cmd |= I2CM_STOP_CMD;

		writel(0, &priv->regs->m_dma_len);
		writel(I2CM_SET_RX_DMA_LEN(0), &priv->regs->m_dma_len);

		writel(cmd, &priv->regs->cmd_sts);

		ret = readl_poll_timeout(&priv->regs->isr, isr,
					 isr & I2CM_PKT_DONE,
					 I2C_TIMEOUT_US);

		if (ret)
			return -ETIMEDOUT;

		*buffer = rx_data[0];

		writel(isr, &priv->regs->isr);

		if (isr & I2CM_TX_NAK)
			return -EREMOTEIO;
	}

	return 0;
}

static int ast2700_i2c_write_data(struct ast2600_i2c_priv *priv, u8 chip_addr,
				  u8 *buffer, size_t len, bool send_stop)
{
	int tx_cnt, ret = 0;
	u32 cmd, isr;
	u8 tx_data[32];
	u8 rx_data[32];

	/* Set DMA buffer */
	writel(I2CM_SET_DMA_BASE_H((uintptr_t)&tx_data), &priv->regs->m_dma_tx_hi);
	writel(I2CM_SET_DMA_BASE_H((uintptr_t)&rx_data), &priv->regs->m_dma_rx_hi);
	writel(I2CM_SET_DMA_BASE_L((uintptr_t)&tx_data), &priv->regs->m_dma_txa);
	writel(I2CM_SET_DMA_BASE_L((uintptr_t)&rx_data), &priv->regs->m_dma_rxa);

	/* scan case */
	if (!len) {
		cmd = I2CM_PKT_EN | I2CM_PKT_ADDR(chip_addr) |
		      I2CM_START_CMD | I2CM_STOP_CMD;

		writel(cmd, &priv->regs->cmd_sts);
		ret = readl_poll_timeout(&priv->regs->isr, isr,
					 isr & I2CM_PKT_DONE,
					 I2C_TIMEOUT_US);
		if (ret)
			return -ETIMEDOUT;

		writel(isr, &priv->regs->isr);

		if (isr & I2CM_TX_NAK)
			return -EREMOTEIO;
	}

	/* write case */
	for (tx_cnt = 0; tx_cnt < len; tx_cnt++, buffer++) {
		cmd = I2CM_TX_DMA_EN | I2CM_PKT_EN | I2CM_PKT_ADDR(chip_addr);
		cmd |= I2CM_TX_CMD;

		if (!tx_cnt)
			cmd |= I2CM_START_CMD;

		if (send_stop && ((len - 1) == tx_cnt))
			cmd |= I2CM_STOP_CMD;

		tx_data[0] = *buffer;
		flush_dcache_range((uintptr_t)&tx_data, (uintptr_t)&tx_data);

		writel(0, &priv->regs->m_dma_len);
		writel(I2CM_SET_TX_DMA_LEN(0), &priv->regs->m_dma_len);

		writel(cmd, &priv->regs->cmd_sts);
		ret = readl_poll_timeout(&priv->regs->isr, isr,
					 isr & I2CM_PKT_DONE,
					 I2C_TIMEOUT_US);

		if (ret)
			return -ETIMEDOUT;

		writel(isr, &priv->regs->isr);

		if (isr & I2CM_TX_NAK)
			return -EREMOTEIO;
	}

	return 0;
}

static int ast2600_i2c_read_data(struct ast2600_i2c_priv *priv, u8 chip_addr,
				 u8 *buffer, size_t len, bool send_stop)
{
	int rx_cnt, ret = 0;
	u32 cmd, isr;

	for (rx_cnt = 0; rx_cnt < len; rx_cnt++, buffer++) {
		cmd = I2CM_PKT_EN | I2CM_PKT_ADDR(chip_addr) |
		      I2CM_RX_CMD;
		if (!rx_cnt)
			cmd |= I2CM_START_CMD;

		if ((len - 1) == rx_cnt)
			cmd |= I2CM_RX_CMD_LAST;

		if (send_stop && ((len - 1) == rx_cnt))
			cmd |= I2CM_STOP_CMD;

		writel(cmd, &priv->regs->cmd_sts);

		ret = readl_poll_timeout(&priv->regs->isr, isr,
					 isr & I2CM_PKT_DONE,
					 I2C_TIMEOUT_US);
		if (ret)
			return -ETIMEDOUT;

		*buffer =
			I2CC_GET_RX_BUFF(readl(&priv->regs->trx_buff));

		writel(I2CM_PKT_DONE, &priv->regs->isr);

		if (isr & I2CM_TX_NAK)
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
		cmd = I2CM_PKT_EN | I2CM_PKT_ADDR(chip_addr) |
		      I2CM_START_CMD;

		writel(cmd, &priv->regs->cmd_sts);
		ret = readl_poll_timeout(&priv->regs->isr, isr,
					 isr & I2CM_PKT_DONE,
					 I2C_TIMEOUT_US);
		if (ret)
			return -ETIMEDOUT;

		writel(I2CM_PKT_DONE, &priv->regs->isr);

		if (isr & I2CM_TX_NAK)
			return -EREMOTEIO;
	}

	for (tx_cnt = 0; tx_cnt < len; tx_cnt++, buffer++) {
		cmd = I2CM_PKT_EN | I2CM_PKT_ADDR(chip_addr);
		cmd |= I2CM_TX_CMD;

		if (!tx_cnt)
			cmd |= I2CM_START_CMD;

		if (send_stop && ((len - 1) == tx_cnt))
			cmd |= I2CM_STOP_CMD;

		writel(*buffer, &priv->regs->trx_buff);
		writel(cmd, &priv->regs->cmd_sts);
		ret = readl_poll_timeout(&priv->regs->isr, isr,
					 isr & I2CM_PKT_DONE,
					 I2C_TIMEOUT_US);
		if (ret)
			return -ETIMEDOUT;

		writel(I2CM_PKT_DONE, &priv->regs->isr);

		if (isr & I2CM_TX_NAK)
			return -EREMOTEIO;
	}

	return 0;
}

static int i2c_do_deblock(struct udevice *dev)
{
	struct ast2600_i2c_priv *priv = dev_get_priv(dev);
	u32 csr = readl(&priv->regs->cmd_sts);
	u32 isr;
	int ret;

	/* reinit */
	writel(0, &priv->regs->fun_ctrl);
	/* Enable Master Mode. Assuming single-master */
	writel(I2CC_BUS_AUTO_RELEASE | I2CC_MASTER_EN |
		       I2CC_MULTI_MASTER_DIS,
	       &priv->regs->fun_ctrl);

	csr = readl(&priv->regs->cmd_sts);

	if (!(csr & I2CC_SDA_LINE_STS) &&
	    (csr & I2CC_SCL_LINE_STS)) {
		debug("Bus stuck (%x), attempting recovery\n", csr);
		writel(I2CM_RECOVER_CMD_EN, &priv->regs->cmd_sts);
		ret = readl_poll_timeout(&priv->regs->isr, isr,
					 isr & (I2CM_BUS_RECOVER_FAIL |
						I2CM_BUS_RECOVER),
					 I2C_TIMEOUT_US);
		writel(~0, &priv->regs->isr);
		if (ret)
			return -EREMOTEIO;
	}

	return 0;
}

static int i2c_xfer(struct udevice *dev, struct i2c_msg *msg, int nmsgs)
{
	struct ast2600_i2c_priv *priv = dev_get_priv(dev);
	int ret;

	if (readl(&priv->regs->trx_buff) & I2CC_BUS_BUSY_STS)
		return -EREMOTEIO;

	for (; nmsgs > 0; nmsgs--, msg++) {
		if (msg->flags & I2C_M_RD) {
			debug("i2c_read: chip=0x%x, len=0x%x, flags=0x%x\n",
			      msg->addr, msg->len, msg->flags);
			if (priv->version == AST2600)
				ret = ast2600_i2c_read_data(priv, msg->addr, msg->buf, msg->len, (nmsgs == 1));
			else
				ret = ast2700_i2c_read_data(priv, msg->addr, msg->buf, msg->len, (nmsgs == 1));
		} else {
			debug("i2c_write: chip=0x%x, len=0x%x, flags=0x%x\n",
			      msg->addr, msg->len, msg->flags);
			if (priv->version == AST2600)
				ret = ast2600_i2c_write_data(priv, msg->addr, msg->buf, msg->len, (nmsgs == 1));
			else
				ret = ast2700_i2c_write_data(priv, msg->addr, msg->buf, msg->len, (nmsgs == 1));
		}
		if (ret) {
			debug("%s: error (%d)\n", __func__, ret);
			return -EREMOTEIO;
		}
	}

	return 0;
}

static int ast2700_i2c_set_speed(struct udevice *dev, unsigned int speed)
{
	struct ast2600_i2c_priv *priv = dev_get_priv(dev);
	unsigned long base_clk;
	int baseclk_idx;
	u32 apb_clk;
	u32 scl_low;
	u32 scl_high;
	int divisor;
	u32 data;

	debug("Setting speed for I2C%d to <%u>\n", dev->seq_, speed);
	if (!speed) {
		debug("No valid speed specified\n");
		return -EINVAL;
	}

	apb_clk = clk_get_rate(&priv->clk);
	for (int i = 0; i < 0x100; i++) {
		base_clk = (apb_clk) / (i + 1);
		if ((base_clk / speed) <= 32) {
			baseclk_idx = i;
			divisor = DIV_ROUND_UP(base_clk, speed);
			break;
		}
	}

	baseclk_idx = min_t(int, baseclk_idx, 0xff);
	divisor = min_t(int, divisor, 32);
	scl_low = ((divisor * 9) / 16) - 1;
	scl_low = min_t(u32, scl_low, 0xf);
	scl_high = (divisor - scl_low - 2) & 0xf;
	/* Divisor : Base Clock : tCKHighMin : tCK High : tCK Low  */
	data = ((scl_high - 1) << 20) | (scl_high << 16) | (scl_low << 12) |
	       baseclk_idx;
	/* Set AC Timing */
	writel(data, &priv->regs->ac_timing);

	return 0;
}

static int ast2600_i2c_set_speed(struct udevice *dev, unsigned int speed)
{
	struct ast2600_i2c_priv *priv = dev_get_priv(dev);
	unsigned long base_clk1, base_clk2, base_clk3, base_clk4;
	int multiply = 10;
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

	clk_div_reg = readl(&priv->global_regs->clk_divid);
	base_clk1 = (apb_clk * multiply) / (((GET_CLK1_DIV(clk_div_reg) + 2) * multiply) / 2);
	base_clk2 = (apb_clk * multiply) / (((GET_CLK2_DIV(clk_div_reg) + 2) * multiply) / 2);
	base_clk3 = (apb_clk * multiply) / (((GET_CLK3_DIV(clk_div_reg) + 2) * multiply) / 2);
	base_clk4 = (apb_clk * multiply) / (((GET_CLK4_DIV(clk_div_reg) + 2) * multiply) / 2);

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
	       baseclk_idx;
	/* Set AC Timing */
	writel(data, &priv->regs->ac_timing);

	return 0;
}

int i2c_set_speed(struct udevice *dev, unsigned int speed)
{
	struct ast2600_i2c_priv *priv = dev_get_priv(dev);
	int ret;

	if (priv->version == AST2600)
		ret = ast2600_i2c_set_speed(dev, speed);
	else
		ret = ast2700_i2c_set_speed(dev, speed);

	return ret;
}

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
static int ast2600_i2c_probe(struct udevice *dev)
{
	struct ast2600_i2c_priv *priv = dev_get_priv(dev);
	struct reset_ctl reset_ctl;
	int rc;

	rc = reset_get_by_index(dev, 0, &reset_ctl);
	if (rc) {
		printf("%s: Failed to get reset signal\n", __func__);
		return rc;
	}

	if (reset_status(&reset_ctl) > 0) {
		reset_assert(&reset_ctl);
		mdelay(10);
		reset_deassert(&reset_ctl);
	}

	writel(GLOBAL_INIT, &priv->global_regs->global_ctrl);

	/* set device specified setting */
	if (priv->version == AST2600)
		writel(I2CCG_DIV_CTRL, &priv->global_regs->clk_divid);

	/* Reset device */
	writel(0, &priv->regs->fun_ctrl);
	/* Enable Master Mode. Assuming single-master */
	writel(I2CC_BUS_AUTO_RELEASE | I2CC_MASTER_EN |
		       I2CC_MULTI_MASTER_DIS,
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
	if (!priv->regs)
		return -EINVAL;

	ret = clk_get_by_index(dev, 0, &priv->clk);
	if (ret < 0) {
		debug("%s: Can't get clock for %s: %d\n", __func__, dev->name,
		      ret);
		return ret;
	}

	priv->global_regs = (struct ast2600_i2c_global_regs *)((uintptr_t)priv->regs & 0xFFFFF000);

	priv->version = dev_get_driver_data(dev);

	return 0;
}

static const struct dm_i2c_ops ast2600_i2c_ops = {
	.xfer = i2c_xfer,
	.deblock = i2c_do_deblock,
	.set_bus_speed = i2c_set_speed,
};

static const struct udevice_id ast2600_i2c_ids[] = {
	{ .compatible = "aspeed,ast2600-i2cv2", .data = AST2600 },
	{ .compatible = "aspeed,ast2700-i2cv2", .data = AST2700 },
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

