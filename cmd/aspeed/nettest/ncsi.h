/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef _NCSITEST_H_
#define _NCSITEST_H_

#define NCSI_PACKAGE_MAX 8
#define NCSI_CHANNEL_MAX 31

#define NCSI_MODE_NONE		0
#define NCSI_MODE_MULTI		1
#define NCSI_MODE_SINGLE	2

#define NCSI_PACKAGE_SHIFT      5
#define NCSI_PACKAGE_INDEX(c)   (((c) >> NCSI_PACKAGE_SHIFT) & 0x7)
#define NCSI_RESERVED_CHANNEL   0x1f
#define NCSI_CHANNEL_INDEX(c)   ((c) & ((1 << NCSI_PACKAGE_SHIFT) - 1))
#define NCSI_TO_CHANNEL(p, c)   (((p) << NCSI_PACKAGE_SHIFT) | (c))

/* NCSI packet revision */
#define NCSI_PKT_REVISION	0x01

/* NCSI packet commands */
#define NCSI_PKT_CMD_CIS	0x00 /* Clear Initial State              */
#define NCSI_PKT_CMD_SP		0x01 /* Select Package                   */
#define   SP_DISABLE_HW_ARB		0x01
#define NCSI_PKT_CMD_DP		0x02 /* Deselect Package                 */
#define NCSI_PKT_CMD_EC		0x03 /* Enable Channel                   */
#define NCSI_PKT_CMD_DC		0x04 /* Disable Channel                  */
#define NCSI_PKT_CMD_RC		0x05 /* Reset Channel                    */
#define NCSI_PKT_CMD_ECNT	0x06 /* Enable Channel Network Tx        */
#define NCSI_PKT_CMD_DCNT	0x07 /* Disable Channel Network Tx       */
#define NCSI_PKT_CMD_AE		0x08 /* AEN Enable                       */
#define NCSI_PKT_CMD_SL		0x09 /* Set Link                         */
#define NCSI_PKT_CMD_GLS	0x0a /* Get Link                         */
#define NCSI_PKT_CMD_SVF	0x0b /* Set VLAN Filter                  */
#define NCSI_PKT_CMD_EV		0x0c /* Enable VLAN                      */
#define NCSI_PKT_CMD_DV		0x0d /* Disable VLAN                     */
#define NCSI_PKT_CMD_SMA	0x0e /* Set MAC address                  */
#define NCSI_PKT_CMD_EBF	0x10 /* Enable Broadcast Filter          */
#define NCSI_PKT_CMD_DBF	0x11 /* Disable Broadcast Filter         */
#define NCSI_PKT_CMD_EGMF	0x12 /* Enable Global Multicast Filter   */
#define NCSI_PKT_CMD_DGMF	0x13 /* Disable Global Multicast Filter  */
#define NCSI_PKT_CMD_SNFC	0x14 /* Set NCSI Flow Control            */
#define NCSI_PKT_CMD_GVI	0x15 /* Get Version ID                   */
#define NCSI_PKT_CMD_GC		0x16 /* Get Capabilities                 */
#define NCSI_PKT_CMD_GP		0x17 /* Get Parameters                   */
#define NCSI_PKT_CMD_GCPS	0x18 /* Get Controller Packet Statistics */
#define NCSI_PKT_CMD_GNS	0x19 /* Get NCSI Statistics              */
#define NCSI_PKT_CMD_GNPTS	0x1a /* Get NCSI Pass-throu Statistics   */
#define NCSI_PKT_CMD_GPS	0x1b /* Get package status               */
#define NCSI_PKT_CMD_OEM	0x50 /* OEM                              */
#define NCSI_PKT_CMD_PLDM	0x51 /* PLDM request over NCSI over RBT  */
#define NCSI_PKT_CMD_GPUUID	0x52 /* Get package UUID                 */

/* NCSI packet responses */
#define NCSI_PKT_RSP_CIS	(NCSI_PKT_CMD_CIS    + 0x80)
#define NCSI_PKT_RSP_SP		(NCSI_PKT_CMD_SP     + 0x80)
#define NCSI_PKT_RSP_DP		(NCSI_PKT_CMD_DP     + 0x80)
#define NCSI_PKT_RSP_EC		(NCSI_PKT_CMD_EC     + 0x80)
#define NCSI_PKT_RSP_DC		(NCSI_PKT_CMD_DC     + 0x80)
#define NCSI_PKT_RSP_RC		(NCSI_PKT_CMD_RC     + 0x80)
#define NCSI_PKT_RSP_ECNT	(NCSI_PKT_CMD_ECNT   + 0x80)
#define NCSI_PKT_RSP_DCNT	(NCSI_PKT_CMD_DCNT   + 0x80)
#define NCSI_PKT_RSP_AE		(NCSI_PKT_CMD_AE     + 0x80)
#define NCSI_PKT_RSP_SL		(NCSI_PKT_CMD_SL     + 0x80)
#define NCSI_PKT_RSP_GLS	(NCSI_PKT_CMD_GLS    + 0x80)
#define NCSI_PKT_RSP_SVF	(NCSI_PKT_CMD_SVF    + 0x80)
#define NCSI_PKT_RSP_EV		(NCSI_PKT_CMD_EV     + 0x80)
#define NCSI_PKT_RSP_DV		(NCSI_PKT_CMD_DV     + 0x80)
#define NCSI_PKT_RSP_SMA	(NCSI_PKT_CMD_SMA    + 0x80)
#define NCSI_PKT_RSP_EBF	(NCSI_PKT_CMD_EBF    + 0x80)
#define NCSI_PKT_RSP_DBF	(NCSI_PKT_CMD_DBF    + 0x80)
#define NCSI_PKT_RSP_EGMF	(NCSI_PKT_CMD_EGMF   + 0x80)
#define NCSI_PKT_RSP_DGMF	(NCSI_PKT_CMD_DGMF   + 0x80)
#define NCSI_PKT_RSP_SNFC	(NCSI_PKT_CMD_SNFC   + 0x80)
#define NCSI_PKT_RSP_GVI	(NCSI_PKT_CMD_GVI    + 0x80)
#define NCSI_PKT_RSP_GC		(NCSI_PKT_CMD_GC     + 0x80)
#define NCSI_PKT_RSP_GP		(NCSI_PKT_CMD_GP     + 0x80)
#define NCSI_PKT_RSP_GCPS	(NCSI_PKT_CMD_GCPS   + 0x80)
#define NCSI_PKT_RSP_GNS	(NCSI_PKT_CMD_GNS    + 0x80)
#define NCSI_PKT_RSP_GNPTS	(NCSI_PKT_CMD_GNPTS  + 0x80)
#define NCSI_PKT_RSP_GPS	(NCSI_PKT_CMD_GPS    + 0x80)
#define NCSI_PKT_RSP_OEM	(NCSI_PKT_CMD_OEM    + 0x80)
#define NCSI_PKT_RSP_PLDM	(NCSI_PKT_CMD_PLDM   + 0x80)
#define NCSI_PKT_RSP_GPUUID	(NCSI_PKT_CMD_GPUUID + 0x80)

#define PCI_VID_INTEL_8086			0x8086
#define PCI_VID_INTEL_8087			0x8087
#define PCI_VID_INTEL_163C			0x163C
#define PCI_VID_BROADCOM			0x14E4
#define PCI_VID_MELLANOX			0x15B3
#define PCI_VID_EMULEX				0x10DF

int ncsitest(struct test_s *test_obj);
void ncsi_print_info(struct test_s *test_obj);

#endif /* _NCSITEST_H_ */
