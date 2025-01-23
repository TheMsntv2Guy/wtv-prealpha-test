/* ===========================================================================
 *	sscanf.c
 *
 *	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
 * ======================================================================== */

#include "Headers.h"




static int
_sscanf_char(const char** buf, int* nchar, const char code,
			va_list* args, int noStore, int width)
{
	int result = 0;
	char* dest = NULL;
	char ch;

	if (noStore != 0)
	{	dest = va_arg(*args, char*);
		if (!ValidWriteLocation(dest))
			dest = NULL;
	}
	
	if ( IsError(code != 'c') )
		return 0;
		
	
	if (width == 0)
		width = 1;
	while (width-- > 0)
	{
		if ((ch = **buf) != '\0')
		{
			(*buf)++;
			(*nchar)++;
			if (dest != NULL)
				*dest++ = ch;
			if (noStore == 0)
				result = 1;
		}
		else
		{	*buf = nil;	/* tell _sscanf we hit EOF */
			return 0;
		}
	}
	return result;
}

static int
_sscanf_float(const char** UNUSED(buf), int* UNUSED(nchar), const char code,
			va_list* UNUSED(args), int UNUSED(noStore), int UNUSED(width), int UNUSED(qual))
{
	if ( IsError((code != 'g') && (code != 'G')) )
		return 0;
	PostulateFinal(false);
	Message(("trying to scan floats"));
	
	return 0;
}

#define MAX_SSCANF_INT_DIGITS 40	/* just a guess...32 bit binary plus sign bit or whatever */
static int
_sscanf_int(const char** buf, int* nchar, const char code,
			va_list* args, int noStore, int width, int qual)
{
	static const char flit[] = "diouxXp";
	static const char bases[] = {10, 0, 8, 10, 16, 16, 16};
	static const char digits[] = "0123456789ABCDEFabcdef";

	char tempBuf[MAX_SSCANF_INT_DIGITS];
	char *p;
	char ch;
	char seenDigit = false;
	
	if ( IsError(strchr(flit, code) == NULL) )
		return 0;
	int base = bases[(const char*)strchr(flit, code) - flit];
	int validDigits;
	
	if ((width == 0) || (width > MAX_SSCANF_INT_DIGITS))
		width = MAX_SSCANF_INT_DIGITS;

	p = tempBuf;
	ch = **buf;
	
	if ((ch == '+') || (ch == '-'))
	{
		*p++ = ch;
		ch = *(++(*buf));
		(*nchar)++;
		width--;
	}
	if (ch == '0')
	{
		seenDigit = true;
		*p++ = ch;
		ch = *(++(*buf));
		(*nchar)++;
		width--;
		if (((ch == 'x') || (ch == 'X')) && ((base == 0) || (base == 16)))
		{
			base = 16;
			*p++ = ch;
			ch = *(++(*buf));
			(*nchar)++;
			width--;
		}
		else
		if (base == 0)
		{
			base = 8;
		}
	}
	validDigits = base;
	if (validDigits == 0)
		validDigits = 10;
	if (validDigits == 16)
		validDigits = 16+6;
	
	while (memchr(digits, ch, validDigits) && (width > 0))
	{	*p++ = ch;
		ch = *(++(*buf));
		(*nchar)++;
		width--;
		seenDigit = true;
	}
	
	if (!seenDigit)
	{	*buf = "";	/* no more! */
		return 0;
	}
	
	*p = '\0';
	
	if (noStore)
	{	return 0;
	}
	
	if ((code == 'd') || (code == 'i'))
	{
		long lval = strtol(tempBuf, NULL, base);
		
		if (qual == 'h')
		{
			short* svalp = va_arg(*args, short*);
			if (ValidWriteLocation(svalp))
				*svalp = (short)lval;
		}
		else if (qual != 'l')
		{
			int* ivalp = va_arg(*args, int*);
			if (ValidWriteLocation(ivalp))
				*ivalp = (int)lval;
		}
		else
		{
			long* lvalp = va_arg(*args, long*);
			if (ValidWriteLocation(lvalp))
				*lvalp = lval;
		}
	}
	else
	{
		unsigned long ulval = strtoul(tempBuf, NULL, base);
		
		if (code == 'p')
		{
			void** vvalp = va_arg(*args, void**);
			if (ValidWriteLocation(vvalp))
				*vvalp = (void*)ulval;
		}
		else if (qual == 'h')
		{
			unsigned short* usvalp = va_arg(*args, unsigned short*);
			if (ValidWriteLocation(usvalp))
				*usvalp = (unsigned short)ulval;
		}
		else if (qual != 'l')
		{
			unsigned int* uivalp = va_arg(*args, unsigned int*);
			if (ValidWriteLocation(uivalp))
				*uivalp = (unsigned int)ulval;
		}
		else
		{
			unsigned long* ulvalp = va_arg(*args, unsigned long*);
			if (ValidWriteLocation(ulvalp))
				*ulvalp = ulval;
		}
	}
	return 1;
}

static int
_sscanf_scanset(const char** buf, int* nchar, const char** fmt,
			va_list* args, int noStore, int width)
{
	char* dest = NULL;
	char ch;
	char wantMatch = true;
	void* match;
	const char* scanSet;
	size_t scanSetSize;

	if (**fmt == '^')
	{	wantMatch = false;
		(*fmt)++;
	}
	
	scanSet = *fmt;
	*fmt = strchr((**fmt=='['/* ] <- for bracket balancing */)
					? (*fmt)+1 : *fmt, /* [ <- for bracket balancing */
					']');
	
	if (IsError(*fmt == NULL))	/* misformed scanset */
	{	*fmt = scanSet;
		*buf = "";
		return 0;
	}
	
	scanSetSize = *fmt - scanSet;
	(*fmt)++;	/* move beyond closing bracket */
	
	if (noStore == 0)
	{	dest = va_arg(*args, char*);
		if (!ValidWriteLocation(dest))
			dest = NULL;
	}
	
	if (width == 0)
		width = INT_MAX;
		
	while (((ch=**buf) != '\0') && (width-- > 0))
	{
		match = memchr(scanSet, ch, scanSetSize);
		if (((match == NULL) && wantMatch) || ((match != NULL) && !wantMatch))
			break;
		
		(*buf)++;
		(*nchar)++;
		if (dest != NULL)
			*dest++ = ch;
	}
	if (dest != NULL)
		*dest++ = 0;
	
	return noStore ? 0 : 1;
}

static int
_sscanf_string(const char** buf, int* nchar, const char code,
			va_list* args, int noStore, int width)
{
	char* dest = NULL;
	char ch;

	if ( IsError(code != 's') )
		return 0;

	if (noStore == 0)
	{	dest = va_arg(*args, char*);
		if (!ValidWriteLocation(dest))
			dest = NULL;
	}

	if (width == 0)
		width = INT_MAX;
	
	while (((ch=**buf) != '\0') && (width-- > 0))
	{
		(*buf)++;
		(*nchar)++;
		if (isspace(ch))
			break;
		if (dest != NULL)
			*dest++ = ch;
	}
	if (dest != NULL)
		*dest++ = 0;

	return noStore ? 0 : 1;
}



static int _sscanf(const char* buf, const char* fmt, va_list* args)
{
	int argsStored = 0; 	/* args stored */
	int nchar = 0;			/* chars used from buf */
	int noStore;
	int width;
	int qual;
	char code;
	
	if ((!ValidReadLocation(buf)) || (!ValidReadLocation(fmt)))
		return EOF;
	
	while ((fmt != NULL) && (*fmt != '\0') && (buf != NULL) && (*buf != '\0'))
	{
		if (isspace(*fmt))
		{	/* skipping white space */
			while (isspace(*fmt))
				fmt++;
			while (isspace(*buf))
			{	buf++;
				nchar++;
			}
		}
		else if (*fmt != '%')
		{	/* matching non-whitespace */
			if (*fmt != *buf)
			{	return argsStored;
			}
			fmt++;
			buf++;
			nchar++;
		}
		else
		{	
			fmt++;	/* skip over %...we're going to parse conversions */
			code = *fmt++;
			
			noStore = 0;
			if (code == '*')
			{	noStore = code;
				code = *fmt++;	/* oops, NEXT is code */
			}
			
			width = 0;
			while (isdigit(code))
			{	width = (width*10) + code - '0';
				code = *fmt++;	/* oops, NEXT is code */
			}
			
			qual = 0;
			if ((code == 'h') || (code == 'l') || (code == 'L'))
			{	qual = code;
				code = *fmt++;	/* oops, NEXT is code */
			}
			
			if (!((code == 'c') || (code == 'n') || (code == '[' /* ] <- for bracket balancing */)))
			{	while (isspace(*buf))
				{	buf++;
					nchar++;
				}
			}
			
			switch (code)
			{
				case 'c':
					argsStored += _sscanf_char(&buf, &nchar, code, args, noStore, width);
					break;
				
				case 'd':
				case 'i':
				case 'o':
				case 'u':
				case 'x':
				case 'X':
				case 'p':
					argsStored += _sscanf_int(&buf, &nchar, code, args, noStore, width, qual);
					break;
				
				case 'g':
				case 'G':
					argsStored += _sscanf_float(&buf, &nchar, code, args, noStore, width, qual);
					break;
				
				case 'n':
					if (qual == 'h')
					{
						short* sptr = va_arg(*args, short*);
						if (ValidWriteLocation(sptr))
							*sptr = nchar;
					}
					else if (qual != 'l')
					{
						int* iptr = va_arg(*args, int*);
						if (ValidWriteLocation(iptr))
							*iptr = (int)nchar;
					}
					else
					{
						long* lptr = va_arg(*args, long*);
						if (ValidWriteLocation(lptr))
							*lptr = (long)nchar;
					}
					break;
				
				case 's':
					argsStored += _sscanf_string(&buf, &nchar, code, args, noStore, width);
					break;
					
				case '%':
					if (*buf != '%')
					{	return argsStored;
					}
					buf++;
					nchar++;
					break;
					
				case '[': /* ] <- for bracket balancing */
					argsStored += _sscanf_scanset(&buf, &nchar, &fmt, args, noStore, width);
					break;
					
				default:
					if ( IsWarning(true) )
						Message(("unhandled scanf code '%c'",code));	/* we were unable to understand this */
					break;
			}
		}
	}
	
	return (buf == nil) ? EOF : argsStored;	/* (buf==nil) ==> ran out of space! */
}




/* ---------------------------------------------------------------------------
	sscanf()
--------------------------------------------------------------------------- */
int sscanf(const char* buf, const char* fmt, ...)
{
	int result;
	va_list args;
	
	va_start(args, fmt);
	result = _sscanf(buf, fmt, &args);
	va_end(args);
	
	return (result);
}




/* ---------------------------------------------------------------------------
	Old sscanf (before Mick lost his mind)
--------------------------------------------------------------------------- */

/****************************************************************
Copyright (C) AT&T 1992, 1993
All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appear in all
copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the name of AT&T or any of its entities
not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

AT&T DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
IN NO EVENT SHALL AT&T OR ANY OF ITS ENTITIES BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.
****************************************************************/

/* This is for compilers (like MetaWare High C) whose sscanf is broken.
 * It implements only the relevant subset of sscanf.
 * With sensible compilers, you can omit sscanf.o
 * if you add -DSscanf=sscanf to CFLAGS (in the makefile).
 */
 
/*
static void bad(const char *fmt)
{
	printf("bad fmt in sscanf, starting with \"%s\"\n", fmt);
}

int sscanf (const char *s, const char *fmt, ...)
{
	const char *s0;
	va_list ap;
	long L, *Lp;
	int i, *ip, rc = 0;
	char *sp;

	va_start(ap, fmt);

	for(;;) {
		for(;;) {
			switch(i = *(unsigned char *)fmt++) {
				case 0:
					goto done;
				case '%':
					break;
				default:
					if (i <= ' ') {
						while(*s <= ' ')
							if (!*s++)
								return rc;
						}
					else if (*s++ != i)
						return rc;
					continue;
				}
			break;
			}
		switch(*fmt++) {
			case 'l':
				if (*fmt != 'd')
					bad(fmt);
				fmt++;
				Lp = va_arg(ap, long*);
				L = strtol(s0 = s, (char**)&s, 10);
				if (s > s0) {
					rc++;
					*Lp = L;
					continue;
					}
				return rc;
			case 'x':
				ip = va_arg(ap, int*);
				L = strtol(s0 = s, (char**)&s, 16);
				if (s > s0) {
					rc++;
					*ip = (int)L;
					continue;
					}
				return rc;
			case 'd':
				ip = va_arg(ap, int*);
				L = strtol(s0 = s, (char**)&s, 10);
				if (s > s0) {
					rc++;
					*ip = (int)L;
					continue;
					}
				return rc;
			case 's':
				sp = va_arg(ap, char*);
				while((*s!=0) && (*s!=0x0d))
				{
					*sp = *s;
					sp++;
					s++;
				}
				*sp = 0;
				rc++;
				return rc;
			default:
				bad(fmt);
			}
		}
 done:
	return rc;
	}
*/