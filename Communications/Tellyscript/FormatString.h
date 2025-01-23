/* ===========================================================================
	FormatString.h

	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
=========================================================================== */

#ifndef __FORMATSTRING_H__
#define __FORMATSTRING_H__

Byte *FormatString(Byte *, Byte *, ...);
Byte *SFormatString(Byte *, Byte *, long *);

#else
#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
#error "Attempted to #include FormatString.h multiple times"
#endif
#endif /* __FORMATSTRING_H__ */
