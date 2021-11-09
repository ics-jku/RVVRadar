/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <core/algset.h>


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
	int priv_data_len)
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

	/* alloc optional private data area */
	if (priv_data_len == 0)
		return impl;
	impl->priv_data = calloc(1, priv_data_len);
	if (impl->priv_data == NULL) {
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

	/* free optional private data area */
	free(impl->priv_data);

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

	printf("set;algorithm(parameters);implementation;runs;fails;");
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
	       impl->alg->algset->name,
	       impl->alg->name,
	       impl->alg->parastr,
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
 * ALGORITHM
 */

alg_t *alg_create(
	const char *name,
	const char *parastr,
	alg_preexec_fp_t preexec,
	alg_postexec_fp_t postexec,
	unsigned int priv_data_len)
{
	/* name must be given */
	if (name == NULL || strlen(name) == 0) {
		errno = EINVAL;
		return NULL;
	}

	alg_t *alg = calloc(1, sizeof(alg_t));
	if (alg == NULL)
		return NULL;

	alg->name = strdup(name);
	if (alg->name == NULL) {
		free(alg);
		return NULL;
	}
	if (parastr == NULL)
		parastr = "";
	alg->parastr = strdup(parastr);
	if (alg->parastr == NULL) {
		free(alg);
		return NULL;
	}
	alg->preexec = preexec;
	alg->postexec = postexec;

	/* alloc optional private data area */
	if (priv_data_len == 0)
		return alg;
	alg->priv_data = calloc(1, priv_data_len);
	if (alg->priv_data == NULL) {
		free(alg->parastr);
		free(alg->name);
		free(alg);
		return NULL;
	}

	return alg;
}


void alg_destroy(alg_t *alg)
{
	if (alg == NULL)
		return;
	impl_t *s = alg->impls_head;
	while (s != NULL) {
		impl_t *n = s->next;
		impl_destroy(s);
		s = n;
	}

	free(alg->name);
	free(alg->parastr);

	/* free optional private data area */
	free(alg->priv_data);

	free(alg);
}


impl_t *alg_add_impl(
	alg_t *alg,
	const char *name,
	impl_init_fp_t init,
	impl_preexec_fp_t preexec,
	impl_exec_fp_t exec,
	impl_postexec_fp_t postexec,
	impl_cleanup_fp_t cleanup,
	unsigned int priv_data_len)
{
	impl_t *impl = impl_create(
			       name,
			       init, preexec, exec, postexec, cleanup,
			       priv_data_len);
	if (impl == NULL)
		return NULL;

	/* add to link list */
	impl->index = alg->impls_len;
	if (alg->impls_tail == NULL)
		/* first element */
		alg->impls_head = impl;
	else
		alg->impls_tail->next = impl;
	alg->impls_tail = impl;
	alg->impls_len++;

	/* link alg (parent) */
	impl->alg = alg;

	return impl;
}


static void alg_reset(alg_t *alg)
{
	if (alg == NULL)
		return;
	for (
		impl_t *s = alg->impls_head;
		s != NULL;
		s = s->next
	)
		impl_reset(s);
}


impl_t *alg_get_first_impl(alg_t *alg)
{
	if (alg == NULL) {
		errno = EINVAL;
		return NULL;
	}

	return alg->impls_head;
}


impl_t *alg_get_next_impl(impl_t *impl)
{
	if (impl == NULL) {
		errno = EINVAL;
		return NULL;
	}

	return impl->next;
}


int alg_call_preexec(alg_t *alg, int seed)
{
	if (alg == NULL) {
		errno = ENODEV;
		return -1;
	}

	/* nothing todo? */
	if (alg->preexec == NULL)
		return 0;

	return alg->preexec(alg, seed);
}


static int alg_call_postexec(alg_t *alg)
{
	if (alg == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* nothing todo? */
	if (alg->postexec == NULL)
		return 0;

	return alg->postexec(alg);
}


static int alg_run(alg_t *alg, int seed, int iterations, bool verify, bool verbose)
{
	int ret;

	if (alg == NULL) {
		errno = EINVAL;
		return -1;
	}

	pinfo("   + algorithm: %s(%s)\n", alg->name, alg->parastr);

	/* call preexec */
	ret = alg_call_preexec(alg, seed);
	if (ret < 0)
		return -1;

	/* run all implementations */
	for (
		impl_t *s = alg->impls_head;
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
	ret = alg_call_postexec(alg);
	if (ret < 0)
		return -1;

	return 0;
}


/*
 * ALGORITHM SET
 */

algset_t *algset_create(const char *name)
{
	/* name must be given */
	if (name == NULL || strlen(name) == 0) {
		errno = EINVAL;
		return NULL;
	}

	algset_t *algset = calloc(1, sizeof(algset_t));
	if (algset == NULL)
		return NULL;

	algset->name = strdup(name);
	if (algset->name == NULL) {
		free(algset);
		return NULL;
	}

	return algset;
}


void algset_destroy(algset_t *algset)
{
	if (algset == NULL)
		return;
	alg_t *t = algset->algs_head;
	while (t != NULL) {
		alg_t *n = t->next;
		alg_destroy(t);
		t = n;
	}

	free(algset->name);
	free(algset);
}


int algset_add_alg(algset_t *algset, alg_t *alg)
{
	if (algset == NULL || alg == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* add to link list */
	alg->index = algset->algs_len;
	if (algset->algs_tail == NULL)
		/* first element */
		algset->algs_head = alg;
	else
		algset->algs_tail->next = alg;
	algset->algs_tail = alg;
	algset->algs_len++;

	/* link algset (parent) */
	alg->algset = algset;

	return 0;
}


void algset_reset(algset_t *algset)
{
	if (algset == NULL)
		return;
	for (
		alg_t *t = algset->algs_head;
		t != NULL;
		t = t->next
	)
		alg_reset(t);
}


int algset_run(algset_t *algset, int seed, int iterations, bool verify, bool verbose)
{
	int ret;

	if (algset == NULL) {
		errno = EINVAL;
		return -1;
	}

	impl_print_csv_head(DATAOUT);

	pinfo(" + set: %s\n", algset->name);
	for (
		alg_t *b = algset->algs_head;
		b != NULL;
		b = b->next
	) {
		ret = alg_run(b, seed, iterations, verify, verbose);
		if (ret < 0)
			return -1;
	}

	return 0;
}
