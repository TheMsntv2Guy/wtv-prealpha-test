// ===========================================================================
//	RealAudio.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef	__REALAUDIO_H__
#define	__REALAUDIO_H__


#ifndef	__AUDIO_H__
#include "Audio.h"
#endif

class CRaSession;

class	RealAudioStream : public AudioStream {
public:
						RealAudioStream();
	virtual				~RealAudioStream();

	virtual	void		Start();
	virtual void		Stop();
	virtual Boolean		Idle();
	void				SetURL(const char* url);
	
protected:
	void				CloseStream();
	Boolean				StillPlaying();
	short				OpenStream();
	long				StartPlaying();
	long				fError;
	
	char*				fURL;
	CRaSession*			fSession;	
};

#endif	/* __REALAUDIO_H__ */

