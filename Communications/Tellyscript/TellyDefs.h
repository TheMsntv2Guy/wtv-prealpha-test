/* ===========================================================================
	TellyDefs.h

	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
=========================================================================== */

#ifndef __TELLYDEFS_H__
#define __TELLYDEFS_H__

#ifdef FOR_MAC
#ifndef __SETJMP__
#include <setjmp.h>
#endif
#ifndef	_UNIX
#include <unix.h>
#endif
#ifndef	_FCNTL
#include <fcntl.h>
#endif
#endif /* FOR_MAC */

#define EOLN '\015'
#define EOLNS "\015"
#define T_EOF 0xFF
#define kSCRIPT_VERSION 1

typedef uchar Byte;

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include TellyDefs.h multiple times"
	#endif
#endif /* __TELLYDEFS_H__ */