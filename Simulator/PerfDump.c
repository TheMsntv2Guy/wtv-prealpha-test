// ===========================================================================
//	PerfDump.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================


#include "Headers.h"

#ifdef FOR_MAC
	#ifndef __MACSIMULATOR_H__
	#include "MacSimulator.h"
	#endif
	#ifndef __MACINTOSHUTILITIES_H__
	#include "MacintoshUtilities.h"
	#endif
#endif
#ifndef __PERFDUMP_H__
#include "PerfDump.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif

// ===========================================================================
//	PerfDump
// ===========================================================================




// ===========================================================================
//	ProfileUnit
// ===========================================================================

static Boolean gProfileUnitEnabled = false;

const char kPrefsProfileFormat[]		= "Profile \"%.50s\"";

static const char* const kProfileUnitNames[] = {
	"Network::Idle",
	"Network::IdleActive",
	"Screen::Draw"
};

static const int kNumProfileUnits
		= sizeof(kProfileUnitNames) / sizeof(kProfileUnitNames[0]);

static Boolean kProfileUnitSetting[kNumProfileUnits]; // do we profile this?

static int gProfileUnitDepth = 0;

// ===========================================================================

void
InitializeProfileUnit(void)
{
	char prefName[128];
	for (int index=0; index<kNumProfileUnits; index++) {
		snprintf(prefName, sizeof(prefName), kPrefsProfileFormat, kProfileUnitNames[index]);
		gMacSimulator->GetPreferenceBoolean(prefName, &(kProfileUnitSetting[index]));
	}
}

void
FinalizeProfileUnit(void)
{
	char prefName[128];
	for (int index=0; index<kNumProfileUnits; index++) {
		snprintf(prefName, sizeof(prefName), kPrefsProfileFormat, kProfileUnitNames[index]);
		gMacSimulator->SetPreferenceBoolean(prefName, kProfileUnitSetting[index]);
	}
}

Boolean
GetProfileUnitEnabled(void)
{
	return gProfileUnitEnabled;	
}

void
SetProfileUnitEnabled(Boolean enabled)
{
	gProfileUnitEnabled = enabled;
	
	if (enabled)
		gMacSimulator->StartProfiling();
	else
		gMacSimulator->StopProfiling();
}

void
ProfileUnitEnter(int index)
{
	if (IsError((index < 0) || (index >= kNumProfileUnits)))
		return;
	
	if (kProfileUnitSetting[index]) {
		if (gProfileUnitDepth == 0) {
			if (gProfileUnitEnabled) {
#ifdef FOR_MAC
				gMacSimulator->StartProfiling();
#endif
			}
		}
		gProfileUnitDepth++;
	}
}

void
ProfileUnitEnter(const char* name)
{
	for (int i=0; i<kNumProfileUnits; i++) {
		if (EqualString(name, kProfileUnitNames[i])) {
			ProfileUnitEnter(i);
			return;
		}
	}
	Complain(("ProfileUnitEnter(%s) didn't recognize %s.  Add it "
			  "to the array kProfileUnitNames in PerfDump.c.", name, name));
}

void
ProfileUnitExit(int index)
{
	if (IsError((index < 0) || (index >= kNumProfileUnits)))
		return;
	
	if (kProfileUnitSetting[index]) {
		gProfileUnitDepth--;
		if (gProfileUnitDepth == 0) {
			if (gProfileUnitEnabled) {
#ifdef FOR_MAC
				gMacSimulator->StopProfiling();
#endif
			}
		}
	}
}

void
ProfileUnitExit(const char* name)
{
	for (int i=0; i<kNumProfileUnits; i++) {
		if (EqualString(name, kProfileUnitNames[i])) {
			ProfileUnitExit(i);
			return;
		}
	}
	Complain(("ProfileUnitExit(%s) didn't recognize %s.  Add it "
			  "to the array kProfileUnitNames in ProfileUnit.c.", name, name));
}

int
GetNumProfileUnits()
{
	return kNumProfileUnits;
}

const char*
GetProfileUnitName(int index)
{
	if (IsError((index < 0) || (index >= kNumProfileUnits)))
		return nil;
	
	return kProfileUnitNames[index];	
}

Boolean
GetProfileUnitOn(int index)
{
	if (IsError((index < 0) || (index >= kNumProfileUnits)))
		return false;
	
	return kProfileUnitSetting[index];	
}
Boolean
GetProfileUnitOn(const char* name)
{
	for (int i=0; i<kNumProfileUnits; i++) {
		if (EqualString(name, kProfileUnitNames[i])) {
			return GetProfileUnitOn(i);
		}
	}
	Complain(("GetProfileUnitOn(%s) didn't recognize %s.  Add it "
			  "to the array kProfileUnitNames in ProfileUnit.c.", name, name));
	return false;
}
void
SetProfileUnitOn(int index, Boolean isOn)
{
	if (IsError((index < 0) || (index >= kNumProfileUnits)))
		return;
	
	kProfileUnitSetting[index] = isOn;	
}
void
SetProfileUnitOn(const char* name, Boolean isOn)
{
	for (int i=0; i<kNumProfileUnits; i++) {
		if (EqualString(name, kProfileUnitNames[i])) {
			SetProfileUnitOn(i, isOn);
			return;
		}
	}
	Complain(("SetProfileUnitOn(%s, %s) didn't recognize %s.  Add it "
			  "to the array kProfileUnitNames in ProfileUnit.c.",
			  name, isOn ? "true" : "false", name));
	return;
}

// ------------------------------------------------------------------------------------
//	Performance dumping
// ------------------------------------------------------------------------------------

#ifdef DEBUG_PERFDUMP

struct PerfDumpRecord {
	const char*	tag;
	const char*	file;
	unsigned	line : 24;
	unsigned	event : 8;
	ulong		time;
};

const kNumPerfDumpRecords = 32*1024;

// ------------------------------------------------------------------------------------

static Boolean gPerfDumpActive = false;
static long gCurrPerfDumpIndex = 0;
static PerfDumpRecord gPerfDumpRecord[kNumPerfDumpRecords];

// ------------------------------------------------------------------------------------

Boolean
GetPerfDumpActive()
{
	return gPerfDumpActive;
}

void
PerfDumpInitialize(void)
{
	gCurrPerfDumpIndex = 0;
	for (int index=0; index<kNumPerfDumpRecords; index++) {
		gPerfDumpRecord[index].event = kPerfDumpEventNone;
	}
	gPerfDumpActive = true;
}

void
PerfDumpFinalize(void)
{
#ifdef SIMULATOR
	gPerfDumpActive = false;

	// generate filename
	char filename[64];
	snprintf(filename, sizeof(filename), "%s", GenerateDumpFilename("PerfDump"));

	// open file
	FILE* fp = fopen(filename, "w");
	if (IsError(fp == nil))
		return;

	fprintf(fp, "PerfDump version 1\n");
	
	// write each record
	Boolean foundBaseTime = false;
	ulong baseTime = 0;
		
	for (int count = 0; count < kNumPerfDumpRecords; count++) {
		// is it valid?
		if (gPerfDumpRecord[gCurrPerfDumpIndex].event != kPerfDumpEventNone) {
		
			// were we looking for the first one to get a base time?
			if (!foundBaseTime) {
				foundBaseTime = true;
				baseTime = gPerfDumpRecord[gCurrPerfDumpIndex].time;
			}
			
			// what is the name of this event?
			const char* eventString = "<none>";
			char eventStringBuffer[20];
			switch (gPerfDumpRecord[gCurrPerfDumpIndex].event) {
				case kPerfDumpEventNone:	eventString = "<none>";	break;
				case kPerfDumpEventEnter:	eventString = "Enter";	break;
				case kPerfDumpEventExit:	eventString = "Exit";	break;
				case kPerfDumpEventMark:	eventString = "Mark";	break;
				default:
					snprintf(eventStringBuffer, sizeof(eventStringBuffer), "<%d>",
								(int)gPerfDumpRecord[gCurrPerfDumpIndex].event);
					eventString = eventStringBuffer;
					break;
			};
			
			// print the event
			fprintf(fp, "%s\t"  "%s\t"  "%lu\t"  "%s\t"  "%d\n",
					gPerfDumpRecord[gCurrPerfDumpIndex].tag,
					eventString,
					(ulong)(gPerfDumpRecord[gCurrPerfDumpIndex].time - baseTime),
					gPerfDumpRecord[gCurrPerfDumpIndex].file,
					(int)(gPerfDumpRecord[gCurrPerfDumpIndex].line));
		}
		
		// advance to next record
		if (gCurrPerfDumpIndex < kNumPerfDumpRecords - 1) { 
			gCurrPerfDumpIndex++;
		} else {
			gCurrPerfDumpIndex = 0;
		}
		
	}
	
	// close file
	fclose(fp);
	
#endif /* SIMULATOR */
}

void
RecordPerfDumpEvent(const char* tag, PerfDumpEvent event, const char* file, int line)
{
	gPerfDumpRecord[gCurrPerfDumpIndex].tag = tag;
	gPerfDumpRecord[gCurrPerfDumpIndex].file = file;
	gPerfDumpRecord[gCurrPerfDumpIndex].line = line;
	gPerfDumpRecord[gCurrPerfDumpIndex].event = event;

#ifdef SIMULATOR
	UnsignedWide wide;
	Microseconds(&wide);
	gPerfDumpRecord[gCurrPerfDumpIndex].time = wide.lo;
#endif

	if (gCurrPerfDumpIndex < kNumPerfDumpRecords - 1) { 
		gCurrPerfDumpIndex++;
	} else {
		gCurrPerfDumpIndex = 0;
	}
}

#endif /* DEBUG_PERFDUMP */