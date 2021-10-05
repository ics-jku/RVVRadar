/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <stdint.h>

#if RVVBMARK_RVV_SUPPORT == 1

/* using only widest element (e32) */
void mac_16_32_32_rvv_e32(int32_t *add_res, int16_t *mul1, int16_t *mul2, unsigned int len)
{
	unsigned int vl;

	while (len) {
		/* 32bit elements in groups of 8 vregs */
		asm volatile ("vsetvli		%0, %1, e32, m8" : "=r" (vl) : "r" (len));

		/* load mul1 in [v8-v15](e32) */
		asm volatile ("vlh.v		v8, (%0)" : : "r" (mul1));
		mul1 += vl;

		/* load mul2 in [v16-v23](e32) */
		asm volatile ("vlh.v		v16, (%0)" : : "r" (mul2));
		mul2 += vl;

		/* load add_res in [v0-v7](e32) */
		asm volatile ("vlw.v		v0, (%0)" : : "r" (add_res));

		/* mac: [v0-v7](e32) = [v0-v7](e32) + ([v8-v15](e32) * [v16-v24](e32) */
		asm volatile ("vmacc.vv		v0, v8, v16");

		/* store add_res from [v0-v7](e32) */
		asm volatile ("vsw.v		v0, (%0)" : : "r" (add_res));
		add_res += vl;

		len -= vl;
	}
}


/* using e16 and widen to e32 on MAC */
void mac_16_32_32_rvv_e16_widening(int32_t *add_res, int16_t *mul1, int16_t *mul2, unsigned int len)
{
	unsigned int vl;

	while (len) {
		/* initialize [v0-v7](e32] with values from add_res(32bit) */

		/* 32bit elements in groups of 8 vregs */
		asm volatile ("vsetvli		%0, %1, e32, m8" : "=r" (vl) : "r" (len));

		/* load add_res(32bit) in [v0-v7](e32) */
		asm volatile ("vlw.v		v0, (%0)" : : "r" (add_res));


		/* load multiplicants */

		/* 16bit elements in groups of 4 vregs */
		asm volatile ("vsetvli		zero, %0, e16, m4" : : "r" (len));

		/* load mul1 in [v8-v11](e16) */
		asm volatile ("vlh.v		v8, (%0)" : : "r" (mul1));
		mul1 += vl;

		/* load mul2 in [v12-v15](e16) */
		asm volatile ("vlh.v		v12, (%0)" : : "r" (mul2));
		mul2 += vl;


		/* calculate */

		/* mac with widening: [v0-v7](e32) = [v0-v7](e32) + ([v8-v11](e16) * [v12-v15](e16)) */
		asm volatile ("vwmacc.vv	v0, v8, v12");


		/* save results */

		/* 32bit elements in groups of 8 vregs */
		asm volatile ("vsetvli		zero, %0, e32, m8" : : "r" (len));

		/* store add_res from [v0-v7](e32) */
		asm volatile ("vsw.v		v0, (%0)" : : "r" (add_res));
		add_res += vl;

		len -= vl;
	}
}

#endif /* RVVBMARK_RVV_SUPPORT */
