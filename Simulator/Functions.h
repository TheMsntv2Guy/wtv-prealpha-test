// ===========================================================================
//	Functions.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================
#ifndef __FUNCTIONS_H__
#define __FUNCTIONS_H__

void BuildFunctionsMenu(void);
void HandleFunctionsMenu(short menuItem);

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Functions.h multiple times"
	#endif
#endif /* __FUNCTIONS_H__ */
