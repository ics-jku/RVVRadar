/*
 * Copyright (C) 2021 Manfred Schlaegl <manfred.schlaegl@gmx.at>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

# QEMU:
# vl = 16*8 = 128bit
# D1:
# vl = 16*8 = 128bit

	.globl	memcpy_rvv_8, memcpy_rvv_32

# dest src len
memcpy_rvv_8:
memcpy_rvv_8_loop:
	# get "worksize" (16 x 8bit elements per register in 8 grouped vector registers -> 16 * 8 = 128 elements (8bit) per op)
	vsetvli		t0, a2, e8, m8

	# load elements and update src pointer
	vlbu.v		v0, (a1)
	add		a1, a1, t0

	# store elements and update dest pointer
	vsb.v		v0, (a0)
	add		a0, a0, t0

	# update len
	sub		a2, a2, t0

	# loop if still elements to copy
	bnez		a2, memcpy_rvv_8_loop
	ret



# dest src len
memcpy_rvv_32:
memcpy_rvv_32_loop:
	# get "worksize"
	vsetvli		t0, a2, e32
	slli		t0, t0, 2	# *4

	# load elements and update src pointer
	vlwu.v		v0, (a1)
	add		a1, a1, t0

	# store elements and update dest pointer
	vsw.v		v0, (a0)
	add		a0, a0, t0

	# update len
	sub		a2, a2, t0

	# loop if still elements to copy
	bnez		a2, memcpy_rvv_32_loop
	ret
