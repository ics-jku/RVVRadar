
	.globl	memcpy_rv_wlenx4

# dest src len
memcpy_rv_wlenx4:
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
	bnez		a2, memcpy_rv_wlenx4
	ret
