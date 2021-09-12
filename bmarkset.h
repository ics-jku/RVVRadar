#ifndef BMARKSET_H
#define BMARKSET_H

#include <stdbool.h>

#include "chrono.h"


struct subbmark;
typedef int (*bmark_init_subbmark_t)(struct subbmark *subbmark, int iteration);
typedef int (*bmark_run_subbmark_t)(struct subbmark *subbmark);
typedef int (*bmark_cleanup_subbmark_t)(struct subbmark *subbmark);


typedef struct subbmark {
	const char *name;			// name of the subbmark
	unsigned int index;			// index in subbmark list
	bool rv;				// is a RISC-V bmark
	bool rvv;				// is a RISC-V vector bmark

	bmark_init_subbmark_t init;		// called before subbmark
	bmark_run_subbmark_t run;		// subbmark function (measured)
	bmark_cleanup_subbmark_t cleanup;	// called after subbmark TODO: cleanup + check!

	struct bmark *bmark;			// parent bmark
	struct subbmark *next;			// next in subbmark list

	chrono_t chrono;			// chrono (including result statistics)

	void *data;				// optional data for the subbmark
} subbmark_t;


struct bmark;
typedef int (*bmark_init_bmark_t)(struct bmark *bmark, int seed);
typedef int (*bmark_cleanup_bmark_t)(struct bmark *bmark);

typedef struct bmark {
	const char *name;			// name of the bmark
	unsigned int index;			// index in bmark list

	// linked list of subbmarks
	struct subbmark *subbmarks_head;
	struct subbmark *subbmarks_tail;
	unsigned int subbmarks_len;

	bmark_init_bmark_t init;		// called before bmark
	bmark_cleanup_bmark_t cleanup;		// called after bmark

	struct bmarkset *bmarkset;		// parent bmarkset
	struct bmark *next;			// next in bmark list

	void *data;				// optional data for the bmark
} bmark_t;


typedef struct bmarkset {
	const char *name;

	// linked list of subbmarks
	struct bmark *bmarks_head;
	struct bmark *bmarks_tail;
	unsigned int bmarks_len;
} bmarkset_t;


/*
 * allocated and create a new bmarkset
 */
bmarkset_t *bmarkset_create(const char *name);


/*
 * reset all bmarks
 */
void bmarkset_reset(bmarkset_t *bmarkset);


/*
 * destroy the bmarkset
 */
void bmarkset_destroy(bmarkset_t *bmarkset);


/*
 * create a bmark
 * handling of optional given data (free) is handled by bmark!
 */
bmark_t *bmark_create(
	const char *name,
	bmark_init_bmark_t init,
	bmark_cleanup_bmark_t cleanup,
	unsigned int data_len);


/*
 * destroy the bmark
 * (data will also freed here)
 */
void bmark_destroy(bmark_t *bmark);


int bmarkset_add_bmark(bmarkset_t *bmarkset, bmark_t *bmark);


/*
 * allocated and create a new bmark
 * returns NULL on error
 */
int bmark_add_subbmark(
	bmark_t *bmark,
	const char *name, bool rv, bool rvv,
	bmark_init_subbmark_t init,
	bmark_run_subbmark_t run,
	bmark_cleanup_subbmark_t cleanup,
	unsigned int data_len);

int bmark_subbmark_exec(subbmark_t *subbmark, int iteration);

int bmark_init(bmark_t *bmark, int seed);
int bmark_cleanup(bmark_t *bmark);
subbmark_t *bmark_get_first_subbmark(bmark_t *bmark);
subbmark_t *bmark_get_next_subbmark(subbmark_t *subbmark);

#endif /* BMARKSET_H */
