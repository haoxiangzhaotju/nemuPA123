#include "cpu/exec/template-start.h"

#define instr shl

static void do_execute () {
	DATA_TYPE src = op_src->val;
	DATA_TYPE dest = op_dest->val;

	uint8_t count = src & 0x1f;
	dest <<= count;
	OPERAND_W(op_dest, dest);
	int len = (DATA_BYTE << 3) - 1;
	cpu.CF=0;
	cpu.OF=0;
	cpu.SF=dest >> len;
    	cpu.ZF=!dest;
		dest ^= dest >>4;
	dest ^= dest >>2;
	dest ^= dest >>1;
	cpu.PF=!(dest & 1);
	/* TODO: Update EFLAGS. */
	//panic("please implement me");

	print_asm_template2();
}

make_instr_helper(rm_1)
make_instr_helper(rm_cl)
make_instr_helper(rm_imm)

#include "cpu/exec/template-end.h"
