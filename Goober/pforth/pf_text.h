/* @(#) pf_text.h 95/11/09 1.6 */
#ifndef _pforth_text_h
#define _pforth_text_h

/***************************************************************
** Include file for PForth Text
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

#define PF_ERR_INDEX_MASK (0xFFFF)
#define PF_ERR_BASE            (0x80000000)
#define PF_ERR_NO_MEM          (PF_ERR_BASE |  0)
#define PF_ERR_BAD_ADDR        (PF_ERR_BASE |  1)
#define PF_ERR_TOO_BIG         (PF_ERR_BASE |  2)
#define PF_ERR_NUM_PARAMS      (PF_ERR_BASE |  3)
#define PF_ERR_OPEN_FILE       (PF_ERR_BASE |  4)
#define PF_ERR_WRONG_FILE      (PF_ERR_BASE |  5)
#define PF_ERR_BAD_FILE        (PF_ERR_BASE |  6)
#define PF_ERR_READ_FILE       (PF_ERR_BASE |  7)
#define PF_ERR_WRITE_FILE      (PF_ERR_BASE |  8)
#define PF_ERR_CORRUPT_DIC     (PF_ERR_BASE |  9)
#define PF_ERR_NOT_SUPPORTED   (PF_ERR_BASE | 10)
#define PF_ERR_VERSION_FUTURE  (PF_ERR_BASE | 11)
#define PF_ERR_VERSION_PAST    (PF_ERR_BASE | 12)
#define PF_ERR_COLON_STACK     (PF_ERR_BASE | 13)
#define PF_ERR_HEADER_ROOM     (PF_ERR_BASE | 14)
#define PF_ERR_CODE_ROOM       (PF_ERR_BASE | 15)
#define PF_ERR_NO_SHELL        (PF_ERR_BASE | 16)
#define PF_ERR_NO_NAMES        (PF_ERR_BASE | 17)

#ifdef __cplusplus
extern "C" {
#endif

void pfReportError( char *FunctionName, Err ErrCode );

#ifdef __cplusplus
}   
#endif

#endif /* _pforth_text_h */
