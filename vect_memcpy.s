#
# Risc-V Assembler program to print "Hello World!"
# to stdout.
#
# a0-a2 - parameters to linux function services
# a7 - linux function number
#

# Setup the parameters to print hello world
# and then call Linux to do it.

# QEMU:
# vl = 16*8 = 128bit
# D1:
# vl = 16*8 = 128bit

	.globl	int_memcpy, vect_memcpy, vect_memcpy32

# dest src len
int_memcpy:
int_memcpy_loop:
	# load elements and update src pointer
	lwu		t0, (a1)
	addi		a1, a1, 4
	lwu		t1, (a1)
	addi		a1, a1, 4
	lwu		t2, (a1)
	addi		a1, a1, 4
	lwu		t3, (a1)
	addi		a1, a1, 4

	# store elements and update dest pointer
	sw		t0, (a0)
	addi		a0, a0, 4
	sw		t1, (a0)
	addi		a0, a0, 4
	sw		t2, (a0)
	addi		a0, a0, 4
	sw		t3, (a0)
	addi		a0, a0, 4

	# update len
	addi		a2, a2, -16

	# loop if still elements to copy
	bnez		a2, int_memcpy_loop
	ret


# dest src len
vect_memcpy:
	addi		t1, zero, 1
vect_memcpy_loop:
	# get "worksize" (16 x 8bit elements per register in 8 grouped vector registers -> 16 * 8 = 128 elements (8bit) per op)
	vsetvli		t0, a2, e8, m8
	mul		t0, t0, t1

	# load elements and update src pointer
	vlbu.v		v0, (a1)
	add		a1, a1, t0

	# store elements and update dest pointer
	vsb.v		v0, (a0)
	add		a0, a0, t0

	# update len
	sub		a2, a2, t0

	# loop if still elements to copy
	bnez		a2, vect_memcpy_loop
	ret



# dest src len
vect_memcpy32:
	addi		t1, zero, 4
vect_memcpy32_loop:
	# get "worksize"
	vsetvli		t0, a2, e32
	mul		t0, t0, t1

	# load elements and update src pointer
	vlwu.v		v0, (a1)
	add		a1, a1, t0

	# store elements and update dest pointer
	vsw.v		v0, (a0)
	add		a0, a0, t0

	# update len
	sub		a2, a2, t0

	# loop if still elements to copy
	bnez		a2, vect_memcpy32_loop
	ret
