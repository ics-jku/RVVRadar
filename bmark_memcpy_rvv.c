/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <stdint.h>

#include "rvv_helpers.h"


#if RVVBMARK_RVV_SUPPORT

/* use e8 elements */
void memcpy_rvv_8(uint8_t *dest, uint8_t *src, unsigned int len)
{
	unsigned int vl;

	while (len) {

		/* copy e8 elements in group of 8 vector registers at once */

		asm volatile ("vsetvli		%0, %1, e8, m8" : "=r" (vl) : "r" (len));

		asm volatile (VLE8_V"		v0, (%0)" : : "r" (src));
		src += vl;

		asm volatile (VSE8_V"		v0, (%0)" : : "r" (dest));
		dest += vl;

		len -= vl;
	}
}


/* use e32 elements
 * (len must be a multiple of 4 bytes)
 */
void memcpy_rvv_32(uint8_t *dest, uint8_t *src, unsigned int len)
{
	unsigned int vl;
	len >>= 2;

	while (len) {

		/* copy e8 elements in group of 8 vector registers at once */
		asm volatile ("vsetvli		%0, %1, e32, m8" : "=r" (vl) : "r" (len));
		len -= vl;

		asm volatile (VLE32_V"		v0, (%0)" : : "r" (src));
		vl <<= 2;
		src += vl;

		asm volatile (VSE32_V"		v0, (%0)" : : "r" (dest));
		dest += vl;
	}
}

#endif /* RVVBMARK_RVV_SUPPORT */
