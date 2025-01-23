/* ===========================================================================
	Scanner.c

	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
=========================================================================== */

#include "Headers.h"

#ifndef __INTERPRETER_H__
#include "Interpreter.h"
#endif
#ifndef __TELLYDEFS_H__
#include "TellyDefs.h"
#endif
#ifndef __SCANNER_H__
#include "Scanner.h"
#endif

/* ===========================================================================
	#defines, consts, enums, etc.
=========================================================================== */

#define LINE 256					/* Maximum line length */

extern SYMBOL *Globals;				/* Function/variable symbol table */
extern Byte *tptr;					/* Running token pointer */
extern Byte *LoMem;					/* Array allocation starts here */
extern SYMBOL *SymTop;				/* Last symbol in table */
extern long linenumber;				/* Line number in source file */
extern Byte *StackPtr;				/* Arrays and function parameters */
extern Byte *TokenBuffer;			/* Compiled token buffer */
extern SYMBOL *EndGlobals;			/* Last global symbol */

extern Byte *EmptyStr;				/* Empty string */
extern char *errstr;

static Byte gettoken(void);
static Byte getok(void);
static Byte iskeyword(void);
static Byte isident(void);
static Byte istoken(void);
static Byte getword(void);
static Byte getcx(void);
static Byte escseq(void);
static Byte h2(void);

/* ----- Characters in source, not copied to token buffer -------------- */

#define COMMENT1	'/'
#define COMMENT2	'*'
#define QUOTES		'"'
#define QUOTE		'\''

/* ----- Table of keywords and their tokens ---------------------------- */

static struct keywords {
	Byte *kw;
	Byte kwtoken;
} kwds[] = {
	(Byte *)EOLNS,		LINENO,
	(Byte *)"break",	BREAK,
	(Byte *)"char",		CHAR,
	(Byte *)"else",		ELSE,
	(Byte *)"for",		FOR,
	(Byte *)"if",		IF,
	(Byte *)"int",		INT,
	(Byte *)"return",	RETURN,
	(Byte *)"while",	WHILE,
	NULL,				0
};

/* ----- Table of direct translate tokens ------------------------------ */

static Byte tokens[] = {
	COMMA, LBRACE, RBRACE, LPAREN, RPAREN, EQUAL, NOT, POINTER,
	LESS, GREATER, AND, OR, SEMICOLON, LBRACKET, RBRACKET,
	MULTIPLY, DIVIDE, MODULO, PLUS, MINUS, T_EOF, 0
};

/* ----- Local data ---------------------------------------------------- */

/*
	Memory layout:                 		<- Globals
	High addr	+---------------------+
				| global symbols      |
				|.....................| <- EndGlobals
				|                     |
				| local symbol        |
				| (function params)   |	<- SymTop (grows down)
				+---------------------+
				|                     |
				| free memory         |
				|                     | <- StackPtr (grows up)
				+---------------------+
				|                     | 
				| arrays and function |
				| parameters          | <- LoMem
				+---------------------+
				|                     |
				| token buffer        |
				|                     | <- TokenBuffer
	Low addr	+---------------------+

*/

/* ----- Return the next token ----------------------------------------- */

#ifdef SIMULATOR	// currently unused: this is never defined
static Byte gettoken()
{
	register Byte tok;

	tok = getword();
	if (!tok)						/* Not a char/string constant */
		if (!(tok = iskeyword()))	/* No keyword */
			if (!(tok = istoken()))	/* No one character token */
				tok = isident();	/* Then should be ident. or constant */
	if (!tok)
		error(UNRECOGNIZED, tptr);
	return tok;
}

/* ----- Test to see if current word is a one character token ---------- */

static Byte istoken()
{
	register Byte *t = tokens;	/* Single character tokens */
	register Byte t2;

	if (strlen((char *)tptr) != 1)
		return 0;
	while (*t)
		if (*tptr == *t++) {
			switch ((char) *tptr) {
				case (char) T_EOF:
					break;
				case AND:		/* Distinction between & and && */
					if ((t2 = getcx()) != AND) {
						*tptr = ADDRESS;
						SI_UngetSource(t2);
					}
					break;
				case OR:		/* Must be || */
					if (getcx() != OR)
						error(MISSING, tptr);
					break;
				case PLUS:		/* Distinction between +, ++ and += */
				case MINUS:		/* Distinction between -, -- and -= */
					if ((t2 = getcx()) == *tptr)
						*tptr = (*tptr == PLUS) ? AUTOINC : AUTODEC;
					else if (t2 == EQUAL)
						*tptr = (*tptr == PLUS) ? AUTOADD : AUTOSUB;
					else
						SI_UngetSource(t2);
					break;
				case RBRACE:	/* May be last token */
				case SEMICOLON:
					break;
				default:
					if ((t2 = getcx()) == EQUAL) {
						switch (*tptr) {
							case EQUAL:				/* == */
								return EQUALTO;
							case NOT:				/* != */
								return NOTEQUAL;
							case LESS:				/* <= */
								return LE;
							case GREATER:			/* >= */
								return GE;
							case MULTIPLY:			/* *= */
								return AUTOMUL;
							case DIVIDE:			/* /= */
								return AUTODIV;
							case MODULO:			/* %= */
								return AUTOMOD;
						}
					}
					SI_UngetSource(t2);
					break;
			}
			return *tptr;
		}
	return 0;
}
#endif

/* ----- Lexical scan and call linker ---------------------------------- */
void AddIntrinsics(register INTRINSIC *intrinsics)
{
	/* Add intrinsic functions to symbol table */

	StackPtr = LoMem = tptr;
	for ( ; intrinsics->fn; intrinsics++)
		addsymbol(Globals,intrinsics->fname,(long)intrinsics->fn,0,0);
}

void SetupPointers(Byte *memory, long size, Byte *pointer)
{
	/* Set up memory pointers */

	size = (size + 3) & ~3;

	if (size & 1)		/* Make sure address is even */
		size--;
	LoMem = (Byte *)(SymTop = Globals = (SYMBOL *)((TokenBuffer = memory) + size)) - LINE;
	if (pointer == 0)
		tptr = memory;
	else
		{
		tptr = (Byte *)(((long)pointer + 3) & ~3);
	
		if ((long)tptr & 1)	/* Make sure address is even for the next guy */
			tptr++;
		}

	linenumber = 0;	/* From now on error() must count LINENO tokens */
}

#ifdef SIMULATOR
long LoadTokenBuffer()
{
	register short	tok;
	register short	n;
	Byte			*start = tptr;
	
	/* Load token buffer */

	linenumber = 1;
	do {
		if (tptr >= LoMem)
			error(BUFFULL, EmptyStr);
		n = linenumber;
		tptr++;
		tok = gettoken();
		*(tptr - 1) = tok;

		n = linenumber - n;
		switch (tok) {
			case CONSTANT:
			case IDENT:
			case STRING:
				bypass();
				break;
			case LINENO:
				++linenumber;
				break;
		}
		while (n--) {
			if (tptr >= LoMem)
				error(BUFFULL, EmptyStr);
			*tptr++ = LINENO;
		}
	} while (tok != T_EOF);

	tptr = (Byte *)(((long)tptr + 3) & ~3);

	if ((long)tptr & 1)	/* Make sure address is even for the next guy */
		tptr++;
	linenumber = 0;	/* From now on error() must count LINENO tokens */

	return tptr - start;
}
#endif

void LinkFunctions()
{
	register short tok;

	/* Link the global variables and functions */

	tptr = TokenBuffer;
	while ((tok = nexttoken()) != T_EOF) {
		if (tok == CHAR || tok == INT) {		/* Variable declaration */
			do {
				register SYMBOL *symbole;
				short ind = 0;
				while (iftoken(POINTER))
					ind++;						/* char *xyz */
				needtoken(IDENT);
				symbole = addsymbol(Globals, tptr, 0,
					(tok == CHAR) ? 1 : sizeof(long), ind);
				bypass();
				if (iftoken(LBRACKET)) {
					if (iftoken(RBRACKET))		/* xyz[] */
						(symbole->ind)++;
					else {						/* xyz[...] */
						short size;
						size = (symbole->size == 1 && symbole->ind == 0) ?
							1 : sizeof(long);
						symbole->value =
							(long)allocate(size * expression(Globals));
						(symbole->ind)++;
						needtoken(RBRACKET);
					}
				}
				if (iftoken(EQUAL)) {
					if (iftoken(LBRACE)) {		/* x = { xxx, ... } */
						long *p;
						symbole->value = (long)StackPtr;
						do {
							p = (long *)allocate(sizeof(long));
							*p = expression(Globals);
						} while (iftoken(COMMA));
						needtoken(RBRACE);
					} else {					/* x = xxx */
						symbole->value = expression(Globals);
					}
				}
			} while (iftoken(COMMA));
			needtoken(SEMICOLON);
		} else if (tok == IDENT) {		/* Function definition */
			Byte *name = tptr;
			bypass();
			addsymbol(Globals, name, (long)tptr, 0, 0);
			skippair(LPAREN, RPAREN);
			skippair(LBRACE, RBRACE);			/* xyz(...) {...} */
		} else
			error(EARLYEOF, (Byte *) "LinkFunctions()");
	}
	EndGlobals = SymTop;
}

/* ----- Test word for a keyword --------------------------------------- */

#ifdef SIMULATOR
static Byte iskeyword()
{
	register struct keywords *k = kwds;

	while (k->kw)
		if (!strcmp((char *)k->kw, (char *)tptr))
			return k->kwtoken;
		else
			k++;
	return 0;
}

/* ----- Test for an ident (or constant) ------------------------------- */

static Byte isident()
{
	register Byte *wd = tptr;
	register long n = 0;

	if (iscsymf(*wd))			/* Letter or underscore */
		return IDENT;
	if (!strncmp((char *)wd, "0x", 2) || !strncmp((char *)wd, "0X", 2)) {
		wd += 2;				/* 0x... hex constant */
		while (*wd) {
			if (!isxdigit(*wd))
				return 0;		/* Not a hex digit */
			n = (n << 4) + (isdigit(*wd) ? *wd - '0':
				tolower(*wd) - 'a' + 10);
			wd++;
		}
	} else
		while (*wd) {
			if (!isdigit(*wd))
				return 0;		/* Not a digit */
			n = (n * 10) + (*wd -'0');
			wd++;
		}
	x2str(n, (Byte *)tptr);		/* Converted constant */
	return CONSTANT;
}

/* ----- Get the next word from the input stream ----------------------- */

static Byte getword()
{
	register Byte *wd = tptr;
	register Byte c;
	register Byte tok;

	do
		c = getok();				/* Bypass white space */
	while (iswhite(c));
	if (c == QUOTE) {
		register unsigned long n = 0;
		register short max = 4;		/* Maximum 4 characters */
		while ((c = getcx()) != QUOTE) {
			if (!max)
				error(MISSING, (Byte *)"'");/* Needs the other quote */
			max--;
			if (c  == '\\')			/* Escape sequence (\015) */
				c = escseq();
			n = (n << 8) | (c & ((Byte) 0xFF));
		}
		x2str(n, (Byte *)tptr);		/* Build the constant value */
		return CONSTANT;
	}
	if (c == QUOTES) {
		tok = STRING;				/* Quoted string "abc" */
		while ((c = getcx()) != QUOTES)
			*wd++ = (c == '\\') ? escseq() : c;
	} else {
		tok = 0;
		*wd++ = c;					/* 1st char of word */
		while (iscsym(c)) {			/* Build an ident */
			c = getok();
			if (iscsym(c))
				*wd++ = c;
			else
				SI_UngetSource(c);
		}
	}
	*wd = '\0';		/* Null terminate the string or word */
	return tok;
}

/* ----- Escape sequence in litteral constant or string ---------------- */

static Byte h2()
{
	register Byte v = 0;
	register short n = 2;
	register Byte c;

	while (n--) {
		c = getcx();
		if (!isxdigit(c)) {
			Byte s[2];
			s[0] = c;
			s[1] = 0;
			error(OUTOFPLACE, s);	/* Not a hex digit */
		}
		v = (v << 4) + (isdigit(c) ? c - '0': tolower(c) - 'a' + 10);
	}
	return v;
}

static Byte escseq()
{
	register Byte c = getcx();

	return (c == 'n' ? '\012' :				/* 0x0A (LF)	*/
		c == 't' ? '\011' :					/* 0x09 (TAB)	*/
		c == 'f' ? '\014' :					/* 0x0C (FF)	*/
		c == 'a' ? '\007' :					/* 0x07 (BEL)	*/
		c == 'b' ? '\010' :					/* 0x08 (BS)	*/
		c == 'r' ? '\015' :					/* 0x0D (CR)	*/
		c == 'v' ? '\013' :					/* 0x0B	(VT)	*/
		c == '0' ? '\0' :					/* 0x00 (NUL)	*/
		(c == 'x') || (c == 'X') ? h2() :	/* 2 hex digits */
		c);
}

/* ----- Get a character from the input stream ------------------------- */

static Byte getok()
{
	register short c;
	register short c1;

	while ((c = SI_GetSource()) == COMMENT1) {
		if ((c1 = SI_GetSource()) != COMMENT2) {
			SI_UngetSource(c1);
			break;
		}
		do {
			while ((c1 = getcx()) != COMMENT2)
				if (c1 == EOLN)
					++linenumber;
			c1 = getcx();
			if (c1 == EOLN)
				++linenumber;
		} while (c1 != COMMENT1);
	}
	return c;
}

/* ----- Read a character from input, error if EOF --------------------- */

static Byte getcx()
{
	register short c;

	if ((c = SI_GetSource()) == -1)
		error(EARLYEOF, (Byte *) "getcx");
	return c;
}
#endif

