// ===========================================================================
//	MemoryCheckpointing.h
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __MEMORYCHECKPOINTING_H__
#define __MEMORYCHECKPOINTING_H__

#if defined(DEBUG) && defined(MEMORY_TRACKING)
	void HandleNewMemoryCheckpoint(void);
	void ReportMemoryDifference(void);
	Boolean MemoryCheckpointed(void);
#endif

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include MemoryCheckpointing.h multiple times"
	#endif
#endif /* __MEMORYCHECKPOINTING_H__ */
