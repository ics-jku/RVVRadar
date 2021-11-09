/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef ALGSET_H
#define ALGSET_H

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

	struct alg *alg;			// parent algorithm the implementation belongs to
	struct impl *next;			// next in the implementation list

	unsigned int runs;			// number of runs
	unsigned int fails;			// number of failed runs
	chrono_t chrono;			// chrono (including result statistics)

	void *data;				// optional private data for the implementation
} impl_t;


struct alg;
typedef int (*alg_preexec_fp_t)(struct alg *alg, int seed);
typedef int (*alg_postexec_fp_t)(struct alg *alg);

typedef struct alg {
	char *name;				// name of the algorithm
	char *parastr;				// string containing parameters as string
	unsigned int index;			// index in algorithm list

	// linked list of algorithm implementations
	impl_t *impls_head;
	impl_t *impls_tail;
	unsigned int impls_len;

	alg_preexec_fp_t preexec;		// called before running the algorithm
	alg_postexec_fp_t postexec;		// called after running the algorithm

	struct algset *algset;			// parent algorithm set
	struct alg *next;			// next in algorithm list

	void *data;				// optional privated data for the algorithm
} alg_t;


typedef struct algset {
	char *name;

	// linked list of algorithms
	struct alg *algs_head;
	struct alg *algs_tail;
	unsigned int algs_len;
} algset_t;


/*
 * ALGORITHM
 */

/*
 * create an algorithm
 * handling of optional given data (free) is handled by alg!
 * name and parastr will be duplicated and handled by alg (e.g. heap
 * allocated parameters are valid)
 */
alg_t *alg_create(
	const char *name,
	const char *parastr,
	alg_preexec_fp_t preexec,
	alg_postexec_fp_t postexec,
	unsigned int data_len);


/*
 * destroy the algorithm
 * (data and linked implementations will be also destroyed here)
 */
void alg_destroy(alg_t *alg);


/*
 * create and add a new algorithm implementation
 * name will be duplicated and handled by alg (e.g. heap allocated
 * parameters are valid)
 * returns NULL on error
 */
impl_t *alg_add_impl(
	alg_t *alg,
	const char *name,
	impl_init_fp_t init,
	impl_preexec_fp_t preexec,
	impl_exec_fp_t exec,
	impl_postexec_fp_t postexec,
	impl_cleanup_fp_t cleanup,
	unsigned int data_len);


/*
 * ALGSET
 */

/*
 * allocated and create a new set of algorithms
 * name will be duplicated and handled by algset (e.g. heap allocated
 * parameters are valid)
 */
algset_t *algset_create(const char *name);


/*
 * destroy the algorithm set
 * (linked algorithms will be also destroyed here)
 */
void algset_destroy(algset_t *algset);


/*
 * add a new algorithm to the set
 */
int algset_add_alg(algset_t *algset, alg_t *alg);


/*
 * reset the state of the whole set
 * (collected data, measurements, ...)
 */
void algset_reset(algset_t *algset);


/*
 * run the algorithm set
 */
int algset_run(algset_t *alg, int seed, int iterations, bool verify, bool verbose);


#endif /* ALGSET_H */
