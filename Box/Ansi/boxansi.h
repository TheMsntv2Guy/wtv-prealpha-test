#ifndef __BOXANSI_H__
#define __BOXANSI_H__

#ifndef _LIMITS
#include "limits.h"
#endif

/* ---------------------------------------------------------------------------
	type stuff from <ctype.h>, <time.h>, <yvals.h>
--------------------------------------------------------------------------- */

#ifndef __WTVTYPES_H__
#include "WTVTypes.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------------
	stuff from stddef.h
--------------------------------------------------------------------------- */
#ifndef offsetof
#define offsetof(structure,field) ((size_t)&((structure *) 0)->field)
#endif
/* ---------------------------------------------------------------------------
	va_arg stuff from stdarg.h
--------------------------------------------------------------------------- */

typedef char * va_list;

#define __va_rounded_size(__TYPE)  \
  		(((sizeof(__TYPE) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))

#define va_start(__AP, __LASTARG) 						\
 		(__AP = ((char *) &(__LASTARG) + __va_rounded_size (__LASTARG)))

#define va_end(__AP)

/* For big-endian machines.  */
#define va_arg(__AP, __type)	(((__type *)((__AP) =						\
									(char *)((sizeof(__type) > 4)			\
										? (((int)(__AP) + 2*8 - 1) & -8) :	\
										  (((int)(__AP) + 2*4 - 1) & -4))))[-1])
/* ---------------------------------------------------------------------------
	character macros from <ctype.h>
--------------------------------------------------------------------------- */

#define _XA		0x200 /* extra alphabetic */
#define _XS		0x100 /* extra space */
#define _BB		0x80 /* BEL, BS, etc. */
#define _CN		0x40 /* CR, FF, HT, NL, VT */
#define _DI		0x20 /* '0'-'9' */
#define _LO		0x10 /* 'a'-'z' */
#define _PU		0x08 /* punctuation */
#define _SP		0x04 /* space */
#define _UP		0x02 /* 'A'-'Z' */
#define _XD		0x01 /* '0'-'9', 'A'-'F', 'a'-'f' */

int isalnum(int), isalpha(int), iscntrl(int), isdigit(int);
int isgraph(int), islower(int), isprint(int), ispunct(int);
int isspace(int), isupper(int), isxdigit(int);
int tolower(int), toupper(int);

extern const short * _BoxCtype;
extern const short * _BoxTolower;
extern const short * _BoxToupper;

#define isalnum(c)	(_BoxCtype[((unsigned char)(c))] & (_DI|_LO|_UP|_XA))
#define isalpha(c)	(_BoxCtype[((unsigned char)(c))] & (_LO|_UP|_XA))
#define iscntrl(c)	(_BoxCtype[((unsigned char)(c))] & (_BB|_CN))
#define isdigit(c)	(_BoxCtype[((unsigned char)(c))] & _DI)
#define isgraph(c)	(_BoxCtype[((unsigned char)(c))] & (_DI|_LO|_PU|_UP|_XA))
#define islower(c)	(_BoxCtype[((unsigned char)(c))] & _LO)
#define isprint(c)	(_BoxCtype[((unsigned char)(c))] & (_DI|_LO|_PU|_SP|_UP|_XA))
#define ispunct(c)	(_BoxCtype[((unsigned char)(c))] & _PU)
#define isspace(c)	(_BoxCtype[((unsigned char)(c))] & (_CN|_SP|_XS))
#define isupper(c)	(_BoxCtype[((unsigned char)(c))] & _UP)
#define isxdigit(c)	(_BoxCtype[((unsigned char)(c))] & _XD)
#define tolower(c)	(_BoxTolower[((unsigned char)(c))])
#define toupper(c)	(_BoxToupper[((unsigned char)(c))])

/* not part of ansi, but what the hell */
int	isascii(int c);
int toascii(int c);

#define isascii(c)	((unsigned)(c) <= 0177)
#define	toascii(c)	((c) & 0177)

/* ---------------------------------------------------------------------------
	functions from <stdio.h>
--------------------------------------------------------------------------- */
#ifdef SIMULATOR
#define INCLUDE_FPRINTF
#endif

#ifdef INCLUDE_FPRINTF
	#ifndef __MWERKS__
		#ifndef FILE
			typedef void * FILE;
		#endif
		#ifndef stdin
			#define stdin	((FILE*) kBusError) /* important that it's non-nil */
		#endif
		#ifndef stdout
			#define stdout	((FILE*) kBusError) /* important that it's non-nil */
		#endif
	#else
		#include <stdio.h>
	#endif
#endif

#ifndef EOF
	#define	EOF	(-1)
#endif

int				getchar(void);										/* chario.c */
int				putchar(int);										/* chario.c */

int				nullprintf(const char*, ...); 						/* printf.c */
int				printf(const char *, ...);							/* printf.c */
#ifdef INCLUDE_FPRINTF
int				fprintf(FILE *, const char *, ...);					/* printf.c */
#endif /* #ifdef INCLUDE_FPRINTF */
int				sprintf(char *, const char *, ...);					/* printf.c */
int				snprintf(char *, size_t num, const char *, ...);	/* printf.c */

int				vnullprintf(const char*, va_list);					/* printf.c */
int				vprintf(const char *, va_list);						/* printf.c */
#ifdef INCLUDE_FPRINTF
int				vfprintf(FILE *, const char *, va_list);			/* printf.c */
#endif /* #ifdef INCLUDE_FPRINTF */
int				vsprintf(char *, const char *, va_list);			/* printf.c */
int				vsnprintf(char *, size_t num, const char *, va_list);	/* printf.c */

int				sscanf(const char *, const char *, ...);		/* sscanf.c */

/* ---------------------------------------------------------------------------
	functions from <stdlib.h> (alphabetical order)
--------------------------------------------------------------------------- */
#ifdef SIMULATOR
	/* temporary stopgap until we purge free/malloc/calloc from WebTV... */
	void free(void* ptr);
	#define free(ptr)			DisposeSimulatorMemory((char*)ptr)
	#define malloc(size)		((void*)NewSimulatorMemory(size, false))
	#define calloc(nelem, size)	((void*)NewSimulatorMemory(nelem*size, true))
#endif

int				atoi(const char *);							/* atoi.c */
long			atol(const char *);							/* atoi.c */
long			strtol(const char *, char **, int);			/* strtol.c */
unsigned long	strtoul(const char *, char **, int);		/* strtoul.c */

/* ---------------------------------------------------------------------------
	functions from <string.h> (alphabetical order)
--------------------------------------------------------------------------- */

void*			memchr(const void *, int, size_t);
int				memcmp(const void *, const void *, size_t);
void*			memcpy(void *, const void *, size_t);
void*			memmove(void *, const void *, size_t);
void*			memset(void *, int, size_t);

char*			strcat(char *, const char *);				/* strcat.c */
char*			strchr(const char *, int);					/* str.c */
int 			strcmp(const char *, const char *);
char*			strcpy(char *, const char *);
/*size_t		strcspn(const char *, const char *);*/
size_t			strlen(const char *);
char*			strncat(char *, const char *, size_t);
int				strncmp(const char *, const char *, size_t);
char*			strncpy(char *, const char *, size_t);
char*			strrchr(const char *, int);
size_t			strspn(const char *cs, const char *ct);
char*			strstr(const char *, const char *);

/* ----------------------------------------
	empty string (declared in ctype.c)
---------------------------------------- */

extern const char _kEmptyString[];
#define kEmptyString (&_kEmptyString[0])


/* ---------------------------------------------------------------------------
	These are not yet implemented...
--------------------------------------------------------------------------- */
/*void *			memmove(void *, const void *, size_t);					*/
/*int				strcoll(const char *, const char *);					*/
/*char *			strerror(int);											*/
/*char *			strncat(char *, const char *, size_t);					*/
/*int				strncmp(const char *, const char *, size_t);			*/
/*size_t			strspn(const char *, const char *);						*/
/*char *			strtok(char *, const char *);							*/
/*size_t			strxfrm(char *, const char *, size_t);					*/
/*char *			_Strerror(int, char *);									*/
/*void *			memcpy(void *, const void *, size_t);					*/
/*char *			strpbrk(const char *, const char *);					*/

#ifdef __cplusplus
}
#endif

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include boxansi.h multiple times"
	#endif
#endif /* __BOXANSI_H__ */

