/***********************************************************************
 *
 * ssim.c - Sequential RISC-V simulator
 *
 ***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include "isa.h"
#include "sim.h"

#define MAXARGS 128
#define MAXBUF 1024

/***************
 * Begin Globals
 ***************/

/* Simulator name defined and initialized by the compiled HCL file */
/* according to the -n argument supplied to hcl2c */
extern char simname[];

/* Parameters modifed by the command line */
char *object_filename;   /* The input object file name. */
FILE *object_file;       /* Input file handle */
bool_t verbosity = 2;    /* Verbosity level [TTY only] (-v) */
word_t instr_limit = 10000; /* Instruction limit [TTY only] (-l) */
bool_t do_check = FALSE; /* Test with YIS? [TTY only] (-t) */

/*************
 * End Globals
 *************/


/***************************
 * Begin function prototypes
 ***************************/

static void usage(char *name);           /* Print helpful usage message */
static void run_tty_sim();               /* Run simulator in TTY mode */


/*************************
 * End function prototypes
 *************************/


/*******************************************************************
 * Part 1: This part is the initial entry point that handles general
 * initialization. It parses the command line and does any necessary
 * setup to run in either TTY or GUI mode, and then starts the
 * simulation.
 *******************************************************************/

/*
 * sim_main - main simulator routine. This function is called from the
 * main() routine in the HCL file.
 */
int sim_main(int argc, char **argv)
{
    int i;
    int c;

    /* Parse the command line arguments */
    while ((c = getopt(argc, argv, "htgl:v:")) != -1) {
	switch(c) {
	case 'h':
	    usage(argv[0]);
	    break;
	case 'l':
	    instr_limit = atoll(optarg);
	    break;
	case 'v':
	    verbosity = atoi(optarg);
	    if (verbosity < 0 || verbosity > 2) {
		printf("Invalid verbosity %d\n", verbosity);
		usage(argv[0]);
	    }
	    break;
	case 't':
	    do_check = TRUE;
	    break;
	default:
	    printf("Invalid option '%c'\n", c);
	    usage(argv[0]);
	    break;
	}
    }


    /* Do we have too many arguments? */
    if (optind < argc - 1) {
	printf("Too many command line arguments:");
	for (i = optind; i < argc; i++)
	    printf(" %s", argv[i]);
	printf("\n");
	usage(argv[0]);
    }


    /* The single unflagged argument should be the object file name */
    object_filename = NULL;
    object_file = NULL;
    if (optind < argc) {
	object_filename = argv[optind];
	object_file = fopen(object_filename, "r");
	if (!object_file) {
	    fprintf(stderr, "Couldn't open object file %s\n", object_filename);
	    exit(1);
	}
    }

    /* Otherwise, run the simulator in TTY mode (no -g flag) */
    run_tty_sim();

    exit(0);
}

/*
 * run_tty_sim - Run the simulator in TTY mode
 */
static void run_tty_sim()
{
    word_t icount = 0;//the number of instruction
    status = STAT_AOK;
    word_t byte_cnt = 0;//the number of byte
    mem_t mem0, reg0;//initial value
    state_ptr isa_state = NULL;//


    /* In TTY mode, the default object file comes from stdin */
    if (!object_file) {
	object_file = stdin;
    }

    /* Initializations */
    if (verbosity >= 2)
	sim_set_dumpfile(stdout);
    sim_init();

    /* Emit simulator name */
    printf("%s\n", simname);

    byte_cnt = load_mem(mem, object_file, 1);
    if (byte_cnt == 0) {
	fprintf(stderr, "No lines of code found\n");
	exit(1);
    } else if (verbosity >= 2) {
	printf("%d bytes of code read\n", byte_cnt);
    }
    fclose(object_file);

    if (do_check) {
	isa_state = new_state(0);
	free_mem(isa_state->r);
	free_mem(isa_state->m);
	isa_state->m = copy_mem(mem);
	isa_state->r = copy_mem(reg);
    }

    mem0 = copy_mem(mem);
    reg0 = copy_mem(reg);


    icount = sim_run(instr_limit, &status);

    if (verbosity > 0) {
	printf("%d instructions executed\n", icount);
	printf("Status = %s\n", stat_name(status));
	printf("Changed Register State:\n");
	diff_reg(reg0, reg, stdout);
	printf("Changed Memory State:\n");
	diff_mem(mem0, mem, stdout);
    }
}



/*
 * usage - print helpful diagnostic information
 */
static void usage(char *name)
{
    printf("Usage: %s [-htg] [-l m] [-v n] file.yo\n", name);
    printf("file.yo required in GUI mode, optional in TTY mode (default stdin)\n");
    printf("   -h     Print this message\n");
    printf("   -g     Run in GUI mode instead of TTY mode (default TTY)\n");
    printf("   -l m   Set instruction limit to m [TTY mode only] (default %d)\n", instr_limit);
    printf("   -v n   Set verbosity level to 0 <= n <= 2 [TTY mode only] (default %d)\n", verbosity);
    printf("   -t     Test result against ISA simulator (yis) [TTY mode only]\n");
    exit(0);
}



/*********************************************************
 * Part 2: This part contains the core simulator routines.
 *********************************************************/

/**********************
 * Begin Part 2 Globals
 **********************/

mem_t mem;  /* Instruction and data memory */

mem_t reg;               /* Register file */

/* Program Counter */
word_t pc = 0; /* Program counter value */
word_t pc_in = 0;/* Input to program counter */

/* Intermediate values */
byte_t icode = I_NOP;
word_t ifun1 = 0;
word_t ifun2 = 0;
word_t instr = 0;
word_t rs1 = REG_NONE;
word_t rs2 = REG_NONE;
word_t rd = REG_NONE;
word_t valc = 0;
word_t valp = 0;
bool_t imem_error;
bool_t instr_valid;

word_t srcA = REG_NONE;
word_t srcB = REG_NONE;
word_t destE = REG_NONE;
word_t destM = REG_NONE;
word_t vala = 0;
word_t valb = 0;
word_t vale = 0;

bool_t cond = FALSE;
word_t valm = 0;
bool_t dmem_error;

bool_t mem_write = FALSE;
word_t mem_addr = 0;
word_t mem_data = 0;
byte_t status = STAT_AOK;


/* Values computed by control logic */
word_t gen_need_ifun1();
word_t gen_need_ifun2();
word_t gen_need_rs1();
word_t gen_need_rs2();
word_t gen_need_rd();
word_t gen_need_valC();
word_t gen_instr_valid();
word_t gen_srcA();
word_t gen_srcB();
word_t gen_dstE();
word_t gen_dstM();
word_t gen_aluA();
word_t gen_aluB();
word_t gen_mem_addr();
word_t gen_mem_data();
word_t gen_mem_read();
word_t gen_mem_write();
word_t gen_Stat();
word_t gen_new_pc();

/* Log file */
FILE *dumpfile = NULL;

/********************
 * End Part 2 Globals
 ********************/

static int initialized = 0;
void sim_init()
{

    /* Create memory and register files */
    initialized = 1;
    mem = init_mem(MEM_SIZE);
    reg = init_reg();
    sim_reset();
    clear_mem(mem);
}

void sim_reset()
{
    if (!initialized)
	sim_init();
    clear_mem(reg);

    set_reg_val(reg, REG_X0, 0);
    pc_in = 0;

    destE = REG_NONE;
    destM = REG_NONE;
    mem_write = FALSE;
    mem_addr = 0;
    mem_data = 0;

    /* Reset intermediate values to clear display */
    icode = I_NOP;
    ifun1 = 0;
    ifun2 = 0;
    instr = 0;
    rs1 = REG_NONE;
    rs2 = REG_NONE;
    rd = REG_NONE;
    valc = 0;
    valp = 0;

    srcA = REG_NONE;
    srcB = REG_NONE;
    destE = REG_NONE;
    destM = REG_NONE;
    vala = 0;
    valb = 0;
    vale = 0;

    cond = FALSE;
    valm = 0;

}

/* Update the processor state */
static void update_state()
{

    pc = pc_in;

    /* Writeback */
    //writeback vale to destE
    //REG_X0 can not change
    // vale 是经过alu计算出来的存入到dstE
    if (destE != REG_NONE && destE != REG_X0 ){
	set_reg_val(reg, destE, vale);

    }
////////////////////////////////////
    //PART C: writeback valm to destM
    // 从内存取出来的值放入 dstM
    if (destM != REG_NONE && destM != REG_X0) {
    set_reg_val(reg, destM, valm);

    }

////////////////////////////////////
    if (mem_write) {
      /* Should have already tested this address */
      set_halfword_val(mem, mem_addr, mem_data);
	sim_log("Wrote 0x%x to address 0x%x\n", mem_data, mem_addr);
    }
}

/* Execute one instruction */
/* Return resulting status */
static byte_t sim_step()
{
//int
    word_t aluA;
    word_t aluB;
//unsigned int
    uword_t aluA1;
    uword_t aluB1;

    cond = FALSE;

    status = STAT_AOK;
    imem_error = dmem_error = FALSE;

    update_state(); /* Update state from last cycle */

    valp = pc;

    instr = 0;
    imem_error = !get_riscv4byte_val(mem, valp, &instr);
    if (imem_error) {
	sim_log("Couldn't fetch at address 0x%x\n", valp);
    }

    //get icode
    if(imem_error){
	icode = 0;
    }
    else{
	icode = instr&0x7f;
    }

    //get ifun1,ifun2 if have
    if(gen_need_ifun1()){
	ifun1 = (instr >> 12)&0x7;
    }
    else ifun1=0;

    if(gen_need_ifun2()){
	ifun2 = (instr >> 25)&0x7f;
    }
    else {
		ifun2 = 0; // original
    }

    instr_valid = gen_instr_valid();

    //get rs1,rs2,rd if have
    if(gen_need_rs1()){
	rs1 = (instr >> 15)&0x1f;
    }
    else{
	rs1 = REG_NONE;
    }

    if(gen_need_rs2()){
	rs2 = (instr >> 20)&0x1f;
    }
    else{
	rs2 = REG_NONE;
    }

    if(gen_need_rd()){
	rd = (instr >> 7)&0x1f;
    }
    else{
	rd = REG_NONE;
    }

    valc = 0;
//get the immediate data
    if (gen_need_valC()) {
	if((icode)==(I_LUI) || (icode)==(I_AUIPC) ){
		valc = ((instr >> 12)&0xfffff) << 12;
	}
	if((icode)==(I_JAL) ){
		valc = valc | (((instr>>31)&0x1)<<20) | (((instr>>21)&0x3ff)<<1) | (((instr>>20)&0x1)<<11) | (((instr>>12)&0xff)<<12);
		valc = (valc << 11) >> 11;
	}
	if((icode)==(I_JALR)){
		valc = (instr >> 20)&0xfff;
		valc = (valc << 20) >> 20; // 先保留符号位

	}
/////////////////////////////////////////////
	//PART B: get the immediate data of addi/slti/sltiu/xori/ori/andi/slli/srli/srai
	// for addi
	if((icode)==(I_OP)){
		if((ifun1 == 1) || (ifun1 == 5)){
			valc = ((instr >> 20)&0x3f);
		} else {
			valc = (((int)instr >> 20)&0xfff);
			valc = (valc << 20) >> 20;
		}
	}
	//PART C: get the immediate data of lw
    if((icode)==(I_L)){
        valc = ((instr >> 20)&0xfff);
        valc = (valc << 20) >> 20;
        //valc = valc;
    }


/////////////////////////////////////////////
	if((icode)==(I_B) ){
		valc = valc | (((instr>>31)&0x1)<<12) | (((instr>>25)&0x3f)<<5) | (((instr>>8)&0xf)<<1) | (((instr>>7)&0x1)<<11);
		valc = (valc << 19) >> 19;
	}
	if((icode)==(I_S) ){
		valc = valc | (((instr>>25)&0x7f)<<5) | ((instr>>7)&0x1f);
		valc = (valc << 20) >> 20;
	}

    }
    else {
	valc = 0;
    }
//each instructions is 4 byte
    valp+=4;
//output related information
// 以上就是译码部分

    sim_log("IF: Fetched %s at 0x%x.  rs1=%s, rs2=%s, rd=%s, Imm = 0x%x\n",
	    iname(icode,ifun1,ifun2), pc, reg_name(rs1), reg_name(rs2), reg_name(rd), valc);
//we already have icode,ifun1,ifun2,rs1,rs2,rd,imm

    if (status == STAT_AOK && icode == 0) {
	status = STAT_HLT;
    }
//we are going to get vala,valb, cond? ,destE,destM,aluA,aluB,
    srcA = gen_srcA();
    if (srcA != REG_NONE) {
	vala = get_reg_val(reg, srcA);
    } else {
	vala = 0;
    }

    srcB = gen_srcB();
    if (srcB != REG_NONE) {
	valb = get_reg_val(reg, srcB);
    } else {
	valb = 0;
    }

    destE = gen_dstE();
    destM = gen_dstM();
    aluA = gen_aluA();
    aluB = gen_aluB();

//determine which function will be used
    switch(icode){
	case I_B:
		//in this ifun, if the comparison is correct, make cond true.
		switch(ifun1){
			case 0:
				if(aluA == aluB) cond = TRUE;
				break;
//////////////////////////////////////
/*PART A: supplement the function of bne/ble/bge here */
			// for bne
			case 1:
				if(aluA != aluB) cond = TRUE;
				break;
			// for ble
			case 4:
				if(aluA < aluB) cond = TRUE;
				break;
			case 5:
				if(aluA > aluB) cond = TRUE;
				break;
//////////////////////////////////////


//////////////////////////////////////
			case 6:
				aluA1 = aluA;
				aluB1 = aluB;
				if(aluA1 < aluB1) cond = TRUE;
				break;
			case 7:
				aluA1 = aluA;
				aluB1 = aluB;
				if(aluA1 >= aluB1) cond = TRUE;
				break;
		}
		break;
////////////////////// part c
    case I_L:
        switch(ifun1){
            case 2:
                vale = aluA + valc;
                break;
        }
        break;
////////////////////////////
	//PART B: supplement the function of addi/slti/sltiu/xori/ori/andi/slli/srli/srai
	case I_OP:
		switch(ifun1){
			case 0: // addi
					vale = valc + aluA;
					break;

			case 1: // slli, it is wrong!  notes:
					vale = (aluA << (valc&0x3f));
                    //  rd = rs1 << shamt
				break;

			case 2: //slti It is wrong
				if(aluA < valc){
					vale = 1;
					break;
				} else {
					vale = 0;
					break;
				}
			case 3: //sltiu
				if(aluA > valc) {
					vale = 1;
					break;
				} else {
					vale = 0;
					break;
				}
			case 4: // xori
				vale = aluA ^ valc;
				break;
			case 5: // srli and srai
				if (ifun2 == 0x20){
					//valc = 0x4;
					vale = (aluA >> valc);
					break;
				} else {
					vale = ((unsigned int)aluA >> valc);
					break;

				}
				break;

			case 6: // ori
				vale = aluA | valc;
				break;
			case 7: // andi
				vale = aluA & valc;
				break;
		}
		break;
////////////////////////////
	case I_R:
		switch(ifun1){
			case 0:
				if(ifun2 == 0){
					vale = aluA+aluB;
					break;
				}
				else{
					vale = aluA-aluB;
					break;
				}
			case 1:
				vale = aluA<<aluB;
				break;
			case 2:
				if(aluA < aluB) vale = 1;
				else vale = 0;
				break;
			case 3:
				aluA1 = aluA;
				aluB1 = aluB;
				if(aluA1 < aluB1) vale = 1;
				else vale = 0;
				break;
			case 4:
				vale = aluA^aluB;
				break;
			case 5:
				if(ifun2 != 0){
					vale = aluA>>aluB;
					break;
				}
				else{
					aluA1 = aluA;
					vale = aluA1>>aluB;
					break;
				}
			case 6:
				vale = aluA|aluB;
				break;
			case 7:
				vale = aluA&aluB;
				break;


			default:
				vale = 0;
		}
		break;

	default:
		vale = aluA+aluB;
		break;
    }
//   alu 单元的运算


//get the address of memory and the data which will be written into memory
    mem_addr = gen_mem_addr();
    mem_data = gen_mem_data();

    //if need read, read the data from mem_addr to valm
    if (gen_mem_read()) {
      dmem_error = dmem_error || !get_halfword_val(mem, mem_addr, &valm);
      if (dmem_error) {
	sim_log("Couldn't read at address 0x%x\n", mem_addr);
      }
    } else
      valm = 0;

    mem_write = gen_mem_write();
    if (mem_write) {
      /* Do a test read of the data memory to make sure address is OK */
      word_t junk;
      dmem_error = dmem_error || !get_halfword_val(mem, mem_addr, &junk);
    }

//change the state
    status = gen_Stat();


    /* Update PC */
    pc_in = gen_new_pc();
//in jal and jalr,pc+4 will be writen into rd
    if(((icode)==(I_JAL) || (icode)==(I_JALR)))
	vale = valp;

    return status;
}

/*
  Run processor until one of following occurs:
  - An error status is encountered in WB.
  - max_instr instructions have completed through WB

  Return number of instructions executed.
  if statusp nonnull, then will be set to status of final instruction
*/
word_t sim_run(word_t max_instr, byte_t *statusp)
{
    word_t icount = 0;
    byte_t run_status = STAT_AOK;
    while (icount < max_instr) {
	run_status = sim_step();
	icount++;
	if (run_status != STAT_AOK)
	    break;
    }
    if (statusp)
	*statusp = run_status;
    return icount;
}

/* If dumpfile set nonNULL, lots of status info printed out */
void sim_set_dumpfile(FILE *df)
{
    dumpfile = df;
}

/*
 * sim_log dumps a formatted string to the dumpfile, if it exists
 * accepts variable argument list
 */
void sim_log( const char *format, ... ) {
    if (dumpfile) {
	va_list arg;
	va_start( arg, format );
	vfprintf( dumpfile, format, arg );
	va_end( arg );
    }
}


