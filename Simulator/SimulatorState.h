//
//	SimulatorState.h
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
//

#ifndef __SIMULATORSTATE_H__
#define __SIMULATORSTATE_H__

void SaveSimulatorStateAs(FSSpec* saveFile);
void SaveSimulatorState(void);
void LoadSimulatorStateAs(FSSpec* loadFile);
void LoadSimulatorState(void);

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include SimulatorState.h multiple times"
	#endif
#endif /* __SIMULATORSTATE_H__ */
