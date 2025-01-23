/* ===========================================================================
	defs.h

	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
=========================================================================== */

#ifndef __DEFS_H__
#define __DEFS_H__

#ifndef __WTVTYPES_H__
#include "WTVTypes.h"
#endif

/*
These are already defined in WTVTypes.h...

typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;
*/
/* typedef unsigned char byte; */

/*
These are already defined in WTVTypes.h...

#define true 1
#define false 0

#define NULL 0
*/
#define NULL_IFC 0

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include defs.h multiple times"
	#endif
#endif /* __DEFS_H__ */