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
 */
void png_filter_paeth_rvv_m1(unsigned int bpp, unsigned int rowbytes, uint8_t *row, uint8_t *prev_row)
{
	unsigned int vl = 0;

	/*
	 * row:      | c | x |
	 * prev_row: | b | a |
	 *
	 * v0-v31
	 * mask .. 	[v0]
	 * a .. 	[v2](e8)
	 * b ..		[v4](e8)
	 * c ..		[v6](e8)
	 * x ..		[v8](e8)
	 * p .. 	[v12-v13](e16)
	 * pa ..	[v16-v17](e16)
	 * pb ..	[v20-v21](e16)
	 * pc ..	[v24-v25](e16)
	 * tmpmask ..	[v31]
	 */

	/* first pixel */
	uint8_t *rp_end = row + bpp;
	while (row < rp_end) {

		asm volatile ("vsetvli		%0, %1, e8, m1" : "=r" (vl) : "r" (bpp));

		/* a = *row + *prev_row; */
		asm volatile ("vlbu.v		v2, (%0)" : : "r" (row));	/* load *row */
		asm volatile ("vlbu.v		v6, (%0)" : : "r" (prev_row));	/* load *prev_row */
		asm volatile ("vadd.vv		v2, v2, v6");

		/* *row = (uint8_t)a; */
		asm volatile ("vsb.v		v2, (%0)" : : "r" (row));	/* save a */

		prev_row += vl;
		row += vl;
	}

	/* remaining pixels */
	rp_end = rp_end + (rowbytes - bpp);
	while (row < rp_end) {

		/* b = *prev_row; */
		asm volatile ("vlbu.v		v4, (%0)" : : "r" (prev_row));
		/* x = *row */
		asm volatile ("vlbu.v		v8, (%0)" : : "r" (row));

		/* sub (widening to 16bit) */
		/* p = b - c; */
		asm volatile ("vwsubu.vv	v12, v4, v6");
		/* pc = a - c; */
		asm volatile ("vwsubu.vv 	v24, v2, v6");

		/* switch to widened */
		asm volatile ("vsetvli		zero, %0, e16, m2" : : "r" (bpp));

		/* pa = abs(p) -> pa = p < 0 ? -p : p; */
		asm volatile ("vmv.v.v		v16, v12");			/* pa = p */
		asm volatile ("vmslt.vx 	v0, v16, zero");		/* set mask[i] if pa[i] < 0 */
		asm volatile ("vrsub.vx		v16, v16, zero, v0.t"); 	/* invert negative values in pa; vd[i] = 0 - vs2[i] (if mask[i])
										 * could be replaced by vneg in rvv >= 1.0
										 */

		/* pb = abs(p) -> pb = pc < 0 ? -pc : pc; */
		asm volatile ("vmv.v.v		v20, v24");			/* pb = pc */
		asm volatile ("vmslt.vx		v0, v20, zero"); 		/* set mask[i] if pc[i] < 0 */
		asm volatile ("vrsub.vx		v20, v20, zero, v0.t");		/* invert negative values in pb; vd[i] = 0 - vs2[i] (if mask[i])
										 * could be replaced by vneg in rvv >= 1.0
										 */

		/* pc = abs(p + pc) -> pc = (p + pc) < 0 ? -(p + pc) : p + pc; */
		asm volatile ("vadd.vv		v24, v24, v12");		/* pc = p + pc */
		asm volatile ("vmslt.vx		v0, v24, zero");		/* set mask[i] if pc[i] < 0 */
		asm volatile ("vrsub.vx		v24, v24, zero, v0.t"); 	/* invert negative values in pc; vd[i] = 0 - vs2[i] (if mask[i])
										 * could be replaced by vneg in rvv >= 1.0
										 */

		/*
		 * if (pb < pa) {
		 *	pa = pb;
		 *	a = b;	(see (*1))
		 * }
		 */
		asm volatile ("vmslt.vv		v0, v20, v16");			/* set mask[i] if pb[i] < pa[i] */
		asm volatile ("vmerge.vvm	v16, v16, v20, v0");		/* pa[i] = pb[i] (if mask[i]) */


		/*
		 * if (pc < pa)
		 *	a = c;	(see (*2))
		 */
		asm volatile ("vmslt.vv		v31, v24, v16");		/* set tmpmask[i] if pc[i] < pa[i] */

		/* switch to narrow */
		asm volatile ("vsetvli		zero, %0, e8, m1" : : "r" (bpp));

		/* (*1) */
		asm volatile ("vmerge.vvm	v2, v2, v4, v0");		/* a = b (if mask[i]) */

		/* (*2) */
		asm volatile ("vmand.mm		v0, v31, v31");			/* mask = tmpmask
										 * vmand works for rvv 0.7 up to 1.0
										 * could be replaced by vmcpy in 0.7.1/0.8.1
										 * or vmmv.m in 1.0
										 */
		asm volatile ("vmerge.vvm	v2, v2, v6, v0"); 		/* a = c (if mask[i]) */

		/* a += x */
		asm volatile ("vadd.vv		v2, v2, v8");

		/* *row = a */
		asm volatile ("vsb.v		v2, (%0)" : : "r" (row));


		/* prepare next iteration (prev_row is already in a) */
		/* c = b */
		asm volatile ("vmv.v.v		v6, v4");
		prev_row += vl;
		row += vl;
	}
}

#endif /* RVVBMARK_RVV_SUPPORT */
