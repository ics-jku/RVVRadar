#ifndef TESTSET_H
#define TESTSET_H

#include <stdbool.h>

#include "chrono.h"


struct subtest;
typedef int (*test_init_subtest_t)(struct subtest *subtest, int iteration);
typedef int (*test_run_subtest_t)(struct subtest *subtest);
typedef int (*test_cleanup_subtest_t)(struct subtest *subtest);


typedef struct subtest {
	const char *name;			// name of the subtest
	unsigned int index;			// index in subtest list
	bool rv;				// is a RISC-V test
	bool rvv;				// is a RISC-V vector test

	test_init_subtest_t init;		// called before subtest
	test_run_subtest_t run;			// subtest function (measured)
	test_cleanup_subtest_t cleanup;		// called after subtest TODO: cleanup + check!

	struct test *test;			// parent test
	struct subtest *next;			// next in subtest list

	chrono_t chrono;			// chrono (including result statistics)

	void *data;				// optional data for the subtest
} subtest_t;


struct test;
typedef int (*test_init_test_t)(struct test *test, int seed);
typedef int (*test_cleanup_test_t)(struct test *test);

typedef struct test {
	const char *name;			// name of the test
	unsigned int index;			// index in test list

	// linked list of subtests
	struct subtest *subtests_head;
	struct subtest *subtests_tail;
	unsigned int subtests_len;

	test_init_test_t init;			// called before test
	test_cleanup_test_t cleanup;		// called after test

	struct testset *testset;		// parent testset
	struct test *next;			// next in test list

	void *data;				// optional data for the test
} test_t;


typedef struct testset {
	const char *name;

	// linked list of subtests
	struct test *tests_head;
	struct test *tests_tail;
	unsigned int tests_len;
} testset_t;


/*
 * allocated and create a new testset
 */
testset_t *testset_create(const char *name);


/*
 * reset all tests
 */
void testset_reset(testset_t *testset);


/*
 * destroy the testset
 */
void testset_destroy(testset_t *testset);


/*
 * create a test
 * handling of optional given data (free) is handled by test!
 */
test_t *test_create(
	const char *name,
	test_init_test_t init,
	test_cleanup_test_t cleanup,
	unsigned int data_len);


/*
 * destroy the test
 * (data will also freed here)
 */
void test_destroy(test_t *test);


int testset_add_test(testset_t *testset, test_t *test);


/*
 * allocated and create a new test
 * returns NULL on error
 */
int test_add_subtest(
	test_t *test,
	const char *name, bool rv, bool rvv,
	test_init_subtest_t init,
	test_run_subtest_t run,
	test_cleanup_subtest_t cleanup,
	unsigned int data_len);

int test_subtest_exec(subtest_t *subtest, int iteration);

int test_init(test_t *test, int seed);
int test_cleanup(test_t *test);
subtest_t *test_get_first_subtest(test_t *test);
subtest_t *test_get_next_subtest(subtest_t *subtest);

#endif /* TESTSET_H */
