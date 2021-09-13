#ifndef BMARKSET_H
#define BMARKSET_H

#include <stdbool.h>

#include "chrono.h"


struct subbmark;
typedef int (*subbmark_preexec_fp_t)(struct subbmark *subbmark, int iteration);
typedef int (*subbmark_exec_fp_t)(struct subbmark *subbmark);
typedef int (*subbmark_postexec_fp_t)(struct subbmark *subbmark);


typedef struct subbmark {
	const char *name;			// name of the subbmark
	unsigned int index;			// index in subbmark list
	bool rv;				// is a RISC-V bmark
	bool rvv;				// is a RISC-V vector bmark

	subbmark_preexec_fp_t preexec;		// called before subbmark
	subbmark_exec_fp_t exec;		// subbmark function (measured)
	subbmark_postexec_fp_t postexec;	// called after subbmark

	struct bmark *bmark;			// parent bmark
	struct subbmark *next;			// next in subbmark list

	unsigned int runs;			// number of runs
	unsigned int fails;			// number of failed runs
	chrono_t chrono;			// chrono (including result statistics)

	void *data;				// optional data for the subbmark
} subbmark_t;


struct bmark;
typedef int (*bmark_preexec_fp_t)(struct bmark *bmark, int seed);
typedef int (*bmark_postexec_fp_t)(struct bmark *bmark);

typedef struct bmark {
	const char *name;			// name of the bmark
	char *parastr;				// string containing parameters as string
	unsigned int index;			// index in bmark list

	// linked list of subbmarks
	struct subbmark *subbmarks_head;
	struct subbmark *subbmarks_tail;
	unsigned int subbmarks_len;

	bmark_preexec_fp_t preexec;		// called before bmark
	bmark_postexec_fp_t postexec;		// called after bmark

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
 * SUBBMARK
 */

int subbmark_exec(subbmark_t *subbmark, int iteration);


/*
 * BMARK
 */

/*
 * create a bmark
 * handling of optional given data (free) is handled by bmark!
 * parastr will be duplicated and handled by bmark -> heap allocated paramers are valid!
 */
bmark_t *bmark_create(
	const char *name,
	char *parastr,
	bmark_preexec_fp_t preexec,
	bmark_postexec_fp_t postexec,
	unsigned int data_len);


/*
 * destroy the bmark
 * (data will also freed here)
 */
void bmark_destroy(bmark_t *bmark);


/*
 * allocated and create a new bmark
 * returns NULL on error
 */
int bmark_add_subbmark(
	bmark_t *bmark,
	const char *name, bool rv, bool rvv,
	subbmark_preexec_fp_t preexec,
	subbmark_exec_fp_t exec,
	subbmark_postexec_fp_t postexec,
	unsigned int data_len);


int bmark_call_preexec(bmark_t *bmark, int seed);
int bmark_call_postexec(bmark_t *bmark);

subbmark_t *bmark_get_first_subbmark(bmark_t *bmark);
subbmark_t *bmark_get_next_subbmark(subbmark_t *subbmark);


/*
 * BMARKSET
 */

/*
 * allocated and create a new bmarkset
 */
bmarkset_t *bmarkset_create(const char *name);


/*
 * destroy the bmarkset
 */
void bmarkset_destroy(bmarkset_t *bmarkset);


/*
 * add a new bmark to set
 */
int bmarkset_add_bmark(bmarkset_t *bmarkset, bmark_t *bmark);


/*
 * reset all bmarks
 */
void bmarkset_reset(bmarkset_t *bmarkset);


#endif /* BMARKSET_H */
