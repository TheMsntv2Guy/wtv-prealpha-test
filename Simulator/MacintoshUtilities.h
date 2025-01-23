// ===========================================================================
//	MacintoshUtilities.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __MACINTOSHUTILITIES_H__
#define __MACINTOSHUTILITIES_H__

// ---------------------------------------------------------------------------

/*
void AlertUser(short error, short messageID);
*/
void MacAlert(char *msg);
void EmergencyExit(short message);

// ---------------------------------------------------------------------------

Ptr NewSimulatorMemory(ulong amount, Boolean shouldClear = false);
void DisposeSimulatorMemory(Ptr ptr);

// ---------------------------------------------------------------------------

const char* GenerateDumpFilename(const char* filenamePrefix);

// ---------------------------------------------------------------------------

void SetColor(short r, short g, short b);
void SetBackColor(short r, short g, short b);
void SetGray(short gray);
void SetBackGray(short gray);

// ---------------------------------------------------------------------------

Boolean OptionKeyDown(void);

// ---------------------------------------------------------------------------

void DelayTicks(long forTicks);

// ---------------------------------------------------------------------------

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include MacintoshUtilities.h multiple times"
	#endif
#endif /* __MACINTOSHUTILITIES_H__ */
