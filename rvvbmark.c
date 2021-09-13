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

int memcpybmarkbmark()
{
	int ret = 0;

	bmarkset_t *bmarkset = bmarkset_create("rvvbmark");
	if (bmarkset == NULL)
		return -1;

	/* 128 byte to 16MiB, doubling the size after every iteration */
	for (int len = 128; len <= 1024 * 1024 * 16; len <<= 1)
		if (bmark_memcpy_add(bmarkset, len) < 0) {
			goto __ret_bmarkset_destroy;
			return -1;
		}

	bmarkset_reset(bmarkset);

	bmarkset_run(bmarkset, 0, 1000, true);

	ret = 0;

__ret_bmarkset_destroy:
	bmarkset_destroy(bmarkset);
	return ret;
}




int main(int argc, char **argv)
{
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

	memcpybmarkbmark();

	return 0;
}
