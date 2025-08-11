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

#include <asm/arch/platform.h>
#include <ast_loader.h>
#include <linux/usb/ch9.h>

#define USB_VHUBA_REG	(void __iomem *)0x12011000
#define USB_VHUBB_REG	(void __iomem *)0x12021000
#define SCU0_REG	(void __iomem *)0x12c02000
#define USB_VHUBC_REG	(void __iomem *)0x14120000
#define USB_VHUBD_REG	(void __iomem *)0x14122000
#define SCU1_REG	(void __iomem *)0x14c02000

#define SCU1_HWSTRAP1				(SCU1_REG + 0x010)
#define   SCU1_HWSTRAP1_RECOVERY_USB_PORT	GENMASK(29, 28)

#define SCU1_CHIP_UNIQ_ID0			(SCU1_REG + 0x810)
#define SCU1_CHIP_UNIQ_ID1			(SCU1_REG + 0x814)
/*****************************
 *                           *
 * VHUB register definitions *
 *                           *
 *****************************/
#define	VHUB_CTRL				0x00	/* Root Function Control & Status Register */
#define	VHUB_CONF				0x04	/* Root Configuration Setting Register */
#define	VHUB_IER				0x08	/* Interrupt Ctrl Register */
#define	VHUB_ISR				0x0C	/* Interrupt Status Register */
#define	VHUB_EP_ACK_IER				0x10	/* Programmable Endpoint Pool ACK Interrupt Enable Register */
#define	VHUB_EP_NACK_IER			0x14	/* Programmable Endpoint Pool NACK Interrupt Enable Register  */
#define VHUB_EP_ACK_ISR				0x18	/* Programmable Endpoint Pool ACK Interrupt Status Register  */
#define VHUB_EP_NACK_ISR			0x1C	/* Programmable Endpoint Pool NACK Interrupt Status Register  */
#define VHUB_SW_RESET				0x20	/* Device Controller Soft Reset Enable Register */
#define VHUB_USBSTS				0x24	/* USB Status Register */
#define VHUB_EP_TOGGLE				0x28	/* Programmable Endpoint Pool Data Toggle Value Set */
#define VHUB_ISO_FAIL_ACC			0x2C	/* Isochronous Transaction Fail Accumulator */
#define VHUB_EP0_CTRL				0x30	/* Endpoint 0 Contrl/Status Register */
#define VHUB_EP0_DATA				0x34	/* Base Address of Endpoint 0 In/OUT Data Buffer Register */
#define VHUB_EP1_CTRL				0x38	/* Endpoint 1 Contrl/Status Register */
#define VHUB_EP1_STS_CHG			0x3C	/* Endpoint 1 Status Change Bitmap Data */
#define VHUB_SETUP0				0x80	/* Root Device Setup Data Buffer0 */
#define VHUB_SETUP1				0x84	/* Root Device Setup Data Buffer1 */

/* Main control reg */
#define VHUB_CTRL_PHY_CLK			BIT(31)
#define VHUB_CTRL_PHY_LOOP_TEST			BIT(25)
#define VHUB_CTRL_DN_PWN			BIT(24)
#define VHUB_CTRL_DP_PWN			BIT(23)
#define VHUB_CTRL_LONG_DESC			BIT(18)
#define VHUB_CTRL_ISO_RSP_CTRL			BIT(17)
#define VHUB_CTRL_SPLIT_IN			BIT(16)
#define VHUB_CTRL_LOOP_T_RESULT			BIT(15)
#define VHUB_CTRL_LOOP_T_STS			BIT(14)
#define VHUB_CTRL_PHY_BIST_RESULT		BIT(13)
#define VHUB_CTRL_PHY_BIST_CTRL			BIT(12)
#define VHUB_CTRL_PHY_RESET_DIS			BIT(11)
#define VHUB_CTRL_SET_TEST_MODE(x)		((x) << 8)
#define VHUB_CTRL_MANUAL_REMOTE_WAKEUP		BIT(4)
#define VHUB_CTRL_AUTO_REMOTE_WAKEUP		BIT(3)
#define VHUB_CTRL_CLK_STOP_SUSPEND		BIT(2)
#define VHUB_CTRL_FULL_SPEED_ONLY		BIT(1)
#define VHUB_CTRL_UPSTREAM_CONNECT		BIT(0)

/* IER & ISR */
#define VHUB_IRQ_DEV1_BIT			9
#define VHUB_IRQ_USB_CMD_DEADLOCK		BIT(18)
#define VHUB_IRQ_EP_POOL_NAK			BIT(17)
#define VHUB_IRQ_EP_POOL_ACK_STALL		BIT(16)
#define VHUB_IRQ_DEVICE1			BIT(VHUB_IRQ_DEV1_BIT)
#define VHUB_IRQ_BUS_RESUME			BIT(8)
#define VHUB_IRQ_BUS_SUSPEND			BIT(7)
#define VHUB_IRQ_BUS_RESET			BIT(6)
#define VHUB_IRQ_HUB_EP1_IN_DATA_ACK		BIT(5)
#define VHUB_IRQ_HUB_EP0_IN_DATA_NAK		BIT(4)
#define VHUB_IRQ_HUB_EP0_IN_ACK_STALL		BIT(3)
#define VHUB_IRQ_HUB_EP0_OUT_NAK		BIT(2)
#define VHUB_IRQ_HUB_EP0_OUT_ACK_STALL		BIT(1)
#define VHUB_IRQ_HUB_EP0_SETUP			BIT(0)
#define VHUB_IRQ_ACK_ALL			0x1ff

/* Downstream device IRQ mask. */
#define VHUB_DEV_IRQ(n)				(VHUB_IRQ_DEVICE1 << (n))

/* SW reset reg */
#define VHUB_SW_RESET_ROOT_HUB			BIT(0)

/* HUB EP0 control */
#define VHUB_EP0_CTRL_STALL			BIT(0)
#define VHUB_EP0_TX_BUFF_RDY			BIT(1)
#define VHUB_EP0_RX_BUFF_RDY			BIT(2)
#define VHUB_EP0_RX_LEN(x)			(((x) >> 16) & 0x7f)
#define VHUB_EP0_SET_TX_LEN(x)			(((x) & 0x7f) << 8)
#define VHUB_EP0_SET_HIGH_ADDR(x)		(((x) & 0x3) << 30)

/* SCU ctrl */
#define SCU0_USB_MULTI_CTRL			0x410
#define SCU1_USB_MULTI_CTRL			0x3B0
#define SCU_RST_CTRL				0x220
#define SCU0_CLK_STOP_CTRL			0x240
#define SCU1_CLK_STOP_CTRL			0x260

enum usb_status_code {
	STS_OKAY = 0,
	STS_IN_WRONG_STATE,
	STS_OUT_WRONG_STATE,
	STS_SETUP_WRONG_STATE,
	STS_CRQ_NOT_SUPPORT,
	STS_SLI_UNAVAIL,
	STS_MEMCPY_FAIL,
};

/*************************************************************************
 * USB configuration
 ************************************************************************/
#define USB_MAX_CTRL_MPS			64	/* maximum packet size (MPS) for EP0 */
#define USB_DEVICE_VID				0x2245
#define USB_DEVICE_DFU_PID			0x2700
#define USB_DFU_DETACH_TIMEOUT			1000
#define USB_DFU_MAX_XFER_SIZE			4096
#define USB_DFU_DEFAULT_POLLTIMEOUT		0

#define USB_DEVICE_MANUFACTURER			"ASPEED"
#define USB_DEVICE_PRODUCT			"AST2700"
#define USB_DEVICE_SN				"8000000080000000"
#define FIRMWARE_IMAGE_0_LABEL			"SPL DFU"

#define CPU_SRAM_BASE				0x10000000
#define CPU_SRAM_SIZE				0x20000
#define USB_DMA_BUF_ADDR			CPU_SRAM_BASE
#define USB_DMA_BUF_SIZE			CPU_SRAM_SIZE
#define DRAM_BLOCK_SIZE				USB_DFU_MAX_XFER_SIZE

/*************************************************************************
 * DFU
 ************************************************************************/

/** DFU Class Subclass */
#define DFU_SUBCLASS			0x01

/** DFU Class runtime Protocol */
#define DFU_RT_PROTOCOL			0x01

/** DFU Class DFU mode Protocol */
#define DFU_MODE_PROTOCOL		0x02

/**
 * @brief DFU Class Specific Requests
 */
#define DFU_DETACH			0x00
#define DFU_DNLOAD			0x01
#define DFU_UPLOAD			0x02
#define DFU_GETSTATUS			0x03
#define DFU_CLRSTATUS			0x04
#define DFU_GETSTATE			0x05
#define DFU_ABORT			0x06

/** DFU FUNCTIONAL descriptor type */
#define DFU_FUNC_DESC			0x21

/** DFU attributes DFU Functional Descriptor */
#define DFU_ATTR_WILL_DETACH		0x08
#define DFU_ATTR_MANIFESTATION_TOLERANT	0x04
#define DFU_ATTR_CAN_UPLOAD		0x02
#define DFU_ATTR_CAN_DNLOAD		0x01

/** DFU Specification release */
#define DFU_VERSION			0x0110

/** Run-Time Functional Descriptor */
struct dfu_runtime_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bmAttributes;
	uint16_t wDetachTimeOut;
	uint16_t wTransferSize;
	uint16_t bcdDFUVersion;
} __packed;

/** bStatus values for the DFU_GETSTATUS response */
enum dfu_status {
	statusOK,
	errTARGET,
	errFILE,
	errWRITE,
	errERASE,
	errCHECK_ERASED,
	errPROG,
	errVERIFY,
	errADDRESS,
	errNOTDONE,
	errFIRMWARE,
	errVENDOR,
	errUSB,
	errPOR,
	errUNKNOWN,
	errSTALLEDPKT
};

/** bState values for the DFU_GETSTATUS response */
enum dfu_state {
	appIDLE,
	appDETACH,
	dfuIDLE,
	dfuDNLOAD_SYNC,
	dfuDNBUSY,
	dfuDNLOAD_IDLE,
	dfuMANIFEST_SYNC,
	dfuMANIFEST,
	dfuMANIFEST_WAIT_RST,
	dfuUPLOAD_IDLE,
	dfuERROR,
};

const char *usb_strings[] = {
	"",
	USB_DEVICE_MANUFACTURER,
	USB_DEVICE_PRODUCT,
	USB_DEVICE_SN,
	FIRMWARE_IMAGE_0_LABEL,
};

enum usbd_req_rc {
	USBD_REQ_HANDLED = 0,
	USBD_REQ_NOTSUPP,
	USBD_REQ_MEMCPY_FAIL,
};

struct request_ctx {
	struct usb_ctrlrequest crq;
	uint32_t length;
	uint32_t actual;
	uint32_t block_nr;
};

/* DFU mode device descriptor */
struct dev_dfu_mode_descriptor {
	struct usb_device_descriptor device_desc;
	struct usb_config_descriptor cfg_desc;
	struct usb_dfu_config {
		/* sram */
		struct usb_interface_descriptor if0;
		struct dfu_runtime_descriptor dfu_run_desc;
	} dfu_cfg;
};

static struct dev_dfu_mode_descriptor dfu_mode_desc = {
	/* Device descriptor */
	.device_desc = {
		.bLength = sizeof(struct usb_device_descriptor),
		.bDescriptorType = USB_DT_DEVICE,
		.bcdUSB = cpu_to_le16(0x0200),
		.bDeviceClass = 0,
		.bDeviceSubClass = 0,
		.bDeviceProtocol = 0,
		.bMaxPacketSize0 = USB_MAX_CTRL_MPS,
		.idVendor = cpu_to_le16((uint16_t)USB_DEVICE_VID),
		.idProduct = cpu_to_le16((uint16_t)USB_DEVICE_DFU_PID),
		.iManufacturer = 1,
		.iProduct = 2,
		.iSerialNumber = 3,
		.bNumConfigurations = 1,
	},
	/* Configuration descriptor */
	.cfg_desc = {
		.bLength = sizeof(struct usb_config_descriptor),
		.bDescriptorType = USB_DT_CONFIG,
		.wTotalLength = sizeof(dfu_mode_desc.cfg_desc) +
				sizeof(dfu_mode_desc.dfu_cfg),
		.bNumInterfaces = 1,
		.bConfigurationValue = 1,
		.iConfiguration = 0,
		.bmAttributes = USB_CONFIG_ATT_ONE |
				USB_CONFIG_ATT_SELFPOWER,
		.bMaxPower = 0x32,
	},
	.dfu_cfg = {
		/* Interface descriptor */
		.if0 = {
			.bLength = sizeof(struct usb_interface_descriptor),
			.bDescriptorType = USB_DT_INTERFACE,
			.bInterfaceNumber = 0,
			.bAlternateSetting = 0,
			.bNumEndpoints = 0,
			.bInterfaceClass = USB_CLASS_APP_SPEC,
			.bInterfaceSubClass = DFU_SUBCLASS,
			.bInterfaceProtocol = DFU_MODE_PROTOCOL,
			.iInterface = 4,
		},
		.dfu_run_desc = {
			.bLength = sizeof(struct dfu_runtime_descriptor),
			.bDescriptorType = DFU_FUNC_DESC,
			.bmAttributes = DFU_ATTR_CAN_DNLOAD |
					DFU_ATTR_MANIFESTATION_TOLERANT,
			.wDetachTimeOut =
				cpu_to_le16(USB_DFU_DETACH_TIMEOUT),
			.wTransferSize =
				cpu_to_le16(USB_DFU_MAX_XFER_SIZE),
			.bcdDFUVersion =
				cpu_to_le16(DFU_VERSION),
		},
	},
};

#define VENDOR_REQ_MS_OS_DESC             0x12
#define WINDEX_OS_FEATURE_EXT_COMPAT_ID   4
#define WINDEX_OS_FEATURE_EXT_PROPERTIES  5

#define OS_STRING_IDX			       0xEE

struct usb_os_string {
	uint8_t	bLength;
	uint8_t	bDescriptorType;
	uint8_t	qwSignature[14];
	uint8_t	bMS_VendorCode;
	uint8_t	bPad;
} __packed;

struct usb_os_compat_id_desc {
	// Header
	uint32_t dwLength;
	uint16_t bcdVersion;
	uint16_t wIndex;
	uint8_t  bCount;
	uint8_t  bReserved1[7];
	// Function Section 1
	uint8_t  bFirstInterfaceNumber;
	uint8_t  bReserved2;
	uint8_t  bCompatibleID[8];
	uint8_t  bSubCompatibleID[8];
	uint8_t  bReserved3[6];
} __packed;

struct usb_os_ext_properties_desc {
	// Header
	uint32_t dwLength;
	uint16_t bcdVersion;
	uint16_t wIndex;
	uint16_t wCount;
	// Custom Property Section 1
	uint32_t dwSize;
	uint32_t dwPropertyDataType;
	uint16_t wPropertyNameLength;
	uint8_t  bPropertyName[40];
	uint32_t dwPropertyDataLength;
	uint8_t  bPropertyData[78];
} __packed;

struct usb_os_string const desc_string_ms_10 = {
	.bLength = sizeof(struct usb_os_string),
	.bDescriptorType = USB_DT_STRING,
	.qwSignature = {'M', 0, 'S', 0, 'F', 0, 'T', 0, '1', 0, '0', 0, '0', 0,},
	.bMS_VendorCode = VENDOR_REQ_MS_OS_DESC,
	.bPad = 0
};

struct usb_os_compat_id_desc const desc_compat_id_ms = {
	// Header
	.dwLength = cpu_to_le32(sizeof(struct usb_os_compat_id_desc)),
	.bcdVersion = cpu_to_le16(0x0100),
	.wIndex = cpu_to_le16(0x0004), // The index for Extended compat ID descriptor
	.bCount = 0x01,
	.bReserved1 = {0},
	// Function Section 1
	.bFirstInterfaceNumber = 0x00, //ITF_NUM_DFU_RT
	.bReserved2 = 1,
	.bCompatibleID = "WINUSB",
	.bSubCompatibleID = {0},
	.bReserved3 = {0}
};

struct usb_os_ext_properties_desc const desc_ext_properties_ms = {
	// Header
	.dwLength = cpu_to_le32(sizeof(struct usb_os_ext_properties_desc)),
	.bcdVersion = cpu_to_le16(0x0100),
	.wIndex = cpu_to_le16(0x0005), // The index for extended property OS descriptors.
	.wCount = cpu_to_le16(0x0001), // Only 1 Custom Property Section
	// Custom Property Section 1
	.dwSize = cpu_to_le32(0x00000084),
	.dwPropertyDataType = cpu_to_le32(0x00000001), //Property Data Type: A NULL-terminated Unicode String (REG_SZ)
	.wPropertyNameLength = cpu_to_le16(0x0028),
	.bPropertyName = {'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00, 'e', 0x00, 'I', 0x00,
					'n', 0x00, 't', 0x00, 'e', 0x00, 'r', 0x00, 'f', 0x00, 'a', 0x00, 'c', 0x00,
					'e', 0x00, 'G', 0x00, 'U', 0x00, 'I', 0x00, 'D', 0x00, 0x00, 0x00},
	.dwPropertyDataLength = cpu_to_le32(0x0000004E),
	// {AA536045-0A1E-4782-A559-2E1ACA555AAE}
	.bPropertyData = {'{', 0x00, 'A', 0x00, 'A', 0x00, '5', 0x00, '3', 0x00, '6', 0x00, '0', 0x00,
					'4', 0x00, '5', 0x00, '-', 0x00, '0', 0x00, 'A', 0x00, '1', 0x00, 'E', 0x00,
					'-', 0x00, '4', 0x00, '7', 0x00, '8', 0x00, '2', 0x00, '-', 0x00, 'A', 0x00,
					'5', 0x00, '5', 0x00, '9', 0x00, '-', 0x00, '2', 0x00, 'E', 0x00, '1', 0x00,
					'A', 0x00, 'C', 0x00, 'A', 0x00, '5', 0x00, '5', 0x00, '5', 0x00, 'A', 0x00,
					'A', 0x00, 'E', 0x00, '}', 0x00, 0x00, 0x00}
};

/*
 * The USB Unicode bString is encoded in UTF16LE, which means it takes up
 * twice the amount of bytes than the same string encoded in ASCII7.
 * Use this macro to determine the length of the bString array.
 *
 * bString length without null character:
 *   bString_length = (sizeof(initializer_string) - 1) * 2
 * or:
 *   bString_length = sizeof(initializer_string) * 2 - 2
 */
#define USB_BSTRING_LENGTH(s)		(sizeof(s) * 2 - 2)

/*
 * The length of the string descriptor (bLength) is calculated from the
 * size of the two octets bLength and bDescriptorType plus the
 * length of the UTF16LE string:
 *
 *   bLength = 2 + bString_length
 *   bLength = 2 + sizeof(initializer_string) * 2 - 2
 *   bLength = sizeof(initializer_string) * 2
 * Use this macro to determine the bLength of the string descriptor.
 */
#define USB_STRING_DESCRIPTOR_LENGTH(s)	(sizeof(s) * 2)

struct usb_string_desription {
	struct usb_string_descriptor lang_desc;
	struct usb_mfr_descriptor {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint16_t bString[USB_BSTRING_LENGTH(USB_DEVICE_MANUFACTURER) / 2];
	} utf16le_mfr;

	struct usb_product_descriptor {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint16_t bString[USB_BSTRING_LENGTH(USB_DEVICE_PRODUCT) / 2];
	} utf16le_product;

	struct usb_sn_descriptor {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint16_t bString[USB_BSTRING_LENGTH(USB_DEVICE_SN) / 2];
	} utf16le_sn;

	struct image_0_descriptor {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint16_t bString[USB_BSTRING_LENGTH(FIRMWARE_IMAGE_0_LABEL) / 2];
	} utf16le_image0;
};

static struct usb_string_desription string_desc = {
	.lang_desc = {
		.bLength = sizeof(struct usb_string_descriptor),
		.bDescriptorType = USB_DT_STRING,
		.wData[0] = cpu_to_le16(0x0409),
	},
	/* Manufacturer String Descriptor */
	.utf16le_mfr = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(USB_DEVICE_MANUFACTURER),
		.bDescriptorType = USB_DT_STRING,
	},
	/* Product String Descriptor */
	.utf16le_product = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(USB_DEVICE_PRODUCT),
		.bDescriptorType = USB_DT_STRING,
	},
	/* Serial Number String Descriptor */
	.utf16le_sn = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(USB_DEVICE_SN),
		.bDescriptorType = USB_DT_STRING,
	},
	/* Image 0 String Descriptor */
	.utf16le_image0 = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(FIRMWARE_IMAGE_0_LABEL),
		.bDescriptorType = USB_DT_STRING,
	},
};

/* Device data structure */
struct dfu_data_t {
	uint32_t alt_setting;              /* DFU alternate setting */
	enum dfu_state state;              /* State of the DFU device */
	enum dfu_status status;            /* Status of the DFU device */
	uint16_t block_nr;                 /* DFU block number */
	uint16_t bwPollTimeout;
};

enum usb_state {
	IDLE,
	STALL,
	DATA_IN,
	LAST_DATA_IN,
	STATUS_IN,
	DATA_OUT,
	LAST_DATA_OUT,
	STATUS_OUT,
};

enum usb_port {
	PORT_A,
	PORT_B,
	PORT_C,
	PORT_D,
	PORT_NUM,
};

#define OTP_USB_UART_NUM	13 //OTP configurable USB2UART number (No UART13/14)
#define USB_UART_NUM		15 //Total USB2UART number

/* OTP defined UART Mode vlaues */
#define USB_UART_MODE_0		0 //UART and Device both not connected
#define USB_UART_MODE_1		1 //UART Real mode, and Device connected
#define USB_UART_MODE_2		2 //UART Pseduo mode, and Device connected
#define USB_UART_MODE_3		3 //UART not connected, but Device connected

/* USB2COM Mode Selection for Register 0x14120810 */
#define UART_NOT_CONNCET	(0)
#define UART_REAL_CONNCET	BIT(0)
#define UART_PSEUDO_CONNCET	BIT(1)

struct usb_uart_cfg {
	struct {
		uint8_t	mode;
		uint8_t	tx_discard_off;
	} usb_uart[USB_UART_NUM];
	uint8_t hub_23ports;
	uint8_t mode3;
	uint8_t put_all_msg;
} _packed;

struct usb_vhub_config {
	void *base;
	void *scu_multi_func;
	void *scu_reset;
	void *scu_clock_stop;
	uint32_t func_mask;
	uint32_t func_bits;
	uint32_t reset_bits;
	uint32_t clock_bits;
};

static struct usb_vhub_config usb_cfg[PORT_NUM] = {
	{
		USB_VHUBA_REG,
		(SCU0_REG + SCU0_USB_MULTI_CTRL),
		(SCU0_REG + SCU_RST_CTRL),
		(SCU0_REG + SCU0_CLK_STOP_CTRL),
		(GENMASK(25, 24) | BIT(18) | GENMASK(3, 2)),
		BIT(2), //vHubA1
		BIT(0),
		BIT(14),
	},
	{
		USB_VHUBB_REG,
		(SCU0_REG + SCU0_USB_MULTI_CTRL),
		(SCU0_REG + SCU_RST_CTRL),
		(SCU0_REG + SCU0_CLK_STOP_CTRL),
		(GENMASK(29, 28) | BIT(18) | GENMASK(7, 6)),
		(BIT(18) | BIT(6)), //vHubB1 and PortB access SRAM
		BIT(3),
		BIT(7),
	},
	{
		USB_VHUBC_REG,
		(SCU1_REG + SCU1_USB_MULTI_CTRL),
		(SCU1_REG + SCU_RST_CTRL),
		(SCU1_REG + SCU1_CLK_STOP_CTRL),
		(GENMASK(1, 0)),
		BIT(0), //vHubC
		BIT(27),
		BIT(17),
	},
	{
		USB_VHUBD_REG,
		(SCU1_REG + SCU1_USB_MULTI_CTRL),
		(SCU1_REG + SCU_RST_CTRL),
		(SCU1_REG + SCU1_CLK_STOP_CTRL),
		(GENMASK(3, 2)),
		BIT(2), //vHubD
		BIT(29),
		BIT(18),
	},
};

struct bootusb_priv {
	bool is_dnload_done;
	u32 *dfu_dst_addr;
	u32 dfu_max_len;
	u32 dfu_recv_len;
	uint8_t *ep0_ctrl_buf;
	enum usb_state usb_fsm_state;
	struct request_ctx usb_req_ctx;
	struct dfu_data_t dfu_data;
	bool usb_uart_enabled;
	enum usb_port usb_vhub_port;
};

static void usb_serial_number_desc(uint16_t bString[], int size)
{
	static const char hexmap[] = "0123456789ABCDEF";
	uint8_t byte;
	uint32_t id[2] = {
		readl(SCU1_CHIP_UNIQ_ID0),
		readl(SCU1_CHIP_UNIQ_ID1)
	};

	for (int i = 0; i < 8; i++) {
		byte = (id[1 - (i / 4)] >> ((3 - (i % 4)) * 8)) & 0xFF;
		if ((i * 2 + 1) < size) {
			bString[i * 2] = hexmap[(byte >> 4) & 0xF];
			bString[i * 2 + 1] = hexmap[byte & 0xF];
		}
	}
}

static int safe_memcpy(void *dest, size_t dest_size, const void *src, size_t num_bytes)
{
	/* Check if buffer is not NULL */
	if (!dest || !src)
		return -1;

	/* Check if the destination buffer is large enough */
	if (num_bytes > dest_size)
		num_bytes = dest_size;

	memcpy(dest, src, num_bytes);
	return 0;
}

static void vhub_ep0_tx(struct bootusb_priv *hci, u64 addr, int size)
{
	struct usb_vhub_config *usb = &usb_cfg[hci->usb_vhub_port];
	uint8_t high_addr = addr >> 32;

	/* low addr */
	writel(addr, usb->base + VHUB_EP0_DATA);

	/* high addr/tx len/tx ready */
	writel(VHUB_EP0_SET_HIGH_ADDR(high_addr) |
	       VHUB_EP0_SET_TX_LEN(size) |
	       VHUB_EP0_TX_BUFF_RDY,
	       usb->base + VHUB_EP0_CTRL);
}

static void vhub_ep0_rx(struct bootusb_priv *hci, u64 addr)
{
	struct usb_vhub_config *usb = &usb_cfg[hci->usb_vhub_port];
	uint8_t high_addr = addr >> 32;

	/* low addr */
	writel(addr, usb->base + VHUB_EP0_DATA);

	/* high addr/rx ready */
	writel(VHUB_EP0_SET_HIGH_ADDR(high_addr) |
	       VHUB_EP0_RX_BUFF_RDY,
	       usb->base + VHUB_EP0_CTRL);
}

static void vhub_req_cleanup(struct bootusb_priv *hci)
{
	hci->usb_req_ctx.length = 0;
	hci->usb_req_ctx.actual = 0;
}

static uint8_t vhub_ep0_in(struct bootusb_priv *hci)
{
	uint32_t mps, data_size_max;
	uint32_t chunk;
	u32 offset;
	u64 tx_buff_addr;

	switch (hci->usb_fsm_state) {
	case DATA_IN:
		mps = dfu_mode_desc.device_desc.bMaxPacketSize0;
		chunk = hci->usb_req_ctx.length - hci->usb_req_ctx.actual;
		if (chunk > mps)
			chunk = mps;
		tx_buff_addr = (uintptr_t)hci->ep0_ctrl_buf + hci->usb_req_ctx.actual;
		vhub_ep0_tx(hci, tx_buff_addr, chunk);
		hci->usb_req_ctx.actual += chunk;

		data_size_max = le16_to_cpu(hci->usb_req_ctx.crq.wLength);

		/* Go to status stage if payload size is less than MPS or has transferred exactly
		 * the amount of data specified during the Setup stage (data_size_max).
		 * Otherwise, go to DATA stage again for more data or a ZLP (short transfer).
		 */
		if (chunk < mps || hci->usb_req_ctx.actual == data_size_max)
			hci->usb_fsm_state = LAST_DATA_IN;
		else
			hci->usb_fsm_state = DATA_IN;
		break;
	case LAST_DATA_IN:
		vhub_ep0_rx(hci, 0);
		hci->usb_fsm_state = STATUS_OUT;
		break;
	case STATUS_IN:
		hci->usb_fsm_state = IDLE;
		if (hci->dfu_data.state == dfuDNLOAD_IDLE) {
			offset = hci->usb_req_ctx.block_nr * DRAM_BLOCK_SIZE;
			if (hci->dfu_max_len > offset) {
				if (safe_memcpy((uint8_t *)hci->dfu_dst_addr + offset,
						hci->dfu_max_len - offset,
						hci->ep0_ctrl_buf,
						hci->usb_req_ctx.length))
					return (hci->usb_fsm_state << 4 | STS_MEMCPY_FAIL);

				hci->dfu_recv_len = offset + hci->usb_req_ctx.length;
			} else {
				/* This address is out of max. length of FW size, so do not copy to SRAM.
				 * Also repot DFU bStatus Error-8 for DFU_GETSTATUS Request
				 */

				hci->dfu_data.state = dfuERROR;
				hci->dfu_data.status = errADDRESS;
			}

			vhub_req_cleanup(hci);
		}
		break;
	default:
		return (hci->usb_fsm_state << 4 | STS_IN_WRONG_STATE);
	}

	return 0;
}

static uint8_t vhub_ep0_out(struct bootusb_priv *hci)
{
	struct usb_vhub_config *usb = &usb_cfg[hci->usb_vhub_port];
	uint32_t val;

	switch (hci->usb_fsm_state) {
	case DATA_OUT:
		val = readl(usb->base + VHUB_EP0_CTRL);
		hci->usb_req_ctx.actual += VHUB_EP0_RX_LEN(val);

		if (hci->usb_req_ctx.length == hci->usb_req_ctx.actual) {
			hci->dfu_data.state = dfuDNLOAD_IDLE;
			vhub_ep0_tx(hci, 0, 0);
			hci->usb_fsm_state = STATUS_IN;

		} else {
			vhub_ep0_rx(hci, (uintptr_t)hci->ep0_ctrl_buf + hci->usb_req_ctx.actual);
		}
		break;
	case STATUS_OUT:
		hci->usb_fsm_state = IDLE;
		/* Download complete */
		if (hci->dfu_data.state == dfuIDLE && hci->usb_req_ctx.block_nr != -1)
			hci->is_dnload_done = true;

		/* DFU Error reported and download complete */
		if (hci->dfu_data.state == dfuERROR)
			hci->is_dnload_done = true;

		break;
	default:
		return (hci->usb_fsm_state << 4 | STS_OUT_WRONG_STATE);
	}

	return 0;
}

static int vhub_std_request(struct bootusb_priv *hci, struct usb_ctrlrequest *crq)
{
	struct usb_vhub_config *usb = &usb_cfg[hci->usb_vhub_port];
	uint16_t wValue = le16_to_cpu(crq->wValue);
	int desc_idx, desc_type;
	int len;

	desc_idx = wValue & 0xff;
	desc_type = wValue >> 8;

	switch (crq->bRequest) {
	case USB_REQ_SET_ADDRESS:
		writel(wValue, usb->base + VHUB_CONF);
		vhub_ep0_tx(hci, 0, 0);
		return USBD_REQ_HANDLED;

	case USB_REQ_GET_DESCRIPTOR:
		hci->is_dnload_done = false;
		hci->usb_req_ctx.block_nr = -1;
		switch (wValue >> 8) {
		case USB_DT_DEVICE:
			/* copy device descriptor */
			if (safe_memcpy(hci->ep0_ctrl_buf, USB_DMA_BUF_SIZE,
					&dfu_mode_desc.device_desc,
					sizeof(dfu_mode_desc.device_desc)))
				return USBD_REQ_MEMCPY_FAIL;
			hci->usb_req_ctx.length = sizeof(dfu_mode_desc.device_desc);
			return USBD_REQ_HANDLED;

		case USB_DT_CONFIG:
			/* copy config descriptor */
			if (safe_memcpy(hci->ep0_ctrl_buf, USB_DMA_BUF_SIZE,
					&dfu_mode_desc.cfg_desc,
					sizeof(dfu_mode_desc.cfg_desc) +
					sizeof(dfu_mode_desc.dfu_cfg)))
				return USBD_REQ_MEMCPY_FAIL;
			hci->usb_req_ctx.length = sizeof(dfu_mode_desc.cfg_desc) +
				sizeof(dfu_mode_desc.dfu_cfg);
			return USBD_REQ_HANDLED;

		case USB_DT_STRING:
			/* copy string descriptor */
			len = strlen(usb_strings[desc_idx]);
			switch (desc_idx) {
			case 0x0:
				/* Send Language ID descriptor */
				if (safe_memcpy(hci->ep0_ctrl_buf, USB_DMA_BUF_SIZE,
						&string_desc.lang_desc,
						sizeof(string_desc.lang_desc)))
					return USBD_REQ_MEMCPY_FAIL;
				hci->usb_req_ctx.length = sizeof(string_desc.lang_desc);
				break;
			case 0x1:
				for (int i = 0; i < len; i++)
					string_desc.utf16le_mfr.bString[i] = usb_strings[desc_idx][i];
				/* Send manufacturer descriptor */
				if (safe_memcpy(hci->ep0_ctrl_buf, USB_DMA_BUF_SIZE,
						&string_desc.utf16le_mfr,
						sizeof(string_desc.utf16le_mfr)))
					return USBD_REQ_MEMCPY_FAIL;
				hci->usb_req_ctx.length = sizeof(string_desc.utf16le_mfr);
				break;
			case 0x2:
				for (int i = 0; i < len; i++)
					string_desc.utf16le_product.bString[i] = usb_strings[desc_idx][i];
				/* Send product descriptor */
				if (safe_memcpy(hci->ep0_ctrl_buf, USB_DMA_BUF_SIZE,
						&string_desc.utf16le_product,
						sizeof(string_desc.utf16le_product)))
					return USBD_REQ_MEMCPY_FAIL;
				hci->usb_req_ctx.length = sizeof(string_desc.utf16le_product);
				break;
			case 0x3:
				usb_serial_number_desc(string_desc.utf16le_sn.bString,
						       ARRAY_SIZE(string_desc.utf16le_sn.bString));
				/* Send serial number descriptor */
				if (safe_memcpy(hci->ep0_ctrl_buf, USB_DMA_BUF_SIZE,
						&string_desc.utf16le_sn,
						sizeof(string_desc.utf16le_sn)))
					return USBD_REQ_MEMCPY_FAIL;
				hci->usb_req_ctx.length = sizeof(string_desc.utf16le_sn);
				break;
			case 0x4:
				for (int i = 0; i < len; i++)
					string_desc.utf16le_image0.bString[i] = usb_strings[desc_idx][i];
				/* Send if0 string descriptor */
				if (safe_memcpy(hci->ep0_ctrl_buf, USB_DMA_BUF_SIZE,
						&string_desc.utf16le_image0,
						sizeof(string_desc.utf16le_image0)))
					return USBD_REQ_MEMCPY_FAIL;
				hci->usb_req_ctx.length = sizeof(string_desc.utf16le_image0);
				break;
			case OS_STRING_IDX:
				/* Send Microsoft OS String Descriptor */
				if (safe_memcpy(hci->ep0_ctrl_buf, USB_DMA_BUF_SIZE,
						&desc_string_ms_10,
						sizeof(desc_string_ms_10)))
					return USBD_REQ_MEMCPY_FAIL;
				hci->usb_req_ctx.length = sizeof(desc_string_ms_10);
				break;
			}
			return USBD_REQ_HANDLED;
		}
		break;
	case USB_REQ_GET_CONFIGURATION:
		hci->ep0_ctrl_buf[0] = 1;
		hci->usb_req_ctx.length = 1;
		return USBD_REQ_HANDLED;
	case USB_REQ_SET_CONFIGURATION:
		return USBD_REQ_HANDLED;
	case USB_REQ_GET_STATUS:
		if ((crq->bRequestType & USB_RECIP_MASK) == USB_RECIP_DEVICE) {
			uint8_t status[2];

			status[0] = 1 << USB_DEVICE_SELF_POWERED;
			status[0] |= 0 << USB_DEVICE_REMOTE_WAKEUP;
			if (safe_memcpy(hci->ep0_ctrl_buf, USB_DMA_BUF_SIZE, status, sizeof(status)))
				return USBD_REQ_MEMCPY_FAIL;
			hci->usb_req_ctx.length = sizeof(status);
			return USBD_REQ_HANDLED;
		}
		break;
	case USB_REQ_SET_INTERFACE:
		return USBD_REQ_HANDLED;
	default:
		break;
	}

	return USBD_REQ_NOTSUPP;
}

static int vhub_class_request(struct bootusb_priv *hci, struct usb_ctrlrequest *crq)
{
	uint8_t *data = hci->ep0_ctrl_buf;

	switch (crq->bRequest) {
	case DFU_DNLOAD:
		hci->usb_req_ctx.block_nr = crq->wValue;

		switch (hci->dfu_data.state) {
		case dfuIDLE:
			hci->dfu_data.state = dfuDNBUSY;
			hci->usb_req_ctx.length = crq->wLength;
			break;
		case dfuDNLOAD_IDLE:
			if (crq->wLength == 0) {
				/* download complete */
				hci->dfu_data.state = dfuMANIFEST_SYNC;
				break;
			}
			hci->dfu_data.state = dfuDNBUSY;
			hci->usb_req_ctx.length = crq->wLength;
			break;
		default:
			hci->dfu_data.state = dfuERROR;
			hci->dfu_data.status = errUNKNOWN;
			return -1;
		}
		return USBD_REQ_HANDLED;
	case DFU_CLRSTATUS:
		return USBD_REQ_HANDLED;
	case DFU_ABORT:
		return USBD_REQ_HANDLED;
	case DFU_DETACH:
		return USBD_REQ_HANDLED;
	case DFU_GETSTATUS:
		if (hci->dfu_data.state == dfuMANIFEST_SYNC)
			hci->dfu_data.state = dfuIDLE;

		/* bStatus */
		data[0] = hci->dfu_data.status;
		/* bwPollTimeout */
		data[1] = hci->dfu_data.bwPollTimeout;
		data[3] = 0U;
		data[2] = 0U;
		/* bState */
		data[4] = hci->dfu_data.state;
		/* iString */
		data[5] = 0U;
		hci->usb_req_ctx.length = 6;

		return USBD_REQ_HANDLED;
	case DFU_GETSTATE:
		return USBD_REQ_HANDLED;
	}

	return USBD_REQ_NOTSUPP;
}

static int vhub_vendor_request(struct bootusb_priv *hci, struct usb_ctrlrequest *crq)
{
	switch (crq->bRequest) {
	case VENDOR_REQ_MS_OS_DESC:
		if (crq->wIndex == WINDEX_OS_FEATURE_EXT_COMPAT_ID) {
			if (safe_memcpy(hci->ep0_ctrl_buf, USB_DMA_BUF_SIZE,
					&desc_compat_id_ms,
					sizeof(desc_compat_id_ms)))
				return USBD_REQ_MEMCPY_FAIL;
			hci->usb_req_ctx.length = sizeof(desc_compat_id_ms);
			return USBD_REQ_HANDLED;
		} else if (crq->wIndex == WINDEX_OS_FEATURE_EXT_PROPERTIES) {
			if (safe_memcpy(hci->ep0_ctrl_buf, USB_DMA_BUF_SIZE,
					&desc_ext_properties_ms,
					sizeof(desc_ext_properties_ms)))
				return USBD_REQ_MEMCPY_FAIL;
			hci->usb_req_ctx.length = sizeof(desc_ext_properties_ms);
			return USBD_REQ_HANDLED;
		}
	}
	return USBD_REQ_NOTSUPP;
}

static uint8_t vhub_ep0_setup(struct bootusb_priv *hci)
{
	struct usb_vhub_config *usb = &usb_cfg[hci->usb_vhub_port];
	struct usb_ctrlrequest *crq = &hci->usb_req_ctx.crq;
	uint32_t data_size_max, mps;
	uint32_t len, act, chunk;
	bool diection_in;
	u64 tx_buff_addr;
	int ret;

	vhub_req_cleanup(hci);

	/*
	 * The usb state shouldn't be DATA stage when setup token comes.
	 * If really happens, set dfu state to error for DFU_GETSTATUS.
	 * Return error because DL FW may be corrupted. Cannot continue.
	 */
	if (hci->usb_fsm_state == DATA_IN || hci->usb_fsm_state == LAST_DATA_IN ||
	    hci->usb_fsm_state == DATA_OUT || hci->usb_fsm_state == LAST_DATA_OUT) {
		hci->dfu_data.state = dfuERROR;
		hci->dfu_data.status = errUNKNOWN;
		return  (hci->usb_fsm_state << 4 | STS_SETUP_WRONG_STATE);
	}

	if (safe_memcpy(crq, sizeof(struct usb_ctrlrequest),
			(void *)(usb->base + VHUB_SETUP0),
			sizeof(struct usb_ctrlrequest)))
		return STS_MEMCPY_FAIL;

	if ((crq->bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD)
		ret = vhub_std_request(hci, crq);
	else if ((crq->bRequestType & USB_TYPE_MASK) == USB_TYPE_CLASS)
		ret = vhub_class_request(hci, crq);
	else if ((crq->bRequestType & USB_TYPE_MASK) == USB_TYPE_VENDOR)
		ret = vhub_vendor_request(hci, crq);
	else
		ret = USBD_REQ_NOTSUPP;

	if (ret == USBD_REQ_MEMCPY_FAIL)
		return STS_MEMCPY_FAIL;
	else if (ret)
		return STS_CRQ_NOT_SUPPORT;

	data_size_max = le16_to_cpu(crq->wLength);

	if (data_size_max) {
		/* data stage */
		diection_in = !!(crq->bRequestType & USB_DIR_IN);
		mps = dfu_mode_desc.device_desc.bMaxPacketSize0;
		len = hci->usb_req_ctx.length;
		act = hci->usb_req_ctx.actual;
		chunk = len - act;

		if (data_size_max < chunk) {
			chunk = data_size_max;
			if (diection_in) {
				/* Data more than host expected, only return wLength (setup) size of data */
				hci->usb_req_ctx.length = data_size_max;
			}
		}
		if (!diection_in) {
			/* OUT transmission */
			vhub_ep0_rx(hci, (uintptr_t)hci->ep0_ctrl_buf);
			hci->usb_fsm_state = DATA_OUT;
		} else {
			if (mps < chunk) {
				/* Normal IN transmission */
				chunk = mps;
				tx_buff_addr = (uintptr_t)hci->ep0_ctrl_buf + act;
				vhub_ep0_tx(hci, tx_buff_addr, chunk);
				hci->usb_req_ctx.actual += chunk;
				hci->usb_fsm_state = DATA_IN;
			} else {
				/* End of IN transmission */
				tx_buff_addr = (uintptr_t)hci->ep0_ctrl_buf + act;
				vhub_ep0_tx(hci, tx_buff_addr, chunk);
				hci->usb_req_ctx.actual += chunk;

				/* Go to status stage if payload size is less than MPS or has transferred exactly
				 * the amount of data specified during the Setup stage (data_size_max).
				 * Otherwise, go to DATA stage again for more data or a ZLP (short transfer).
				 */
				if (chunk < mps || hci->usb_req_ctx.actual == data_size_max)
					hci->usb_fsm_state = LAST_DATA_IN;
				else
					hci->usb_fsm_state = DATA_IN;
			}
		}
	} else {
		/* status stage: send 0 packet */
		vhub_ep0_tx(hci, 0, 0);
		hci->usb_fsm_state = STATUS_IN;
	}

	return STS_OKAY;
}

uint8_t usb_poll(struct bootusb_priv *hci)
{
	struct usb_vhub_config *usb = &usb_cfg[hci->usb_vhub_port];
	uint32_t istat;
	uint8_t ret;

	istat = readl(usb->base + VHUB_ISR);
	if (!istat)
		return 0;
	/* Ack interrupts */
#if 1
	if (istat & 0x1ff)
		writel(istat & 0x1ff, usb->base + VHUB_ISR);
#else
	writel(istat, usb->base + VHUB_ISR);
#endif
	if (istat & VHUB_IRQ_HUB_EP0_IN_ACK_STALL) {
		ret = vhub_ep0_in(hci);
		if (ret)
			return ret;
	}

	if (istat & VHUB_IRQ_HUB_EP0_OUT_ACK_STALL) {
		ret = vhub_ep0_out(hci);
		if (ret)
			return ret;
	}

	if (istat & VHUB_IRQ_HUB_EP0_SETUP) {
		ret = vhub_ep0_setup(hci);
		if (ret)
			return ret;
	}

	return 0;
}

void usb_pinctrl(struct bootusb_priv *hci)
{
	struct usb_vhub_config *usb = &usb_cfg[hci->usb_vhub_port];
	uint32_t val;

	val = readl(usb->scu_multi_func);

	val = val & ~(usb->func_mask);

	if (hci->usb_vhub_port == PORT_C && hci->usb_uart_enabled == true) {
		/* Switch PortC from Mode-1 (vHUB only) to Mode-0 (UART + vHUB) */
		usb->func_bits &= ~GENMASK(1, 0);
	}

	writel(val | usb->func_bits, usb->scu_multi_func);
}

void usb_clk_enable_reset(enum usb_port port)
{
	struct usb_vhub_config *usb = &usb_cfg[port];

	/* Enable reset */
	writel(usb->reset_bits, usb->scu_reset);

	/* Enable (clear stop) clock */
	writel(usb->clock_bits, usb->scu_clock_stop + 0x04);

	/* Wait PLL locking */
	mdelay(10);

	/* Disable reset */
	writel(usb->reset_bits, usb->scu_reset + 0x4);
}

static int usb_init(struct udevice *dev)
{
	struct bootusb_priv *hci = dev_get_priv(dev);
	struct usb_vhub_config *usb;
	uint32_t val, reg;

	hci->ep0_ctrl_buf = (uint8_t *)USB_DMA_BUF_ADDR + CPU_SRAM_SIZE - USB_DFU_MAX_XFER_SIZE;
	hci->usb_fsm_state = IDLE;
	hci->dfu_max_len = U32_MAX;
	hci->dfu_data.state = dfuIDLE;
	hci->dfu_data.status = statusOK;
	hci->dfu_data.bwPollTimeout = USB_DFU_DEFAULT_POLLTIMEOUT;
	hci->usb_uart_enabled = false;
	hci->is_dnload_done = false;

	reg = readl(SCU1_HWSTRAP1);
	hci->usb_vhub_port = FIELD_GET(SCU1_HWSTRAP1_RECOVERY_USB_PORT, reg);

	/* Select the usb configuration */
	usb = &usb_cfg[hci->usb_vhub_port];

	/* Configure PinCtrl for USB vhub function */
	usb_pinctrl(hci);

	/* vHUB controller clock enable and reset */
	usb_clk_enable_reset(hci->usb_vhub_port);

	udelay(1);

	/* Enable SRAM access */
	val = readl(usb->base + 0x800);
	if (hci->usb_vhub_port == PORT_A || hci->usb_vhub_port == PORT_B)
		/* vHUBA & vHUBB. CPU Die: BIT4 for SRAM access */
		writel(val | BIT(4), usb->base + 0x800);
	else if (hci->usb_vhub_port == PORT_C || hci->usb_vhub_port == PORT_D)
		/* vHUBC & vHUBD. I/O Die: BIT10 for SRAM access, BIT5 for AHBM Addr 34 */
		writel(val | BIT(10) | BIT(5), usb->base + 0x800);

	/* Disable PHY reset */
	val = VHUB_CTRL_PHY_CLK |
	      VHUB_CTRL_PHY_RESET_DIS;
	writel(val, usb->base + VHUB_CTRL);

	/* SW reset device controller */
	writel(VHUB_SW_RESET_ROOT_HUB, usb->base + VHUB_SW_RESET);
	udelay(1);
	writel(0, usb->base + VHUB_SW_RESET);

	/* Enable upstream port connection */
	val |= VHUB_CTRL_UPSTREAM_CONNECT;
	writel(val, usb->base + VHUB_CTRL);

	return 0;
}

static int usb_load(struct udevice *dev, u32 *dst, u32 *len)
{
	struct bootusb_priv *hci = dev_get_priv(dev);
	int ret;

	/* Reset 'is_dnload_done flag' and all state/status for the next DL */
	hci->is_dnload_done = false;
	hci->usb_fsm_state = IDLE;
	hci->dfu_data.state = dfuIDLE;
	hci->dfu_data.status = statusOK;

	/* Read usb_vhub_port again. SPL will clear it */
	hci->usb_vhub_port = FIELD_GET(SCU1_HWSTRAP1_RECOVERY_USB_PORT, readl(SCU1_HWSTRAP1));

	/* Save this time DFU destination and max. length */
	hci->dfu_dst_addr = dst;

	//printf(" usb_load /start [port = %d dst_addr = %x max_len = %x]\n", hci->usb_vhub_port, (u32) hci->dfu_dst_addr, hci->dfu_max_len);
	while (1) {
		ret = usb_poll(hci);

		if (ret)
			break;

		if (hci->is_dnload_done)
			break;
	}
	*len = hci->dfu_recv_len;
	//printf("usb_load /end [len = %d, ret =%d]\n", *len, ret);
	return ret;
}

static const struct udevice_id bootusb_ids[] = {
	{ .compatible = "aspeed,bootusb" },
	{ }
};

static struct ast_loader_ops bootusb_ops = {
	.init = usb_init,
	.load = usb_load,
};

U_BOOT_DRIVER(bootusb) = {
	.name		= "bootusb",
	.id		= UCLASS_MISC,
	.of_match	= bootusb_ids,
	.priv_auto	= sizeof(struct bootusb_priv),
	.ops		= &bootusb_ops,
};
