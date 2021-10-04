/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <stdint.h>

#if RVVBMARK_RVV_SUPPORT == 1

/*
 * read, process and save single pixels using e8/m1
 *
 * implicitly assumes VLEN >= 32bit(bpp=4) which is valid according to
 *  * riscv-v-spec-0.7.1/0.8.1/0.9 -- Chapter 2 ("VLEN >= SLEN >= 32")
 *  * riscv-v-spec-1.0 -- Chapter 18
 */
void png_filters_avg_rvv(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row)
{
	uint8_t *rp_end = row + rowbytes;

	/*
	 * row:      | a | x |
	 * prev_row: |   | b |
	 *
	 * v0-v31
	 * a .. 	[v2](e8)
	 * b ..		[v4](e8)
	 * x ..		[v8](e8)
	 * tmp .. 	[v12-v13](e16)
	 */


	/* first pixel */

	asm volatile ("vsetvli		zero, %0, e8, m1" : : "r" (bpp));

	/* b = *prev_row; */
	asm volatile ("vlbu.v		v4, (%0)" : : "r" (prev_row));
	/* x = *row */
	asm volatile ("vlbu.v		v8, (%0)" : : "r" (row));

	/* b = b / 2 */
	asm volatile ("vsrl.vi		v4, v4, 1");
	/* a = x + b */
	asm volatile ("vadd.vv		v2, v4, v8");

	/* *row = a */
	asm volatile ("vsb.v		v2, (%0)" : : "r" (row));

	prev_row += bpp;
	row += bpp;


	/* remaining pixels */

	while (row < rp_end) {

		/* b = *prev_row; */
		asm volatile ("vlbu.v		v4, (%0)" : : "r" (prev_row));
		/* x = *row */
		asm volatile ("vlbu.v		v8, (%0)" : : "r" (row));

		/* tmp = a + b */
		asm volatile ("vwaddu.vv	v12, v2, v4");		/* add with widening */
		/* a = tmp/2 */
		asm volatile ("vnsrl.vi		v2, v12, 1");		/* divide/shift with narrowing */
		/* a += x */
		asm volatile ("vadd.vv		v2, v2, v8");

		/* *row = a */
		asm volatile ("vsb.v		v2, (%0)" : : "r" (row));

		prev_row += bpp;
		row += bpp;
	}
}

#endif /* RVVBMARK_RVV_SUPPORT */
