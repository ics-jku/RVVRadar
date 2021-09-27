/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "bmark_png_filter_paeth.h"


/* data for bmark */
struct data {
	unsigned int len;
	unsigned int bpp;	// bytes per pixel
	uint8_t *prev_row;	// input last row
	uint8_t *row_orig;	// input row
	uint8_t *row;		// input/output row
	uint8_t *row_compare;	// output row to compare
};


/* data for subbmark */
typedef int (*png_filter_paeth_fp_t)(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
struct subdata {
	png_filter_paeth_fp_t png_filter_paeth;	// png_filter_paeth to be called by wrapper
};


static void diff_row(uint8_t *output, uint8_t *original, int len)
{
	fprintf(stderr, "\n");
	for (int i = 0; i < len; i++)
		if (output[i] != original[i])
			fprintf(stderr, "%s: ERROR: diff on idx=%i: output=%i != original=%i\n",
				__FILE__, i, output[i], original[i]);
	fprintf(stderr, "\n");
}


static int subbmark_preexec(subbmark_t *subbmark, int iteration)
{
	struct data *d = (struct data*)subbmark->bmark->data;
	/* restore row before benchmark run */
	memcpy(d->row, d->row_orig, d->len * sizeof(*d->row));
	return 0;
}


static int subbmark_exec_wrapper(subbmark_t *subbmark)
{
	struct data *d = (struct data*)subbmark->bmark->data;
	struct subdata *sd = (struct subdata*)subbmark->data;
	sd->png_filter_paeth(d->bpp, d->len, d->row, d->prev_row);
	return 0;
}


static int subbmark_postexec(subbmark_t *subbmark)
{
	struct data *d = (struct data*)subbmark->bmark->data;

	/* use memcpy for speed -> use diff only if error was detected */
	int ret = memcmp(d->row, d->row_compare, d->len * sizeof(*d->row));
	if (ret) {
		diff_row(d->row, d->row_compare, d->len * sizeof(*d->row));
		return 1;	/* data error */
	}

	return 0;
}


static int subbmark_add(
	bmark_t *bmark,
	const char *name,
	png_filter_paeth_fp_t png_filter_paeth)
{
	subbmark_t *subbmark;

	subbmark = bmark_add_subbmark(bmark,
				      name,
				      NULL,
				      subbmark_preexec,
				      subbmark_exec_wrapper,
				      subbmark_postexec,
				      NULL,
				      sizeof(struct subdata));
	if (subbmark == NULL)
		return -1;

	struct subdata *sd = (struct subdata*)subbmark->data;
	sd->png_filter_paeth = png_filter_paeth;

	return 0;
}



extern void png_filter_paeth_c_byte_avect(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
extern void png_filter_paeth_c_byte_noavect(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
#if RVVBMARK_RVV_SUPPORT == 1
extern void png_filter_paeth_rvv_m1(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
extern void png_filter_paeth_rvv_m2(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
#endif /* RVVBMARK_RVV_SUPPORT == 1 */

static int subbmarks_add(bmark_t *bmark)
{
	int ret = 0;

	ret |= subbmark_add(bmark, "c byte noavect",	(png_filter_paeth_fp_t)png_filter_paeth_c_byte_noavect);
	ret |= subbmark_add(bmark, "c byte avect",	(png_filter_paeth_fp_t)png_filter_paeth_c_byte_avect);
#if RVVBMARK_RVV_SUPPORT == 1
	ret |= subbmark_add(bmark, "rvv_m1",		(png_filter_paeth_fp_t)png_filter_paeth_rvv_m1);
	ret |= subbmark_add(bmark, "rvv_m2",		(png_filter_paeth_fp_t)png_filter_paeth_rvv_m2);
#endif /* RVVBMARK_RVV_SUPPORT == 1 */

	if (ret)
		return -1;

	return 0;
}


static int bmark_preexec(struct bmark *bmark, int seed)
{
	struct data *d = (struct data*)bmark->data;

	/* alloc */
	d->prev_row = malloc(d->len * sizeof(*d->prev_row));
	if (d->prev_row == NULL)
		goto __err_prev_row;
	d->row_orig = malloc(d->len * sizeof(*d->row_orig));
	if (d->row_orig == NULL)
		goto __err_row_orig;
	d->row = malloc(d->len * sizeof(*d->row));
	if (d->row == NULL)
		goto __err_row;
	d->row_compare = malloc(d->len * sizeof(*d->row_compare));
	if (d->row_compare == NULL)
		goto __err_row_compare;

	/* init */
	srandom(seed);
	for (int i = 0; i < d->len; i++) {
		d->prev_row[i] = random();
		d->row_orig[i] = random();
	}
	memcpy(d->row, d->row_orig, d->len * sizeof(*d->row));

	/* calculate compare */
	png_filter_paeth_c_byte_avect(d->bpp, d->len, d->row, d->prev_row);
	memcpy(d->row_compare, d->row, d->len * sizeof(*d->row_compare));

	return 0;

__err_row_compare:
	free(d->row);
__err_row:
	free(d->row_orig);
__err_row_orig:
	free(d->prev_row);
__err_prev_row:
	return -1;
}


static int bmark_postexec(struct bmark *bmark)
{
	struct data *d = (struct data*)bmark->data;
	if (d == NULL)
		return 0;
	free(d->prev_row);
	free(d->row_orig);
	free(d->row);
	free(d->row_compare);
	return 0;
}


int bmark_png_filter_paeth_add(bmarkset_t *bmarkset, enum bmark_png_filter_paeth_type type, unsigned int len)
{
	unsigned int bpp;
	const char *name;

	/* parse type */
	switch (type) {
	case paeth3:
		bpp = 3;
		name = "png_filter_paeth3";
		break;
	case paeth4:
		bpp = 4;
		name = "png_filter_paeth4";
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	/* ensure, that len is a multiple of bpp */
	len = ((len + bpp - 1) / bpp) * bpp;

	/* build parameter string */
	char parastr[256] = "\0";
	snprintf(parastr, 256, "len=%u", len);


	/* create benchmark */
	bmark_t *bmark = bmark_create(
				 name,
				 parastr,
				 bmark_preexec,
				 bmark_postexec,
				 sizeof(struct data));
	if (bmark == NULL)
		return -1;

	/* set private data and add sub benchmarks */
	struct data *d = (struct data*)bmark->data;
	d->len = len;
	d->bpp = bpp;

	if (subbmarks_add(bmark) < 0) {
		bmark_destroy(bmark);
		return -1;
	}

	/* add benchmark to set */
	if (bmarkset_add_bmark(bmarkset, bmark) < 0) {
		bmark_destroy(bmark);
		return -1;
	}

	return 0;
}
