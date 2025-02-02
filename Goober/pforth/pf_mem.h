/* @(#) pf_mem.h 95/11/09 1.4 */
#ifndef _pforth_mem_h
#define _pforth_mem_h

/***************************************************************
** Include file for PForth Fake Memory Allocator
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

#ifdef PF_NO_MALLOC

	char *pfAllocMem( int32 NumBytes );
	void pfFreeMem( void *Mem );			// joe - changed char * to void *

#else

	#define pfAllocMem malloc
	#define pfFreeMem free
	
#endif /* PF_NO_MALLOC */


#ifdef __cplusplus
}   
#endif

#endif /* _pforth_mem_h */
