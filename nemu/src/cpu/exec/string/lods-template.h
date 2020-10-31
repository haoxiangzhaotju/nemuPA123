#include "cpu/exec/template-start.h"

#define instr lods

static void do_execute(){
//	DATA_TYPE src;
	if(ops_decoded.is_operand_size_16){
		//src = swaddr_read(reg_w(R_SI),DATA_BYTE);
		swaddr_write(REG(R_AX),2,swaddr_read(reg_w(R_SI),4));
		if(cpu.DF==0)reg_w(R_SI)+=DATA_BYTE;
		else reg_l(R_SI)-=DATA_BYTE;
	}
	else{
		//src=swaddr_read(reg_l(R_ESI),DATA_BYTE);
		swaddr_write(REG(R_EAX),4,swaddr_read(reg_l(R_ESI),4));
		if(cpu.DF==0)reg_l(R_ESI)+=DATA_BYTE;
		else reg_l(R_ESI)-=DATA_BYTE;
	}
	print_asm("lods");
}

make_instr_helper(n)

#include "cpu/exec/template-end.h"
