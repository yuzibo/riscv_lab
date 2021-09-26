char simname[] = "Risc-v Processor: seq";
#include <stdio.h>
#include "isa.h"
#include "sim.h"
int sim_main(int argc, char *argv[]);
word_t gen_pc(){return 0;}
int main(int argc, char *argv[])
  {return sim_main(argc,argv);}

/////////////////////////////
//PART B: you need add (icode)==(...) in right place.
//PART C: you need add (icode)==(...) in right place.Be careful, in some places you will change more than that.

//if the instruction has ifun1
long long gen_need_ifun1()
{
    return ((icode)==(I_JALR) || (icode)==(I_B) || (icode)==(I_S) || (icode)==(I_R) || (icode)==(I_CSR) || (icode) == (I_OP));
}
//if the instruction has ifun2
long long gen_need_ifun2()
{
    return ((icode)==(I_R));
}
//if the instruction is valid
long long gen_instr_valid()
{
    return ((icode)==(I_HALT) || (icode)==(I_LUI) || (icode)==(I_AUIPC) || (icode)==(I_JAL) ||
		 (icode)==(I_JALR) || (icode)==(I_B) || (icode)==(I_S) ||
		(icode)==(I_R) || (icode)==(I_CSR) || (icode)==(I_OP));
}
//if the instruction has rs1
long long gen_need_rs1()
{
    return ((icode)==(I_JALR) || (icode)==(I_B) || (icode)==(I_S) ||
		(icode)==(I_R) || (icode)==(I_CSR) || (icode)==(I_OP));
}
//if the instruction has rs2
long long gen_need_rs2()
{
    return ((icode)==(I_B) || (icode)==(I_S) || (icode)==(I_R));
}
//if the instruction has imm
long long gen_need_valC()
{
    return ((icode)==(I_LUI) || (icode)==(I_AUIPC) || (icode)==(I_JAL) ||
		 (icode)==(I_JALR) || (icode)==(I_B) || (icode)==(I_S));
}
//if the instruction has rd
long long gen_need_rd()
{
    return ((icode)==(I_LUI) || (icode)==(I_AUIPC) || (icode)==(I_JAL) ||
		 (icode)==(I_JALR) || (icode)==(I_R) || (icode)==(I_CSR) || (icode)==(I_OP));
}
//get the value of rs1 if the instruction has rs1
long long gen_srcA()
{
    return (((icode)==(I_JALR) || (icode)==(I_B) || (icode)==(I_S) || (icode)==(I_R)) ? (rs1) : (REG_NONE));
}
//get the value of rs2 if the instruction has rs2
long long gen_srcB()
{
    return (((icode)==(I_B) || (icode)==(I_S) || (icode)==(I_R)) ? (rs2) : (REG_NONE));
}
//write the value calculated by ALU to rd
long long gen_dstE()
{
    return (((icode)==(I_LUI) || (icode)==(I_AUIPC) || (icode)==(I_JAL) || (icode)==(I_JALR) || (icode)==(I_R)) ? (rd) : (REG_NONE));
}
//write the value in memory to rd
long long gen_dstM()
{
    return (REG_NONE);
}
//in alu, there are two operands,one is in aluA, another is in aluB
long long gen_aluA()
{
    return (((icode)==(I_JALR) || (icode)==(I_B) || (icode)==(I_S) || (icode)==(I_R)) ? (vala) : (((icode)==(I_AUIPC)) ? (pc) : 0));
}

long long gen_aluB()
{
    return (((icode)==(I_B) || (icode)==(I_R)) ? (valb) : (((icode)==(I_LUI) || (icode)==(I_AUIPC) || (icode)==(I_JAL) || (icode)==(I_JALR) || (icode)==(I_S)) ? (valc) : 0));
}
//if the instruction needs to read data from memory
long long gen_mem_read()
{
    return 0;
}
//if the instruction needs to write data from memory
long long gen_mem_write()
{
    return ((icode) == (I_S));
}
//the address of the memory
long long gen_mem_addr()
{
    return (((icode)==(I_S)) ? (vale) : 0);
}
//the data which will be written into memory
long long gen_mem_data()
{
    return (((icode)==(I_S)) ? (valb) : 0);
}
//get the Stat(like the Y86)
long long gen_Stat()
{
    return (((imem_error) | (dmem_error)) ? (STAT_ADR) : !(instr_valid) ?
      (STAT_INS) : ((icode) == (I_HALT)) ? (STAT_HLT) : (STAT_AOK));
}
//the new pc
long long gen_new_pc()
{
    return (((icode)==(I_B) && (cond)) ? (valc+pc) : (((icode)==(I_JAL) || (icode)==(I_JALR)) ? (vale) : (valp)));
}
//////////////////////////////////////
