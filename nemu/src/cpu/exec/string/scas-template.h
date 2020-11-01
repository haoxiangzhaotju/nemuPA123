#include "cpu/exec/template-start.h"

#define instr scas

make_helper(concat(scas_n_,SUFFIX)){
	swaddr_t s1=REG(R_EAX),s2=swaddr_read(reg_l(R_EDI),DATA_BYTE);
	uint32_t res=s1-s2;
	if(cpu.DF==0)
	{
		reg_l(R_EDI)+=DATA_BYTE;
	}else{
		reg_l(R_EDI)-=DATA_BYTE;
	}
	int len = (DATA_BYTE<<3)-1;
	cpu.ZF = !res;
	cpu.SF = res >> ((DATA_BYTE <<3)-1);
	cpu.CF=s1<s2;
	//int temp1 = (s1) >> ((DATA_BYTE<<3)-1);
	//int temp2 = (s2) >> ((DATA_BYTE<<3)-1);
	cpu.OF=(((s1>>len)!=(s2>>len))&&((s2>>len)==cpu.SF));
	res ^= res >>4;
	res ^= res >>2;
	res ^= res >>1;
	res &=1;
	cpu.PF = !res;
	print_asm("scas%s",str(SUFFIX));

	return 1;
}


#include "cpu/exec/template-end.h"
				
