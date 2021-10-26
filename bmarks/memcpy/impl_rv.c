/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <stdint.h>

#if RVVBMARK_RV_SUPPORT

/* use 4 32bit integer registers
 * (len must be a multiple of 16 bytes)
 */
void memcpy_rv_wlenx4(uint8_t *dest, uint8_t *src, unsigned int len)
{
	uint8_t *dest_end = dest + len;

	while (dest < dest_end) {

		/* load elements and update src pointer */
		asm volatile ("lwu		t0, (%0)" : : "r" (src) : "t0", "t1", "t2", "t3", "memory");
		src += 4;
		asm volatile ("lwu		t1, (%0)" : : "r" (src) : "t0", "t1", "t2", "t3", "memory");
		src += 4;
		asm volatile ("lwu		t2, (%0)" : : "r" (src) : "t0", "t1", "t2", "t3", "memory");
		src += 4;
		asm volatile ("lwu		t3, (%0)" : : "r" (src) : "t0", "t1", "t2", "t3", "memory");
		src += 4;

		/* store elements and update dest pointer */
		asm volatile ("sw		t0, (%0)" : : "r" (dest) : "t0", "t1", "t2", "t3", "memory");
		dest += 4;
		asm volatile ("sw		t1, (%0)" : : "r" (dest) : "t0", "t1", "t2", "t3", "memory");
		dest += 4;
		asm volatile ("sw		t2, (%0)" : : "r" (dest) : "t0", "t1", "t2", "t3", "memory");
		dest += 4;
		asm volatile ("sw		t3, (%0)" : : "r" (dest) : "t0", "t1", "t2", "t3", "memory");
		dest += 4;
	}
}

#endif /* RVVBMARK_RV_SUPPORT */
