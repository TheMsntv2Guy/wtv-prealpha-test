// ===========================================================================
//	SimulatorStatistics.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#error еее Obsolete

#if 0

#ifndef __SIMULATORSTATISTICS_H__
#define __SIMULATORSTATISTICS_H__

#ifdef SIMULATOR

#include <stdio.h>

#ifndef __WTVTYPES_H__
#include "WTVTypes.h"
#endif

#ifndef __INPUT_H__
#include "Input.h"
#endif

// ----------------------------------------------------------------------------
//	function prototypes
// ----------------------------------------------------------------------------
extern void PrintSimulatorStatistics(void);

// ----------------------------------------------------------------------------
//  LapTimer class definition
// ----------------------------------------------------------------------------

class LapTimer
{
	enum { kNumLapsRecorded = 50 };
	enum { kLapTimerBigLapCutoff = 10 };	// anything over 10 ticks is part of
										// a different avg. lap time
public:
	LapTimer();
	~LapTimer();

	void Start();
	void Stop();
	void Reset();
	ulong Lap();
	void Pause();
	void Resume();
	void Print(FILE* fp) const;
	void SetTitle(const char* title);
	void SetBigLapCutoff(ulong bigLapCutoff);
	
	ulong GetTotalTime(void) const;
	ulong GetPauseTime(void) const;
	ulong GetBigLapCutoff(void) const;
	ulong GetActiveTime(void) const;
	ulong GetSmallLapsTime(void) const;
	ulong GetBigLapsTime(void) const;
	ulong GetNumLaps(void) const;
	ulong GetNumBigLaps(void) const;
	ulong GetNumSmallLaps(void) const;
	Boolean IsActive(void) const;		// has it been started and not stopped?
	Boolean IsPaused(void) const;		// can be active and paused
	
protected:
	void RecordLapTime(ulong lapTime);

protected:
	const char* fTitle;
	ulong	fWorstTime[kNumLapsRecorded];
	ulong	fStartTime;
	ulong	fLastLapStartTime;
	ulong	fPauseTime;
	ulong	fLastLapPauseTime;
	ulong	fCurrPauseTime;
	ulong	fMinLapTime;
	ulong	fMaxLapTime;
	ulong	fBigLapCutoff;
	ulong   fBigLapTime;
	ulong	fNumLaps;
	ulong	fNumBigLaps;
	Boolean fPaused;
	Boolean fActive;
};

class IdleLapTimer	:	public LapTimer
{
	public:
		enum { kIdleLapTimerBigLapCutoff = 30 };	// one-half second
		IdleLapTimer();
};
class PageLapTimer	:	public LapTimer
{
	public:
		enum { kPageLapTimerBigLapCutoff = 60 };	// one second
		PageLapTimer();
};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//  InputCounter class definition
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
typedef enum {
	kIncomingPress = 0,
	kNonSystemPress = 1,
	kUnhandledPress = 2,
	kNumInputCounterStages = 3
} InputCounterStage;

enum
{
	kUpKeyIndex = 0,
	kLeftKeyIndex,
	kRightKeyIndex,
	kDownKeyIndex,
	kOptionsKeyIndex,
	kExecuteKeyIndex,
	kPowerKeyIndex,
	kForwardKeyIndex,
	kBackKeyIndex,
	kScrollUpKeyIndex,
	kScrollDownKeyIndex,
	kRecentKeyIndex,
	kHomeKeyIndex,
	kKeyboardBaseIndex,
	kNumKeyboardEntries = 128,
	kFirstKeyIndex = kUpKeyIndex,
	kNumKeyIndices = kKeyboardBaseIndex + kNumKeyboardEntries
};

class InputCounter
{	
public:
	InputCounter();
	Boolean Initialize(void);
	~InputCounter();
	
	void Print(FILE* fp) const;
	void LogInput(Input* input, InputCounterStage stage);
	
protected:
	ulong fPresses[kNumKeyIndices][kNumInputCounterStages];
};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//  ErrorLog class definition
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


class TextList
{
protected:
	enum { kTextLogCacheIncr = 2*1024 };

public:
	TextList();
	~TextList();
	
	Boolean Add(const char* str);
	void Clear(void);
	Boolean Find(const char* str);
	const char* GetFirst(void);
	const char* GetNext(void);
	const char* GetCurrent(void);
	ulong		GetCurrentIndex(void) const;
	//const char* GetIndex(ulong index);
	Boolean AtEnd(void);

	void Print(FILE* fp);

protected:
	Boolean Resize(const size_t newLogSize);

protected:
	size_t fCurrGet;
	size_t fCurrPut;
	size_t fLogSize;
	char*  fLog;
};

class KeyValueTextList
{
public:
	KeyValueTextList();
	~KeyValueTextList();
	
	void SetTitle(const char* title);
	void SetKeyLabel(const char* keyLabel);
	void SetValueLabel(const char* valueLabel);
	const char* GetValue(const char* key);

	void Add(const char* key, const char* value);
	void Clear(void);
	
	void Print(FILE* fp);
	
protected:
	TextList	fKeyList;
	TextList	fValueList;
	const char* fKeyLabel;
	const char* fValueLabel;
	const char* fTitle;
};


//#define LOG_ALL_TAGS_AND_ATTRIBUTES
//#define LOG_BAD_TAGS_AND_ATTRIBUTES

#if (!defined(LOG_ALL_TAGS_AND_ATTRIBUTES)) && (!defined(LOG_BAD_TAGS_AND_ATTRIBUTES))
class TagLog		  : public KeyValueTextList	{	public: TagLog();			};
class KnownTagLog     : public TagLog			{	public:	KnownTagLog();		};
class UnknownTagLog   : public TagLog			{	public:	UnknownTagLog();	};
class UnhandledTagLog : public TagLog			{	public:	UnhandledTagLog();	};

class AttributeLog	  : public KeyValueTextList
{
	public:
		AttributeLog();
		void AddTagAndAttribute(const char* tag, const char* attribute,
								const char* context, const char* URL);
};
class BadAttributeLog : public AttributeLog 	{	public:	BadAttributeLog();	};

#endif // (!defined(LOG_ALL_TAGS_AND_ATTRIBUTES)) && (!defined(LOG_BAD_TAGS_AND_ATTRIBUTES))

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//	globals
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
extern LapTimer gLapTimer;
extern IdleLapTimer gIdleLapTimer;
extern PageLapTimer gPageLapTimer;
extern InputCounter gInputCounter;

#if defined(LOG_ALL_TAGS_AND_ATTRIBUTES)
extern KnownTagLog gKnownTagLog;
extern AttributeLog gAttributeLog;
#endif // defined(LOG_ALL_TAGS_AND_ATTRIBUTES)

#if defined(LOG_BAD_TAGS_AND_ATTRIBUTES)
extern BadAttributeLog gBadAttributeLog;
extern UnknownTagLog gUnknownTagLog;
extern UnhandledTagLog gUnhandledTagLog;
#endif // defined(LOG_BAD_TAGS_AND_ATTRIBUTES)


#endif /*	SIMULATOR */

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include SimulatorStatistics.h multiple times"
	#endif
#endif /* __SIMULATORSTATISTICS_H__ */

#endif /* #if 0 */