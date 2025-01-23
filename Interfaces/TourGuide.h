// ===========================================================================
//	TourGuide.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __TOURGUIDE_H__
#define __TOURGUIDE_H__

#ifndef __SIMULATORSTATISTICS_H__
#include "SimulatorStatistics.h"		/* TextList */
#endif

#ifndef __URLPARSER_H__
#include "URLParser.h"					/* URLParser */
#endif

#ifndef __UTILITIES_H__
#include "Utilities.h"					/* kOneSecond */
#endif


class Document;

// ===========================================================================

class Tour : public TextList
{
public:
	Boolean Load(const Document* document);

protected:
	URLParser			fParser;
};

class TourGuide
{
public:
	TourGuide(void);
	~TourGuide(void);
	
	Boolean Load(void);
	void Start(void);
	void Stop(void);
	void Print(FILE* fp = nil);
	void AddMessage(const char*);
	void Idle(void);
	Boolean GetIsActive() const		{ return fActive; };
	ulong GetPageCount() const;
	
	Boolean IsActive(void) const;
	
protected:
	void Reset(void);
	void GetFirst(void);
	void GetNext(void);
	void OpenURL(void);

protected:	
	enum { kTimeOut = kOneSecond * 90 };		// after 1.5 minutes, stop waiting...move on
	enum { kLookTime = kOneSecond * 2 };		// look at finished page for n seconds
	enum { kAlertLookTime = kOneSecond * 2 };	// look at alert window for n seconds

	Tour				fTour;			// where we'll be walking
	
	FILE*				fDumpFile;		// output a log to here
	Resource			fRootResource;	// where we started
	char*				fDumpFilename;	// our dumpfile
	
	ulong				fLookTime;		// wait this long, just staring at page
	ulong				fTimeOut;		// wait this long before giving up and going on

	ulong				fPagesVisited;	// how many did we visit?
	ulong				fPagesSkipped;	// how many did we have to skip?
	ulong				fPagesTotal;	// how many did we have to skip?

#if defined __MWERKS__ && !defined FOR_MPW && __profile__ == 1
	Boolean				fIsProfiling;	// is profiling turned on?
#endif // defined __MWERKS__ && !defined FOR_MPW && __profile__ == 1
	Boolean				fTourLoaded;	// if TRUE, there's a tour loaded in
	Boolean				fActive;		// if TRUE, we're walking...
	Boolean				fDocumentLoaded;// if TRUE, new URL has been loaded
};

extern TourGuide gTourGuide;

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include TourGuide.h multiple times"
	#endif
#endif /* __TOURGUIDE_H__ */
