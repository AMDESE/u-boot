// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2013
 * Texas Instruments, <www.ti.com>
 *
 * Dan Murphy <dmurphy@ti.com>
 *
 * Derived work from spl_usb.c
 */

#include <common.h>
#include <spl.h>
#include <asm/u-boot.h>
#include <sata.h>
#include <scsi.h>
#include <errno.h>
#include <fat.h>
#include <image.h>
#include <asm/arch-aspeed/abr.h>

static int sata_load_legacy(struct spl_image_info *spl_image,
			    struct spl_boot_device *bootdev,
			    struct blk_desc *stor_dev,
			    ulong sector, struct legacy_img_hdr *header)
{
	u32 image_offset_sectors;
	u32 image_size_sectors;
	unsigned long count;
	u32 image_offset;
	int ret;

	ret = spl_parse_image_header(spl_image, bootdev, header);
	if (ret)
		return ret;

	image_size_sectors = DIV_ROUND_UP(spl_image->size, stor_dev->blksz);
	image_offset_sectors = spl_image->offset / stor_dev->blksz;
	image_offset = spl_image->offset % stor_dev->blksz;

	count = blk_dread(stor_dev, sector + image_offset_sectors,
			image_size_sectors,
			(void *)spl_image->load_addr);
	if (count != image_size_sectors)
		return -EIO;

	if (image_offset)
		memmove((void *)spl_image->load_addr,
			(void *)spl_image->load_addr + image_offset,
			spl_image->size);

	return 0;
}

static ulong spl_sata_load_read(struct spl_load_info *load, ulong sector,
				ulong count, void *buf)
{
	struct blk_desc *stor_dev = load->dev;

	return blk_dread(stor_dev, sector, count, buf);
}

static int spl_sata_load_image_raw(struct spl_image_info *spl_image,
				   struct spl_boot_device *bootdev,
				   struct blk_desc *stor_dev, unsigned long sector)
{
	struct legacy_img_hdr *header;
	unsigned long count;
	int ret;

	if (abr_enabled())
		stor_dev->lun = (1 << abr_get_indicator());
	else
		stor_dev->lun = 1;

	header = spl_get_load_buffer(-sizeof(*header), stor_dev->blksz);
	count = blk_dread(stor_dev, sector, 1, header);
	if (count == 0)
		return -EIO;

	if (IS_ENABLED(CONFIG_SPL_LOAD_FIT) &&
	    image_get_magic(header) == FDT_MAGIC) {
		struct spl_load_info load;

		debug("Found FIT\n");
		load.dev = stor_dev;
		load.priv = NULL;
		load.filename = NULL;
		load.bl_len = stor_dev->blksz;
		load.read = spl_sata_load_read;
		ret = spl_load_simple_fit(spl_image, &load, sector, header);
	} else {
		ret = sata_load_legacy(spl_image, bootdev, stor_dev, sector, header);
	}

	if (ret)
		return -1;

	return 0;
}

static int spl_sata_load_image(struct spl_image_info *spl_image,
			       struct spl_boot_device *bootdev)
{
	int err = -ENOSYS;
	struct blk_desc *stor_dev;

	/* try to recognize storage devices immediately */
	scsi_scan(false);
	stor_dev = blk_get_devnum_by_uclass_id(UCLASS_SCSI, 0);
	if (!stor_dev)
		return -ENODEV;

#if CONFIG_IS_ENABLED(OS_BOOT)
	if (spl_start_uboot() ||
	    spl_load_image_fat_os(spl_image, bootdev, stor_dev,
				  CONFIG_SYS_SATA_FAT_BOOT_PARTITION))
#endif
	{
#ifdef CONFIG_SPL_FS_FAT
		err = spl_load_image_fat(spl_image, bootdev, stor_dev,
				CONFIG_SYS_SATA_FAT_BOOT_PARTITION,
				CONFIG_SPL_FS_LOAD_PAYLOAD_NAME);
#elif defined(CONFIG_SPL_SATA_RAW_U_BOOT_USE_SECTOR)
		err = spl_sata_load_image_raw(spl_image, bootdev, stor_dev,
			CONFIG_SPL_SATA_RAW_U_BOOT_SECTOR);
#endif
	}
	if (err) {
		puts("Error loading sata device\n");
		return err;
	}

	return 0;
}
SPL_LOAD_IMAGE_METHOD("SATA", 0, BOOT_DEVICE_SATA, spl_sata_load_image);
