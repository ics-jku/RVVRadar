/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <errno.h>

#include "chrono.h"


/* time difference in ns */
static long long timespec_diff(const struct timespec start, const struct timespec end)
{
	struct timespec diff;

	if ((end.tv_nsec - start.tv_nsec) < 0) {
		diff.tv_sec = end.tv_sec - start.tv_sec - 1;
		diff.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
	} else {
		diff.tv_sec = end.tv_sec - start.tv_sec;
		diff.tv_nsec = end.tv_nsec - start.tv_nsec;
	}

	return diff.tv_sec * 1000000000 + diff.tv_nsec;
}


static inline void chrono__start(struct timespec *start)
{
	clock_gettime(CLOCK_MONOTONIC, start);
}


static inline long long chrono__stop(const struct timespec start)
{
	struct timespec end;
	clock_gettime(CLOCK_MONOTONIC, &end);
	return timespec_diff(start, end);
}


/*
 * update size of histogram buckets in chrono structure
 *
 * ensures non-overlapping buckets with equal, whole-number sizes
 */
static void chrono_hist_update_bucketsize(chrono_t *chrono)
{
	chrono->hist_bucketsize =
		((chrono->tdmax - chrono->tdmin) + CHRONO_HIST_BUCKETS) / CHRONO_HIST_BUCKETS;
}


/*
 * get bucket index from value
 * (chrono_hist_update_bucketsize needed once before)
 */
static inline unsigned int chrono_hist_bucket_idx_by_value(chrono_t *chrono, long long value)
{
	return (value - chrono->tdmin) / chrono->hist_bucketsize;
}


/*
 * get start of bucket by index
 * (chrono_hist_update_bucketsize needed once before)
 */
static inline long long chrono_hist_get_bucket_start_by_idx(chrono_t *chrono, unsigned int bucket_idx)
{
	return chrono->tdmin + chrono->hist_bucketsize * bucket_idx;
}


/*
 * get start of bucket by index
 * (chrono_hist_update_bucketsize needed once before)
 */
static inline long long chrono_hist_get_bucket_end_by_idx(chrono_t *chrono, unsigned int bucket_idx)
{
	return chrono_hist_get_bucket_start_by_idx(chrono, bucket_idx + 1) - 1;
}


/*
 * comparator function for qsort
 */
static int chrono_qsort_td_compare(const void * a, const void * b)
{
	return ( *(long long*)a - * (long long*)b );
}


/* update statistics - histogram buckets */
static void chrono_update_statistics_hist_buckets(chrono_t *chrono)
{
	chrono_hist_update_bucketsize(chrono);

	memset(chrono->hist_buckets, 0, CHRONO_HIST_BUCKETS * sizeof(chrono->hist_buckets[0]));
	for (int i = 0; i < chrono->nmeasure; i++) {
		int bidx = chrono_hist_bucket_idx_by_value(chrono, chrono->tdlist[i]);
		/* paranoia checks - TODO: remove? */
		if (bidx < 0) {
			fprintf(stderr, "INTERNAL ERROR: histogram idx %i < 0 -> ABORT!\n", bidx);
			exit(1);
		} else if (bidx >= CHRONO_HIST_BUCKETS) {
			fprintf(stderr, "INTERNAL ERROR: histogram idx %i >= %i -> ABORT!\n", bidx, CHRONO_HIST_BUCKETS);
			exit(1);
		} else
			chrono->hist_buckets[bidx]++;
	}

	/* paranoia checks - TODO: remove? */
	long long hist_start = chrono_hist_get_bucket_start_by_idx(chrono, 0);
	long long hist_end = chrono_hist_get_bucket_end_by_idx(chrono, CHRONO_HIST_BUCKETS - 1);
	if (hist_start < chrono->tdmin) {
		fprintf(stderr, "INTERNAL ERROR: minimum below histogram start %lli < %lli < 0 -> ABORT!\n",
			hist_start, chrono->tdmin);
		exit(1);
	}
	if (chrono->tdmax > hist_end) {
		fprintf(stderr, "INTERNAL ERROR: maximum above histogram end %lli < %lli < 0 -> ABORT!\n",
			hist_end, chrono->tdmax);
		exit(1);
	}
}


/* update statistics */
static void chrono_update_statistics(chrono_t *chrono)
{
	/* is already updated? -> nothing to do */
	if (chrono->nmeasure == chrono->nmeasure_on_last_update)
		return;

	/* save update point */
	chrono->nmeasure_on_last_update = chrono->nmeasure;

	/* update variance and standard deviation */
	long long varsum = 0;
	for (int i = 0; i < chrono->nmeasure; i++) {
		long long diff = chrono->tdlist[i] - chrono->tdavg;
		varsum += diff * diff;
	}
	chrono->tdvar = varsum / chrono->nmeasure;
	chrono->tdstdev = sqrt(chrono->tdvar);

	/* update median */
	qsort(chrono->tdlist, chrono->nmeasure, sizeof(long long), chrono_qsort_td_compare);
	chrono->tdmedian = chrono->tdlist[chrono->nmeasure / 2];
	if ((chrono->nmeasure & 1) == 0) {
		/* even number of elements -> average of middle two elements */
		chrono->tdmedian = (chrono->tdmedian + chrono->tdlist[chrono->nmeasure / 2 - 1]) / 2;
	}

	/* update histogram buckets */
	chrono_update_statistics_hist_buckets(chrono);
}



/*
 * API
 */

int chrono_init(chrono_t *chrono)
{
	if (chrono == NULL) {
		errno = EINVAL;
		return -1;
	}

	memset(chrono, 0, sizeof(chrono_t));
	chrono->tdmin = LLONG_MAX;

	chrono->max_nmeasure = CHRONO_MAX_MEASUREMENTS;
	chrono->tdlist = calloc(chrono->max_nmeasure, sizeof(long long));
	if (chrono->tdlist == NULL)
		return -1;

	return 0;
}


void chrono_cleanup(chrono_t *chrono)
{
	if (chrono == NULL)
		return;
	free(chrono->tdlist);
}


int chrono_start(chrono_t *chrono)
{
	if (chrono == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* start chronometer */
	chrono__start(&chrono->tstart);

	return 0;
}


int chrono_stop(chrono_t *chrono)
{
	long long td = 0;

	if (chrono == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* stop chronometer */
	td = chrono__stop(chrono->tstart);

	/* abort, if no space */
	if (chrono->nmeasure > chrono->max_nmeasure) {
		errno = ENOMEM;
		return -1;
	}

	chrono->tdlist[chrono->nmeasure] = td;

	/* update live statistics */
	chrono->nmeasure++;
	chrono->tdlast = td;
	chrono->tdsum += td;
	if (td > chrono->tdmax)
		chrono->tdmax = td;
	if (td < chrono->tdmin)
		chrono->tdmin = td;
	chrono->tdavg = chrono->tdsum / chrono->nmeasure;

	return 0;
}


int chrono_print_csv_head(FILE *out)
{
	int ret = 0;

	if (out == NULL) {
		errno = EINVAL;
		return -1;
	}

	ret = fprintf(out, "nmeasure;tdmin [ns];tdmax [ns];tdavg [ns];tdvar [ns];tdstdev [ns];tdmedian [ns];nbuckets");
	if (ret < 0)
		return ret;

	for (int i = 0; i < CHRONO_HIST_BUCKETS; i++) {
		ret = fprintf(out, ";hist_bucket[%i]", i);
		if (ret < 0)
			return ret;
	}

	return 0;
}


int chrono_print_csv(chrono_t *chrono, FILE *out)
{
	int ret = 0;
	if (chrono == NULL || out == NULL) {
		errno = EINVAL;
		return -1;
	}

	chrono_update_statistics(chrono);

	ret = fprintf(out, "%i;%lli;%lli;%lli;%lli;%lli;%lli;%i",
		      chrono->nmeasure,
		      chrono->tdmin,
		      chrono->tdmax,
		      chrono->tdavg,
		      chrono->tdvar,
		      chrono->tdstdev,
		      chrono->tdmedian,
		      CHRONO_HIST_BUCKETS);
	if (ret < 0)
		return ret;


	for (int i = 0; i < CHRONO_HIST_BUCKETS; i++) {
		ret = fprintf(out, ";%u", chrono->hist_buckets[i]);
		if (ret < 0)
			return ret;
	}

	return 0;
}


int chrono_print_pretty(chrono_t *chrono, const char *indent, FILE *out)
{
	int ret = 0;

	if (chrono == NULL || out == NULL) {
		errno = EINVAL;
		return -1;
	}

	chrono_update_statistics(chrono);

	ret = fprintf(out,
		      "%snmeasure:    %u\n"
		      "%smin [ns]:    %lli\n"
		      "%smax [ns]:    %lli\n"
		      "%savg [ns]:    %lli\n"
		      "%svar [ns]:    %lli\n"
		      "%sstdev [ns]:  %lli\n"
		      "%smedian [ns]: %lli\n",
		      indent, chrono->nmeasure,
		      indent, chrono->tdmin,
		      indent, chrono->tdmax,
		      indent, chrono->tdavg,
		      indent, chrono->tdvar,
		      indent, chrono->tdstdev,
		      indent, chrono->tdmedian);
	if (ret < 0)
		return ret;

	for (int i = 0; i < CHRONO_HIST_BUCKETS; i++) {
		ret = fprintf(out,
			      "%shist[%.3i]:   %5.1u [%lli, %lli]\n",
			      indent, i, chrono->hist_buckets[i],
			      chrono_hist_get_bucket_start_by_idx(chrono, i),
			      chrono_hist_get_bucket_end_by_idx(chrono, i));
		if (ret < 0)
			return ret;
	}

	return 0;
}
