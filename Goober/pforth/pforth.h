/* @(#) pforth.h 95/11/09 1.8 */
#ifndef _pforth_h
#define _pforth_h

/***************************************************************
** Include file for pForth, a portable Forth based on 'C'
**
** This file is included in any application that uses pForth as a tool.
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
**
**	941027 rdg Added PF_HOST_3DO
**
***************************************************************/

/* For Mac Metrowerks */

/*
#ifdef __MWERKS__
	#ifndef PF_HOST_MACINTOSH
		#define PF_HOST_MACINTOSH
	#endif
#endif

#if defined(PF_HOST_MACINTOSH)
	#include <StdLib.h>
	#include <StdIO.h>
	#include <String.h>
#elif defined(PF_HOST_3DO)
	#include "types.h"
	#include "stdlib.h"
	#include "stdio.h"
	#include "string.h"
#else
	#include <ctype.h>
	#include <malloc.h>
	#include <memory.h>
	#include <string.h>
	#include <stdlib.h>
	#include <stdio.h>
#endif
*/


/* Define stubs for data types so we can pass pointers but not touch inside. */
typedef struct cfTaskData   cfTaskData;
typedef struct cfDictionary cfDictionary;

typedef long ExecToken;              /* Execution Token */

#ifdef __cplusplus
extern "C" {
#endif

/* Turn off messages. */
void  pfSetQuiet( int IfQuiet );

/* Send a message using low level I/O of pForth */
void  pfMessage( const char *CString );

/* Create a task used to maintain context of execution. */
cfTaskData *pfCreateTask( int UserStackDepth, int ReturnStackDepth );

/* Establish this task as the current task. */
void  pfSetCurrentTask( cfTaskData *cftd );

/* Delete task created by pfCreateTask */
void  pfDeleteTask( cfTaskData *cftd );

/* Build a dictionary with all the basic kernel words. */
cfDictionary *pfBuildDictionary( int HeaderSize, int CodeSize );

/* Create an empty dictionary. */
cfDictionary *pfCreateDictionary( int HeaderSize, int CodeSize );

/* Load dictionary from a file. */
cfDictionary *pfLoadDictionary( char *FileName, ExecToken *EntryPointPtr );

/* Delete dictionary data. */
void  pfDeleteDictionary( cfDictionary *dic );

/* Execute the pForth interpreter. */
int   pfRunForth( void );

/* Execute a single execution in the current task. */
void pfExecuteToken( ExecToken XT );

/* Include the given pForth source code file. */
int   pfIncludeFile( char *FileName );

#ifdef __cplusplus
}   
#endif

#endif
