
#ifndef __HW_AUDIO__
#define __HW_AUDIO__

#ifdef __cplusplus
extern "C" {
#endif

#define	kAudioBufferSize	2048	/* 11.6ms @ 44100 16-bit stereo */
#define	kAudioBufAlign		31

#define	kAudioBufferSegment	0xC0000000	/* make buffer write-through */

extern ulong gAudioBuf0;			/* These are PHYSICAL addresses */
extern ulong gAudioBuf1;			/* for the DMA engine. */

extern ulong gCurrentAudioBuf;


void InitAudio(void);				/* Alloc audio bufs, init HW */

void SetAudioOptions(ulong flags);	/* See kAudFCntl bits in HWRegs */
ulong GetAudioOptions(void);

void SetAudioDMASize(ulong size);
void SetAudioDMAStart(ulong startbuf);		/* pass in 0 or 1 */

void StartAudioDMA(void);
void StopAudioDMA(void);
void audioInterruptHandler(void);

#ifdef __cplusplus
}
#endif

#endif
