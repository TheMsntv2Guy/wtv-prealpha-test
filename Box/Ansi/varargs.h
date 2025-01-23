#error This file is obsolete

/*
	File:		varargs.h

	Contains:	xxx put contents here xxx

	Written by:	joe britt

	Copyright:	© 1995 by WebTV, Inc., all rights reserved.

	Change History (most recent first):

		 <1>	 9/11/95	JOE		first checked in

	To Do:
*/

#ifndef _VARARGS_H
#define _VARARGS_H

typedef char *va_list;

#define __va_rounded_size(__TYPE)  \
  		(((sizeof (__TYPE) + sizeof (int) - 1) / sizeof (int)) * sizeof (int))

#define va_start(__AP, __LASTARG) 						\
 		(__AP = ((char *) &(__LASTARG) + __va_rounded_size (__LASTARG)))

#define va_end(__AP)

/* For big-endian machines.  */
#define va_arg(__AP, __type)	((__type *)(__AP = (char *)(sizeof(__type) > 4 ? (int)__AP + 2*8 - 1 & -8 :		\
																				 (int)__AP + 2*4 - 1 & -4)))[-1]


#endif /*_VARARGS_H_*/