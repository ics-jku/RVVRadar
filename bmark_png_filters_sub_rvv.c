/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <stdint.h>

#if RVVBMARK_RVV_SUPPORT == 1

/*
 * sub neighbors in vectors and save result back
 * different vector sizes m1, m2, m4, m8
 */

/* simple m1
 * two loads on same field
 */
void png_filters_sub_rvv_dload(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row)
{
	uint8_t *rp_end = row + rowbytes;
	uint8_t *row_next = row + bpp;

	/*
	 * row:      | a | x |
	 *
	 * x_new = a + x
	 *
	 * v0-v31
	 * a .. 	[v0](e8)
	 * x ..		[v8](e8)
	 */


	while (row < rp_end) {

		asm volatile ("vsetvli		zero, %0, e8, m1" : : "r" (bpp));

		/* a = *row + *prev_row; */
		asm volatile ("vlbu.v		v0, (%0)" : : "r" (row));
		asm volatile ("vlbu.v		v8, (%0)" : : "r" (row_next));
		asm volatile ("vadd.vv		v0, v0, v8");

		/* *row = (uint8_t)a; */
		asm volatile ("vsb.v		v0, (%0)" : : "r" (row_next));	/* save a */

		row += bpp;
		row_next += bpp;
	}
}


/*
 * optimized
 * load field once and reuse result
 *
 * implicitly assumes VLEN >= 32bit(bpp=4) which is valid according to
 *  * riscv-v-spec-0.7.1/0.8.1/0.9 -- Chapter 2 ("VLEN >= SLEN >= 32")
 *  * riscv-v-spec-1.0 -- Chapter 18
 */
void png_filters_sub_rvv_reuse(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row)
{
	uint8_t *rp_end = row + rowbytes;

	/*
	 * row:      | a | x |
	 *
	 * x_new = a + x
	 *
	 * v0-v31
	 * a .. 	[v0-v1](e8)
	 * x ..		[v8-v9](e8)
	 */


	asm volatile ("vsetvli		zero, %0, e8, m1" : : "r" (bpp));

	/* c = *row++ */
	asm volatile ("vlbu.v		v0, (%0)" : : "r" (row));
	row += bpp;

	while (row < rp_end) {

		/* x = *row */
		asm volatile ("vlbu.v		v8, (%0)" : : "r" (row));
		/* c = c + x */
		asm volatile ("vadd.vv		v0, v0, v8");

		/* *row++ = c; */
		asm volatile ("vsb.v		v0, (%0)" : : "r" (row));
		row += bpp;
	}
}

#endif /* RVVBMARK_RVV_SUPPORT */
