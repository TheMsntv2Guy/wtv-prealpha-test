// ===========================================================================
//	SimulatorStatistics.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#error еее Obsolete

#if 0

#include "Headers.h"

#ifndef __INPUT_H__
#include "Input.h"
#endif
#ifndef __MACINTOSHUTILITIES_H__
#include "MacintoshUtilities.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __SIMULATORSTATISTICS_H__
#include "SimulatorStatistics.h"
#endif
#ifndef __TEWINDOW_H__
#include "TEWindow.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif



// ===========================================================================
//	globals
// ===========================================================================

#ifdef FOR_MAC
#define BULLET_CHAR 'е'
#define BULLET_STR "е"
#else
#define BULLET_CHAR 0xA5
#define BULLET_STR "\245"	/* 245 (base 8) == a5 (base 16) */
#endif

const ulong kTicksPerSecond = 60;
LapTimer gLapTimer;
IdleLapTimer gIdleLapTimer;
PageLapTimer gPageLapTimer;
InputCounter gInputCounter;

#if defined(LOG_ALL_TAGS_AND_ATTRIBUTES)
KnownTagLog gKnownTagLog;
AttributeLog gAttributeLog;
#endif // defined(LOG_ALL_TAGS_AND_ATTRIBUTES)

#if defined(LOG_BAD_TAGS_AND_ATTRIBUTES)
BadAttributeLog gBadAttributeLog;
UnknownTagLog gUnknownTagLog;
UnhandledTagLog gUnhandledTagLog;
#endif // defined(LOG_BAD_TAGS_AND_ATTRIBUTES)

static const char kLapTimerDefaultTitle[] = "LapTimer Default Title";
static const char kKeyValueTextListDefaultTitle[] = "KeyValueTextList Default Title";
static const char kKeyValueTextListDefaultKeyLabel[] = "Key";
static const char kKeyValueTextListDefaultValueLabel[] = "Value";




// ===========================================================================
//	implementation
// ===========================================================================

void PrintSimulatorStatistics(void)
{
	const char* filename = GenerateDumpFilename("WebTV-Warrior Stat Dump ");

	FILE* fp = fopen(filename, "w");
	
	if (fp == nil)
	{
		Complain(("Can't open output file to dump simulator statistics"));
		return;
	}
	
	const char* dateString = DateFunction();
	const char* timeString = TimeFunction();

	fprintf(fp, "Statistics Dump to '%s' (%s; %s)\n", filename, timeString, dateString);
									fprintf(fp, "\n");
	gLapTimer.Print(fp);
									fprintf(fp, "\n");
	gInputCounter.Print(fp);
									fprintf(fp, "\n");
#if defined(LOG_ALL_TAGS_AND_ATTRIBUTES)
	gKnownTagLog.Print(fp);
									fprintf(fp, "\n");
	gAttributeLog.Print(fp);
									fprintf(fp, "\n");
#endif // defined(LOG_ALL_TAGS_AND_ATTRIBUTES)
#if defined(LOG_BAD_TAGS_AND_ATTRIBUTES)
	gBadAttributeLog.Print(fp);
									fprintf(fp, "\n");
	gUnknownTagLog.Print(fp);
									fprintf(fp, "\n");
	gUnhandledTagLog.Print(fp);
#endif // defined(LOG_BAD_TAGS_AND_ATTRIBUTES)

	fclose(fp);
	
#ifdef FOR_MAC
		TEWindow* tw = newMac(TEWindow);
		if (tw)
		{
			tw->ResizeWindow(300, 200);
			tw->ReadFromFile(filename);
			tw->SetTitle(filename);
			tw->ShowWindow();
		}
#endif // #ifdef FOR_MAC

//	DisposeSimulatorMemory(filename);
	return;
}
// ============================================================================

InputCounter::InputCounter(void)
{
	Initialize();
}
InputCounter::~InputCounter(void)
{
}

Boolean
InputCounter::Initialize(void)
{
	for (int key=0; key<kNumKeyIndices; key++)
	{
		for (int stage = 0; stage < kNumInputCounterStages; stage++)
			fPresses[key][stage] = 0;
	}
	return true;
}

void InputCounter::Print(FILE* fp) const
{
	fprintf(fp, "InputCounter statistics:\n");
	fprintf(fp, "Input\tTotal\tSystem\tOther\tUnhandled\n");
	
#if 0
	for (int index=0; index < kNumKeyIndices; index++)
	{
		ulong value = QuadChar('?','?','?','?');

		switch (index)
		{
			case kDownKeyIndex:			value = kDownKey;			break;
			case kExecuteKeyIndex:		value = kExecuteKey;		break;
			case kHomeKeyIndex:			value = kHomeKey;			break;
			case kLeftKeyIndex:			value = kLeftKey;			break;
			case kOptionsKeyIndex:		value = kOptionsKey;		break;
			case kBackKeyIndex:			value = kBackKey;			break;
			case kScrollDownKeyIndex:	value = kScrollDownKey;		break;
			case kForwardKeyIndex:		value = kForwardKey;		break;
			case kScrollUpKeyIndex:		value = kScrollUpKey;		break;
			case kPowerKeyIndex:		value = kPowerKey;			break;
			case kRecentKeyIndex:		value = kRecentKey;			break;
			case kRightKeyIndex:		value = kRightKey;			break;
			case kUpKeyIndex:			value = kUpKey;				break;

			default:
				if (IsKeyboardInput(index - kKeyboardBaseIndex))
				{
					// construct 0xHH hexadecimal for keyboard presses
					value = QuadChar('0','x','0','0');
					
					int sixteens = ((index - kKeyboardBaseIndex)>>4) & 0xf;
					if (sixteens >= 10)
						value += ('a' - '0' - 10)<<8;
					value += sixteens<<8;
					
					int ones = (index - kKeyboardBaseIndex) & 0xf;			
					if (ones >= 10)
						value += ('a' - '0' - 10);
					value += ones;
				}
				else
				{
					Complain(("Unrecognized input...don't know how to print it to logfile"));
				}
				break;
		}

		char a = (char)((value>>24) & 0x0ff);
		char b = (char)((value>>16) & 0x0ff);
		char c = (char)((value>>8)  & 0x0ff);
		char d = (char)(value       & 0x0ff);
		if (fPresses[index][kIncomingPress] > 0)
		{	fprintf(fp, "%c%c%c%c    \t%d    \t%d    \t%d    \t%d\n", a, b, c, d,
					fPresses[index][kIncomingPress],
					fPresses[index][kIncomingPress]  - fPresses[index][kNonSystemPress],
					fPresses[index][kNonSystemPress] -  fPresses[index][kUnhandledPress],
					fPresses[index][kUnhandledPress]);
		}
	}	
#endif	
}

void InputCounter::LogInput(Input* input, InputCounterStage stage)
{
	int index;
	
	if (input->data != 0)
	{
		index = 3;
	}
	
	switch (input->data)
	{
		case kDownKey:			index = kDownKeyIndex;			break;
		case kExecuteKey:		index = kExecuteKeyIndex;		break;
		case kHomeKey:			index = kHomeKeyIndex;			break;
		case kLeftKey:			index = kLeftKeyIndex;			break;
		case kOptionsKey:		index = kOptionsKeyIndex;		break;
		case kBackKey:			index = kBackKeyIndex;			break;
		case kScrollDownKey:	index = kScrollDownKeyIndex;	break;
		case kForwardKey:		index = kForwardKeyIndex;		break;
		case kScrollUpKey:		index = kScrollUpKeyIndex;		break;
		case kPowerKey:			index = kPowerKeyIndex;			break;
		case kRecentKey:		index = kRecentKeyIndex;		break;
		case kRightKey:			index = kRightKeyIndex;			break;
		case kUpKey:			index = kUpKeyIndex;			break;
		
		default:
			if (IsKeyboardInput(input) && (input->data != 0))
				index = input->data + kKeyboardBaseIndex;
			else
				return;
	}

	Assert(index < kNumKeyIndices);
	Assert(stage < kNumInputCounterStages);
	
	fPresses[index][stage]++;
	return;
}

// ============================================================================

LapTimer::LapTimer(void)
{
	Reset();
	SetTitle(kLapTimerDefaultTitle);
	SetBigLapCutoff(kLapTimerBigLapCutoff);
}

LapTimer::~LapTimer(void)
{
}

void
LapTimer::Start()
{
	if (fActive)
	{
		Complain(("Tried to start a lap timer that was running...resetting timer"));
	}
	Reset();
	fStartTime = Now();
	fLastLapStartTime = fStartTime;
	fPaused = false;
	fActive = true;
}

void
LapTimer::Stop()
{
	if (fActive)
	{
		fActive = false;
		Resume();
	}
}

void
LapTimer::Reset()
{
	for (int i=0; i<kNumLapsRecorded; i++)
	{
		fWorstTime[i] = 0;
	}
	fStartTime        = 0;
	fLastLapStartTime = 0;
	fPauseTime        = 0;
	fLastLapPauseTime = 0;
	fCurrPauseTime    = 0;
	fMinLapTime       = 0x7fff;
	fMaxLapTime       = 0;
	fBigLapTime       = 0;
	fNumLaps          = 0;
	fNumBigLaps       = 0;
	fPaused           = false;
	fActive           = false;
}

void
LapTimer::Pause()
{
	if (!fPaused)
	{
		fCurrPauseTime = Now();
		fPaused = true;
	}
}

void
LapTimer::Resume()
{
	if (fPaused)
	{
		ulong pauseDelta = Now() - fCurrPauseTime;
		fLastLapPauseTime += pauseDelta;
		fPaused = false;
	}
}

ulong
LapTimer::Lap()
{
	if (!fActive)
	{
		Message(("LapTimer::Lap() called when timer isn't active!"));
		return (ulong)0;
	}

	if (fPaused)
	{
		Message(("LapTimer::Lap() called when timer is paused!"));
		return (ulong)0;
	}

	ulong nowTime = Now();
	ulong lapTime = (nowTime - fLastLapStartTime) - fLastLapPauseTime;
	
	fNumLaps++;
	fPauseTime += fLastLapPauseTime;
	fLastLapPauseTime = 0;
	if (fStartTime == fLastLapStartTime)
	{
		fMinLapTime = lapTime;
		fMaxLapTime = lapTime;
	}
	else
	{
		if (lapTime < fMinLapTime)
			fMinLapTime = lapTime;
		if (lapTime > fMaxLapTime)
			fMaxLapTime = lapTime;
	}
	fLastLapStartTime = nowTime;
	RecordLapTime(lapTime);
	if (lapTime >= fBigLapCutoff)
	{
		fBigLapTime += lapTime;
		fNumBigLaps++;
	}
	return lapTime;
}

void
LapTimer::RecordLapTime(ulong lapTime)
{
	if (lapTime > fWorstTime[kNumLapsRecorded-1])
	{
		int i = 0;
		while (lapTime < fWorstTime[i])	// where to insert this?
		{
			i++;
		}
		while (i<kNumLapsRecorded) // slide everything else down
		{
			ulong newLapTime = fWorstTime[i];
			fWorstTime[i] = lapTime;
			lapTime = newLapTime;
			i++;
		}
	}
}

void
LapTimer::SetTitle(const char* title)
{
	fTitle = title;
}

void
LapTimer::SetBigLapCutoff(ulong bigLapCutoff)
{
	fBigLapCutoff = bigLapCutoff;
}

ulong LapTimer::GetTotalTime(void) const		{ return fLastLapStartTime - fStartTime; }
ulong LapTimer::GetPauseTime(void) const		{ return fPauseTime; }
ulong LapTimer::GetActiveTime(void) const		{ return GetTotalTime() - fPauseTime; }
ulong LapTimer::GetBigLapCutoff(void) const		{ return fBigLapCutoff; }
ulong LapTimer::GetSmallLapsTime(void) const	{ return GetActiveTime() - fBigLapTime; }
ulong LapTimer::GetBigLapsTime(void) const		{ return fBigLapTime; }

ulong LapTimer::GetNumLaps(void) const			{ return fNumLaps; }
ulong LapTimer::GetNumBigLaps(void) const		{ return fNumBigLaps; }
ulong LapTimer::GetNumSmallLaps(void) const		{ return fNumLaps - fNumBigLaps; }

Boolean LapTimer::IsActive(void) const			{ return fActive; }
Boolean LapTimer::IsPaused(void) const			{ return fPaused; }

void
LapTimer::Print(FILE* fp) const
{
	fprintf(fp, "%s\n", fTitle);
	fprintf(fp, "Start time:   %d\n", fStartTime);
	fprintf(fp, "End time:     %d\n", fLastLapStartTime);
	fprintf(fp, "Paused time:  %d ticks (%2.2f seconds)\n",
				fPauseTime,
				(float)fPauseTime/kOneSecond);
	fprintf(fp, "Active time:  %d ticks (%2.2f seconds)\n",
				GetActiveTime(),
				(float)GetActiveTime()/kOneSecond);
	fprintf(fp, "\n");
	if (fNumLaps > 0)
	{
		fprintf(fp, "Num laps:     %d (%d=%2.2f%% took more than %d ticks)\n",
					fNumLaps,
					fNumBigLaps,
					100.0*fNumBigLaps/fNumLaps,
					fBigLapCutoff);
		fprintf(fp, "Avg time/lap: %2.2f ticks (%2.2f seconds)\n",
					(float)GetActiveTime()/fNumLaps,
					(float)GetActiveTime()/fNumLaps/kOneSecond);
		fprintf(fp, "Avg of small laps (< %d ticks): %2.2f ticks (%2.2f seconds)\n",
					fBigLapCutoff,
				 	(float)(GetActiveTime()-fBigLapTime)/(fNumLaps-fNumBigLaps),
				 	(float)(GetActiveTime()-fBigLapTime)/(fNumLaps-fNumBigLaps)/kTicksPerSecond);
		fprintf(fp, "Avg of big laps   (>=%d ticks): %2.2f ticks (%2.2f seconds)\n",
					fBigLapCutoff,
				 	(float)fBigLapTime/fNumBigLaps,
				 	(float)fBigLapTime/fNumBigLaps/kOneSecond);
		fprintf(fp, "Min lap time: %d ticks (%2.2f seconds)\n",
					fMinLapTime,
					(float)fMinLapTime/kOneSecond);
		fprintf(fp, "Max lap time: %d ticks (%2.2f seconds)\n",
					fMaxLapTime,
					(float)fMaxLapTime/kOneSecond);
		fprintf(fp, "Top %d Worst laps (ticks):", kNumLapsRecorded);
		for (int i=0; (i<kNumLapsRecorded) && (fWorstTime[i]>0); i++)
		{
			fprintf(fp, " %d", fWorstTime[i]);
		}
		fprintf(fp, "\n");
	}
}

IdleLapTimer::IdleLapTimer(void)
	: LapTimer()
{
	SetTitle("Idle loop lap timer");
	SetBigLapCutoff(kIdleLapTimerBigLapCutoff);
}

PageLapTimer::PageLapTimer(void)
	: LapTimer()
{
	SetTitle("Page loop lap timer");
	SetBigLapCutoff(kPageLapTimerBigLapCutoff);
}
// ============================================================================

TextList::TextList(void)
{
	fCurrGet = 0;
	fCurrPut = 0;
	fLogSize = 0;
	fLog = nil;
}

TextList::~TextList(void)
{
	Clear();
}

Boolean
TextList::Add(const char* str)
{
	size_t len = strlen(str) + 1;
	if (len + fCurrPut > fLogSize)
	{
		size_t expandBy = ((len / kTextLogCacheIncr) + 1) * kTextLogCacheIncr;
		
		if (Resize(fLogSize + expandBy))
			return Add(str);
		else
			return false;
	}
	
	memcpy((void*)&(fLog[fCurrPut]), (void*)str, len);
	fCurrPut += len;
	return true; 
}

void
TextList::Clear(void)
{
	fCurrGet = 0;
	fCurrPut = 0;
	fLogSize = 0;
	if (fLog != nil)
	{
		DisposeSimulatorMemory(fLog);
		fLog = nil;
	}
}

Boolean
TextList::Find(const char* str)
{
	size_t tempGet = fCurrGet;	// save iterator
	
	for(const char* item = GetFirst(); !AtEnd(); item = GetNext())
	{
		if (EqualString(str, item))
		{
			fCurrGet = tempGet;	// restore iterator
			return true;
		}
	}
	
	fCurrGet = tempGet;	// restore iterator
	return false;
}

const char*
TextList::GetFirst(void)
{
	fCurrGet = 0;
	if (!AtEnd())
		return fLog;
	else
		return nil;
}

const char*
TextList::GetNext(void)
{
	if (AtEnd())
		return nil;
		
	fCurrGet += strlen(&fLog[fCurrGet]) + 1;

	if (AtEnd())
		return nil;

	return &(fLog[fCurrGet]);
}

const char*
TextList::GetCurrent(void)
{
	if (!AtEnd())
		return &(fLog[fCurrGet]);
	else
		return nil;
}

ulong
TextList::GetCurrentIndex(void) const
{
	size_t		offset;
	ulong		index = 0;
	
	for (offset = 0; offset < fCurrPut; offset += strlen(&fLog[offset]) + 1)
	{
		if (offset == fCurrGet)
			break;
		index++;
	}

	return index;
}

// This code has never been tested
//const char*
//TextList::GetIndex(ulong index)
//{
//	size_t offset = 0;
//	while (index-- > 0)
//	{
//		offset += strlen(&fLog[offset]) + 1;
//		if (offset >= fCurrPut)
//			return nil;
//	}
//	return &(fLog[offset]);
//}

Boolean
TextList::AtEnd(void)
{
	return (fCurrGet >= fCurrPut);
}

void
TextList::Print(FILE* fp)
{
	size_t tempGet = fCurrGet;
	
	const char* item   = GetFirst();
	
	fprintf(fp, "Item\n");
	fprintf(fp, "----\n");
	while (!AtEnd())
	{
		fprintf(fp, "%s\n", item);
		item = GetNext();
	}
	fprintf(fp, "----\n");
	
	fCurrGet = tempGet;
}

Boolean
TextList::Resize(const size_t newLogSize)
{
	char* newLog = NewSimulatorMemory(newLogSize);
	if ((newLog == nil) && (newLogSize != 0))
	{
		Complain(("Can't Resize() TextLog from %d to %d bytes", fLogSize, newLogSize));
		return false;
	}
	
	if ((fLog != nil) && (fLogSize != 0))
	{
		memcpy(newLog, fLog, fLogSize);
		DisposeSimulatorMemory(fLog);
	}
	
	fLog = newLog;
	fLogSize = newLogSize;
	fLog[fLogSize-1] = 0;	// make sure it's zero-terminated
	return true;
}

// ============================================================================

KeyValueTextList::KeyValueTextList(void)
{
	fKeyList.Clear();
	fValueList.Clear();
	SetTitle(kKeyValueTextListDefaultTitle);
	SetKeyLabel(kKeyValueTextListDefaultKeyLabel);
	SetValueLabel(kKeyValueTextListDefaultValueLabel);
}

KeyValueTextList::~KeyValueTextList(void)
{
}

const char*
KeyValueTextList::GetValue(const char* key)
{
	const char* keyString   = fKeyList.GetFirst();
	const char* valueString = fValueList.GetFirst();
	
	while ((!fKeyList.AtEnd()) && (!fValueList.AtEnd()))
	{
		if (EqualString(keyString, key))
			return valueString;
		keyString   = fKeyList.GetNext();
		valueString = fValueList.GetNext();
	}
	Assert(fKeyList.AtEnd() && fValueList.AtEnd());	// else lists are different lengths
	return nil;
}

void
KeyValueTextList::Add(const char* key, const char* value)
{
	if (!fKeyList.Find(key))	// if we didn't find it...
	{
		fKeyList.Add(key);			// add the key
		fValueList.Add(value);		// and the value
	}
}

void
KeyValueTextList::Clear(void)
{
	fKeyList.Clear();
	fValueList.Clear();
}


void
KeyValueTextList::Print(FILE* fp)
{
	const char* keyString = fKeyList.GetFirst();
	const char* valueString = fValueList.GetFirst();
	const TABSIZE = 4;
	
	fprintf(fp, "%s\n", fTitle);
	fprintf(fp, "%s\t%s\n", fKeyLabel, fValueLabel);
	
	if (fKeyList.AtEnd())
	{
		fprintf(fp, "<NONE>\n");
	}
	else
	{
		int length = (((strlen(fKeyLabel)+TABSIZE)/TABSIZE)*TABSIZE) + strlen(fValueLabel);
		int count;
		
		for (count = 0; count < length; count++)
		{
			fprintf(fp, "-");
		}
		fprintf(fp, "\n");
	
		while ((!fKeyList.AtEnd()) && (!fValueList.AtEnd()))
		{
			fprintf(fp, "%s\t%s\n", keyString, valueString);
			keyString   = fKeyList.GetNext();
			valueString = fValueList.GetNext();
		}
		
		Assert(fKeyList.AtEnd() && fValueList.AtEnd());	// else lists are different lengths
		
		for (count = 0; count < length; count++)
		{
			fprintf(fp, "-");
		}
		fprintf(fp, "\n");
	}
}

void
KeyValueTextList::SetTitle(const char* title)
{
	fTitle = title;
}

void
KeyValueTextList::SetKeyLabel(const char* keyLabel)
{
	fKeyLabel = keyLabel;
}

void
KeyValueTextList::SetValueLabel(const char* valueLabel)
{
	fValueLabel = valueLabel;
}

// ============================================================================

#if (!defined(LOG_ALL_TAGS_AND_ATTRIBUTES)) && (!defined(LOG_BAD_TAGS_AND_ATTRIBUTES))

TagLog::TagLog(void) : KeyValueTextList()
{
	SetTitle("Tag log");
	SetKeyLabel("Tag:");
	SetValueLabel("First found in URL:");
}

KnownTagLog::KnownTagLog(void) : TagLog()
{
	SetTitle("Known tags: (recognized these tags)");
}

UnhandledTagLog::UnhandledTagLog(void) : TagLog()
{
	SetTitle("Unhandled tags: (didn't know what to do with these tags)");
}

UnknownTagLog::UnknownTagLog(void) : TagLog()
{
	SetTitle("Unknown tags: (didn't even *recognize* these tags)");
}

AttributeLog::AttributeLog(void) : KeyValueTextList()	
{
	SetTitle("Tag" BULLET_STR "Attribute\t<Context>" BULLET_STR "<URL>");
	SetKeyLabel("Tag" BULLET_STR "Attribute");
	SetValueLabel("First found:");
}

void
AttributeLog::AddTagAndAttribute(const char* tag, const char* attribute,
								 const char* context, const char* URL)
{
	char temp1[128];
	int tagLength = strlen(tag);
	int attributeLength = strlen(attribute);
	Assert(tagLength + attributeLength + 1 < 128);

	strcpy(temp1, tag);
	temp1[tagLength]=BULLET_CHAR; // use BULLET_CHAR to separate tag and attribute
	strcpy(&(temp1[tagLength+1]), attribute);
	
	char temp2[1024];
	int contextLength = strlen(context);
	int URLLength = strlen(URL);
	Assert(contextLength + URLLength + 5 < 1024);
	
	temp2[0]='<';
	strcpy(&temp2[1], context);
	temp2[contextLength+1]='>';
	temp2[contextLength+2]=BULLET_CHAR;
	temp2[contextLength+3]='<';
	strcpy(&(temp2[contextLength+4]), URL);
	temp2[contextLength+URLLength+5]='>';
	
	Add(temp1, temp2);
}

BadAttributeLog::BadAttributeLog(void) : AttributeLog()	
{
	SetTitle("Bad attribute: (didn't recognize these attributes)");
}
#endif // (!defined(LOG_ALL_TAGS_AND_ATTRIBUTES)) && (!defined(LOG_BAD_TAGS_AND_ATTRIBUTES))

#endif /* #if 0 */