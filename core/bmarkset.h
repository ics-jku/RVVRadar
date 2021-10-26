/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef BMARKSET_H
#define BMARKSET_H

#include <stdbool.h>

#include <core/chrono.h>


struct subbmark;
/*
 * all functions have to return:
 * >=0 .. if everything was ok
 * <0 .. on error -> errno has to be set!
 * EXCEPTION: subbmark_postexec_fp_t (verify function):
 * 0 .. if everything was ok
 * >0 .. on data error (e.g. incorrect benchmark results)
 * <0 .. on error -> errno has to be set!
 */
typedef int (*subbmark_init_fp_t)(struct subbmark *subbmark);
typedef int (*subbmark_preexec_fp_t)(struct subbmark *subbmark, int iteration, bool verify);
typedef int (*subbmark_exec_fp_t)(struct subbmark *subbmark, bool verify);
typedef int (*subbmark_postexec_fp_t)(struct subbmark *subbmark, bool verify);
typedef int (*subbmark_cleanup_fp_t)(struct subbmark *subbmark);


typedef struct subbmark {
	char *name;				// name of the subbmark
	unsigned int index;			// index in subbmark list

	subbmark_init_fp_t init;		// called before iteration over subbmarks
	subbmark_preexec_fp_t preexec;		// called before each subbmark
	subbmark_exec_fp_t exec;		// subbmark function (measured)
	subbmark_postexec_fp_t postexec;	// called after each subbmark
	subbmark_cleanup_fp_t cleanup;		// called after iteration over subbmarks

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
	char *name;				// name of the bmark
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
	char *name;

	// linked list of subbmarks
	struct bmark *bmarks_head;
	struct bmark *bmarks_tail;
	unsigned int bmarks_len;
} bmarkset_t;


/*
 * BMARK
 */

/*
 * create a bmark
 * handling of optional given data (free) is handled by bmark!
 * name and parastr will be duplicated and handled by bmark (e.g. heap
 * allocated parameters are valid)
 */
bmark_t *bmark_create(
	const char *name,
	const char *parastr,
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
 * name will be duplicated and handled by bmark (e.g. heap allocated
 * parameters are valid)
 * returns NULL on error
 */
subbmark_t *bmark_add_subbmark(
	bmark_t *bmark,
	const char *name,
	subbmark_init_fp_t init,
	subbmark_preexec_fp_t preexec,
	subbmark_exec_fp_t exec,
	subbmark_postexec_fp_t postexec,
	subbmark_cleanup_fp_t cleanup,
	unsigned int data_len);


/*
 * BMARKSET
 */

/*
 * allocated and create a new bmarkset
 * name will be duplicated and handled by bmark (e.g. heap allocated
 * parameters are valid)
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


/*
 * run the benchmark set
 */
int bmarkset_run(bmarkset_t *bmark, int seed, int iterations, bool verify, bool verbose);


#endif /* BMARKSET_H */
