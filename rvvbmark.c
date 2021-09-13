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


void runbmark(bmark_t *bmark, int seed, int iterations)
{
	bmark_call_preexec(bmark, seed);

	for (
		subbmark_t *s = bmark_get_first_subbmark(bmark);
		s != NULL;
		s = bmark_get_next_subbmark(s)
	)
		//subbmark_exec_iterations(s, iterations, true);

	bmark_call_postexec(bmark);
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

	printf("RISC-V support is ");
#if RVVBMARK_RV_SUPPORT == 1
	printf("enabled\n");

	printf("RISC-V RVV support is ");
#if RVVBMARK_RVV_SUPPORT == 1
	printf("enabled\n");
#else /* RVVBMARK_RVV_SUPPORT */
	printf("disabled\n");
#endif /* RVVBMARK_RVV_SUPPORT */

#else /* RVVBMARK_RV_SUPPORT */
	printf("disabled\n");
#endif /* RVVBMARK_RV_SUPPORT */

	memcpybmarkbmark();

	return 0;
}
