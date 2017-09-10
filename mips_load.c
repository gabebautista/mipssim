#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<time.h>
#include	<ctype.h>

#include	"mipssim.h"

#define		TOKEN_VARIABLE	    1000
#define		TOKEN_NUMBER	    1001
#define		TOKEN_REGISTER	    1002

char * gInstructionName [] = {
    "NOP",
    "LW", 
    "SW", 
    "ADD", 
    "SUB",
    "ADDI",
    "BNE", 
    "BEQ",
    "AND",
    "OR",
    "NOR",
    "XOR",
    0,
};

typedef	struct   struct_label_tag {
    char    name[MAX_LABEL_LENGTH]; 
    UINT32  value;
    int	    valid;
} Variable;

static Variable    aLabels[MAX_NUM_LABELS];
static int	num_Labels;

/**** Functions related to loading instrcutions from a file **/
static	char    ldi_line[MAX_LINE_LENGTH];
static	char	ldi_tmp[MAX_LINE_LENGTH];
static	int	ldi_tmp_len;
static	int	ldi_line_number;
static	int	ldi_line_ind; // next character to be processed

static int 
strlcpy(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0) {
		while (--n != 0) {
			if ((*d++ = *s++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}

static	int addLabel(char *s, UINT32 value, int valid)
{
    int	    i; 

    for (i = 0; i < num_Labels; i ++) {
	if (! strcmp(s, aLabels[i].name)) 
	    break;
    }
    if (i >= num_Labels) {
	if (num_Labels == MAX_NUM_LABELS) 
	    myerror("Error: Too many labels are defined.");
	i = num_Labels ++;
	strlcpy(aLabels[i].name, s, MAX_LINE_LENGTH);
	aLabels[i].value = value;
	aLabels[i].valid = valid;
    }
    else { // Found
	if (aLabels[i].valid && valid) 
	    myerror(SPRINTF("Line %d: %s\n'%s' already defined.", ldi_line_number, ldi_line, s));
	if (valid) {
	    aLabels[i].value = value;
	    aLabels[i].valid = valid;
	}
    }
    return i;
}

static void	LDI_processLabels(Instruction *pInstr, int num_instr)
{
    int	    i;

    for (i = 0; i < num_instr; i ++, pInstr ++) {
	int	j;

	j = pInstr->label_ind;
	if (j < 0) 
	    continue;

	if (! aLabels[j].valid) {
	    myerror(SPRINTF("Error: '%s' not defined.", aLabels[j].name));
	}

	switch (pInstr->op_code) {
	    case I_SW:
	    case I_LW:
	    case I_ADDI:
		pInstr->immed = aLabels[j].value;
		break;
	    case I_BNE:
	    case I_BEQ:
		pInstr->immed = (int)PC2INDEX(aLabels[j].value) - (int)PC2INDEX(pInstr->addr) - 1;
		break;
	    default:
		myerror(SPRINTF("Instruction tyep %d should not have an immediate '%s'.", pInstr->op_code, aLabels[j].name));
	}
    }
}

// return 0 if it reaches the end of string
static	int LDI_skipSpace()
{
    int	    c;

    while ((c = ldi_line[ldi_line_ind]) != 0) {
	if (! isspace(c)) {
	    if (c == '#') {
		ldi_line[ldi_line_ind] = 0;
		return 0;
	    }
	    else {
		return c;
	    }
	}
	ldi_line_ind ++;
    }
    return 0;
}

/*
 * Get a token from a input string.
 *
 * @return int 0: End of String. 1-255: a character. 
 */
static	int LDI_getToken()
{
    int	c;

    c = LDI_skipSpace();
    switch (c) {
	case 0:
	    ldi_tmp[0] = 0;
	    return 0;
	case ',':
	case '(':
	case ')':
	case ':':
	    ldi_line_ind ++;
	    ldi_tmp[0] = c;
	    ldi_tmp[1] = 0;
	    return c;
    }

    // copy it to a temp buffer 
    ldi_tmp_len = 0;
    while (c && (isalnum(c) || c == '_' || c == '$' || c == '-' || c == '+')) { // valid characters
	ldi_tmp[ldi_tmp_len ++] = toupper(c);
	c = ldi_line[++ldi_line_ind];
    }
    ldi_tmp[ldi_tmp_len] = 0;

    if (! ldi_tmp_len) 
	myerror(SPRINTF("Line %d: %s\nInvalid character (%c).", ldi_line_number, ldi_line, c));

    // printf("Token=%s\n", ldi_tmp);
    c = ldi_tmp[0];
    if (c == '$') 
	return TOKEN_REGISTER;
    if (isdigit(c) || c == '-' || c == '+')
	return TOKEN_NUMBER;
    return TOKEN_VARIABLE;
}

static int LDI_getRegisterNumber(char *s)
{
    int	    v = 0, c;

    c = s[0];
    if (c != '$')
	return -1;

    c = s[1];
    if (! isdigit(c)) 
	return -1;
    v = c - '0';

    c = s[2];
    if (!c)
	return v;
    if (! isdigit(s[2])) 
	return -1;
    v = v * 10 + (c - '0');
    if (s[3] || v > 31)
	return -1;
    return v;
}

static int LDI_getRegister()
{
    int	r, v;

    v = 0;
    r = LDI_getToken();
    if ((r != TOKEN_REGISTER) || ((v = LDI_getRegisterNumber(ldi_tmp)) < 0)) 
	myerror(SPRINTF("Line %d: %s\nA register is expected at (%s).", ldi_line_number, ldi_line, ldi_tmp));
    return v;
}

/** 
 * @param flag [in] flag 0: signed, 1: unsigned, 2: label only 
 */
static int  LDI_getImmediate(Instruction *pInstr, int c_alter, int flag)
{
    int	r;

    r = LDI_getToken();
    if (c_alter >= 0 && r == c_alter) {
	pInstr->immed = 0;
	return 0;
    }
    if (r != TOKEN_NUMBER && r != TOKEN_VARIABLE) 
	myerror(SPRINTF("Line %d: %s\nAn immediate is expected at (%s).", ldi_line_number, ldi_line, ldi_tmp));
    if (r == TOKEN_NUMBER) {
	char  *ep;
	long int l = strtol(ldi_tmp, &ep, 0);

	if (flag == 2) {
	    myerror(SPRINTF("Line %d: %s\nA label is expected (%s).", ldi_line_number, ldi_line, ldi_tmp));
	}

	if (*ep) 
	    myerror(SPRINTF("Line %d: %s\nA number is expected (%s).", ldi_line_number, ldi_line, ldi_tmp));
	pInstr->immed = l;
    }
    else {
	pInstr->immed = 0;
	pInstr->label_ind = addLabel(ldi_tmp, 0, 0);  
    }
    return 1;
}

static void LDI_expectChar(int c)
{
    int	r;

    r = LDI_getToken();
    if (r != c) { 
	if (c) 
	    myerror(SPRINTF("Line %d: %s\n'%c' is missing (%s).", ldi_line_number, ldi_line, c, ldi_tmp));
	else 
	    myerror(SPRINTF("Line %d: %s\nEnd of line is expected (%s).", ldi_line_number, ldi_line, ldi_tmp));
    }
}

// three registers
static	void LDI_instr_3reg(Instruction *pInstr)
{
    pInstr->rd = LDI_getRegister(); LDI_expectChar(',');
    pInstr->rs = LDI_getRegister(); LDI_expectChar(',');
    pInstr->rt = LDI_getRegister(); 
    LDI_expectChar(0);
}

// instruciotn with immediate
static	void LDI_instr_immediate(Instruction *pInstr)
{
    pInstr->rt = LDI_getRegister(); LDI_expectChar(',');
    pInstr->rs = LDI_getRegister(); LDI_expectChar(',');
    LDI_getImmediate(pInstr, -1, 0); // only signed for now. 
    LDI_expectChar(0);
    pInstr->rd = 0;
}

// load/store instructions
static	void LDI_instr_load_store(Instruction *pInstr)
{
    int	    r;

    pInstr->rt = LDI_getRegister(); LDI_expectChar(',');
    r = LDI_getImmediate(pInstr, '(', 0); 
    if (r) 
	LDI_expectChar('(');
    pInstr->rs = LDI_getRegister(); 
    LDI_expectChar(')');
    LDI_expectChar(0);
    pInstr->rd = 0;

    /* dst register is determined by simulator
    if (store) {
	pInstr->src2 = pInstr->dst;
	pInstr->dst = 0;
    }
    else {
	pInstr->src2 = 0;
    } */
}

// branch conditional instructions
static	void LDI_instr_cond_branch(Instruction *pInstr)
{
    pInstr->rs = LDI_getRegister(); LDI_expectChar(',');
    pInstr->rt = LDI_getRegister(); LDI_expectChar(',');
    LDI_getImmediate(pInstr, -1, 2); // only label 
    LDI_expectChar(0);
}

/* check if it is a valid instruction name
 */
static	int	get_instruction_type (char *s)
{
    int	    i;

    i = 0;
    while (gInstructionName[i]) {
	if (!strcmp(s, gInstructionName[i])) 
	    return i;
	i ++;
    }
    return -1;
}

static	int LDI_getInstruciton(Instruction *pInstr)
{
    int	    r, again, lpos, idx;

    do {
	r = LDI_getToken();
	if (!r)
	    return 0;
	if (r != TOKEN_VARIABLE) 
	    myerror(SPRINTF("Line %d: %s\nA label or valid instruction is expected.", ldi_line_number, ldi_line));

	again = 0;
	idx = get_instruction_type (ldi_tmp);

	pInstr->op_code = idx;
	switch (idx) {
	    case I_LW:
	    case I_SW:
		LDI_instr_load_store(pInstr);
		break;
	    case I_NOP:
		LDI_expectChar(0);
		break;
	    case I_ADDI: 
		LDI_instr_immediate(pInstr);
		break;
	    case I_ADD:
	    case I_SUB:
	    case I_AND:
	    case I_NOR:
	    case I_OR:
	    case I_XOR:
		LDI_instr_3reg(pInstr);
		break;
	    case I_BNE:
	    case I_BEQ:
		LDI_instr_cond_branch(pInstr);
		break;
	    default: 
		lpos = addLabel(ldi_tmp, pInstr->addr, 1);
		r = LDI_getToken();
		if (r != ':') 
		    myerror(SPRINTF("Line %d: %s\n':' is expected after label '%s'.", ldi_line_number, ldi_line, aLabels[lpos].name));
		pInstr->op_code = I_NOP;
		again = 1; 
		break;
	}
    } while (again);
    return 1;
}


/**	Load instructions into the array.
 *
 * @param fn in The file name where instrcutinos will be loaded.
 * @param pInstr_in in The array to store loaded instructions.
 * @param max_num_instructions The max number of instructions can be loaded. 
 * @return the number of instructions loaded.  Negative values for errors. 
 *
 **/
int	LDI_loadInstructions(char *fn, Instruction *pInstr_in, int max_num_instructions)
{
    FILE    *fp;
    Instruction *pInstr;
    int	    num_instr;

    num_Labels = 0;
    num_instr = 0;
    pInstr = pInstr_in;
    pInstr->addr = INDEX2PC(num_instr);
    pInstr->label_ind = -1;

    fp = fopen(fn, "r"); 
    if (fp == NULL) {
	myerror(SPRINTF("Cannot open file: %s", fn)); 
    }

    ldi_line_number = 0;
    while (fgets(ldi_line, MAX_LINE_LENGTH, fp)) {
	int r;

	ldi_line_number ++;
	ldi_line_ind = 0;

	r = LDI_getInstruciton(pInstr); 
	if (!r) // No instruction is found.
	    continue;
	num_instr ++;
	if (num_instr == max_num_instructions)
	    break;
	pInstr++;
	pInstr->addr = INDEX2PC(num_instr);
	pInstr->immed = 0;
	pInstr->label_ind = -1;
    }
    fclose(fp);

    LDI_processLabels(pInstr_in, num_instr); 
    return num_instr;
}

/* LDI_setNOP: set an instruction to NOP */
void	LDI_setNOP(Instruction *pInstr_in)
{
    pInstr_in->op_code = I_NOP;
    pInstr_in->rs = 0;
    pInstr_in->rt = 0;
    pInstr_in->rd = 0;
    pInstr_in->immed = 0;
    pInstr_in->label_ind = -1;
}

void LDI_print_3reg(Instruction *pInstr)
{
    printf ("\t$%d, $%d, $%d", pInstr->rd, pInstr->rs, pInstr->rt);
}

void LDI_print_immediate(Instruction *pInstr)
{
    printf ("\t$%d, $%d, %d # %04X", pInstr->rt, pInstr->rs, pInstr->immed, pInstr->immed & 0xFFFF);
}

void LDI_print_load_store (Instruction *pInstr)
{
    printf ("\t$%d, %d($%d) # %04X", pInstr->rt, pInstr->immed, pInstr->rs, pInstr->immed & 0xFFFF);
}

void LDI_print_cond_branch(Instruction *pInstr)
{
    printf ("\t$%d, $%d, %d # %04X", pInstr->rs, pInstr->rt, pInstr->immed, (pInstr->addr + (1+pInstr->immed)*4));
}

int	LDI_printInstructions(Instruction *pInstr, int n)
{
    int	    i;

    for (i = 0; i < n; i ++, pInstr ++) {
	printf("%04X:\t%s", pInstr->addr, gInstructionName[pInstr->op_code]);
	switch (pInstr->op_code) {
	    case I_ADD:
	    case I_SUB:
	    case I_AND:
	    case I_OR:
	    case I_NOR:
	    case I_XOR:
		LDI_print_3reg(pInstr);
		break;
	    case I_ADDI:
		LDI_print_immediate(pInstr);
		break;
	    case I_LW:
	    case I_SW:
		LDI_print_load_store(pInstr);
		break;
	    case I_BNE:
	    case I_BEQ:
		LDI_print_cond_branch(pInstr);
		break;
	    default: 
		break;
	}
	printf("\n");
    }
    return 1;
}

