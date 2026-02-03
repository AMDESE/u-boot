/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef _PATTERN_H_
#define _PATTERN_H_

const char *patterns[] = {
	"netdiag -o 0 -l phy -s 1000 -i rgmii-id",
	"netdiag -o 0 -l phy -s 100 -i rgmii-id",
	"netdiag -o 0 -l phy -s 10 -i rgmii-id",
	"netdiag -o 0 -l phy -s 1000 -i rgmii-id -m scan -k 1,4",
	"netdiag -o 1 -l phy -s 1000 -i rgmii-id",
	"netdiag -o 1 -l phy -s 100 -i rgmii-id",
	"netdiag -o 1 -l phy -s 10 -i rgmii-id",
	"netdiag -o 1 -l phy -s 1000 -i rgmii-id -m scan -k 1,4",
};

#endif	/* _PATTERN_H_ */
