/**
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/random.h>

#include "bmarkset.h"
#include "bmark_memcpy.h"




void dump_field(char *f, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		printf("0x%.2X ", f[i]);
		if ((i + 1) % 16 == 0)
			printf("\n");
	}
	printf("\n");
}

void run_subtest_iterations(subtest_t *subtest, int iterations)
{
	for (int i = 0; i < iterations; i++) {
		printf("iteration %i: ", i);
		if (test_subtest_exec(subtest, i) < 0) {
			printf("FAIL!\n");
			//dump_field(src, len);
			//dump_field(dest, len);
			continue;
		}

		printf("OK (%lluus)\n", subtest->chrono.tdlast);
	}
	printf("done. (%lluus)\n", subtest->chrono.tdsum);

	printf("Summary (type;len;#iterations;min;max;avg)\n");
	printf("%s;%s;", subtest->test->name, subtest->name);
	chrono_fprintf_csv(&subtest->chrono, stdout);

	chrono_fprintf_pretty(&subtest->chrono, " + ", stdout);
}

void runtest(test_t *test, int seed, int iterations)
{
	test_init(test, seed);

	for (
		subtest_t *s = test_get_first_subtest(test);
		s != NULL;
		s = test_get_next_subtest(s)
	)
		run_subtest_iterations(s, iterations);

	test_cleanup(test);
}

int memcpytesttest()
{
	int ret = 0;

	testset_t *testset = testset_create("rvvbmark");
	if (testset == NULL)
		return -1;

	if (test_memcpy_add(testset, 1024 * 1024 * 10 + 1) < 0) {
		goto __ret_testset_destroy;
		return -1;
	}

	testset_reset(testset);

	runtest(testset->tests_head, 0, 10);

	ret = 0;

__ret_testset_destroy:
	testset_destroy(testset);
	return ret;
}




int main(int argc, char **argv)
{
	printf("%s (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>\n", RVVBMARK_VERSION_STR);
	printf("RISC-V RVV support is ");
#if RVVBMARK_RVV_SUPPORT == 1
	printf("enabled\n");
#else
	printf("disabled\n");
#endif


	memcpytesttest();

	return 0;
}
