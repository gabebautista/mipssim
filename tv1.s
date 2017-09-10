# There are two data items in EXE_MEM, 
# EXE_MEM.ALU: the output of ALU
# EXE_MEM.ST: the data to be written to memory in the store instructions
#     Remember that Store needs two registers. One is used in EXE to 
#     calcuate address, the other is stored to MEM.
# There also two data items in MEM_WB
# MEM_WB.LD: the data loaded from memory
# MEM_WB.ALU: the result of the ALU  
#     These two can go through a MUX first. 

# forwarding paths
# 1, 2	: EXE_MEM to EXE (two inputs to ALU)
# 3, 4  : MEM_WB to EXE (two inputs to ALU)
# 5	: MEM_WB to MEM (data to memory)
# 7, 8  : EXE_MEM.ALU to ID  (two inputs to the comparitor)

main:				    # define a label. main = 0x4000.
	addi	$10, $0, 0x1000
	addi	$1, $0, 0x1	    # $1 = 1
	add	$2, $1, $1	    # $2 = 2, FwP 1,2
	add	$3, $2, $1	    # $3 = 3, FwP 1,5 
	add	$5, $2, $3	    # $5 = 5, FwP 5,2
	sw	$3, ($10)	    # MEM[0x1000] = 3, FwP 3
	add	$4, $5, $3	    # $4 = 8
	sw	$4, 4($10)	    # MEM[0x1004] = 8, FwP 1

	lw	$6, ($10)	    # $6 = 3 
	sw	$6, 8($10)	    # MEM[0x1008] = 3, FwP 5, NO STALL

	add	$2, $1, $1	    # $2 = 2
	beq	$2, $0, main	    # NOT TAKEN, FwP 7, STALL

	add	$2, $1, $1
	nop
	beq	$2, $0, main	    # NOT TAKEN, FwP none

	lw	$6, ($10)	    # $6 = 3
	beq	$6, $0, main	    # NOT TAKEN, STALL 2 cycles, FwP none

	lw	$6, ($10)	    # $6 = 3
	add	$3, $3, $0	    # $3 = 3
	bne	$6, $3, main	    # NOT TAKEN, FwP 8, STALL

	lw	$6, ($10)	    
	add	$6, $6, $6	    # $6 = 6, FwP 3,4, STALL
	addi	$11, $0, 0x100C
	sw	$6, ($11)	    # MEM[0x100C] = 6, FwP 1, 4

	addi	$6, $6, -1	    # $6 = 5
	add	$5, $6, $6	    # $5 = 10, FwP 1, 2
	add	$4, $6, $6	    # $4 = 10, FwP 3, 4
	add	$4, $5, $5	    # $4 = 20, FwP 3, 4
	add	$3, $4, $4	    # $3 = 40, FwP 1, 2, not 3 and 4

