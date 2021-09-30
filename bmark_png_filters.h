/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef BMARK_PNG_FILTERS_H
#define BMARK_PNG_FILTERS_H

#include "bmarkset.h"

enum bmark_png_filters_filter {
	up,		// bpp is irrelevant/ignored
	paeth
};
enum bmark_png_filters_bpp {bpp3, bpp4};

int bmark_png_filters_add(
	bmarkset_t *bmarkset,
	enum bmark_png_filters_filter filter,
	enum bmark_png_filters_bpp bpp,
	unsigned int len);

#endif /* BMARK_PNG_FILTERS_H */
