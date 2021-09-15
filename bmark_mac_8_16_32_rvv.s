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

	.globl	mac_8_16_32_rvv_e32, mac_8_16_32_rvv_e16_widening, mac_8_16_32_rvv_e8_widening

# void mac_8_16_32_rvv_e32(int32_t *res, int16_t *add, int8_t *mul1, int8_t *mul2, unsigned int len)
#
# using only widest element (e32)
#
# res .. a0
# add .. a1
# mul1 .. a2
# mul2 .. a3
# len .. a4
#
mac_8_16_32_rvv_e32:
mac_8_16_32_rvv_e32_loop:
	# 32bit elements in groups of 8 vregs
	vsetvli		t0, a4, e32, m8

	# load mul1 in [v8-v15](e32) and update mul1 pointer
	vlb.v		v8, (a2)
	add		a2, a2, t0

	# load mul2 in [v16-v23](e32) and update mul2 pointer
	vlb.v		v16, (a3)
	add		a3, a3, t0

	# load add in [v0-v7](e32) and update add pointer
	vlh.v		v0, (a1)
	slli		t1, t0, 1	# t0*2
	add		a1, a1, t1

	# mac: [v0-v7](e32) = [v0-v7](e32) + ([v8-v15](e32) * [v16-v24](e32)
	vmacc.vv	v0, v8, v16

	# store res from [v0-v7](e32) and update res pointer
	vsw.v		v0, (a0)
	slli		t1, t1, 1	# t1*2 (=t0*4)
	add		a0, a0, t1

	# update len
	sub		a4, a4, t0

	# loop if still elements to process
	bnez		a4, mac_8_16_32_rvv_e32_loop
	ret


# void mac_8_16_32_rvv_e16_widening(int32_t *res, int16_t *add, int8_t *mul1, int8_t *mul2, unsigned int len)
#
# using e16 and widen to e32 on MAC
#
# res .. a0
# add .. a1
# mul1 .. a2
# mul2 .. a3
# len .. a4
#
mac_8_16_32_rvv_e16_widening:
mac_8_16_32_rvv_e16_widening_loop:

	# initialize [v0-v7](e32] with values from add(16bit)

	# 32bit elements in groups of 8 vregs
	vsetvli		t0, a4, e32, m8

	# load add(16bit) in [v0-v7](e32) and update add pointer
	vlh.v		v0, (a1)
	slli		t1, t0, 1	# t0*2
	add		a1, a1, t1


	# load multiplicants

	# 16bit elements in groups of 4 vregs
	vsetvli		zero, a4, e16, m4

	# load mul1 in [v8-v11](e16) and update mul1 pointer
	vlb.v		v8, (a2)
	add		a2, a2, t0

	# load mul2 in [v12-v15](e16) and update mul2 pointer
	vlb.v		v12, (a3)
	add		a3, a3, t0


	# calculate

	# mac with widening: [v0-v7](e32) = [v0-v7](e32) + ([v8-v11](e16) * [v12-v15](e16))
	vwmacc.vv	v0, v8, v12


	# save results

	# 32bit elements in groups of 8 vregs
	vsetvli		zero, a4, e32, m8

	# store res from [v0-v7](e32) and update res pointer
	vsw.v		v0, (a0)
	slli		t1, t1, 1	# t1*2 (=t0*4)
	add		a0, a0, t1

	# update len
	sub		a4, a4, t0

	# loop if still elements to process
	bnez		a4, mac_8_16_32_rvv_e16_widening_loop
	ret


# void mac_8_16_32_rvv_e8_widening(int32_t *res, int16_t *add, int8_t *mul1, int8_t *mul2, unsigned int len)
#
# using e8 and widen two times(MUL, ADD) to e32
#
# res .. a0
# add .. a1
# mul1 .. a2
# mul2 .. a3
# len .. a4
#
mac_8_16_32_rvv_e8_widening:
mac_8_16_32_rvv_e8_widening_loop:
	# 8bit elements in groups of 2 vregs
	vsetvli		t0, a4, e8, m2

	# load mul1 in [v0-v1](e8) and update mul1 pointer
	vlb.v		v0, (a2)
	add		a2, a2, t0

	# load mul2 in [v2-v3](e8) and update mul2 pointer
	vlb.v		v2, (a3)
	add		a3, a3, t0

	# mul [v0-v1](e8) with [v2-v3](e8), widen and save to [v4-v7](e16)
	vwmul.vv	v4, v0, v2

	# 16bit elements in groups of 4 vregs
	vsetvli		zero, a4, e16, m4

	# load add (16bit elements) in [v0-v3](e16) and update add pointer
	vlh.v		v0, (a1)
	slli		t1, t0, 1	# t0*2
	add		a1, a1, t1

	# add [v0-v3](e16) and [v4-v7](e16), widen and save to [v8-v15](e32)
	vwadd.vv	v8, v0, v4

	# 32bit elements in groups of 8 vregs
	vsetvli		zero, a4, e32, m8

	# store res from [v8-v15](e32) and update res pointer
	vsw.v		v8, (a0)
	slli		t1, t1, 1	# t1*2 (=t0*4)
	add		a0, a0, t1

	# update len
	sub		a4, a4, t0

	# loop if still elements to process
	bnez		a4, mac_8_16_32_rvv_e8_widening_loop
	ret
