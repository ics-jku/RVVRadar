#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include "bmarkset.h"


static subbmark_t *subbmark_create(
	const char *name, bool rv, bool rvv,
	bmark_init_subbmark_t init,
	bmark_run_subbmark_t run,
	bmark_cleanup_subbmark_t cleanup,
	int data_len)
{
	subbmark_t *subbmark = calloc(1, sizeof(subbmark_t));
	if (subbmark == NULL)
		return NULL;

	subbmark->name = name;
	subbmark->rv = rv;
	subbmark->rvv = rvv;
	subbmark->init = init;
	subbmark->run = run;
	subbmark->cleanup = cleanup;

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

	/* free optional data area */
	if (subbmark->data)
		free(subbmark->data);

	free(subbmark);
}


static void subbmark_reset(subbmark_t *subbmark)
{
	if (subbmark == NULL)
		return;
	chrono_reset(&subbmark->chrono);
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




/*
 * wrappers for function pointers
 */

static int bmark_init_subbmark(subbmark_t *subbmark, int iteration)
{
	if (subbmark == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* nothing todo? */
	if (subbmark->init == NULL)
		return 0;

	return subbmark->init(subbmark, iteration);
}


static int bmark_run_subbmark(subbmark_t *subbmark)
{
	if (subbmark == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* nothing todo? */
	if (subbmark->run == NULL)
		return 0;

	return subbmark->run(subbmark);

}


static int bmark_cleanup_subbmark(subbmark_t *subbmark)
{
	if (subbmark == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* nothing todo? */
	if (subbmark->cleanup == NULL)
		return 0;

	return subbmark->cleanup(subbmark);
}




/*
 * API
 */


/*
 * allocated and create a new bmarkset
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


bmark_t *bmark_create(
	const char *name,
	bmark_init_bmark_t init,
	bmark_cleanup_bmark_t cleanup,
	unsigned int data_len)
{
	bmark_t *bmark = calloc(1, sizeof(bmark_t));
	if (bmark == NULL)
		return NULL;

	bmark->name = name;
	bmark->init = init;
	bmark->cleanup = cleanup;

	/* alloc optional data area */
	if (data_len == 0)
		return bmark;
	bmark->data = calloc(1, data_len);
	if (bmark->data == NULL) {
		free(bmark);
		return NULL;
	}

	return bmark;
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

	/* free optional data area */
	if (bmark->data)
		free(bmark->data);

	free(bmark);
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


int bmark_add_subbmark(
	bmark_t *bmark,
	const char *name, bool rv, bool rvv,
	bmark_init_subbmark_t init,
	bmark_run_subbmark_t run,
	bmark_cleanup_subbmark_t cleanup,
	unsigned int data_len)
{
	subbmark_t *subbmark = subbmark_create(
				       name, rv, rvv,
				       init, run, cleanup,
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


int bmark_init(bmark_t *bmark, int seed)
{
	if (bmark == NULL) {
		errno = ENODEV;
		return -1;
	}

	/* nothing todo? */
	if (bmark->init == NULL)
		return 0;

	return bmark->init(bmark, seed);
}


int bmark_cleanup(bmark_t *bmark)
{
	if (bmark == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* nothing todo? */
	if (bmark->cleanup == NULL)
		return 0;

	return bmark->cleanup(bmark);
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


int bmark_subbmark_exec(subbmark_t *subbmark, int iteration)
{
	int ret = 0;

	/* init run */
	ret = bmark_init_subbmark(subbmark, iteration);
	if (ret < 0)
		return -1;

	/* run and measure */
	chrono_start(&subbmark->chrono);
	ret = bmark_run_subbmark(subbmark);
	if (ret < 0)
		return -1;
	chrono_stop(&subbmark->chrono);

	/* cleanup run */
	ret = bmark_cleanup_subbmark(subbmark);
	if (ret < 0)
		return -1;

	return 0;
}
