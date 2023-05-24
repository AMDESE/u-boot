/* SPDX-License-Identifier: GPL-2.0 */

#define OTP_INFO_VER		"1.0.0"

struct otpstrap_info {
	signed char bit_offset;
	signed char length;
	signed char value;
	const char *information;
};

struct otpconf_info {
	signed char dw_offset;
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

static const struct otpstrap_info a1_strap_info[] = {
};

static const struct otpconf_info a3_conf_info[] = {
};

static const struct scu_info a1_scu_info[] = {
};

