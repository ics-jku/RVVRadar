#include <string.h>
#include <limits.h>
#include <errno.h>

#include "chrono.h"


/* time difference in us */
static unsigned long long timespec_diff(const struct timespec start, const struct timespec end)
{
	struct timespec diff;

	if ((end.tv_nsec - start.tv_nsec) < 0) {
		diff.tv_sec = end.tv_sec - start.tv_sec - 1;
		diff.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
	} else {
		diff.tv_sec = end.tv_sec - start.tv_sec;
		diff.tv_nsec = end.tv_nsec - start.tv_nsec;
	}

	return diff.tv_sec * 1000000 + diff.tv_nsec / 1000;
}


static inline void chrono__start(struct timespec *start)
{
	clock_gettime(CLOCK_MONOTONIC, start);
}


static inline unsigned long long chrono__stop(const struct timespec start)
{
	struct timespec end;
	clock_gettime(CLOCK_MONOTONIC, &end);
	return timespec_diff(start, end);
}





/*
 * API
 */


int chrono_reset(chrono_t *chrono)
{
	if (chrono == NULL) {
		errno = EINVAL;
		return -1;
	}

	memset(chrono, 0, sizeof(chrono_t));
	chrono->tdmin = ULLONG_MAX;

	return 0;
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
	unsigned long long td = 0;

	if (chrono == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* stop chronometer */
	td = chrono__stop(chrono->tstart);

	/* update statistics */
	chrono->count++;
	chrono->tdlast = td;
	chrono->tdsum += td;
	if (td > chrono->tdmax)
		chrono->tdmax = td;
	if (td < chrono->tdmin)
		chrono->tdmin = td;
	chrono->tdavg = chrono->tdsum / chrono->count;

	return 0;
}


int chrono_print_csv(chrono_t *chrono, FILE *out)
{
	if (chrono == NULL || out == NULL) {
		errno = EINVAL;
		return -1;
	}

	return fprintf(out, "%i;%llu;%llu;%llu",
		       chrono->count,
		       chrono->tdmin,
		       chrono->tdmax,
		       chrono->tdavg);
}


int chrono_print_pretty(chrono_t *chrono, const char *indent, FILE *out)
{
	if (chrono == NULL || out == NULL) {
		errno = EINVAL;
		return -1;
	}

	return fprintf(out,
		       "%scount: %i\n"
		       "%smin:   %llu\n"
		       "%smax:   %llu\n"
		       "%savg:   %llu\n",
		       indent, chrono->count,
		       indent, chrono->tdmin,
		       indent, chrono->tdmax,
		       indent, chrono->tdavg);
}
