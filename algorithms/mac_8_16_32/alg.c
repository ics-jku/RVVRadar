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
	int8_t *mul1;
	int8_t *mul2;
	int16_t *add;
	int32_t *res;
	int32_t *compare;
};


/* implementation specific data */
typedef int (*mac_8_16_32_fp_t)(int32_t *res, int16_t *add, int8_t *mul1, int8_t *mul2, unsigned int len);
struct impldata {
	mac_8_16_32_fp_t mac_8_16_32;	// mac to be called by wrapper
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


static int impl_preexec(impl_t *impl, int iteration, bool verify)
{
	/* result-verify disabled -> nothing to do */
	if (!verify)
		return 0;

	struct data *d = IMPL_GET_ALG_PRIV_DATA(struct data*, impl);
	/* reset result array before execution */
	memset(d->res, 0, d->len * sizeof(*d->res));
	return 0;
}


static int impl_exec_wrapper(impl_t *impl, bool verify)
{
	struct data *d = IMPL_GET_ALG_PRIV_DATA(struct data*, impl);
	struct impldata *sd = IMPL_GET_PRIV_DATA(struct impldata*, impl);
	sd->mac_8_16_32(d->res, d->add, d->mul1, d->mul2, d->len);
	return 0;
}


static int impl_postexec(impl_t *impl, bool verify)
{
	/* result-verify disabled -> nothing to do */
	if (!verify)
		return 0;

	struct data *d = IMPL_GET_ALG_PRIV_DATA(struct data*, impl);

	/* use mac for speed -> use diff only if error was detected */
	int ret = memcmp(d->res, d->compare, d->len * sizeof(*d->res));
	if (ret) {
		diff_fields(d->res, d->compare, d->len);
		return 1;	/* data error */
	}

	return 0;
}


static int impl_add(
	alg_t *alg,
	const char *name,
	mac_8_16_32_fp_t mac_8_16_32)
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
	sd->mac_8_16_32 = mac_8_16_32;

	return 0;
}



extern void mac_8_16_32_c_byte_noavect(int32_t *res, int16_t *add, int8_t *mul1, int8_t *mul2, unsigned int len);
extern void mac_8_16_32_c_byte_avect(int32_t *res, int16_t *add, int8_t *mul1, int8_t *mul2, unsigned int len);
#if RVVRADAR_RVV_SUPPORT
#if RVVRADAR_RVV_SUPPORT == RVVRADAR_RVV_SUPPORT_VER_07_08
/*
 * These implementations only make sense for rvv v0.7 and v0.8
 * In newer specs, there are no signed loads. Instead a unsigned load
 * must be combined with vsext, which adds an additional penalty.
 * Since these implementations generally are known to be less performant
 * it was decided to drop them completely for newer rvv drafts.
 */
extern void mac_8_16_32_rvv_e32(int32_t *res, int16_t *add, int8_t *mul1, int8_t *mul2, unsigned int len);
extern void mac_8_16_32_rvv_e16_widening(int32_t *res, int16_t *add, int8_t *mul1, int8_t *mul2, unsigned int len);
#endif /* RVVRADAR_RVV_SUPPORT_VER_07_08 */
extern void mac_8_16_32_rvv_e8_widening(int32_t *res, int16_t *add, int8_t *mul1, int8_t *mul2, unsigned int len);
#endif /* RVVRADAR_RVV_SUPPORT */

static int impls_add(alg_t *alg)
{
	int ret = 0;

	ret |= impl_add(alg, "c byte noavect",				(mac_8_16_32_fp_t)mac_8_16_32_c_byte_noavect);
	ret |= impl_add(alg, "c byte avect",				(mac_8_16_32_fp_t)mac_8_16_32_c_byte_avect);
#if RVVRADAR_RVV_SUPPORT
#if RVVRADAR_RVV_SUPPORT == RVVRADAR_RVV_SUPPORT_VER_07_08
	ret |= impl_add(alg, "rvv 32bit elements",			(mac_8_16_32_fp_t)mac_8_16_32_rvv_e32);
	ret |= impl_add(alg, "rvv 16bit elements with widening",	(mac_8_16_32_fp_t)mac_8_16_32_rvv_e16_widening);
#endif /* RVVRADAR_RVV_SUPPORT_VER_07_08 */
	ret |= impl_add(alg, "rvv 8bit elements with double widening",	(mac_8_16_32_fp_t)mac_8_16_32_rvv_e8_widening);
#endif /* RVVRADAR_RVV_SUPPORT */

	if (ret)
		return -1;

	return 0;
}


static int alg_preexec(struct alg *alg, int seed)
{
	struct data *d = ALG_GET_PRIV_DATA(struct data*, alg);

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
	d->res = malloc(d->len * sizeof(*d->res));
	if (d->res == NULL)
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
	memset(d->res, 0, d->len * sizeof(*d->res));

	/* calculate compare */
	mac_8_16_32_c_byte_avect(d->compare, d->add, d->mul1, d->mul2, d->len);

	return 0;

__err_compare:
	free(d->res);
__err_res:
	free(d->add);
__err_add:
	free(d->mul2);
__err_mul2:
	free(d->mul1);
__err_mul1:
	return -1;
}


static int alg_postexec(struct alg *alg)
{
	struct data *d = ALG_GET_PRIV_DATA(struct data*, alg);

	free(d->mul1);
	free(d->mul2);
	free(d->add);
	free(d->res);
	free(d->compare);

	return 0;
}


int alg_mac_8_16_32_add(algset_t *algset, unsigned int len)
{
	/* build parameter string */
	char parastr[256] = "\0";
	snprintf(parastr, 256, "len=%u", len);

	/* create algorithm */
	alg_t *alg = alg_create(
			     "mac 32bit = 16bit + 8bit * 8bit",
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
