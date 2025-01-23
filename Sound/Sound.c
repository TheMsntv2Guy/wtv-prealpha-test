// ===========================================================================
//	Sound.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"
#include "Sound.h"

#if !defined(FOR_MAC)

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
	void SystemBeep(ulong)
	{
	}
#endif
	
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
	void PlaySound(ulong /* soundID */)
	{
	#if 0
		Handle sndHandle;			// handle to an 'snd ' resource
		SndChannelPtr sndChan;		// pointer to a sound channel
		ulong resourceID;
		
		switch (soundID)
		{
			case kClickSound :	resourceID = rPhoneClickSnd; break;
			case kSlideSound :  resourceID = rSlideSnd; break;
			default: 
				break;
		}
	
		sndChan = nil;
	
		// Read in 'snd' resource from resource file
		sndHandle = GetResource ('snd ', resourceID);
	
		// check for a nil handle
		if (sndHandle == nil )
			return;
			
		(void)SndPlay(sndChan, (SndListResource**)sndHandle, true);			
	#endif
	}
#endif
	
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
	void StopSound(void)
	{
	}
#endif

#else /* of #if !defined(FOR_MAC) */

	#include "Resources.h"
	#include "Simulator.rsrc.h"

	static SndChannelPtr gChannel = nil;
	static Handle gSoundResource = nil;
	static ulong GetResourceID(ulong soundID);
	static pascal void SoundCallBack(SndChannelPtr, SndCommand);

	void SystemBeep(ulong milliseconds)
	{
		SysBeep(milliseconds*60/1000);
	}
	
	ulong GetResourceID(ulong soundID)
	{
		switch (soundID)
		{
			case kClickSound :	return rPhoneClickSnd;
			case kSlideSound :  return rSlideSnd;
			default:  break;
		}
		
		return -1;
	}
	
	static pascal void
	SoundCallBack(SndChannelPtr, SndCommand)
	{
		SndCommand sCmd;
		Ptr data = (Ptr)((*gSoundResource) + 20);	// skip resource header			
	
		sCmd.cmd = bufferCmd;
		sCmd.param1 = 0;
		sCmd.param2 = (unsigned long)data;
		SndDoCommand(gChannel, &sCmd, 0);
			
		StopSound();
	}
	
	void PlaySound(ulong soundID)
	{
		ulong resourceID = GetResourceID(soundID);
		SndCommand sCmd;
		Ptr data;
		OSErr fuck;			
		
		if (resourceID == -1)
			return;
			
		if (gChannel != nil || gSoundResource != nil)
			StopSound();
			
		gSoundResource = GetResource('snd ', resourceID);
		
		if (gSoundResource == nil)
			return;
						
		MoveHHi(gSoundResource);
		HLock(gSoundResource);
	
		data = (Ptr)((*gSoundResource) + 20);	// skip resource header	
	    fuck = SndNewChannel(&gChannel, sampledSynth, initStereo, (SndCallBackUPP)nil);
	
	    sCmd.cmd = bufferCmd;
	    sCmd.param1 = 0;
	    sCmd.param2 = (ulong)data;
	    fuck = SndDoCommand(gChannel, &sCmd, 0);
	}
	
	void StopSound(void)
	{
		if (gChannel != nil)
			SndDisposeChannel(gChannel, 1);
	
		if (gSoundResource != nil)
			ReleaseResource(gSoundResource);
			
		gChannel = nil;
		gSoundResource = nil;
	}

#endif  /* of #else of #if !defined(FOR_MAC) */



