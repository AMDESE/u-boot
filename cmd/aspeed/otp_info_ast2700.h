/* SPDX-License-Identifier: GPL-2.0 */

#define OTP_INFO_VER		"1.0.0"

#define OTP_REG_RESERVED        -1

struct otpstrap_info {
	signed char bit_offset;
	signed char length;
	signed char value;
	const char *information;
};

struct otp_f_strap_info {
	signed char bit_offset;
	signed char length;
	signed char value;
	const char *information;
};

struct otpconf_info {
	signed char w_offset;
	signed char bit_offset;
	signed char length;
	signed char value;
	const char *information;
};

struct scu_info {
	signed char bit_offset;
	signed char length;
	const char *information;
};

static const struct otpstrap_info a0_strap_info[] = {
	{ 1, 1, 0, "Boot from IO die SPI" },
	{ 1, 1, 1, "Boot from CPU die SPI" },
	{ 2, 2, 0, "HPLL default frequency: 2.0GHz" },
	{ 2, 2, 1, "HPLL default frequency: 1.9GHz" },
	{ 2, 2, 2, "HPLL default frequency: 1.8GHz" },
	{ 2, 2, 3, "HPLL default frequency: 1.7GHz" },
	{ 4, 1, 0, "CPU clock PLL: MPLL" },
	{ 4, 1, 1, "CPU clock PLL: HPLL" },
	{ 5, 2, 0, "AXI/AHB clock selection: 400/500MHz" },
	{ 5, 2, 1, "AXI/AHB clock selection: 400/500MHz" },
	{ 5, 2, 2, "AXI/AHB clock selection: 266/333MHz" },
	{ 5, 2, 3, "AXI/AHB clock selection: 200/250Mhz" },
	{ 7, 1, 0, "AXI/AHB clock PLL: MPLL" },
	{ 7, 1, 1, "AXI/AHB clock PLL: HPLL" },
	{ 8, 1, 0, "Enable Debug Interfaces for CPU die" },
	{ 8, 1, 1, "Disable Debug Interfaces for CPU die" },
	{ 9, 1, 0, "TSP reset pin selection: SSPRSTN" },
	{ 9, 1, 1, "TSP reset pin selection: GPIO18E0" },
	{ 16, 1, 0, "Disable Secure Boot" },
	{ 16, 1, 1, "Enable Secure Boot" },
	{ 17, 1, 0, "Enable Debug Interfaces for IO die" },
	{ 17, 1, 1, "Disable Debug Interfaces for IO die" },
	{ 18, 1, 0, "Enable WDT reset full" },
	{ 18, 1, 1, "Disable WDT reset full" },
	{ 19, 1, 0, "Enable Uart Debug" },
	{ 19, 1, 1, "Disable Uart Debug" },
	{ 20, 1, 0, "Selection of IO for Uart Debug: IO13" },
	{ 20, 1, 1, "Selection of IO for Uart Debug: IO1" },
	{ 21, 1, 0, "Enable JTAG of Caliptra permanently" },
	{ 21, 1, 1, "Disable JTAG of Caliptra permanently" },
	{ 23, 1, 0, "Enable BootMCU ROM" },
	{ 23, 1, 1, "Disable BootMCU ROM" },
	{ 24, 1, 0, "Boot from eMMC or UFS selection: eMMC" },
	{ 24, 1, 1, "Boot from eMMC or UFS selection: UFS" },
	{ 25, 1, 0, "Disable allow BROM to wait SPI flash ready bit" },
	{ 25, 1, 1, "Enable allow BROM to wait SPI flash ready bit" },
	{ 26, 1, 0, "Disable use 4-byte command to read flash" },
	{ 26, 1, 1, "Enable use 4-byte command to read flash" },
	{ 27, 1, 0, "Recovery from Uart or USB selection: UART" },
	{ 27, 1, 1, "Recovery from Uart or USB selection: USB" },
};

static const struct otp_f_strap_info a0_f_strap_info[] = {
	{ 16, 1, 0, "Enable ARM ICE" },
	{ 16, 1, 1, "Disable ARM ICE" },
	{ 17, 1, 0, "Disable VGA Option ROM" },
	{ 17, 1, 1, "Enable VGA Option ROM" },
	{ 18, 1, 0, "VGA Class Code: VGA device" },
	{ 18, 1, 1, "VGA Class Code: Video device" },
	{ 20, 1, 0, "eMMC Boot Speed: 25MHz" },
	{ 20, 1, 1, "eMMC Boot Speed: 50MHz" },
	{ 21, 1, 0, "Enable PCIe XHCI" },
	{ 21, 1, 1, "Disable PCIe XHCI" },
	{ 22, 1, 0, "Enable ARM ICE in Trust World" },
	{ 22, 1, 1, "Disable ARM ICE in Trust World" },
	{ 25, 1, 0, "Enable WDT reset full" },
	{ 25, 1, 1, "Disable WDT reset full" },
	{ 30, 1, 0, "Enable RVAS" },
	{ 30, 1, 1, "Disable RVAS" },
	{ 32, 1, 0, "Disable boot storage ABR" },
	{ 32, 1, 1, "Enable boot storage ABR" },
	{ 33, 1, 0, "Disable allow BROM to clear unused SRAM" },
	{ 33, 1, 1, "Enable allow BROM to clear unused SRAM" },
	{ 34, 5, 0, "TPM PCR index selection" },
	{ 34, 5, 1, "TPM PCR index selection" },
	{ 34, 5, 2, "TPM PCR index selection" },
	{ 34, 5, 3, "TPM PCR index selection" },
	{ 34, 5, 4, "TPM PCR index selection" },
	{ 34, 5, 5, "TPM PCR index selection" },
	{ 34, 5, 6, "TPM PCR index selection" },
	{ 34, 5, 7, "TPM PCR index selection" },
	{ 34, 5, 8, "TPM PCR index selection" },
	{ 34, 5, 9, "TPM PCR index selection" },
	{ 34, 5, 10, "TPM PCR index selection" },
	{ 34, 5, 11, "TPM PCR index selection" },
	{ 34, 5, 12, "TPM PCR index selection" },
	{ 34, 5, 13, "TPM PCR index selection" },
	{ 34, 5, 14, "TPM PCR index selection" },
	{ 34, 5, 15, "TPM PCR index selection" },
	{ 34, 5, 16, "TPM PCR index selection" },
	{ 34, 5, 17, "TPM PCR index selection" },
	{ 34, 5, 18, "TPM PCR index selection" },
	{ 34, 5, 19, "TPM PCR index selection" },
	{ 34, 5, 20, "TPM PCR index selection" },
	{ 34, 5, 21, "TPM PCR index selection" },
	{ 34, 5, 22, "TPM PCR index selection" },
	{ 34, 5, 23, "TPM PCR index selection" },
	{ 34, 5, 24, "TPM PCR index selection" },
	{ 34, 5, 25, "TPM PCR index selection" },
	{ 34, 5, 26, "TPM PCR index selection" },
	{ 34, 5, 27, "TPM PCR index selection" },
	{ 34, 5, 28, "TPM PCR index selection" },
	{ 34, 5, 29, "TPM PCR index selection" },
	{ 34, 5, 30, "TPM PCR index selection" },
	{ 34, 5, 31, "TPM PCR index selection" },
	{ 45, 3, 0, "Size of FW SPI Flash: disable" },
	{ 45, 3, 1, "Size of FW SPI Flash: 8MB" },
	{ 45, 3, 2, "Size of FW SPI Flash: 16MB" },
	{ 45, 3, 3, "Size of FW SPI Flash: 32MB" },
	{ 45, 3, 4, "Size of FW SPI Flash: 64MB" },
	{ 45, 3, 5, "Size of FW SPI Flash: 128MB" },
	{ 45, 3, 6, "Size of FW SPI Flash: 256MB" },
	{ 45, 3, 7, "Size of FW SPI Flash: 512MB" },
	{ 48, 1, 0, "Disable FWSPI auxiliary pins" },
	{ 48, 1, 1, "Enable FWSPI auxiliary pins" },
	{ 49, 2, 0, "Size of FWSPI CRTM" },
	{ 49, 2, 1, "Size of FWSPI CRTM" },
	{ 49, 2, 2, "Size of FWSPI CRTM" },
	{ 49, 2, 3, "Size of FWSPI CRTM" },
	{ 51, 3, 0, "LTPI Max frequency: 1GHz" },
	{ 51, 3, 1, "LTPI Max frequency: 800MHz" },
	{ 51, 3, 2, "LTPI Max frequency: 400MHz" },
	{ 51, 3, 3, "LTPI Max frequency: 250MHz" },
	{ 51, 3, 4, "LTPI Max frequency: 200MHz" },
	{ 51, 3, 5, "LTPI Max frequency: 100MHz" },
	{ 51, 3, 6, "LTPI Max frequency: 50MHz" },
	{ 51, 3, 7, "LTPI Max frequency: 25MHz" },
	{ 54, 1, 0, "LTPI IO Type: LVDS" },
	{ 54, 1, 1, "LTPI IO Type: Single-End" },
	{ 55, 2, 0, "Boot SPI Frequency Selection: 12.5MHz" },
	{ 55, 2, 1, "Boot SPI Frequency Selection: 25MHz" },
	{ 55, 2, 2, "Boot SPI Frequency Selection: 40MHz" },
	{ 55, 2, 3, "Boot SPI Frequency Selection: 50MHz" },
	{ 57, 2, 0, "SPI TPM HASH Algorithm: SHA256" },
	{ 57, 2, 1, "SPI TPM HASH Algorithm: SHA384" },
	{ 57, 2, 2, "SPI TPM HASH Algorithm: SHA512" },
	{ 57, 2, 3, "SPI TPM HASH Algorithm: Reserved" },
	{ 59, 1, 0, "Disable TPM PCR extension" },
	{ 59, 1, 1, "Enable TPM PCR extension" },
	{ 60, 1, 0, "Enable CS swap after dual flash ABR is triggerred" },
	{ 60, 1, 1, "Disable CS swap after dual flash ABR is triggerred" },
	{ 61, 1, 0, "FMC ABR mode: dual flash ABR" },
	{ 61, 1, 1, "FMC ABR mode: single flash ABR" },
	{ 64, 3, 0, "Node 0 Size of Host SPI Flash 0: disable" },
	{ 64, 3, 1, "Node 0 Size of Host SPI Flash 0: 8MB" },
	{ 64, 3, 2, "Node 0 Size of Host SPI Flash 0: 16MB" },
	{ 64, 3, 3, "Node 0 Size of Host SPI Flash 0: 32MB" },
	{ 64, 3, 4, "Node 0 Size of Host SPI Flash 0: 64MB" },
	{ 64, 3, 5, "Node 0 Size of Host SPI Flash 0: 128MB" },
	{ 64, 3, 6, "Node 0 Size of Host SPI Flash 0: 256MB" },
	{ 64, 3, 7, "Node 0 Size of Host SPI Flash 0: 512MB" },
	{ 67, 1, 0, "Node 0 Disable Host SPI auxiliary pins" },
	{ 67, 1, 1, "Node 0 Enable Host SPI auxiliary pins" },
	{ 68, 2, 0, "Node 0 Size of Host SPI0 CRTM:" },
	{ 68, 2, 1, "Node 0 Size of Host SPI0 CRTM:" },
	{ 68, 2, 2, "Node 0 Size of Host SPI0 CRTM:" },
	{ 68, 2, 3, "Node 0 Size of Host SPI0 CRTM:" },
	{ 70, 1, 0, "Node 0 Selection of SIO decode address: 0x2E" },
	{ 70, 1, 1, "Node 0 Selection of SIO decode address: 0x4E" },
	{ 71, 1, 0, "Node 0 Enable SIO Decoding" },
	{ 71, 1, 1, "Node 0 Disable SIO Decoding" },
	{ 72, 1, 0, "Node 0 Disable ACPI" },
	{ 72, 1, 1, "Node 0 Enable ACPI" },
	{ 73, 1, 0, "Node 0 Disable LPC" },
	{ 73, 1, 1, "Node 0 Enable LPC" },
	{ 74, 1, 0, "Node 0 Disable eDAF" },
	{ 74, 1, 1, "Node 0 Enable eDAF" },
	{ 75, 1, 0, "Node 0 Disable eSPI RTC" },
	{ 75, 1, 1, "Node 0 Enable eSPI RTC" },
	{ 80, 3, 0, "Node 1 Size of Host SPI Flash 1: disable" },
	{ 80, 3, 1, "Node 1 Size of Host SPI Flash 1: 8MB" },
	{ 80, 3, 2, "Node 1 Size of Host SPI Flash 1: 16MB" },
	{ 80, 3, 3, "Node 1 Size of Host SPI Flash 1: 32MB" },
	{ 80, 3, 4, "Node 1 Size of Host SPI Flash 1: 64MB" },
	{ 80, 3, 5, "Node 1 Size of Host SPI Flash 1: 128MB" },
	{ 80, 3, 6, "Node 1 Size of Host SPI Flash 1: 256MB" },
	{ 80, 3, 7, "Node 1 Size of Host SPI Flash 1: 512MB" },
	{ 83, 1, 0, "Node 1 Disable Host SPI auxiliary pins" },
	{ 83, 1, 1, "Node 1 Enable Host SPI auxiliary pins" },
	{ 84, 2, 0, "Node 1 Size of Host SPI1 CRTM:" },
	{ 84, 2, 1, "Node 1 Size of Host SPI1 CRTM:" },
	{ 84, 2, 2, "Node 1 Size of Host SPI1 CRTM:" },
	{ 84, 2, 3, "Node 1 Size of Host SPI1 CRTM:" },
	{ 86, 1, 0, "Node 1 Selection of SIO decode address: 0x2E" },
	{ 86, 1, 1, "Node 1 Selection of SIO decode address: 0x4E" },
	{ 87, 1, 0, "Node 1 Enable SIO Decoding" },
	{ 87, 1, 1, "Node 1 Disable SIO Decoding" },
	{ 88, 1, 0, "Node 1 Disable ACPI" },
	{ 88, 1, 1, "Node 1 Enable ACPI" },
	{ 89, 1, 0, "Node 1 Disable LPC" },
	{ 89, 1, 1, "Node 1 Enable LPC" },
	{ 90, 1, 0, "Node 1 Disable eDAF" },
	{ 90, 1, 1, "Node 1 Enable eDAF" },
	{ 91, 1, 0, "Node 1 Disable eSPI RTC" },
	{ 91, 1, 1, "Node 1 Enable eSPI RTC" },
	{ 96, 1, 0, "Enable SSP running: (A0 not support)" },
	{ 96, 1, 1, "Disable SSP running: (A0 not support)" },
	{ 97, 1, 0, "Enable TSP running: (A0 not support)" },
	{ 97, 1, 1, "Disable TSP running: (A0 not support)" },
	{ 98, 1, 0, "Node0 : Enable Option ROM: Sys BIOS" },
	{ 98, 1, 1, "Node0 : Enable Option ROM: DRAM" },
	{ 99, 1, 0, "Node1 : Enable Option ROM: Sys BIOS" },
	{ 99, 1, 1, "Node1 : Enable Option ROM: DRAM" },
	{ 100, 1, 0, "Node0 : Size of VGA RAM: 32MB" },
	{ 100, 1, 1, "Node0 : Size of VGA RAM: 64MB" },
	{ 101, 1, 0, "Node1 : Size of VGA RAM: (A0 not support)" },
	{ 101, 1, 1, "Node1 : Size of VGA RAM: (A0 not support)" },
	{ 102, 1, 0, "Enable DP: (A0 not support)" },
	{ 102, 1, 1, "Enable DP: (A0 not support)" },
	{ 103, 1, 0, "Enable PCIe VGA0" },
	{ 103, 1, 1, "Disable PCIe VGA0" },
	{ 105, 1, 0, "Enable PCIe EHCI0" },
	{ 105, 1, 1, "Disable PCIe EHCI0" },
	{ 106, 1, 0, "Enable PCIe XHCI0" },
	{ 106, 1, 1, "Disable PCIe XHCI0" },
	{ 107, 1, 0, "Enable PCIe VGA1" },
	{ 107, 1, 1, "Disable PCIe VGA1" },
	{ 109, 1, 0, "Enable PCIe EHCI1" },
	{ 109, 1, 1, "Disable PCIe EHCI1" },
	{ 110, 1, 0, "Enable PCIe XHCI1" },
	{ 110, 1, 1, "Disable PCIe XHCI1" },
};
