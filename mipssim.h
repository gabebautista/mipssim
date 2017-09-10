/* Define constants*/
#define	    UINT32	unsigned int
#define	    INT32	int

#define	    NUM_REGISTERS	    32
#define	    MAX_NUM_INSTRUCTIONS    256
#define	    MAX_LABEL_LENGTH	    32
#define	    MAX_NUM_LABELS	    32
#define	    MAX_LINE_LENGTH	    256

/* define the memory space that can be used */
#define	    MEM_INSTR_START	    0x4000
#define	    MEM_DATA_START	    0x1000
#define	    MEM_DATA_SIZE	    0x1000

/* Reigster number of $gp and $sp */
#define	    REG_GP		28
#define	    REG_SP		29

#define	    PRINT_BUF_SIZE	2048
extern char g_print_buf[PRINT_BUF_SIZE];	    // Should be large enough
#define	    SPRINTF(format, ...)    (snprintf(g_print_buf, 1024, format, __VA_ARGS__), g_print_buf)

/* conversion between the value of PC and the index into the instruction array */
/* For example, instruction 0 in the array is loacated at MEM_INSTR_START */
#define	    PC2INDEX(x)	    (((x) - MEM_INSTR_START) >> 2)
#define	    INDEX2PC(x)	    (((x) << 2) + MEM_INSTR_START) 

// These values do not match real MIPS encoding. 
typedef enum instruction_op_tag  {
    I_NOP = 0,
    I_LW,
    I_SW,
    I_ADD,
    I_SUB,
    I_ADDI,
    I_BNE,
    I_BEQ,
    I_AND, 
    I_OR, 
    I_NOR, 
    I_XOR, 
} InstructionOPCode;

/* Define data stuctures */

typedef	struct   struct_instruction_tag {
    UINT32	addr;		    // addrss of this instruction
    InstructionOPCode op_code;	    // instruction opcode
    int		rs, rt, rd;  // register numbers
    INT32	immed;		// immediate, already extended to 32 bits
    int		label_ind;
} Instruction;

void	myerror(char *s);
int	LDI_loadInstructions(char *fn, Instruction *pInstr_in, int max_num_instructions);
int	LDI_printInstructions(Instruction *pInstr, int n);
void	LDI_setNOP(Instruction *pInstr_in);


