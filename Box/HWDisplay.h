
#ifndef __HW_DISPLAY__
#define __HW_DISPLAY__

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
	kDisplayModeNTSC, 
	kDisplayModePAL
} DisplayMode;


void SetDisplayMode(DisplayMode mode);

void InitDisplay(ulong flags);					/* Set everything up & get a stable raster */

void KillDisplay(void);
void EnableDisplay(void);
void EnableInterlacedDisplay(void);

void EraseDisplay(ulong select);

void SetDisplayOptions(ulong flags);			/* See FCntl bits in HWRegs for flag defs */
ulong GetDisplayOptions(void);


ulong SetInterruptHLine(ulong line);			/* Pass in a FIELD line (returns previous setting) */
ulong GetInterruptHLine(void);			

ulong SetDisplayInterrupts(ulong mask);			/* returns int enable state AFTER enable */
ulong ClearDisplayInterrupts(ulong mask);		/* returns int enable state BEFORE disable */
ulong GetDisplayInterruptEnables(void);

uchar *DisplayPage(ulong page);					/* 0 or 1, for Primary or Secondary page */
ulong GetCurrentDisplayPage(void);				/* return whichever is selected */

uchar *GetDisplayPageBase(ulong page);			/* 0: Page 0, ODD field */
												/* 1: Page 1, ODD field */
												/* 2: Page 0, EVEN field */
												/* 3: Page 1, EVEN field */
												
ulong GetDisplayRowBytes(void);		

ulong GetDisplayWidth();
ulong GetDisplayHeight();

void SetDisplayOverscanColor(ulong color,long scanLine);		/* color is: xYCrCb */

void SetDisplayActiveAreaOffset(ulong x, ulong y);
void GetDisplayActiveAreaOffset(ulong *x, ulong *y);



#ifdef SPOT1
void SPOT1ShiftHack(void);
#endif



#ifdef __cplusplus
}
#endif

#endif