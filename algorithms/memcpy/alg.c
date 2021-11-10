/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <stdlib.h>
#include <string.h>

#include <core/rvv_helpers.h>
#include "alg.h"


/* algorithm specific data */
struct data {
	unsigned int len;
	void *src;
	void *dest;
};


/* implementation specific data */
typedef int (*memcpy_fp_t)(void *dest, void *src, unsigned int len);
struct impldata {
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


static int impl_preexec(impl_t *impl, int iteration, bool verify)
{
	/* result-verify disabled -> nothing to do */
	if (!verify)
		return 0;

	struct data *d = IMPL_GET_ALG_PRIV_DATA(struct data*, impl);
	/* reset dest array before execution */
	memset(d->dest, 0, d->len);
	return 0;
}


static int impl_exec_wrapper(impl_t *impl, bool verify)
{
	struct data *d = IMPL_GET_ALG_PRIV_DATA(struct data*, impl);
	struct impldata *sd = IMPL_GET_PRIV_DATA(struct impldata*, impl);
	sd->memcpy(d->dest, d->src, d->len);
	return 0;
}


static int impl_postexec(impl_t *impl, bool verify)
{
	/* result-verify disabled -> nothing to do */
	if (!verify)
		return 0;

	struct data *d = IMPL_GET_ALG_PRIV_DATA(struct data*, impl);

	/* use memcpy for speed -> use diff only if error was detected */
	int ret = memcmp(d->dest, d->src, d->len);
	if (ret) {
		diff_fields(d->dest, d->src, d->len);
		return 1;	/* data error */
	}

	return 0;
}


static int impl_add(
	alg_t *alg,
	const char *name,
	memcpy_fp_t memcpy)
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
	sd->memcpy = memcpy;

	return 0;
}



extern void memcpy_c_byte_avect(char *dest, char *src, unsigned int len);
extern void memcpy_c_byte_noavect(char *dest, char *src, unsigned int len);
#if RVVRADAR_RV_SUPPORT
extern void memcpy_rv_wlenx4(void *dest, void *src, unsigned int len);
#if RVVRADAR_RVV_SUPPORT
extern void memcpy_rvv_8_m1(void *dest, void *src, unsigned int len);
extern void memcpy_rvv_8_m2(void *dest, void *src, unsigned int len);
extern void memcpy_rvv_8_m4(void *dest, void *src, unsigned int len);
extern void memcpy_rvv_8_m8(void *dest, void *src, unsigned int len);
extern void memcpy_rvv_32(void *dest, void *src, unsigned int len);
#endif /* RVVRADAR_RVV_SUPPORT */
#endif /* RVVRADAR_RV_SUPPORT */

static int impls_add(alg_t *alg)
{
	int ret = 0;

	ret |= impl_add(alg, "c byte noavect",	 			(memcpy_fp_t)memcpy_c_byte_noavect);
#if RVVRADAR_RV_SUPPORT
	ret |= impl_add(alg, "4 int regs",	 			(memcpy_fp_t)memcpy_rv_wlenx4);
#endif /* RVVRADAR_RV_SUPPORT */
	ret |= impl_add(alg, "c byte avect",	 			(memcpy_fp_t)memcpy_c_byte_avect);
	ret |= impl_add(alg, "system",		 			(memcpy_fp_t)memcpy);
#if RVVRADAR_RVV_SUPPORT
	ret |= impl_add(alg, "rvv 32bit elements", 			(memcpy_fp_t)memcpy_rvv_32);
	ret |= impl_add(alg, "rvv 8bit elements (no grouping)",  	(memcpy_fp_t)memcpy_rvv_8_m1);
	ret |= impl_add(alg, "rvv 8bit elements (group two)",  		(memcpy_fp_t)memcpy_rvv_8_m2);
	ret |= impl_add(alg, "rvv 8bit elements (group four)",  	(memcpy_fp_t)memcpy_rvv_8_m4);
	ret |= impl_add(alg, "rvv 8bit elements (group eight)",  	(memcpy_fp_t)memcpy_rvv_8_m8);
#endif /* RVVRADAR_RVV_SUPPORT */

	if (ret)
		return -1;

	return 0;
}


static int alg_preexec(struct alg *alg, int seed)
{
	struct data *d = ALG_GET_PRIV_DATA(struct data*, alg);
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


static int alg_postexec(struct alg *alg)
{
	struct data *d = ALG_GET_PRIV_DATA(struct data*, alg);

	free(d->src);
	free(d->dest);

	return 0;
}


int alg_memcpy_add(algset_t *algset, unsigned int len)
{
	/*
	 * fixup data len
	 *
	 * some implementations only allow muliples of x bytes
	 * multiples of 4*64 bit will work for every test
	 * -> ensure len is a multiple of 4*64 bits (= 32 bytes)
	 */
	unsigned int multiple = 4 * 64 / 8;
	len = ((len + multiple - 1) / multiple) * multiple;

	/* build parameter string */
	char parastr[256] = "\0";
	snprintf(parastr, 256, "len=%u", len);

	/* create algorithm */
	alg_t *alg = alg_create(
			     "memcpy",
			     parastr,
			     alg_preexec,
			     alg_postexec,
			     sizeof(struct data));
	if (alg == NULL)
		return -1;

	/* set private data */
	struct data *d = ALG_GET_PRIV_DATA(struct data*, alg);
	d->len = len;

	/* add implementations */
	if (impls_add(alg) < 0) {
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
