#include "cpu/exec/template-start.h"

#define instr scas

static void do_execute(){
	DATA_TYPE src,dest;
	if (ops_decoded.is_operand_size_16)
	{
		src = swaddr_read (reg_w(R_AX),DATA_BYTE);
//current_sreg = R_DS;
		dest = swaddr_read (reg_w (R_DI),DATA_BYTE);		
		if(cpu.DF==0)reg_w(R_DI)+=DATA_BYTE;
		else reg_w(R_DI)-=DATA_BYTE;
	}
	else
	{
		src = swaddr_read(reg_l(R_EAX),DATA_BYTE);
		dest = swaddr_read (reg_l (R_EDI),DATA_BYTE);
		if(cpu.DF==0)reg_l(R_EDI)+=DATA_BYTE;
		else reg_w(R_EDI)-=DATA_BYTE;
	}
	DATA_TYPE result = src- dest;
	int len = (DATA_BYTE << 3) - 1;
	cpu.CF = dest > src;
	cpu.SF= result >> len;
    	int s1,s2;
	s1=dest>>len;
	s2=src>>len;
    	cpu.OF=(s1 != s2 && s2 == cpu.SF) ;
    	cpu.ZF=!result;
	result ^= result >>4;
	result ^= result >>2;
	result ^= result >>1;
	cpu.PF=!(result & 1);
	print_asm("cmps");
}

make_instr_helper(n)

#include "cpu/exec/template-end.h"
				
