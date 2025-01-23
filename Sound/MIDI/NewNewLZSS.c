/* -------------------------------------------------------------------------------- *
 *   Authors:	Justin V. McCormick & Mick Foley
 *  Revision:	0.0 89/04/06 - H. Okumura, original public domain version (MSDOS)
 *			 	1.0 91/04/12 - M. Foley, rewritten to work, new encoder.
 *				2.0 91/04/14 - J. McCormick, cleaned, optimized.
 *				2.1 91/07/23 - S. Shumway, fixed bugs in findLongestMatch, doLZSSEncode.
 *				3.0 91/07/23 - S. Shumway, cleaned up a lot.
 *				4.0	94/02/14 - Decode routines Converted to 'C'
**	1/18/96		Spruced up for C++ extra error checking
**	2/18/96		Removed Mac stuff, and added X_API.h
 *
 * Haruhiko's original header:
 *	LZSS.C -- A Data Compression Program
 *	4/6/1989 Haruhiko Okumura
 *	Use, distribute, and modify this program freely.
 *	Please send me your improved versions.
 *		PC-VAN		SCIENCE
 *		NIFTY-Serve	PAF01022
 *		CompuServe	74050,1022
 * -------------------------------------------------------------------------------- */
#include "X_API.h"

#if X_HARDWARE_PLATFORM == X_MACINTOSH
	#include <Types.h>
	#include <Memory.h>
#endif


#define TOKENBITS	4								/* number of bits used for token size	*/
#define OFFSETBITS	12								/* number of bits used for token offset	*/
#define CODEBITS	(TOKENBITS+OFFSETBITS)			/* total token bits						*/

#define MAXTOKENS	(1 << TOKENBITS)				/* range of token sizes					*/
#define THRESHOLD	((CODEBITS / 8) + 1)			/* minimum match length					*/
#define MAXMATCH	(THRESHOLD + MAXTOKENS - 1)		/* maximum match length					*/
#define LOOKBACK	(1 << OFFSETBITS)				/* size of lookback buffer				*/


void				LZSSCompress(unsigned char * pSource, long size, unsigned char * pDest, long* pSize);
void				LZSSUncompress(unsigned char * pSource, long size, unsigned char * pDest, long* pSize);
void				LZSSDeltaCompress(unsigned char * pSource, long size, unsigned char * pDest, long* pSize);
void				LZSSDeltaUncompress(unsigned char * pSource, long size, unsigned char * pDest, long* pSize);

static short int	findLongestMatch(unsigned char * theData, long dataLen, long patternStart, short* codeWord);
static long			doLZSSEncode(unsigned char * srcBuffer, long srcSize, unsigned char * dstBuffer);
static void			doLZSSDecode(unsigned char * srcBuffer, long srcSize, unsigned char * dstBuffer, long dstSize);
static void			DeltaSample(unsigned char * pData, long size);
static void			UnDeltaSample(unsigned char * pData, long size);

#if X_HARDWARE_PLATFORM == X_MACINTOSH
Handle				DecompressHandle(Handle theHandle);
Handle				CompressHandle(Handle theHandle);
#endif


#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void LZSSCompress(unsigned char * pSource, long size, unsigned char * pDest, long * pSize)
{
	*pSize = doLZSSEncode(pSource, size, pDest);
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void LZSSUncompress(unsigned char * pSource, long size, unsigned char * pDest, long* pSize)
{
	doLZSSDecode(pSource, size, pDest, *pSize);
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void LZSSDeltaCompress(unsigned char * pSource, long size, unsigned char * pDest, long* pSize)
{
	DeltaSample(pSource, size);
	*pSize = doLZSSEncode(pSource, size, pDest);
	UnDeltaSample(pSource, size);
}
#endif

void LZSSDeltaUncompress(unsigned char * pSource, long size, unsigned char * pDest, long* pSize)
{
	doLZSSDecode(pSource, size, pDest, *pSize);
	UnDeltaSample(pDest, *pSize);
}




#if X_HARDWARE_PLATFORM == X_MACINTOSH
/* -------------------------------------------------------------------------------- *
 * Function: findLongestMatch
 *
 * Description:
 *	This routine finds the longest match of the string starting at patternStart
 *  in the previous LOOKBACK characters.  If it finds a match of from THRESHOLD bytes
 *	to MAXMATCH bytes, it encodes the position as a code word and places the value in 
 *	codeWord and returns the number of characters matched.
 *	If it does not find a match, it returns 0.
 *
 * Parameters:
 *	unsigned char * theData;			-- the buffer of data to search.
 *	long	dataLen;			-- the size of the buffer
 *	long	patternStart;		-- the buffer position of the pattern string.
 *	short*	codeWord;			-- encoded token: AAAA BBBB BBBB BBBB
 *									A = size of match string - THRESHOLD
 *									B = offset of match string from patternStart - LOOKBACK
 * Result:
 *	int 						-- If a match of THRESHOLD or more characters made,
 *									number of characters matched is returned.
 *								-- If no match is found, 0 is returned.
 *
 * -------------------------------------------------------------------------------- */
#if THINK_C
#define ASM_findLongestMatch		1

static short int findLongestMatch(unsigned char * theData, long dataLen, long patternStart, short* codeWord)
{
	asm 68000
	{
				MOVEM.L		D2-D6/A2-A3,-(SP)		;Save all registers

				MOVEQ		#0,D4					;Initial match length

	;Start scanning up to 4096 characters back in buffer
				MOVE.L		#LOOKBACK,D0
				MOVE.L		patternStart,D1			;d1 = patternStart
				CMP.L		D0,D1					;Is current pos >= 4096?
				BGE.S		@1						;Yep, can go all the way back
				MOVE.L		D1,D2					;else count = patternStart, start at beginning of buffer
				BRA.S		@2
	@1:			MOVE.L		D0,D2					;count = 4096, go back 4k in buffer
	@2:

	;Set how far ahead we can scan without falling off end of buffer
				MOVEQ		#MAXMATCH,D3			; max match length
				MOVE.L		dataLen,D0				; D0 = dataLen - patternStart = howFarToEOF
				SUB.L		D1,D0
				CMP.L		D2,D0					; MIN(count, (dataLen - patternStart))
				BLT.S		@min
				MOVE.L		D2,D0
	@min:		CMP.L		D3,D0					; D3 = MIN(18, count, howFarToEOF)
				BGE.S		@3
				MOVE.L		D0,D3
	
	; Set the buffer pointers
	@3:			MOVEA.L		theData,A0
				MOVEA.L		A0,A3
				ADDA.L		D1,A3					;a3 = patternPtr = theData + patternStart

				MOVEA.L		A3,A2
				SUBA.L		D2,A2					;a2 = bufferPtr = theData + patternStart - count

				MOVEQ		#0,D6
				MOVE.B		(A3),D6					;d6 = *patternPtr for quick compares

	;Loop until we have checked all the bytes in our scan...
	@flmLoop:	TST.W		D2						;while (count > 0) ...
				BLE.S		@flmDone				; count <= 0, we are done!
				CMP.B		(A2)+,D6				;if (*bufferPtr == *patternPtr) ...
				BEQ.S		@flmMatch				;	see how many bytes match

				SUBQ.W		#1,D2					;count--
				BRA.S		@flmLoop				;do while

	@flmMatch:	MOVEQ		#1,D0					;i = 1
				LEA			1(A3),A1				;a1 = &patternPtr[1]
	@4:			CMP.W		D3,D0					;if (i >= loopMax)
				BGE.S		@5						;	we have reached max match length
				CMPM.B		(A2)+,(A1)+				;if (patternPtr[i] != bufferPtr[i])
		 		BNE.S		@5						;	we have hit the end of the matching bytes
				ADDQ.W		#1,D0					;else i++, bump number of bytes that matched
				BRA.S		@4						;check next pair of bytes

	;If this match length (i) is greater than previous max length, save this as new best length
	@5:			CMP.W		D4,D0					;if (i >= bestMatchLen)
		 		BLE.S		@6						;	nope, bestMatchLen was still better
				MOVE.W		D2,D5					;else bestMatch = count
				MOVE.W		D0,D4					; and bestMatchLen = i
	
	;Now move past the matching bytes in buffer
	@6:			SUBQ.W		#1,A2
				SUB.W		D0,D2					;count -= i
				BRA.S		@flmLoop				;do till count <= 0

	;If the bestMatchLen is greater than coding threshold length (2) then store the coded bytes
	;and return the match length
	@flmDone:	CMP.W		#THRESHOLD,D4			;if (bestMatchLen > THRESHOLD)
		 		BLT.S		@flmNoCode				;	nope, 2 or less bytes matched at patternStart

				MOVE.L		#LOOKBACK,D0			;encode offset
				SUB.W		D5,D0
				
				MOVE.W		D4,D1					;encode length
				SUBQ.W		#THRESHOLD,D1
				LSL.W		#8,D1
				LSL.W		#4,D1

				OR.W		D1,D0					; save code word
				MOVEA.L		codeWord,A0
				MOVE.W		D0,(A0)

				MOVE.W		D4,D0			;return (bestMatchLen)
				BRA.S		@flmReturn

	@flmNoCode:	MOVEQ		#0,D0			;return (0)

	@flmReturn:	MOVEM.L		(SP)+,D2-D6/A2-A3
	}
}
#endif
#endif

#ifdef SIMULATOR
#ifndef ASM_findLongestMatch
static short int findLongestMatch(unsigned char * theData, long dataLen, long patternStart, short* codeWord)
{
unsigned char			*pointer;				// scanning source pointer
register unsigned char *lookback;				// scanning compare pointer
register unsigned char *tPointer;				// scanning source pointer
register unsigned char *tLookback;				// scanning compare pointer
register long			counter;				// number of bytes to scan
register short			maxLen;					// maximum match length
register short			bestLen;				// best match length
register long			bestOff;				// best match offset
register short			length;					// scanning length
long					forward;				// bytes to scan ahead

// find how far to look back
	counter = LOOKBACK;
	if (patternStart < counter)
	{
		counter = patternStart;
	}

// find maximum match length	
	maxLen = MAXMATCH;
	forward = dataLen - patternStart;
	if (forward < counter)
	{
		if (forward < maxLen)
		{
			maxLen = forward;
		}
	}
	else
	{
		if (counter < maxLen)
		{
			maxLen = counter;
		}
	}
	
// initialize
	pointer = theData + patternStart;
	lookback = pointer - counter;
	bestLen = 0;
	bestOff = 0;
	
// scan the lookback buffer
	while (counter > 0)
	{
		length = 0;
		tPointer = pointer;
		tLookback = lookback;
		while (length < maxLen)
		{
			if (*tPointer++ != *tLookback++)
			{
				break;
			}
			length++;
		}
/*
		for (length = 0; length < maxLen; length++)
		{
			if (lookback[length] != pointer[length])
			{
				break;
			}
		}
*/
		if (length > bestLen)
		{
			bestLen = length;
			bestOff = counter;
			if (length >= maxLen)
			{
				break;
			}
		}
		lookback++;
		counter--;
	}

// build code word for a match string
	if (bestLen > THRESHOLD)
	{
		*codeWord = (LOOKBACK - bestOff) | ((bestLen - THRESHOLD) << OFFSETBITS);
	}
	else
	{
		bestLen = 0;
	}
	return bestLen;
}
#endif
#endif


/* -------------------------------------------------------------------------------- *
 * Compress srcBuffer using LZSS technique.
 * -------------------------------------------------------------------------------- */
#ifdef SIMULATOR
static long doLZSSEncode(unsigned char * srcBuffer, long srcSize, unsigned char * dstBuffer)
//	srcBuffer;					/* pointer to uncompressed data */
//	srcSize;					/* size of uncompressed data */
//	dstBuffer;					/* pointer to compressed data */
{
register long			dataPos;					/* buffer position for uncompressed data */
register long			cdataPos;					/* buffer position for compressed data */
register short			codeIndex;					/* index for the code group */
register short			codeNumber;					/* the number 0-7 of the code element */
register unsigned char	flags;						/* the flags byte of a code group */
register unsigned char *dataPtr;					/* pointer to uncompressed data */
register unsigned char *cdataPtr;					/* pointer to compressed data */
short					matchLen;					/* the length of the match found */
short					codeWord;					/* coded token */
unsigned char			codeBuf[16];				/* buffer for the code group */

/* initalize the index variables */
	dataPtr = srcBuffer;
	dataPos = 0;
	
	cdataPtr = dstBuffer;
	cdataPos = 0;

/* until we run out of data */
	while (dataPos < srcSize)
	{
		flags = 0x00;
		codeIndex = 0;

	/* build code blocks in groups of 8 */
		for (codeNumber = 0; codeNumber < 8; codeNumber++)
		{
		/* get the longest match */
			matchLen = findLongestMatch(srcBuffer, srcSize, dataPos, &codeWord);
			if (matchLen)
			{
			/* if we have a match over THRESHOLD characters, encode it */
				codeBuf[codeIndex++] = (unsigned char)(codeWord >> 8);
				codeBuf[codeIndex++] = (unsigned char)(codeWord & 0x00FF);
				dataPos += matchLen;
			}
			else
			{
			/* otherwise, pass the character through and mark the appropiate flags bit */
				codeBuf[codeIndex++] = dataPtr[dataPos];
				flags |= (1 << codeNumber);
				dataPos++;
			}
		/* if we run out of data in the middle of a code group, exit */
			if (dataPos >= srcSize)
				break;
		}
	/* write out the flags byte */
		if (cdataPos < srcSize)
		{
			cdataPtr[cdataPos] = (unsigned char)flags;
		}
		cdataPos++;

	/* write out the 8 (or less) characters/code blocks */
		for (codeNumber = 0; codeNumber < codeIndex; codeNumber++)
		{
			if (cdataPos < srcSize)
			{
				cdataPtr[cdataPos] = codeBuf[codeNumber];
			}
			cdataPos++;
		}
	}

/* If compressing was a net loss, set flag and copy the data verbatim */
	if (cdataPos >= srcSize)
	{
		return(-1);
	}
	else
	{
		return(cdataPos);
	}
}
#endif


//	srcBuffer;					/* pointer to compressed data */
//	srcSize;					/* size of compressed data */
//	dstBuffer;					/* pointer to uncompressed data */
//	dstSize;					/* size of uncompressed data */
/* -------------------------------------------------------------------------------- */
#if ((! powerc) && THINK_C)
#define ASM_doLZSSDecode
static void	doLZSSDecode(unsigned char * srcBuffer, long srcSize, unsigned char * dstBuffer, long dstSize)
{
	asm 68000
	{
				MOVEM.L		D2-D7/A2-A3,-(A7)
				MOVEA.L		srcBuffer,A2
				MOVE.L		srcSize,D6
				MOVEA.L		dstBuffer,A3
				MOVE.L		dstSize,D7
	
	@block:		SUBQ.L		#1,D6						; test source ptr
				BMI.S		@done
				MOVE.B		(A2)+,D3					; get flags
				MOVEQ		#7,D2						; 8 bits
				
	@flag:		LSR.B		#1,D3						; get flag bit
				BCC.S		@rept
				
				SUBQ.L		#1,D6						; test source ptr
				BMI.S		@done
				SUBQ.L		#1,D7						; test dest ptr
				BMI.S		@done
				MOVE.B		(A2)+,(A3)+					; copy literal byte
				DBRA		D2,@flag					; next flag bit
				BRA.S		@block
				
	@rept:		SUBQ.L		#2,D6						; test source ptr
				BMI.S		@done
				MOVE.B		(A2)+,D0					; get code word
				LSL.W		#8,D0
				MOVE.B		(A2)+,D0
				
				MOVE.W		D0,D5						; make offset
				AND.W		#0x0FFF,D5
				MOVEQ		#0x000F,D4					; make count
				ROL.W		#4,D0
				AND.W		D0,D4
				ADDQ.W		#(THRESHOLD-1),D4
				
				MOVEA.L		A3,A0						; make copy ptr
				SUBA.W		#LOOKBACK,A0
				ADDA.W		D5,A0
				
	@copy:		SUBQ.L		#1,D7						; test dest ptr
				BMI.S		@done
				MOVE.B		(A0)+,(A3)+					; copy repeated byte
				DBRA		D4,@copy

				DBRA		D2,@flag					; next flag bit
				BRA.S		@block
				
	@done:		MOVEM.L		(A7)+,D2-D7/A2-A3
	}
}
#endif


#if !defined ASM_doLZSSDecode
static void	doLZSSDecode(unsigned char * srcBuffer, long srcSize, unsigned char * dstBuffer, long dstSize)
{
	register short				temp,temp2,regD5,regD4;
	register unsigned char		regD3;
	register short				regD2;
	register unsigned char *	prevBuffer;

	while(--srcSize >= 0)
	{
		regD3 = *(srcBuffer++);
		regD2 = 7;
		while(regD2 >= 0)
		{
			temp = regD3 & 1;
			regD3 >>= 1;
			if(temp == 0)
			{
				srcSize -= 2;
				if(srcSize < 0) return;
				temp = *(srcBuffer++);					// temp = D0
				temp <<= 8;
				temp |= *(srcBuffer++);
				regD5 = temp & 0x0FFF;
				regD4 = 0x000F;
				temp2 = (temp & 0xF000) >> 12;
				temp <<= 4;
				temp |= temp2;
				regD4 &= temp;
				regD4 += THRESHOLD-1;
				prevBuffer = dstBuffer - LOOKBACK + regD5;
				while(regD4 >= 0)
				{
					if(--dstSize < 0) return;
					*(dstBuffer++) = *(prevBuffer)++;
					regD4--;
				}
			}
			else
			{
				if(--srcSize < 0) return;
				if(--dstSize < 0) return;
				*(dstBuffer++) = *(srcBuffer++);
			}
			regD2--;
		}
	}
}
#endif


#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
/* convert buffer by byte differences */
static void DeltaSample(unsigned char * pData, long size)
{
#if ((! powerc) && THINK_C)
	asm 68000
	{
				MOVEA.L		pData,A0
				MOVE.L		size,D0
				MOVE.B		(A0)+,D1
				BRA.S		@test
	@loop:		MOVE.B 		(A0),D2
				SUB.B		D1,(A0)+
				MOVE.B		D2,D1
	@test		SUBQ.L		#1,D0
				BNE.S		@loop
	}
#else
	register	unsigned char	regD1;
	register	unsigned char	regD2;
	
	regD1 = *(pData++);
	while(--size != 0)								/* taken from 68K, should this be bpl? */
	{
		regD2 = *pData;
		*(pData++) -= regD1;
		regD1 = regD2;
	}
#endif
}
#endif

/* unconvert byte difference buffer */
static void UnDeltaSample(unsigned char * pData, long size)
{
#if ((! powerc) && THINK_C)
	asm 68000
	{
				MOVEA.L		pData,A0
				MOVE.L		size,D0
				MOVE.B		(A0)+,D1
				BRA.S		@test
	@loop:		ADD.B	 	(A0),D1
				MOVE.B	 	D1,(A0)+
	@test:		SUBQ.L		#1,D0
				BNE.S		@loop
	}
#else
	register	unsigned char	regD1;
	
	regD1 = *(pData++);
	while(--size != 0)								/* taken from 68K, should this be bpl? */
	{
		regD1 += *pData;
		*(pData++) = regD1;
	}
#endif
}



#if X_HARDWARE_PLATFORM == X_MACINTOSH
Handle CompressHandle(Handle theHandle)
{
	Handle newHandle;
	long size, newSize;
	char *pData;

	if (theHandle)
	{
		size = GetHandleSize(theHandle);
		newHandle = NewHandle(size + sizeof(long));
		if (newHandle)
		{
			HLock(newHandle);
			HLock(theHandle);
			pData = *newHandle;
			pData += sizeof(long);
			LZSSCompress((unsigned char *)*theHandle, size, (unsigned char *)pData, &newSize);
			BlockMoveData(&size, *newHandle, (long)sizeof(long));
			HUnlock(theHandle);
			DisposHandle(theHandle);
			theHandle = NULL;
			if (newSize > 1L)
			{
				newSize += sizeof(long);
#if 1
				theHandle = NewHandle(newSize);
				if (theHandle)
				{
					HLock(theHandle);
					HLock(newHandle);
					BlockMoveData(*newHandle, *theHandle, newSize);
					HUnlock(newHandle);
					HUnlock(theHandle);
				}
				DisposHandle(newHandle);
#else
				SetHandleSize(newHandle, newSize);
				theHandle = newHandle;
#endif
			}
		}
	}
	return(theHandle);
}

Handle DecompressHandle(Handle theData)
{
	long theTotalSize;
	Handle theNewData;

	HLock(theData);
	BlockMove(*theData, &theTotalSize, (long)sizeof(long));
	HUnlock(theData);

	theNewData = NewHandle(theTotalSize);
	if (theNewData)
	{
		HLock(theData);
		HLock(theNewData);
		LZSSUncompress((unsigned char *)(*theData) + sizeof(long), 
							GetHandleSize(theData) - sizeof(long), 
							(unsigned char *)*theNewData, &theTotalSize);
		HUnlock(theNewData);
		HUnlock(theData);
		DisposHandle(theData);
	}

	return(theNewData);
}
#endif

/* EOF of NewLZSS.c
*/
