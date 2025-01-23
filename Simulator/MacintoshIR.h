/* ===========================================================================
	MacintoshIR.h
	
	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
=========================================================================== */

#ifndef __MACINTOSHIR_H__
#define __MACINTOSHIR_H__

void	MacintoshIRFinalize();
void	MacintoshIRInitialize();
Boolean	MacintoshIRInitialized();
void	DumpMacintoshIRBuffer();

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include MacintoshIR.h multiple times"
	#endif
#endif /* __MACINTOSHIR_H__ */
