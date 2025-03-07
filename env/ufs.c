// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2008-2011 Freescale Semiconductor, Inc.
 */

/* #define DEBUG */

#include <common.h>
#include <asm/global_data.h>

#include <command.h>
#include <env.h>
#include <env_internal.h>
#include <fdtdec.h>
#include <linux/stddef.h>
#include <malloc.h>
#include <memalign.h>
#include <scsi.h>
#include <ufs.h>
#include <part.h>
#include <search.h>
#include <errno.h>
#include <dm/ofnode.h>

#define ENV_UFS_INVALID_OFFSET ((s64)-1)

struct ufs {
	struct blk_desc *bd;
	u32 write_bl_len;
	u32 read_bl_len;
};

struct ufs g_ufs;
struct ufs *ufs = &g_ufs;
/* Default ENV offset when not defined in Device Tree */
#define ENV_UFS_OFFSET		CONFIG_ENV_OFFSET

DECLARE_GLOBAL_DATA_PTR;

static inline s64 ufs_offset(struct ufs *ufs, int copy)
{
	s64 offset = 0x400000;

	return offset;
}

__weak int ufs_get_env_addr(struct ufs *ufs, int copy, u32 *env_addr)
{
	s64 offset = ufs_offset(ufs, copy);

	if (offset == ENV_UFS_INVALID_OFFSET) {
		printf("Invalid ENV offset in UFS, copy=%d\n", copy);
		return -ENOENT;
	}

	*env_addr = offset;

	return 0;
}

static inline int ufs_set_env_part(struct ufs *ufs, uint part) {return 0; };
static inline int ufs_set_env_part_restore(struct ufs *ufs) {return 0; };

static void fini_ufs_for_env(struct ufs *ufs)
{
	ufs_set_env_part_restore(ufs);
}

static inline int write_env(struct ufs *ufs, unsigned long size,
			    unsigned long offset, const void *buffer)
{
	uint blk_start, blk_cnt, n;
	struct blk_desc *desc = ufs->bd;

	blk_start	= ALIGN(offset, ufs->write_bl_len) / ufs->write_bl_len;
	blk_cnt		= ALIGN(size, ufs->write_bl_len) / ufs->write_bl_len;

	n = blk_dwrite(desc, blk_start, blk_cnt, (u_char *)buffer);

	return (n == blk_cnt) ? 0 : -1;
}

static int env_ufs_save(void)
{
	ALLOC_CACHE_ALIGN_BUFFER(env_t, env_new, 1);
	u32	offset;
	int	ret, copy = 0;

	ret = env_export(env_new);
	if (ret)
		goto fini;

	if (ufs_get_env_addr(ufs, copy, &offset)) {
		ret = 1;
		goto fini;
	}

	printf("Writing to %sUFS... ", copy ? "redundant " : "");
	if (write_env(ufs, CONFIG_ENV_SIZE, offset, (u_char *)env_new)) {
		puts("failed\n");
		ret = 1;
		goto fini;
	}

	ret = 0;

fini:
	fini_ufs_for_env(ufs);

	return ret;
}

static inline int read_env(struct ufs *ufs, unsigned long size,
			   unsigned long offset, const void *buffer)
{
	uint blk_start, blk_cnt, n;
	struct blk_desc *desc = ufs->bd;

	blk_start	= ALIGN(offset, ufs->read_bl_len) / ufs->read_bl_len;
	blk_cnt		= ALIGN(size, ufs->read_bl_len) / ufs->read_bl_len;

	n = blk_dread(desc, blk_start, blk_cnt, (uchar *)buffer);

	return (n == blk_cnt) ? 0 : -1;
}

static int env_ufs_load(void)
{
	ALLOC_CACHE_ALIGN_BUFFER(char, buf, CONFIG_ENV_SIZE);
	struct blk_desc *bd;
	u32 offset;
	int ret;
	const char *errmsg = NULL;
	env_t *ep = NULL;

	if (IS_ENABLED(CONFIG_DM_SCSI)) {
		scsi_scan(false);

		bd = blk_get_devnum_by_uclass_id(UCLASS_SCSI, 0);
		if (!bd) {
			printf("Get scsi device failed\n");
			return -ENODEV;
		}
	}

	ufs->bd = bd;
	ufs->write_bl_len = 0x1000;
	ufs->read_bl_len = 0x1000;

	if (ufs_get_env_addr(ufs, 0, &offset)) {
		ret = -EIO;
		goto fini;
	}

	if (read_env(ufs, CONFIG_ENV_SIZE, offset, buf)) {
		errmsg = "!read failed";
		ret = -EIO;
		goto fini;
	}

	ret = env_import(buf, 1, H_EXTERNAL);
	if (!ret) {
		ep = (env_t *)buf;
		gd->env_addr = (ulong)&ep->data;
	}

fini:
	fini_ufs_for_env(ufs);

	if (ret)
		env_set_default(errmsg, 0);

	return ret;
}

U_BOOT_ENV_LOCATION(ufs) = {
	.location	= ENVL_UFS,
	ENV_NAME("UFS")
	.load		= env_ufs_load,
	.save		= env_save_ptr(env_ufs_save),
};
