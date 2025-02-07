// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Aspeed Technology Inc.
 */

#include <asm/arch/spi.h>
#include <asm/io.h>
#include <common.h>
#include <image.h>
#include <spl.h>

uint32_t gbl_spl_load_fit_addr;

static ulong ast_spl_ram_load_read(struct spl_load_info *load, ulong sector,
				   ulong count, void *buf)
{
	ulong addr;

	debug("%s: sector %lx, count %lx, buf %lx\n",
	      __func__, sector, count, (ulong)buf);

	addr = (ulong)gbl_spl_load_fit_addr + sector + aspeed_spi_abr_offset();
	memcpy(buf, (void *)addr, count);

	return count;
}

static int ast_spl_ram_load_image(struct spl_image_info *spl_image)
{
	struct legacy_img_hdr *header;
	int ret = -EINVAL;

	header = (struct legacy_img_hdr *)(gbl_spl_load_fit_addr + aspeed_spi_abr_offset());

	if (IS_ENABLED(CONFIG_SPL_LOAD_FIT) &&
	    image_get_magic(header) == FDT_MAGIC) {
		struct spl_load_info load;

		load.bl_len = 1;
		load.read = ast_spl_ram_load_read;
		ret = spl_load_simple_fit(spl_image, &load, 0, header);
	}

	return ret;
}

__section(".export_sym") void ast_loader(uint32_t addr)
{
	struct spl_image_info spl_image;

	memset(&spl_image, '\0', sizeof(spl_image));
	gbl_spl_load_fit_addr = addr;

	/* TODO: support different bootdev */
	ast_spl_ram_load_image(&spl_image);

	spl_board_prepare_for_boot();
}
