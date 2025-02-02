/* limits.h standard header -- 8-bit version */
#ifndef _LIMITS
#define _LIMITS

#if 0
#ifndef _YVALS
#include <yvals.h>
#endif

#if _BITS_BYTE != 8
 #error <limits.h> assumes 8 bits per byte
#endif
#endif

#if __MWERKS__
#pragma options align=mac68k
#pragma direct_destruction off
#endif

#define _C2   1

		/* char properties */
#define CHAR_BIT	8
#if _CSIGN
#define CHAR_MAX	127
#define CHAR_MIN	(-127-_C2)
#else
#define CHAR_MAX	255
#define CHAR_MIN	0
#endif
		/* int properties */
#if _ILONG
#define INT_MAX		2147483647
#define INT_MIN		(-2147483647-_C2)
#define UINT_MAX	4294967295U
#else
#define INT_MAX		32767
#define INT_MIN		(-32767-_C2)
#define UINT_MAX	65535U
#endif
		/* long properties */
#define LONG_MAX	2147483647
#define LONG_MIN	(-2147483647-_C2)
		/* multibyte properties */
#define MB_LEN_MAX	_MBMAX
		/* signed char properties */
#define SCHAR_MAX	127
#define SCHAR_MIN	(-127-_C2)
		/* short properties */
#define SHRT_MAX	32767
#define SHRT_MIN	(-32767-_C2)
		/* unsigned properties */
#define UCHAR_MAX	255U
#define ULONG_MAX	4294967295U
#define USHRT_MAX	65535U

#if __MWERKS__
#pragma options align=reset
#pragma direct_destruction reset
#endif

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include limits.h multiple times"
	#endif
#endif

/*
 * Copyright (c) 1994 by P.J. Plauger.  ALL RIGHTS RESERVED. 
 * Consult your license regarding permissions and restrictions.
 */

/* Change log:
 *94June04 PlumHall baseline
 *94Oct07 Inserted MW changes.
 */
