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

	.globl	mac_16_32_32_rvv_e32, mac_16_32_32_rvv_e16_widening

# void mac_16_32_32_rvv_e32(int32_t *add_res, int16_t *mul1, int16_t *mul2, unsigned int len)
#
# using only widest element (e32)
#
# add_res .. a0
# mul1 .. a1
# mul2 .. a2
# len .. a3
#
mac_16_32_32_rvv_e32:
mac_16_32_32_rvv_e32_loop:
	# 32bit elements in groups of 8 vregs
	vsetvli		t0, a3, e32, m8

	# load mul1 in [v8-v15](e32) and update mul1 pointer
	vlh.v		v8, (a1)
	slli		t1, t0, 1	# t0*2
	add		a1, a1, t1

	# load mul2 in [v16-v23](e32) and update mul2 pointer
	vlh.v		v16, (a2)
	add		a2, a2, t1

	# load add_res in [v0-v7](e32)
	vlw.v		v0, (a0)

	# mac: [v0-v7](e32) = [v0-v7](e32) + ([v8-v15](e32) * [v16-v24](e32)
	vmacc.vv	v0, v8, v16

	# store add_res from [v0-v7](e32) and update add_res pointer
	vsw.v		v0, (a0)
	slli		t1, t1, 1	# t1*2 (=t0*4)
	add		a0, a0, t1

	# update len
	sub		a3, a3, t0

	# loop if still elements to process
	bnez		a3, mac_16_32_32_rvv_e32_loop
	ret


# void mac_16_32_32_rvv_e16_widening(int32_t *add_res, int16_t *mul1, int16_t *mul2, unsigned int len)
#
# using e16 and widen to e32 on MAC
#
# add_res .. a0
# mul1 .. a1
# mul2 .. a2
# len .. a3
#
mac_16_32_32_rvv_e16_widening:
mac_16_32_32_rvv_e16_widening_loop:

	# initialize [v0-v7](e32] with values from add_res(32bit)

	# 32bit elements in groups of 8 vregs
	vsetvli		t0, a3, e32, m8

	# load add_res(32bit) in [v0-v7](e32)
	vlw.v		v0, (a0)


	# load multiplicants

	# 16bit elements in groups of 4 vregs
	vsetvli		zero, a3, e16, m4

	# load mul1 in [v8-v11](e16) and update mul1 pointer
	vlh.v		v8, (a1)
	slli		t1, t0, 1	# t0*2
	add		a1, a1, t1

	# load mul2 in [v12-v15](e16) and update mul2 pointer
	vlh.v		v12, (a2)
	add		a2, a2, t1


	# calculate

	# mac with widening: [v0-v7](e32) = [v0-v7](e32) + ([v8-v11](e16) * [v12-v15](e16))
	vwmacc.vv	v0, v8, v12


	# save results

	# 32bit elements in groups of 8 vregs
	vsetvli		zero, a3, e32, m8

	# store add_res from [v0-v7](e32) and update add_res pointer
	vsw.v		v0, (a0)
	slli		t1, t1, 1	# t1*2 (=t0*4)
	add		a0, a0, t1

	# update len
	sub		a3, a3, t0

	# loop if still elements to process
	bnez		a3, mac_16_32_32_rvv_e16_widening_loop
	ret
