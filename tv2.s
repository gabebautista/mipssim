# Fibonacci sequence
main:				    # define a label. main = 0x4000.
	addi	$10, $0, 0x1000
	addi	$1, $0, 0x1	    
	sw	$0, ($10)
	sw	$1, 4($10)

	addi	$9, $0, 46
again:
	lw	$2, ($10)
	lw	$3, 4($10)
	add	$4, $2, $3
	sw	$4, 8($10)
	addi	$9, $9, -1
	addi	$10, $10, 4
	bne	$9, $0, again

# check the values in $2, $3, $4, $9, and $10
