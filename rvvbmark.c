/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <stdlib.h>
#include <getopt.h>

#include "bmarkset.h"
#include "bmark_memcpy.h"
#include "bmark_mac_16_32_32.h"
#include "bmark_mac_8_16_32.h"


/* ids of benchmarks */
#define BMARK_ID_MEMCPY			0
#define BMARK_ID_MAC_16_32_32		1
#define BMARK_ID_MAC_8_16_32		2

/* bmark mask helpers */
#define bmark_mask(id)			(1 << id)
#define bmark_enabled(mask, id)		((mask) & bmark_mask(id))

/* default parameters */
#define DEFAULT_QUIET			false
#define DEFAULT_RANDSEED		0
#define DEFAULT_ITERATIONS		10000
#define DEFAULT_LEN_START		32
#define DEFAULT_LEN_END			1024 * 1024 * 16
#define DEFAULT_BMARK_ENA_MASK	( \
				  bmark_mask(BMARK_ID_MEMCPY)		| \
				  bmark_mask(BMARK_ID_MAC_16_32_32)	| \
				  bmark_mask(BMARK_ID_MAC_8_16_32))


void print_version(void)
{
	fprintf(stderr,
		"%s (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>\n",
		RVVBMARK_VERSION_STR);

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
}


void print_usage(const char *name)
{
	fprintf(stderr,
		"Usage: %s [options]"
		"\n\n"
		"Options:\n"
		"  [--help|-h]\n"
		"     Print this help.\n"
		"\n"
		"  [--quiet|-q]\n"
		"     Prevent human readable output (progress and statistics).\n"
		"     (Default: %s)\n"
		"\n"
		"  [--randseed|-r <seed>]\n"
		"     Set random seed for benchmarks.\n"
		"     (Default: %u)\n"
		"\n"
		"  [--iterations|-i <#iterations>]\n"
		"     Number of iterations to run each (sub-)benchmark.\n"
		"     (Default: %u)\n"
		"\n"
		"  [--len_start|-s <#elements>]\n"
		"     Number of elements to start (sub-)benchmarks with.\n"
		"     (Will be doubled until len_end is reached).\n"
		"     (Default: %u)\n"
		"\n"
		"  [--len_end|-e <#elements>]\n"
		"     Number of elements to end benchmarks with.\n"
		"     (len_start will be doubled until len_end is reached).\n"
		"     (Default: %u)\n"
		"\n"
		"  [--bmarks_enabled|-b <bmark_mask>]\n"
		"     Bitmask of benchmarks to run (hexadecimal).\n"
		"       bit             bmark\n"
		"         0             memcpy\n"
		"         1             mac_16_32_32\n"
		"         2             mac_8_16_32\n"
		"     (Default: 0x%X)\n"
		"\n\n"
		"Output:\n"
		"  stdout: results as comma separated values (csv).\n"
		"          (including column headers)\n"
		"  stderr: human readable output (supressed if quiet was set)\n"
		"          and errors (independent of quiet).\n"
		"\n",
		name,
		DEFAULT_QUIET ? "true" : "false",
		DEFAULT_RANDSEED,
		DEFAULT_ITERATIONS,
		DEFAULT_LEN_START,
		DEFAULT_LEN_END,
		DEFAULT_BMARK_ENA_MASK);
}


int main(int argc, char **argv)
{
	int ret = 0;

	/* parameters */

	bool quiet = DEFAULT_QUIET;
	int randseed = DEFAULT_RANDSEED;
	unsigned int iterations = DEFAULT_ITERATIONS;
	unsigned int len_start = DEFAULT_LEN_START;
	unsigned int len_end = DEFAULT_LEN_END;
	unsigned int bmark_ena_mask = DEFAULT_BMARK_ENA_MASK;

	/* parameter parsing */

	int long_index, opt = 0;
	struct option long_options[] = {
		{"quiet",		no_argument,		0,	'q'	},
		{"randseed",		required_argument,	0,	'r'	},
		{"iterations",		required_argument,	0,	'i'	},
		{"len_start",		required_argument,	0,	's'	},
		{"len_end",		required_argument,	0,	'e'	},
		{"bmarks_enabled",	required_argument,	0,	'b'	},
		{"help",		no_argument,		0,	'h'	},
		{0,			0,			0,	0	}
	};

	while ((opt = getopt_long(argc, argv, "qr:i:s:e:b:h",
				  long_options, &long_index )) != -1) {
		int ret = -1;
		switch (opt) {
		case 'q':
			quiet = true;
			break;
		case 'r':
			randseed = atoi(optarg);
			break;
		case 'i':
			iterations = atoi(optarg);
			break;
		case 's':
			len_start = atoi(optarg);
			break;
		case 'e':
			len_end = atoi(optarg);
			break;
		case 'b':
			sscanf(optarg, "%X", &bmark_ena_mask);
			break;
		case 'h':
			ret = 0;
		default:
			print_usage(argv[0]);
			return ret;
		}
	}

	if (len_start == 0) {
		fprintf(stderr,
			"Error: Invalid argument: len_start=0 makes no sense!\n");
		print_usage(argv[0]);
		return -1;
	}

	if (len_end < len_start) {
		fprintf(stderr,
			"Error: Invalid argument: len_end(%i) < len_start(%i)!\n",
			len_end, len_start);
		print_usage(argv[0]);
		return -1;
	}

	if (!quiet) {
		fprintf(stderr, " + parameters:\n");
		fprintf(stderr, "   + randseed:       %u\n", randseed);
		fprintf(stderr, "   + iterations:     %u\n", iterations);
		fprintf(stderr, "   + len_start:      %u\n", len_start);
		fprintf(stderr, "   + len_end:        %u\n", len_end);
		fprintf(stderr, "   + bmarks_enabled: 0x%X\n", bmark_ena_mask);
	}

	/* buildup benchmarks */

	bmarkset_t *bmarkset = bmarkset_create("rvvbmark");
	if (bmarkset == NULL)
		return -1;

	/* start with len_start and double len until len_end */
	for (int len = len_start; len <= len_end; len <<= 1) {

		if (bmark_enabled(bmark_ena_mask, BMARK_ID_MEMCPY))
			if (bmark_memcpy_add(bmarkset, len) < 0) {
				perror("Error adding memcpy");
				ret = -1;
				goto __ret_bmarkset_destroy;
			}

		if (bmark_enabled(bmark_ena_mask, BMARK_ID_MAC_16_32_32))
			if (bmark_mac_16_32_32_add(bmarkset, len) < 0) {
				perror("Error adding mac_16_32_32");
				ret = -1;
				goto __ret_bmarkset_destroy;
			}

		if (bmark_enabled(bmark_ena_mask, BMARK_ID_MAC_8_16_32))
			if (bmark_mac_8_16_32_add(bmarkset, len) < 0) {
				perror("Error adding mac_8_16_32");
				ret = -1;
				goto __ret_bmarkset_destroy;
			}
	}

	/* execution */
	bmarkset_reset(bmarkset);

	if (bmarkset_run(bmarkset, randseed, iterations, !quiet) < 0) {
		perror("Error on run");
		ret = -1;
		goto __ret_bmarkset_destroy;
	}

	ret = 0;

	/* cleanup */

__ret_bmarkset_destroy:
	bmarkset_destroy(bmarkset);
	exit(ret);
}
