/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <stdint.h>

#include "rvv_helpers.h"


#if RVVBMARK_RVV_SUPPORT

#if RVVBMARK_RVV_SUPPORT == RVVBMARK_RVV_SUPPORT_VER_07_08
/*
 * These implementations only make sense for rvv v0.7 and v0.8
 * In newer specs, there are no signed loads. Instead a unsigned load
 * must be combined with vsext, which adds an additional penalty.
 * Since these implementations generally are known to be less performant
 * it was decided to drop them completely for newer rvv drafts.
 */

/* using only widest element (e32) */
void mac_8_16_32_rvv_e32(int32_t *res, int16_t *add, int8_t *mul1, int8_t *mul2, unsigned int len)
{
	unsigned int vl;

	while (len) {

		/* 32bit elements in groups of 8 vregs */
		asm volatile ("vsetvli		%0, %1, e32, m8" : "=r" (vl) : "r" (len));

		/* load mul1 in [v8-v15](e32) */
		asm volatile ("vlb.v		v8, (%0)" : : "r" (mul1));
		mul1 += vl;

		/* load mul2 in [v16-v23](e32) */
		asm volatile ("vlb.v		v16, (%0)" : : "r" (mul2));
		mul2 += vl;

		/* load add in [v0-v7](e32) */
		asm volatile ("vlh.v		v0, (%0)" : : "r" (add));
		add += vl;

		/* mac: [v0-v7](e32) = [v0-v7](e32) + ([v8-v15](e32) * [v16-v24](e32) */
		asm volatile ("vmacc.vv		v0, v8, v16");

		/* store res from [v0-v7](e32) */
		asm volatile (VSE32_V"		v0, (%0)" : : "r" (res));
		res += vl;

		len -= vl;
	}
}


/* using e16 and widen to e32 on MAC */
void mac_8_16_32_rvv_e16_widening(int32_t *res, int16_t *add, int8_t *mul1, int8_t *mul2, unsigned int len)
{
	unsigned int vl;

	while (len) {

		/* 32bit elements in groups of 8 vregs */
		asm volatile ("vsetvli		%0, %1, e32, m8" : "=r" (vl) : "r" (len));

		/* load add(16bit) in [v0-v7](e32) */
		//asm volatile (VLE16_V"		v0, (%0)" : : "r" (add));
		asm volatile ("vlh.v		v0, (%0)" : : "r" (add));
		add += vl;


		/* load multiplicants */

		/* 16bit elements in groups of 4 vregs */
		asm volatile ("vsetvli		zero, %0, e16, m4" : : "r" (len));

		/* load mul1 in [v8-v11](e16) */
		//asm volatile (VLE8_V"		v8, (%0)" : : "r" (mul1));
		asm volatile ("vlb.v		v8, (%0)" : : "r" (mul1));
		mul1 += vl;

		/* load mul2 in [v12-v15](e16) */
		//asm volatile (VLE8_V"		v12, (%0)" : : "r" (mul2));
		asm volatile ("vlb.v		v12, (%0)" : : "r" (mul2));
		mul2 += vl;


		/* calculate */

		/* mac with widening: [v0-v7](e32) = [v0-v7](e32) + ([v8-v11](e16) * [v12-v15](e16)) */
		asm volatile ("vwmacc.vv	v0, v8, v12");


		/* save results */

		/* 32bit elements in groups of 8 vregs */
		asm volatile ("vsetvli		zero, %0, e32, m8" : : "r" (len));

		/* store res from [v0-v7](e32) */
		asm volatile (VSE32_V"		v0, (%0)" : : "r" (res));
		res += vl;

		len -= vl;
	}
}

#endif /* RVVBMARK_RVV_SUPPORT_VER_07_08 */


/* using e8 and widen two times(MUL, ADD) to e32 */
void mac_8_16_32_rvv_e8_widening(int32_t *res, int16_t *add, int8_t *mul1, int8_t *mul2, unsigned int len)
{
	unsigned int vl;

	while (len) {

		/* 8bit elements in groups of 2 vregs */
		asm volatile ("vsetvli		%0, %1, e8, m2" : "=r" (vl) : "r" (len));

		/* load mul1 in [v0-v1](e8) */
		asm volatile (VLE8_V"		v0, (%0)" : : "r" (mul1));
		mul1 += vl;

		/* load mul2 in [v2-v3](e8) */
		asm volatile (VLE8_V"		v2, (%0)" : : "r" (mul2));
		mul2 += vl;

		/* mul [v0-v1](e8) with [v2-v3](e8), widen and save to [v4-v7](e16) */
		asm volatile ("vwmul.vv		v4, v0, v2");

		/* 16bit elements in groups of 4 vregs */
		asm volatile ("vsetvli		zero, %0, e16, m4" : : "r" (len));

		/* load add (16bit elements) in [v0-v3](e16) */
		asm volatile (VLE16_V"		v0, (%0)" : : "r" (add));
		add += vl;

		/* add [v0-v3](e16) and [v4-v7](e16), widen and save to [v8-v15](e32) */
		asm volatile ("vwadd.vv		v8, v0, v4");

		/* 32bit elements in groups of 8 vregs */
		asm volatile ("vsetvli		zero, %0, e32, m8" : : "r" (len));

		/* store res from [v8-v15](e32) */
		asm volatile (VSE32_V"		v8, (%0)" : : "r" (res));
		res += vl;

		len -= vl;
	}
}

#endif /* RVVBMARK_RVV_SUPPORT */
