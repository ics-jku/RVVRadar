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

#include "test_memcpy.h"

#if RVVBENCH_RV_SUPPORT == 1
extern void memcpy_rv_wlenx4(void *src, void *dest, unsigned int len);
#if RVVBENCH_RVV_SUPPORT == 1
extern void memcpy_rvv_8(void *src, void *dest, unsigned int len);
extern void memcpy_rvv_32(void *src, void *dest, unsigned int len);
#endif /* RVVBENCH_RVV_SUPPORT == 1 */
#endif /* RVVBENCH_RV_SUPPORT == 1 */


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


/* cleanup dest array before test */
static int subtest_init(subtest_t *subtest, int iteration)
{
	struct data *d = (struct data*)subtest->test->data;
	memset(d->dest, 0, d->len);
	return 0;
}


static int subtest_cleanup(subtest_t *subtest)
{
	struct data *d = (struct data*)subtest->test->data;
	int ret = memcmp(d->dest, d->src, d->len);
	if (!ret)
		return 0;
	return -1;
}


static int subtest_run_sys(subtest_t *subtest)
{
	struct data *d = (struct data*)subtest->test->data;
	memcpy(d->dest, d->src, d->len);
	return 0;
}


#if RVVBENCH_RV_SUPPORT == 1
static int subtest_run_rv_wlenx4(subtest_t *subtest)
{
	struct data *d = (struct data*)subtest->test->data;
	memcpy_rv_wlenx4(d->dest, d->src, d->len);
	return 0;
}


#if RVVBENCH_RVV_SUPPORT == 1
static int subtest_run_rvv_8(subtest_t *subtest)
{
	struct data *d = (struct data*)subtest->test->data;
	memcpy_rvv_8(d->dest, d->src, d->len);
	return 0;
}


static int subtest_run_rvv_32(subtest_t *subtest)
{
	struct data *d = (struct data*)subtest->test->data;
	memcpy_rvv_32(d->dest, d->src, d->len);
	return 0;
}
#endif /* RVVBENCH_RVV_SUPPORT == 1 */
#endif /* RVVBENCH_RV_SUPPORT == 1 */


static int subtests_add(test_t *test)
{
	int ret;

	ret = test_add_subtest(test,
			       "system",
			       false, false,
			       subtest_init,
			       subtest_run_sys,
			       subtest_cleanup,
			       0);
	if (ret < 0)
		return ret;

#if RVVBENCH_RV_SUPPORT == 1
	ret = test_add_subtest(test,
			       "4 int regs",
			       true, false,
			       subtest_init,
			       subtest_run_rv_wlenx4,
			       subtest_cleanup,
			       0);
	if (ret < 0)
		return ret;

#if RVVBENCH_RVV_SUPPORT == 1
	ret = test_add_subtest(test,
			       "rvv 8bit elements",
			       true, true,
			       subtest_init,
			       subtest_run_rvv_8,
			       subtest_cleanup,
			       0);
	if (ret < 0)
		return ret;

	ret = test_add_subtest(test,
			       "rvv 32bit elements",
			       true, true,
			       subtest_init,
			       subtest_run_rvv_32,
			       subtest_cleanup,
			       0);
	if (ret < 0)
		return ret;
#endif /* RVVBENCH_RVV_SUPPORT == 1 */
#endif /* RVVBENCH_RV_SUPPORT == 1 */

	return 0;
}


static int init(struct test *test, int seed)
{
	struct data *d = (struct data*)test->data;
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


static int cleanup(struct test *test)
{
	struct data *d = (struct data*)test->data;
	if (d == NULL)
		return 0;
	free(d->src);
	free(d->dest);
	return 0;
}


int test_memcpy_add(testset_t *testset, unsigned int len)
{
	test_t *test = test_create("memcpy", init, cleanup, sizeof(struct data));
	if (test == NULL)
		return -1;

	struct data *d = (struct data*)test->data;
	d->len = len;

	if (subtests_add(test) < 0) {
		test_destroy(test);
		return -1;
	}

	if (testset_add_test(testset, test) < 0) {
		test_destroy(test);
		return -1;
	}

	return 0;
}
