// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2025 Aspeed Technology Inc.
 */
#include <asm/io.h>
#include <clk.h>
#include <dm.h>
#include <dm/devres.h>
#include <dm/read.h>
#include <generic-phy.h>
#include <linux/bitfield.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <pci.h>
#include <regmap.h>
#include <reset.h>
#include <stdio.h>
#include <syscon.h>

DECLARE_GLOBAL_DATA_PTR;

/*
 * PCIe r6.0, sec 6.6.1 <Conventional Reset>
 *
 * - "With a Downstream Port that does not support Link speeds greater
 *    than 5.0 GT/s, software must wait a minimum of 100 ms following exit
 *    from a Conventional Reset before sending a Configuration Request to
 *    the device immediately below that Port."
 *
 * - "With a Downstream Port that supports Link speeds greater than
 *    5.0 GT/s, software must wait a minimum of 100 ms after Link training
 *    completes before sending a Configuration Request to the device
 *    immediately below that Port."
 */
#define PCIE_RESET_CONFIG_WAIT_MS	100

#define ASPEED_RESET_RC_WAIT_MS		10

/* AST2600 AHBC Registers */
#define ASPEED_AHBC_KEY			0x00
#define  ASPEED_AHBC_UNLOCK_KEY			0xaeed1a03
#define  ASPEED_AHBC_UNLOCK			0x01
#define ASPEED_AHBC_ADDR_MAPPING	0x8c
#define  ASPEED_PCIE_RC_MEMORY_EN		BIT(5)

/* AST2600 H2X Controller Registers */
/* reg08 h2x_int_sts */
#define ASPEED_PCIE_TX_IDLE_CLEAR	BIT(0)
#define ASPEED_PCIE_INTX_STS		GENMASK(3, 0)
/* reg24 h2x_sts */
#define ASPEED_PCIE_TX_IDLE		BIT(31)
#define ASPEED_PCIE_STATUS_OF_TX	GENMASK(25, 24)
#define ASPEED_PCIE_RC_H_TX_COMPLETE	BIT(25)
#define ASPEED_PCIE_TRIGGER_TX		BIT(0)
/* reg60 h2x_ahb_addr_config0 */
#define ASPEED_AHB_REMAP_LO_ADDR(x)	((x) & GENMASK(15, 4))
#define ASPEED_AHB_MASK_LO_ADDR(x)	FIELD_PREP(GENMASK(31, 20), x)
/* reg64 h2x_ahb_addr_config1 */
#define ASPEED_AHB_REMAP_HI_ADDR(x)	(x)
/* reg68 h2x_ahb_addr_config2 */
#define ASPEED_AHB_MASK_HI_ADDR(x)	(x)
/* regc0 h2x_dev_ctrl */
#define ASPEED_PCIE_RX_DMA_EN		BIT(9)
#define ASPEED_PCIE_RX_LINEAR		BIT(8)
#define ASPEED_PCIE_RX_MSI_SEL		BIT(7)
#define ASPEED_PCIE_RX_MSI_EN		BIT(6)
#define ASPEED_PCIE_UNLOCK_RX_BUFF	BIT(4)
#define ASPEED_PCIE_WAIT_RX_TLP_CLR	BIT(2)
#define ASPEED_PCIE_RC_RX_ENABLE	BIT(1)
#define ASPEED_PCIE_RC_ENABLE		BIT(0)
/* regc8 h2x_dev_sts */
#define ASPEED_PCIE_RC_RX_DONE_ISR	BIT(4)
/* regfc h2x_dev_tx_tag */
#define ASPEED_RC_TLP_TX_TAG_NUM	0x28

/* AST2700 H2X */
/* reg00 h2x_ctrl */
#define ASPEED_H2X_BRIDGE_EN		BIT(0)
#define ASPEED_H2X_BRIDGE_DIRECT_EN	BIT(1)
/* reg08 h2x_int_sts */
#define ASPEED_CFGE_TX_IDLE		BIT(0)
#define ASPEED_CFGE_RX_BUSY		BIT(1)
/* reg20 h2x_cfgi_tlp */
#define ASPEED_CFGI_BYTE_EN_MASK	GENMASK(19, 16)
#define ASPEED_CFGI_BYTE_EN(x) \
		FIELD_PREP(ASPEED_CFGI_BYTE_EN_MASK, (x))
/* reg24 h2x_cfgi_wdata*/
#define ASPEED_CFGI_WRITE		BIT(20)
/* reg28 h2x_cfgi_ctrl*/
#define ASPEED_CFGI_TLP_FIRE		BIT(0)
/* reg38 h2x_cfge_ctrl*/
#define ASPEED_CFGE_TLP_FIRE		BIT(0)
/* reg70 h2x_prefetch_addr */
#define ASPEED_REMAP_PREF_ADDR_63_32(x)	(x)
/* reg74 h2x_remap_pci_addr_hi */
#define ASPEED_REMAP_PCI_ADDR_63_32(x)	((u32)((u64)(x) >> 32))
/* reg78 h2x_remap_pci_addr_lo */
#define ASPEED_REMAP_PCI_ADDR_31_12(x)	((x) & GENMASK(31, 12))

/* AST2700 SCU */
#define ASPEED_SCU_60			0x60
#define  ASPEED_RC_E2M_PATH_EN			BIT(0)
#define  ASPEED_RC_H2XS_PATH_EN			BIT(16)
#define  ASPEED_RC_H2XD_PATH_EN			BIT(17)
#define  ASPEED_RC_H2XX_PATH_EN			BIT(18)
#define  ASPEED_RC_UPSTREAM_MEM_EN		BIT(19)
#define ASPEED_SCU_64			0x64
#define  ASPEED_RC0_DECODE_DMA_BASE(x)		FIELD_PREP(GENMASK(7, 0), x)
#define  ASPEED_RC0_DECODE_DMA_LIMIT(x)		FIELD_PREP(GENMASK(15, 8), x)
#define  ASPEED_RC1_DECODE_DMA_BASE(x)		FIELD_PREP(GENMASK(23, 16), x)
#define  ASPEED_RC1_DECODE_DMA_LIMIT(x)		FIELD_PREP(GENMASK(31, 24), x)
#define ASPEED_SCU_70			0x70
#define  ASPEED_DISABLE_EP_FUNC			0

/* AST2700 SCU1 PERST */
#define SCU1_PCIE3_CTRL		0x0
#define   SCU1_PCIE3_PERST_OUTPUT	BIT(1)
#define   SCU1_PCIE3_PERST_DEASSERT	BIT(0)
#define   SCU1_PCIE3_PERST_ASSERT	0

/* Format of TLP; PCIe r7.0, sec 2.2.1 */
#define PCIE_TLP_FMT_3DW_NO_DATA	0x00 /* 3DW header, no data */
#define PCIE_TLP_FMT_4DW_NO_DATA	0x01 /* 4DW header, no data */
#define PCIE_TLP_FMT_3DW_DATA		0x02 /* 3DW header, with data */
#define PCIE_TLP_FMT_4DW_DATA		0x03 /* 4DW header, with data */

/* Type of TLP; PCIe r7.0, sec 2.2.1 */
#define PCIE_TLP_TYPE_CFG0_RD		0x04 /* Config Type 0 Read Request */
#define PCIE_TLP_TYPE_CFG0_WR		0x04 /* Config Type 0 Write Request */
#define PCIE_TLP_TYPE_CFG1_RD		0x05 /* Config Type 1 Read Request */
#define PCIE_TLP_TYPE_CFG1_WR		0x05 /* Config Type 1 Write Request */

/* Cpl. status of Complete; PCIe r7.0, sec 2.2.9.1 */
#define PCIE_CPL_STS_SUCCESS		0x00 /* Successful Completion */

/* Macro to combine Fmt and Type into the 8-bit field */
#define ASPEED_TLP_FMT_TYPE(fmt, type)	((((fmt) & 0x7) << 5) | ((type) & 0x1f))
#define ASPEED_TLP_COMMON_FIELDS	GENMASK(31, 24)

/* Completion status */
#define CPL_STS(x)	FIELD_GET(GENMASK(15, 13), (x))
/* TLP configuration type 0 and type 1 */
#define CFG0_READ_FMTTYPE                                        \
	FIELD_PREP(ASPEED_TLP_COMMON_FIELDS,                     \
		   ASPEED_TLP_FMT_TYPE(PCIE_TLP_FMT_3DW_NO_DATA, \
				       PCIE_TLP_TYPE_CFG0_RD))
#define CFG0_WRITE_FMTTYPE                                    \
	FIELD_PREP(ASPEED_TLP_COMMON_FIELDS,                  \
		   ASPEED_TLP_FMT_TYPE(PCIE_TLP_FMT_3DW_DATA, \
				       PCIE_TLP_TYPE_CFG0_WR))
#define CFG1_READ_FMTTYPE                                        \
	FIELD_PREP(ASPEED_TLP_COMMON_FIELDS,                     \
		   ASPEED_TLP_FMT_TYPE(PCIE_TLP_FMT_3DW_NO_DATA, \
				       PCIE_TLP_TYPE_CFG1_RD))
#define CFG1_WRITE_FMTTYPE                                    \
	FIELD_PREP(ASPEED_TLP_COMMON_FIELDS,                  \
		   ASPEED_TLP_FMT_TYPE(PCIE_TLP_FMT_3DW_DATA, \
				       PCIE_TLP_TYPE_CFG1_WR))
#define CFG_PAYLOAD_SIZE		0x01 /* 1 DWORD */
#define TLP_HEADER_BYTE_EN(x, y)	((GENMASK((x) - 1, 0) << ((y) % 4)))
#define TLP_GET_VALUE(x, y, z)	\
	(((x) >> ((((z) % 4)) * 8)) & GENMASK((8 * (y)) - 1, 0))
#define TLP_SET_VALUE(x, y, z)	\
	((((x) & GENMASK((8 * (y)) - 1, 0)) << ((((z) % 4)) * 8)))
#define AST2600_TX_DESC1_VALUE		0x00002000
#define AST2700_TX_DESC1_VALUE		0x00401000

struct ast2600_h2x_reg {
	u32 h2x_ctrl;			// 0x00
	u32 h2x_reg04;
	u32 h2x_int_sts;		// 0x08
	u32 h2x_host_rx_desc_data;	// 0x0c
	u32 h2x_tx_desc0;		// 0x10
	u32 h2x_tx_desc1;		// 0x14
	u32 h2x_tx_desc2;		// 0x18
	u32 h2x_tx_desc3;		// 0x1c
	u32 h2x_tx_desc_data;		// 0x20
	u32 h2x_sts;			// 0x24
	u32 h2x_reserved0[14];		// 0x28 ~ 0x5c
	u32 h2x_ahb_addr_config0;	// 0x60
	u32 h2x_ahb_addr_config1;	// 0x64
	u32 h2x_ahb_addr_config2;	// 0x68
	u32 h2x_reserved1[21];		// 0x6c ~0xbc
	u32 h2x_dev_ctrl;		// 0xc0
	u32 h2x_regc4;
	u32 h2x_dev_sts;		// 0xc8
	u32 h2x_dev_rx_desc_data;	// 0xcc
	u32 h2x_regd0;
	u32 h2x_dev_rx_desc1;		// 0xd4
	u32 h2x_reserved[9];		// 0xd8~0xf8
	u32 h2x_dev_tx_tag;		// 0xfc
};

struct ast2700_h2x_reg {
	u32 h2x_ctrl;			// 0x00
	u32 h2x_reg04;
	u32 h2x_int_sts;		// 0x08
	u32 h2x_reserved0[5];		// 0x0c ~ 0x1c
	u32 h2x_cfgi_tlp;		// 0x20
	u32 h2x_cfgi_wdata;		// 0x24
	u32 h2x_cfgi_ctrl;		// 0x28
	u32 h2x_cfgi_rdata;		// 0x2c
	u32 h2x_cfge_tlp1;		// 0x30
	u32 h2x_cfge_tlpn;		// 0x34
	u32 h2x_cfge_ctrl;		// 0x38
	u32 h2x_cfge_data;		// 0x3c
	u32 h2x_reserved[12];		// 0x40 ~ 0x6c
	u32 h2x_prefetch_addr;		// 0x70
	u32 h2x_remap_pci_addr_hi;	// 0x74
	u32 h2x_remap_pci_addr_lo;	// 0x78
};

/*
 * Aspeed PCIe RC variants
 */
enum aspeed_pcie_rc_model {
	ASTEED_AST2600,
	ASTEED_AST2700,
};

struct aspeed_pcie_port {
	struct list_head list;
	void __iomem *base;
	struct clk clk;
	struct phy phy;
	struct reset_ctl perst;
	u32 slot;
};

struct aspeed_pcie {
	struct aspeed_pcie_rc_platform *platform;
	struct list_head ports;

	struct ast2600_h2x_reg *h2x_reg_26;
	struct ast2700_h2x_reg *h2x_reg_27;

	struct reset_ctl h2xrst;

	int domain;
	struct regmap *cfg;
	struct regmap *ahbc;
	u8 tx_tag;

	u32 root_bus_nr;
};

struct aspeed_pcie_rc_platform {
	int (*setup)(struct udevice *dev);
	int (*read_config)(const struct udevice *bus, pci_dev_t bdf,
			   uint offset, ulong *valuep, enum pci_size_t size);
	int (*child_read_config)(const struct udevice *bus, pci_dev_t bdf,
				 uint offset, ulong *valuep,
				 enum pci_size_t size);
	int (*write_config)(const struct udevice *bus, pci_dev_t bdf,
			    uint offset, ulong value, enum pci_size_t size);
	int (*child_write_config)(const struct udevice *bus, pci_dev_t bdf,
				  uint offset, ulong value,
				  enum pci_size_t size);
	void (*pcie_map_ranges)(struct udevice *bus, pci_addr_t pci_addr);
};

static u32 aspeed_pcie_get_bdf_offset(pci_dev_t bdf, int offset)
{
	return (PCI_BUS(bdf) << 24) | (PCI_DEV(bdf) << 19) |
		(PCI_FUNC(bdf) << 16) | (offset & ~3);
}

static int aspeed_ast2600_conf(const struct udevice *pbus, pci_dev_t bdf,
			       uint offset, uint size, ulong *val, u32 fmt_type,
			       bool write)
{
	struct aspeed_pcie *pcie = dev_get_priv(pbus);
	struct ast2600_h2x_reg *h2x_reg = pcie->h2x_reg_26;
	u32 bdf_offset, cfg_val, isr;
	u32 bus = PCI_BUS(bdf);
	u32 dev = PCI_DEV(bdf);
	u32 func = PCI_FUNC(bdf);
	int ret;

	bdf_offset = aspeed_pcie_get_bdf_offset(bdf, offset);

	/* Driver may set unlock RX buffer before triggering next TX config */
	cfg_val = readl(&h2x_reg->h2x_dev_ctrl);
	writel(ASPEED_PCIE_UNLOCK_RX_BUFF | cfg_val,
	       &h2x_reg->h2x_dev_ctrl);

	cfg_val = fmt_type | CFG_PAYLOAD_SIZE;
	writel(cfg_val, &h2x_reg->h2x_tx_desc0);

	cfg_val = AST2600_TX_DESC1_VALUE |
		  FIELD_PREP(GENMASK(11, 8), pcie->tx_tag) |
		  TLP_HEADER_BYTE_EN(size, offset);
	writel(cfg_val, &h2x_reg->h2x_tx_desc1);

	writel(bdf_offset, &h2x_reg->h2x_tx_desc2);
	writel(0, &h2x_reg->h2x_tx_desc3);
	if (write)
		writel(TLP_SET_VALUE(*val, size, offset),
		       &h2x_reg->h2x_tx_desc_data);

	cfg_val = readl(&h2x_reg->h2x_sts);
	cfg_val |= ASPEED_PCIE_TRIGGER_TX;
	writel(cfg_val, &h2x_reg->h2x_sts);

	ret = readl_poll_timeout(&h2x_reg->h2x_sts, cfg_val,
				 (cfg_val & ASPEED_PCIE_TX_IDLE), 50);
	if (ret) {
		pr_err("%02x:%02x.%d CR tx timeout sts: 0x%08x\n",
		       bus, dev, func, cfg_val);
		goto out;
	}

	cfg_val = readl(&h2x_reg->h2x_int_sts);
	cfg_val |= ASPEED_PCIE_TX_IDLE_CLEAR;
	writel(cfg_val, &h2x_reg->h2x_int_sts);

	cfg_val = readl(&h2x_reg->h2x_sts);
	switch (cfg_val & ASPEED_PCIE_STATUS_OF_TX) {
	case ASPEED_PCIE_RC_H_TX_COMPLETE:
		ret = readl_poll_timeout(&h2x_reg->h2x_dev_sts, isr,
					 (isr & ASPEED_PCIE_RC_RX_DONE_ISR),
					 50);
		if (ret) {
			pr_err("%02x:%02x.%d CR rx timeout sts: 0x%08x\n",
			       bus, dev, func, isr);
			goto out;
		}
		if (!write) {
			cfg_val = readl(&h2x_reg->h2x_dev_rx_desc1);
			if (CPL_STS(cfg_val) != PCIE_CPL_STS_SUCCESS) {
				*val = pci_get_ff(size);
				ret = -EINVAL;
				goto out;
			} else {
				*val = readl(&h2x_reg->h2x_dev_rx_desc_data);
			}
		}
		break;
	case ASPEED_PCIE_STATUS_OF_TX:
		*val = pci_get_ff(size);
		ret = -EINVAL;
		goto out;
	default:
		*val = readl(&h2x_reg->h2x_host_rx_desc_data);
		break;
	}

	cfg_val = readl(&h2x_reg->h2x_dev_ctrl);
	cfg_val |= ASPEED_PCIE_UNLOCK_RX_BUFF;
	writel(cfg_val, &h2x_reg->h2x_dev_ctrl);

	*val = TLP_GET_VALUE(*val, size, offset);

	ret = 0;
out:
	cfg_val = readl(&h2x_reg->h2x_dev_sts);
	writel(cfg_val, &h2x_reg->h2x_dev_sts);
	pcie->tx_tag = (pcie->tx_tag + 1) % 0x8;
	return ret;
}

static int aspeed_ast2600_rd_conf(const struct udevice *pbus, pci_dev_t bdf,
				  uint offset, ulong *valuep,
				  enum pci_size_t size)
{
	if (PCI_DEV(bdf) != 8) {
		*valuep = pci_get_ff(size);
		return 0;
	}

	return aspeed_ast2600_conf(pbus, bdf, offset, (1 << size), valuep,
				   CFG0_READ_FMTTYPE, false);
}

static int aspeed_ast2600_child_rd_conf(const struct udevice *pbus,
					pci_dev_t bdf, uint offset,
					ulong *valuep, enum pci_size_t size)
{
	return aspeed_ast2600_conf(pbus, bdf, offset, (1 << size), valuep,
				   CFG1_READ_FMTTYPE, false);
}

static int aspeed_ast2600_wr_conf(const struct udevice *pbus, pci_dev_t bdf,
				  uint offset, ulong value,
				  enum pci_size_t size)
{
	if (PCI_DEV(bdf) != 8)
		return 0;

	return aspeed_ast2600_conf(pbus, bdf, offset, (1 << size), &value,
				   CFG0_WRITE_FMTTYPE, true);
}

static int aspeed_ast2600_child_wr_conf(const struct udevice *pbus,
					pci_dev_t bdf, uint offset,
					ulong value, enum pci_size_t size)
{
	return aspeed_ast2600_conf(pbus, bdf, offset, (1 << size), &value,
				   CFG1_WRITE_FMTTYPE, true);
}

static int aspeed_ast2700_config(const struct udevice *pbus, uint offset,
				 uint size, ulong *val, bool write)
{
	struct aspeed_pcie *pcie = dev_get_priv(pbus);
	struct ast2700_h2x_reg *h2x_reg = pcie->h2x_reg_27;
	u32 cfg_val;

	cfg_val = ASPEED_CFGI_BYTE_EN(TLP_HEADER_BYTE_EN(size, offset)) |
		  (offset & ~3);
	if (write)
		cfg_val |= ASPEED_CFGI_WRITE;
	writel(cfg_val, &h2x_reg->h2x_cfgi_tlp);

	writel(TLP_SET_VALUE(*val, size, offset), &h2x_reg->h2x_cfgi_wdata);
	writel(ASPEED_CFGI_TLP_FIRE, &h2x_reg->h2x_cfgi_ctrl);
	*val = readl(&h2x_reg->h2x_cfgi_rdata);
	*val = TLP_GET_VALUE(*val, size, offset);

	return 0;
}

static int aspeed_ast2700_child_config(const struct udevice *pbus,
				       pci_dev_t bdf, uint offset, ulong *val,
				       uint size, bool write)
{
	struct aspeed_pcie *pcie = dev_get_priv(pbus);
	struct ast2700_h2x_reg *h2x_reg = pcie->h2x_reg_27;
	u32 bdf_offset, status, cfg_val;
	u32 bus = PCI_BUS(bdf);
	u32 dev = PCI_DEV(bdf);
	u32 func = PCI_FUNC(bdf);
	int ret;

	bdf_offset = aspeed_pcie_get_bdf_offset(bdf, offset);

	cfg_val = CFG_PAYLOAD_SIZE;
	if (write)
		cfg_val |= (bus == (pcie->root_bus_nr + 1)) ?
				    CFG0_WRITE_FMTTYPE :
				    CFG1_WRITE_FMTTYPE;
	else
		cfg_val |= (bus == (pcie->root_bus_nr + 1)) ?
				    CFG0_READ_FMTTYPE :
				    CFG1_READ_FMTTYPE;

	writel(cfg_val, &h2x_reg->h2x_cfge_tlp1);

	cfg_val = AST2700_TX_DESC1_VALUE |
		  FIELD_PREP(GENMASK(11, 8), pcie->tx_tag) |
		  TLP_HEADER_BYTE_EN(size, offset);
	writel(cfg_val, &h2x_reg->h2x_cfge_tlpn);

	writel(bdf_offset, &h2x_reg->h2x_cfge_tlpn);
	if (write)
		writel(TLP_SET_VALUE(*val, size, offset),
		       &h2x_reg->h2x_cfge_tlpn);
	writel(ASPEED_CFGE_TX_IDLE | ASPEED_CFGE_RX_BUSY,
	       &h2x_reg->h2x_int_sts);
	writel(ASPEED_CFGE_TLP_FIRE, &h2x_reg->h2x_cfge_ctrl);

	ret = readl_poll_timeout(&h2x_reg->h2x_int_sts, status,
				 (status & ASPEED_CFGE_TX_IDLE), 50);
	if (ret) {
		pr_err("%02x:%02x.%d CR tx timeout sts: 0x%08x\n",
		       bus, dev, func, status);
		goto out;
	}

	ret = readl_poll_timeout(&h2x_reg->h2x_int_sts, status,
				 (status & ASPEED_CFGE_RX_BUSY), 50000);
	if (ret) {
		pr_err("%02x:%02x.%d CR rx timeout sts: 0x%08x\n",
		       bus, dev, func, status);
		goto out;
	}
	*val = readl(&h2x_reg->h2x_cfge_data);
	*val = TLP_GET_VALUE(*val, size, offset);

	ret = 0;
out:
	writel(status, &h2x_reg->h2x_int_sts);
	pcie->tx_tag = (pcie->tx_tag + 1) % 0xf;
	return ret;
}

static int aspeed_ast2700_rd_conf(const struct udevice *pbus, pci_dev_t bdf,
				  uint offset, ulong *valuep,
				  enum pci_size_t size)
{
	if (PCI_DEV(bdf) != 0) {
		*valuep = pci_get_ff(size);
		return 0;
	}

	return aspeed_ast2700_config(pbus, offset, (1 << size), valuep, false);
}

static int aspeed_ast2700_child_rd_conf(const struct udevice *pbus,
					pci_dev_t bdf, uint offset,
					ulong *valuep, enum pci_size_t size)
{
	return aspeed_ast2700_child_config(pbus, bdf, offset, valuep,
					   (1 << size), false);
}

static int aspeed_ast2700_wr_conf(const struct udevice *pbus, pci_dev_t bdf,
				  uint offset, ulong value,
				  enum pci_size_t size)
{
	if (PCI_DEV(bdf) != 0)
		return 0;

	return aspeed_ast2700_config(pbus, offset, (1 << size), &value, true);
}

static int aspeed_ast2700_child_wr_conf(const struct udevice *pbus,
					pci_dev_t bdf, uint offset,
					ulong value, enum pci_size_t size)
{
	return aspeed_ast2700_child_config(pbus, bdf, offset, &value,
					   (1 << size), true);
}

static int aspeed_pcie_read_config(const struct udevice *bus, pci_dev_t bdf,
				   uint offset, ulong *valuep,
				   enum pci_size_t size)
{
	struct aspeed_pcie *pcie = dev_get_priv(bus);
	struct aspeed_pcie_rc_platform *platform = pcie->platform;
	int ret;

	if (PCI_BUS(bdf) == pcie->root_bus_nr)
		ret = platform->read_config(bus, bdf, offset, valuep, size);
	else
		ret = platform->child_read_config(bus, bdf, offset, valuep,
						  size);

	return ret;
}

static int aspeed_pcie_write_config(struct udevice *bus, pci_dev_t bdf,
				    uint offset, ulong value,
				    enum pci_size_t size)
{
	struct aspeed_pcie *pcie = dev_get_priv(bus);
	struct aspeed_pcie_rc_platform *platform = pcie->platform;
	int ret;

	if (PCI_BUS(bdf) == pcie->root_bus_nr)
		ret = platform->write_config(bus, bdf, offset, value, size);
	else
		ret = platform->child_write_config(bus, bdf, offset, value, size);

	return ret;
}

static void aspeed_host_reset(struct aspeed_pcie *pcie)
{
	reset_assert(&pcie->h2xrst);
	mdelay(ASPEED_RESET_RC_WAIT_MS);
	reset_deassert(&pcie->h2xrst);
}

static int aspeed_ast2600_setup(struct udevice *dev)
{
	struct aspeed_pcie *pcie = (struct aspeed_pcie *)dev_get_priv(dev);
	struct ast2600_h2x_reg *h2x_reg = pcie->h2x_reg_26;

	pcie->domain = 0;

	pcie->ahbc = syscon_regmap_lookup_by_phandle(dev, "aspeed,ahbc");
	if (IS_ERR(pcie->ahbc))
		return PTR_ERR(pcie->ahbc);

	aspeed_host_reset(pcie);

	regmap_write(pcie->ahbc, ASPEED_AHBC_KEY, ASPEED_AHBC_UNLOCK_KEY);
	regmap_update_bits(pcie->ahbc, ASPEED_AHBC_ADDR_MAPPING,
			   ASPEED_PCIE_RC_MEMORY_EN, ASPEED_PCIE_RC_MEMORY_EN);
	regmap_write(pcie->ahbc, ASPEED_AHBC_KEY, ASPEED_AHBC_UNLOCK);

	writel(ASPEED_H2X_BRIDGE_EN, &h2x_reg->h2x_ctrl);

	writel(ASPEED_PCIE_RX_DMA_EN | ASPEED_PCIE_RX_LINEAR |
	       ASPEED_PCIE_RX_MSI_SEL | ASPEED_PCIE_RX_MSI_EN |
	       ASPEED_PCIE_WAIT_RX_TLP_CLR | ASPEED_PCIE_RC_RX_ENABLE |
	       ASPEED_PCIE_RC_ENABLE,
	       &h2x_reg->h2x_dev_ctrl);

	writel(ASPEED_RC_TLP_TX_TAG_NUM, &h2x_reg->h2x_dev_tx_tag);

	return 0;
}

static void aspeed_ast2600_pcie_map_ranges(struct udevice *dev,
					   pci_addr_t pci_addr)
{
	struct aspeed_pcie *pcie = (struct aspeed_pcie *)dev_get_priv(dev);
	u32 pci_addr_lo = pci_addr & GENMASK(31, 0);

	pci_addr_lo >>= 16;
	writel(ASPEED_AHB_REMAP_LO_ADDR(pci_addr_lo) |
	       ASPEED_AHB_MASK_LO_ADDR(0xe00),
	       &pcie->h2x_reg_26->h2x_ahb_addr_config0);
	writel(ASPEED_AHB_REMAP_HI_ADDR(0),
	       &pcie->h2x_reg_26->h2x_ahb_addr_config1);
	writel(ASPEED_AHB_MASK_HI_ADDR(~0),
	       &pcie->h2x_reg_26->h2x_ahb_addr_config2);
}

static int aspeed_ast2700_setup(struct udevice *dev)
{
	struct aspeed_pcie *pcie = (struct aspeed_pcie *)dev_get_priv(dev);
	struct ast2700_h2x_reg *h2x_reg = pcie->h2x_reg_27;

	pcie->domain = dev_read_u32_default(dev, "linux,pci-domain", 0);

	pcie->cfg = syscon_regmap_lookup_by_phandle(dev, "aspeed,pciecfg");
	if (IS_ERR(pcie->cfg))
		return PTR_ERR(pcie->cfg);

	regmap_update_bits(pcie->cfg, ASPEED_SCU_60,
			   ASPEED_RC_E2M_PATH_EN | ASPEED_RC_H2XS_PATH_EN |
			   ASPEED_RC_H2XD_PATH_EN | ASPEED_RC_H2XX_PATH_EN |
			   ASPEED_RC_UPSTREAM_MEM_EN,
			   ASPEED_RC_E2M_PATH_EN | ASPEED_RC_H2XS_PATH_EN |
			   ASPEED_RC_H2XD_PATH_EN | ASPEED_RC_H2XX_PATH_EN |
			   ASPEED_RC_UPSTREAM_MEM_EN);
	regmap_write(pcie->cfg, ASPEED_SCU_64,
		     ASPEED_RC0_DECODE_DMA_BASE(0) |
		     ASPEED_RC0_DECODE_DMA_LIMIT(0xff) |
		     ASPEED_RC1_DECODE_DMA_BASE(0) |
		     ASPEED_RC1_DECODE_DMA_LIMIT(0xff));
	regmap_write(pcie->cfg, ASPEED_SCU_70, ASPEED_DISABLE_EP_FUNC);

	aspeed_host_reset(pcie);

	writel(0, &h2x_reg->h2x_ctrl);
	writel(ASPEED_H2X_BRIDGE_EN | ASPEED_H2X_BRIDGE_DIRECT_EN,
	       &h2x_reg->h2x_ctrl);

	/* Prepare for 64-bit BAR pref */
	writel(ASPEED_REMAP_PREF_ADDR_63_32(0x3), &h2x_reg->h2x_prefetch_addr);

	return 0;
}

static void aspeed_ast2700_pcie_map_ranges(struct udevice *dev,
					   pci_addr_t pci_addr)
{
	struct aspeed_pcie *pcie = (struct aspeed_pcie *)dev_get_priv(dev);

	writel(ASPEED_REMAP_PCI_ADDR_63_32(pci_addr),
	       &pcie->h2x_reg_27->h2x_remap_pci_addr_hi);
	writel(ASPEED_REMAP_PCI_ADDR_31_12(pci_addr),
	       &pcie->h2x_reg_27->h2x_remap_pci_addr_lo);
}

static struct aspeed_pcie_rc_platform pcie_ast2600 = {
	.setup = aspeed_ast2600_setup,
	.read_config = aspeed_ast2600_rd_conf,
	.child_read_config = aspeed_ast2600_child_rd_conf,
	.write_config = aspeed_ast2600_wr_conf,
	.child_write_config = aspeed_ast2600_child_wr_conf,
	.pcie_map_ranges = aspeed_ast2600_pcie_map_ranges,
};

static struct aspeed_pcie_rc_platform pcie_ast2700 = {
	.setup = aspeed_ast2700_setup,
	.read_config = aspeed_ast2700_rd_conf,
	.child_read_config = aspeed_ast2700_child_rd_conf,
	.write_config = aspeed_ast2700_wr_conf,
	.child_write_config = aspeed_ast2700_child_wr_conf,
	.pcie_map_ranges = aspeed_ast2700_pcie_map_ranges,
};

static void aspeed_pcie_map_ranges(struct udevice *dev)
{
	struct aspeed_pcie *pcie = (struct aspeed_pcie *)dev_get_priv(dev);
	struct pci_region *io, *mem, *pref;

	pci_get_regions(dev, &io, &mem, &pref);

	if (mem)
		pcie->platform->pcie_map_ranges(dev, mem->bus_start);
}

static void aspeed_pcie_port_free(struct aspeed_pcie_port *port)
{
	list_del(&port->list);
	free(port);
}

static int aspeed_pcie_port_init(struct aspeed_pcie_port *port)
{
	int ret;

	ret = clk_enable(&port->clk);
	if (ret)
		goto err_clk;

	ret = generic_phy_init(&port->phy);
	if (ret)
		goto err_phy_init;

	return 0;

err_phy_init:
	clk_disable(&port->clk);
err_clk:
	aspeed_pcie_port_free(port);
	return ret;
}

static int aspeed_reset_get_by_name_nodev(ofnode node, const char *name,
					  struct reset_ctl *reset_ctl)
{
	int index = 0;

	reset_ctl->dev = NULL;

	if (name) {
		index = ofnode_stringlist_search(node, "reset-names", name);
		if (index < 0)
			return index;
	}

	return reset_get_by_index_nodev(node, index, reset_ctl);
}

static int aspeed_pcie_parse_port(struct udevice *dev, ofnode subnode, int slot)
{
	struct aspeed_pcie *pcie = (struct aspeed_pcie *)dev_get_priv(dev);
	struct aspeed_pcie_port *port;
	struct reset_ctl perst_oe;
	void *scu1_perst;
	int ret;

	port = devm_kzalloc(dev, sizeof(*port), GFP_KERNEL);
	if (!port)
		return -ENOMEM;

	ret = clk_get_by_index_nodev(subnode, 0, &port->clk);
	if (ret)
		return ret;

	ret = generic_phy_get_by_index_nodev(subnode, 0, &port->phy);
	if (ret)
		return ret;

	if (pcie->domain == 2) {
		ofnode perst_node;
		uint32_t phandle;

		ret = ofnode_read_u32(subnode, "aspeed,perst", &phandle);
		if (ret)
			return -ENODEV;

		perst_node = ofnode_get_by_phandle(phandle);
		if (!ofnode_valid(perst_node))
			return -ENODEV;

		scu1_perst = (void *)ofnode_get_addr(perst_node);
		if (scu1_perst == (void *)FDT_ADDR_T_NONE)
			return -ENODEV;

		writel(SCU1_PCIE3_PERST_OUTPUT | SCU1_PCIE3_PERST_ASSERT,
		       scu1_perst + SCU1_PCIE3_CTRL);
	} else {
		ret = aspeed_reset_get_by_name_nodev(subnode, "perst", &port->perst);
		if (ret)
			return ret;

		ret = aspeed_reset_get_by_name_nodev(subnode, "perst_oe", &perst_oe);
		if (ret)
			return ret;

		ret = reset_assert(&perst_oe);
		if (ret)
			return ret;
		mdelay(10);

		ret = reset_assert(&port->perst);
		if (ret)
			return ret;
	}

	port->slot = slot;

	INIT_LIST_HEAD(&port->list);
	list_add_tail(&port->list, &pcie->ports);

	ret = aspeed_pcie_port_init(port);
	if (ret)
		return ret;

	if (pcie->domain == 2) {
		writel(SCU1_PCIE3_PERST_OUTPUT | SCU1_PCIE3_PERST_DEASSERT,
		       scu1_perst + SCU1_PCIE3_CTRL);
	} else {
		ret = reset_deassert(&port->perst);
		if (ret)
			return ret;
	}
	mdelay(PCIE_RESET_CONFIG_WAIT_MS);

	return 0;
}

static int aspeed_pcie_parse_dt(struct udevice *dev)
{
	struct fdt_pci_addr addr;
	unsigned int slot;
	ofnode subnode;
	int ret;

	dev_for_each_subnode(subnode, dev) {
		ret = ofnode_read_pci_addr(subnode, 0, "reg", &addr);
		if (ret)
			return ret;

		slot = PCI_DEV(addr.phys_hi);

		ret = aspeed_pcie_parse_port(dev, subnode, slot);
		if (ret)
			return ret;
	}

	return 0;
}

static int aspeed_pcie_probe(struct udevice *dev)
{
	struct aspeed_pcie *pcie = (struct aspeed_pcie *)dev_get_priv(dev);
	const u32 *values;
	int len;
	int ret;

	pcie->tx_tag = 0;

	values = dev_read_prop(dev, "bus-range", &len);
	if (!values || len < sizeof(*values) * 2)
		return -EINVAL;
	pcie->root_bus_nr = be32_to_cpu(values[0]);

	ret = reset_get_by_name(dev, "h2x", &pcie->h2xrst);
	if (ret) {
		pr_err("Failed to get h2x reset: %d\n", ret);
		return ret;
	}

	ret = pcie->platform->setup(dev);
	if (ret) {
		pr_err("Failed to setup PCIe RC: %d\n", ret);
		return ret;
	}

	aspeed_pcie_map_ranges(dev);

	ret = aspeed_pcie_parse_dt(dev);
	if (ret)
		return ret;

	return 0;
}

static int aspeed_pcie_of_to_plat(struct udevice *dev)
{
	struct aspeed_pcie *pcie = dev_get_priv(dev);

	/* Get the controller base address */
	switch (dev_get_driver_data(dev)) {
	case ASTEED_AST2700:
		pcie->h2x_reg_27 = (void *)devfdt_get_addr_index(dev, 0);
		pcie->platform = &pcie_ast2700;
		break;
	case ASTEED_AST2600:
		pcie->h2x_reg_26 = (void *)devfdt_get_addr_index(dev, 0);
		pcie->platform = &pcie_ast2600;
		break;
	default:
		pr_err("%s(): invalid data from udevce_id\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static const struct dm_pci_ops aspeed_pcie_ops = {
	.read_config	= aspeed_pcie_read_config,
	.write_config	= aspeed_pcie_write_config,
};

static const struct udevice_id aspeed_pcie_ids[] = {
	{ .compatible = "aspeed,ast2600-pcie", .data = ASTEED_AST2600 },
	{ .compatible = "aspeed,ast2700-pcie", .data = ASTEED_AST2700 },
	{ }
};

U_BOOT_DRIVER(aspeed_pcie) = {
	.name			= "aspeed_pcie",
	.id			= UCLASS_PCI,
	.of_match		= aspeed_pcie_ids,
	.ops			= &aspeed_pcie_ops,
	.of_to_plat		= aspeed_pcie_of_to_plat,
	.probe			= aspeed_pcie_probe,
	.priv_auto		= sizeof(struct aspeed_pcie),
};
