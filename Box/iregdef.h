/*
**	iregdef.h - IDT R3000 register structure header file
**
**	Copyright 1989 Integrated Device Technology, Inc
**	All Rights Reserved
**
*/
#ifndef __IREGDEF_H__
#define __IREGDEF_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HARDWARE

/*
**	ireg_desc - describes the fields within a register
*/
#ifdef CLANGUAGE
struct ireg_desc {
	char 	*ptr_field_name;	/* field name   */
	short	num_digits;		/* number ofdigits to display */
	short	num_spaces;		/* number of spaces to follow */
	int	fld_mask;		/* mask to extract value of field */
	int	fld_shift;		/* shift amount to position field */
	char *CONST *ptr_enum_list;	/* ptr to an enumeration list */
	  };
/*
** reg_name - structure that gives the reg. name, alt. reg name
**		the reg index for fetching the value, the number
**		of spaces req. so a tabular display will align
**		a pointer to a structure defining the fields if
**		required and a flag for the output type.
*/
struct reg_name {
	char	*register_name;
	char	*alt_reg_name;
	short	reg_index;
	short	space_pad;
	CONST struct ireg_desc *ptr_reg_desc_flds;
	int	format_type;
	  };
#endif /* CLANGUAGE */
/*
** register names 
*/
#define r0	$0
#define r1	$1
#define r2	$2
#define r3	$3
#define r4	$4
#define r5	$5
#define r6	$6
#define r7	$7
#define r8	$8
#define r9	$9
#define r10	$10
#define r11	$11
#define r12	$12
#define r13	$13

#define r14	$14
#define r15	$15
#define r16	$16
#define r17	$17
#define r18	$18
#define r19	$19
#define r20	$20
#define r21	$21
#define r22	$22
#define r23	$23
#define r24	$24
#define r25	$25
#define r26	$26
#define r27	$27
#define r28	$28
#define r29	$29
#define r30	$30
#define r31	$31

#define fp0	$f0
#define fp1	$f1
#define fp2	$f2
#define fp3	$f3
#define fp4	$f4
#define fp5	$f5
#define fp6	$f6
#define fp7	$f7
#define fp8	$f8
#define fp9	$f9
#define fp10	$f10
#define fp11	$f11
#define fp12	$f12
#define fp13	$f13
#define fp14	$f14
#define fp15	$f15
#define fp16	$f16
#define fp17	$f17
#define fp18	$f18
#define fp19	$f19
#define fp20	$f20
#define fp21	$f21
#define fp22	$f22
#define fp23	$f23
#define fp24	$f24
#define fp25	$f25
#define fp26	$f26
#define fp27	$f27
#define fp28	$f28
#define fp29	$f29
#define fp30	$f30
#define fp31	$f31

#define fcr0	$0
#define fcr30	$30
#define fcr31	$31

#define zero	$0	/* wired zero */
#define AT	$at	/* assembler temp */
#define v0	$2	/* return value */
#define v1	$3
#define a0	$4	/* argument registers a0-a3 */
#define a1	$5
#define a2	$6
#define a3	$7
#define t0	$8	/* caller saved  t0-t9 */
#define t1	$9
#define t2	$10
#define t3	$11
#define t4	$12
#define t5	$13
#define t6	$14
#define t7	$15
#define s0	$16	/* callee saved s0-s8 */
#define s1	$17
#define s2	$18
#define s3	$19
#define s4	$20
#define s5	$21
#define s6	$22
#define s7	$23
#define t8	$24
#define t9	$25
#define k0	$26	/* kernel usage */
#define k1	$27	/* kernel usage */
#define gp	$28	/* sdata pointer */
#define sp	$29	/* stack pointer */
#define s8	$30	/* yet another saved reg for the callee */
#define fp	$30	/* frame pointer - this is being phased out by MIPS */
#define ra	$31	/* return address */


/*
** relative position of registers in save reg area
*/

#ifdef CPU_R4640

#define	R_R1		0
#define	R_R2		1
#define	R_R3		2
#define	R_R4		3
#define	R_R5		4
#define	R_R6		5
#define	R_R7		6
#define	R_R8		7
#define	R_R9		8
#define	R_R10		9
#define	R_R11		10
#define	R_R12		11
#define	R_R13		12
#define	R_R14		13
#define	R_R15		14
#define	R_R16		15
#define	R_R17		16
#define	R_R18		17
#define	R_R19		18
#define	R_R20		19
#define	R_R21		20
#define	R_R22		21
#define	R_R23		22
#define	R_R24		23
#define	R_R25		24
#define	R_R26		25
#define	R_R27		26
#define	R_R28		27
#define	R_R29		28
#define	R_R30		29
#define	R_R31		30

#define	R_IBASE		31
#define	R_IBOUND	32
#define	R_DBASE		33
#define	R_DBOUND	34
#define	R_BADVADDR	35
#define	R_COUNT		36
#define	R_COMPARE	37
#define	R_SR		38
#define	R_CAUSE		39
#define	R_EPC		40
#define	R_PRID		41
#define	R_CONFIG	42
#define	R_CALG		43
#define	R_IWATCH	44
#define	R_DWATCH	45
#define	R_ECC		46
#define	R_CACHEERR	47
#define	R_TAGLO		48
#define	R_ERROREPC	49

#define	NREGS		50

#else /* CPU_R4640 */

#define	R_R0		0
#define	R_R1		1
#define	R_R2		2
#define	R_R3		3
#define	R_R4		4
#define	R_R5		5
#define	R_R6		6
#define	R_R7		7
#define	R_R8		8
#define	R_R9		9
#define	R_R10		10
#define	R_R11		11
#define	R_R12		12
#define	R_R13		13
#define	R_R14		14
#define	R_R15		15
#define	R_R16		16
#define	R_R17		17
#define	R_R18		18
#define	R_R19		19
#define	R_R20		20
#define	R_R21		21
#define	R_R22		22
#define	R_R23		23
#define	R_R24		24
#define	R_R25		25
#define	R_R26		26
#define	R_R27		27
#define	R_R28		28
#define	R_R29		29
#define	R_R30		30
#define	R_R31		31
#define	R_F0		32
#define	R_F1		33
#define	R_F2		34
#define	R_F3		35
#define	R_F4		36
#define	R_F5		37
#define	R_F6		38
#define	R_F7		39
#define	R_F8		40
#define	R_F9		41
#define	R_F10		42
#define	R_F11		43
#define	R_F12		44
#define	R_F13		45
#define	R_F14		46
#define	R_F15		47
#define	R_F16		48
#define	R_F17		49
#define	R_F18		50
#define	R_F19		51
#define	R_F20		52
#define	R_F21		53
#define	R_F22		54
#define	R_F23		55
#define	R_F24		56
#define	R_F25		57
#define	R_F26		58
#define	R_F27		59
#define	R_F28		60
#define	R_F29		61
#define	R_F30		62
#define	R_F31		63
#define NCLIENTREGS	64
#define	R_EPC		64
#define	R_MDHI		65
#define	R_MDLO		66
#define	R_SR		67
#define	R_CAUSE		68
#define	R_TLBHI		69
#define	R_TLBLO		70
#define	R_BADVADDR	71
#define	R_INX		72
#define	R_RAND		73
#define	R_CTXT		74
#define	R_EXCTYPE	75
#define R_MODE		76
#define	R_PRID		77
#define R_FCSR		78
#define R_FEIR		79
#define	NREGS		80

#endif /* CPU_R4640 */


/*
** For those who like to think in terms of the compiler names for the regs
*/
#define	R_ZERO		R_R0
#define	R_AT		R_R1
#define	R_V0		R_R2
#define	R_V1		R_R3
#define	R_A0		R_R4
#define	R_A1		R_R5
#define	R_A2		R_R6
#define	R_A3		R_R7
#define	R_T0		R_R8
#define	R_T1		R_R9
#define	R_T2		R_R10
#define	R_T3		R_R11
#define	R_T4		R_R12
#define	R_T5		R_R13
#define	R_T6		R_R14
#define	R_T7		R_R15
#define	R_S0		R_R16
#define	R_S1		R_R17
#define	R_S2		R_R18
#define	R_S3		R_R19
#define	R_S4		R_R20
#define	R_S5		R_R21
#define	R_S6		R_R22
#define	R_S7		R_R23
#define	R_T8		R_R24
#define	R_T9		R_R25
#define	R_K0		R_R26
#define	R_K1		R_R27
#define	R_GP		R_R28
#define	R_SP		R_R29
#define	R_FP		R_R30
#define	R_RA		R_R31

#else /* HARDWARE */

#define	NREGS		50

#endif /* HARDWARE */

#ifdef __cplusplus
}
#endif

#endif /* __IREGDEF_H__ */

