#include "cpu/exec/template-start.h"

#define instr lods

static void do_execute(){
	if(ops_decode.is_operand_size_16){
		src = swaddr_read(reg_w(R_SI),DATA_BYTE);
		swaddr_write(reg_w(R_AX),2,src);
		if(cpu.DF==0)reg_w(R_DI)+=DATA_BYTE;
		else reg_w(R_DI)-=DATA_BYTE;
	}
	else{
		src=swaddr_read(reg_l(R_ESI),2,DATA_BYTE);
		swaddr_write(reg_l(R_EAX),4,src);
		if(cpu.DF==0)reg_l(R_EDI)+=DATA_BYTE;
		else reg_l(R_EDI)-=DATA_BYTE;
	}
	print_asm("lods");
}

make_instr_helper(n)

#include "cpu/exec/template-end.h"
