/**
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <sys/random.h>
#include <time.h>

#include "bmark_memcpy.h"

extern void memcpy_c_byte(char *src, char *dest, unsigned int len);
#if RVVBMARK_RV_SUPPORT == 1
extern void memcpy_rv_wlenx4(void *src, void *dest, unsigned int len);
#if RVVBMARK_RVV_SUPPORT == 1
extern void memcpy_rvv_8(void *src, void *dest, unsigned int len);
extern void memcpy_rvv_32(void *src, void *dest, unsigned int len);
#endif /* RVVBMARK_RVV_SUPPORT == 1 */
#endif /* RVVBMARK_RV_SUPPORT == 1 */


struct data {
	unsigned int len;
	void *src;
	void *dest;
};


static void dump_field(char *f, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		printf("0x%.2X ", f[i]);
		if ((i + 1) % 16 == 0)
			printf("\n");
	}
	printf("\n");
}


/* reset dest array before benchmark run */
static int subbmark_init(subbmark_t *subbmark, int iteration)
{
	struct data *d = (struct data*)subbmark->bmark->data;
	memset(d->dest, 0, d->len);
	return 0;
}


static int subbmark_cleanup(subbmark_t *subbmark)
{
	struct data *d = (struct data*)subbmark->bmark->data;
	int ret = memcmp(d->dest, d->src, d->len);
	if (!ret)
		return 0;
	return -1;
}


static int subbmark_run_sys(subbmark_t *subbmark)
{
	struct data *d = (struct data*)subbmark->bmark->data;
	memcpy(d->dest, d->src, d->len);
	return 0;
}


static int subbmark_run_c_byte(subbmark_t *subbmark)
{
	struct data *d = (struct data*)subbmark->bmark->data;
	memcpy_c_byte(d->dest, d->src, d->len);
	return 0;
}


#if RVVBMARK_RV_SUPPORT == 1
static int subbmark_run_rv_wlenx4(subbmark_t *subbmark)
{
	struct data *d = (struct data*)subbmark->bmark->data;
	memcpy_rv_wlenx4(d->dest, d->src, d->len);
	return 0;
}


#if RVVBMARK_RVV_SUPPORT == 1
static int subbmark_run_rvv_8(subbmark_t *subbmark)
{
	struct data *d = (struct data*)subbmark->bmark->data;
	memcpy_rvv_8(d->dest, d->src, d->len);
	return 0;
}


static int subbmark_run_rvv_32(subbmark_t *subbmark)
{
	struct data *d = (struct data*)subbmark->bmark->data;
	memcpy_rvv_32(d->dest, d->src, d->len);
	return 0;
}
#endif /* RVVBMARK_RVV_SUPPORT == 1 */
#endif /* RVVBMARK_RV_SUPPORT == 1 */


static int subbmarks_add(bmark_t *bmark)
{
	int ret;

	ret = bmark_add_subbmark(bmark,
				 "system",
				 false, false,
				 subbmark_init,
				 subbmark_run_sys,
				 subbmark_cleanup,
				 0);
	if (ret < 0)
		return ret;

	ret = bmark_add_subbmark(bmark,
				 "c_byte",
				 false, false,
				 subbmark_init,
				 subbmark_run_c_byte,
				 subbmark_cleanup,
				 0);
	if (ret < 0)
		return ret;

#if RVVBMARK_RV_SUPPORT == 1
	ret = bmark_add_subbmark(bmark,
				 "4 int regs",
				 true, false,
				 subbmark_init,
				 subbmark_run_rv_wlenx4,
				 subbmark_cleanup,
				 0);
	if (ret < 0)
		return ret;

#if RVVBMARK_RVV_SUPPORT == 1
	ret = bmark_add_subbmark(bmark,
				 "rvv 8bit elements",
				 true, true,
				 subbmark_init,
				 subbmark_run_rvv_8,
				 subbmark_cleanup,
				 0);
	if (ret < 0)
		return ret;

	ret = bmark_add_subbmark(bmark,
				 "rvv 32bit elements",
				 true, true,
				 subbmark_init,
				 subbmark_run_rvv_32,
				 subbmark_cleanup,
				 0);
	if (ret < 0)
		return ret;
#endif /* RVVBMARK_RVV_SUPPORT == 1 */
#endif /* RVVBMARK_RV_SUPPORT == 1 */

	return 0;
}


static int init(struct bmark *bmark, int seed)
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

	/* init (TODO deterministic with seed !!! */
	if (getrandom(d->src, d->len, 0) != d->len) {
		ret = -1;
		goto __err_random;
	}

	return 0;

__err_random:
	free(d->dest);
__err_dest:
	free(d->src);
__err_src:
	return ret;
}


static int cleanup(struct bmark *bmark)
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
	 * multiples of 4*64 byte will work for every test
	 * -> ensure len is a multiple of 4*64 bytes
	 *
	 * Caution: implementation works only for powers of two!
	 */
	len &= ~(4 * 64 - 1);	// cut down to prev multiple
	len |= 4 * 64;		// round up to next multiple

	/* build parameter string */
	char parastr[256] = "\0";
	snprintf(parastr, 256, "len=%u", len);

	/* create benchmark */
	bmark_t *bmark = bmark_create(
				 "memcpy",
				 parastr,
				 init,
				 cleanup,
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
