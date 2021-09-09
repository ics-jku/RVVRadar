/**
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <sys/random.h>
#include <time.h>

extern void int_memcpy(void *src, void *dest, unsigned int len);
#if RVVBENCH_RVV_SUPPORT == 1
extern int vect_memcpy(void *src, void *dest, unsigned int len);
extern void vect_memcpy32(void *src, void *dest, unsigned int len);
#endif

/* time difference in us */
unsigned long long timespec_diff(const struct timespec start, const struct timespec end)
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

void timer_start(struct timespec *start)
{
	clock_gettime(CLOCK_MONOTONIC, start);
}

unsigned long long timer_stop(const struct timespec start)
{
	struct timespec end;
	clock_gettime(CLOCK_MONOTONIC, &end);
	return timespec_diff(start, end);
}


void dump_field(char *f, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		printf("0x%.2X ", f[i]);
		if ((i + 1) % 16 == 0)
			printf("\n");
	}
	printf("\n");
}

unsigned long long memcpytest(int type, void *dest, void *src, unsigned int len)
{
	int ret;
	unsigned long long time;
	struct timespec start;

	/* init */
	memset(dest, 0, len);

	/* copy */
	timer_start(&start);
	if (type == 0)
		memcpy(dest, src, len);
	else
#if RVVBENCH_RVV_SUPPORT == 1
		vect_memcpy(dest, src, len);
#else
		/* error */
		return 0;
#endif
//	time = timer_stop(start);

	/* check */
	ret = memcmp(dest, src, len);
	if (!ret)
		memset(dest, 0, len);
	time = timer_stop(start);

	if (!ret)
		return time;

	/* error */
	return 0;
}

int main(int argc, char **argv)
{
	int type = 1;
	int iterations = 1000;
	int len = 1024 * 1024 * 10 + 1;
	int ret = 0;
	int i;
	char *src;
	char *dest;
	unsigned long long time = 0;
	unsigned long long time_min = ULLONG_MAX;
	unsigned long long time_max = 0;
	unsigned long long time_sum = 0;

	printf("%s (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>\n", RVVBENCH_VERSION_STR);
	printf("RISC-V RVV support is ");
#if RVVBENCH_RVV_SUPPORT == 1
	printf("enabled\n");
#else
	printf("disabled\n");
#endif

	/* alloc */
	src = malloc(len);
	if (src == NULL) {
		perror("allocating src");
		ret = -1;
		goto __err_src;
	}

	dest = malloc(len);
	if (dest == NULL) {
		perror("allocating dest");
		ret = -1;
		goto __err_dest;
	}

	/* init */
	if (getrandom(src, len, 0) != len) {
		perror("allocating dest");
		ret = -1;
		goto __err_random;
	}

	/* run */
	for (i = 0; i < iterations; i++) {
		printf("iteration %i: ", i);
		time = memcpytest(type, dest, src, len);
		if (!time) {
			printf("FAIL!\n");
			dump_field(src, len);
			dump_field(dest, len);
			continue;
		}

		printf("OK (%lluus)\n", time);
		time_sum += time;
		if (time > time_max)
			time_max = time;
		if (time < time_min)
			time_min = time;
	}

	printf("done. (%lluus)\n", time_sum);

	printf("Summary (type;len;#iterations;min;max;avg)\n");
	printf("%i;%i;%i;%llu;%llu;%llu\n", type, len, iterations, time_min, time_max, time_sum / iterations);
	ret = 0;

__err_random:
	free(dest);
__err_dest:
	free(src);
__err_src:
	return ret;
}
