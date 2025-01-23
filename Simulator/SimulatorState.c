// ===========================================================================
//	SimulatorState.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"
#ifndef __MACINTOSH_H__
#include "Macintosh.h"
#endif
#ifndef __SIMULATOR_H__
#include "Simulator.h"
#endif
#ifndef __SIMULATORSTATE_H__
#include "SimulatorState.h"
#endif

const char* kSimulatorStateName = "Simulator State";

void
SaveSimulatorStateAs(FSSpec* saveFile)
{
	OSErr err;
	short refNum;
	
	ImportantMessage(("Saving state to file '%s'...", &(saveFile->name[1])));
	err = FSpCreate(saveFile, rAppSignature, 'RAM ', smSystemScript);
	if (err != noErr)
	{	Complain(("Cannot write to file %s (error %d)", &(saveFile->name[1]), err));
		return;
	}
	
	err = FSpOpenDF(saveFile, fsWrPerm, &refNum);
	if (err != noErr)
	{	Complain(("Cannot open file %s (error %d)", &(saveFile->name[1]), err));
		return;
	}
	
	char* baseRAMPtr = gSimulator->GetRAMBaseAddress();
	long RAMSize = gSimulator->GetRAMSize();
	long RAMWritten = RAMSize;
	
	FSWrite(refNum, &RAMWritten, baseRAMPtr);
	if (err != noErr)
	{	Complain(("Error writing to file %s, (error %d)", &(saveFile->name[1]), err));
	}
	else if (RAMWritten != RAMSize)
	{	Complain(("WARNING: only wrote %d of %d bytes to %s",
					RAMWritten, RAMSize, &(saveFile->name[1])));
	}

	FSClose(refNum);
	ImportantMessage(("Done saving state to file '%s'", &(saveFile->name[1])));
}


void
LoadSimulatorStateAs(FSSpec* UNUSED(loadFile))
{
	ImportantMessage(("Calling LoadSimulatorStateAs() (file: %s, line: %d)", __FILE__,  __LINE__));
}


void
SaveSimulatorState(void)
{
	ImportantMessage(("Calling SaveSimulatorState() (file: %s, line: %d)", __FILE__,  __LINE__));
}


void
LoadSimulatorState()
{
	ImportantMessage(("Calling LoadSimulatorState() (file: %s, line: %d)", __FILE__,  __LINE__));
}