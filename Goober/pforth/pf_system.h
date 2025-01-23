/* @(#) pf_system.h 95/11/09 1.6 */
#ifndef _pf_system_h
#define _pf_system_h

/***************************************************************
** System Dependant Includes for PForth based on 'C'
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

/*
** This version uses ANSI standard I/O calls.
** Some embedded systems may not have support for these
** at early stages of development.
*/

#define sdOutputChar    putchar
#define sdInputChar     getchar
#define sdFlushFile    
//#define sdFlushFile     fflush
#define sdOpenFile      fopen
#define sdReadFile      fread
#define sdWriteFile     fwrite
#define sdSeekFile      fseek
#define sdTellFile      ftell
#define sdCloseFile     fclose

/*
** printf() is only used for debugging purposes.
** It is not required for normal operation.
*/
#define PRT(x) { printf x; sdFlushFile((FileStream *)stdout); }

#endif /* _pf_system_h */
