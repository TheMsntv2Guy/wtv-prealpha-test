#ifndef __SOUND_H__
#define __SOUND_H__

#ifndef __WTVTYPES_H__
#include "WTVTypes.h"
#endif

void SystemBeep(ulong milliseconds);
void PlaySound(ulong soundID);
void StopSound();

const ulong kClickSound = 0;
const ulong kSlideSound = 1;

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Sound.h multiple times"
	#endif
#endif /* __SOUND_H__ */
