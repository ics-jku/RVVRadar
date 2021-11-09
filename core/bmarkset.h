/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef BMARKSET_H
#define BMARKSET_H

#include <stdbool.h>

#include <core/chrono.h>


/*
 * all functions have to return:
 * >=0 .. if everything was ok
 * <0 .. on error -> errno has to be set!
 * EXCEPTION: impl_postexec_fp_t (verify function):
 * 0 .. if everything was ok
 * >0 .. on data error (e.g. incorrect implementation results)
 * <0 .. on error -> errno has to be set!
 */
struct impl;
typedef int (*impl_init_fp_t)(struct impl *impl);
typedef int (*impl_preexec_fp_t)(struct impl *impl, int iteration, bool verify);
typedef int (*impl_exec_fp_t)(struct impl *impl, bool verify);
typedef int (*impl_postexec_fp_t)(struct impl *impl, bool verify);
typedef int (*impl_cleanup_fp_t)(struct impl *impl);

/* algorithm implementation */
typedef struct impl {
	char *name;				// name of the implementation
	unsigned int index;			// index in the list of implementations

	impl_init_fp_t init;			// called before iterations over implementations
	impl_preexec_fp_t preexec;		// called before each implementation execution
	impl_exec_fp_t exec;			// implementation function (measured)
	impl_postexec_fp_t postexec;		// called after each implementation execution
	impl_cleanup_fp_t cleanup;		// called after iterations over implementations

	struct bmark *bmark;			// parent bmark
	struct impl *next;			// next in the implementation list

	unsigned int runs;			// number of runs
	unsigned int fails;			// number of failed runs
	chrono_t chrono;			// chrono (including result statistics)

	void *data;				// optional private data for the implementation
} impl_t;


struct bmark;
typedef int (*bmark_preexec_fp_t)(struct bmark *bmark, int seed);
typedef int (*bmark_postexec_fp_t)(struct bmark *bmark);

typedef struct bmark {
	char *name;				// name of the bmark
	char *parastr;				// string containing parameters as string
	unsigned int index;			// index in bmark list

	// linked list of algorithm implementations
	impl_t *impls_head;
	impl_t *impls_tail;
	unsigned int impls_len;

	bmark_preexec_fp_t preexec;		// called before bmark
	bmark_postexec_fp_t postexec;		// called after bmark

	struct bmarkset *bmarkset;		// parent bmarkset
	struct bmark *next;			// next in bmark list

	void *data;				// optional data for the bmark
} bmark_t;


typedef struct bmarkset {
	char *name;

	// linked list of bmarks
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
 * create and add a new algorithm implementation
 * name will be duplicated and handled by bmark (e.g. heap allocated
 * parameters are valid)
 * returns NULL on error
 */
impl_t *bmark_add_impl(
	bmark_t *bmark,
	const char *name,
	impl_init_fp_t init,
	impl_preexec_fp_t preexec,
	impl_exec_fp_t exec,
	impl_postexec_fp_t postexec,
	impl_cleanup_fp_t cleanup,
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
