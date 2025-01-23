#include "WTVTypes.h"
#include "BoxBoot.h"
#include "BoxUtils.h"
#include "BoxHWEquates.h"
#include "BoxAbsoluteGlobals.h"
#include "SystemGlobals.h"
#include "Debug.h"
#include "Interrupts.h"
#include "HWRegs.h"

#include "HWDisplay.h"
#include "HWRegs.h"
#include "IIC.h"

#include "boxansi.h"


#ifdef SPOT1
#include "System.h"
#endif

#define	kScreenWidthAlign			15			/* For efficiency, align on 16 pixel boundary ( 2 bytes/pixel ) */

#define	kScreenBufAlign				31			/* SPOT3 wants video on a 32B boundary. */
												/* (Just do it for everyone.) */
#define	kScreenBufferSegment		0x80000000	/* access screen buffer in cached seg */
#define	kBytesPerPixel				2

#define	kVidQSize					16
#define	kVidQMask					0xf

#define kNTSCScreenWidth			((560 + kScreenWidthAlign) & ~kScreenWidthAlign)
#define kNTSCScreenHeight			420
#define kNTSCMaxHActive				640
#define kNTSCMaxVActive				420

#define kPALScreenWidth				((624 + kScreenWidthAlign) & ~kScreenWidthAlign)
#define kPALScreenHeight			480
#define kPALMaxHActive				768
#define kPALMaxVActive				480


/*	position of raster */

#define		kVideoHStart			159
#define		kVideoVStart			32

/* scanline where vblank starts */

#define		kLastActiveLine			484

/* black pixel values */

#define		kBlackYUV 				0x108080
#define		kBlackYUYV 				0x10801080


	
#define	SPLIT_FIELDS	true // EMAC: removed to fix video (defined(SPOT1) || !defined(HW_INTERLACING)) 

#define	INTERLACE_VIDEO_DMA_BUG	(SPOTVersion() == kSPOT3 &&  HW_INTERLACING)



#define	EXTRA_SCANLINES_FOR_FILTER		/* add lines at top and bottom for flicker filter */



typedef struct {
	ulong	address;
	ulong	data;
} vidqelem;


ulong gShiftAmount = 0;
ulong gShifts = 0;

ulong gVBump = 0;
ulong gRefreshSkips = 0;


static void QVidWrite(ulong address, ulong data);

void Init7187(void);

/* private globals - used by AInterrupt.s */

ulong				oddBuffer = 0;		/* these point to the fields for the active page */


#if		SPLIT_FIELDS
ulong				evenBuffer = 0;
static ulong 		evenBuffer0 = 0;
static ulong		evenBuffer1 = 0;
#endif




static ulong		oddBuffer0 = 0;		/* these point to the fields for the 2 pages */
static ulong		oddBuffer1 = 0;

ulong				dmaSize = 0;

ulong				gScreenHeight = 0;
ulong				gScreenWidth = 0;
ulong 				gScreenMaxHActive = 0;
ulong				gScreenMaxVActive = 0;


vidqelem			vidq[kVidQSize];
ulong				vidqhead;
ulong				vidqtail;

void
SetDisplayMode(DisplayMode mode)
{
	switch (mode)
	{
	case kDisplayModeNTSC:
		gScreenWidth = kNTSCScreenWidth;
		gScreenHeight = kNTSCScreenHeight;
		gScreenMaxHActive = kNTSCMaxHActive;
		gScreenMaxVActive = kNTSCMaxVActive;
		break;
	case kDisplayModePAL:
		gScreenWidth = kPALScreenWidth;
		gScreenHeight = kPALScreenHeight;
		gScreenMaxHActive = kPALMaxHActive;
		gScreenMaxVActive = kPALMaxVActive;
		break;
	}
}

ulong
GetDisplayWidth()
{
	return gScreenWidth;
}

ulong
GetDisplayHeight()
{
	return gScreenHeight;
}



#ifdef	HARDWARE



/* Set starting line, interrupt line, init settings */

void InitDisplay(ulong flags)
{


	ulong datatop;
	ulong fieldBufferSize;
	ulong fcntl = 0;

	vidqhead = vidqtail = 0;	/* index into q of writes to occur at vbl time */

	/* Put the field buffers right above the data space in low RAM.
	   The system heap will be placed right after the field buffers.
	   The field buffers are placed in LOW memory to ensure that they 
	   live in local RAM (not expansion RAM, which can be slower).
	 */

	datatop = (ulong)kDataSegmentBase + 
					((((ROMHeader *)ROMBASE)->data_length) << 2) +
					((((ROMHeader *)ROMBASE)->bss_length) << 2);
						
	datatop = ((datatop + kScreenBufAlign) & ~kScreenBufAlign);	

	/* now calculate field buffer base pointers and sizes. */

	if ( *(volatile ulong *)kROMSysConfigReg & kNTSCBit )
	{	
		gScreenWidth = kNTSCScreenWidth;
		gScreenHeight = kNTSCScreenHeight;
		gScreenMaxHActive = kNTSCMaxHActive;
		gScreenMaxVActive = kNTSCMaxVActive;
	}
	else
	{	
		gScreenWidth = kPALScreenWidth;
		gScreenHeight = kPALScreenHeight;
		gScreenMaxHActive = kPALMaxHActive;
		gScreenMaxVActive = kPALMaxVActive;
	}
	
	fieldBufferSize = ( (gScreenWidth * kBytesPerPixel) * (gScreenHeight >> 1) );
	fieldBufferSize = ((fieldBufferSize + kScreenBufAlign) & ~kScreenBufAlign);		


	datatop = ( datatop + kScreenBufAlign) &  ~kScreenBufAlign;

	if (	INTERLACE_VIDEO_DMA_BUG )
		datatop += 2*(gScreenWidth * kBytesPerPixel);

#ifdef	EXTRA_SCANLINES_FOR_FILTER
	/* extra scanline at top for filtering */
	datatop += (gScreenWidth * kBytesPerPixel);
#endif


	oddBuffer0 	= datatop;
#if		SPLIT_FIELDS
	evenBuffer0 = oddBuffer0 + fieldBufferSize;
#endif
	oddBuffer1 	= evenBuffer0 + fieldBufferSize;

#ifdef	EXTRA_SCANLINES_FOR_FILTER

	/* extra scanline at bottom for flicker filtering */
	
	oddBuffer1 	+= (gScreenWidth * kBytesPerPixel);
	datatop 	+= 2*(gScreenWidth * kBytesPerPixel);
#endif

#if		SPLIT_FIELDS
	evenBuffer1 = oddBuffer1 + fieldBufferSize;
#endif
	datatop		+= (fieldBufferSize*4);					/* system heap starts after 4 field bufs */

	WRITE_AG(agSysHeapBase, (char*)datatop);		

	EraseDisplay(0);
	EraseDisplay(1);
	
	// EMAC: reversed these to fix video.
	oddBuffer = evenBuffer0;									/* init to page 0 */
#if		SPLIT_FIELDS
	evenBuffer = oddBuffer0;
#endif

	*(volatile ulong *)kVidBlnkColReg = kBlackYUV;			/* black border */

#ifdef P7187
	Init7187();
	fcntl = kBlankInvert;
#else
	switch( ( *(volatile ulong *)kROMSysConfigReg & 0x600 ) >> 9 )
	{
		case 1: 	/* BT 851 */
			fcntl = kClkSync;
			break;
		case 2:	 	/* BT 852 */
			fcntl = kBT852Mode | kBlankColorEnable;
			break;
		case 3:		/* P 7187 or BT 866 */
			/* 
			FIXME - need to handle gracefully if we think 7187 is there but isn't
			Init7187();
			fcntl = kBlankInvert | kCrCbSwap;
			 */
			break;
		default:
			break;
	}
#endif

	*(volatile ulong *)kVidHIntLineReg 	= kLastActiveLine/2+2;
	
	*(volatile ulong *)kVidFCntlReg 	= fcntl | kInterlaceMode; 	/* must enable kInterlaceMode to get VBLs */	
}


void KillDisplay(void)					/* Queued vidunit writes are NOT flushed */
{
ulong temp;
ulong ticks;

	temp = *(volatile ulong *)kVidFCntlReg;		/* unlike other vidunit accesses, these are IMMEDIATE */
	temp &= ~kVidEnable;

	QVidWrite(kVidFCntlReg, temp);
	ticks = gSystemTicks + 2;
	while((volatile ulong)gSystemTicks < ticks)
		;
	QVidWrite(kVidDMACntlReg, 0);
}


void EnableInterlacedDisplay(void)
{
ulong ticks;
ulong fcntl;
ulong height;

	*(volatile ulong *)kVidHStartReg 	= kVideoHStart;				/* should be based on used pref */
	*(volatile ulong *)kVidHSizeReg 	= gScreenWidth;
	
	*(volatile ulong *)kVidVStartReg 	= kVideoVStart;				/* should be based on user pref */
	
	height = (gScreenHeight >> 1);
	
#ifdef	EXTRA_SCANLINES_FOR_FILTER
	height += 1;
#endif
	*(volatile ulong *)kVidVSizeReg 	= height;

	dmaSize = gScreenWidth * height * kBytesPerPixel;	// one field

	/* assume this will be fixed in later spots */
	
	if (	INTERLACE_VIDEO_DMA_BUG ) {
		/* 
			due to a SPOT3 HW Interlacing bug, we need to offset the buffer and count to shift the 
			video up a line so that the odd and even fields display correctly.
		*/
	
		*(volatile ulong *)kVidNStartReg 	= oddBuffer - 2*(gScreenWidth*kBytesPerPixel);	
		*(volatile ulong *)kVidNSizeReg 	= dmaSize<<1;			
	}
	else {
		*(volatile ulong *)kVidNStartReg 	= oddBuffer;	
		*(volatile ulong *)kVidNSizeReg 	= dmaSize;			
	}
	
	
	*(volatile ulong *)kVidDMACntlReg 	= kDMA_Interlace_Enable;
	*(volatile ulong *)kVidDMACntlReg 	= (kDMA_Interlace_Enable | kDMA_NV | kDMA_NVF);
	*(volatile ulong *)kVidDMACntlReg 	= (kDMA_Interlace_Enable | kDMA_Enable | kDMA_NV | kDMA_NVF);

	/* wait for DMA engine to crank up */
	{
		ulong xxx;
		for (xxx=0;xxx!=32;xxx++)
			*(volatile ulong *)kVidCCntReg;
	}
	
	fcntl = *(volatile ulong *)kVidFCntlReg;
	*(volatile ulong *)kVidFCntlReg = (kVidEnable | fcntl);
	
	/* assume this will be fixed in later spots */
	
	if(SPOTVersion() == kSPOT3) {

		/* once video has cranked up, wait a couple VBLs and set kVidNSizeReg to normal value */

		ticks = gSystemTicks + 2;
		while((volatile ulong)gSystemTicks < ticks)
			;
	
		*(volatile ulong *)kVidNSizeReg = dmaSize;
	
		/* this is sick - this hack only works on systems greater than 37 MHz */
		if( (READ_AG(agBusSpeed) >= 37000000)) {
			Message(("EnableInterlacedDisplay: Doing wacky SPOT3 shift hack (EMAC: no you're not)"));
			gVBump++;
		}
	}
}


void EnableDisplay(void)	/* Any pending queued vidunit writes will complete if ints are enabled */
{
ulong fcntl;

#ifdef HW_INTERLACING
	if(SPOTVersion() == kSPOT1)
#else
	if(1)
#endif
	{
		*(volatile ulong *)kVidNStartReg 	= oddBuffer;		
		*(volatile ulong *)kVidNSizeReg		= ( (gScreenWidth * kBytesPerPixel) * (gScreenHeight >> 1) );			
		*(volatile ulong *)kVidHStartReg	= kVideoHStart;				/* should be based on used pref */
		*(volatile ulong *)kVidHSizeReg		= gScreenWidth;

		*(volatile ulong *)kVidVStartReg	= kVideoVStart;				/* should be based on user pref */
		*(volatile ulong *)kVidVSizeReg		= (gScreenHeight >> 1);

		*(volatile ulong *)kVidDMACntlReg	= (kDMA_NV | kDMA_NVF);
		*(volatile ulong *)kVidDMACntlReg	= (kDMA_Enable | kDMA_NV | kDMA_NVF);

		/* wait for DMA engine to crank up */
		{
		ulong xxx;
			for (xxx=0;xxx!=32;xxx++)
				*(volatile ulong *)kVidCCntReg;
		}

		fcntl = *(volatile ulong *)kVidFCntlReg;
		*(volatile ulong *)kVidFCntlReg = (kVidEnable | fcntl);

		gVideoShift = 0; /* for spot1 shift hack */

		Message(("EnableDisplay : using software interlacing"));
	}
	else	/* SPOT2 or SPOT3 */
	{
		EnableInterlacedDisplay();
		Message(("EnableDisplay : using hardware interlacing"));
	}
}


#if !defined(BOOTROM) && !defined(APPROM)
void SetDisplayOptions(ulong flags)	
{
	QVidWrite(kVidFCntlReg, flags);
}
#endif


#if !defined(BOOTROM) && !defined(APPROM)
ulong GetDisplayOptions(void)
{
	return *(volatile ulong *)kVidFCntlReg;
}
#endif


#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
ulong SetInterruptHLine(ulong line)		
{
ulong oldHLine;

	oldHLine = *(volatile ulong *)kVidHIntLineReg;

	*(volatile ulong *)kVidHIntLineReg = line;				/* go ahead and pound this one */
	
	return oldHLine;
}
#endif


ulong SetDisplayInterrupts(ulong mask)
{
	*(volatile ulong *)kVidIntEnableSetReg = mask;			/* go ahead and pound this one */

	return *(volatile ulong *)kVidIntEnableSetReg;
}


#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
ulong ClearDisplayInterrupts(ulong mask)
{
ulong oldInts;

	oldInts = *(volatile ulong *)kVidIntEnableSetReg;

	QVidWrite(kVidIntEnableClearReg, mask);

	return(oldInts);
}
#endif


#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
ulong GetDisplayInterruptEnables(void)
{
	return *(volatile ulong *)kVidIntEnableSetReg;
}
#endif


#if !defined(BOOTROM) && !defined(APPROM)
uchar *DisplayPage(ulong page)			/* ����this should really wait for an odd field vbl */
{
	if(page)
	{
		oddBuffer = oddBuffer1;								
#if		SPLIT_FIELDS
		evenBuffer = evenBuffer1;
#endif
		return (uchar *)(kScreenBufferSegment + oddBuffer0);
	}
	else
	{
		oddBuffer = oddBuffer0;							
#if		SPLIT_FIELDS
		evenBuffer = evenBuffer0;
#endif
		return (uchar *)(kScreenBufferSegment + oddBuffer1);
	}
}
#endif


#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
ulong GetCurrentDisplayPage(void)				/* return whichever is selected */
{
	if(oddBuffer == oddBuffer1)
		return 1;
	else
		return 0;
}
#endif


uchar *GetDisplayPageBase(ulong page)
{
	uchar *base = (uchar *)kScreenBufferSegment + oddBuffer0;
	
	switch(page)
	{
	case 0:
		base = (uchar *)(kScreenBufferSegment + oddBuffer0);
		break;
	case 1:
		base = (uchar *)(kScreenBufferSegment + oddBuffer1);
		break;
#if		SPLIT_FIELDS
	case 2:
		base = (uchar *)(kScreenBufferSegment + evenBuffer0);
		break;
	case 3:
		base = (uchar *)(kScreenBufferSegment + evenBuffer1);
		break;
#endif
	}
#ifdef	EXTRA_SCANLINES_FOR_FILTER
	base += (gScreenWidth * kBytesPerPixel);
#endif
	return base;
}


ulong GetDisplayRowBytes(void)
{
	return (gScreenWidth * kBytesPerPixel);
}


long gOverscanColorLineStart = 0;
long gOverscanColor = 0;
long gOverscanDefaultColor = 0;

void SetDisplayOverscanColor(ulong color,long startLine)		
{

//	startLine = 0;		// for now, don't do splits since interrupt latency is too high and it jitters badly
	if ( startLine == 0 ) {
		gOverscanDefaultColor = color;
		QVidWrite(kVidBlnkColReg, color);
	}
	else {
		startLine /= 2;
		if ( startLine < kLastActiveLine/2 ) {
			gOverscanColorLineStart = startLine;
			gOverscanColor = color;
		}
	}
}





#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void SetDisplayActiveAreaOffset(ulong x, ulong y)	
{
	if (x > (gScreenMaxHActive-gScreenWidth))	/* pin x & y */
		x = (gScreenMaxHActive-gScreenWidth);
		
	x |= 1;										/* x must be ODD */
	
	if (y > (gScreenMaxVActive-gScreenHeight))
		y = (gScreenMaxVActive-gScreenHeight);
		
	y &= ~1;									/* y must be EVEN */

	QVidWrite(kVidHStartReg, x);
	QVidWrite(kVidVStartReg, y);
}
#endif


#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void GetDisplayActiveAreaOffset(ulong *x, ulong *y)
{
	*x = *(volatile ulong *)kVidHStartReg;
	*y = *(volatile ulong *)kVidVStartReg;
}
#endif


void EraseDisplay(ulong select)
{
ulong *p;
ulong iii;
ulong wordsize;

	wordsize = ( (gScreenWidth * kBytesPerPixel) * (gScreenHeight >> 1) );
	
	wordsize >>= 2;		/* bytes -> words */
	
	p = (ulong *)GetDisplayPageBase(select);
	
	for(iii=0;iii!=wordsize;iii++)
		*p++ = kBlackYUYV;

	p = (ulong *)GetDisplayPageBase(select + 2);
	
	for(iii=0;iii!=wordsize;iii++)
		*p++ = kBlackYUYV;
}


void QVidWrite(ulong address, ulong data)
{
ulong headcopy;
	
	headcopy = vidqhead;
	
	headcopy++;
	headcopy &= kVidQMask;
	
	while(headcopy == vidqtail)
		;							/* spinloop if q fills up */
		
	vidq[headcopy].address = address;
	vidq[headcopy].data = data;
	
	vidqhead = headcopy;
}

#ifdef SPOT1
#ifdef APPROM
extern System* gSystem;
void SPOT1ShiftHack(void)
{
    return;
    /*ulong ticks;
    if(gSystem->GetIsOn() && gVideoShift) {
        KillDisplay();
        ticks = gSystemTicks + 4;
        while(gSystemTicks < ticks)
            ;
        EnableDisplay();
    }*/
}
#endif
#endif

#define k7187Addr 0x8c

void Init7187(void)
{
	int i;
	
	for(i=0;i<=0x39;i++)
		IICWrite(k7187Addr,i,0x00);

	IICWrite(k7187Addr,0x3a,0x0f);
	
	for(i=0x3b;i<=0x41;i++)
		IICWrite(k7187Addr,i,0x00);

	/* color bars */
	IICWrite(k7187Addr,0x42,0x6b);
	IICWrite(k7187Addr,0x43,0x00);
	IICWrite(k7187Addr,0x44,0x00);
	IICWrite(k7187Addr,0x45,0x52);
	IICWrite(k7187Addr,0x46,0x90);
	IICWrite(k7187Addr,0x47,0x12);
	IICWrite(k7187Addr,0x48,0x2a);
	IICWrite(k7187Addr,0x49,0x26);
	IICWrite(k7187Addr,0x4a,0x90);
	IICWrite(k7187Addr,0x4b,0x11);
	IICWrite(k7187Addr,0x4c,0xb6);
	IICWrite(k7187Addr,0x4d,0xa2);
	IICWrite(k7187Addr,0x4e,0xea);
	IICWrite(k7187Addr,0x4f,0x4a);
	
	IICWrite(k7187Addr,0x50,0x5e);
	IICWrite(k7187Addr,0x51,0xd1);
	IICWrite(k7187Addr,0x52,0xda);
	IICWrite(k7187Addr,0x53,0x70);
	IICWrite(k7187Addr,0x54,0xa9);
	IICWrite(k7187Addr,0x55,0x70);
	IICWrite(k7187Addr,0x56,0xee);
	IICWrite(k7187Addr,0x57,0x90);
	IICWrite(k7187Addr,0x58,0x00);
	IICWrite(k7187Addr,0x59,0x00);
	IICWrite(k7187Addr,0x5a,0x1a);
	IICWrite(k7187Addr,0x5b,0x76);
	IICWrite(k7187Addr,0x5c,0xa5);
	IICWrite(k7187Addr,0x5d,0x3c);
	IICWrite(k7187Addr,0x5e,0x3a);
	IICWrite(k7187Addr,0x5f,0x00);
	
	IICWrite(k7187Addr,0x60,0x00);
	IICWrite(k7187Addr,0x61,0x15);
	IICWrite(k7187Addr,0x62,0xe6);
	IICWrite(k7187Addr,0x63,0x55);
	IICWrite(k7187Addr,0x64,0x55);
	IICWrite(k7187Addr,0x65,0x55);
	IICWrite(k7187Addr,0x66,0x25);
	/* 67 - 6b don't care */
	IICWrite(k7187Addr,0x6c,0x3b);
	IICWrite(k7187Addr,0x6d,0x00);
	IICWrite(k7187Addr,0x6e,0x00);
	IICWrite(k7187Addr,0x6f,0x00);
	
	IICWrite(k7187Addr,0x70,0xc0);
	IICWrite(k7187Addr,0x71,0x00);
	IICWrite(k7187Addr,0x72,0x00);
	IICWrite(k7187Addr,0x73,0x00);
	IICWrite(k7187Addr,0x74,0x00);
	IICWrite(k7187Addr,0x75,0x00);
	IICWrite(k7187Addr,0x76,0x00);
	IICWrite(k7187Addr,0x77,0x17);
	IICWrite(k7187Addr,0x78,0x2a);
	IICWrite(k7187Addr,0x79,0x06);
	IICWrite(k7187Addr,0x7a,0x0c);
	IICWrite(k7187Addr,0x7b,0x12);
	IICWrite(k7187Addr,0x7c,0x03);
	IICWrite(k7187Addr,0x7d,0x22);
	
}



#endif
