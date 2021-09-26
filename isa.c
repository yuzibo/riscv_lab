#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "isa.h"


/* Are we running in GUI mode? */
extern int gui_mode;

/* Bytes Per Line = Block size of memory */
#define BPL 32

struct {
    char *name;
    int id;
} reg_table[REG_ERR+1] =
{
    {"x0",   REG_X0},
    {"x1",   REG_X1},
    {"x2",   REG_X2},
    {"x3",   REG_X3},
    {"x4",   REG_X4},
    {"x5",   REG_X5},
    {"x6",   REG_X6},
    {"x7",   REG_X7},
    {"x8",   REG_X8},
    {"x9",   REG_X9},
    {"a0",   REG_X10},
    {"a1",   REG_X11},
    {"a2",   REG_X12},
    {"a3",   REG_X13},
    {"a4",   REG_X14},
    {"a5",   REG_X15},
    {"a6",   REG_X16},
    {"a7",   REG_X17},
    {"x18",   REG_X18},
    {"x19",   REG_X19},
    {"x20",   REG_X20},
    {"x21",   REG_X21},
    {"x22",   REG_X22},
    {"x23",   REG_X23},
    {"x24",   REG_X24},
    {"x25",   REG_X25},
    {"x26",   REG_X26},
    {"x27",   REG_X27},
    {"x28",   REG_X28},
    {"x29",   REG_X29},
    {"x30",   REG_X30},
    {"x31",   REG_X31},
    {"pc",   REG_PC},
    {"----",   REG_NONE},
    {"----",   REG_ERR}
};


reg_id_t find_register(char *name)
{
    int i;
    for (i = 0; i < REG_NONE; i++)
	if (!strcmp(name, reg_table[i].name))
	    return reg_table[i].id;
    return REG_ERR;
}

char *reg_name(reg_id_t id)
{
    if (id >= 0 && id < REG_NONE)
	return reg_table[id].name;
    else
	return reg_table[REG_NONE].name;
}

/* Is the given register ID a valid program register? */
int reg_valid(reg_id_t id)
{
  return id >= 0 && id < REG_NONE && reg_table[id].id == id;
}




instr_t instruction_set[] =
{  //{name, icode, bytes, ifun1,ifun2}
   //if the instruction do not have ifun1/ifun2, write 0 in that position.
    {"lui", 0x37, 4, 0, 0 },
    {"auipc", 0x17, 4, 0, 0 },
    {"jal", 0x6f, 4, 0, 0 },
    {"jalr", 0x67, 4, 0, 0 },
    {"beq", 0x63, 4, 0, 0 },
////////////////////////////
    //PART A:add bne/blt/bge
    {"bne", 0x63, 4, 1, 0},
    {"blt", 0x63, 4, 4, 0},
    {"bge", 0x63, 4, 5, 0},


////////////////////////////
    {"bltu", 0x63, 4, 6, 0 },
    {"bgeu", 0x63, 4, 7, 0 },
////////////////////////////
    //PART C:add lw

////////////////////////////
    {"sw", 0x23, 4, 2, 0 },
////////////////////////////
    //PART B:add addi/slti/sltiu/xori/ori/andi/slli/srli/srai
    {"addi", 0x13, 4, 0, 0 },

////////////////////////////
    {"add", 0x33, 4, 0, 0 },
    {"sub", 0x33, 4, 0, 0x20 },
    {"sll", 0x33, 4, 1, 0 },
    {"slt", 0x33, 4, 2, 0 },
    {"sltu", 0x33, 4, 3, 0 },
    {"xor", 0x33, 4, 4, 0 },
    {"srl", 0x33, 4, 5, 0 },
    {"sra", 0x33, 4, 5, 0x20 },
    {"or", 0x33, 4, 6, 0 },
    {"and", 0x33, 4, 7, 0 },

    {"halt", 0x0, 4, 0, 0 }

};




instr_t invalid_instr =
    {"XXX", 0, 0, 0, 0 };

instr_ptr find_instr(char *name)
{
    int i;
    for (i = 0; instruction_set[i].name; i++)
	if (strcmp(instruction_set[i].name,name) == 0)
	    return &instruction_set[i];
    return NULL;
}

/* Return name of instruction given its encoding */
char *iname(int icode,int ifun1,int ifun2) {
    int i;
    for (i = 0; instruction_set[i].name; i++) {
	if (icode == instruction_set[i].code && ifun1 == instruction_set[i].ifun1 && ifun2 == instruction_set[i].ifun2)
	    return instruction_set[i].name;
    }
    return "<bad>";
}


instr_ptr bad_instr()
{
    return &invalid_instr;
}


mem_t init_mem(int len)
{

    mem_t result = (mem_t) malloc(sizeof(mem_rec));
    len = ((len+BPL-1)/BPL)*BPL;
    result->len = len;
    result->contents = (byte_t *) calloc(len, 1);
    return result;
}

void clear_mem(mem_t m)
{
    memset(m->contents, 0, m->len);
}

void free_mem(mem_t m)
{
    free((void *) m->contents);
    free((void *) m);
}

mem_t copy_mem(mem_t oldm)
{
    mem_t newm = init_mem(oldm->len);
    memcpy(newm->contents, oldm->contents, oldm->len);
    return newm;
}

bool_t diff_mem(mem_t oldm, mem_t newm, FILE *outfile)
{
    word_t pos;
    int len = oldm->len;
    bool_t diff = FALSE;
    if (newm->len < len)
	len = newm->len;
    for (pos = 0; (!diff || outfile) && pos < len; pos += 4) {
        word_t ov = 0;  word_t nv = 0;
	get_halfword_val(oldm, pos, &ov);
	get_halfword_val(newm, pos, &nv);
	if (nv != ov) {
	    diff = TRUE;
	    if (outfile)
		fprintf(outfile, "0x%.4x:\t0x%.8x\t0x%.8x\n", pos, ov, nv);
	}
    }
    return diff;
}

int hex2dig(char c)
{
    if (isdigit((int)c))
	return c - '0';
    if (isupper((int)c))
	return c - 'A' + 10;
    else
	return c - 'a' + 10;
}

#define LINELEN 4096
int load_mem(mem_t m, FILE *infile, int report_error)
{
    /* Read contents of .yo file */
    char buf[LINELEN];
    char c, ch, cl;
    int byte_cnt = 0;
    int lineno = 0;
    word_t bytepos = 0;
#ifdef HAS_GUI
    int empty_line = 1;
    int addr = 0;
    char hexcode[21];
    /* For display */
    int line_no = 0;
    char line[LINELEN];
    int index = 0;
#endif /* HAS_GUI */
    while (fgets(buf, LINELEN, infile)) {
	int cpos = 0;
#ifdef HAS_GUI
	empty_line = 1;
#endif
	lineno++;
	/* Skip white space */
	while (isspace((int)buf[cpos]))
	    cpos++;

	if (buf[cpos] != '0' ||
	    (buf[cpos+1] != 'x' && buf[cpos+1] != 'X'))
	    continue; /* Skip this line */
	cpos+=2;

	/* Get address */
	bytepos = 0;
	while (isxdigit((int)(c=buf[cpos]))) {
	    cpos++;
	    bytepos = bytepos*16 + hex2dig(c);
	}

	while (isspace((int)buf[cpos]))
	    cpos++;

	if (buf[cpos++] != ':') {
	    if (report_error) {
		fprintf(stderr, "Error reading file. Expected colon\n");
		fprintf(stderr, "Line %d:%s\n", lineno, buf);
		fprintf(stderr,
			"Reading '%c' at position %d\n", buf[cpos], cpos);
	    }
	    return 0;
	}

#ifdef HAS_GUI
	addr = bytepos;
	index = 0;
#endif

	while (isspace((int)buf[cpos]))
	    cpos++;

	/* Get code */
	while (isxdigit((int)(ch=buf[cpos++])) &&
	       isxdigit((int)(cl=buf[cpos++]))) {
	    byte_t byte = 0;
	    if (bytepos >= m->len) {
		if (report_error) {
		    fprintf(stderr,
			    "Error reading file. Invalid address. 0x%x\n",
			    bytepos);
		    fprintf(stderr, "Line %d:%s\n", lineno, buf);
		}
		return 0;
	    }
	    byte = hex2dig(ch)*16+hex2dig(cl);
	    m->contents[bytepos++] = byte;
	    byte_cnt++;
#ifdef HAS_GUI
	    empty_line = 0;
	    hexcode[index++] = ch;
	    hexcode[index++] = cl;
#endif
	}
#ifdef HAS_GUI
	/* Fill rest of hexcode with blanks.
	   Needs to be 2x longest instruction */
	for (; index < 20; index++)
	    hexcode[index] = ' ';
	hexcode[index] = '\0';

	if (gui_mode) {
	    /* Now get the rest of the line */
	    while (isspace((int)buf[cpos]))
		cpos++;
	    cpos++; /* Skip over '|' */

	    index = 0;
	    while ((c = buf[cpos++]) != '\0' && c != '\n') {
		line[index++] = c;
	    }
	    line[index] = '\0';
	    if (!empty_line)
		report_line(line_no++, addr, hexcode, line);
	}
#endif /* HAS_GUI */
    }
    return byte_cnt;
}

bool_t get_byte_val(mem_t m, word_t pos, byte_t *dest)
{
    if (pos < 0 || pos >= m->len)
	return FALSE;
    *dest = m->contents[pos];
    return TRUE;
}

bool_t get_halfword_val(mem_t m, word_t pos, word_t *dest)
{
    int i;
    word_t val;
    if (pos < 0 || pos + 4 > m->len)
	return FALSE;
    val = 0;
    for (i = 0; i < 4; i++) {
	word_t b =  m->contents[pos+i] & 0xFF;
	val = val | (b <<(8*i));
    }
    *dest = val;
    return TRUE;
}

bool_t get_riscv4byte_val(mem_t m, word_t pos, word_t *dest)
{
    int i;
    word_t val;
    if (pos < 0 || pos + 4 > m->len)
	return FALSE;
    val = 0;
    for (i = 0; i < 4; i++) {
	word_t b =  m->contents[pos+i] & 0xFF;
	val = (val << 8) | b;
    }
    *dest = val;
    return TRUE;
}

bool_t get_word_val(mem_t m, word_t pos, word_t *dest)
{
    int i;
    word_t val;
    if (pos < 0 || pos + 8 > m->len)
	return FALSE;
    val = 0;
    for (i = 0; i < 8; i++) {
	word_t b =  m->contents[pos+i] & 0xFF;
	val = val | (b <<(8*i));
    }
    *dest = val;
    return TRUE;
}

bool_t set_byte_val(mem_t m, word_t pos, byte_t val)
{
    if (pos < 0 || pos >= m->len)
	return FALSE;
    m->contents[pos] = val;
    return TRUE;
}

bool_t set_halfword_val(mem_t m, word_t pos, word_t val)
{
    int i;
    if (pos < 0 || pos + 4 > m->len)
	return FALSE;
    for (i = 0; i < 4; i++) {
	m->contents[pos+i] = (byte_t) val & 0xFF;
	val >>= 8;
    }
    return TRUE;
}

bool_t set_word_val(mem_t m, word_t pos, word_t val)
{
    int i;
    if (pos < 0 || pos + 8 > m->len)
	return FALSE;
    for (i = 0; i < 8; i++) {
	m->contents[pos+i] = (byte_t) val & 0xFF;
	val >>= 8;
    }
    return TRUE;
}

void dump_memory(FILE *outfile, mem_t m, word_t pos, int len)
{
    int i, j;
    while (pos % BPL) {
	pos --;
	len ++;
    }

    len = ((len+BPL-1)/BPL)*BPL;

    if (pos+len > m->len)
	len = m->len-pos;

    for (i = 0; i < len; i+=BPL) {
	word_t val = 0;
	fprintf(outfile, "0x%.4x:", pos+i);
	for (j = 0; j < BPL; j+= 4) {
	    get_halfword_val(m, pos+i+j, &val);
	    fprintf(outfile, " %.8x", val);
	}
    }
}

mem_t init_reg()
{
    return init_mem(128);
}

void free_reg(mem_t r)
{
    free_mem(r);
}

mem_t copy_reg(mem_t oldr)
{
    return copy_mem(oldr);
}

bool_t diff_reg(mem_t oldr, mem_t newr, FILE *outfile)
{
    word_t pos;
    int len = oldr->len;
    bool_t diff = FALSE;
    if (newr->len < len)
	len = newr->len;
    for (pos = 0; (!diff || outfile) && pos < len; pos += 4) {
        word_t ov = 0;
        word_t nv = 0;
	get_halfword_val(oldr, pos, &ov);
	get_halfword_val(newr, pos, &nv);
	if (nv != ov) {
	    diff = TRUE;
	    if (outfile)
		fprintf(outfile, "%s:\t0x%.8x\t0x%.8x\n",
			reg_table[pos/4].name, ov, nv);
	}
    }
    return diff;
}

word_t get_reg_val(mem_t r, reg_id_t id)
{
    word_t val = 0;
    if (id >= REG_NONE)
	return 0;
    get_halfword_val(r,id*4, &val);
    return val;
}

void set_reg_val(mem_t r, reg_id_t id, word_t val)
{
    if (id < REG_NONE) {
	set_halfword_val(r,id*4,val);
#ifdef HAS_GUI
	if (gui_mode) {
	    signal_register_update(id, val);
	}
#endif /* HAS_GUI */
    }
}

void dump_reg(FILE *outfile, mem_t r) {
    reg_id_t id;
    for (id = 0; reg_valid(id); id++) {
	fprintf(outfile, "   %s  ", reg_table[id].name);
    }
    fprintf(outfile, "\n");
    for (id = 0; reg_valid(id); id++) {
	word_t val = 0;
	get_halfword_val(r, id*4, &val);
	fprintf(outfile, " %x", val);
    }
    fprintf(outfile, "\n");
}

struct {
    char symbol;
    int id;
} alu_table[A_NONE+1] =
{
    {'+',   A_ADD},
    {'-',   A_SUB},
    {'&',   A_AND},
    {'^',   A_XOR},
    {'?',   A_NONE}
};

char op_name(alu_t op)
{
    if (op < A_NONE)
	return alu_table[op].symbol;
    else
	return alu_table[A_NONE].symbol;
}

word_t compute_alu(alu_t op, word_t argA, word_t argB)
{
    word_t val;
    switch(op) {
    case A_ADD:
	val = argA+argB;
	break;
    case A_SUB:
	val = argB-argA;
	break;
    case A_AND:
	val = argA&argB;
	break;
    case A_XOR:
	val = argA^argB;
	break;
    case A_OR:
	val = argA|argB;
	break;
    default:
	val = 0;
    }
    return val;
}

cc_t compute_cc(alu_t op, word_t argA, word_t argB)
{
    word_t val = compute_alu(op, argA, argB);
    bool_t zero = (val == 0);
    bool_t sign = ((word_t)val < 0);
    bool_t ovf;
    switch(op) {
    case A_ADD:
        ovf = (((word_t) argA < 0) == ((word_t) argB < 0)) &&
  	       (((word_t) val < 0) != ((word_t) argA < 0));
	break;
    case A_SUB:
        ovf = (((word_t) argA > 0) == ((word_t) argB < 0)) &&
	       (((word_t) val < 0) != ((word_t) argB < 0));
	break;
    case A_AND:
    case A_XOR:
    case A_OR:
	ovf = FALSE;
	break;
    default:
	ovf = FALSE;
    }
    return PACK_CC(zero,sign,ovf);

}

char *cc_names[8] = {
    "Z=0 S=0 O=0",
    "Z=0 S=0 O=1",
    "Z=0 S=1 O=0",
    "Z=0 S=1 O=1",
    "Z=1 S=0 O=0",
    "Z=1 S=0 O=1",
    "Z=1 S=1 O=0",
    "Z=1 S=1 O=1"};

char *cc_name(cc_t c)
{
    int ci = c;
    if (ci < 0 || ci > 7)
	return "???????????";
    else
	return cc_names[c];
}

/* Status types */

char *stat_names[] = { "BUB", "AOK", "HLT", "ADR", "INS", "PIP" };

char *stat_name(stat_t e)
{
    if (e < 0 || e > STAT_PIP)
	return "Invalid Status";
    return stat_names[e];
}

/**************** Implementation of ISA model ************************/

state_ptr new_state(int memlen)
{
    state_ptr result = (state_ptr) malloc(sizeof(state_rec));
    result->pc = 0;
    result->r = init_reg();
    result->m = init_mem(memlen);
    result->cc = DEFAULT_CC;
    return result;
}

void free_state(state_ptr s)
{
    free_reg(s->r);
    free_mem(s->m);
    free((void *) s);
}

state_ptr copy_state(state_ptr s) {
    state_ptr result = (state_ptr) malloc(sizeof(state_rec));
    result->pc = s->pc;
    result->r = copy_reg(s->r);
    result->m = copy_mem(s->m);
    result->cc = s->cc;
    return result;
}

bool_t diff_state(state_ptr olds, state_ptr news, FILE *outfile) {
    bool_t diff = FALSE;

    if (olds->pc != news->pc) {
	diff = TRUE;
	if (outfile) {
	    fprintf(outfile, "pc:\t0x%.16x\t0x%.16x\n", olds->pc, news->pc);
	}
    }
    if (olds->cc != news->cc) {
	diff = TRUE;
	if (outfile) {
	    fprintf(outfile, "cc:\t%s\t%s\n", cc_name(olds->cc), cc_name(news->cc));
	}
    }
    if (diff_reg(olds->r, news->r, outfile))
	diff = TRUE;
    if (diff_mem(olds->m, news->m, outfile))
	diff = TRUE;
    return diff;
}


/* Branch logic */
bool_t cond_holds(cc_t cc, cond_t bcond) {
    bool_t zf = GET_ZF(cc);
    bool_t sf = GET_SF(cc);
    bool_t of = GET_OF(cc);
    bool_t jump = FALSE;

    switch(bcond) {
    case C_YES:
	jump = TRUE;
	break;
    case C_LE:
	jump = (sf^of)|zf;
	break;
    case C_L:
	jump = sf^of;
	break;
    case C_E:
	jump = zf;
	break;
    case C_NE:
	jump = zf^1;
	break;
    case C_GE:
	jump = sf^of^1;
	break;
    case C_G:
	jump = (sf^of^1)&(zf^1);
	break;
    default:
	jump = FALSE;
	break;
    }
    return jump;
}


/* Execute single instruction.  Return status. */
stat_t step_state(state_ptr s, FILE *error_file)
{
    return STAT_AOK;
}
