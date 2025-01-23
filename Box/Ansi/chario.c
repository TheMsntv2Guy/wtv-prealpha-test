/* ===========================================================================
 *	chario.c
 *
 *	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
 * ======================================================================== */

/*
	File:		chario.c

	Contains:	xxx put contents here xxx

	Written by:	joe britt

	Copyright:	© 1995 by WebTV, Inc., all rights reserved.

	Change History (most recent first):

		 <1>	 9/19/95	JOE		first checked in

	To Do:
*/

#include "Headers.h"
#include "SystemGlobals.h"

#include "HWRegs.h"
#include "BoxUtils.h"
#include "HWDisplay.h"
#include "HWKeyboard.h"
#include "ObjectStore.h"
#include "chario.h"

#include "CrashLogC.h"

#ifdef GOOBER
#include "GooberUtils.h"
#endif

#include "debugfont.c"

Boolean gCrashLogPrintfs = true;

Boolean gPutCharTV = false;

#ifdef PRINTF_ENABLED
Boolean gPrintEnable = true; 
#else
Boolean gPrintEnable = false; 
#endif

/* NOTE: putchar() & getchar() only use the terminal (serial debug) port! */

int putchar(int c)
{
	/* Always log the character in the little crashlog printf buffer, 
	   unless we're in the debugger.
	*/

	if(gCrashLogPrintfs)
	{
		CrashLogStruct *cls = (CrashLogStruct *)kCrashLogBase;
		ulong iii;
		
		if(cls->crashSig != kValidCrashLogSig)		/* don't capture if there's a valid crashlog */
		{
			iii = cls->printfBufTail;
			
			iii &= kPrintfBufMask;					/* make sure we're sane */
			cls->printfBuf[iii] = c;
			iii++;
			iii &= kPrintfBufMask;
			
			cls->printfBufTail = iii;
		}
	}
	
	/* Now decide if we need to send it anyplace else */

	if(gPutCharTV)
		TV_putchar(c);

	if (!gPrintEnable)
		return c;
	
#ifdef SERIAL_DEBUG
	if(!gPutCharTV)
	{
		BangSerial(c);
		if (c == '\n')
			BangSerial('\r');
	}
#endif

	return c;
}


/* returns 0 if no char avail */

int getchar(void)
{
	return(PollHWKeyboard());
}


void getstring(uchar *s,ulong length,ulong timeout)
{
uchar 	c = 0;
ulong 	curTime;
int 	i;

	length--;
	
	curTime = gSystemTicks;
	curTime += timeout;
	
	while((c != '\n') && (c != '\r') && length)
	{
		do
		{
#ifdef GOOBER
			GooberHeartbeat();		/* for the debugger, to let us know that we're alive */
#endif		
			/* refresh RAM on SPOT3 */
			if(SPOTVersion() == kSPOT3)
				for(i=0;i<1024;i++)
					*(volatile ulong*)kMemCmdReg = 0x48000000;

			c = getchar();

			if(timeout)
				if(gSystemTicks > curTime)
				{
					*s = 0;			/* term the string & bail */
					return;
				}
		}
		while(!c);					/* buzz til a char comes in */
	
		putchar(c);					/* echo it to the terminal */
		
		if (c != 0x08)				/* BS? */
		{
			*s++ = c;
			length++;
		}
		else
			s--;
			
		length--;
	}

	*s = 0;	
}



/* 	-----------------------------------------------------------------------
	Goober Parallel Port Text
*/

#ifdef GOOBER

void ParallelWaitStrobe(uchar assert)
{
	ulong timeout = 0;
	
	if(assert) {
		while((*(volatile ulong*)kGooberParallelCntrlIn & kStrobeBit) == 0 ) 
		{
			if(++timeout == 0x1000000)
				break;
		}
	} else {
		while((*(volatile ulong*)kGooberParallelCntrlIn & kStrobeBit) == kStrobeBit ) 
		{
			if(++timeout == 0x1000000)
				break;
		}
	}
}

void ParallelAck(uchar assert)
{
	if(assert) {
		*(volatile ulong*)kGooberParallelCntrlOut = (kAckBit | kDirBit);
	} else {
		*(volatile ulong*)kGooberParallelCntrlOut = (kDirBit);
	}
}

void ParallelWrite(uchar byte)
{
	*(volatile ulong*)kGooberParallelOut = byte;
}

void Parallel_putchar(int c)
{
	ParallelWaitStrobe(true);
	ParallelWrite((uchar)c);
	ParallelAck(true);
	ParallelWaitStrobe(false);
	ParallelAck(false);
}

#endif /* GOOBER */




/* 	-----------------------------------------------------------------------
	TV Text
*/

ulong	currow = 0;
ulong	curcol = 0;

#define	kNumCols	70
#define	kNumRows	26

uchar	tvtextbuf[kNumCols][40];


static void TVTab(void);

void TVTab(void)
{
ulong iii;
	
	iii = 0;
	
	if (curcol < 13) 
		iii = 13-curcol;
	else 
		if (curcol < 25)
			iii = 25-curcol;
		else
			if (curcol < 32)
				iii = 32-curcol;

	while(iii--)
	{
		tvtextbuf[curcol][currow] = ' ';
		curcol++;
	}
}

void TV_putchar(int c)
{
ulong iii;

	switch (c)
	{
		case '\t':
				TV_paintchar(' ',curcol,currow);		/* erase cursor */
				TVTab();
				break;
		
		case '\n':
				TV_paintchar(' ',curcol,currow);		/* erase cursor */
				for(iii=curcol;iii!=kNumCols;iii++)
					tvtextbuf[iii][currow] = ' ';
				curcol = 0;
				currow++;
				break;

		case 0x08:
				TV_paintchar(' ',curcol,currow);		/* erase cursor */
				if (curcol)								/* back up (to next line, if necessary) */
					curcol--;
				else
				{
					curcol = kNumCols-1;
					if (currow)
						currow--;
				}
				tvtextbuf[curcol][currow] = ' ';		/* change old prev char in buf to space */
				break;									/* new cursor drawn at fn end */
								
		default:		
				if ((c < 0x20) || (c > 0x7f))			/* toss all other nonprinters */
					return;

				tvtextbuf[curcol][currow] = c;
				TV_paintchar(c,curcol,currow);	
				curcol++;
				break;
	}
	
	
	if (curcol == kNumCols)
	{
		curcol = 0;
		currow++;
	}
	
	
	if (currow == kNumRows)		/* do we need to scroll? */
	{
		for(currow=0;currow!=(kNumRows-1);currow++)
			for(curcol=0;curcol!=kNumCols;curcol++)
			{
				tvtextbuf[curcol][currow] = tvtextbuf[curcol][currow + 1];
				TV_paintchar(tvtextbuf[curcol][currow],curcol,currow);	
			}

		for(curcol=0;curcol!=kNumCols;curcol++)
		{
			tvtextbuf[curcol][currow] = ' ';
			TV_paintchar(' ',curcol,currow);	
		}

		curcol = 0;
	}

	TV_paintchar(0x7f,curcol,currow);		/* paint cursor */
	
}




void TV_paintchar(uchar c, ulong col, ulong row)
{
ulong *scrny;
ulong fieldOffset;
ulong rowwords;
uchar *glyph;
uchar rawrow;
ulong iii,jjj;
ulong temp;

	if ((c < 0x20) || (c > 0x7f))
		return;
		
	c -= 0x20;
	
	scrny = (ulong *)GetDisplayPageBase(0);
	
	if (SPOTVersion() == kSPOT1)
	{
		fieldOffset = (GetDisplayPageBase(2) - GetDisplayPageBase(0)) >> 2;
		
		rowwords = GetDisplayRowBytes();				
		scrny += ((col << 3) << 1) >> 2;			/* each char is 8 pixels wide, each pixel is 2 bytes */
		scrny += (((row << 3) * rowwords)) >> 2;	/* each char is 16 pixels tall (but shift 3 'cos interlaced) */
		rowwords >>= 2;		
	
		glyph = (uchar*) debugfont;
		glyph += (c << 4);
		
		for(iii=0;iii!=16/2;iii++)
		{
			rawrow = *glyph++;
	
			scrny += fieldOffset;
			
			for(jjj=0;jjj!=4;jjj++)
			{
				if(rawrow & 0x80)
					temp = 0xce801080;
				else
					temp = 0x10801080;
	
				rawrow <<= 1;
					
				if(rawrow & 0x80)
				{
					temp &= 0xFFFF00FF;
					temp |= 0x0000ce00;
				}
		
				*scrny++ = temp;
				
				rawrow <<= 1;
			}	
	
			scrny -= fieldOffset;
			scrny -= ((8*2) >> 2);
			
			rawrow = *glyph++;
	
			for(jjj=0;jjj!=4;jjj++)
			{
				if(rawrow & 0x80)
					temp = 0xce801080;
				else
					temp = 0x10801080;
	
				rawrow <<= 1;
					
				if(rawrow & 0x80)
				{
					temp &= 0xFFFF00FF;
					temp |= 0x0000ce00;
				}
		
				*scrny++ = temp;
				
				rawrow <<= 1;
			}	
	
			scrny -= ((8*2) >> 2);
			scrny += rowwords;
		}
	}
	else
	{
		rowwords = GetDisplayRowBytes();				
		scrny += ((col << 3) << 1) >> 2;			/* each char is 8 pixels wide, each pixel is 2 bytes */
		scrny += (((row << 4) * rowwords)) >> 2;	/* each char is 16 pixels tall */
		rowwords >>= 2;		
	
		glyph = (uchar*) debugfont;
		glyph += (c << 4);
		
		for(iii=0;iii!=16;iii++)
		{
			rawrow = *glyph++;
				
			for(jjj=0;jjj!=4;jjj++)
			{
				if(rawrow & 0x80)
					temp = 0xce801080;
				else
					temp = 0x10801080;
	
				rawrow <<= 1;
					
				if(rawrow & 0x80)
				{
					temp &= 0xFFFF00FF;
					temp |= 0x0000ce00;
				}
		
				*scrny++ = temp;
				
				rawrow <<= 1;
			}	
	
			scrny -= ((8*2) >> 2);
			scrny += rowwords;
		}
	}
}

