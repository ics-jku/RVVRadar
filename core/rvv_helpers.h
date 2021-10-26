/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef RVV_HELPERS_H
#define RVV_HELPERS_H

/* values for RVVBMARK_RVV_SUPPORT */
#define RVVBMARK_RVV_SUPPORT_NO			0
#define RVVBMARK_RVV_SUPPORT_VER_07_08		1
#define RVVBMARK_RVV_SUPPORT_VER_09_10_100	2

/* build time replacement of mnemonics */
#if RVVBMARK_RV_SUPPORT

#if RVVBMARK_RVV_SUPPORT == RVVBMARK_RVV_SUPPORT_VER_07_08
#define VLE8_V		"vlbu.v"
#define VLE16_V		"vlhu.v"
#define VLE32_V		"vlwu.v"
#define VSE8_V		"vsb.v"
#define VSE16_V		"vsh.v"
#define VSE32_V		"vsw.v"
#define VNSRL_WI	"vnsrl.vi"

#elif RVVBMARK_RVV_SUPPORT == RVVBMARK_RVV_SUPPORT_VER_09_10_100
#define VLE8_V		"vle8.v"
#define VLE16_V		"vle16.v"
#define VLE32_V		"vle32.v"
#define VSE8_V		"vse8.v"
#define VSE16_V		"vse16.v"
#define VSE32_V		"vse32.v"
#define VNSRL_WI	"vnsrl.wi"

#else
#error "unsupported RVV version -- check RVVBMARK_RVV_SUPPORT!"

#endif /* RVVBMARK_RVV_SUPPORT */
#endif /* RVVBMARK_RV_SUPPORT */

#endif /* RVV_HELPERS_H */
