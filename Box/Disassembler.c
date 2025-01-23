/*
** idtdisasm.c - disassembler/assembler for the R3000/R3010
**
** Copyright Integrated Device Technology, Inc.
** All Rights Reserved
**
** Aug. 16, 1989   D. Cahoon
**
*/

#include "WTVTypes.h"
#include "boxansi.h"
#include "BoxUtils.h"

#define MIPSEB 1

#define u_char	unsigned char
#define u_short unsigned short
#define u_int	unsigned int

#define CONST const


/*
**	idtinst.h - header file for the IDT disassembler/assembler
**
**	Copyright Integrated Device Technology, Inc.
**	All Rights Reserved
**
**	Aug. 16, 1989 - D. Cahoon
*/

/*
**	The following defines are used in the asm_tab structure to
**	specify the type of instruction (format) - (i,j,r) The 'END_TYPE'
**	is used to mark the end of the table. 'F' type is for floating
**	point instructions 
*/

#define J_TYPE		1
#define I_TYPE		2
#define R_TYPE		3
#define F_TYPE		4
#define END_TYPE	0x20

/*
**	The following defines are used in the asm_tab structure to 
**	allow for easy determination of branch and jump instructions.
**	these flags are used by the breakpoint code.
*/
#define JUMP		1	/* this instruction is a jump or branch */
#define	LINK		2	/* this instruction is a jump or br and link */
#define	COND		4	/* this instruction is conditional */
#define LOAD		8	/* This instr is a load from mem */
#define STORE		0x10	/* This instr is a store to mem */
#define RETN		0x20	/* This instr is typ a rtn from subr */

/*
** the following are codes that indicate the type of operand to be
**	printed out for the instruction being decoded. Example:
**	Suppose we were disassembling the instruction - lw  a0,24(sp)
**	The operands for this instruction would be rt, signed immediate,
**	and a base register. So the arguments would be RT_ARG, 
**	SIGNED_IMD_ARG and BASE_ARG. In the asm_tab structure the member
**	'arg_order' contains these codes as 4 bit quantities ordered from
**	right to left. The value in 'arg_order' would be - 0x000085f2.
**	The print routines uses this value by fetching the code in the
**	bottom four bits and printing the operand pointed too. It then 
**	shifts right 4 bits and continues until a zero code is found.
**	This sceme will allow a max of seven operands. The comma is treated
**	like an operand ( hence the 'f' code ).
*/ 
#define RS_ARG		1
#define RT_ARG		2
#define RD_ARG		3
#define SHAMT_ARG	4
#define SIGNED_IMD_ARG	5
#define UNSIGNED_IMD_ARG	6
#define TARGET_ARG	7
#define BASE_ARG	8
#define FT_ARG		9
#define FS_ARG		10
#define FD_ARG		11
#define CODE_ARG	12
#define C0_ARG		14
#define COMMA_ARG	15


/*
**	these flags indicate that additional decoding is required
**	to determine the type of instruction. The pointer to asm_tab
**	in the asm_tab structure points to the next decode table.
*/	
#define NORM_DECODE	0	/* regular ole decode */
#define	SPC_DECODE	1	/* this is in the class of 'special' inst */
#define	BCOND_DECODE	2	/* this is in the class if cond br inst */
#define	COP0_DECODE	3	/* this is a coprocesser 0 instruction */
#define	COP1_DECODE	4	/* this is a coprocessor 1 instr. */

#define N_ENC		0x00
#define SPC_ENC		0x10
#define B_ENC		0x20
#define CP0_ENC		0x30
#define CP1_ENC		0x40
/*
**	masks for op field qualification
*/
#define	OP_MASK		0x3f00
#define CO_OP_MASK	0x2000
#define COP_OP_MASK	0x3e00
#define BOP_OP_MASK	0x3700

/*
** These flags are for those special mnemonics that don't fall out of the
** normal decoding - (ie. addiu  r3,zero,0x1234 is called a li r3,0x1234)
*/
#define MAY_BE_MOVE	0x00010000
#define MAY_BE_NOP	0x00020000
#define MAY_BE_LI	0x00030000
#define MAY_BE_BR	0x00040000

#define DIS_NEXT	0x01000000	/* delay slot in next instruction */

/*
**	masks for getting the various fields out of the opt_decode 
**	member in asm_tab
**	
*/
#define OPT_DEC_MASK	0x0000000f	/* optional decode field */
#define OPT_ENC_MASK	0x000000f0	/* optional enccode field */
#define OPT_OP_MASK	0x0000ff00	/* op field mask */
#define OPT_MN_MASK	0x00ff0000	/* the 'MAY_BE...' field */
#define DELAY_MASK	0xff000000	/* next instr in a delay slot */

#define DONT_SHOW_REGS	0	/* don't display register contents */
#define SHOW_REGS	1	/* flag to disassembler - display registers */
#define DO_DELAY	2	/* disassemble the delay slot inst too */
#define NO_ADDR		4	/* dont print out address */
#define USE_EXCEPT	8	/* Use except saved regs instead of client */

/*
** These defines are used by the breakpoint routines. The values
** for the various types of breakpoint are actually the machine code
** for a break instruction. The type of breakpoint is contained in the
** code field(bits 8-11) of the break instruction. The breakpoint 
** number of the sticky type of breakpoint is in the code field 
** (bits 16-19) also.
*/
#define BP_STICKY	0xa0d		/* created by break command */
#define BP_NON_STICKY	0xb0d		/* temporary to impliment single step */
#define BP_RIPPLE	0xc0d		/* continuing (running) from a sticky */
					/* or non_sticky breakpoint */
#define BP_TRACE	0xd0d		/* trace type breakpoint */
#define COND_STRT_TRC	0x00100000	/* start tracing on bp */
#define COND_STOP_TRC	0x00200000	/* stop tracing on bp */

#define BP_NUM_MASK	0x000f0000
#define BP_CT_MASK	0x00f00000
#define BP_NUM_SHIFT	16
#define BP_TYPE_MASK	0xfff

#define	NUMBRKPT	16
#define EMPTYFLAG	(unsigned*)-1

#define	BRK_ON_BREAK	1 
#define	BRK_ON_STORES	2
#define	BRK_ON_LOADS	4
#define	BRK_ON_FULL	0x8
#define	BRK_ON_INSTR	0x10

#define	TRACE_ALL  	0x100
#define	TRACE_CALLS 	0x200
#define	TRACE_LOADS 	0x400
#define	TRACE_STORES 	0x800
#define	TRACE_INSTR 	0x1000

#define STOP_MASK	0x00ff
#define TRACE_MASK	0xff00

#define TRBUF_SIZE	512

typedef struct {
	unsigned	*addr;	   /* the address executed */
	unsigned	instr;	   /* the instruction  executed */
	u_char		argnum[4]; /* arg. code - either reg num or code */
	unsigned	arg[4];    /* arg contents */
	unsigned	type;	   /* decode type */
	unsigned	argkey;	   /* key to decoding arguments */
	}tr_tab ;


#ifdef MIPSEB
union r3k_inst {
	unsigned asm_code;
	struct {
		unsigned op : 6;
		unsigned target : 26;
	} j_type;
	struct {
		unsigned op : 6;
		unsigned rs : 5;
		unsigned rt : 5;
		signed signed_imd : 16;
	} i_type;
	struct {
		unsigned op : 6;
		unsigned rs : 5;
		unsigned rt : 5;
		unsigned unsigned_imd : 16;
	} u_type;
	struct {
		unsigned op : 6;
		unsigned rs : 5;
		unsigned rt : 5;
		unsigned rd : 5;
		unsigned shamt : 5;
		unsigned funct : 6;
	} r_type;
	struct {
		unsigned op : 6;
		unsigned : 1;
		unsigned fmt : 4;
		unsigned ft : 5;
		unsigned fs : 5;
		unsigned fd : 5;
		unsigned funct : 6;
	} f_type;
};
#endif

#ifdef MIPSEL
union r3k_inst {
	unsigned asm_code;
	struct {
		unsigned target : 26;
		unsigned op : 6;
	} j_type;
	struct {
		signed signed_imd : 16;
		unsigned rt : 5;
		unsigned rs : 5;
		unsigned op : 6;
	} i_type;
	struct {
		unsigned unsigned_imd : 16;
		unsigned rt : 5;
		unsigned rs : 5;
		unsigned op : 6;
	} u_type;
	struct {
		unsigned funct : 6;
		unsigned shamt : 5;
		unsigned rd : 5;
		unsigned rt : 5;
		unsigned rs : 5;
		unsigned op : 6;
	} r_type;
	struct {
		unsigned funct : 6;
		unsigned fd : 5;
		unsigned fs : 5;
		unsigned ft : 5;
		unsigned fmt : 4;
		unsigned : 1;
		unsigned op : 6;
	} f_type;
};
#endif



#define	NREGS		80
unsigned client_regs[NREGS];	
extern unsigned except_regs[NREGS];	

int gBigEndian;

extern int default_reg_set;
extern int default_base;
extern char *atob();

#define	NIL 0


struct asm_tab {
	int	op_val;			/* op-code value */
	CONST struct asm_tab *asm_tab_ptr;/* pointer to next decode table */
	int	opt_decode;		/* optional decode flag or flags */ 
	CONST char	*CONST mnem;			/* assembler mnemonic */
	short	inst_type;		/* type of instruction */
	short	br_jmp_type;		/* if branch - tells what type */
	int	arg_order;		/* arguments and order for this inst */
	  };

CONST struct asm_tab  asm_tab_spec[] = {
	{ 0x0 , NIL, OP_MASK|MAY_BE_NOP|SPC_ENC,"sll", R_TYPE,NIL, 0x4f2f3 },
	{ 0x0 , NIL, OP_MASK|SPC_ENC, "nop", R_TYPE, NIL, 0x0 },
	{ 0x2 , NIL, OP_MASK|SPC_ENC, "srl", R_TYPE, NIL, 0x4f2f3 },
	{ 0x3 , NIL, OP_MASK|SPC_ENC, "sra", R_TYPE, NIL, 0x4f2f3 },
	{ 0x4 , NIL, OP_MASK|SPC_ENC, "sllv", R_TYPE, NIL, 0x1f2f3 },
	{ 0x6 , NIL, OP_MASK|SPC_ENC, "srlv", R_TYPE, NIL, 0x1f2f3 },
	{ 0x7 , NIL, OP_MASK|SPC_ENC, "srav", R_TYPE, NIL, 0x1f2f3 },
	{ 0x8 , NIL, OP_MASK|DIS_NEXT|SPC_ENC, "jr", R_TYPE,JUMP|RETN, 0x1 },
	{ 0x9 , NIL,  OP_MASK|DIS_NEXT|SPC_ENC,"jalr",R_TYPE,JUMP|LINK,0x1f3},
	{ 0xc , NIL, OP_MASK|SPC_ENC, "syscall", R_TYPE, NIL, 0x0 },
	{ 0xd , NIL, OP_MASK|SPC_ENC, "break", R_TYPE, NIL, 0xd },
	{ 0x10 , NIL, OP_MASK|SPC_ENC, "mfhi", R_TYPE, NIL, 0x3 },
	{ 0x11 , NIL, OP_MASK|SPC_ENC, "mthi", R_TYPE, NIL, 0x1 },
	{ 0x12 , NIL, OP_MASK|SPC_ENC, "mflo", R_TYPE, NIL, 0x3 },
	{ 0x13 , NIL, OP_MASK|SPC_ENC, "mtlo", R_TYPE, NIL,0x1 },
	{ 0x18 , NIL, OP_MASK|SPC_ENC, "mult", R_TYPE, NIL, 0x2f1 },
	{ 0x19 , NIL, OP_MASK|SPC_ENC, "multu", R_TYPE, NIL, 0x2f1 },
	{ 0x1a , NIL, OP_MASK|SPC_ENC, "div", R_TYPE, NIL, 0x2f1 },
	{ 0x1b , NIL, OP_MASK|SPC_ENC, "divu", R_TYPE, NIL, 0x2f1 },
	{ 0x20 , NIL, OP_MASK|SPC_ENC, "add", R_TYPE, NIL, 0x2f1f3 },
	{ 0x21,NIL,OP_MASK|MAY_BE_MOVE|SPC_ENC,"addu",R_TYPE,NIL,0x2f1f3 },
	{ 0x21 , NIL, OP_MASK|SPC_ENC, "move", R_TYPE, NIL, 0x1f3 },
	{ 0x22 , NIL, OP_MASK|SPC_ENC, "sub", R_TYPE, NIL, 0x2f1f3 },
	{ 0x23 , NIL, OP_MASK|SPC_ENC, "subu", R_TYPE, NIL, 0x2f1f3 },
	{ 0x24 , NIL, OP_MASK|SPC_ENC, "and", R_TYPE, NIL, 0x2f1f3 },
	{ 0x25 , NIL, OP_MASK|SPC_ENC, "or", R_TYPE, NIL, 0x2f1f3 },
	{ 0x26 , NIL, OP_MASK|SPC_ENC, "xor", R_TYPE, NIL, 0x2f1f3 },
	{ 0x27 , NIL, OP_MASK|SPC_ENC, "nor", R_TYPE, NIL, 0x2f1f3 },
	{ 0x2a , NIL, OP_MASK|SPC_ENC, "slt", R_TYPE, NIL, 0x2f1f3 },
	{ 0x2b , NIL, OP_MASK|SPC_ENC, "sltu", R_TYPE, NIL, 0x2f1f3 },	
	{ 0x2c , NIL, OP_MASK|SPC_ENC, "dad", R_TYPE, NIL, 0x2f1f3 },			
	{ 0x2d , NIL, OP_MASK|SPC_ENC, "dadu", R_TYPE, NIL, 0x2f1f3 },
	{ NIL, NIL, NIL, NIL, END_TYPE, NIL, NIL }
	   };
	   
CONST struct asm_tab  asm_tab_spec2[] = {
	{ 0x0 , NIL, OP_MASK|SPC_ENC, "mad", R_TYPE, NIL, 0x2f1 },
	{ 0x1 , NIL, OP_MASK|SPC_ENC, "madu", R_TYPE, NIL, 0x2f1 },
	{ 0x2 , NIL, OP_MASK|SPC_ENC, "mul", F_TYPE,NIL, 0x2f1f3 },
	{ NIL, NIL, NIL, NIL, END_TYPE, NIL, NIL }
	   };
	   
	   
CONST struct asm_tab asm_tab_bcond[] = {
	{ 0x0,NIL,OP_MASK|DIS_NEXT|B_ENC,"bltz",I_TYPE,JUMP|COND, 0x7f1 },
	{ 0x1,NIL,OP_MASK|DIS_NEXT|B_ENC,"bgez",I_TYPE,JUMP|COND, 0x7f1},
	{0x10,NIL,OP_MASK|DIS_NEXT|B_ENC,"bltzal",I_TYPE,JUMP|LINK|COND,0x7f1},
	{0x11,NIL,OP_MASK|DIS_NEXT|B_ENC,"bgezal",I_TYPE,JUMP|LINK|COND,0x7f1},
	{ NIL, NIL, NIL, NIL, END_TYPE, NIL, NIL }
	    };


CONST struct asm_tab asm_tab_cp0[] = {
	{ 0x1 , NIL, OP_MASK|SPC_ENC, "tlbr", R_TYPE, NIL, 0x0 },
	{ 0x2 , NIL, OP_MASK|SPC_ENC, "tlbwi", R_TYPE, NIL, 0x0 },
	{ 0x6 , NIL, OP_MASK|SPC_ENC, "tlbwr", R_TYPE, NIL, 0x0 },
	{ 0x8 , NIL, OP_MASK|SPC_ENC, "tlbp", R_TYPE, NIL, 0x0 },
	{ 0x10,NIL,OP_MASK|DIS_NEXT| NIL|SPC_ENC,"rfe",R_TYPE,NIL,0x0 },
	{ NIL, NIL, NIL, NIL, END_TYPE, NIL, NIL }
	    };

CONST struct asm_tab asm_tab_cp1[] = {
	{ 0x0 , NIL, OP_MASK|SPC_ENC, "add.", F_TYPE, NIL,0x9fafb },
	{ 0x1 , NIL, OP_MASK|SPC_ENC, "sub.", F_TYPE, NIL,0x9fafb },
	{ 0x2 , NIL, OP_MASK|SPC_ENC, "mul.", F_TYPE, NIL,0x9fafb },
	{ 0x3 , NIL, OP_MASK|SPC_ENC, "div.", F_TYPE, NIL,0x9fafb },
	{ 0x5 , NIL, OP_MASK|SPC_ENC, "abs.", F_TYPE, NIL,0xafb },
	{ 0x6 , NIL, OP_MASK|SPC_ENC, "mov.", F_TYPE, NIL,0xafb },
	{ 0x7 , NIL, OP_MASK|SPC_ENC, "neg.", F_TYPE, NIL,0xafb },
	{ 0x20 , NIL, OP_MASK|SPC_ENC, "cvt.s.", F_TYPE, NIL,0xafb },
	{ 0x21 , NIL, OP_MASK|SPC_ENC, "cvt.d.", F_TYPE, NIL,0xafb },
	{ 0x24 , NIL, OP_MASK|SPC_ENC, "cvt.w.", F_TYPE, NIL,0xafb },
	{ 0x30 , NIL, OP_MASK|SPC_ENC, "c.f.", F_TYPE, NIL,0x9fa },
	{ 0x31 , NIL, OP_MASK|SPC_ENC, "c.un.", F_TYPE, NIL,0x9fa },
	{ 0x32 , NIL, OP_MASK|SPC_ENC, "c.eq.", F_TYPE, NIL,0x9fa },
	{ 0x33 , NIL, OP_MASK|SPC_ENC, "c.ueq.", F_TYPE, NIL,0x9fa },
	{ 0x34 , NIL, OP_MASK|SPC_ENC, "c.olt.", F_TYPE, NIL,0x9fa },
	{ 0x35 , NIL, OP_MASK|SPC_ENC, "c.ult.", F_TYPE, NIL,0x9fa },
	{ 0x36 , NIL, OP_MASK|SPC_ENC, "c.ole.", F_TYPE, NIL,0x9fa },
	{ 0x37 , NIL, OP_MASK|SPC_ENC, "c.ule.", F_TYPE, NIL,0x9fa },
	{ 0x38 , NIL,OP_MASK| NIL|SPC_ENC, "c.sf.", F_TYPE, NIL,0x9fa },
	{ 0x39 , NIL, OP_MASK|SPC_ENC, "c.ngle.", F_TYPE, NIL,0x9fa },
	{ 0x3a , NIL, OP_MASK|SPC_ENC, "c.seq.", F_TYPE, NIL,0x9fa },
	{ 0x3b , NIL, OP_MASK|SPC_ENC, "c.ngl.", F_TYPE, NIL,0x9fa },
	{ 0x3c , NIL, OP_MASK|SPC_ENC, "c.lt.", F_TYPE, NIL,0x9fa },
	{ 0x3d , NIL, OP_MASK|SPC_ENC, "c.nge.", F_TYPE, NIL,0x9fa },
	{ 0x3e , NIL, OP_MASK|SPC_ENC, "c.le.", F_TYPE, NIL,0x9fa },
	{ 0x3f , NIL, OP_MASK|SPC_ENC, "c.ngt.", F_TYPE, NIL,0x9fa },
	{ NIL, NIL, NIL, NIL, END_TYPE, NIL, NIL }
	    };

CONST struct asm_tab asm_tab_cop0[] = {
	{ 0x0 , NIL, COP_OP_MASK|CP0_ENC, "mfc0", R_TYPE, NIL, 0xef2 },
	{ 0x8 , NIL, COP_OP_MASK|CP0_ENC, "mtc0", R_TYPE, NIL, 0xef2 },
	{ 0x10,NIL,BOP_OP_MASK|DIS_NEXT|CP0_ENC,"bc0f",I_TYPE,JUMP|COND,0x7},
	{ 0x11,NIL,BOP_OP_MASK|DIS_NEXT|CP0_ENC,"bc0t",I_TYPE,JUMP|COND,0x7},
	{ 0x4 , NIL, COP_OP_MASK|CP0_ENC, "cfc0", R_TYPE, NIL, 0xef2 },
	{ 0xc , NIL, COP_OP_MASK|CP0_ENC, "ctc0", R_TYPE, NIL, 0xef2 },
	{ 0x20,asm_tab_cp0, CO_OP_MASK|SPC_DECODE|CP0_ENC,NIL,NIL,NIL,NIL},
	{ NIL, NIL, NIL, NIL, END_TYPE, NIL, NIL }
	    };

CONST struct asm_tab asm_tab_cop1[] = {
	{ 0x0 , NIL, COP_OP_MASK|CP1_ENC, "mfc1", R_TYPE, NIL, 0xaf2 },
	{ 0x8 , NIL, COP_OP_MASK|CP1_ENC, "mtc1", R_TYPE, NIL, 0xaf2 },
	{ 0x10,NIL,BOP_OP_MASK|DIS_NEXT|CP1_ENC,"bc1f",I_TYPE,JUMP|COND,0x7},
	{ 0x11,NIL,BOP_OP_MASK|DIS_NEXT|CP1_ENC,"bc1t",I_TYPE,JUMP|COND,0x7},
	{ 0x4 , NIL, COP_OP_MASK|CP1_ENC, "cfc1", R_TYPE, NIL, 0xef2 },
	{ 0xc , NIL, COP_OP_MASK|CP1_ENC, "ctc1", R_TYPE, NIL, 0xef2 },
	{ 0x20,asm_tab_cp1, CO_OP_MASK|SPC_DECODE|CP1_ENC,NIL,NIL,NIL,NIL},
	{ NIL, NIL, NIL, NIL, END_TYPE, NIL, NIL }
	    };

CONST struct asm_tab asm_tab_cop2[] = {
	{ 0x10,NIL,BOP_OP_MASK|DIS_NEXT|CP1_ENC,"bc2f",I_TYPE,JUMP|COND,0x7},
	{ 0x11,NIL,BOP_OP_MASK|DIS_NEXT|CP1_ENC,"bc2t",I_TYPE,JUMP|COND,0x7},
	{ NIL, NIL, NIL, NIL, END_TYPE, NIL, NIL }
	    };

CONST struct asm_tab asm_tab_cop3[] = {
	{ 0x10,NIL,BOP_OP_MASK|DIS_NEXT|CP1_ENC,"bc3f",I_TYPE,JUMP|COND,0x7},
	{ 0x11,NIL,BOP_OP_MASK|DIS_NEXT|CP1_ENC,"bc3t",I_TYPE,JUMP|COND,0x7},
	{ NIL, NIL, NIL, NIL, END_TYPE, NIL, NIL }
	    };

CONST struct asm_tab  asm_tab_op[] = {
	{ 0,asm_tab_spec,OP_MASK|SPC_DECODE|N_ENC, NIL, NIL,NIL,NIL },
	{ 1,asm_tab_bcond,OP_MASK|BCOND_DECODE|N_ENC,NIL,NIL,NIL,NIL},
	{ 2,NIL,OP_MASK|DIS_NEXT| NIL|N_ENC, "j", J_TYPE,JUMP, 0x7 },
	{ 3,NIL,OP_MASK|DIS_NEXT| NIL|N_ENC,"jal",J_TYPE,JUMP|LINK,0x7},
	{ 4,NIL,OP_MASK|DIS_NEXT|MAY_BE_BR|N_ENC,"beq",I_TYPE,JUMP|COND,0x7f2f1},
	{ 4,NIL,OP_MASK|DIS_NEXT| NIL|N_ENC, "b", I_TYPE,JUMP, 0x7},
	{ 5,NIL,OP_MASK|DIS_NEXT|N_ENC,"bne",I_TYPE,JUMP|COND,0x7f2f1},
	{ 6,NIL,OP_MASK|DIS_NEXT|N_ENC,"blez", I_TYPE,JUMP|COND,0x7f1},
	{ 7,NIL,OP_MASK|DIS_NEXT|N_ENC,"bgtz",I_TYPE,JUMP|COND,0x7f1},
	{ 8,NIL,OP_MASK|N_ENC,"addi",I_TYPE, NIL, 0x5f1f2},
	{ 9,NIL,OP_MASK|MAY_BE_LI|N_ENC,"addiu",I_TYPE,NIL,0x6f1f2},
	{ 9,NIL,OP_MASK|N_ENC, "li", I_TYPE, NIL, 0x5f2},
	{ 0xa,NIL, OP_MASK|N_ENC, "slti", I_TYPE, NIL, 0x5f1f2},
	{ 0xb,NIL, OP_MASK|N_ENC, "sltiu", I_TYPE, NIL, 0x6f1f2},
	{ 0xc,NIL, OP_MASK|N_ENC, "andi", I_TYPE, NIL, 0x6f1f2},
	{ 0xd,NIL, OP_MASK|N_ENC, "ori", I_TYPE, NIL, 0x6f1f2},
	{ 0xe,NIL, OP_MASK|N_ENC, "xori", I_TYPE, NIL, 0x6f1f2},
	{ 0xf,NIL, OP_MASK|N_ENC, "lui", I_TYPE, NIL, 0x6f2},
	{ 0x10,asm_tab_cop0,OP_MASK|COP0_DECODE|N_ENC,NIL,NIL,NIL,NIL },
	{ 0x11,asm_tab_cop1,OP_MASK|COP1_DECODE|N_ENC,NIL,NIL,NIL,NIL },
	{ 0x12,asm_tab_cop2,OP_MASK|COP0_DECODE|N_ENC,NIL,NIL,NIL,NIL },
	{ 0x13,asm_tab_cop3,OP_MASK|COP0_DECODE|N_ENC,NIL,NIL,NIL,NIL },
	{ 0x14,NIL,OP_MASK|DIS_NEXT|MAY_BE_BR|N_ENC,"beql",I_TYPE,JUMP|COND,0x7f2f1},
	{ 0x15,NIL,OP_MASK|DIS_NEXT|N_ENC,"bnel",I_TYPE,JUMP|COND,0x7f2f1},
	{ 0x16,NIL,OP_MASK|DIS_NEXT|N_ENC,"blezl", I_TYPE,JUMP|COND,0x7f1},
	{ 0x17,NIL,OP_MASK|DIS_NEXT|N_ENC,"bgtzl",I_TYPE,JUMP|COND,0x7f1},
	{ 0x1c,asm_tab_spec2,OP_MASK|SPC_DECODE|N_ENC, NIL, NIL,NIL,NIL },
	{ 0x20,NIL, OP_MASK|N_ENC, "lb", I_TYPE, LOAD, 0x85f2 },
	{ 0x21,NIL, OP_MASK|N_ENC, "lh", I_TYPE, LOAD, 0x85f2 },
	{ 0x22,NIL, OP_MASK|N_ENC, "lwl", I_TYPE, LOAD, 0x85f2 },
	{ 0x23,NIL, OP_MASK|N_ENC, "lw", I_TYPE, LOAD, 0x85f2 },
	{ 0x24,NIL, OP_MASK|N_ENC, "lbu", I_TYPE, LOAD, 0x85f2 },
	{ 0x25,NIL, OP_MASK|N_ENC, "lhu", I_TYPE, LOAD, 0x85f2 },
	{ 0x26,NIL, OP_MASK|N_ENC, "lwr", I_TYPE, LOAD, 0x85f2 },
	{ 0x28,NIL, OP_MASK|N_ENC, "sb", I_TYPE, STORE, 0x85f2 },
	{ 0x29,NIL, OP_MASK|N_ENC, "sh", I_TYPE, STORE, 0x85f2 },
	{ 0x2a,NIL, OP_MASK|N_ENC, "swl", I_TYPE, STORE, 0x85f2 },
	{ 0x2b,NIL, OP_MASK|N_ENC, "sw", I_TYPE, STORE, 0x85f2 },
	{ 0x2e,NIL, OP_MASK|N_ENC, "swr", I_TYPE, STORE, 0x85f2 },
	{ 0x30,NIL, OP_MASK|N_ENC, "lwc0", I_TYPE, LOAD, 0x85fe },
	{ 0x31,NIL, OP_MASK|N_ENC, "lwc1", I_TYPE, LOAD, 0x85f9 },
	{ 0x32,NIL, OP_MASK|N_ENC, "lwc2", I_TYPE, LOAD, 0x85f2 },
	{ 0x33,NIL, OP_MASK|N_ENC, "lwc3", I_TYPE, LOAD, 0x85f2 },
	{ 0x38,NIL, OP_MASK|N_ENC, "swc0", I_TYPE, STORE, 0x85fe },
	{ 0x39,NIL,OP_MASK| NIL|N_ENC, "swc1", I_TYPE, STORE, 0x85f9 },
	{ 0x3a,NIL, OP_MASK|N_ENC, "swc2", I_TYPE, STORE, 0x85f2 },
	{ 0x3b,NIL, OP_MASK|N_ENC, "swc3", I_TYPE, STORE, 0x85f2 },
	{ NIL,NIL, NIL, NIL, END_TYPE, NIL, NIL }
	   };

static	const char	*CONST rregs[2][32] = {
	{
	 "zero","at","v0","v1","a0","a1","a2","a3",
	 "t0","t1","t2","t3","t4","t5","t6","t7",
	 "s0","s1","s2","s3","s4","s5","s6","s7",
	 "t8","t9","k0","k1","gp","sp","fp","ra"
	},
	{
	 "r0","r1","r2","r3","r4","r5","r6","r7" ,
	 "r8","r9","r10","r11","r12","r13","r14","r15" ,
	 "r16","r17","r18","r19","r20","r21","r22","r23" ,
	 "r24","r25","r26","r27","r28","r29","r30","r31" 
	}
		};
		
		
/*
** the array below cheats a little and includes the implementation
** and control and status register for cp1 - (the floating point
** coprocessor) - this is possible because the regsters in cp0 and 
** cp1 don't overlap.
*/
static	const char	*CONST c0regs[32] = {
	"index","random","tlblo","3","context","5","6","7",
	"badvaddr","9","tlbhi","11","sr","cr","epc","15",
	"16","17","18","19","20","21","22","23",
	"24","25","26","27","28","29","feir","fcsr"
	};

static	const char	*CONST fregs[32] = {
	 "f0","f1","f2","f3","f4","f5","f6","f7" ,
	 "f8","f9","f10","f11","f12","f13","f14","f15" ,
	 "f16","f17","f18","f19","f20","f21","f22","f23" ,
	 "f24","f25","f26","f27","f28","f29","f30","f31" 
		};
/*
**	static definitions used by the idt disassembler 
*/
static	int	op;		/* instruction opcode */
static	int	rs;		/* rs register number */
static	int	rt;		/* rt register number */
static	int	rd;		/* rd register number */
static	int	shamt;		/* shift amount */
static	int	funct;		/* function code for special */
static	short	signed_imd;	/* signed immediate value */
static	unsigned unsigned_imd;	/* unsigned immediate value */
static	int	target;		/* target address of br and jump */
static	int	format;		/* floating point format */
static	int	base;		/* the index reg in loads and stores */
static	int	ft;		/* floating pt ft reg # */
static	int	fs;		/* fs reg # */
static	int	fd;		/* fd reg # */

static	int	delay_slot;	/* a flag to signal next instr in delay slot */
static	int	reg_set;	/* what set of regs to use for display */
static	int	der_regs[4];	/* contains list of regs used in this inst */
static	int	is_fp_reg[4];	/* if regs were floating point */
static	int	num_regs;	/* the num. of regs in the arrays above */


/*
** do_decode() - extracts the operands for the particular instruction
**		 type.
**	entry:
**		int type - the type of instruction
**		struct r3k_inst inst - the instruction to decode
**		unsigned *addr - the address of the instr. ( used
**				 for calc. offsets and targets )
**
**	return - none
*/
static void do_decode(int type,union r3k_inst inst,unsigned long *addr)
{
	switch ( type )
	  {
	   case J_TYPE:
		target = ((int)addr&0xf0000000) | inst.j_type.target << 2;
		break;
	   case R_TYPE:
		rs = inst.r_type.rs;
		rt = inst.r_type.rt;
		rd = inst.r_type.rd;
		fs = inst.f_type.fs;
		shamt = inst.r_type.shamt;
		funct = inst.r_type.funct;
		break;
	   case I_TYPE:
		base = inst.i_type.rs;
		rs = base;
		rt = inst.i_type.rt;
		ft = inst.f_type.ft;
		rd = rt;
		signed_imd = inst.i_type.signed_imd;
		target = (int)addr + (signed_imd<<2) + 4;
		unsigned_imd = inst.u_type.unsigned_imd;
		break;
	   case F_TYPE:
		format = inst.f_type.fmt;
		ft = inst.f_type.ft;
		fs = inst.f_type.fs;
		fd = inst.f_type.fd;
		rt = inst.r_type.rt;
		funct = inst.f_type.funct;
		break;
	  }
}


/*
**	do_opt_decode() - extract the opcode from the instruction
**
**	entry:
**		code - type of decode to do :
**			SPC_DECODE - extract op for special class
**			BCOND_DECODE - BCOND type of instructions
**			COP0_DECODE - Coprocessor 0 instr.
**			COP1_DECODE - Floating point instr.
**
*/
static void do_opt_decode(int code,union r3k_inst inst)
{
	switch ( code )
	  {
	   case SPC_DECODE:
		op = inst.r_type.funct;
		break;
	   case BCOND_DECODE:
		op = inst.r_type.rt;
		break;
	   case COP0_DECODE:
		op = (inst.r_type.rs<<1) + (inst.r_type.rt & 0x1);
		break;
	   case COP1_DECODE:
		op = (inst.r_type.rs<<1) + (inst.r_type.rt & 0x1);
	   }
}


/*
** print_mnem() - prints out the mnemonic
**
**	entry:
**		struct asm_tab - pointer to an asmtab array of structures
**		int index into the array. 
**
**	return - returns the updated index into the asm_tab
**
**	this routine may recurse if there is a possible alternate
**	mnemonic for this instruction (i.e addu/move). depending
**	on the table, recursion could take place multiple times.
*/
static int print_mnem(CONST struct asm_tab *asmtab,int i)
{
	int omn_code;
	const char	*ch;
	omn_code = asmtab[i].opt_decode&OPT_MN_MASK;
	switch (omn_code)
	   {
	    case MAY_BE_MOVE:
		if ( rt == 0 )
		 { i = print_mnem(asmtab,++i );
		   return(i); }
		break;
	    case MAY_BE_NOP:
		if ( (rt == 0) && ( rs == 0) && ( rd == 0) && (shamt == 0) )
		 { i = print_mnem(asmtab,++i );
		   return(i); }
		break;
	    case MAY_BE_BR:
		if ( (rt == 0) && ( rs == 0) )
		 { i = print_mnem(asmtab,++i );
		   return(i); }
		break;
	    case MAY_BE_LI:
		if ( rs == 0 )
		 { i = print_mnem(asmtab,++i );
		   return(i); }
	   }
	printf("%s",asmtab[i].mnem);
	ch = asmtab[i].mnem;
	while (*ch++ != NIL )
		;
	ch -= 2;
	if ( *ch == '.') 
	   { if ( format == 0 )
		printf("s");
	     else
		printf("d");
	   }
	printf("\t");
	return(i);
}	


#if	0
/*
** get_arg(code,reg) - returns argument value based on code
*/
static int get_arg(int code,int *reg)
{
	switch (code)
	   {
	    case RS_ARG:
		*reg = rs;
		return(client_regs[rs]);
		break;
	    case RT_ARG:
		*reg = rt;
		return(client_regs[rt]);
		break;
	    case RD_ARG:
		*reg = rd;
		return(client_regs[rd]);
		break;
	    case SHAMT_ARG:
		*reg = 0xf4;
		return(shamt);
		break;
	    case SIGNED_IMD_ARG:
		*reg = 0xf1;
		return(signed_imd);
		break;
	    case UNSIGNED_IMD_ARG:
		*reg = 0xf2;
		return(unsigned_imd);
		break;
	    case TARGET_ARG:
		*reg = 0xf0;
		return(target);
		break;
	    case BASE_ARG:
		*reg = base;
		return(client_regs[base]);
		break;
	    case FT_ARG:
		*reg = ft + 32;
		return(client_regs[ft+32]);
		break;
	    case FS_ARG:
		*reg = fs + 32;
		return(client_regs[fs+32]);
		break;
	    case FD_ARG:
		*reg = fd + 32;
		return(client_regs[fd+32]);
		break;
	    case CODE_ARG:
		*reg = 0xf5;
		return( (rs<<10)+(rt<<5)+rd );
		break;
	    case C0_ARG:
		*reg = 0xf6;
		return(0);
		break;
	   }
	return(0);
}
#endif

/*
** print_arg() - prints the arguments to the instruction
**	entry:
**		code - argument code
*/
static void print_arg(int code)
{
	switch (code)
	   {
	    case RS_ARG:
		printf("%s", rregs[reg_set][rs] );
		der_regs[num_regs++] = rs;
		break;
	    case RT_ARG:
		printf("%s", rregs[reg_set][rt] );
		der_regs[num_regs++] = rt;
		break;
	    case RD_ARG:
		printf("%s", rregs[reg_set][rd] );
		der_regs[num_regs++] = rd;
		break;
	    case SHAMT_ARG:
		printf("#%d",shamt);
		break;
	    case SIGNED_IMD_ARG:
		printf("%#x",signed_imd);
		break;
	    case UNSIGNED_IMD_ARG:
		printf("%#x",unsigned_imd);
		break;
	    case TARGET_ARG:
		printf("%#x",target);
		if(gSymbols)
		 {
		 	Symbol *s;
			s = FindSymbol((unsigned long)target);
			printf(" <%s+%lx>",s->symbol,(unsigned long)target - s->address);
		 }
		break;
	    case BASE_ARG:
		printf("(%s)",rregs[reg_set][base]);
		der_regs[num_regs++] = base;
		if(gSymbols && reg_set == 0 && base == 28)
		 {
		 	Symbol *s;
			ulong  addr;
			addr = except_regs[27] + signed_imd;
			s = FindSymbol((unsigned long)addr);
			printf(" <%s @ %lx>",s->symbol,addr);
		 }
		break;
	    case FT_ARG:
		printf("%s",fregs[ft]);
		der_regs[num_regs] = ft;
		is_fp_reg[num_regs++] = ft+32;
		break;
	    case FS_ARG:
		printf("%s",fregs[fs]);
		der_regs[num_regs] = fs;
		is_fp_reg[num_regs++] = fs+32;
		break;
	    case FD_ARG:
		printf("%s",fregs[fd]);
		der_regs[num_regs] = fd;
		is_fp_reg[num_regs++] = fd+32;
		break;
	    case CODE_ARG:
		printf("%#x",((rs<<10)+(rt<<5)+rd) );
		break;
	    case C0_ARG:
		printf("%s",c0regs[rd]);
		break;
	    case COMMA_ARG:
		printf(",");
		break;
	   }
}

/*
** find_op(asmtab,inst) - searches the asm_tabs for opcode and
**			follows the path until the instruction
**			is completely disassembled
**
**	entry:
**		struct asm_tab *asmtab   pointer to the asm_tab
**		union r3k_inst inst      instruction to disassemble
**
*/
static void find_op(CONST struct asm_tab *asmtab,union r3k_inst inst,int mode,unsigned long *addr)
{
	int	i,argkey,tmp_op;
	for ( i = 0 ; asmtab[i].inst_type != END_TYPE; i++ )
	 { tmp_op = op & ((asmtab[i].opt_decode&OPT_OP_MASK)>>8);
	   if (tmp_op == asmtab[i].op_val )
	     { if ( (asmtab[i].opt_decode&OPT_DEC_MASK) != 0 )
		  { do_opt_decode((asmtab[i].opt_decode&OPT_DEC_MASK),inst);
		    find_op(asmtab[i].asm_tab_ptr,inst,mode,addr);
		    return; }
	       else
		  { do_decode(asmtab[i].inst_type,inst,addr);
		    i = print_mnem(asmtab,i);
		    argkey = asmtab[i].arg_order;
		    while ( (argkey & 0xf) != 0 )
		        {print_arg(argkey&0xf);
		         argkey = argkey >> 4; }
		    break;
		   }
	     }
	  }
	if((asmtab[i].opt_decode&DELAY_MASK) == DIS_NEXT )
	   delay_slot = 1;
	else
	   delay_slot = 0;
	if ( (mode & SHOW_REGS) && (num_regs > 0) )
	  {
	   for ( i = 0; i < num_regs; i++ ) 
		{ if ( i == 0 )
			printf("\t<");
		  else
			printf(", ");
		  if(mode & USE_EXCEPT)
		    { if( is_fp_reg[i] == 0 )
			printf("%s=%x",rregs[reg_set][der_regs[i]],
					except_regs[der_regs[i]]);
		      else
			printf("%s=%x",fregs[der_regs[i]],
					except_regs[is_fp_reg[i]]);
		    }	
		  else	
		    { if( is_fp_reg[i] == 0 )
			printf("%s=%x",rregs[reg_set][der_regs[i]],
					client_regs[der_regs[i]]);
		      else
			printf("%s=%x",fregs[der_regs[i]],
					client_regs[is_fp_reg[i]]);
		    }	
		}
	    printf(">");
	   }
	if( asmtab[i].inst_type == END_TYPE )
	  if(mode & SHOW_REGS)
		printf("%08x\t",inst.asm_code);
}


/*
** disasm(addr,mode,reg_set) - disassemble the instruction at addr
**	entry:
**		unsigned addr - address of instruction to be disassembled
**		int mode - flag to tell if the registers can be shown
**			   also a bit 'DO_DELAY' controls if branch delay
**			   slots are treated autominously
**		int reg_set - 0 = use compiler notation for reg display
**			      1 = use raw hardware notation
**
**	return - returns the number of bytes disassembled. This is
**		 normally 4 for one instruction. If the delay slot
**		 was disassembled also, the number will be 8.
**		 retuns 0 if error.
**
**	this routine may recurse if the instruction in the delay
**	slot needs to be disassembled. Branches and jumps and
**	their delay slots are treated autominously(controlled by bit 1 in
**	mode flag - needs to be a one(1) if autominous).
*/

int disasm(unsigned long *addr,unsigned long *mem_addr,int mode,int regset);
int disasm(unsigned long *addr,unsigned long *mem_addr,int mode,int regset)
{
	union r3k_inst inst;
	int i,num_bytes;
	Symbol *s;
	
	reg_set = regset;
	num_regs = 0;
	for(i=0 ; i<4; i++)
	   is_fp_reg[i] = 0;
	num_bytes = 4;
	if( !(mode & NO_ADDR) )
	{
		 if(gSymbols)
		 {
		 	unsigned long delta;
			
			s = FindSymbol((unsigned long)addr);
			delta = (unsigned long)addr - s->address;
			if(delta == 0)
				printf("%s\n  ",s->symbol);
			printf("   +%04lx ",delta);
		 }
	     printf(" %08lx  ",(ulong)addr);
	}
	 
	if((int)addr&3 != 0)
	{ 
	   	printf("(disasm); Unaligned Address Specified\n");
	     	return(0); 
	}
	
	inst.asm_code = *addr;
	
	op = inst.r_type.op;
	if( !(mode & SHOW_REGS) )
	    printf("%08x\t",inst.asm_code);
	find_op( asm_tab_op,inst,mode,addr );
	
	if((delay_slot == 1) && (mode & DO_DELAY) )
	{
		printf("\n  ");
	   	disasm(addr+1,mem_addr+1,mode,regset);
	   	num_bytes += 4; 
	}
	
	return(num_bytes);
}


/*
main(int argc, char **argv)
{
FILE *fp;
unsigned long *data;
unsigned long *copy;
unsigned int x,cnt;
unsigned long *target_addr;

	gBigEndian = 1;

 	copy = data = calloc( 0x80000, 1 );
	
	if (data)
	{
 		fp = fopen( "FIRST.BIN", "rb" );
 		fread( data, 1, 0x80000, fp );
 		fclose( fp );

		target_addr = (unsigned long *)0x80030000;
		copy += (0x00018000/4);
		
		for(x=0;x!=(0x68000/4);x++)
		{
			cnt = disasm(target_addr,copy,0,0);
			printf("\n");
			cnt >>= 2;
			target_addr += cnt;
			copy += cnt;
		}
		
		free(data);
	}
	else
		printf("%s: can't alloc buffer!\n",argv[0]);
		
}

*/
