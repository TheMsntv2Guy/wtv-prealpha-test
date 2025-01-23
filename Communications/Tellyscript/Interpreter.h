/* ===========================================================================
	Interpreter.h

	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
=========================================================================== */

#ifndef __INTERPRETER_H__
#define __INTERPRETER_H__

/* ----- Intrinsic function table (provided by shell) ------------------ */

typedef long (*IFUNC)(long *);

typedef struct {
	Byte *fname;
	IFUNC fn;	/* Parameter is (long *) */
} INTRINSIC;

/* ----- Functions provided by interpreter ----------------------------- */

long SI_Interpret(void);					/* Start the interpreter */
long SI_stack(long *);				/* Remaining stack space */
long SI_setabort(long *);

/* ----- Functions provided by the shell ------------------------------- */

Byte SI_GetSource(void);					/* Get next char from source */
void SI_UngetSource(Byte);					/* char not needed now */
void SI_Error(short, Byte *, short);		/* Error from interpreter */

#else
#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
#error "Attempted to #include Interpreter.h multiple times"
#endif
#endif /* __INTERPRETER_H__ */