/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <sys/random.h>
#include <time.h>

#include "bmark_mac_16_32_32.h"


/* data for bmark */
struct data {
	unsigned int len;
	int16_t *mul1;
	int16_t *mul2;
	int32_t *add;
	int32_t *add_res;
	int32_t *compare;
};


/* data for subbmark */
typedef int (*mac_16_32_32_fp_t)(int32_t *add_res, int16_t *mul1, int16_t *mul2, unsigned int len);
struct subdata {
	mac_16_32_32_fp_t mac_16_32_32;	// mac to be called by wrapper
};


static void diff_fields(int32_t *dest, int32_t *src, int len)
{
	fprintf(stderr, "\n");
	for (int i = 0; i < len; i++)
		if (dest[i] != src[i])
			fprintf(stderr, "%s: ERROR: diff on idx=%i: dest=%i != src=%i\n",
				__FILE__, i, dest[i], src[i]);
	fprintf(stderr, "\n");
}


static int subbmark_preexec(subbmark_t *subbmark, int iteration)
{
	struct data *d = (struct data*)subbmark->bmark->data;
	/* init add_res with add before benchmark run */
	memcpy(d->add_res, d->add, d->len * sizeof(*d->add));
	return 0;
}


static int subbmark_exec_wrapper(subbmark_t *subbmark)
{
	struct data *d = (struct data*)subbmark->bmark->data;
	struct subdata *sd = (struct subdata*)subbmark->data;
	sd->mac_16_32_32(d->add_res, d->mul1, d->mul2, d->len);
	return 0;
}


static int subbmark_postexec(subbmark_t *subbmark)
{
	struct data *d = (struct data*)subbmark->bmark->data;

	/* use mac for speed -> use diff only if error was detected */
	int ret = memcmp(d->add_res, d->compare, d->len * sizeof(*d->add_res));
	if (ret) {
		diff_fields(d->add_res, d->compare, d->len);
		return -1;
	}

	return 0;
}


static int subbmark_add(
	bmark_t *bmark,
	const char *name,
	bool rv,
	bool rvv,
	mac_16_32_32_fp_t mac_16_32_32)
{
	subbmark_t *subbmark;

	subbmark = bmark_add_subbmark(bmark,
				      name, rv, rvv,
				      subbmark_preexec,
				      subbmark_exec_wrapper,
				      subbmark_postexec,
				      sizeof(struct subdata));
	if (subbmark == NULL)
		return -1;

	struct subdata *sd = (struct subdata*)subbmark->data;
	sd->mac_16_32_32 = mac_16_32_32;

	return 0;
}



extern void mac_16_32_32_c_byte_noavect(int32_t *add_res, int16_t *mul1, int16_t *mul2, unsigned int len);
extern void mac_16_32_32_c_byte_avect(int32_t *add_res, int16_t *mul1, int16_t *mul2, unsigned int len);
#if RVVBMARK_RVV_SUPPORT == 1
extern void mac_16_32_32_rvv_e32(int32_t *add_res, int16_t *mul1, int16_t *mul2, unsigned int len);
extern void mac_16_32_32_rvv_e16_widening(int32_t *add_res, int16_t *mul1, int16_t *mul2, unsigned int len);
#endif /* RVVBMARK_RVV_SUPPORT == 1 */

static int subbmarks_add(bmark_t *bmark)
{
	int ret = 0;

	ret |= subbmark_add(bmark, "c byte noavect",				false, false, (mac_16_32_32_fp_t)mac_16_32_32_c_byte_noavect);
	ret |= subbmark_add(bmark, "c byte avect",				false, false, (mac_16_32_32_fp_t)mac_16_32_32_c_byte_avect);
#if RVVBMARK_RVV_SUPPORT == 1
	ret |= subbmark_add(bmark, "rvv 32bit elements",			true,  true,  (mac_16_32_32_fp_t)mac_16_32_32_rvv_e32);
	ret |= subbmark_add(bmark, "rvv 16bit elements with widening",		true,  true,  (mac_16_32_32_fp_t)mac_16_32_32_rvv_e16_widening);
#endif /* RVVBMARK_RVV_SUPPORT == 1 */

	if (ret)
		return -1;

	return 0;
}


static int bmark_preexec(struct bmark *bmark, int seed)
{
	struct data *d = (struct data*)bmark->data;

	/* alloc */
	d->mul1 = malloc(d->len * sizeof(*d->mul1));
	if (d->mul1 == NULL)
		goto __err_mul1;
	d->mul2 = malloc(d->len * sizeof(*d->mul2));
	if (d->mul2 == NULL)
		goto __err_mul2;
	d->add = malloc(d->len * sizeof(*d->add));
	if (d->add == NULL)
		goto __err_add;
	d->add_res = malloc(d->len * sizeof(*d->add_res));
	if (d->add_res == NULL)
		goto __err_res;
	d->compare = malloc(d->len * sizeof(*d->compare));
	if (d->compare == NULL)
		goto __err_compare;

	/* init */
	srandom(seed);
	for (int i = 0; i < d->len; i++) {
		d->mul1[i] = random();
		d->mul2[i] = random();
		d->add[i] = random();
	}

	/* calculate compare */
	memcpy(d->add_res, d->add, d->len * sizeof(*d->add));
	mac_16_32_32_c_byte_avect(d->add_res, d->mul1, d->mul2, d->len);
	memcpy(d->compare, d->add_res, d->len * sizeof(*d->add_res));

	return 0;

__err_compare:
	free(d->add_res);
__err_res:
	free(d->add);
__err_add:
	free(d->mul2);
__err_mul2:
	free(d->mul1);
__err_mul1:
	return -1;
}


static int bmark_postexec(struct bmark *bmark)
{
	struct data *d = (struct data*)bmark->data;
	if (d == NULL)
		return 0;

	free(d->mul1);
	free(d->mul2);
	free(d->add);
	free(d->add_res);
	free(d->compare);

	return 0;
}


int bmark_mac_16_32_32_add(bmarkset_t *bmarkset, unsigned int len)
{
	/* build parameter string */
	char parastr[256] = "\0";
	snprintf(parastr, 256, "len=%u", len);

	/* create benchmark */
	bmark_t *bmark = bmark_create(
				 "mac 32bit = 32bit + (16bit*16bit)",
				 parastr,
				 bmark_preexec,
				 bmark_postexec,
				 sizeof(struct data));
	if (bmark == NULL)
		return -1;

	/* set private data and add sub benchmarks */
	struct data *d = (struct data*)bmark->data;
	d->len = len;

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
