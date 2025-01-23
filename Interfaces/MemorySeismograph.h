// ===========================================================================
//	MemorySeismograph.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __MEMORYSEISMOGRAPH_H__
#define __MEMORYSEISMOGRAPH_H__

#ifdef DEBUG_MEMORYSEISMOGRAPH

#ifndef __STDWINDOW_H__
#include "StdWindow.h"
#endif	/* __STDWINDOW_H__ */

#ifndef MEMORY_TRACKING
#error "DEBUG_MEMORYSEISMOGRAPH requires you to #define MEMORY_TRACKING"
#endif /* MEMORY_TRACKING */

// -----------------------------------------------------------------------------------

class SeismographReading
{
	public:
		SeismographReading(void);
		~SeismographReading(void);		// don't want the expense of a virtual function table

		void			Init(void);
		void			Reset(void);
		void			TaggedAlloc(MemoryTag* tag);
		void			TaggedFree(MemoryTag* tag);
		void			TaggedNew(MemoryTag* tag);
		void			TaggedDelete(MemoryTag* tag);
		short			DrawReadingSummary(Rect* r);
		void			DrawReading(short hCoord, short loVCoord, short hiVCoord, long loValue, long hiValue);
		void			AssumeActiveFrom(SeismographReading* oldReading);

	protected:
		short			ValueToCoord(long value, short topCoord, short bottomCoord, long loValue, long hiValue);
	
	public:
		ulong			startTime;
		
		const char*		currURL;
		long			currSize;
		long			newSize;
		long			delSize;

		long			cacheUsed;
		long			cacheEntries;

		enum			{kNumOldURLs = 32};
		const char*		oldURL[kNumOldURLs];
		long			oldURLAlloc[kNumOldURLs];
		char			oldURLColorIndex[kNumOldURLs];
};

class MemorySeismographWindow : public StdWindow
{
public:
							MemorySeismographWindow();
	virtual					~MemorySeismographWindow();
	
	virtual void			DoAdjustMenus(ushort modifiers);
	virtual Boolean			DoMenuChoice(long menuChoice, ushort modifiers);

	void					TaggedAlloc(MemoryTag* tag);
	void					TaggedFree(MemoryTag* tag);
	void					TaggedNew(MemoryTag* tag);
	void					TaggedDelete(MemoryTag* tag);
	

	virtual void			Click(struct MacPoint *where, ushort modifiers);
	virtual	void			DrawHeader(Rect* r);
	virtual	void			DrawBody(Rect *r, short hScroll, short vScroll, Boolean scrolling);
	virtual	void			Idle(void);
	virtual void			Close(void);
	
protected:
	void					CheckForAdvance(void);
	void					AffectReading(long deltaCurrSize,
										  long deltaNewSize,
										  long deltaCurrCPlusSize,
										  long deltaNewCPlusSize);
	void					GetGraphRect(Rect* graphRect);
	void					DrawGraph(Rect* graphRect);
	void					ChooseDisplayReading(struct MacPoint *where, ushort modifiers);
	void					RescaleGraph(struct MacPoint *where, ushort modifiers);
	long					CoordToValue(short coord, short topCoord, short bottomCoord, long loValue, long hiValue);

protected:
	enum 					{ kSeismographLogSize = 500 };
	SeismographReading		fReading[kSeismographLogSize];
	Rect					fGraphRect;
	short					fUpdateReadings;	// draw must update these...
	short					fActiveReading;
	short					fDisplayReading;
	short					fAdjustHeaderHeight;	// adjust header when more URLs come in
	short					fTimeInterval;
	long					fDispMin;
	long					fDispMax;
};

extern MemorySeismographWindow* gMemorySeismographWindow;

#endif /* #ifdef DEBUG_MEMORYSEISMOGRAPH */

// -----------------------------------------------------------------------------------

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include MemorySeismograph.h multiple times"
	#endif
#endif /* __MEMORYSEISMOGRAPH_H__ */
