/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <stdlib.h>
#include <getopt.h>

#include <core/rvv_helpers.h>
#include <core/algset.h>

#include <algorithms/memcpy/alg.h>
#include <algorithms/mac_16_32_32/alg.h>
#include <algorithms/mac_8_16_32/alg.h>
#include <algorithms/png_filters/alg.h>


/* ids of algorithms */
#define ALG_ID_MEMCPY			0
#define ALG_ID_MAC_16_32_32		1
#define ALG_ID_MAC_8_16_32		2
#define ALG_ID_PNG_FILTER_UP3		3
#define ALG_ID_PNG_FILTER_UP4		4
#define ALG_ID_PNG_FILTER_SUB3		5
#define ALG_ID_PNG_FILTER_SUB4		6
#define ALG_ID_PNG_FILTER_AVG3		7
#define ALG_ID_PNG_FILTER_AVG4		8
#define ALG_ID_PNG_FILTER_PAETH3	9
#define ALG_ID_PNG_FILTER_PAETH4	10

/* alg mask helpers */
#define alg_mask(id)			(1 << id)
#define alg_enabled(mask, id)		((mask) & alg_mask(id))

/* default parameters */
#define DEFAULT_QUIET			false
#define DEFAULT_VERIFY			false
#define DEFAULT_RANDSEED		0


void print_version(void)
{
	fprintf(stderr,
		"%s (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>\n",
		RVVRADAR_VERSION_STR);

	fprintf(stderr, "RISC-V support is ");
#if RVVRADAR_RV_SUPPORT
	fprintf(stderr, "enabled\n");

	fprintf(stderr, "RISC-V RVV support is ");
#if RVVRADAR_RVV_SUPPORT == RVVRADAR_RVV_SUPPORT_NO
	fprintf(stderr, "disabled\n");
#elif RVVRADAR_RVV_SUPPORT == RVVRADAR_RVV_SUPPORT_VER_07
	fprintf(stderr, "enabled (v0.7)\n");
#elif RVVRADAR_RVV_SUPPORT == RVVRADAR_RVV_SUPPORT_VER_08
	fprintf(stderr, "enabled (v0.8)\n");
#elif RVVRADAR_RVV_SUPPORT == RVVRADAR_RVV_SUPPORT_VER_09_10_100
	fprintf(stderr, "enabled (v0.9/v0.10/v1.0)\n");
#endif /* RVVRADAR_RVV_SUPPORT */

#else /* RVVRADAR_RV_SUPPORT */
	fprintf(stderr, "disabled\n");
#endif /* RVVRADAR_RV_SUPPORT */
}


void print_usage(const char *name)
{
	print_version();
	fprintf(stderr, "\n");
	fprintf(stderr,
		"Usage: %s options"
		"\n\n"
		"Options:\n"
		"  --algs_enabled|-a <algs_mask>\n"
		"     Bitmask of algorithms to run (hexadecimal).\n"
		"       bit             algorithm\n"
		"         0             memcpy\n"
		"         1             mac_16_32_32 (32bit += 16bit * 16bit)\n"
		"         2             mac_8_16_32  (32bit = 16bit + 8bit * 8bit)\n"
		"         3             png_filter_up3\n"
		"         4             png_filter_up4\n"
		"         5             png_filter_sub3\n"
		"         6             png_filter_sub4\n"
		"         7             png_filter_avg3\n"
		"         8             png_filter_avg4\n"
		"         9             png_filter_paeth3\n"
		"        10             png_filter_paeth4\n"
		"\n"
		"  --iterations|-i <#iterations>\n"
		"     Number of iterations to run each algorithm implementation.\n"
		"\n"
		"  --len_start|-s <#elements>\n"
		"     Initial number of elements to run algorithm implementations\n"
		"     with.\n"
		"     (Will be doubled until len_end is reached).\n"
		"\n"
		"  [--len_end|-e <#elements>]\n"
		"     Final number of elements to run algorithm implementations\n"
		"     with.\n"
		"     (len_start will be doubled until len_end is reached).\n"
		"     (Default: value given with --len_start)\n"
		"\n"
		"  [--randseed|-r <seed>]\n"
		"     Set random seed for test data.\n"
		"     (Default: %u)\n"
		"\n"
		"  [--verify|-v]\n"
		"     Enable verification of algorithm implementation results.\n"
		"     Enable this, if you want to make sure, that calculations\n"
		"     are correct (e.g. while development, functional tests, ...).\n"
		"     Leave it disabled if you want to execute with as little\n"
		"     interference (caches, ...) as possible.\n"
		"     (Default: %s)\n"
		"\n"
		"  [--quiet|-q]\n"
		"     Prevent human readable output (progress and statistics).\n"
		"     (Default: %s)\n"
		"\n"
		"  [--help|-h]\n"
		"     Print this help.\n"
		"\n\n"
		"Output:\n"
		"  stdout: results as comma separated values (csv).\n"
		"          (including column headers)\n"
		"  stderr: human readable output (supressed if quiet was set)\n"
		"          and errors (independent of quiet).\n"
		"\n",
		name,
		DEFAULT_RANDSEED,
		DEFAULT_VERIFY ? "true" : "false",
		DEFAULT_QUIET ? "true" : "false");
}


int main(int argc, char **argv)
{
	int ret = 0;

	/* parameters */

	bool quiet = DEFAULT_QUIET;
	int randseed = DEFAULT_RANDSEED;
	bool verify = DEFAULT_VERIFY;
	unsigned int iterations = 0;
	unsigned int len_start = 0;
	unsigned int alg_ena_mask = 0;
	unsigned int len_end = 0;

	/* parameter parsing */

	int long_index, opt = 0;
	struct option long_options[] = {
		{"quiet",		no_argument,		0,	'q'	},
		{"randseed",		required_argument,	0,	'r'	},
		{"verify",		no_argument,		0,	'v'	},
		{"iterations",		required_argument,	0,	'i'	},
		{"len_start",		required_argument,	0,	's'	},
		{"len_end",		required_argument,	0,	'e'	},
		{"algs_enabled",	required_argument,	0,	'a'	},
		{"help",		no_argument,		0,	'h'	},
		{0,			0,			0,	0	}
	};

	while ((opt = getopt_long(argc, argv, "qr:vi:s:e:a:h",
				  long_options, &long_index )) != -1) {
		int ret = -1;
		switch (opt) {
		case 'q':
			quiet = true;
			break;
		case 'r':
			randseed = atoi(optarg);
			break;
		case 'v':
			verify = true;
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
		case 'a':
			sscanf(optarg, "%X", &alg_ena_mask);
			break;
		case 'h':
			ret = 0;
		default:
			print_usage(argv[0]);
			return ret;
		}
	}

	if (alg_ena_mask == 0) {
		fprintf(stderr,
			"Error: Missing, or invalid argument \"--algs_enabled\"!\n");
		print_usage(argv[0]);
		return -1;
	}

	if (iterations == 0) {
		fprintf(stderr,
			"Error: Missing, or invalid argument \"--iterations\"!\n");
		print_usage(argv[0]);
		return -1;
	}

	if (len_start == 0) {
		fprintf(stderr,
			"Error: Missing, or invalid \"--len_start\"!\n");
		print_usage(argv[0]);
		return -1;
	}
	if (len_end == 0)
		len_end = len_start;

	if (len_end < len_start) {
		fprintf(stderr,
			"Error: Invalid argument: len_end(%i) < len_start(%i)!\n",
			len_end, len_start);
		print_usage(argv[0]);
		return -1;
	}

	if (!quiet) {
		print_version();
		fprintf(stderr, "\n");
		fprintf(stderr, " + parameters:\n");
		fprintf(stderr, "   + randseed:       %u\n", randseed);
		fprintf(stderr, "   + verify:         %s\n", verify ? "true" : "false");
		fprintf(stderr, "   + iterations:     %u\n", iterations);
		fprintf(stderr, "   + len_start:      %u\n", len_start);
		fprintf(stderr, "   + len_end:        %u\n", len_end);
		fprintf(stderr, "   + algs_enabled: 0x%X\n", alg_ena_mask);
	}

	/* build up set of algorithms */

	algset_t *algset = algset_create("RVVRadar");
	if (algset == NULL)
		return -1;

	/* start with len_start and double len until len_end */
	for (int len = len_start; len <= len_end; len <<= 1) {

		if (alg_enabled(alg_ena_mask, ALG_ID_MEMCPY))
			if (alg_memcpy_add(algset, len) < 0) {
				perror("Error adding memcpy");
				ret = -1;
				goto __ret_algset_destroy;
			}

		if (alg_enabled(alg_ena_mask, ALG_ID_MAC_16_32_32))
			if (alg_mac_16_32_32_add(algset, len) < 0) {
				perror("Error adding mac_16_32_32");
				ret = -1;
				goto __ret_algset_destroy;
			}

		if (alg_enabled(alg_ena_mask, ALG_ID_MAC_8_16_32))
			if (alg_mac_8_16_32_add(algset, len) < 0) {
				perror("Error adding mac_8_16_32");
				ret = -1;
				goto __ret_algset_destroy;
			}

		if (alg_enabled(alg_ena_mask, ALG_ID_PNG_FILTER_UP3))
			if (alg_png_filters_add(algset, up, bpp3, len) < 0) {
				perror("Error adding png_filter_up3");
				ret = -1;
				goto __ret_algset_destroy;
			}

		if (alg_enabled(alg_ena_mask, ALG_ID_PNG_FILTER_UP4))
			if (alg_png_filters_add(algset, up, bpp4, len) < 0) {
				perror("Error adding png_filter_up4");
				ret = -1;
				goto __ret_algset_destroy;
			}

		if (alg_enabled(alg_ena_mask, ALG_ID_PNG_FILTER_SUB3))
			if (alg_png_filters_add(algset, sub, bpp3, len) < 0) {
				perror("Error adding png_filter_sub3");
				ret = -1;
				goto __ret_algset_destroy;
			}

		if (alg_enabled(alg_ena_mask, ALG_ID_PNG_FILTER_SUB4))
			if (alg_png_filters_add(algset, sub, bpp4, len) < 0) {
				perror("Error adding png_filter_sub4");
				ret = -1;
				goto __ret_algset_destroy;
			}

		if (alg_enabled(alg_ena_mask, ALG_ID_PNG_FILTER_AVG3))
			if (alg_png_filters_add(algset, avg, bpp3, len) < 0) {
				perror("Error adding png_filter_avg3");
				ret = -1;
				goto __ret_algset_destroy;
			}

		if (alg_enabled(alg_ena_mask, ALG_ID_PNG_FILTER_AVG4))
			if (alg_png_filters_add(algset, avg, bpp4, len) < 0) {
				perror("Error adding png_filter_avg4");
				ret = -1;
				goto __ret_algset_destroy;
			}

		if (alg_enabled(alg_ena_mask, ALG_ID_PNG_FILTER_PAETH3))
			if (alg_png_filters_add(algset, paeth, bpp3, len) < 0) {
				perror("Error adding png_filter_paeth3");
				ret = -1;
				goto __ret_algset_destroy;
			}

		if (alg_enabled(alg_ena_mask, ALG_ID_PNG_FILTER_PAETH4))
			if (alg_png_filters_add(algset, paeth, bpp4, len) < 0) {
				perror("Error adding png_filter_paeth4");
				ret = -1;
				goto __ret_algset_destroy;
			}
	}

	/* execution */
	algset_reset(algset);

	if (algset_run(algset, randseed, iterations, verify, !quiet) < 0) {
		perror("Error on run");
		ret = -1;
		goto __ret_algset_destroy;
	}

	ret = 0;

	/* cleanup */

__ret_algset_destroy:
	algset_destroy(algset);
	exit(ret);
}
