#ifndef __SCANNER_H__
#define __SCANNER_H__

/* ----- Tokens (found in token buffer) -------------------------------- */

#define LINENO		127		/* '\015', must be unique */
#define BREAK		'b'		/* break */
#define CHAR		'c'		/* char */
#define ELSE		'e'		/* else */
#define FOR			'f'		/* for */
#define IF			'i'		/* if */
#define INT			'l'		/* int */
#define RETURN		'r'		/* return */
#define WHILE		'w'		/* while */

#define IDENT		'I'		/* <identifier> */
#define CONSTANT	'C'		/* <constant> */
#define STRING		'S'		/* <string> */

#define AUTOINC		'P'		/* ++ */
#define AUTODEC		'D'		/* -- */
#define EQUALTO		'E'		/* == */
#define NOTEQUAL	'N'		/* != */
#define GE			'G'		/* >= */
#define LE			'L'		/* <= */
#define AUTOADD		'A'		/* += */
#define AUTOSUB		'B'		/* -= */
#define AUTOMUL		'M'		/* *= */
#define AUTODIV		'V'		/* /= */
#define AUTOMOD		'M'		/* %= */
#define ADDRESS		'@'		/* &  */

#define AND			'&'		/* && */
#define OR			'|'		/* || */
#define POINTER		'*'		/* pointer */
#define PLUS		'+'
#define	MINUS		'-'
#define MULTIPLY	'*'
#define DIVIDE		'/'
#define MODULO		'%'
#define EQUAL		'='
#define LESS		'<'
#define GREATER		'>'
#define NOT			'!'
#define LPAREN		'('
#define RPAREN		')'
#define LBRACE		'{'
#define RBRACE		'}'
#define LBRACKET	'['
#define RBRACKET	']'
#define COMMA		','
#define SEMICOLON	';'

/* ----- Symbol table structure ---------------------------------------- */

typedef struct {
	Byte *name;			/* Points to symbol name (in token buffer) */
	long value;			/* Value (integer or pointer) */
	Byte size;			/* 0: function, 1: char, 4: int */
	Byte ind;			/* Indirection level */
} SYMBOL;

/* ----- Environment for expression evaluation ------------------------- */

typedef struct {
	SYMBOL *sp;			/* Local symbol table pointer */
	long value;			/* Value or address of variable */
	Byte size;			/* 0: function, 1: char, 4: int */
	Byte ind;			/* Indirection level */
	Byte adr;			/* 0: value, 1: address */
} ENV;

/* ----- Error codes -------------------------------------------------- */

enum errs {
	EARLYEOF = 1,		/* Unexpected end of file */
	UNRECOGNIZED,		/* ... unrecognized */
	DUPL_DECLARE,		/* ... duplicate identifier */
	TABLEOVERFLOW,		/* Symbol table full */
	MEMERR,				/* Out of heap memory */
	UNDECLARED,			/* ... undeclared identifier */
	SYNTAX,				/* Syntax error */
	MATCHERR,			/* ... unmatched */
	MISSING,			/* ... missing */
	NOTFUNC,			/* Not a function */
	OUTOFPLACE,			/* ... out of place */
	BUFFULL,			/* Token buffer overflow */
	DIVIDEERR,			/* Divide by zero */
	POINTERERR,			/* Pointer error */
	PARAMERR			/* Parameter error */
};

void error(enum errs, Byte *);
SYMBOL *addsymbol(SYMBOL *, Byte *, long, Byte, Byte);
Byte nexttoken(void);
void needtoken(Byte);
Boolean iftoken(Byte);
Byte *allocate(long);
void skippair(Byte, Byte);
void x2str(long, Byte *);

long expression(SYMBOL *);

void AddIntrinsics(register INTRINSIC *intrinsics);
void SetupPointers(Byte *memory, long size, Byte* pointer);
long LoadTokenBuffer(void);
void LinkFunctions(void);

/* ----- Function macros ----------------------------------------------- */

#define bypass()			tptr += strlen((char *)tptr) + 1
#define iswhite(c)			(c == ' ' || c == '\t')
#define iscsymf(c)			(isalpha(c) || c == '_')
#define iscsym(c)			(isalnum(c) || c == '_')

#endif /* __SCANNER_H__ */