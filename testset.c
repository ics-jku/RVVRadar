#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include "testset.h"


static subtest_t *subtest_create(
	const char *name, bool rv, bool rvv,
	test_init_subtest_t init,
	test_run_subtest_t run,
	test_cleanup_subtest_t cleanup,
	int data_len)
{
	subtest_t *subtest = calloc(1, sizeof(subtest_t));
	if (subtest == NULL)
		return NULL;

	subtest->name = name;
	subtest->rv = rv;
	subtest->rvv = rvv;
	subtest->init = init;
	subtest->run = run;
	subtest->cleanup = cleanup;

	/* alloc optional data area */
	if (data_len == 0)
		return subtest;
	subtest->data = calloc(1, data_len);
	if (subtest->data == NULL) {
		free(subtest);
		return NULL;
	}

	return subtest;
}


static void subtest_destroy(subtest_t *subtest)
{
	if (subtest == NULL)
		return;

	/* free optional data area */
	if (subtest->data)
		free(subtest->data);

	free(subtest);
}


static void subtest_reset(subtest_t *subtest)
{
	if (subtest == NULL)
		return;
	chrono_reset(&subtest->chrono);
}


static void test_reset(test_t *test)
{
	if (test == NULL)
		return;
	for (
		subtest_t *s = test->subtests_head;
		s != NULL;
		s = s->next
	)
		subtest_reset(s);
}




/*
 * wrappers for function pointers
 */

static int test_init_subtest(subtest_t *subtest, int iteration)
{
	if (subtest == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* nothing todo? */
	if (subtest->init == NULL)
		return 0;

	return subtest->init(subtest, iteration);
}


static int test_run_subtest(subtest_t *subtest)
{
	if (subtest == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* nothing todo? */
	if (subtest->run == NULL)
		return 0;

	return subtest->run(subtest);

}


static int test_cleanup_subtest(subtest_t *subtest)
{
	if (subtest == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* nothing todo? */
	if (subtest->cleanup == NULL)
		return 0;

	return subtest->cleanup(subtest);
}




/*
 * API
 */


/*
 * allocated and create a new testset
 */
testset_t *testset_create(const char *name)
{
	testset_t *testset = calloc(1, sizeof(testset_t));
	if (testset == NULL)
		return NULL;

	testset->name = name;

	return testset;
}

void testset_destroy(testset_t *testset)
{
	if (testset == NULL)
		return;
	test_t *t = testset->tests_head;
	while (t != NULL) {
		test_t *n = t->next;
		test_destroy(t);
		t = n;
	}
	free(testset);
}


test_t *test_create(
	const char *name,
	test_init_test_t init,
	test_cleanup_test_t cleanup,
	unsigned int data_len)
{
	test_t *test = calloc(1, sizeof(test_t));
	if (test == NULL)
		return NULL;

	test->name = name;
	test->init = init;
	test->cleanup = cleanup;

	/* alloc optional data area */
	if (data_len == 0)
		return test;
	test->data = calloc(1, data_len);
	if (test->data == NULL) {
		free(test);
		return NULL;
	}

	return test;
}


void testset_reset(testset_t *testset)
{
	if (testset == NULL)
		return;
	for (
		test_t *t = testset->tests_head;
		t != NULL;
		t = t->next
	)
		test_reset(t);
}


void test_destroy(test_t *test)
{
	if (test == NULL)
		return;
	subtest_t *s = test->subtests_head;
	while (s != NULL) {
		subtest_t *n = s->next;
		subtest_destroy(s);
		s = n;
	}

	/* free optional data area */
	if (test->data)
		free(test->data);

	free(test);
}


int testset_add_test(testset_t *testset, test_t *test)
{
	if (testset == NULL || test == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* add to link list */
	test->index = testset->tests_len;
	if (testset->tests_tail == NULL)
		/* first element */
		testset->tests_head = test;
	else
		testset->tests_tail->next = test;
	testset->tests_tail = test;
	testset->tests_len++;

	/* link test (parent) */
	test->testset = testset;

	return 0;
}


int test_add_subtest(
	test_t *test,
	const char *name, bool rv, bool rvv,
	test_init_subtest_t init,
	test_run_subtest_t run,
	test_cleanup_subtest_t cleanup,
	unsigned int data_len)
{
	subtest_t *subtest = subtest_create(
				     name, rv, rvv,
				     init, run, cleanup,
				     data_len);
	if (subtest == NULL)
		return -1;

	/* add to link list */
	subtest->index = test->subtests_len;
	if (test->subtests_tail == NULL)
		/* first element */
		test->subtests_head = subtest;
	else
		test->subtests_tail->next = subtest;
	test->subtests_tail = subtest;
	test->subtests_len++;

	/* link test (parent) */
	subtest->test = test;

	return 0;
}


int test_init(test_t *test, int seed)
{
	if (test == NULL) {
		errno = ENODEV;
		return -1;
	}

	/* nothing todo? */
	if (test->init == NULL)
		return 0;

	return test->init(test, seed);
}


int test_cleanup(test_t *test)
{
	if (test == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* nothing todo? */
	if (test->cleanup == NULL)
		return 0;

	return test->cleanup(test);
}


subtest_t *test_get_first_subtest(test_t *test)
{
	if (test == NULL) {
		errno = EINVAL;
		return NULL;
	}

	return test->subtests_head;
}


subtest_t *test_get_next_subtest(subtest_t *subtest)
{
	if (subtest == NULL) {
		errno = EINVAL;
		return NULL;
	}

	return subtest->next;
}


int test_subtest_exec(subtest_t *subtest, int iteration)
{
	int ret = 0;

	/* init run */
	ret = test_init_subtest(subtest, iteration);
	if (ret < 0)
		return -1;

	/* run and measure */
	chrono_start(&subtest->chrono);
	ret = test_run_subtest(subtest);
	if (ret < 0)
		return -1;
	chrono_stop(&subtest->chrono);

	/* cleanup run */
	ret = test_cleanup_subtest(subtest);
	if (ret < 0)
		return -1;

	return 0;
}
