#include	<stdio.h>
#include	<stdlib.h>
#include	<math.h>
#include	<time.h>
#include	<unistd.h>    /* for getopt */

/* user defined header file */
#include	"mipssim.h"

int	EXE_rd_to_ID_rt = 0; //
int	EXE_rd_to_ID_rs = 0; //
int	MEM_rd_to_ID_rt = 0; //
int	MEM_rd_to_ID_rs = 0; //
int	LW_output_to_SW_reginput = 0; // 
int	LW_output_to_SW_meminput = 0; // 
int	LWMEM_rd_to_ID_rt = 0; //
int	LWMEM_rd_to_ID_rs = 0; // 
//int	INST_rd_to_SW_rt = 0; // 

/* define buffer for printing out messages */
char	g_print_buf[PRINT_BUF_SIZE];	    // Should be large enough

/* Define register file and data memory */
UINT32	    RegFile[NUM_REGISTERS];
UINT32	    DataMem[MEM_DATA_SIZE/4];

/* list of instructions */
// A list can be used.
Instruction aInstructions[MAX_NUM_INSTRUCTIONS];
int	num_Instructions;

/* PC and cycle count */
UINT32	PC;
UINT32	nCycle;

/* special NOP instruction */
static Instruction NOP_Instr;

/*  Pipeline registers
Add necessary fields in each structure. 

pI is a pointer to the instruction structure. After loading instructions into array (aInstructions), you do not need to change anything in the array. Also pay attention to the case that a instruction can have multiple copies in the pipeline (e.g., in a loop). */
struct IF_ID_tag {
    Instruction *pI;
} IF_ID;

/* For example, rs and rt are read from register file. The values are stored in the pipeline register */ 
/* You can put the real destination register number here (for convenience) */ 
struct ID_EX_tag {
    Instruction *pI;
    UINT32  v_rs, v_rt;
    int    checkBNE;                      
    int    checkBEQ;                      
    int     address;   
    int	    rd;	
} ID_EX;

/* For example, ALU result is store in r_alu */
struct EX_ME_tag {
    Instruction *pI;
    int     rd; 
    UINT32  v_rs, v_rt;
    UINT32  v_alu_output;
                           
} EX_ME;

/* For example, ALU result is passed into ME/WB stage and the load instruciton may also have result from memmory (r_mem) */
/* rs and rt are not useful in WB stage */
struct ME_WB_tag {
    Instruction *pI;
    UINT32  v_alu_output, v_mem_output, v_mem_input;
    int     rd;                        
} ME_WB;


// options
// -c : print Core information every cycle
// -r : print registers
int	option_c = 0, option_n = 1, option_r = 0, option_l = 0;    

/* Functions */

void	myerror(char *s)
{
    fprintf (stderr, "%s\n", s);
    exit(1);
}

/************** You can add your data and functions below */
void	printRegisters()
{
    int	    i;
    for (i = 0; i < NUM_REGISTERS; i ++)  {
	   printf("R%02d=%08X ", i, RegFile[i]);
	   if ((i & 7) == 7) {
	       printf("\n");
	   }
    }
}

void	printCoreInfo()
{
    printf("============ Cycle=%d PC=%04X\n", nCycle, PC);
    printf("IF_ID:  ");
    LDI_printInstructions(IF_ID.pI, 1);
    printf("ID_EX:  ");                     
    LDI_printInstructions(ID_EX.pI, 1);     
    printf("EX_ME:  ");                     
    LDI_printInstructions(EX_ME.pI, 1);     
    printf("ME_WB:  ");                     
    LDI_printInstructions(ME_WB.pI, 1);

    if (option_r) {
	   printRegisters();
    }

    printf("\n");

    if(EXE_rd_to_ID_rt == 1) {
		printf("Forward EXE rd [$%02d] to ID rt [$%02d]\n\n", EX_ME.rd, ID_EX.pI->rt);
    }

    if(EXE_rd_to_ID_rs == 1) {
		printf("Forward EXE rd [$%02d] to ID rs [$%02d]\n\n", EX_ME.rd, ID_EX.pI->rs);
    }

    if(MEM_rd_to_ID_rt == 1) {
		printf("Forward MEM rd [$%02d] to ID rt [$%02d]\n\n", ME_WB.rd, ID_EX.pI->rt);
    }

    if(MEM_rd_to_ID_rs == 1) {
		printf("Forward MEM rd [$%02d] to ID rs [$%02d]\n\n", ME_WB.rd, ID_EX.pI->rs);
    }

    if(LW_output_to_SW_reginput == 1) {
		printf("Forward LW output rt [$%02d] to SW rt [$%02d]\n\n", ME_WB.rd, EX_ME.pI->rt);
    }
/*
    if(LW_output_to_SW_meminput == 1) {
		printf("Forward LW output data from MEM [$%02d] to memory location [$%02d]\n\n", (ME_WB.v_mem_output + EX_ME.pI->immed), EX_ME.v_alu_output);
    }
*/
    if(LWMEM_rd_to_ID_rt == 1) {
		printf("Forward LW output [$%02d] to ID rt [$%02d]\n\n", ME_WB.rd, ID_EX.pI->rt);
    }

    if(LWMEM_rd_to_ID_rs == 1) {
		printf("Forward LW output to [$%02d] to ID rs [$%02d]\n\n", ME_WB.rd, ID_EX.pI->rs);
    }
	/*
    if(INST_rd_to_SW_rt == 1) {
		printf("Forward EXE rd [$%02d] to SW rt [$%02d]\n\n", EX_ME.rd, ID_EX.pI->rt);
    }
	*/
    if(ME_WB.pI->op_code == 1) {
		printf("Data [%d] from address [%02d] loaded into register [$%02d]\n\n", ME_WB.v_mem_output, ME_WB.pI->rs, ME_WB.rd);
    }

    if(ME_WB.pI->op_code == 2) {
		printf("Data [%d] from register [$%02d] stored into address [%d]\n\n", ME_WB.v_mem_input, ME_WB.pI->rt, ME_WB.pI->rs);  // EX_ME.v_alu_output
    }

}

/* simulate Data memory */
UINT32	DMEM_read(UINT32 addr)
{
    if (addr >= (MEM_DATA_START + MEM_DATA_SIZE) || addr < (MEM_DATA_START) || (addr & 3))
    	myerror(SPRINTF("DMEM_read: Invalid data memory address (%08X).", addr)); 
    return DataMem[(addr - MEM_DATA_START) >> 2];
}

void DMEM_write(UINT32 addr, UINT32 value)
{
    if (addr >= (MEM_DATA_START + MEM_DATA_SIZE) || addr < (MEM_DATA_START) || (addr & 3))
	   myerror(SPRINTF("DMEM_write: Invalid data memory address (%08X).", addr)); 
    DataMem[(addr - MEM_DATA_START) >> 2] = value;
}

void	IF_stage()
{
    // Example
    int idxI;
    /* Currently, PC is incremented by 4 each cycle */
    /* You need to check if you should use the branch target address */ 

    if (ID_EX.checkBNE == 1 || ID_EX.checkBEQ == 1) {	// cancels previous instructions if branch taken
        IF_ID.pI = &NOP_Instr;
        PC -= 4;
    }
    else {
        idxI = PC2INDEX(PC);
        if (idxI < num_Instructions) {
    	   IF_ID.pI = &aInstructions[idxI];
    	   PC += 4;
        }
        else {
    	   IF_ID.pI = &NOP_Instr;
        }
    }
}

void	ID_stage()
{
    ID_EX.pI = IF_ID.pI;
    int opCode = ID_EX.pI->op_code;  // opCode = current op_code 
	EXE_rd_to_ID_rt = 0; //
	EXE_rd_to_ID_rs = 0; //
	MEM_rd_to_ID_rt = 0; //
	MEM_rd_to_ID_rs = 0; //
	LWMEM_rd_to_ID_rt = 0; //
	LWMEM_rd_to_ID_rs = 0; //
	//INST_rd_to_SW_rt = 0; // fwd printing 

	if(opCode == 1 || opCode == 5) { // decode from register file for lw and addi (rt = rd) 
	    ID_EX.v_rs = RegFile[ID_EX.pI->rs];
	    ID_EX.rd = ID_EX.pI->rt;
	}
	else if(opCode == 2) {  // decode for sw
	    ID_EX.v_rs = RegFile[ID_EX.pI->rs];
	    ID_EX.v_rt = RegFile[ID_EX.pI->rt];
	}
	//branches are handled with 'check' variable in ID stage so that rs and rt registers use ones from 		//current cycle and resolve in ID stage
	else if(opCode == 6) { // decode for bne
	    ID_EX.v_rs = RegFile[ID_EX.pI->rs];
	    ID_EX.v_rt = RegFile[ID_EX.pI->rt];
	}
	else if(opCode == 7) { // decode for beq
	    ID_EX.v_rs = RegFile[ID_EX.pI->rs];
	    ID_EX.v_rt = RegFile[ID_EX.pI->rt];
	}
	// decode for all other instructions
	else if(opCode == 3 || opCode == 4 || opCode == 8 || opCode == 9 || opCode == 10 || opCode == 11) {
	    ID_EX.v_rt = RegFile[ID_EX.pI->rt];
	    ID_EX.v_rs = RegFile[ID_EX.pI->rs];
	    ID_EX.rd = ID_EX.pI->rd;
	}

    //////load-use hazard detection unit//////

    // fixes load hazard for loads in EXE with stall.. will stall to load value into register before source registers are used. no need to stall if sw inst. uses forwarding to put value from register load into source, reducing 2 stall requirement to 1. 
    if(EX_ME.pI->op_code == 1 && opCode != 2) { 
	    if(ID_EX.pI->rs == EX_ME.rd || ID_EX.pI->rt == EX_ME.rd) {
			ID_EX.pI = &NOP_Instr;
			PC -= 4;
	    }
    }

    ///////data forwarding//////

        // fixes data dependencies if rs depends on rs from EX_ME or ME_WB, forward output from stage needed to rs
	// EXtoRS
	if((EX_ME.pI->op_code != 1) || (EX_ME.pI->op_code != 2) || (EX_ME.pI->op_code != 6) || (EX_ME.pI->op_code != 7)) {
		if(ID_EX.pI->rs == EX_ME.rd && EX_ME.rd != 0) { // && (ID_EX.pI->rs != ME_WB.rd)
			ID_EX.v_rs = EX_ME.v_alu_output;
			EXE_rd_to_ID_rs = 1;
		}
	}
	// MEtoRS
	if((ME_WB.pI->op_code != 1) || (ME_WB.pI->op_code != 2) || (ME_WB.pI->op_code != 6) || (ME_WB.pI->op_code != 7)) {
		if(ID_EX.pI->rs == ME_WB.rd && ME_WB.rd != 0 && (ID_EX.pI->rs != EX_ME.rd)) { // && (ID_EX.pI->rs != EX_ME.rd)
			ID_EX.v_rs = ME_WB.v_alu_output;
			MEM_rd_to_ID_rs = 1;
		}
	}
	// fixes data dependencies if rt depends on rs from EX_ME or ME_WB, forward output from stage needed to rt
	//EXtoRT
	if((EX_ME.pI->op_code != 1) || (EX_ME.pI->op_code != 2) || (EX_ME.pI->op_code != 6) || (EX_ME.pI->op_code != 7) || (EX_ME.pI->op_code != 5)) {
		if(ID_EX.pI->rt == EX_ME.rd && EX_ME.rd != 0 && opCode != 1) { // && (ID_EX.pI->rt != ME_WB.rd)
			ID_EX.v_rt = EX_ME.v_alu_output;
			EXE_rd_to_ID_rt = 1;
		}
	}
	//MEtoRT
	if((ME_WB.pI->op_code != 1) || (ME_WB.pI->op_code != 2) || (ME_WB.pI->op_code != 6) || (ME_WB.pI->op_code != 7) || (ME_WB.pI->op_code != 5)) {
		if(ID_EX.pI->rt == ME_WB.rd  && ME_WB.rd != 0 && (ID_EX.pI->rt != EX_ME.rd)) { // && (ID_EX.pI->rt != EX_ME.rd)
			ID_EX.v_rt = ME_WB.v_alu_output;
			MEM_rd_to_ID_rt = 1;
		}
	}

	// fixes data dependencies if instruction in ID was dependent on LW loading data to register. Must use memory output since this is where load writes. (I believe RAW hazard) 
	if(ME_WB.pI->op_code == 1) {
		if(ID_EX.pI->rt == ME_WB.rd  && ME_WB.rd != 0) {
			ID_EX.v_rt = ME_WB.v_mem_output;
			LWMEM_rd_to_ID_rt = 1;
		}
	}
	if(ME_WB.pI->op_code == 1  && opCode != 2) { //
		if(ID_EX.pI->rs == ME_WB.rd && ME_WB.rd != 0) {
			ID_EX.v_rs = ME_WB.v_mem_output;
			LWMEM_rd_to_ID_rs = 1;
		}
	}


/*
	if(opCode == 2 && EX_ME.pI->op_code != 2 && EX_ME.pI->op_code != 6 && EX_ME.pI->op_code != 7) {
		if(ID_EX.pI->rt == EX_ME.rd) {
			ID_EX.v_rt = EX_ME.rd;
			//INST_rd_to_SW_rt = 1;
		}
	}
*/

    if (opCode == 6 && ID_EX.v_rs != ID_EX.v_rt) { // handle bne if taken
	ID_EX.checkBNE = 1;
    }

    if (opCode == 7 && ID_EX.v_rs == ID_EX.v_rt) { // handle beq if taken
	ID_EX.checkBEQ = 1;
    }

    // Update branch address if branch taken
    if(opCode == 6 && ID_EX.checkBNE == 1) {
        ID_EX.address = PC + (ID_EX.pI->immed * 4);
    }
    if(opCode == 7 && ID_EX.checkBEQ == 1) {
        ID_EX.address = PC + (ID_EX.pI->immed * 4);
    }


}

void	EXE_stage()
{
    EX_ME.pI = ID_EX.pI;
    int opCode = EX_ME.pI->op_code;

    // perform alu operations 
    
    /*
    EX_ME.rd = ID_EX.rd;
    EX_ME.v_rt = ID_EX.v_rt;
    EX_ME.v_rs = ID_EX.v_rs;
    */

    if(opCode == 1) {	// LW
        EX_ME.v_alu_output = ID_EX.v_rs + EX_ME.pI->immed;
        EX_ME.rd = ID_EX.rd;
    }
    else if(opCode == 2) {	// SW
        EX_ME.v_alu_output = ID_EX.v_rs + EX_ME.pI->immed;
        EX_ME.v_rt = ID_EX.v_rt;
	EX_ME.rd = ID_EX.v_rs;
    }
    else if(opCode == 3) {	// ADD
        EX_ME.v_alu_output = ID_EX.v_rs + ID_EX.v_rt;
        EX_ME.rd = ID_EX.rd;
    }
    else if(opCode == 4) {	// SUB
        EX_ME.v_alu_output = ID_EX.v_rs - ID_EX.v_rt;
        EX_ME.rd = ID_EX.rd;
    }
    else if(opCode == 5) {	// ADDI
        EX_ME.v_alu_output = ID_EX.v_rs + EX_ME.pI->immed;
        EX_ME.rd = ID_EX.rd;
    }
    else if(opCode == 8) {	// AND
        EX_ME.v_alu_output = ID_EX.v_rs & ID_EX.v_rt;
        EX_ME.rd = ID_EX.rd;
    }
    else if(opCode == 9) {	// OR
        EX_ME.v_alu_output = ID_EX.v_rs | ID_EX.v_rt;
        EX_ME.rd = ID_EX.rd;
    }
    else if(opCode == 10) {	// NOR
        EX_ME.v_alu_output = ~(ID_EX.v_rs | ID_EX.v_rt);
        EX_ME.rd = ID_EX.rd;
    }
    else if(opCode == 11) {	// XOR
        EX_ME.v_alu_output = ID_EX.v_rs ^ ID_EX.v_rt;
        EX_ME.rd = ID_EX.rd;
    }

    // update program counter to correct value if branch taken - bne
    if(opCode == 6 && ID_EX.checkBNE == 1) {
        PC = ID_EX.address;
        ID_EX.checkBNE = 0; // reset check for branch
    }
    // update program counter to correct value if branch taken - beq
    if(opCode == 7 && ID_EX.checkBEQ == 1) {
        PC = ID_EX.address;
        ID_EX.checkBEQ = 0; // reset check for branch
    }

    ///////load before store (memory-to-memory) forwarding///////

	LW_output_to_SW_reginput = 0;
	LW_output_to_SW_meminput = 0;

/*
    // handles load before store hazard when register that holds data to be stored depends on previous load to finish. forward data from load to register that holds data to be stored. rt == rt
    if(opCode == 2 && ME_WB.pI->op_code == 1) { 
	if(EX_ME.pI->rt == ME_WB.rd && ME_WB.rd != 0) {
        	EX_ME.v_rt = ME_WB.v_mem_output;
		LW_output_to_SW_reginput = 1;
	}
    }
*/
    // handles load before store hazard when data to be stored depends on data loaded. forward output of load to data to be stored. rs == rt
    if(opCode == 2 && ME_WB.pI->op_code == 1) { 
	if(EX_ME.pI->rs == ME_WB.rd && ME_WB.rd != 0) {
        	EX_ME.v_alu_output = ME_WB.v_mem_output + EX_ME.pI->immed;
		LW_output_to_SW_meminput = 1;
	}
    }

}

void	MEM_stage()
{
//    ME_WB.pI = EX_ME.pI;
    int opCode = EX_ME.pI->op_code;

    if(opCode == 1) { 
	ME_WB.rd = EX_ME.rd;
        ME_WB.v_mem_output = DMEM_read(EX_ME.v_alu_output);  // update memory output for load from rs

	// other solution to memory-memory forwarding??
	/*
	if(EX_ME.pI->op_code == 2) {
		if(EX_ME.pI->rt == ME_WB.rd && ME_WB.rd != 0) {
			EX_ME.rd = ME_WB.v_mem_output;
			LW_output_to_SW_reginput = 1;
		}
	}
	*/
	

    }

    else if(opCode == 2) { // update memory with data
	if(ME_WB.pI->op_code == 1) {
		if(EX_ME.pI->rt == ME_WB.rd && ME_WB.rd != 0) {
			EX_ME.v_rt = ME_WB.v_mem_output;
			LW_output_to_SW_reginput = 1;
		}
		printf ("forwarding: $%d = %08X\n", EX_ME.pI->rt, EX_ME.v_rt);
	}
        DMEM_write(EX_ME.v_alu_output, EX_ME.v_rt);
	ME_WB.rd = EX_ME.rd;
	ME_WB.v_mem_input = DMEM_read(EX_ME.v_alu_output); // for printing purposes
    }


    else if(opCode == 3 || opCode == 4 || opCode == 8 || opCode == 9 || opCode == 10 || opCode == 11 || opCode == 5) { // pass along data and registers
		ME_WB.rd = EX_ME.rd;
        ME_WB.v_alu_output = EX_ME.v_alu_output;
	ME_WB.v_mem_input = EX_ME.v_alu_output;	
    }
    ME_WB.pI = EX_ME.pI;
}

void	WB_stage()
{
    // Update register file
    // Check if you need to update registe file. 
    // If it is needed, what value you shoud write to register file and to which register.

    int opCode = ME_WB.pI->op_code;

    if(opCode == 3 || opCode == 4 || opCode == 8 || opCode == 9 || opCode == 10 || opCode == 11 || opCode == 5 || opCode == 1) { // write output to register file
        RegFile[ME_WB.rd] = ME_WB.v_alu_output;
    }

    if(opCode == 1) { // write output to register file
        RegFile[ME_WB.rd] = ME_WB.v_mem_output;
    }

}

void	do_Sim()
{
    int	done;

    /* initialize register for data memory*/
    RegFile[REG_GP] = MEM_DATA_START;
    RegFile[REG_SP] = MEM_DATA_START + MEM_DATA_SIZE - 4;
    RegFile[0] = 0;

    /* special NOP opration */
    LDI_setNOP(&NOP_Instr);

    IF_ID.pI = &NOP_Instr;
    ID_EX.pI = &NOP_Instr;
    EX_ME.pI = &NOP_Instr;
    ME_WB.pI = &NOP_Instr;

    // initialization
    nCycle = 0;
    PC = INDEX2PC(0);
    done = 0;
    char string[10];      
    while (! done) {
	/* perform operations in each stage */
	/* new values are stored in pipeline register directly */
	/* if you need to use old values, you can save the old values in fields like old_v_res. 
	 * This is not what hardware does, but easier for you to implement in software.
	 * */

        if (option_n == 1) {                                                                
            printf("Enter # of cycles: ");     
            if(fgets(string, 10, stdin) != NULL) {                                          
                if(string[0] == '\n') {                                                    
                    option_n++;                                                            
                }                                                                         
                else {                                                                  
                    option_n += atoi(string);                                              
                }                                                                        
            }                                                                               
        }                                                                                

    	WB_stage();
    	MEM_stage();
    	EXE_stage();
    	ID_stage();
    	IF_stage();

    	// If the option is specified, print out pipeline status every cycle.
    	if (option_c) { 
    	    printCoreInfo();
    	}

    	// Start a new cycle
    	nCycle ++;
        option_n--;                    

    	// You need to change the condition for terminating the simulation
    	if (IF_ID.pI->op_code == 0 && ID_EX.pI->op_code == 0 && EX_ME.pI->op_code == 0 && ME_WB.pI->op_code == 0) {       
    	    done = 1;                                                                                         
    	} 
    }
}

/**  Main function 
 * */
int main (int argc, char **argv)
{
    int c;

    opterr = 0;
    while ((c = getopt (argc, argv, "crln:")) != -1) {
        switch (c) {
            case 'c':
		        option_c = 1;
                break;
            case 'r':
        		option_r = 1;
        		break;
            case 'l':
        		option_l = 1;
        		break;
            case 'n':
        		option_n = atoi(optarg);
        		break;
	        case '?':
        		myerror(SPRINTF("Unknown option (%c) or it requires an argument.", optopt)); 
        		break;
            default:
		        abort();
       }
    }
    if (optind + 1 != argc) 
	   myerror("Error: No file was specified.");
    num_Instructions = LDI_loadInstructions(argv[optind], aInstructions, MAX_NUM_INSTRUCTIONS);

    /* print out the instructions if requested */
    if (option_l) 
	   LDI_printInstructions(aInstructions, num_Instructions);
    do_Sim();
    return 0;
}
