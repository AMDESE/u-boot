// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2025 ASPEED Technology Inc.
 */
#include <config.h>
#include <common.h>
#include <dm.h>
#include <misc.h>
#include <u-boot/cptra_ipc.h>

int cptra_stash_measurement(char *metadata, uint8_t *hash, int hash_len)
{
	enum cptra_ipc_cmd ipccmd = CPTRA_IPCCMD_STASH_MEASUREMENT;
	uint8_t *p8 = (uint8_t *)IPC_CHANNEL_1_NS_CA35_IN_ADDR;
	struct cptra_stash_measurement_ia input;
	struct cptra_stash_measurement_oa output;
	int ret;

	if (hash_len != 48) {
		pr_err("%s: invalid hash length %d\n", __func__, hash_len);
		return -1;
	}

	/* Initialize input data */
	memset(&input, 0, sizeof(struct cptra_stash_measurement_ia));
	memset(&output, 0, sizeof(struct cptra_stash_measurement_oa));

	memcpy(input.metadata, metadata, 4);
	memcpy(input.measure, hash, hash_len);

	/* Copy input data into shared memory */
	memcpy(p8, &input, sizeof(struct cptra_stash_measurement_ia));

	ret = cptra_ipc_trigger(ipccmd);
	if (ret) {
		pr_err("cptra_ipc_trigger:0x%x is failure, ret:0x%x\n", ipccmd,
		       ret);
		goto end;
	}

	ret = cptra_ipc_receive(CPTRA_IPC_RX_TYPE_EXTERNAL, &output,
				sizeof(struct cptra_stash_measurement_oa));
	if (ret) {
		pr_err("cptra_ipc_receive:0x%x is failure, ret:0x%x\n", ipccmd,
		       ret);
		goto end;
	}

	return output.dpe_result;

end:
	return ret;
}
