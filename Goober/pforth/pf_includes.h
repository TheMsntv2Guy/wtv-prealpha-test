/* @(#) pf_includes.h 95/11/09 1.3 */
#ifndef _pforth_includes_h
#define _pforth_includes_h

/***************************************************************
** Include all files nneded for PForth
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
** 940521 PLB Creation.
**
***************************************************************/

#include "boxansi.h"

#include "pf_internal.h"

#include "pf_system.h"
#include "pf_text.h"
#include "pf_io.h"
#include "pf_compile.h"
#include "pf_tools.h"
#include "pf_words.h"
#include "pf_save.h"
#include "pf_mem.h"
#include "pf_c_glue.h"
#include "pf_core.h"

//#define PF_NO_INIT			1

//#define	PF_NO_SHELL			1

#define	PF_MEM_POOL_SIZE	0x20000

#define PF_NO_MALLOC		1
#define	PF_NO_FILEIO		1

#ifdef PF_SUPPORT_FP
#undef PF_SUPPORT_FP
#endif

#endif /* _pforth_includes_h */

