/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/random.h>

#include "bmarkset.h"
#include "bmark_memcpy.h"


/* TODO: implement programm parameters for this */
#define ENABLE_MEMCPY	1
#define ITERATIONS	100
#define LEN_START	128
#define LEN_END		1024*1024*16


int main(int argc, char **argv)
{
	int ret = 0;

	fprintf(stderr, "%s (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>\n", RVVBMARK_VERSION_STR);

	fprintf(stderr, "RISC-V support is ");
#if RVVBMARK_RV_SUPPORT == 1
	fprintf(stderr, "enabled\n");

	fprintf(stderr, "RISC-V RVV support is ");
#if RVVBMARK_RVV_SUPPORT == 1
	fprintf(stderr, "enabled\n");
#else /* RVVBMARK_RVV_SUPPORT */
	fprintf(stderr, "disabled\n");
#endif /* RVVBMARK_RVV_SUPPORT */

#else /* RVVBMARK_RV_SUPPORT */
	fprintf(stderr, "disabled\n");
#endif /* RVVBMARK_RV_SUPPORT */


	bmarkset_t *bmarkset = bmarkset_create("rvvbmark");
	if (bmarkset == NULL)
		return -1;

	/* 128 byte to 16MiB, doubling the size after every iteration */
	for (int len = LEN_START; len <= LEN_END; len <<= 1) {
#if ENABLE_MEMCPY
		if (bmark_memcpy_add(bmarkset, len) < 0) {
			ret = -1;
			goto __ret_bmarkset_destroy;
		}
#endif /* ENABLE_MEMCPY */
	}

	bmarkset_reset(bmarkset);
	bmarkset_run(bmarkset, 0, ITERATIONS, true);

	ret = 0;

__ret_bmarkset_destroy:
	bmarkset_destroy(bmarkset);
	exit(ret);
}
