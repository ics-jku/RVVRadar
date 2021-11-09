/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <core/rvv_helpers.h>
#include "alg.h"


/* algorithm specific data */
struct data {
	enum alg_png_filters_filter filter;
	unsigned int bpp;	// bytes per pixel
	unsigned int rowbytes;	// bytes per line
	uint8_t *prev_row;	// input last row
	uint8_t *row_orig;	// input row
	uint8_t *row;		// input/output row
	uint8_t *row_compare;	// output row to compare
};


/* implementation specific data */
typedef int (*png_filters_fp_t)(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
struct impldata {
	png_filters_fp_t png_filters;	// png_filters to be called by wrapper
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


static int impl_preexec(impl_t *impl, int iteration, bool verify)
{
	struct data *d = IMPL_GET_ALG_PRIV_DATA(struct data*, impl);
	/* restore row before execution */
	memcpy(d->row, d->row_orig, d->rowbytes);
	return 0;
}


static int impl_exec_wrapper(impl_t *impl, bool verify)
{
	struct data *d = IMPL_GET_ALG_PRIV_DATA(struct data*, impl);
	struct impldata *sd = IMPL_GET_PRIV_DATA(struct impldata*, impl);
	sd->png_filters(d->bpp, d->rowbytes, d->row, d->prev_row);
	return 0;
}


static int impl_postexec(impl_t *impl, bool verify)
{
	/* result-verify disabled -> nothing to do */
	if (!verify)
		return 0;

	struct data *d = IMPL_GET_ALG_PRIV_DATA(struct data*, impl);

	/* use memcpy for speed -> use diff only if error was detected */
	int ret = memcmp(d->row, d->row_compare, d->rowbytes);
	if (ret) {
		diff_row(d->row, d->row_compare, d->rowbytes);
		return 1;	/* data error */
	}

	return 0;
}


static int impl_add(
	alg_t *alg,
	const char *name,
	png_filters_fp_t png_filters)
{
	impl_t *impl;

	impl = alg_add_impl(alg,
			    name,
			    NULL,
			    impl_preexec,
			    impl_exec_wrapper,
			    impl_postexec,
			    NULL,
			    sizeof(struct impldata));
	if (impl == NULL)
		return -1;

	struct impldata *sd = IMPL_GET_PRIV_DATA(struct impldata*, impl);
	sd->png_filters = png_filters;

	return 0;
}



/* up implementations */

extern void png_filters_up_c_byte_avect(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
extern void png_filters_up_c_byte_noavect(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
#if RVVBMARK_RVV_SUPPORT
extern void png_filters_up_rvv_m1(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
extern void png_filters_up_rvv_m2(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
extern void png_filters_up_rvv_m4(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
extern void png_filters_up_rvv_m8(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
#endif /* RVVBMARK_RVV_SUPPORT */

static int impls_add_up(alg_t *alg)
{
	int ret = 0;

	ret |= impl_add(alg, "c byte noavect",	(png_filters_fp_t)png_filters_up_c_byte_noavect);
	ret |= impl_add(alg, "c byte avect",	(png_filters_fp_t)png_filters_up_c_byte_avect);
#if RVVBMARK_RVV_SUPPORT
	ret |= impl_add(alg, "rvv_m1",		(png_filters_fp_t)png_filters_up_rvv_m1);
	ret |= impl_add(alg, "rvv_m2",		(png_filters_fp_t)png_filters_up_rvv_m2);
	ret |= impl_add(alg, "rvv_m4",		(png_filters_fp_t)png_filters_up_rvv_m4);
	ret |= impl_add(alg, "rvv_m8",		(png_filters_fp_t)png_filters_up_rvv_m8);
#endif /* RVVBMARK_RVV_SUPPORT */

	if (ret)
		return -1;

	return 0;
}



/* sub implementations */

extern void png_filters_sub_c_byte_avect(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
extern void png_filters_sub_c_byte_noavect(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
#if RVVBMARK_RVV_SUPPORT
extern void png_filters_sub_rvv_dload(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
extern void png_filters_sub_rvv_reuse(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
#endif /* RVVBMARK_RVV_SUPPORT */

static int impls_add_sub(alg_t *alg)
{
	int ret = 0;

	ret |= impl_add(alg, "c byte noavect",	(png_filters_fp_t)png_filters_sub_c_byte_noavect);
	ret |= impl_add(alg, "c byte avect",	(png_filters_fp_t)png_filters_sub_c_byte_avect);
#if RVVBMARK_RVV_SUPPORT
	ret |= impl_add(alg, "rvv_dload",	(png_filters_fp_t)png_filters_sub_rvv_dload);
	ret |= impl_add(alg, "rvv_reuse",	(png_filters_fp_t)png_filters_sub_rvv_reuse);
#endif /* RVVBMARK_RVV_SUPPORT */

	if (ret)
		return -1;

	return 0;
}



/* avg implementations */

extern void png_filters_avg_c_byte_avect(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
extern void png_filters_avg_c_byte_noavect(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
#if RVVBMARK_RVV_SUPPORT
extern void png_filters_avg_rvv(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
#endif /* RVVBMARK_RVV_SUPPORT */

static int impls_add_avg(alg_t *alg)
{
	int ret = 0;

	ret |= impl_add(alg, "c byte noavect",	(png_filters_fp_t)png_filters_avg_c_byte_noavect);
	ret |= impl_add(alg, "c byte avect",	(png_filters_fp_t)png_filters_avg_c_byte_avect);
#if RVVBMARK_RVV_SUPPORT
	ret |= impl_add(alg, "rvv",		(png_filters_fp_t)png_filters_avg_rvv);
#endif /* RVVBMARK_RVV_SUPPORT */

	if (ret)
		return -1;

	return 0;
}



/* paeth implementations */

extern void png_filters_paeth_c_byte_avect(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
extern void png_filters_paeth_c_byte_noavect(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
#if RVVBMARK_RVV_SUPPORT
extern void png_filters_paeth_rvv_read_bulk(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
extern void png_filters_paeth_rvv(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row);
#endif /* RVVBMARK_RVV_SUPPORT */

static int impls_add_paeth(alg_t *alg)
{
	int ret = 0;

	ret |= impl_add(alg, "c byte noavect",	(png_filters_fp_t)png_filters_paeth_c_byte_noavect);
	ret |= impl_add(alg, "c byte avect",	(png_filters_fp_t)png_filters_paeth_c_byte_avect);
#if RVVBMARK_RVV_SUPPORT
	ret |= impl_add(alg, "rvv_read_bulk",	(png_filters_fp_t)png_filters_paeth_rvv_read_bulk);
	ret |= impl_add(alg, "rvv",		(png_filters_fp_t)png_filters_paeth_rvv);
#endif /* RVVBMARK_RVV_SUPPORT */

	if (ret)
		return -1;

	return 0;
}



static int alg_preexec(struct alg *alg, int seed)
{
	struct data *d = ALG_GET_PRIV_DATA(struct data*, alg);

	/* alloc */
	d->prev_row = malloc(d->rowbytes);
	if (d->prev_row == NULL)
		goto __err_alloc_prev_row;
	d->row_orig = malloc(d->rowbytes);
	if (d->row_orig == NULL)
		goto __err_alloc_row_orig;
	d->row = malloc(d->rowbytes);
	if (d->row == NULL)
		goto __err_alloc_row;
	d->row_compare = malloc(d->rowbytes);
	if (d->row_compare == NULL)
		goto __err_alloc_row_compare;

	/* init */
	srandom(seed);
	for (int i = 0; i < d->rowbytes; i++) {
		d->prev_row[i] = random();
		d->row_orig[i] = random();
	}
	memcpy(d->row, d->row_orig, d->rowbytes);

	/* calculate compare */
	switch (d->filter) {
	case up:
		png_filters_up_c_byte_avect(d->bpp, d->rowbytes, d->row, d->prev_row);
		break;
	case sub:
		png_filters_sub_c_byte_avect(d->bpp, d->rowbytes, d->row, d->prev_row);
		break;
	case avg:
		png_filters_avg_c_byte_avect(d->bpp, d->rowbytes, d->row, d->prev_row);
		break;
	case paeth:
		png_filters_paeth_c_byte_avect(d->bpp, d->rowbytes, d->row, d->prev_row);
		break;
	default:
		goto __err_calc_row_compare;
	}
	memcpy(d->row_compare, d->row, d->rowbytes);

	return 0;

__err_calc_row_compare:
__err_alloc_row_compare:
	free(d->row);
__err_alloc_row:
	free(d->row_orig);
__err_alloc_row_orig:
	free(d->prev_row);
__err_alloc_prev_row:
	return -1;
}


static int alg_postexec(struct alg *alg)
{
	struct data *d = ALG_GET_PRIV_DATA(struct data*, alg);

	free(d->prev_row);
	free(d->row_orig);
	free(d->row);
	free(d->row_compare);

	return 0;
}


int alg_png_filters_add(
	algset_t *algset,
	enum alg_png_filters_filter filter,
	enum alg_png_filters_bpp bpp,
	unsigned int len)
{
	int ret = 0;
	unsigned int bppval;

	/* parse parameter bpp */
	switch (bpp) {
	case bpp3:
		bppval = 3;
		break;
	case bpp4:
		bppval = 4;
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	/* parse parameter filter and build name string */
	char namestr[256] = "\0";
	switch (filter) {
	case up:
		sprintf(namestr, "png_filters_up%i", bppval);
		break;
	case sub:
		sprintf(namestr, "png_filters_sub%i", bppval);
		break;
	case avg:
		sprintf(namestr, "png_filters_avg%i", bppval);
		break;
	case paeth:
		sprintf(namestr, "png_filters_paeth%i", bppval);
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	unsigned int rowbytes = len * bppval;

	/* build parameter string */
	char parastr[256] = "\0";
	snprintf(parastr, 256, "len=%u,rowbytes=%u", len, rowbytes);

	/* create algorithm */
	alg_t *alg = alg_create(
			     namestr,
			     parastr,
			     alg_preexec,
			     alg_postexec,
			     sizeof(struct data));
	if (alg == NULL)
		return -1;

	/* set private data */
	struct data *d = ALG_GET_PRIV_DATA(struct data*, alg);
	d->filter = filter;
	d->bpp = bppval;
	d->rowbytes = rowbytes;

	/* add implementations according to parameter filter */
	switch (filter) {
	case up:
		ret = impls_add_up(alg);
		break;
	case sub:
		ret = impls_add_sub(alg);
		break;
	case avg:
		ret = impls_add_avg(alg);
		break;
	case paeth:
		ret = impls_add_paeth(alg);
		break;
	default:
		errno = EINVAL;
		ret = -1;
	}
	if (ret < 0) {
		alg_destroy(alg);
		return -1;
	}

	/* add algorithm to set */
	if (algset_add_alg(algset, alg) < 0) {
		alg_destroy(alg);
		return -1;
	}

	return 0;
}
