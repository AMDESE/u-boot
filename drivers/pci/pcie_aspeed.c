// SPDX-License-Identifier: GPL-2.0+
#include <common.h>
#include <dm.h>
#include <reset.h>
#include <fdtdec.h>
#include <syscon.h>
#include <regmap.h>
#include <pci.h>
#include <asm/io.h>
#include <asm/arch/ahbc_aspeed.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <linux/bitfield.h>
#include "pcie_aspeed.h"

DECLARE_GLOBAL_DATA_PTR;

/*
 * Aspeed PCIe RC variants
 */
enum aspeed_pcie_rc_model {
	ASTEED_AST2600,
	ASTEED_AST2700,
};

struct pcie_aspeed_platform {
	int (*setup)(struct udevice *dev);
	void (*read_config)(const struct udevice *bus, pci_dev_t bdf, uint offset, ulong *valuep,
			    enum pci_size_t size);
	void (*write_config)(struct udevice *bus, pci_dev_t bdf, uint offset, ulong value,
			     enum pci_size_t size);
};
struct pcie_aspeed {
	struct pcie_aspeed_platform *platform;

	struct ast2600_h2x_reg *h2x_reg_26;
	struct ast2700_h2x_reg *h2x_reg_27;

	int domain;
	struct regmap *pciephy;
	struct regmap *device;
	u8 tx_tag;
};

static u8 txTag;

void aspeed_pcie_set_slot_power_limit(struct pcie_aspeed *pcie, int slot)
{
	u32 timeout = 0;
	struct ast2600_h2x_reg *h2x_reg = pcie->h2x_reg_26;

	//optional : set_slot_power_limit
	switch (slot) {
	case 0:
		writel(BIT(4) | readl(&h2x_reg->h2x_rc_l_ctrl),
		       &h2x_reg->h2x_rc_l_ctrl);
		break;
	case 1:
		writel(BIT(4) | readl(&h2x_reg->h2x_rc_h_ctrl),
		       &h2x_reg->h2x_rc_h_ctrl);
		break;
	}

	txTag %= 0x7;

	writel(0x74000001, &h2x_reg->h2x_tx_desc3);
	switch (slot) {
	case 0:	//write for 0.8.0
		writel(0x00400050 | (txTag << 8), &h2x_reg->h2x_tx_desc2);
		break;
	case 1:	//write for 0.4.0
		writel(0x00200050 | (txTag << 8), &h2x_reg->h2x_tx_desc2);
		break;
	}
	writel(0x0, &h2x_reg->h2x_tx_desc1);
	writel(0x0, &h2x_reg->h2x_tx_desc0);

	writel(0x1a, &h2x_reg->h2x_tx_data);

	//trigger tx
	writel(PCIE_TRIGGER_TX, &h2x_reg->h2x_reg24);

	//wait tx idle
	while (!(readl(&h2x_reg->h2x_reg24) & BIT(31))) {
		timeout++;
		if (timeout > 1000)
			goto out;
	};

	//write clr tx idle
	writel(1, &h2x_reg->h2x_reg08);
	timeout = 0;

	switch (slot) {
	case 0:
		//check tx status and clr rx done int
		while (!(readl(&h2x_reg->h2x_rc_l_isr) & PCIE_RC_RX_DONE_ISR)) {
			timeout++;
			if (timeout > 10)
				break;
			mdelay(1);
		}
		writel(PCIE_RC_RX_DONE_ISR, &h2x_reg->h2x_rc_l_isr);
		break;
	case 1:
		//check tx status and clr rx done int
		while (!(readl(&h2x_reg->h2x_rc_h_isr) & PCIE_RC_RX_DONE_ISR)) {
			timeout++;
			if (timeout > 10)
				break;
			mdelay(1);
		}
		writel(PCIE_RC_RX_DONE_ISR, &h2x_reg->h2x_rc_h_isr);
		break;
	}
out:
	txTag++;
}

static void pcie_ast2600_read_config(const struct udevice *bus, pci_dev_t bdf, uint offset,
				     ulong *valuep, enum pci_size_t size)
{
	struct pcie_aspeed *pcie = dev_get_priv(bus);
	struct ast2600_h2x_reg *h2x_reg = pcie->h2x_reg_26;
	u32 timeout = 0;
	u32 bdf_offset;
	u32 type = 0;
	int rx_done_fail = 0;

	/* Only allow one other device besides the local one on the local bus */
	if (PCI_BUS(bdf) == 1 && PCI_DEV(bdf) > 0) {
		debug("- out of range\n");
		/*
		 * If local dev is 0, the first other dev can
		 * only be 1
		 */
		*valuep = pci_get_ff(size);
		return;
	}

	if (PCI_BUS(bdf) == 2 && PCI_DEV(bdf) > 0) {
		debug("- out of range\n");
		/*
		 * If local dev is 0, the first other dev can
		 * only be 1
		 */
		*valuep = pci_get_ff(size);
		return;
	}

	//H2X80[4] (unlock) is write-only.
	//Driver may set H2X80/H2XC0[4]=1 before triggering next TX config.
	writel(BIT(4) | readl(&h2x_reg->h2x_rc_l_ctrl),
	       &h2x_reg->h2x_rc_l_ctrl);
	writel(BIT(4) | readl(&h2x_reg->h2x_rc_h_ctrl),
	       &h2x_reg->h2x_rc_h_ctrl);

	if (PCI_BUS(bdf) == 0)
		type = 0;
	else
		type = 1;

	bdf_offset = (PCI_BUS(bdf) << 24) |
					(PCI_DEV(bdf) << 19) |
					(PCI_FUNC(bdf) << 16) |
					(offset & ~3);

	txTag %= 0x7;

	writel(0x04000001 | (type << 24), &h2x_reg->h2x_tx_desc3);
	writel(0x0000200f | (txTag << 8), &h2x_reg->h2x_tx_desc2);
	writel(bdf_offset, &h2x_reg->h2x_tx_desc1);
	writel(0x00000000, &h2x_reg->h2x_tx_desc0);

	//trigger tx
	writel(PCIE_TRIGGER_TX, &h2x_reg->h2x_reg24);

	//wait tx idle
	while (!(readl(&h2x_reg->h2x_reg24) & PCIE_TX_IDLE)) {
		timeout++;
		if (timeout > 1000) {
			*valuep = 0xffffffff;
			goto out;
		}
	};

	//write clr tx idle
	writel(1, &h2x_reg->h2x_reg08);

	timeout = 0;
	//check tx status
	switch (readl(&h2x_reg->h2x_reg24) & PCIE_STATUS_OF_TX) {
	case PCIE_RC_L_TX_COMPLETE:
		while (!(readl(&h2x_reg->h2x_rc_l_isr) & PCIE_RC_RX_DONE_ISR)) {
			timeout++;
			if (timeout > 10) {
				rx_done_fail = 1;
				*valuep = 0xffffffff;
				break;
			}
			mdelay(1);
		}
		if (!rx_done_fail) {
			if (readl(&h2x_reg->h2x_rc_l_rxdesc2) & BIT(13))
				*valuep = 0xffffffff;
			else
				*valuep = readl(&h2x_reg->h2x_rc_l_rdata);
		}
		writel(BIT(4) | readl(&h2x_reg->h2x_rc_l_ctrl),
		       &h2x_reg->h2x_rc_l_ctrl);
		writel(readl(&h2x_reg->h2x_rc_l_isr),
		       &h2x_reg->h2x_rc_l_isr);
		break;
	case PCIE_RC_H_TX_COMPLETE:
		while (!(readl(&h2x_reg->h2x_rc_h_isr) & PCIE_RC_RX_DONE_ISR)) {
			timeout++;
			if (timeout > 10) {
				rx_done_fail = 1;
				*valuep = 0xffffffff;
				break;
			}
			mdelay(1);
		}
		if (!rx_done_fail) {
			if (readl(&h2x_reg->h2x_rc_h_rxdesc2) & BIT(13))
				*valuep = 0xffffffff;
			else
				*valuep = readl(&h2x_reg->h2x_rc_h_rdata);
		}
		writel(BIT(4) | readl(&h2x_reg->h2x_rc_h_ctrl),
		       &h2x_reg->h2x_rc_h_ctrl);
		writel(readl(&h2x_reg->h2x_rc_h_isr), &h2x_reg->h2x_rc_h_isr);
		break;
	default:	//read rc data
		*valuep = readl(&h2x_reg->h2x_rdata);
		break;
	}

out:
	txTag++;
}

static void pcie_ast2600_write_config(struct udevice *bus, pci_dev_t bdf, uint offset, ulong value,
				      enum pci_size_t size)
{
	struct pcie_aspeed *pcie = dev_get_priv(bus);
	struct ast2600_h2x_reg *h2x_reg = pcie->h2x_reg_26;
	u32 timeout = 0;
	u32 type = 0;
	u32 bdf_offset;
	u8 byte_en = 0;

	writel(BIT(4) | readl(&h2x_reg->h2x_rc_l_ctrl),
	       &h2x_reg->h2x_rc_l_ctrl);
	writel(BIT(4) | readl(&h2x_reg->h2x_rc_h_ctrl),
	       &h2x_reg->h2x_rc_h_ctrl);

	switch (size) {
	case PCI_SIZE_8:
		switch (offset % 4) {
		case 0:
			byte_en = 0x1;
			break;
		case 1:
			byte_en = 0x2;
			break;
		case 2:
			byte_en = 0x4;
			break;
		case 3:
			byte_en = 0x8;
			break;
		}
		break;
	case PCI_SIZE_16:
		switch ((offset >> 1) % 2) {
		case 0:
			byte_en = 0x3;
			break;
		case 1:
			byte_en = 0xc;
			break;
		}
		break;
	default:
		byte_en = 0xf;
		break;
	}

	if (PCI_BUS(bdf) == 0)
		type = 0;
	else
		type = 1;

	bdf_offset = (PCI_BUS(bdf) << 24) |
					(PCI_DEV(bdf) << 19) |
					(PCI_FUNC(bdf) << 16) |
					(offset & ~3);

	txTag %= 0x7;

	writel(0x44000001 | (type << 24), &h2x_reg->h2x_tx_desc3);
	writel(0x00002000 | (txTag << 8) | byte_en, &h2x_reg->h2x_tx_desc2);
	writel(bdf_offset, &h2x_reg->h2x_tx_desc1);
	writel(0x00000000, &h2x_reg->h2x_tx_desc0);

	value = pci_conv_size_to_32(0x0, value, offset, size);

	writel(value, &h2x_reg->h2x_tx_data);

	//trigger tx
	writel(1, &h2x_reg->h2x_reg24);

	//wait tx idle
	while (!(readl(&h2x_reg->h2x_reg24) & BIT(31))) {
		timeout++;
		if (timeout > 1000)
			goto out;
	};

	//write clr tx idle
	writel(1, &h2x_reg->h2x_reg08);

	timeout = 0;
	//check tx status and clr rx done int
	switch (readl(&h2x_reg->h2x_reg24) & PCIE_STATUS_OF_TX) {
	case PCIE_RC_L_TX_COMPLETE:
		while (!(readl(&h2x_reg->h2x_rc_l_isr) & PCIE_RC_RX_DONE_ISR)) {
			timeout++;
			if (timeout > 10)
				break;
			mdelay(1);
		}
		writel(PCIE_RC_RX_DONE_ISR, &h2x_reg->h2x_rc_l_isr);
		break;
	case PCIE_RC_H_TX_COMPLETE:
		while (!(readl(&h2x_reg->h2x_rc_h_isr) & PCIE_RC_RX_DONE_ISR)) {
			timeout++;
			if (timeout > 10)
				break;
			mdelay(1);
		}
		writel(PCIE_RC_RX_DONE_ISR, &h2x_reg->h2x_rc_h_isr);
		break;
	}

out:
	txTag++;
}

static void pcie_ast2700_read_config(const struct udevice *pbus, pci_dev_t bdf, uint offset,
				     ulong *valuep, enum pci_size_t size)
{
	struct pcie_aspeed *pcie = dev_get_priv(pbus);
	struct ast2700_h2x_reg *h2x_reg = pcie->h2x_reg_27;
	u32 bdf_offset, status, type;
	u32 bus = PCI_BUS(bdf);
	u32 dev = PCI_DEV(bdf);
	u32 func = PCI_FUNC(bdf);
	int ret;

	if (bus == 0 && (dev != 0 || func != 0)) {
		*valuep = pci_get_ff(size);
		return;
	}

	if (bus == 0) {
		/* Internal access to bridge */
		writel(0xF << 16 | (offset & ~3), &h2x_reg->h2x_cfgi_tlp);
		writel(CFGI_TLP_FIRE, &h2x_reg->h2x_cfgi_ctrl);
		*valuep = readl(&h2x_reg->h2x_cfgi_rdata);
	} else {
		bdf_offset = (bus << 24) | (dev << 19) | (func << 16) | (offset & ~3);

		pcie->tx_tag %= 0xF;

		/* bus range:
		 * CPU Node 0 = 0x00 - 0x3F
		 * CPU Node 1 = 0x40 - 0x7F
		 * IO         = 0x80 - 0xFF
		 * The first bus on echo RC must send header with type 0.
		 */
		type = ((bus & 0x3F) == 1) ? PCI_HEADER_TYPE_NORMAL : PCI_HEADER_TYPE_BRIDGE;

		/* Prepare TLP */
		writel(CRG_READ_FMTTYPE(type) | CRG_PAYLOAD_SIZE, &h2x_reg->h2x_cfge_tlp1);
		writel(0x40100F | (pcie->tx_tag << 8), &h2x_reg->h2x_cfge_tlpn);
		writel(bdf_offset, &h2x_reg->h2x_cfge_tlpn);
		/* Clear TX/RX status */
		writel(CFGE_TX_IDLE | CFGE_RX_BUSY, &h2x_reg->h2x_int_sts);
		/* Issue command */
		writel(CFGE_TLP_FIRE, &h2x_reg->h2x_cfge_ctrl);

		/* Check TX */
		ret = readl_poll_timeout(&h2x_reg->h2x_int_sts, status, (status & CFGE_TX_IDLE),
					 50);
		if (ret) {
			printf("[%X:%02X:%02X.%02X]CR tx timeout sts: 0x%08x\n", pcie->domain, bus,
			       dev, func, status);
			*valuep = pci_get_ff(size);
			goto out;
		}

		/* Check RX */
		ret = readl_poll_timeout(&h2x_reg->h2x_int_sts, status, (status & CFGE_RX_BUSY),
					 50);
		if (ret) {
			printf("RC [%04X:%02X:%02X.%02X] : RX Conf. timeout, sts: %x\n",
			       pcie->domain, bus, dev, func, status);
			*valuep = pci_get_ff(size);
			goto out;
		}
		*valuep = readl(&h2x_reg->h2x_cfge_data);
	}

out:
	/* Clear status */
	writel(status, &h2x_reg->h2x_int_sts);
	pcie->tx_tag++;
}

static void pcie_ast2700_write_config(struct udevice *pbus, pci_dev_t bdf, uint offset, ulong value,
				      enum pci_size_t size)
{
	struct pcie_aspeed *pcie = dev_get_priv(pbus);
	struct ast2700_h2x_reg *h2x_reg = pcie->h2x_reg_27;
	u32 bdf_offset, status, type;
	u32 bus = PCI_BUS(bdf);
	u32 dev = PCI_DEV(bdf);
	u32 func = PCI_FUNC(bdf);
	u32 shift = 8 * (offset & 3);
	u8 byte_en;
	int ret;

	if (bus == 0 && (dev != 0 || func != 0))
		return;

	switch (size) {
	case PCI_SIZE_8:
		byte_en = 1 << (offset % 4);
		value = (value & 0xff) << shift;
		break;
	case PCI_SIZE_16:
		byte_en = (((offset >> 1) % 2) == 0) ? 0x3 : 0xc;
		value = (value & 0xffff) << shift;
		break;
	default:
		byte_en = 0xf;
		break;
	}

	if (bus == 0) {
		/* Internal access to bridge */
		writel(0x100000 | byte_en << 16 | (offset & ~3), &h2x_reg->h2x_cfgi_tlp);
		writel(value, &h2x_reg->h2x_cfgi_wdata);
		writel(CFGI_TLP_FIRE, &h2x_reg->h2x_cfgi_ctrl);
	} else {
		bdf_offset = (bus << 24) | (dev << 19) | (func << 16) | (offset & ~3);

		pcie->tx_tag %= 0xF;

		/* bus range:
		 * CPU Node 0 = 0x00 - 0x3F
		 * CPU Node 1 = 0x40 - 0x7F
		 * IO         = 0x80 - 0xFF
		 * The first bus on echo RC must send header with type 0.
		 */
		type = ((bus & 0x3F) == 1) ? PCI_HEADER_TYPE_NORMAL : PCI_HEADER_TYPE_BRIDGE;

		/* Prepare TLP */
		writel(CRG_WRITE_FMTTYPE(type) | CRG_PAYLOAD_SIZE, &h2x_reg->h2x_cfge_tlp1);
		writel(0x401000 | (pcie->tx_tag << 8) | byte_en, &h2x_reg->h2x_cfge_tlpn);
		writel(bdf_offset, &h2x_reg->h2x_cfge_tlpn);
		writel(value, &h2x_reg->h2x_cfge_tlpn);
		/* Clear TX/RX idle status */
		writel(CFGE_TX_IDLE | CFGE_RX_BUSY, &h2x_reg->h2x_int_sts);
		/* Issue command */
		writel(CFGE_TLP_FIRE, &h2x_reg->h2x_cfge_ctrl);

		/* Check TX */
		ret = readl_poll_timeout(&h2x_reg->h2x_int_sts, status, (status & CFGE_TX_IDLE),
					 50);
		if (ret)
			printf("[%X:%02X:%02X.%02X]CT tx timeout sts: 0x%08x\n", pcie->domain, bus,
			       dev, func, status);

		/* Check RX */
		ret = readl_poll_timeout(&h2x_reg->h2x_int_sts, status, (status & CFGE_RX_BUSY),
					 50);
		if (ret)
			printf("RC [%04X:%02X:%02X.%02X] : TX Conf. timeout, sts: %x\n",
			       pcie->domain, bus, dev, func, status);

		(void)readl(&h2x_reg->h2x_cfge_data);
	}

	/* Clear status */
	writel(status, &h2x_reg->h2x_int_sts);
	pcie->tx_tag++;
}

static int pcie_aspeed_read_config(const struct udevice *bus, pci_dev_t bdf, uint offset,
				   ulong *valuep, enum pci_size_t size)
{
	struct pcie_aspeed *pcie = dev_get_priv(bus);

	debug("PCIE CFG read: (b,d,f)=(%2d,%2d,%2d)\n",
	      PCI_BUS(bdf), PCI_DEV(bdf), PCI_FUNC(bdf));

	pcie->platform->read_config(bus, bdf, offset, valuep, size);

	*valuep = pci_conv_32_to_size(*valuep, offset, size);
	debug("(addr,val)=(0x%04x, 0x%08lx)\n", offset, *valuep);

	return 0;
}

static int pcie_aspeed_write_config(struct udevice *bus, pci_dev_t bdf, uint offset, ulong value,
				    enum pci_size_t size)
{
	struct pcie_aspeed *pcie = dev_get_priv(bus);

	debug("PCIE CFG write: (b,d,f)=(%2d,%2d,%2d) ",
	      PCI_BUS(bdf), PCI_DEV(bdf), PCI_FUNC(bdf));
	debug("(addr,val)=(0x%04x, 0x%08lx)\n", offset, value);

	pcie->platform->write_config(bus, bdf, offset, value, size);

	return 0;
}

void aspeed_pcie_rc_slot_enable(struct pcie_aspeed *pcie, int slot)

{
	struct ast2600_h2x_reg *h2x_reg = pcie->h2x_reg_26;

	switch (slot) {
	case 0:
		//rc_l
		writel(PCIE_RX_LINEAR | PCIE_RX_MSI_EN |
				PCIE_WAIT_RX_TLP_CLR |
				PCIE_RC_RX_ENABLE | PCIE_RC_ENABLE,
				&h2x_reg->h2x_rc_l_ctrl);
		//assign debug tx tag
		writel((uintptr_t)&h2x_reg->h2x_rc_l_ctrl, &h2x_reg->h2x_rc_l_tx_tag);
		break;
	case 1:
		//rc_h
		writel(PCIE_RX_LINEAR | PCIE_RX_MSI_EN |
				PCIE_WAIT_RX_TLP_CLR |
				PCIE_RC_RX_ENABLE | PCIE_RC_ENABLE,
				&h2x_reg->h2x_rc_h_ctrl);
		//assign debug tx tag
		writel((uintptr_t)&h2x_reg->h2x_rc_h_ctrl, &h2x_reg->h2x_rc_h_tx_tag);
		break;
	}
}

int pcie_ast2600_setup(struct udevice *dev)
{
	struct ofnode_phandle_args phandle_args;
	struct reset_ctl reset_ctl, rc0_reset_ctl, rc1_reset_ctl;
	struct pcie_aspeed *pcie = (struct pcie_aspeed *)dev_get_priv(dev);
	struct ast2600_h2x_reg *h2x_reg = pcie->h2x_reg_26;
	struct udevice *ahbc_dev, *slot0_dev, *slot1_dev;
	int slot0_of_handle, slot1_of_handle;
	int ret = 0;

	txTag = 0;
	ret = reset_get_by_index(dev, 0, &reset_ctl);
	if (ret) {
		printf("%s(): Failed to get pcie reset signal\n", __func__);
		return ret;
	}

	reset_assert(&reset_ctl);
	mdelay(1);
	reset_deassert(&reset_ctl);

	//workaround : Send vender define message for avoid when PCIE RESET send unknown message out
	writel(0x34000000, &h2x_reg->h2x_tx_desc3);
	writel(0x0000007f, &h2x_reg->h2x_tx_desc2);
	writel(0x00001a03, &h2x_reg->h2x_tx_desc1);
	writel(0x00000000, &h2x_reg->h2x_tx_desc0);

	ret = uclass_get_device_by_driver(UCLASS_MISC, DM_DRIVER_GET(aspeed_ahbc), &ahbc_dev);
	if (ret) {
		debug("ahbc device not defined\n");
		return ret;
	}
	aspeed_ahbc_remap_enable(devfdt_get_addr_ptr(ahbc_dev));

	//ahb to pcie rc
	writel(0xe0006000, &h2x_reg->h2x_reg60);
	writel(0x0, &h2x_reg->h2x_reg64);
	writel(0xFFFFFFFF, &h2x_reg->h2x_reg68);

	//PCIe Host Enable
	writel(BIT(0), &h2x_reg->h2x_reg00);

	slot0_of_handle =
		dev_read_phandle_with_args(dev, "slot0-handle", NULL, 0, 0, &phandle_args);
	if (!slot0_of_handle) {
		ret = reset_get_by_index(dev, 1, &rc0_reset_ctl);
		if (ret) {
			printf("%s(): Failed to get rc low reset signal\n", __func__);
			return ret;
		}
		aspeed_pcie_rc_slot_enable(pcie, 0);
		reset_deassert(&rc0_reset_ctl);
		mdelay(50);
		if (uclass_get_device_by_ofnode(UCLASS_MISC, phandle_args.node, &slot0_dev))
			goto slot1;
		if (aspeed_pcie_phy_link_status(slot0_dev))
			aspeed_pcie_set_slot_power_limit(pcie, 0);
	}

slot1:
	slot1_of_handle =
		dev_read_phandle_with_args(dev, "slot1-handle", NULL, 0, 0, &phandle_args);
	if (!slot1_of_handle) {
		ret = reset_get_by_index(dev, 2, &rc1_reset_ctl);
		if (ret) {
			printf("%s(): Failed to get rc high reset signal\n", __func__);
			return ret;
		}
		aspeed_pcie_rc_slot_enable(pcie, 1);
		reset_deassert(&rc1_reset_ctl);
		mdelay(50);
		if (uclass_get_device_by_ofnode(UCLASS_MISC, phandle_args.node, &slot1_dev))
			goto end;
		if (aspeed_pcie_phy_link_status(slot1_dev))
			aspeed_pcie_set_slot_power_limit(pcie, 1);
	}
end:
	return 0;
}

int pcie_ast2700_setup(struct udevice *dev)
{
	struct pcie_aspeed *pcie = (struct pcie_aspeed *)dev_get_priv(dev);
	struct ast2700_h2x_reg *h2x_reg = pcie->h2x_reg_27;
	struct reset_ctl h2xrst, perst, perst_oe;
	u32 cfg_val;
	int ret = 0;

	pcie->tx_tag = 0;

	ret = reset_get_by_name(dev, "h2x", &h2xrst);
	if (ret) {
		printf("%s(): Failed to get h2x reset\n", __func__);
		return ret;
	}

	ret = reset_get_by_name(dev, "perst", &perst);
	if (ret) {
		printf("%s(): Failed to get perst reset\n", __func__);
		return ret;
	}

	ret = reset_get_by_name(dev, "perst_oe", &perst_oe);
	if (ret) {
		printf("%s(): Failed to get perst output enable\n", __func__);
		return ret;
	}

	pcie->pciephy = syscon_regmap_lookup_by_phandle(dev, "pciephy");
	if (IS_ERR(pcie->pciephy)) {
		printf("can't allocate pciephy\n");
		return PTR_ERR(pcie->pciephy);
	}

	pcie->device = syscon_regmap_lookup_by_phandle(dev, "aspeed,device");
	if (IS_ERR(pcie->device)) {
		printf("can't allocate scu device\n");
		return PTR_ERR(pcie->device);
	}

	pcie->domain = dev_read_u32_default(dev, "linux,pci-domain", 0);

	/* Set perst to output enable by reset function */
	reset_assert(&perst_oe);
	mdelay(10);
	reset_assert(&perst);

	regmap_write(pcie->pciephy, PEHR_VID_DID, 0x11501a02);
	regmap_write(pcie->pciephy, PEHR_MISC_70, 0xa00c0);
	regmap_write(pcie->pciephy, PEHR_MISC_78, 0x80030);
	regmap_write(pcie->pciephy, PEHR_MISC_58, LOCAL_SCALE_SUP);

	regmap_update_bits(pcie->device, SCU_60,
			   RC_E2M_PATH_EN | RC_H2XS_PATH_EN | RC_H2XD_PATH_EN | RC_H2XX_PATH_EN |
				   RC_UPSTREAM_MEM_EN,
			   RC_E2M_PATH_EN | RC_H2XS_PATH_EN | RC_H2XD_PATH_EN | RC_H2XX_PATH_EN |
				   RC_UPSTREAM_MEM_EN);
	regmap_write(pcie->device, SCU_64, 0xff00ff00);
	regmap_write(pcie->device, SCU_70, 0);
	regmap_write(pcie->device, SCU_78, (pcie->domain == 1) ? BIT(31) : 0);

	reset_assert(&h2xrst);
	mdelay(10);
	reset_deassert(&h2xrst);

	regmap_write(pcie->pciephy, PEHR_MISC_5C, 0x40000000);
	/* Configure to Root port */
	regmap_read(pcie->pciephy, PEHR_MISC_60, &cfg_val);
	regmap_write(pcie->pciephy, PEHR_MISC_60,
		     (cfg_val & ~PORT_TPYE) | FIELD_PREP(PORT_TPYE, PORT_TYPE_ROOT));

	writel(0, &h2x_reg->h2x_ctrl);
	writel(H2X_BRIDGE_EN | H2X_BRIDGE_DIRECT_EN, &h2x_reg->h2x_ctrl);

	/* The BAR mapping:
	 * CPU Node0: 0x60000000
	 * CPU Node1: 0x80000000
	 * IO       : 0xa0000000
	 */
	writel(0x60000000 + (0x20000000 * pcie->domain), &h2x_reg->h2x_remap_direct);

	reset_deassert(&perst);
	mdelay(1000);

	return 0;
}

static struct pcie_aspeed_platform pcie_ast2600 = {
	.setup = pcie_ast2600_setup,
	.read_config = pcie_ast2600_read_config,
	.write_config = pcie_ast2600_write_config,
};

static struct pcie_aspeed_platform pcie_ast2700 = {
	.setup = pcie_ast2700_setup,
	.read_config = pcie_ast2700_read_config,
	.write_config = pcie_ast2700_write_config,
};

static int pcie_aspeed_probe(struct udevice *dev)
{
	struct pcie_aspeed *pcie = (struct pcie_aspeed *)dev_get_priv(dev);

	if (pcie->platform->setup)
		return pcie->platform->setup(dev);

	return -EINVAL;
}

static int pcie_aspeed_of_to_plat(struct udevice *dev)
{
	struct pcie_aspeed *pcie = dev_get_priv(dev);

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
		printf("%s(): invalid data from udevce_id\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static const struct dm_pci_ops pcie_aspeed_ops = {
	.read_config	= pcie_aspeed_read_config,
	.write_config	= pcie_aspeed_write_config,
};

static const struct udevice_id pcie_aspeed_ids[] = {
	{ .compatible = "aspeed,ast2600-pcie", .data = ASTEED_AST2600 },
	{ .compatible = "aspeed,ast2700-pcie", .data = ASTEED_AST2700 },
	{ }
};

U_BOOT_DRIVER(pcie_aspeed) = {
	.name			= "pcie_aspeed",
	.id			= UCLASS_PCI,
	.of_match		= pcie_aspeed_ids,
	.ops			= &pcie_aspeed_ops,
	.of_to_plat		= pcie_aspeed_of_to_plat,
	.probe			= pcie_aspeed_probe,
	.priv_auto		= sizeof(struct pcie_aspeed),
};
