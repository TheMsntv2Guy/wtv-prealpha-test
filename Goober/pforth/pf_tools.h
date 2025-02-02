/* @(#) pf_tools.h 95/11/09 1.5 */
#ifndef _pforth_tools_h
#define _pforth_tools_h

/***************************************************************
** Include file for PForth tools
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
***************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

char  *ForthStringToC( char *dst, char *FString );
char  *CStringToForth( char *dst, char *CString );
int32 ffCompareText( char *s1, char *s2, int32 len );
int32 ffCompareTextCaseN( char *s1, char *s2, int32 len );

void  DumpMemory( void *addr, int32 cnt);
cell  HexDigitToNumber( char c );
char *ConvertNumberToText( int32 Num, int32 Base, int32 IfSigned, int32 MinChars );
void  TypeName( char *Name );

#ifdef __cplusplus
}   
#endif

#endif /* _pforth_tools_h */
