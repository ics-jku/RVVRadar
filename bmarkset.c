#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "bmarkset.h"


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
 * SUBBMARK
 */

static subbmark_t *subbmark_create(
	const char *name, bool rv, bool rvv,
	subbmark_preexec_fp_t preexec,
	subbmark_exec_fp_t exec,
	subbmark_postexec_fp_t postexec,
	int data_len)
{
	subbmark_t *subbmark = calloc(1, sizeof(subbmark_t));
	if (subbmark == NULL)
		return NULL;

	subbmark->name = name;
	subbmark->rv = rv;
	subbmark->rvv = rvv;
	subbmark->preexec = preexec;
	subbmark->exec = exec;
	subbmark->postexec = postexec;

	/* alloc optional data area */
	if (data_len == 0)
		return subbmark;
	subbmark->data = calloc(1, data_len);
	if (subbmark->data == NULL) {
		free(subbmark);
		return NULL;
	}

	return subbmark;
}


static void subbmark_destroy(subbmark_t *subbmark)
{
	if (subbmark == NULL)
		return;

	chrono_cleanup(&subbmark->chrono);

	/* free optional data area */
	if (subbmark->data)
		free(subbmark->data);

	free(subbmark);
}


static void subbmark_reset(subbmark_t *subbmark)
{
	if (subbmark == NULL)
		return;

	subbmark->runs = 0;
	subbmark->fails = 0;

	chrono_init(&subbmark->chrono);
}


static int subbmark_call_preexec(subbmark_t *subbmark, int iteration)
{
	if (subbmark == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* nothing todo? */
	if (subbmark->preexec == NULL)
		return 0;

	return subbmark->preexec(subbmark, iteration);
}


static int subbmark_call_exec(subbmark_t *subbmark)
{
	if (subbmark == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* nothing todo? */
	if (subbmark->exec == NULL)
		return 0;

	return subbmark->exec(subbmark);

}


static int subbmark_call_postexec(subbmark_t *subbmark)
{
	if (subbmark == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* nothing todo? */
	if (subbmark->postexec == NULL)
		return 0;

	return subbmark->postexec(subbmark);
}


static int subbmark_run_single(subbmark_t *subbmark, int iteration)
{
	int ret = 0;

	if (subbmark == NULL) {
		errno = EINVAL;
		return -1;
	}

	subbmark->runs++;

	ret = subbmark_call_preexec(subbmark, iteration);
	if (ret < 0)
		goto __err;

	/* exec and measure */
	chrono_start(&subbmark->chrono);
	ret = subbmark_call_exec(subbmark);
	if (ret < 0)
		goto __err;
	chrono_stop(&subbmark->chrono);

	ret = subbmark_call_postexec(subbmark);
	if (ret < 0)
		goto __err;

	ret = 0;
	goto __ret;

__err:
	ret = -1;
	subbmark->fails++;
__ret:
	return ret;
}


static int subbmark_print_pretty(subbmark_t *subbmark, FILE *out)
{
	if (subbmark == NULL || out == NULL) {
		errno = EINVAL;
		return -1;
	}

	fprintf(out, "     + sub: %s\n", subbmark->name);
	fprintf(out, "       + runs:  %i\n", subbmark->runs);
	fprintf(out, "       + fails: %i\n", subbmark->fails);
	fprintf(out, "       + timing:\n");
	chrono_print_pretty(&subbmark->chrono, "         + ", out);
	return 0;
}


static int subbmark_print_csv_head(FILE *out)
{
	if (out == NULL) {
		errno = EINVAL;
		return -1;
	}

	printf("set;benchmark(parameters);sub;runs;fails;");
	chrono_print_csv_head(DATAOUT);
	printf("\n");
	return 0;
}


static int subbmark_print_csv(subbmark_t *subbmark, FILE *out)
{
	if (subbmark == NULL || out == NULL) {
		errno = EINVAL;
		return -1;
	}

	printf("%s;%s(%s);%s;%i;%i;",
	       subbmark->bmark->bmarkset->name,
	       subbmark->bmark->name,
	       subbmark->bmark->parastr,
	       subbmark->name,
	       subbmark->runs,
	       subbmark->fails);
	chrono_print_csv(&subbmark->chrono, out);
	printf("\n");
	return 0;
}


static int subbmark_run(subbmark_t *subbmark, int iterations, bool verbose)
{
	if (subbmark == NULL) {
		errno = EINVAL;
		return -1;
	}

	for (int iteration = 0; iteration < iterations; iteration++) {
		pinfo("\r%s: %i/%i -> ", subbmark->name, iteration + 1, iterations);
		if (subbmark_run_single(subbmark, iteration) < 0) {
			pinfo("FAIL!");
			continue;
		}
		pinfo("OK! (%llins)", subbmark->chrono.tdavg);
	}
	pinfo("\r                                                              \r");
	if (verbose)
		subbmark_print_pretty(subbmark, INFOOUT);

	// data output
	subbmark_print_csv(subbmark, DATAOUT);

	return 0;
}


/*
 * BMARK
 */

bmark_t *bmark_create(
	const char *name,
	char *parastr,
	bmark_preexec_fp_t preexec,
	bmark_postexec_fp_t postexec,
	unsigned int data_len)
{
	bmark_t *bmark = calloc(1, sizeof(bmark_t));
	if (bmark == NULL)
		return NULL;

	bmark->name = name;
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
		free(bmark);
		free(bmark->parastr);
		return NULL;
	}

	return bmark;
}


void bmark_destroy(bmark_t *bmark)
{
	if (bmark == NULL)
		return;
	subbmark_t *s = bmark->subbmarks_head;
	while (s != NULL) {
		subbmark_t *n = s->next;
		subbmark_destroy(s);
		s = n;
	}

	free(bmark->parastr);

	/* free optional data area */
	free(bmark->data);

	free(bmark);
}


int bmark_add_subbmark(
	bmark_t *bmark,
	const char *name, bool rv, bool rvv,
	subbmark_preexec_fp_t preexec,
	subbmark_exec_fp_t exec,
	subbmark_postexec_fp_t postexec,
	unsigned int data_len)
{
	subbmark_t *subbmark = subbmark_create(
				       name, rv, rvv,
				       preexec, exec, postexec,
				       data_len);
	if (subbmark == NULL)
		return -1;

	/* add to link list */
	subbmark->index = bmark->subbmarks_len;
	if (bmark->subbmarks_tail == NULL)
		/* first element */
		bmark->subbmarks_head = subbmark;
	else
		bmark->subbmarks_tail->next = subbmark;
	bmark->subbmarks_tail = subbmark;
	bmark->subbmarks_len++;

	/* link bmark (parent) */
	subbmark->bmark = bmark;

	return 0;
}


static void bmark_reset(bmark_t *bmark)
{
	if (bmark == NULL)
		return;
	for (
		subbmark_t *s = bmark->subbmarks_head;
		s != NULL;
		s = s->next
	)
		subbmark_reset(s);
}


subbmark_t *bmark_get_first_subbmark(bmark_t *bmark)
{
	if (bmark == NULL) {
		errno = EINVAL;
		return NULL;
	}

	return bmark->subbmarks_head;
}


subbmark_t *bmark_get_next_subbmark(subbmark_t *subbmark)
{
	if (subbmark == NULL) {
		errno = EINVAL;
		return NULL;
	}

	return subbmark->next;
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


static int bmark_run(bmark_t *bmark, int seed, int iterations, bool verbose)
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

	/* run all sub benchmarks */
	for (
		subbmark_t *s = bmark->subbmarks_head;
		s != NULL;
		s = s->next
	) {
		ret = subbmark_run(s, iterations, verbose);
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
	bmarkset_t *bmarkset = calloc(1, sizeof(bmarkset_t));
	if (bmarkset == NULL)
		return NULL;

	bmarkset->name = name;

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


int bmarkset_run(bmarkset_t *bmarkset, int seed, int iterations, bool verbose)
{
	int ret;

	if (bmarkset == NULL) {
		errno = EINVAL;
		return -1;
	}

	subbmark_print_csv_head(DATAOUT);

	pinfo(" + set: %s\n", bmarkset->name);
	for (
		bmark_t *b = bmarkset->bmarks_head;
		b != NULL;
		b = b->next
	) {
		ret = bmark_run(b, seed, iterations, verbose);
		if (ret < 0)
			return -1;
	}

	return 0;
}
