/*****************************************************/
/*
**	X_API.c
**
**		This provided platform specfic functions
**
**		© Copyright 1989-1996 by Steve Hales, All Rights Reserved.
**		No portion of this program (whether in source, object, or other form)
**		may be copied, duplicated, distributed, or otherwise utilized without
**		direct consent of the copyright holder.
**
**	History	-
**	9/25/95		Created
**	12/14/95	Added XGetAndDetachResource
**	12/19/95	Added XSetBit & XTestBit
**	1/18/96		Spruced up for C++ extra error checking
**	1/28/96		Changed XGetAndDetachResource
**	2/3/96		Removed extra includes
**	2/11/96		Added XIsStereoSupported & XIs16BitSupported
**				Added XFixedDivide & XFixedMultiply
**				Started putting in platform defines around code functions
**	2/21/96		Changed XGetAndDetachResource to return a size
**	3/25/96		Modifed XExpandMace1to6 & XExpandMace1to3 to create silence if not supported
**	3/29/96		Added XPutLong & XPutShort
**	5/14/96		Fixed Odd address error in XIsOurMemoryPtr
*/
/*****************************************************/


#include "X_API.h"

#include "Debug.h"
#include "ObjectStore.h"
#include "MemoryManager.h"

#include "MacintoshUndefines.h"

#include "MacintoshRedefines.h"

// Structures

// Every block of data allocated with XNewPtr will contain this structure before the pointer that is
// passed to the user.
struct XPI_Memblock
{
	long	blockID_one;		// ID that this is our block. part 1
	long	blockSize;			// block size
	long	blockID_two;		// ID that this is our block. part 2
};
typedef struct XPI_Memblock XPI_Memblock;

#define XPI_BLOCK_1_ID		QuadChar('I','G','O','R')
#define XPI_BLOCK_2_ID		QuadChar('G','S','N','D')


// Functions

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
XBOOL XIsVirtualMemoryAvailable(void)
{
#if X_HARDWARE_PLATFORM == X_MACINTOSH
	static long		cachedResult = -1;
	long	feature;

	if (cachedResult == -1)
	{
		feature = 0;
		if (Gestalt(gestaltVMAttr, &feature) == noErr)
		{
			if (feature & (1<<gestaltVMPresent))
			{
				cachedResult = TRUE;
			}
			else
			{
				cachedResult = FALSE;
			}
		}
	}
	return (XBOOL)cachedResult;
#else
	return FALSE;
#endif
}
#endif

static XPI_Memblock *XIsOurMemoryPtr(XPTR data)
{
	char			*pData;
	XPI_Memblock	*pBlock, *pBlockReturn;

	pBlockReturn = NULL;
	if (data)
	{
		pData = (char *)data;
		pData -= sizeof(XPI_Memblock);
		pBlock = (XPI_Memblock *)pData;
		if ( (XGetLong(&pBlock->blockID_one) == XPI_BLOCK_1_ID) &&
			(XGetLong(&pBlock->blockID_two) == XPI_BLOCK_2_ID) )
		{
			pBlockReturn = pBlock;
		}
	}
	return pBlockReturn;
}



#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void XLockMemory(XPTR data, long size)
{
#if X_HARDWARE_PLATFORM == X_MACINTOSH
	if (XIsVirtualMemoryAvailable() && data && size)
	{
		data = (XPTR)XIsOurMemoryPtr(data);
		if (data)
		{
			LockMemory(data, GetPtrSize((Ptr)data));
		}
	}
#else
	data = data;
	size = size;
#endif
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void XUnlockMemory(XPTR data, long size)
{
#if X_HARDWARE_PLATFORM == X_MACINTOSH
	if (XIsVirtualMemoryAvailable() && data && size)
	{
		data = (XPTR)XIsOurMemoryPtr(data);
		if (data)
		{
			UnlockMemory(data, GetPtrSize((Ptr)data));
		}
	}
#else
	data = data;
	size = size;
#endif
}
#endif

// Allocates a block of ZEROED!!!! memory and locks it down
XPTR XNewPtr(long size)
{
	char			*data;
	XPI_Memblock	*pBlock;

	size += sizeof(XPI_Memblock) + 64;
	data = (char *)AllocateTaggedZero(size,"SoundMusicSys");
	if (data)
	{
		pBlock = (XPI_Memblock *)data;
		XPutLong(&pBlock->blockID_one, XPI_BLOCK_1_ID);			// set our ID for this block
		XPutLong(&pBlock->blockID_two, XPI_BLOCK_2_ID);
		pBlock->blockSize = size - sizeof(XPI_Memblock);
		data += sizeof(XPI_Memblock);
	}
	return (XPTR)data;
}

void XDisposePtr(XPTR data)
{
	data = (XPTR)XIsOurMemoryPtr(data);
	if (data)
	{
		FreeTaggedMemory(data, "SoundMusicSys");
	}
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
long XGetPtrSize(XPTR data)
{
	long		size;

	size = 0;
	data = (XPTR)XIsOurMemoryPtr(data);
	if (data)
	{
		size = ((XPI_Memblock *)data)->blockSize;
	}
	else
	{
		// then this block is not ours, so use the system to determine its real size
		size = MemorySize(data);
	}
	return size;
}
#endif

void XBlockMove(XPTR source, XPTR dest, long size)
{
	if (source && dest && size)
	{
		CopyMemory(source, dest, size);
	}
}

// set memory range with value
void XSetMemory(void *pAdr, long len, char value)
{
	register char *pData;

	if (pAdr && (len > 0))
	{
		pData = (char *)pAdr;
		do
		{
			*pData++ = value;
			len--;
		}
		while (len);
	}
}

// Given a pointer, and a bit number; this will set that bit to 1
void XSetBit(void *pBitArray, long whichbit)
{
	unsigned long	byteindex, bitindex;
	unsigned char *byte;

	if (pBitArray)
	{
		byteindex = whichbit / 8;
		bitindex = whichbit % 8;
		byte = &((unsigned char *)pBitArray)[byteindex];
		*byte |= (1L << bitindex);
	}
}

// Given a pointer, and a bit number; this will set that bit to 0
void XClearBit(void *pBitArray, long whichbit)
{
	unsigned long	byteindex, bitindex;
	unsigned char *byte;

	if (pBitArray)
	{
		byteindex = whichbit / 8;
		bitindex = whichbit % 8;
		byte = &((unsigned char *)pBitArray)[byteindex];
		*byte &= ~(1L << bitindex);
	}
}

// Given a pointer, and a bit number; this return the value of that bit
XBOOL XTestBit(void *pBitArray, long whichbit)
{
	unsigned long	byteindex, byte, bitindex;
		
	if (pBitArray)
	{
		byteindex = whichbit / 8;
		bitindex = whichbit % 8;
		byte = ((unsigned char *)pBitArray)[byteindex];
		return (byte & (1L << bitindex)) ? TRUE : FALSE;
	}
	else
	{
		return FALSE;
	}
}

/* Sort an integer array from the lowest to the highest
*/
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void XBubbleSortArray(short int theArray[], short int theCount)
{
	register short int i, j, swapValue;
	
	for (i = 0; i < (theCount - 1); i++)
	{
		for (j = i + 1; j < theCount; j++)
		{
			if (theArray[i] > theArray[j])
			{
				swapValue = theArray[i];
				theArray[i] = theArray[j];
				theArray[j] = swapValue;
			}
		}
	}
}
#endif


// Get a resource and detach it from the resource manager. Which means that you'll need to call XDisposePtr
// to free the memory
XPTR XGetAndDetachResource(long resourceType, long resourceID, long *pReturnedResourceSize)
{
	char		fileName[32];
	char		type[10];
	FSNode	*romFile;
	XPTR	pData;

	switch (resourceType)
	{
		case QuadChar('I','N','S','T'):
			strcpy(type, "INST");
			break;
		case QuadChar('s','n','d',' '):
			strcpy(type, "SND");
			break;
		case QuadChar('c','s','n','d'):
			strcpy(type, "CSND");
			break;
		default:
			XBlockMove((char *)&resourceType, type, 4L);
			type[3] = 0;
			break;
	}
	// First we look for a RAM copy, if not there then we use the ROM versions
	sprintf(fileName, "/RAM/GMPatches/%s_%ld", type, resourceID);
	romFile = Resolve(fileName, false);
	if (romFile == NULL)
	{
		sprintf(fileName, "/ROM/GMPatches/%s_%ld", type, resourceID);
		romFile = Resolve(fileName, false);
	}
	if (romFile == NULL)
	{
		// alternate spelling found in at least one case.  Sigh.
		sprintf(fileName, "/ROM/GM_Patches/%s_%ld", type, resourceID);
		romFile = Resolve(fileName, false);
	}
	pData = NULL;
	*pReturnedResourceSize = 0;
	if (romFile)
	{
		// We just return a pointer and the size. Our dispose system tags our real memory blocks, so
		// nothing should happen if we try to dispose of a rom based pointer.
		*pReturnedResourceSize = romFile->dataLength;
		pData = romFile->data;
#if 0
		// duplicate rom pointer
		pData = XNewPtr(romFile->dataLength);
		if (pData)
		{
			XBlockMove(romFile->data, pData, romFile->dataLength);
		}
#endif
	}
	return pData;
}


// Does sound hardware support stereo output
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
XBOOL XIsStereoSupported(void)
{
	return TRUE;
}
#endif

// Does sound hardware support 16 bit output
#if X_HARDWARE_PLATFORM == X_MACINTOSH
XBOOL XIs16BitSupported(void)
{
	return TRUE;
}
#endif

static const float FixedScalar2 = 1/65536.0;


// do a 16.16 fixed point multiply
XDWORD XFixedMultiply(XDWORD a, XDWORD b)
{
#if 0
	long result;
	float aF, bF;
	aF = a; bF = b * FixedScalar2;
	result = aF * bF;
	return result;
#else
	long result;

	result = (((a >> 16) & 0xFFFF) * ((b >> 16) & 0xFFFF)) << 16;
	result += ((a >> 16) & 0xFFFF) * ( b & 0xFFFF);
	result += ((b >> 16) & 0xFFFF) * ( a & 0xFFFF);
	result += (((a & 0xFFFF) * (b & 0xFFFF)) >> 16) & 0xFFFF;
	return result;
#endif
}

// do a 16.16 fixed point divide
XDWORD XFixedDivide(XDWORD a, XDWORD b)
{
#if 0
	long result;
	float aF, bF;
	aF = a; bF = b * FixedScalar2;
	result = aF / bF;
	return result;
#else
	long result, temp, rfactor, i;
	long f2;
	
	f2 = b;

	temp = a;
	result = 0;
	rfactor = 0x10000;
	for (i = 0; i < 16; i++)
	{
		while ((temp >= f2) && (rfactor != 0) && (temp != 0))
		{
			temp -= f2;
			result += rfactor;
		}
		f2 = f2 >> 1;	
		rfactor = rfactor >> 1;
	}
	return result;
#endif
}

// given a pointer and a value, this with put a short in a ordered 68k way
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void XPutShort(void *pData, unsigned long data)
{
	register unsigned char	*pByte;

	pByte = (unsigned char *) pData;
	pByte[0] = data >> 8;
	pByte[1] = data & 0xFFL;
}
#endif

// given a pointer and a value, this with put a long in a ordered 68k way
void XPutLong(void *pData, unsigned long data)
{
	register unsigned char	*pByte;

	pByte = (unsigned char *) pData;
	pByte[0] = data >> 24L;
	pByte[1] = data >> 16L;
	pByte[2] = data >> 8L;
	pByte[3] = data & 0xFFL;
}

// given a pointer, get a long ordered in a 68k way
unsigned long XGetLong(void *pData)
{
	register unsigned char	*pByte;
	register unsigned long	data;

	pByte = (unsigned char *)pData;

#ifdef FOR_MAC
	if (pByte >= (unsigned char*)LMGetBufPtr()) {
		data = 0;
	}
	else
#endif

	data = ((long)pByte[0] << 24L) | ((long)pByte[1] << 16L) | ((long)pByte[2] << 8L) | ((long)pByte[3]);
	return data;
}


// given a pointer, get a short int ordered in a 68k way
unsigned short int XGetShort(void *pData)
{
	register unsigned char	*pByte;
	register unsigned long	data;

	pByte = (unsigned char *)pData;
	data = ((long)pByte[0] << 8) | (long)pByte[1];
	return data;
}

// expand data 1 to 3 use MacOS MACE compression
void XExpandMace1to3(void *inBuffer, void *outBuffer, unsigned long length, 
					void * inState, void * outState, 
					unsigned long numChannels, unsigned long whichChannel)
{
	inBuffer = inBuffer;
	outBuffer = outBuffer;
	inState = inState;
	outState = outState;
	whichChannel = whichChannel;

	// since this is a specifc Mac 8-bit format, we'll just do silence for now
	XSetMemory(outBuffer, (length * 6) * numChannels, 0x80);
}

// expand data 1 to 6 use MacOS MACE compression
void XExpandMace1to6(void *inBuffer, void *outBuffer, unsigned long length, 
					void * inState, void * outState, 
					unsigned long numChannels, unsigned long whichChannel)
{
	inBuffer = inBuffer;
	outBuffer = outBuffer;
	inState = inState;
	outState = outState;
	whichChannel = whichChannel;

	// since this is a specifc Mac 8-bit format, we'll just do silence for now
	XSetMemory(outBuffer, (length * 6) * numChannels, 0x80);
}


// EOF of X_API.c
