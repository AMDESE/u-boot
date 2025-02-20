// SPDX-License-Identifier: GPL-2.0+
/*
 * Broadcom PHY drivers
 *
 * Copyright 2010-2011 Freescale Semiconductor, Inc.
 * author Andy Fleming
 */
#include <common.h>
#include <phy.h>
#include <linux/delay.h>

/* Broadcom BCM54xx -- taken from linux sungem_phy */
#define MIIM_BCM54xx_AUXCNTL			0x18
#define MIIM_BCM54xx_AUXCNTL_ENCODE(val) (((val & 0x7) << 12)|(val & 0x7))
#define MIIM_BCM54xx_AUXSTATUS			0x19
#define MIIM_BCM54xx_AUXSTATUS_LINKMODE_MASK	0x0700
#define MIIM_BCM54xx_AUXSTATUS_LINKMODE_SHIFT	8

#define MIIM_BCM54XX_SHD			0x1c
#define MIIM_BCM54XX_SHD_WRITE			0x8000
#define MIIM_BCM54XX_SHD_VAL(x)			((x & 0x1f) << 10)
#define MIIM_BCM54XX_SHD_DATA(x)		((x & 0x3ff) << 0)
#define MIIM_BCM54XX_SHD_WR_ENCODE(val, data)	\
	(MIIM_BCM54XX_SHD_WRITE | MIIM_BCM54XX_SHD_VAL(val) | \
	 MIIM_BCM54XX_SHD_DATA(data))

#define MIIM_BCM54XX_EXP_DATA		0x15	/* Expansion register data */
#define MIIM_BCM54XX_EXP_SEL		0x17	/* Expansion register select */
#define MIIM_BCM54XX_EXP_SEL_SSD	0x0e00	/* Secondary SerDes select */
#define MIIM_BCM54XX_EXP_SEL_ER		0x0f00	/* Expansion register select */

#define MIIM_BCM_AUXCNTL_SHDWSEL_MISC	0x0007
#define MIIM_BCM_AUXCNTL_ACTL_SMDSP_EN	0x0800

#define MIIM_BCM_CHANNEL_WIDTH    0x2000

#define MII_BRCM_FET_INTREG		0x1a	/* Interrupt register */
#define MII_BRCM_FET_IR_MASK		0x0100	/* Mask all interrupts */
#define MII_BRCM_FET_IR_LINK_EN		0x0200	/* Link status change enable */
#define MII_BRCM_FET_IR_SPEED_EN	0x0400	/* Link speed change enable */
#define MII_BRCM_FET_IR_DUPLEX_EN	0x0800	/* Duplex mode change enable */
#define MII_BRCM_FET_IR_ENABLE		0x4000	/* Interrupt enable */

#define MII_BRCM_FET_BRCMTEST		0x1f	/* Brcm test register */
#define MII_BRCM_FET_BT_SRE		0x0080	/* Shadow register enable */

#define MII_BRCM_FET_SHDW_AUXMODE4	0x1a	/* Auxiliary mode 4 */
#define MII_BRCM_FET_SHDW_AM4_STANDBY	0x0008	/* Standby enable */
#define MII_BRCM_FET_SHDW_AM4_LED_MASK	0x0003
#define MII_BRCM_FET_SHDW_AM4_LED_MODE1 0x0001

#define PHY_BRCM_AUTO_PWRDWN_ENABLE	0x00000001

#define MII_BRCM_FET_SHDW_AUXSTAT2	0x1b	/* Auxiliary status 2 */
#define MII_BRCM_FET_SHDW_AS2_APDE	0x0020	/* Auto power down enable */

/* BCM5221 Registers */
#define BCM5221_AEGSR			0x1C
#define BCM5221_AEGSR_MDIX_DIS		BIT(11)

#define BCM5221_SHDW_AM4_EN_CLK_LPM	BIT(2)
#define BCM5221_SHDW_AM4_FORCE_LPM	BIT(1)

static void bcm_phy_write_misc(struct phy_device *phydev,
			       u16 reg, u16 chl, u16 value)
{
	int reg_val;

	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_BCM54xx_AUXCNTL,
		  MIIM_BCM_AUXCNTL_SHDWSEL_MISC);

	reg_val = phy_read(phydev, MDIO_DEVAD_NONE, MIIM_BCM54xx_AUXCNTL);
	reg_val |= MIIM_BCM_AUXCNTL_ACTL_SMDSP_EN;
	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_BCM54xx_AUXCNTL, reg_val);

	reg_val = (chl * MIIM_BCM_CHANNEL_WIDTH) | reg;
	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_BCM54XX_EXP_SEL, reg_val);

	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_BCM54XX_EXP_DATA, value);
}

/* Broadcom BCM5461S */
static int bcm5461_config(struct phy_device *phydev)
{
	u32 reg18, reg1c;

	phy_reset(phydev);
	/*
	 * RX interface delay: reg 0x18, shadow value b'0111: misc control
	 * bit[8] RGMII RXD to RXC skew
	 */
	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_BCM54xx_AUXCNTL,
		  MIIM_BCM54xx_AUXCNTL_ENCODE(0x7));
	reg18 = phy_read(phydev, MDIO_DEVAD_NONE, MIIM_BCM54xx_AUXCNTL);
	reg18 &= ~(MIIM_BCM54xx_AUXCNTL_ENCODE(0x7) | BIT(8));
	reg18 |= BIT(15) | MIIM_BCM54xx_AUXCNTL_ENCODE(0x7);
	if (phydev->interface == PHY_INTERFACE_MODE_RGMII_ID ||
	    phydev->interface == PHY_INTERFACE_MODE_RGMII_RXID)
		reg18 |= BIT(8);
	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_BCM54xx_AUXCNTL, reg18);

	/*
	 * TX interface delay: reg 0x1c, shadow value b'0011: clock alignment control
	 * bit[9] GTXCLK clock delay enable
	 */
	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_BCM54XX_SHD,
		  MIIM_BCM54XX_SHD_VAL(0x3));
	reg1c = phy_read(phydev, MDIO_DEVAD_NONE, MIIM_BCM54XX_SHD);
	reg1c &= ~(MIIM_BCM54XX_SHD_VAL(0x1f) | BIT(9));
	reg1c |= BIT(15) | MIIM_BCM54XX_SHD_VAL(0x3);
	if (phydev->interface == PHY_INTERFACE_MODE_RGMII_ID ||
	    phydev->interface == PHY_INTERFACE_MODE_RGMII_TXID)
		reg1c |= BIT(9);
	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_BCM54XX_SHD, reg1c);

	genphy_config_aneg(phydev);

	return 0;
}

static int bcm54xx_parse_status(struct phy_device *phydev)
{
	unsigned int mii_reg;

	mii_reg = phy_read(phydev, MDIO_DEVAD_NONE, MIIM_BCM54xx_AUXSTATUS);

	switch ((mii_reg & MIIM_BCM54xx_AUXSTATUS_LINKMODE_MASK) >>
			MIIM_BCM54xx_AUXSTATUS_LINKMODE_SHIFT) {
	case 1:
		phydev->duplex = DUPLEX_HALF;
		phydev->speed = SPEED_10;
		break;
	case 2:
		phydev->duplex = DUPLEX_FULL;
		phydev->speed = SPEED_10;
		break;
	case 3:
		phydev->duplex = DUPLEX_HALF;
		phydev->speed = SPEED_100;
		break;
	case 5:
		phydev->duplex = DUPLEX_FULL;
		phydev->speed = SPEED_100;
		break;
	case 6:
		phydev->duplex = DUPLEX_HALF;
		phydev->speed = SPEED_1000;
		break;
	case 7:
		phydev->duplex = DUPLEX_FULL;
		phydev->speed = SPEED_1000;
		break;
	default:
		printf("Auto-neg error, defaulting to 10BT/HD\n");
		phydev->duplex = DUPLEX_HALF;
		phydev->speed = SPEED_10;
		break;
	}

	return 0;
}

static int bcm54xx_startup(struct phy_device *phydev)
{
	int ret;

	/* Read the Status (2x to make sure link is right) */
	ret = genphy_update_link(phydev);
	if (ret)
		return ret;

	return bcm54xx_parse_status(phydev);
}

/* Broadcom BCM5482S */
/*
 * "Ethernet@Wirespeed" needs to be enabled to achieve link in certain
 * circumstances.  eg a gigabit TSEC connected to a gigabit switch with
 * a 4-wire ethernet cable.  Both ends advertise gigabit, but can't
 * link.  "Ethernet@Wirespeed" reduces advertised speed until link
 * can be achieved.
 */
static u32 bcm5482_read_wirespeed(struct phy_device *phydev, u32 reg)
{
	return (phy_read(phydev, MDIO_DEVAD_NONE, reg) & 0x8FFF) | 0x8010;
}

static int bcm5482_config(struct phy_device *phydev)
{
	unsigned int reg;

	/* reset the PHY */
	reg = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMCR);
	reg |= BMCR_RESET;
	phy_write(phydev, MDIO_DEVAD_NONE, MII_BMCR, reg);

	/* Setup read from auxilary control shadow register 7 */
	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_BCM54xx_AUXCNTL,
			MIIM_BCM54xx_AUXCNTL_ENCODE(7));
	/* Read Misc Control register and or in Ethernet@Wirespeed */
	reg = bcm5482_read_wirespeed(phydev, MIIM_BCM54xx_AUXCNTL);
	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_BCM54xx_AUXCNTL, reg);

	/* Initial config/enable of secondary SerDes interface */
	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_BCM54XX_SHD,
			MIIM_BCM54XX_SHD_WR_ENCODE(0x14, 0xf));
	/* Write intial value to secondary SerDes Contol */
	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_BCM54XX_EXP_SEL,
			MIIM_BCM54XX_EXP_SEL_SSD | 0);
	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_BCM54XX_EXP_DATA,
			BMCR_ANRESTART);
	/* Enable copper/fiber auto-detect */
	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_BCM54XX_SHD,
			MIIM_BCM54XX_SHD_WR_ENCODE(0x1e, 0x201));

	genphy_config_aneg(phydev);

	return 0;
}

static int bcm5221_config(struct phy_device *phydev)
{
	int reg, err, err2, brcmtest;

	/* Reset the PHY to bring it to a known state. */
	err = phy_write(phydev, MDIO_DEVAD_NONE, MII_BMCR, BMCR_RESET);
	if (err < 0)
		return err;

	/* The datasheet indicates the PHY needs up to 1us to complete a reset,
	 * build some slack here.
	 */
	mdelay(20);

	/* The PHY requires 65 MDC clock cycles to complete a write operation
	 * and turnaround the line properly.
	 *
	 * We ignore -EIO here as the MDIO controller (e.g.: mdio-bcm-unimac)
	 * may flag the lack of turn-around as a read failure. This is
	 * particularly true with this combination since the MDIO controller
	 * only used 64 MDC cycles. This is not a critical failure in this
	 * specific case and it has no functional impact otherwise, so we let
	 * that one go through. If there is a genuine bus error, the next read
	 * of MII_BRCM_FET_INTREG will error out.
	 */
	err = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMCR);
	if (err < 0 && err != -EIO)
		return err;

	/* Enable shadow register access */
	brcmtest = phy_read(phydev, MDIO_DEVAD_NONE, MII_BRCM_FET_BRCMTEST);
	if (brcmtest < 0)
		return brcmtest;

	reg = brcmtest | MII_BRCM_FET_BT_SRE;

	err = phy_write(phydev, MDIO_DEVAD_NONE, MII_BRCM_FET_BRCMTEST, reg);
	if (err < 0)
		return err;

	/* Exit low power mode */
	reg = phy_read(phydev, MDIO_DEVAD_NONE, MII_BRCM_FET_SHDW_AUXMODE4);
	if (reg < 0)
		return reg;
	reg &= ~BCM5221_SHDW_AM4_FORCE_LPM;
	err = phy_write(phydev, MDIO_DEVAD_NONE, MII_BRCM_FET_SHDW_AUXMODE4,
			reg);
	if (err < 0)
		goto done;

done:
	/* Disable shadow register access */
	err2 = phy_write(phydev, MDIO_DEVAD_NONE, MII_BRCM_FET_BRCMTEST,
			 brcmtest);
	if (!err)
		err = err2;

	return err;
}

static void bcm_cygnus_afe(struct phy_device *phydev)
{
	/* ensures smdspclk is enabled */
	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_BCM54xx_AUXCNTL, 0x0c30);

	/* AFE_VDAC_ICTRL_0 bit 7:4 Iq=1100 for 1g 10bt, normal modes */
	bcm_phy_write_misc(phydev, 0x39, 0x01, 0xA7C8);

	/* AFE_HPF_TRIM_OTHERS bit11=1, short cascode for all modes*/
	bcm_phy_write_misc(phydev, 0x3A, 0x00, 0x0803);

	/* AFE_TX_CONFIG_1 bit 7:4 Iq=1100 for test modes */
	bcm_phy_write_misc(phydev, 0x3A, 0x01, 0xA740);

	/* AFE TEMPSEN_OTHERS rcal_HT, rcal_LT 10000 */
	bcm_phy_write_misc(phydev, 0x3A, 0x03, 0x8400);

	/* AFE_FUTURE_RSV bit 2:0 rccal <2:0>=100 */
	bcm_phy_write_misc(phydev, 0x3B, 0x00, 0x0004);

	/* Adjust bias current trim to overcome digital offSet */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1E, 0x02);

	/* make rcal=100, since rdb default is 000 */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x17, 0x00B1);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x15, 0x0010);

	/* CORE_EXPB0, Reset R_CAL/RC_CAL Engine */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x17, 0x00B0);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x15, 0x0010);

	/* CORE_EXPB0, Disable Reset R_CAL/RC_CAL Engine */
	phy_write(phydev, MDIO_DEVAD_NONE, 0x17, 0x00B0);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x15, 0x0000);
}

static int bcm_cygnus_config(struct phy_device *phydev)
{
	genphy_config_aneg(phydev);
	phy_reset(phydev);
	/* AFE settings for PHY stability */
	bcm_cygnus_afe(phydev);
	/* Forcing aneg after applying the AFE settings */
	genphy_restart_aneg(phydev);

	return 0;
}

/*
 * Find out if PHY is in copper or serdes mode by looking at Expansion Reg
 * 0x42 - "Operating Mode Status Register"
 */
static int bcm5482_is_serdes(struct phy_device *phydev)
{
	u16 val;
	int serdes = 0;

	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_BCM54XX_EXP_SEL,
			MIIM_BCM54XX_EXP_SEL_ER | 0x42);
	val = phy_read(phydev, MDIO_DEVAD_NONE, MIIM_BCM54XX_EXP_DATA);

	switch (val & 0x1f) {
	case 0x0d:	/* RGMII-to-100Base-FX */
	case 0x0e:	/* RGMII-to-SGMII */
	case 0x0f:	/* RGMII-to-SerDes */
	case 0x12:	/* SGMII-to-SerDes */
	case 0x13:	/* SGMII-to-100Base-FX */
	case 0x16:	/* SerDes-to-Serdes */
		serdes = 1;
		break;
	case 0x6:	/* RGMII-to-Copper */
	case 0x14:	/* SGMII-to-Copper */
	case 0x17:	/* SerDes-to-Copper */
		break;
	default:
		printf("ERROR, invalid PHY mode (0x%x\n)", val);
		break;
	}

	return serdes;
}

/*
 * Determine SerDes link speed and duplex from Expansion reg 0x42 "Operating
 * Mode Status Register"
 */
static u32 bcm5482_parse_serdes_sr(struct phy_device *phydev)
{
	u16 val;
	int i = 0;

	/* Wait 1s for link - Clause 37 autonegotiation happens very fast */
	while (1) {
		phy_write(phydev, MDIO_DEVAD_NONE, MIIM_BCM54XX_EXP_SEL,
				MIIM_BCM54XX_EXP_SEL_ER | 0x42);
		val = phy_read(phydev, MDIO_DEVAD_NONE, MIIM_BCM54XX_EXP_DATA);

		if (val & 0x8000)
			break;

		if (i++ > 1000) {
			phydev->link = 0;
			return 1;
		}

		udelay(1000);	/* 1 ms */
	}

	phydev->link = 1;
	switch ((val >> 13) & 0x3) {
	case (0x00):
		phydev->speed = 10;
		break;
	case (0x01):
		phydev->speed = 100;
		break;
	case (0x02):
		phydev->speed = 1000;
		break;
	}

	phydev->duplex = (val & 0x1000) == 0x1000;

	return 0;
}

/*
 * Figure out if BCM5482 is in serdes or copper mode and determine link
 * configuration accordingly
 */
static int bcm5482_startup(struct phy_device *phydev)
{
	int ret;

	if (bcm5482_is_serdes(phydev)) {
		bcm5482_parse_serdes_sr(phydev);
		phydev->port = PORT_FIBRE;
		return 0;
	}

	/* Wait for auto-negotiation to complete or fail */
	ret = genphy_update_link(phydev);
	if (ret)
		return ret;

	/* Parse BCM54xx copper aux status register */
	return bcm54xx_parse_status(phydev);
}

U_BOOT_PHY_DRIVER(bcm54616s) = {
	.name = "Broadcom BCM54616S",
	.uid = 0x03625d12,
	.mask = 0xffffffff,
	.features = PHY_GBIT_FEATURES,
	.config = &bcm5461_config,
	.startup = &bcm54xx_startup,
	.shutdown = &genphy_shutdown,
};

U_BOOT_PHY_DRIVER(bcm54612) = {
	.name = "Broadcom BCM54612",
	.uid = 0x03625e6a,
	.mask = 0xffffffff,
	.features = PHY_GBIT_FEATURES,
	.config = &bcm5461_config,
	.startup = &bcm54xx_startup,
	.shutdown = &genphy_shutdown,
};

U_BOOT_PHY_DRIVER(bcm54210e) = {
	.name = "Broadcom BCM54210E",
	.uid = 0x600d84a0,
	.mask = 0xfffffff0,
	.features = PHY_GBIT_FEATURES,
	.config = &bcm5461_config,
	.startup = &bcm54xx_startup,
	.shutdown = &genphy_shutdown,
};

U_BOOT_PHY_DRIVER(bcm5461s) = {
	.name = "Broadcom BCM5461S",
	.uid = 0x2060c0,
	.mask = 0xfffff0,
	.features = PHY_GBIT_FEATURES,
	.config = &bcm5461_config,
	.startup = &bcm54xx_startup,
	.shutdown = &genphy_shutdown,
};

U_BOOT_PHY_DRIVER(bcm5464s) = {
	.name = "Broadcom BCM5464S",
	.uid = 0x2060b0,
	.mask = 0xfffff0,
	.features = PHY_GBIT_FEATURES,
	.config = &bcm5461_config,
	.startup = &bcm54xx_startup,
	.shutdown = &genphy_shutdown,
};

U_BOOT_PHY_DRIVER(bcm5482s) = {
	.name = "Broadcom BCM5482S",
	.uid = 0x143bcb0,
	.mask = 0xffffff0,
	.features = PHY_GBIT_FEATURES,
	.config = &bcm5482_config,
	.startup = &bcm5482_startup,
	.shutdown = &genphy_shutdown,
};

U_BOOT_PHY_DRIVER(bcm_cygnus) = {
	.name = "Broadcom CYGNUS GPHY",
	.uid = 0xae025200,
	.mask = 0xfffff0,
	.features = PHY_GBIT_FEATURES,
	.config = &bcm_cygnus_config,
	.startup = &genphy_startup,
	.shutdown = &genphy_shutdown,
};

U_BOOT_PHY_DRIVER(bcm5221) = {
	.name = "Broadcom BCM5221",
	.uid = 0x004061E0,
	.mask = 0xfffff0,
	.features = PHY_BASIC_FEATURES,
	.config = &bcm5221_config,
	.startup = &genphy_startup,
	.shutdown = &genphy_shutdown,
};
