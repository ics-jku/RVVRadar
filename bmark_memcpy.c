/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <stdlib.h>
#include <string.h>

#include "bmark_memcpy.h"


/* data for bmark */
struct data {
	unsigned int len;
	void *src;
	void *dest;
};


/* data for subbmark */
typedef int (*memcpy_fp_t)(void *dest, void *src, unsigned int len);
struct subdata {
	memcpy_fp_t memcpy;	// memcpy to be called by wrapper
};


static void diff_fields(char *dest, char *src, int len)
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
	/* reset dest array before benchmark run */
	memset(d->dest, 0, d->len);
	return 0;
}


static int subbmark_exec_wrapper(subbmark_t *subbmark)
{
	struct data *d = (struct data*)subbmark->bmark->data;
	struct subdata *sd = (struct subdata*)subbmark->data;
	sd->memcpy(d->dest, d->src, d->len);
	return 0;
}


static int subbmark_postexec(subbmark_t *subbmark)
{
	struct data *d = (struct data*)subbmark->bmark->data;

	/* use memcpy for speed -> use diff only if error was detected */
	int ret = memcmp(d->dest, d->src, d->len);
	if (ret) {
		diff_fields(d->dest, d->src, d->len);
		return 1;	/* data error */
	}

	return 0;
}


static int subbmark_add(
	bmark_t *bmark,
	const char *name,
	memcpy_fp_t memcpy)
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
	sd->memcpy = memcpy;

	return 0;
}



extern void memcpy_c_byte_avect(char *dest, char *src, unsigned int len);
extern void memcpy_c_byte_noavect(char *dest, char *src, unsigned int len);
#if RVVBMARK_RV_SUPPORT == 1
extern void memcpy_rv_wlenx4(void *dest, void *src, unsigned int len);
#if RVVBMARK_RVV_SUPPORT == 1
extern void memcpy_rvv_8(void *dest, void *src, unsigned int len);
extern void memcpy_rvv_32(void *dest, void *src, unsigned int len);
#endif /* RVVBMARK_RVV_SUPPORT == 1 */
#endif /* RVVBMARK_RV_SUPPORT == 1 */

static int subbmarks_add(bmark_t *bmark)
{
	int ret = 0;

	ret |= subbmark_add(bmark, "system",		 (memcpy_fp_t)memcpy);
	ret |= subbmark_add(bmark, "c byte noavect",	 (memcpy_fp_t)memcpy_c_byte_noavect);
	ret |= subbmark_add(bmark, "c byte avect",	 (memcpy_fp_t)memcpy_c_byte_avect);
#if RVVBMARK_RV_SUPPORT == 1
	ret |= subbmark_add(bmark, "4 int regs",	 (memcpy_fp_t)memcpy_rv_wlenx4);
#if RVVBMARK_RVV_SUPPORT == 1
	ret |= subbmark_add(bmark, "rvv 8bit elements",  (memcpy_fp_t)memcpy_rvv_8);
	ret |= subbmark_add(bmark, "rvv 32bit elements", (memcpy_fp_t)memcpy_rvv_32);
#endif /* RVVBMARK_RVV_SUPPORT == 1 */
#endif /* RVVBMARK_RV_SUPPORT == 1 */

	if (ret)
		return -1;

	return 0;
}


static int bmark_preexec(struct bmark *bmark, int seed)
{
	struct data *d = (struct data*)bmark->data;
	int ret = 0;

	/* alloc */
	d->src = malloc(d->len);
	if (d->src == NULL) {
		ret = -1;
		goto __err_src;
	}

	d->dest = malloc(d->len);
	if (d->dest == NULL) {
		ret = -1;
		goto __err_dest;
	}

	/* init with random */
	srandom(seed);
	for (int i = 0; i < d->len; i++)
		((unsigned char*)d->src)[i] = random();

	return 0;

__err_dest:
	free(d->src);
__err_src:
	return ret;
}


static int bmark_postexec(struct bmark *bmark)
{
	struct data *d = (struct data*)bmark->data;
	if (d == NULL)
		return 0;
	free(d->src);
	free(d->dest);
	return 0;
}


int bmark_memcpy_add(bmarkset_t *bmarkset, unsigned int len)
{
	/*
	 * fixup data len
	 *
	 * some implementations only allow muliples of x bytes
	 * multiples of 4*64 bit will work for every test
	 * -> ensure len is a multiple of 4*64 bits (= 32 bytes)
	 *
	 * Caution: implementation works only for powers of two!
	 */
	unsigned int multiple = 4 * 64 / 8;
	// cut down to prev multiple
	len &= ~(multiple - 1);
	// ensure at least multiple
	if (len < multiple)
		len = multiple;

	/* build parameter string */
	char parastr[256] = "\0";
	snprintf(parastr, 256, "len=%u", len);

	/* create benchmark */
	bmark_t *bmark = bmark_create(
				 "memcpy",
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
