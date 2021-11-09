/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef ALG_PNG_FILTERS_H
#define ALG_PNG_FILTERS_H

#include <core/algset.h>

enum alg_png_filters_filter {
	up,		// bpp is irrelevant/ignored
	sub,
	avg,
	paeth
};
enum alg_png_filters_bpp {bpp3, bpp4};

int alg_png_filters_add(
	algset_t *algset,
	enum alg_png_filters_filter filter,
	enum alg_png_filters_bpp bpp,
	unsigned int len);

#endif /* ALG_PNG_FILTERS_H */
