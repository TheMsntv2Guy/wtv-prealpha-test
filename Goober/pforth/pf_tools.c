/* @(#) pf_tools.c 95/11/09 1.6 */
/***************************************************************
** Utilities for PForth based on 'C'
**
** Author: Phil Burk
** Copyright 1994 3DO, Phil Burk, Larry Polansky, Devid Rosenboom
**
** The pForth software code is dedicated to the public domain,
** and any third party may reproduce, distribute and modify
** the pForth software code or any derivative works thereof
** without any compensation or license.  The pForth software
** code is provided on an "as is" basis without any warranty
** of any kind, including, without limitation, the implied
** warranties of merchantability and fitness for a particular
** purpose and their equivalents under the laws of any jurisdiction.
**
****************************************************************
***************************************************************/

#include "pf_includes.h"

/**************************************************************
** Copy a Forth String to a 'C' string.
*/

char *ForthStringToC( char *dst, char *FString )
{
	int32 Len;

	Len = (int) *FString;
	memcpy( (unsigned char *)dst, (const unsigned char *)FString+1, Len );
	dst[Len] = '\0';

	return dst;
}

/**************************************************************
** Copy a NUL terminated string to a Forth counted string.
*/
char *CStringToForth( char *dst, char *CString )
{
	char *s;
	int32 i;

	s = dst+1;
	for( i=0; *CString; i++ )
	{
		*s++ = *CString++;
	}
	*dst = (char ) i;
	return dst;
}

/**************************************************************
** Compare two test strings, case sensitive.
** Return TRUE if they match.
*/
int32 ffCompareText( char *s1, char *s2, int32 len )
{
	int32 i, Result;
	
	Result = TRUE;
	for( i=0; i<len; i++ )
	{
DBUGX(("ffCompareText: *s1 = 0x%x, *s2 = 0x%x\n", *s1, *s2 ));
		if( *s1++ != *s2++ )
		{
			Result = FALSE;
			break;
		}
	}
DBUGX(("ffCompareText: return 0x%x\n", Result ));
	return Result;
}

/**************************************************************
** Compare two test strings, case INsensitive.
** Return TRUE if they match.
*/
int32 ffCompareTextCaseN( char *s1, char *s2, int32 len )
{
	int32 i, Result;
	char  c1,c2;
	
	Result = TRUE;
	for( i=0; i<len; i++ )
	{
		c1 = tolower(*s1);
		s1++;
		c2 = tolower(*s2);
		s2++;
DBUGX(("ffCompareText: c1 = 0x%x, c2 = 0x%x\n", c1, c2 ));
		if( c1 != c2 )
		{
			Result = FALSE;
			break;
		}
	}
DBUGX(("ffCompareText: return 0x%x\n", Result ));
	return Result;
}

/***************************************************************
** Convert number to text.
*/
#define CNTT_PAD_SIZE (32)
static char cnttPad[CNTT_PAD_SIZE];

char *ConvertNumberToText( int32 Num, int32 Base, int32 IfSigned, int32 MinChars )
{
	int32 IfNegative = 0;
	char *p,c;
	int32 NewNum, Rem;
	int32 i = 0;
	
	if( IfSigned )
	{
/* Convert to positive and keep sign. */
		if( Num < 0 )
		{
			IfNegative = TRUE;
			Num = -Num;
		}
	}
	
/* Point past end of Pad */
	p = cnttPad + CNTT_PAD_SIZE;
	*(--p) = (char) 0; /* NUL terminate */
	
	while( (i++<MinChars) || (Num != 0) )
	{
		NewNum = Num / Base;
		Rem = Num - (NewNum * Base);
		c = ( Rem < 10 ) ? (Rem + '0') : (Rem - 10 + 'A');
		*(--p) = c;
		Num = NewNum;
	}
	
	if( IfSigned )
	{
		if( IfNegative ) *(--p) = '-';
	}
	return p;
}

/***************************************************************
** Diagnostic routine that prints memory in table format.
*/
void DumpMemory( void *addr, int32 cnt)
{
	int32 ln, cn, nlines;
	unsigned char *ptr, *cptr, c;

	nlines = (cnt + 15) / 16;

	ptr = (unsigned char *) addr;

	EMIT_CR;
	
	for (ln=0; ln<nlines; ln++)
	{
		MSG( ConvertNumberToText( (int32) ptr, 16, FALSE, 8 ) );
		MSG(": ");
		cptr = ptr;
		for (cn=0; cn<16; cn++)
		{
			MSG( ConvertNumberToText( (int32) *cptr++, 16, FALSE, 2 ) );
			EMIT(' ');
		}
		EMIT(' ');
		for (cn=0; cn<16; cn++)
		{
			c = *ptr++;
			if ((c < ' ') || (c > '}')) c = '.';
			EMIT(c);
		}
		EMIT_CR;
	}
}

/* Convert a single digit to the corresponding hex number. */
cell HexDigitToNumber( char c )
{	
	if( (c >= '0') && (c <= '9') )
	{
		return( c - '0' );
	}
	else if ( (c >= 'A') && (c <= 'F') )
	{
		return( c - 'A' + 0x0A );
	}
	else
	{
		return -1;
	}
}

/* Print name, mask off any dictionary bits. */
void TypeName( char *Name )
{
	char *FirstChar;
	int32 Len;
	
	FirstChar = Name+1;
	Len = *Name & 0x1F;
	
	ioType( FirstChar, Len, (FileStream *)stdout );
}
