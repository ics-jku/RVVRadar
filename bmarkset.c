#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include "bmarkset.h"


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


int subbmark_exec(subbmark_t *subbmark, int iteration)
{
	int ret = 0;

	ret = subbmark_call_preexec(subbmark, iteration);
	if (ret < 0)
		return -1;

	/* exec and measure */
	chrono_start(&subbmark->chrono);
	ret = subbmark_call_exec(subbmark);
	if (ret < 0)
		return -1;
	chrono_stop(&subbmark->chrono);

	ret = subbmark_call_postexec(subbmark);
	if (ret < 0)
		return -1;

	return 0;
}


/*
 * BMARK
 */

bmark_t *bmark_create(
	const char *name,
	bmark_preexec_fp_t preexec,
	bmark_postexec_fp_t postexec,
	unsigned int data_len)
{
	bmark_t *bmark = calloc(1, sizeof(bmark_t));
	if (bmark == NULL)
		return NULL;

	bmark->name = name;
	bmark->preexec = preexec;
	bmark->postexec = postexec;

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


int bmark_call_postexec(bmark_t *bmark)
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
