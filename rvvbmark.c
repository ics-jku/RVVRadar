/**
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/random.h>

#include "bmarkset.h"
#include "bmark_memcpy.h"




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

void run_subbmark_iterations(subbmark_t *subbmark, int iterations)
{
	for (int i = 0; i < iterations; i++) {
		printf("iteration %i: ", i);
		if (bmark_subbmark_exec(subbmark, i) < 0) {
			printf("FAIL!\n");
			//dump_field(src, len);
			//dump_field(dest, len);
			continue;
		}

		printf("OK (%lluus)\n", subbmark->chrono.tdlast);
	}
	printf("done. (%lluus)\n", subbmark->chrono.tdsum);

	printf("Summary (type;len;#iterations;min;max;avg)\n");
	printf("%s;%s;", subbmark->bmark->name, subbmark->name);
	chrono_fprintf_csv(&subbmark->chrono, stdout);

	chrono_fprintf_pretty(&subbmark->chrono, " + ", stdout);
}

void runbmark(bmark_t *bmark, int seed, int iterations)
{
	bmark_init(bmark, seed);

	for (
		subbmark_t *s = bmark_get_first_subbmark(bmark);
		s != NULL;
		s = bmark_get_next_subbmark(s)
	)
		run_subbmark_iterations(s, iterations);

	bmark_cleanup(bmark);
}

int memcpybmarkbmark()
{
	int ret = 0;

	bmarkset_t *bmarkset = bmarkset_create("rvvbmark");
	if (bmarkset == NULL)
		return -1;

	if (bmark_memcpy_add(bmarkset, 1024 * 1024 * 10 + 1) < 0) {
		goto __ret_bmarkset_destroy;
		return -1;
	}

	bmarkset_reset(bmarkset);

	runbmark(bmarkset->bmarks_head, 0, 10);

	ret = 0;

__ret_bmarkset_destroy:
	bmarkset_destroy(bmarkset);
	return ret;
}




int main(int argc, char **argv)
{
	printf("%s (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>\n", RVVBMARK_VERSION_STR);
	printf("RISC-V RVV support is ");
#if RVVBMARK_RVV_SUPPORT == 1
	printf("enabled\n");
#else
	printf("disabled\n");
#endif


	memcpybmarkbmark();

	return 0;
}
