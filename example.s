#   Comments.  Anything after # is comments.
#   valid data memory address is between 0x1000 and 0x2000. 
#   Place the instructions in the memory starts from 0x4000. 
#   You can assume the number of instructions is less than 128. 

#   Label and instruction names are NOT case sensitive.
#   So 'nop' and 'NOP' are the same. 
#   registers are denoted as a number (0..31) after $.
#   An immeidate can be a label or a number (decimal, or hex).
#   0x indiates a hex representation.
#   You can use strtol() to convert a string to number.  

main:				# define a label. main = 0x4000.
	addi	$1, $0, 0x1000	# r1 = r0 + 0x1000. r0 is always 0.
	lw	$2, ($1)	# load a word from the address in 0x1000.
	sw	$2, 4 ($1)	# store a word to 0x10004.
	addi	$1, $0, 100	# r1 = r0 + 100
ex1:				# another lable. ex1 = 0x4010. 
	addi	$1, $1, -1	# the immediate can be negative. 
	add	$30, $30, $30
	sub	$30, $29, $2
	and	$5, $12, $31
	or	$6, $5, $11
	nor	$7, $6, $12
	xor	$8, $7, $13
	nop
	bne	$3, $4, ex1	# jump to ex1 if r3 != r4
	beq	$3, $5, ex1	# jump to ex1 if r3 == r5
	nop			

# The simulator knows how many instructions have been loaded. The IF stage 
# gets a NOP instruction when fetching beyond the last instruction.

