#include "WTVTypes.h"
#include "boxansi.h"
#include "Exceptions.h"
#include "iregdef.h"

#define OP_MASK		0xfc000000	/* opcode */

/* I-TYPE instructions */
#define RS_MASK		0x03e00000  /* src register */
#define RT_MASK		0x001f0000	/* test register */
#define IM_MASK		0x0000ffff	/* immediate value */

/* J-TYPE insructions */
#define TARG_MASK	0x03ffffff	/* jump target */

/* R-TYPE instructions */
#define RD_MASK		0x0000f800  /* dest register */
#define F_MASK		0x0000003f  /* fucntion field */

#define SPECIAL	0x00
#define REGIMM	0x01

#define BLTZ	0x00
#define BGEZ	0x01
#define J		0x02
#define JAL		0x03
#define BEQ		0x04
#define BNE		0x05	
#define BLEZ	0x06
#define BGTZ	0x07
#define JR		0x08
#define JALR	0x09
#define BLTZAL	0x10
#define BGEZAL	0x11

typedef struct {
	unsigned op:6;
	unsigned rs:5;
	unsigned rt:5;
	signed imm:16;
} ITYPE;

typedef struct {
	unsigned op:6;
	unsigned target:26;
} JTYPE;

typedef struct {
	unsigned op:6;
	unsigned rs:5;
	unsigned rt:5;
	unsigned rd:5;
	unsigned shift:5;
	unsigned func:6;
} RTYPE;

typedef union {
	JTYPE	jtype;
	ITYPE	itype;
	RTYPE	rtype;
} Instruction;

ulong *NextInstrAddr(ulong *addr,uchar over,uchar predict);
ulong *TestBranch(ulong *addr,uchar cond);

ulong *TestBranch(ulong *addr,uchar cond,uchar predict)
{
	ulong *next;
	Instruction theInstruction;

	theInstruction.itype = *(ITYPE*)addr;

	if(cond)
	{
		next = (ulong*)((theInstruction.itype.imm<<2) + 4 + (ulong)addr);
		if(predict)
			printf("Will branch\n");
	}
	else
	{
		next = addr+2;
		if(predict)
			printf("Will not branch\n");
	}
	return next;
}

/* 	
 	given a ptr to an instruction, returns pointer to next instruction 
	
 	note that instruction encodings give register number, which we must
	tweak to point to the correct register in the except_regs array.
	
	the first element in the array is R_R1 which has the value 0, so
	we must subtract 1 from what the encoding says.
*/

ulong *NextInstrAddr(ulong *addr,uchar over,uchar predict)
{
	ulong instr;
	ulong op;
	ulong *next;
	Instruction theInstruction;
	ulong rs,rt;
	
	instr = *addr;
	op = (instr & OP_MASK) >> 26;	/* get opcode */
	
	/* get function code if it's a wacky type */
	switch(op)
	{
		case SPECIAL:
			op = instr & F_MASK;
			switch(op)
			{
				case JR:
				case JALR:
						theInstruction.jtype = *(JTYPE*)addr;
						if(over)
							next = addr+2;
						else
							next = (ulong*)except_regs[theInstruction.itype.rs-1];
					break;
				default:
					next = addr+1;
					break;
			}
			break;
		case REGIMM:
			op = (instr & RT_MASK) >> 16;
			theInstruction.itype = *(ITYPE*)addr;
			rs = (theInstruction.itype.rs == 0) ? 0 : except_regs[theInstruction.itype.rs-1];
			switch(op)
			{		
				case BGEZ:
				case BGEZAL:
					next = TestBranch(addr,(long)rs >= 0,predict);
					break;
				case BLTZ:
				case BLTZAL:
					next = TestBranch(addr,(long)rs < 0,predict);
					break;
				default:
					next = addr+1;
			}
			break;
		case J:
		case JAL:
			theInstruction.jtype = *(JTYPE*)addr;
			if(over)
				next = addr+2;
			else
				next = (ulong*)((theInstruction.jtype.target<<2) | ((ulong)(addr+1) & 0xFFF00000));
			break;
		case BEQ:
			theInstruction.itype = *(ITYPE*)addr;
			rs = (theInstruction.itype.rs == 0) ? 0 : except_regs[theInstruction.itype.rs-1];
			rt = (theInstruction.itype.rt == 0) ? 0 : except_regs[theInstruction.itype.rt-1];
			next = TestBranch(addr,rs == rt,predict);
			break;
		case BNE:	
			theInstruction.itype = *(ITYPE*)addr;
			rs = (theInstruction.itype.rs == 0) ? 0 : except_regs[theInstruction.itype.rs-1];
			rt = (theInstruction.itype.rt == 0) ? 0 : except_regs[theInstruction.itype.rt-1];
			next = TestBranch(addr,rs != rt,predict);
			break;
		case BLEZ:
			theInstruction.itype = *(ITYPE*)addr;
			rs = (theInstruction.itype.rs == 0) ? 0 : except_regs[theInstruction.itype.rs-1];
			next = TestBranch(addr,(long)rs <= 0,predict);
			break;
		case BGTZ:
			theInstruction.itype = *(ITYPE*)addr;
			rs = (theInstruction.itype.rs == 0) ? 0 : except_regs[theInstruction.itype.rs-1];
			next = TestBranch(addr,(long)rs> 0,predict);
			break;
		default:
			next = addr+1;
			break;
	}
	
	return next;
}
