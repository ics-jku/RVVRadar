/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <core/bmarkset.h>


/*
 * HELPERS
 */

#define DATAOUT	stdout
#define INFOOUT	stderr

/* print to stderr if verbose == true */
#define pinfo(args...)				\
	do {					\
		if (verbose)			\
			fprintf(INFOOUT, args);	\
	} while(0)


/*
 * ALGORITHM IMPLEMENTATION
 */

static impl_t *impl_create(
	const char *name,
	impl_init_fp_t init,
	impl_preexec_fp_t preexec,
	impl_exec_fp_t exec,
	impl_postexec_fp_t postexec,
	impl_cleanup_fp_t cleanup,
	int data_len)
{
	/* name must be given */
	if (name == NULL || strlen(name) == 0) {
		errno = EINVAL;
		return NULL;
	}

	impl_t *impl = calloc(1, sizeof(impl_t));
	if (impl == NULL)
		return NULL;

	impl->name = strdup(name);
	if (impl->name == NULL) {
		free(impl);
		return NULL;
	}
	impl->init = init;
	impl->preexec = preexec;
	impl->exec = exec;
	impl->postexec = postexec;
	impl->cleanup = cleanup;

	/* alloc optional data area */
	if (data_len == 0)
		return impl;
	impl->data = calloc(1, data_len);
	if (impl->data == NULL) {
		free(impl->name);
		free(impl);
		return NULL;
	}

	return impl;
}


static void impl_destroy(impl_t *impl)
{
	if (impl == NULL)
		return;

	chrono_cleanup(&impl->chrono);

	free(impl->name);

	/* free optional data area */
	free(impl->data);

	free(impl);
}


static void impl_reset(impl_t *impl)
{
	if (impl == NULL)
		return;

	impl->runs = 0;
	impl->fails = 0;

	chrono_init(&impl->chrono);
}


static int impl_call_init(impl_t *impl)
{
	if (impl == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* nothing todo? */
	if (impl->init == NULL)
		return 0;

	return impl->init(impl);
}


static int impl_call_preexec(impl_t *impl, int iteration, bool verify)
{
	if (impl == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* nothing todo? */
	if (impl->preexec == NULL)
		return 0;

	return impl->preexec(impl, iteration, verify);
}


static int impl_call_exec(impl_t *impl, bool verify)
{
	if (impl == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* nothing todo? */
	if (impl->exec == NULL)
		return 0;

	return impl->exec(impl, verify);

}


static int impl_call_postexec(impl_t *impl, bool verify)
{
	if (impl == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* nothing todo? */
	if (impl->postexec == NULL)
		return 0;

	return impl->postexec(impl, verify);
}


static int impl_call_cleanup(impl_t *impl)
{
	if (impl == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* nothing todo? */
	if (impl->cleanup == NULL)
		return 0;

	return impl->cleanup(impl);
}


static int impl_run(impl_t *impl, int iteration, bool verify)
{
	int ret = 0;

	if (impl == NULL) {
		errno = EINVAL;
		return -1;
	}

	impl->runs++;

	ret = impl_call_preexec(impl, iteration, verify);
	if (ret < 0)
		goto __err;

	/* exec and measure */
	chrono_start(&impl->chrono);
	ret = impl_call_exec(impl, verify);
	if (ret < 0)
		goto __err;
	chrono_stop(&impl->chrono);

	ret = impl_call_postexec(impl, verify);
	if (ret)
		goto __data_err;

	ret = 0;
	goto __ret;

__err:
	ret = -1;
__data_err:
	impl->fails++;
__ret:
	return ret;
}


static int impl_print_pretty(impl_t *impl, FILE *out)
{
	if (impl == NULL || out == NULL) {
		errno = EINVAL;
		return -1;
	}

	fprintf(out, "     + implementation: %s\n", impl->name);
	fprintf(out, "       + runs:  %i\n", impl->runs);
	fprintf(out, "       + fails: %i\n", impl->fails);
	fprintf(out, "       + timing:\n");
	chrono_print_pretty(&impl->chrono, "         + ", out);
	return 0;
}


static int impl_print_csv_head(FILE *out)
{
	if (out == NULL) {
		errno = EINVAL;
		return -1;
	}

	printf("set;benchmark(parameters);implementation;runs;fails;");
	chrono_print_csv_head(DATAOUT);
	printf("\n");
	return 0;
}


static int impl_print_csv(impl_t *impl, FILE *out)
{
	if (impl == NULL || out == NULL) {
		errno = EINVAL;
		return -1;
	}

	printf("%s;%s(%s);%s;%i;%i;",
	       impl->bmark->bmarkset->name,
	       impl->bmark->name,
	       impl->bmark->parastr,
	       impl->name,
	       impl->runs,
	       impl->fails);
	chrono_print_csv(&impl->chrono, out);
	printf("\n");
	return 0;
}


static int impl_run_iterations(impl_t *impl, int iterations, bool verify, bool verbose)
{
	if (impl == NULL) {
		errno = EINVAL;
		return -1;
	}

	for (int iteration = 0; iteration < iterations; iteration++) {
		pinfo("\r%s: %i/%i -> ", impl->name, iteration + 1, iterations);
		int ret = impl_run(impl, iteration, verify);
		if (ret < 0)
			return -1;
		else if (ret > 0) {
			/* data error */
			pinfo("FAIL!");
			continue;
		}
		pinfo("OK!");
	}
	pinfo("\r");
	for (int i = 0; i < 10; i++)
		pinfo("          ");
	pinfo("\r");
	if (verbose)
		impl_print_pretty(impl, INFOOUT);

	/* data output */
	impl_print_csv(impl, DATAOUT);

	return 0;
}


/*
 * BMARK
 */

bmark_t *bmark_create(
	const char *name,
	const char *parastr,
	bmark_preexec_fp_t preexec,
	bmark_postexec_fp_t postexec,
	unsigned int data_len)
{
	/* name must be given */
	if (name == NULL || strlen(name) == 0) {
		errno = EINVAL;
		return NULL;
	}

	bmark_t *bmark = calloc(1, sizeof(bmark_t));
	if (bmark == NULL)
		return NULL;

	bmark->name = strdup(name);
	if (bmark->name == NULL) {
		free(bmark);
		return NULL;
	}
	if (parastr == NULL)
		parastr = "";
	bmark->parastr = strdup(parastr);
	if (bmark->parastr == NULL) {
		free(bmark);
		return NULL;
	}
	bmark->preexec = preexec;
	bmark->postexec = postexec;

	/* alloc optional data area */
	if (data_len == 0)
		return bmark;
	bmark->data = calloc(1, data_len);
	if (bmark->data == NULL) {
		free(bmark->parastr);
		free(bmark->name);
		free(bmark);
		return NULL;
	}

	return bmark;
}


void bmark_destroy(bmark_t *bmark)
{
	if (bmark == NULL)
		return;
	impl_t *s = bmark->impls_head;
	while (s != NULL) {
		impl_t *n = s->next;
		impl_destroy(s);
		s = n;
	}

	free(bmark->name);
	free(bmark->parastr);

	/* free optional data area */
	free(bmark->data);

	free(bmark);
}


impl_t *bmark_add_impl(
	bmark_t *bmark,
	const char *name,
	impl_init_fp_t init,
	impl_preexec_fp_t preexec,
	impl_exec_fp_t exec,
	impl_postexec_fp_t postexec,
	impl_cleanup_fp_t cleanup,
	unsigned int data_len)
{
	impl_t *impl = impl_create(
			       name,
			       init, preexec, exec, postexec, cleanup,
			       data_len);
	if (impl == NULL)
		return NULL;

	/* add to link list */
	impl->index = bmark->impls_len;
	if (bmark->impls_tail == NULL)
		/* first element */
		bmark->impls_head = impl;
	else
		bmark->impls_tail->next = impl;
	bmark->impls_tail = impl;
	bmark->impls_len++;

	/* link bmark (parent) */
	impl->bmark = bmark;

	return impl;
}


static void bmark_reset(bmark_t *bmark)
{
	if (bmark == NULL)
		return;
	for (
		impl_t *s = bmark->impls_head;
		s != NULL;
		s = s->next
	)
		impl_reset(s);
}


impl_t *bmark_get_first_impl(bmark_t *bmark)
{
	if (bmark == NULL) {
		errno = EINVAL;
		return NULL;
	}

	return bmark->impls_head;
}


impl_t *bmark_get_next_impl(impl_t *impl)
{
	if (impl == NULL) {
		errno = EINVAL;
		return NULL;
	}

	return impl->next;
}


int bmark_call_preexec(bmark_t *bmark, int seed)
{
	if (bmark == NULL) {
		errno = ENODEV;
		return -1;
	}

	/* nothing todo? */
	if (bmark->preexec == NULL)
		return 0;

	return bmark->preexec(bmark, seed);
}


static int bmark_call_postexec(bmark_t *bmark)
{
	if (bmark == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* nothing todo? */
	if (bmark->postexec == NULL)
		return 0;

	return bmark->postexec(bmark);
}


static int bmark_run(bmark_t *bmark, int seed, int iterations, bool verify, bool verbose)
{
	int ret;

	if (bmark == NULL) {
		errno = EINVAL;
		return -1;
	}

	pinfo("   + benchm: %s(%s)\n", bmark->name, bmark->parastr);

	/* call preexec */
	ret = bmark_call_preexec(bmark, seed);
	if (ret < 0)
		return -1;

	/* run all implementations */
	for (
		impl_t *s = bmark->impls_head;
		s != NULL;
		s = s->next
	) {
		ret = impl_call_init(s);
		if (ret < 0)
			return -1;

		ret = impl_run_iterations(s, iterations, verify, verbose);
		if (ret < 0)
			return -1;

		ret = impl_call_cleanup(s);
		if (ret < 0)
			return -1;
	}

	/* call postexec */
	ret = bmark_call_postexec(bmark);
	if (ret < 0)
		return -1;

	return 0;
}


/*
 * BMARKSET
 */

bmarkset_t *bmarkset_create(const char *name)
{
	/* name must be given */
	if (name == NULL || strlen(name) == 0) {
		errno = EINVAL;
		return NULL;
	}

	bmarkset_t *bmarkset = calloc(1, sizeof(bmarkset_t));
	if (bmarkset == NULL)
		return NULL;

	bmarkset->name = strdup(name);
	if (bmarkset->name == NULL) {
		free(bmarkset);
		return NULL;
	}

	return bmarkset;
}


void bmarkset_destroy(bmarkset_t *bmarkset)
{
	if (bmarkset == NULL)
		return;
	bmark_t *t = bmarkset->bmarks_head;
	while (t != NULL) {
		bmark_t *n = t->next;
		bmark_destroy(t);
		t = n;
	}

	free(bmarkset->name);
	free(bmarkset);
}


int bmarkset_add_bmark(bmarkset_t *bmarkset, bmark_t *bmark)
{
	if (bmarkset == NULL || bmark == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* add to link list */
	bmark->index = bmarkset->bmarks_len;
	if (bmarkset->bmarks_tail == NULL)
		/* first element */
		bmarkset->bmarks_head = bmark;
	else
		bmarkset->bmarks_tail->next = bmark;
	bmarkset->bmarks_tail = bmark;
	bmarkset->bmarks_len++;

	/* link bmark (parent) */
	bmark->bmarkset = bmarkset;

	return 0;
}


void bmarkset_reset(bmarkset_t *bmarkset)
{
	if (bmarkset == NULL)
		return;
	for (
		bmark_t *t = bmarkset->bmarks_head;
		t != NULL;
		t = t->next
	)
		bmark_reset(t);
}


int bmarkset_run(bmarkset_t *bmarkset, int seed, int iterations, bool verify, bool verbose)
{
	int ret;

	if (bmarkset == NULL) {
		errno = EINVAL;
		return -1;
	}

	impl_print_csv_head(DATAOUT);

	pinfo(" + set: %s\n", bmarkset->name);
	for (
		bmark_t *b = bmarkset->bmarks_head;
		b != NULL;
		b = b->next
	) {
		ret = bmark_run(b, seed, iterations, verify, verbose);
		if (ret < 0)
			return -1;
	}

	return 0;
}
