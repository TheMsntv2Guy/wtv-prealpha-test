// ===========================================================================
//	PerfDumpToKScope.h
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __PERFDUMPTOKSCOPE_H__
#define __PERFDUMPTOKSCOPE_H__


#define kPerfDumpToKScopeSignature (   (((unsigned long)'P')<<24) \
									 + (((unsigned long)'D')<<16) \
									 + (((unsigned long)'T')<< 8) \
									 +  ((unsigned long)'K') \
								   )

#define kPerfDumpToKScopeType	   (   (((unsigned long)'T')<<24) \
									 + (((unsigned long)'E')<<16) \
									 + (((unsigned long)'X')<< 8) \
									 +  ((unsigned long)'T') \
								   )

void PerfDumpToKScope(const FSSpecPtr myFSSPtr);


#endif /* __PERFDUMPTOKSCOPE_H__ */