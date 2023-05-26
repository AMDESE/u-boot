/* SPDX-License-Identifier: GPL-2.0 */

#define OTP_INFO_VER		"1.0.0"

#define OTP_REG_RESERVED        -1

struct otpstrap_info {
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

static const struct otpconf_info a0_conf_info[] = {
};
