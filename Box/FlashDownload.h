#ifndef	__FLASH_DOWNLOAD__
#define __FLASH_DOWNLOAD__

#ifndef __Compression__
#include "Compress.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


void DealWithFlashDownload(void);		/* Just call this, it does it all... */
void FlashDownloadIdle(void);			/* Idle proc called by TellyIdle() */


typedef struct FlashBlockHeader {
    ulong               magic;          /* magic value that ids this as flash */
    CompressHeader      compressHeader; /* generic compression stuff */
    ulong               flashAddress;   /* memory address at which to start */
    ushort              blockNum;       /* which block # in the sequence */
    ushort              pad;
    ulong               totalLength;    /* uncr len, useful for progress bar */
} FlashBlockHeader;

#define kFlashBlockMagic        0x96031869      /* big endian */


#define	kMaxGetStringLen		255				/* max len of a GET str we'll send to server */
#define	kMaxSerialNumStringLen	64				/* more than enough */


#define	kMaxBlockFetchErrs		3
#define	kMaxBlockFlashErrs		10

#define	kErrMsgRebootDelay		10				/* in seconds */
#define	kGetDataTimeout			(1*60)			/* in seconds */

/* Flash Download States */

#define kFetchBlock				1
#define	kDecompressBlock		2
#define kFlashBlock				3
#define kFetchError				4
#define kFlashError				5
#define kSuccess				6

/* Flash Download State Machine Errors */

#define kTooManyFetchErrors		1
#define kTooManyFlashErrors		2

/* Block Download States */

#define kAskForBlock			1
#define kGetResponse			2
#define kParseResponse			3
#define kGetCompressedData		4
#define kDownloadFailed			5
#define	kDownloadSuccessful		6


/* Block Download State Machine Errors */


/* ========= Flash Status States ========== */

/* --- Initial messages to user --- */

#define kUpdatingTrashedFlash				0x01
#define	kUpdatingFlashForUser				0x02
#define	kUpdatingFlashForServer				0x03
#define kResumingAbortedFlash				0x04
#define	kPreviousFlashFailed				0x05

/* --- Status --- */
		
#define kCalling800Number					0x11
#define	kGettingLocalNumber					0x12
#define kCallingLocalNumber					0x13
#define kConnectingToFlashServer			0x14
#define kReceivedBlock						0x15
#define	kDecompressing						0x16
#define kFlashingBlock						0x17
		
/* --- Errors --- */

#define kTellyScriptError					0x21
#define kFlashServerNotResponding			0x22
#define kDeepShit							0x23
#define	kErrorFetching						0x24
#define	kErrorFlashing						0x25
#define	kRetrying							0x26
		
/* --- Misc --- */

#define kYouCanStillUseYourBox				0x31
#define kYourBoxIsFucked					0x32
#define	kFlashDownloadFailed				0x33
#define	kBadServerInfo						0x34



#ifdef __cplusplus
}
#endif

#endif /* __FLASH_DOWNLOAD__ */

