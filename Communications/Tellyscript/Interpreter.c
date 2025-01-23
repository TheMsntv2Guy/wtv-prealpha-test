/* ===========================================================================
	Interpreter.c

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

Byte EmptyStr[1] = {0};	/* Empty string */

/* ----- Function prototypes ------------------------------------------- */

long a2x(Byte *);
static Byte *token2str(short);
static SYMBOL *findsymbol(SYMBOL *, Byte *, SYMBOL *);
static SYMBOL *ifsymbol(SYMBOL *, Byte *, SYMBOL *);

static void compound_statement(SYMBOL *);
static void statement(SYMBOL *);
static void statements(SYMBOL *);
static void skip_statements(SYMBOL *);
static long pfunction(Byte *, SYMBOL *);

static void assign(ENV *);
static void or(ENV *);
static void and(ENV *);
static void eq(ENV *);
static void le(ENV *);
static void plus(ENV *);
static void mult(ENV *);
static void unary(ENV *);
static void variable(ENV *);
static void primary(ENV *);
static void rvalue(ENV *);
static void store(ENV *, long);

static short skipping;		/* Semaphore used for skipping statements */
static Boolean aborted;		/* abort and unthread stack */
static short breaking;		/* true if "break" statement executed */
static short returning;		/* true if "return" statement executed */
static long frtn;			/* Return value from a function */
SYMBOL *Globals;			/* Function/variable symbol table */
Byte *tptr;					/* Running token pointer */
Byte *LoMem;				/* Array allocation starts here */
SYMBOL *SymTop;				/* Last symbol in table */
long linenumber;			/* Line number in source file */
Byte *StackPtr;				/* Arrays and function parameters */
Byte *TokenBuffer;			/* Compiled token buffer */
SYMBOL *EndGlobals;			/* Last global symbol */

char *errstr[] = {
	" ",
	"Unexpected end of file",
	"unrecognized",
	"duplicate identifier",
	"Symbol table full",
	"Out of heap memory",
	"undeclared identifier",
	"Syntax error",
	"unmatched",
	"missing",
	"Not a function",
	"out of place",
	"Token buffer overflow",
	"Divide by zero",
	"Pointer error",
	"Parameter error",
	0
};

/* ----- Set aborted flag ---------------------------------- */

long SI_setabort(long* UNUSED(params))	/* Used by shell as intrinsic function */
{
	return aborted = true;
}

/* ----- Return remaining stack space ---------------------------------- */

long SI_stack(long* UNUSED(params))	/* Used by shell as intrinsic function */
{
	return (Byte *)SymTop - StackPtr;
}

/* ----- Allocate memory on the stack ---------------------------------- */

Byte *allocate(register long size)
{
	register Byte *sp = StackPtr;

	size = (size + 3) & ~3;

	if (size & 1)	/* Make sure stack pointer remains even */
		size++;
	if ((StackPtr += size) >= (Byte *)SymTop)
		error (MEMERR, EmptyStr);
	return sp;
}

/* ----- Start the interpreter ----------------------------------------- */

long SI_Interpret()
{
	long retval;

	skipping = 0;
	aborted = breaking = returning = false;
	tptr = (Byte *)"Imain\0();";
	retval = expression(SymTop);
	return retval;
}

/* ----- A function is called thru a pointer --------------------------- */

static long pfunction(register Byte *fp, SYMBOL *sp)
{
	register short i;
	register short p = 0;			/* Number of parameters */
	Byte *savetptr;					/* Will be saved and restored */
	Byte *ap = StackPtr;			/* Start of local arrays */
	register long *pp;

	needtoken(LPAREN);
	if (!iftoken(RPAREN)) {			/* Scan for actual parameters */
		do {
			pp = (long *)allocate(sizeof(long));
			*pp = expression(sp);	/* Evaluate parameter */
			p++;
		} while (iftoken(COMMA));
		needtoken(RPAREN);
	}
	savetptr = tptr;
	if (*fp == LPAREN) {			/* Call token function */
		tptr = fp;
		needtoken(LPAREN);
		sp = SymTop;				/* Local symbols start here */
		pp = (long *)ap;
		for (i = 0; i < p; i++) {	/* Params into local symbol table */
			short size;
			short ind = 0;
			if (iftoken(CHAR))
				size = 1;
			else if (iftoken(INT))
				size = sizeof(long);
			else
				error(PARAMERR, EmptyStr);
			while (iftoken(POINTER))
				ind++;
			needtoken(IDENT);
			addsymbol(sp, tptr, *pp++, size, ind);
			bypass();
			if (i < p-1)
				needtoken(COMMA);
		}
		StackPtr = ap;				/* Remove parameters from stack */
		needtoken(RPAREN);
		compound_statement(sp);		/* Execute the function */
		SymTop = sp;				/* Release the local symbols */
		breaking = returning = false;
	} else {						/* Call intrinisic function */
		frtn = (*(IFUNC)fp)((long *) ap);
		StackPtr = ap;				/* Remove parameters from stack */
	}
	tptr = savetptr;
	return frtn;					/* The function's return value */
}

/* ----- Execute one statement or a {} block --------------------------- */

static void statements(register SYMBOL *sp)
{
	if (iftoken(LBRACE)) {
		--tptr;
		compound_statement(sp);
	} else
		statement(sp);
}

/* ----- Execute a {} statement block ---------------------------------- */

static void compound_statement(register SYMBOL *sp)
{
	register short tok;

	if (!skipping  && !aborted) {
		register Byte *svtptr = tptr;
		register SYMBOL *spp = SymTop;	/* Local symbol table */
		Byte *app = StackPtr;

		needtoken(LBRACE);
		do {							/* Local variables in block */
			register SYMBOL *symbole;
			short size = 1;
			switch (tok = nexttoken()) {
				case INT:
					size = sizeof(long);
				case CHAR:
					do {
						short ind = 0;
						while (iftoken(POINTER))
							ind++;
						needtoken(IDENT);
						symbole = addsymbol(spp, tptr, 0, size, ind);
						bypass();
						if (iftoken(EQUAL))		/* Handle assignments */
							symbole->value = expression(sp);
						else if (iftoken(LBRACKET)) {	/* Array */
							short n =
								(symbole->size == 1 && symbole->ind == 0) ?
								1 : sizeof(long);
							symbole->value =
								(long)allocate(n * expression(sp));
							(symbole->ind)++;
							needtoken(RBRACKET);
						}
					} while (iftoken(COMMA));
					needtoken(SEMICOLON);
					break;
				default:
					tptr--;
					tok = 0;
			}
		} while (tok);
		while (!iftoken(RBRACE) && !breaking && !returning && !aborted)
			statements(sp);
		SymTop = spp;				/* Free the local symbols */
		StackPtr = app;				/* Free the local arrays */
		tptr = svtptr;				/* Point to the opening brace */
	}
	skippair(LBRACE, RBRACE);		/* Skip to end of block */
}

/* ----- Execute a single statement ------------------------------------ */

static void statement(register SYMBOL * sp)
{
	register long rtn;
	register short tok;

	switch (tok = nexttoken()) {
		case IF:
			/* if ( expression ) statements                 */
			/* if ( expression ) statements else statements */
			if (skipping || aborted) {
				skippair(LPAREN, RPAREN);
				skip_statements(sp);
				while (iftoken(ELSE))
					skip_statements(sp);
				break;
			}
			needtoken(LPAREN);
			rtn = expression(sp);		/* Condidtion beeing tested */
			needtoken(RPAREN);
			if (rtn)
				statements(sp);			/* Condition is true */
			else
				skip_statements(sp);	/* Condition is false */
			while (iftoken(ELSE))
				if (rtn)				/* Do the reverse for else */
					skip_statements(sp);
				else
					statements(sp);
			break;
		case WHILE:
			/* while ( expression) statements */
			if (skipping || aborted) {
				skippair(LPAREN, RPAREN);
				skip_statements(sp);
				break;
			}
			{
				Byte *svtptr = tptr;
				breaking = returning = false;
				do {
					tptr = svtptr;
					needtoken(LPAREN);
					rtn = expression(sp);		/* The condition tested */
					needtoken(RPAREN);
					if (rtn)					/* Condition is true */
						statements(sp);
					else						/* Condition is false */
						skip_statements(sp);
				} while (rtn && !breaking && !returning && !aborted);
				breaking = false;
			}
			break;
		case FOR:
			/* for (expression ; expression ; expression) statements */
			if (skipping || aborted) {
				skippair(LPAREN, RPAREN);
				skip_statements(sp);
				break;
			}
			{
				Byte *fortest, *forloop, *forblock;
				Byte *svtptr = tptr;		/* svtptr -> 1st ( after for */

				needtoken(LPAREN);
				if (!iftoken(SEMICOLON)) {
					expression(sp);			/* Initial expression */
					needtoken(SEMICOLON);
				}
				fortest = tptr;				/* fortest:terminating test */
				tptr = svtptr;
				skippair(LPAREN, RPAREN);
				forblock = tptr;			/* forblock: block to run */
				tptr = fortest;
				breaking = returning = false;
				while (true) {
					if (!iftoken(SEMICOLON)) {
						if (!expression(sp))	/* Terminating test */
							break;
						needtoken(SEMICOLON);
					}
					forloop = tptr;
					tptr = forblock;
					statements(sp);			/* The loop statement(s) */
					if (breaking || returning || aborted)
						break;
					tptr = forloop;
					if (!iftoken(RPAREN)) {
						expression(sp);		/* End of loop expression */
						needtoken(RPAREN);
					}
					tptr = fortest;
				}
				tptr = forblock;
				skip_statements(sp);		/* Skip past the block */
				breaking = false;
			}
			break;
		case RETURN:
			/* return ;            */
			/* return expression ; */
			if (!iftoken(SEMICOLON)) {
				frtn = expression(sp);		/* Function return value */
				needtoken(SEMICOLON);
			}
			returning = !skipping;
			break;
		case BREAK:
			/* break ; */
			needtoken(SEMICOLON);
			breaking = !skipping;
			break;
		case IDENT:
		case POINTER:
		case AUTOINC:
		case AUTODEC:
		case LPAREN:
			/* expression ; */
			--tptr;
			expression(sp);
			needtoken(SEMICOLON);
			break;
		case SEMICOLON:
			/* ; */
			break;
		default:
			error(OUTOFPLACE, token2str(tok));
	}
}

/* ----- Bypass statement(s) ------------------------------------------- */

static void skip_statements(register SYMBOL *sp)
{
	skipping++;			/* Semaphore that suppresses assignments, */
	statements(sp);		/* ...breaks, returns, ++, --, function calls */
	--skipping;			/* Turn off semaphore */
}

/* ----- Recursive descent expression analyzer ------------------------- */

static void rvalue(register ENV *env)			/* Read value */
{
	register short character;

	if (skipping || aborted) {
		env->value = 1;
		env->adr = false;
		return;
	}
	if (env->adr) {
		switch (env->size) {
			case 1:
				character = (env->ind) ? false: true;
				break;
			case 0:
			case sizeof(long):
				character = false;
				break;
			default:
				error(SYNTAX, EmptyStr);
		}

		if (character) {
			register Byte *v = (Byte *)env->value;
			env->value = *v;
		} else {
			register long *v = (long *)env->value;
			env->value = *v;
		}
		env->adr = false;
	}
}

static void store(register ENV *env, register long val)		/* Store value */
{
	register short character;

	if (skipping || aborted)
		return;

	if (env->adr) {
		switch (env->size) {
			case 1:
				character = (env->ind) ? false: true;
				break;
			case sizeof(long):
				character = false;
				break;
			default:
				error(SYNTAX, EmptyStr);
		}
		if (character) {
			register Byte *v = (Byte *)env->value;
			*v = val;
		} else {
			register long *v = (long *)env->value;
			*v = val;
		}
	} else
		error(SYNTAX, EmptyStr);
}

long expression(register SYMBOL *sp)	/* Evaluate expression */
{
	ENV env;

	env.sp = sp;
	assign(&env);
	rvalue(&env);
	return env.value;		/* Return expression result */
}

static void assign(register ENV *env)		/* Handle assignments (=) */
{
	ENV env2;

	or(env);
	while (iftoken(EQUAL)) {
		env2.sp = env->sp;
		assign(&env2);
		rvalue(&env2);
		store(env, env2.value);
	}
}

static void or(register ENV *env)		/* Handle logical or (||) */
{
	ENV env2;

	and(env);
	while (iftoken(OR)) {
		rvalue(env);
		env2.sp = env->sp;
		or(&env2);
		rvalue(&env2);
		env->value = env->value || env2.value;
	}
}

static void and(register ENV *env)	/* Handle logical and (&&) */
{
	ENV env2;

	eq(env);
	while (iftoken(AND)) {
		rvalue(env);
		env2.sp = env->sp;
		and(&env2);
		rvalue(&env2);
		env->value = env->value && env2.value;
	}
}

static void eq(register ENV *env)		/* Handle equal (==) and not equal (!=) */
{
	register short tok;
	ENV env2;

	le(env);
	while (true)
		switch (tok = nexttoken()) {
			case EQUALTO:
				rvalue(env);
				env2.sp = env->sp;
				eq(&env2);
				rvalue(&env2);
				env->value = env->value == env2.value;
				break;
			case NOTEQUAL:
				rvalue(env);
				env2.sp = env->sp;
				eq(&env2);
				rvalue(&env2);
				env->value = env->value != env2.value;
				break;
			default:
				tptr--;
				return;
		}
}

static void le(register ENV *env)		/* Handle relational operators: <= >= < > */
{
	register short tok;
	ENV env2;

	plus(env);
	while (true)
		switch (tok = nexttoken()) {
			case LE:
				rvalue(env);
				env2.sp = env->sp;
				le(&env2);
				rvalue(&env2);
				env->value = env->value <= env2.value;
				break;
			case GE:
				rvalue(env);
				env2.sp = env->sp;
				le(&env2);
				rvalue(&env2);
				env->value = env->value >= env2.value;
				break;
			case LESS:
				rvalue(env);
				env2.sp = env->sp;
				le(&env2);
				rvalue(&env2);
				env->value = env->value < env2.value;
				break;
			case GREATER:
				rvalue(env);
				env2.sp = env->sp;
				le(&env2);
				rvalue(&env2);
				env->value = env->value > env2.value;
				break;
			default:
				tptr--;
				return;
		}
}

static void plus(register ENV *env)			/* Handle addition and substraction */
{
	register short tok;
	register short scale;
	ENV env2;

	mult(env);
	while (true)
		switch (tok = nexttoken()) {
			case PLUS:
				rvalue(env);
				env2.sp = env->sp;
				plus(&env2);
				rvalue(&env2);
				scale = ((env->ind == 1 && env->size == sizeof(long)) ||
					env->ind > 1) ? sizeof(long) : 1;
				env->value += scale * env2.value;
				break;
			case MINUS:
				rvalue(env);
				env2.sp = env->sp;
				plus(&env2);
				rvalue(&env2);
				if (env->ind && env2.ind) {		/* Pointer difference */
					if (env->ind != env2.ind)
						error(POINTERERR, EmptyStr);
					scale = ((env->ind == 1 &&
						env->size == sizeof(long)) ||
						env->ind > 1) ? sizeof(long) : 1;
					env->value = (env->value - env2.value) / scale;
					env->size = sizeof(long);
					env->ind = 0;
				} else {
					scale = ((env->ind == 1 &&
						env->size == sizeof(long)) ||
						env->ind > 1) ? sizeof(long) : 1;
					env->value -= scale * env2.value;
				}
				break;
			default:
				tptr--;
				return;
		}
}

static void mult(register ENV *env)		/* Handle multiplication, division, modulo */
{
	register short tok;
	ENV env2;

	unary(env);
	while (true)
		switch (tok = nexttoken()) {
			case MULTIPLY:
				rvalue(env);
				env2.sp = env->sp;
				mult(&env2);
				rvalue(&env2);
				env->value *= env2.value;
				break;
			case DIVIDE:
				rvalue(env);
				env2.sp = env->sp;
				mult(&env2);
				rvalue(&env2);
	         	if (!env2.value)
					error(DIVIDEERR, EmptyStr);
				env->value /= env2.value;
				break;
			case MODULO:
				rvalue(env);
				env2.sp = env->sp;
				mult(&env2);
				rvalue(&env2);
	         	if (!env2.value)
					error(DIVIDEERR, EmptyStr);
				env->value %= env2.value;
				break;
			default:
				tptr--;
				return;
		}
}

/*
	Check for:
	leading ++
	leading --
	unary -
	pointer indicator (*)
	address operator (&)
	trailing ++
	trailing --
*/

static void unary(register ENV *env)
{
	ENV env2;

	if (iftoken(AUTOINC)) {
		unary(env);
		env2 = *env;
		rvalue(&env2);
		env2.value += ((env->ind == 1 && env->size == sizeof(long)) ||
			env->ind > 1) ? sizeof(long) : 1;
		store(env, env2.value);
		return;
	}

	if (iftoken(AUTODEC)) {
		unary(env);
		env2 = *env;
		rvalue(&env2);
		env2.value -= ((env->ind == 1 && env->size == sizeof(long)) ||
			env->ind > 1) ? sizeof(long) : 1;
		store(env, env2.value);
		return;
	}

	if (iftoken(NOT)) {
		unary(env);
		rvalue(env);
		env->value = !env->value;
		env->size = sizeof(long);
		env->ind = 0;
		env->adr = false;
		return;
	}

	if (iftoken(MINUS)) {
		unary(env);
		rvalue(env);
		env->value = -env->value;
		env->size = sizeof(long);
		env->ind = 0;
		env->adr = false;
		return;
	}

	if (iftoken(POINTER)) {
		unary(env);
		rvalue(env);
		if (!env->ind)
			error(POINTERERR, EmptyStr);
		--(env->ind);
		switch (env->size) {
			case 1:
				env->size = (env->ind) ? sizeof(long) : 1;
				break;
			case sizeof(long):
				env->size = sizeof(long);
				break;
			default:
				error(SYNTAX, EmptyStr);
		}
		env->adr = true;
		return;
	}

	if (iftoken(ADDRESS)) {
		unary(env);
		if (!env->adr)
			error(SYNTAX, EmptyStr);
		env->size = sizeof(long);
		env->ind = 0;
		env->adr = false;
		return;
	}

	variable(env);

	if (iftoken(AUTOINC)) {
		register long value;
		env2 = *env;
		rvalue(&env2);
		value = env2.value +
			(((env->ind == 1 && env->size == sizeof(long)) ||
			env->ind > 1) ? sizeof(long) : 1);
		store(env, value);
		*env = env2;
		return;
	}

	if (iftoken(AUTODEC)) {
		register long value;
		env2 = *env;
		rvalue(&env2);
		value = env2.value -
			(((env->ind == 1 && env->size == sizeof(long)) ||
			env->ind > 1) ? sizeof(long) : 1);
		store(env, value);
		*env = env2;
		return;
	}
}

static void variable(register ENV *env)	/* Variables, arrays and functions */
{
	register short tok;
	register long index;
	register short size;

	primary(env);
	switch (tok = nexttoken()) {
		case LPAREN:
			tptr--;
			rvalue(env);
			if (skipping || aborted) {
				skippair(LPAREN, RPAREN);
				env->value = 1;
			} else
				env->value = pfunction((Byte *)env->value, env->sp);
			env->ind = 0;
			env->size = sizeof(long);
			env->adr = false;
			break;
		case LBRACKET:
			index = expression(env->sp);
			needtoken(RBRACKET);
			rvalue(env);
			if (!env->ind)
				error(SYNTAX, EmptyStr);
			--(env->ind);
			switch (env->size) {
				case 1:
					size = (env->ind) ? sizeof(long) : 1;
					break;
				case sizeof(long):
					size = sizeof(long);
					break;
				default:
					error(SYNTAX, EmptyStr);
			}
			env->value += index * size;
			env->adr = true;
			break;
		default:
			tptr--;
	}
}

static void primary(register ENV *env)	/* Constants, strings and identifiers */
{
	short tok;
	register SYMBOL *sym;

	switch (tok = nexttoken()) {
		case LPAREN:
			assign(env);
			needtoken(RPAREN);
			break;
		case CONSTANT:
			env->value = a2x((Byte *)tptr);
			bypass();
			env->ind = 0;
			env->size = sizeof(long);
			env->adr = false;
			break;
		case STRING:
			env->value = (long)tptr;
			bypass();
			env->ind = 0;
			env->size = sizeof(long);
			env->adr = false;
			break;
		case IDENT:
			/* First check locals, then globals */
			if (!(sym = ifsymbol(env->sp, tptr, SymTop)))
				sym = findsymbol(Globals, tptr, EndGlobals);
			bypass();
			env->value = (long)&sym->value;
			/* Adjust address of char variables */
			if (sym->size == 1 && sym->ind == 0)
				env->value += sizeof(long) - 1;
			env->ind = sym->ind;
			env->size = sym->size;
			env->adr = true;
			break;
		default:
			error(OUTOFPLACE, token2str(tok));
	}
}

/* ----- Skip the tokens between a matched pair ------------------------ */

void skippair(register Byte ltok, register Byte rtok)
{
	register short pairct = 0;
	register Byte tok;

	needtoken(tok = ltok);
	while (true) {
		if (tok == ltok)
			pairct++;
		if (tok == rtok)
			if (--pairct == 0)
				break;
		if ((tok = nexttoken()) == T_EOF)
			error(MATCHERR, token2str(ltok));
	}
}

/* ----- A specified token is required next ---------------------------- */

void needtoken(register Byte tk)
{
	if (nexttoken() != tk)
		error(MISSING, token2str(tk));
}

/* ----- Test for a specified token next in line ----------------------- */

Boolean iftoken(register Byte tk)
{
	if (nexttoken() == tk)
		return true;
	--tptr;
	return false;
}

/* ----- Get the next token from the buffer ---------------------------- */

Byte nexttoken()
{
	while (*tptr == LINENO)
		tptr++;
	return *tptr++;
}

/* ----- Add a symbol to the symbol table ------------------------------ */

SYMBOL *addsymbol(
register SYMBOL *s,			/* Start of local symbol table */
register Byte *name,			/* Pointer to symbol name */
register long value,			/* Value of symbol */
register Byte size,				/* Size of value */
register Byte ind				/* Indirection level */
)
{
	if (ifsymbol(s, name, SymTop))
		error(DUPL_DECLARE, name);		/* Already declared */
	s = --SymTop;
	if ((Byte *)s < StackPtr)
		error(TABLEOVERFLOW, name);		/* Symbol table full */
	s->name = name;
	s->value = value;
	s->size = size;
	s->ind = ind;
	return s;
}

/* ----- Find a symbol on the symbol table (error if not found) -------- */

static SYMBOL *findsymbol(register SYMBOL *s, register Byte *sym, register SYMBOL *ends)
{
	if (!(s = ifsymbol(s, sym, ends)))
		error(UNDECLARED, sym);
	return s;
}

/* ----- Test for a symbol on the symbol table ------------------------- */

static SYMBOL *ifsymbol(register SYMBOL *s, register Byte *sym, register SYMBOL *sp)
{
	while (sp < s) {
		if (!strcmp((char *)sym, (char *)sp->name))
			return sp;
		sp++;
	}
	return NULL;
}

/* ----- Post an error to the shell ------------------------------------ */

void error(register enum errs erno, register Byte *s)
{
	register Byte *p;
	register n;
	char str[256];
	
	if (linenumber)
		n = linenumber;
	else {
		if (tptr < TokenBuffer || tptr >= LoMem)
			n = 0;	/* Happens if main() is not found */
		else {
			for (n = 1, p = TokenBuffer; p <= tptr; p++)
				if (*p == LINENO)
					n++;
		}
	}
	strcpy(str, errstr[erno]);
	strcat(str, " :");
	strcat(str, (char *) s);
	SI_Error(erno, (Byte *) str, n);
}

/* ----- Convert token to string (for error messages) ------------------ */

static Byte *token2str(register short token)
{
	static Byte s[2];
	register Byte *p = s;

	switch (token) {
		case AUTOINC:
			*p++ = '+';
			*p++ = '+';
			break;
		case AUTODEC:
			*p++ = '-';
			*p++ = '-';
			break;
		case EQUALTO:
			*p++ = '=';
			*p++ = '=';
			break;
		case NOTEQUAL:
			*p++ = '!';
			*p++ = '=';
			break;
		case GE:
			*p++ = '>';
			*p++ = '=';
			break;
		case LE:
			*p++ = '<';
			*p++ = '=';
			break;
		case AUTOADD:
			*p++ = '+';
			*p++ = '=';
			break;
		case AUTOSUB:
			*p++ = '-';
			*p++ = '=';
			break;
		case AUTOMUL:
			*p++ = '*';
			*p++ = '=';
			break;
		case AUTODIV:
			*p++ = '/';
			*p++ = '=';
			break;
		case AND:
			*p++ = '&';
		case ADDRESS:
			*p++ = '&';
			break;
		case OR:
			*p++ = '|';
		default:
			*p++ = token;
	}
	*p = '\0';
	return s;
}

/* ----- Convert long to string ---------------------------------------- */
#ifdef SIMULATOR
void x2str(register long num, register Byte *str)
{
#ifdef HAVE_ANSI_LIB
	(void)sprintf((char *)str, "%ld", num);
#else
	register short n;
	register Byte nibble;
	register Byte *ostr = str;
	register Boolean digitseen = false;

	for (n = 28; n >=0 ; n -= 4) {
		nibble = (num >> n) & ((Byte) 0x0F);
		if (nibble || digitseen)
			{
			*str++ = nibble | ((Byte) 0x30);
			digitseen = true;
			}
		}
	*str = 0;
#endif
}
#endif

/* ----- Convert string to long ---------------------------------------- */

long a2x(register Byte *s)
{
#ifdef HAVE_ANSI_LIB
	return atol((char *)s);
#else
	register unsigned long v = 0;

	while (isspace(*s))
		s++;

	while (*s >= (Byte) 0x30 && *s <= ((Byte) 0x3F))	/* '0' .. '?' */
		v = (v << 4) + (*s++ & ((Byte) 0x0F));
	
	return (long)v;
#endif	/* HAVE_ANSI_LIB */
}
