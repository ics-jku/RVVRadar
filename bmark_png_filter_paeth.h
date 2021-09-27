/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef BMARK_PNG_FILTER_PAETH_H
#define BMARK_PNG_FILTER_PAETH_H

#include "bmarkset.h"


enum bmark_png_filter_paeth_type {paeth3, paeth4};


int bmark_png_filter_paeth_add(
	bmarkset_t *bmarkset,
	enum bmark_png_filter_paeth_type type,
	unsigned int len);

#endif /* BMARK_PNG_FILTER_PAETH_H */
