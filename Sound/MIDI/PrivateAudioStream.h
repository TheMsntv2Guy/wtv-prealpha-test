/*****************************************************/
/*
**	PrivateAudioStream.h
**
**		This implements multi source audio streaming code.
**
**		© Copyright 1989-1996 by Steve Hales, All Rights Reserved.
**		No portion of this program (whether in source, object, or other form)
**		may be copied, duplicated, distributed, or otherwise utilized without
**		direct consent of the copyright holder.
**  
**
**	History	-
** 	5/1/95		Created
**	11/29/95	long word align AudioStream structure
**
*/
/*****************************************************/

#ifndef AUDIO_STREAM
#define AUDIO_STREAM

// Used for audio streams
#define MAX_STREAMS		16

#define	AudioStream	XAudioStream

struct AudioStream
{
	long					reference;
	short int				streamActive;
	short int				streamShuttingDown;
	long					playbackReference;

	AudioStreamObjectProc	streamCallback;
	AudioStreamData		streamData;
	void *				pStreamBuffer;
	long					streamBufferLength;

	long					streamOrgLength1;
	long					streamOrgLength2;

	void *				pStreamData1;
	void *				pStreamData2;
	long					streamLength1;
	long					streamLength2;
	short int				streamMode;
	short int				streamVolume;
	short int				streamStereoPosition;
	short int				streamPaused;
	short int				streamFirstTime;			// first time active
	short int				alignWord0;
};
typedef struct SMSAudioStream SMSAudioStream;


OSErr	AudioStreamOpen(short int startMaxStreams);
OSErr	AudioStreamClose(void);
void		AudioStreamPause(void);
void		AudioStreamResume(void);

#endif	// AUDIO_STREAM

