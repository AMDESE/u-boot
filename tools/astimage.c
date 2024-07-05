// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2024 Aspeed Technology Inc.
 */

#include <u-boot/ecdsa.h>
#include "imagetool.h"
#include <image.h>
#include "compiler.h"

#define AST_HDR_MAGIC		"ASTH"
#define FMC_MAX_SIZE		88 * 1024 /* 88KB */

#define SHA384_BLOCK_SIZE	128
#define INTPUT_FILE_MAX		10

struct hdr_preamble {
	char magic[4];
	uint8_t sig_ecc[96];
	uint8_t sig_lms[1620];
	uint32_t reserved[18];
};

struct hdr_body {
	uint32_t svn;
	uint32_t fmc_size;
	uint8_t digest_arr[INTPUT_FILE_MAX][48];
	uint32_t reserved[70];
};

struct ast_image_header {
	struct hdr_preamble preamble;
	struct hdr_body body;
} __packed;

struct ast_image_content_info {
	char *fname;
	int size;
};

static struct ast_image_content_info input_images[INTPUT_FILE_MAX] = {0};

int ast_image_check_params(struct image_tool_params *params)
{
	char *datafile = params->datafile;
	int count = 0;

	// fprintf(stdout, "datafile: %s\n", datafile);

	input_images[0].fname = datafile;
	while (input_images[count].fname) {
		// fprintf(stdout, "fname[%d]: %s\n", count, input_images[count].fname);
		count++;
		input_images[count].fname = strchr(input_images[count - 1].fname, ':');
		if (input_images[count].fname) {
			*input_images[count].fname = '\0';
			input_images[count].fname++;
		}
	}

	for (int i = 0; i < count; i++) {
		input_images[i].size = imagetool_get_filesize(params, input_images[i].fname);
		// fprintf(stdout, "fname[%d]: %s\n", i, input_images[i].fname);
		// fprintf(stdout, "fsize[%d]: 0x%x\n", i, input_images[i].size);
	}

	return 0;
}

static int ast_image_verify_header(unsigned char *ptr, int image_size,
				   struct image_tool_params *params)
{
	struct ast_image_header *hdr = (struct ast_image_header *)ptr;

	if (memcmp(hdr->preamble.magic, AST_HDR_MAGIC, sizeof(hdr->preamble.magic))) {
		printf("header magic wrong, %s\n", hdr->preamble.magic);
		return -FDT_ERR_BADSTRUCTURE;
	}

	printf("PASS\n");
	return 0;
}

static void ast_image_print_header(const void *ptr, struct image_tool_params *params)
{
	struct ast_image_header *hdr = (struct ast_image_header *)ptr;

	printf("Image Type: Aspeed Boot Image\n");
	fprintf(stdout, "Image SVN: 0x%x\n", hdr->body.svn);
	fprintf(stdout, "Image Size: 0x%x\n", hdr->body.fmc_size);
}

static void ast_image_set_header(void *ptr, struct stat *sbuf, int ifd,
				 struct image_tool_params *params)
{
	struct ast_image_header *hdr = (struct ast_image_header *)ptr;
	struct checksum_algo *checksum;
	struct image_sign_info info;
	struct image_region region;
	struct crypto_algo *crypto;
	uint8_t *sig, *image_ptr;
	int hdr_size, size;
	int total_size = 0;
	uint sig_len;
	int ret;

	// printf("%s: ptr:0x%p\n", __func__, ptr);
	// printf("%s: file_size=0x%x\n", __func__, params->file_size);

	hdr_size = sizeof(struct ast_image_header);
	image_ptr = (uint8_t *)hdr + hdr_size;
	memcpy(hdr->preamble.magic, AST_HDR_MAGIC, sizeof(hdr->preamble.magic));

	// fprintf(stdout, "ast mkimage file sz = 0x%lx\n", sbuf->st_size);
	// printf("%s..., hdr size:0x%x\n", __func__, hdr_size);

	hdr->body.svn = 0;
	hdr->body.fmc_size = input_images[0].size;

	for (int i = 0; i < INTPUT_FILE_MAX; i++) {
		size = input_images[i].size;
		if (size) {
			sha384_csum_wd(image_ptr + total_size, size, hdr->body.digest_arr[i], SHA384_BLOCK_SIZE);
			total_size += size;
		}
	}

	// printf("Using %s\n", params->algo_name);
	// printf("%s: keyname: %s\n", __func__, params->keyname);

	crypto = image_get_crypto_algo(params->algo_name);
	checksum = image_get_checksum_algo(params->algo_name);
	info.crypto = crypto;
	info.checksum = checksum;
	info.keydir = params->keydir;

	info.keyname = params->keyname;
	region.data = &hdr->body;
	region.size = sizeof(struct hdr_body);

	ret = ecdsa_sign(&info, &region, 1, &sig, &sig_len);
	if (ret)
		fprintf(stdout, "ecdsa_sign ret:0x%x\n", ret);

	fprintf(stdout, "sig_len:0x%x\n", sig_len);

	ret = ecdsa_verify(&info, &region, 1, sig, sig_len);
	if (ret)
		fprintf(stdout, "ecdsa_verify ret:0x%x\n", ret);

	memcpy(hdr->preamble.sig_ecc, sig, sig_len);
}

static int ast_image_check_image_types(uint8_t type)
{
	return 0;
}

static int pad_file(struct image_tool_params *params, int ifd, int pad)
{
	uint8_t zeros[4096];

	memset(zeros, 0, sizeof(zeros));

	while (pad > 0) {
		int todo = sizeof(zeros);

		if (todo > pad)
			todo = pad;
		if (write(ifd, (char *)&zeros, todo) != todo) {
			fprintf(stderr, "%s: Write error on %s: %s\n",
				params->cmdname, params->imagefile,
				strerror(errno));
			return -1;
		}
		pad -= todo;
	}

	return 0;
}

static int copy_file(struct image_tool_params *params, int ifd, int idx,
		     const char *file, int padded_size)
{
	unsigned char *ptr;
	struct stat sbuf;
	int dfd, size;
	int hdr_size;
	struct ast_image_header ast_imghdr = {0};

	hdr_size = sizeof(struct ast_image_header);

	dfd = open(file, O_RDONLY | O_BINARY);
	if (dfd < 0) {
		fprintf(stderr, "%s: Can't open %s: %s\n",
			params->cmdname, file, strerror(errno));
		return -1;
	}

	if (fstat(dfd, &sbuf) < 0) {
		fprintf(stderr, "%s: Can't stat %s: %s\n",
			params->cmdname, file, strerror(errno));
		goto err_close;
	}

	if (params->vflag)
		fprintf(stderr, "Size %u(pad to %u)\n",
			(int)sbuf.st_size, padded_size);

	ptr = mmap(0, sbuf.st_size, PROT_READ, MAP_SHARED, dfd, 0);
	if (ptr == MAP_FAILED) {
		fprintf(stderr, "%s: Can't read %s: %s\n",
			params->cmdname, file, strerror(errno));
		goto err_munmap;
	}

	if (idx == 0) {
		/* Add header size */
		memset(&ast_imghdr, 0, hdr_size);
		printf("Add header size: 0x%x\n", hdr_size);
		if (write(ifd, &ast_imghdr, hdr_size) != hdr_size) {
			fprintf(stderr, "%s: Write hdr_size error on %s: %s\n",
				params->cmdname, params->imagefile, strerror(errno));
			goto err_munmap;
		}
	}

	size = sbuf.st_size;
	printf("%s: size:0x%x\n", __func__, size);
	if (write(ifd, ptr, size) != size) {
		fprintf(stderr, "%s: Write error on %s: %s\n",
			params->cmdname, params->imagefile, strerror(errno));
		goto err_munmap;
	}

	munmap((void *)ptr, sbuf.st_size);
	close(dfd);
	return pad_file(params, ifd, padded_size - size);

err_munmap:
	munmap((void *)ptr, sbuf.st_size);
err_close:
	close(dfd);
	return -1;
}

int ast_copy_image(int ifd, struct image_tool_params *params)
{
	int ret;

	for (int i = 0; i < INTPUT_FILE_MAX; i++) {
		if (input_images[i].fname) {
			ret = copy_file(params, ifd, i, input_images[i].fname,
					input_images[i].size);
			if (ret)
				return ret;
		}
	}

	return ret;
}

U_BOOT_IMAGE_TYPE(
	astimage,
	"ASPEED Boot Image Support",
	0,
	NULL,
	ast_image_check_params,
	ast_image_verify_header,
	ast_image_print_header,
	ast_image_set_header,
	NULL,
	ast_image_check_image_types,
	NULL,
	NULL
);
