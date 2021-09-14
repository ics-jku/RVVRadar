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


/* update statistics part 2 */
static void chrono_update_statistics(chrono_t *chrono)
{
	/* is already updated? -> nothing to do */
	if (chrono->nmeasure == chrono->nmeasure_on_last_update)
		return;

	chrono->nmeasure_on_last_update = chrono->nmeasure;

	/* update variance and standard deviation */
	chrono->tdvar = 0;
	for (int i = 0; i < chrono->nmeasure; i++)
		chrono->tdvar += (chrono->tdlist[i] - chrono->tdvar) ^ 2;
	chrono->tdstdev = sqrt(chrono->tdvar);
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

	/* update statistics part 1 */
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
	if (out == NULL) {
		errno = EINVAL;
		return -1;
	}

	return fprintf(out, "nmeasure;tdmin [ns];tdmax [ns];tdavg [ns];tdvar [ns];tdstdev [ns]");
}


int chrono_print_csv(chrono_t *chrono, FILE *out)
{
	if (chrono == NULL || out == NULL) {
		errno = EINVAL;
		return -1;
	}

	chrono_update_statistics(chrono);

	return fprintf(out, "%i;%lli;%lli;%lli;%lli;%lli",
		       chrono->nmeasure,
		       chrono->tdmin,
		       chrono->tdmax,
		       chrono->tdavg,
		       chrono->tdvar,
		       chrono->tdstdev);
}


int chrono_print_pretty(chrono_t *chrono, const char *indent, FILE *out)
{
	if (chrono == NULL || out == NULL) {
		errno = EINVAL;
		return -1;
	}

	chrono_update_statistics(chrono);

	return fprintf(out,
		       "%snmeasure:   %u\n"
		       "%smin [ns]:   %lli\n"
		       "%smax [ns]:   %lli\n"
		       "%savg [ns]:   %lli\n"
		       "%svar [ns]:   %lli\n"
		       "%sstdev [ns]: %lli\n",
		       indent, chrono->nmeasure,
		       indent, chrono->tdmin,
		       indent, chrono->tdmax,
		       indent, chrono->tdavg,
		       indent, chrono->tdvar,
		       indent, chrono->tdstdev);
}
