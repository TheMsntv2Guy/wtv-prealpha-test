
/*****************************************************/
/*
**	SoundMusicMidiSys.h
**
**		Structure and calls for working with the MIDI Manager
**
**  		(c) 1989-1996 by Steve Hales, All Rights Reserved
**  
**
**	History	-
** 	6/14/93		Created
**	12/11/95	Added connection types
**				Removed MDH_PrepForInstrumentPlayback. Now use EnableLiveLink
**				with a USE_EXTERNAL connection type
**				Added DisableLiveLink
**	2/12/96		Changed Boolean to SMSBoolean
**	3/18/96		Added PV_SetOMSNameDocument
**
*/
/*****************************************************/

#ifndef SOUNDMUSICSYS_DRIVER_MIDI
#define SOUNDMUSICSYS_DRIVER_MIDI

#include <Midi.h>

#include "PrivateSoundMusicSystem.h"

#define jxLiveInput					100		// midi input

//#define kPlayNote					(short int)200	// kPlayNote, NoteInfo *
//#define kStopNote					(short int)201	// kStopNote, NoteInfo *
//#define kStopAllNotes				(short int)202	// kStopAllNotes
//#define kEnableMIDIInput				(short int)203	// kEnableMIDIInput, voices
//#define kEnableMIDIOutput				(short int)204	// kEnableMIDIOutput


//#define EnableMIDIInput(maxV, normV)	MusicDriverHandler(kEnableMIDIInput, (long)(((long)(maxV) << 16L) | (normV)))
//#define EnableMIDIOutput()			MusicDriverHandler(kEnableMIDIOutput, 0)

struct NoteInfo
{
	Word			program;
	Word			pitch;
	Word			duration;
	Word			velocity;
	Word			channel;
	long			timeStamp;		// Used in buffer only
	SMSBoolean	processed;		// FALSE, if not processed
};
typedef struct NoteInfo NoteInfo;

/* Midi Support.c
*/

long PV_GetExternalTimeSync(void);

SMSErr SetupExternalMidi(short theIconID, short connectionType);
void CleanupExternalMidi(void);

void PV_SetOMSNameDocument(XFILENAME *pNameDoc);

void QLockNotes(void);
void QUnlockNotes(void);


// MidiQueue68k.c

void Q68K_ProcessMidiNoteOn(long timeStamp, short int channel, short int note, short int velocity);
void Q68K_ProcessMidiNoteOff(long timeStamp, short int channel, short int note, short int velocity);
void Q68K_ProcessProgramChange(long timeStamp, short int channel, short int program);
void Q68K_ProcessController(long timeStamp, short int channel, short int controller, short int value);
void Q68K_LockExternalMidiQueue(void);
void Q68K_UnlockExternalMidiQueue(void);
void PV_68K_ProcessExternalMIDIQueue(long tempA5);
void ResetMidiParameters_68K(void);
void MDH_PlayNote(NoteInfo *theNote);
void MDH_StopNote(NoteInfo *theNote);


/* MusicSystem.c
*/
//void MDH_StopNotes(void);

#endif //	SOUNDMUSICSYS_DRIVER_MIDI
