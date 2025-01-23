#ifndef __FLASHSTORAGE_H__
#define __FLASHSTORAGE_H__

#ifndef __WTVTYPES_H__
#include "WTVTypes.h"
#endif

/*************** hw constants for flash storage ***************/

/* If you change kNVSize, change AppROMFSTop in Makefile.include! */

#define		kNVSize				0x00002000		/* we use 8k of FLASH for NV storage */
#define		kNVBufSize			0x00020000		/* need 128k to store flash contents */

#ifdef HARDWARE
	#define		kNVStorageBase	0xbfefe000		/* NV storage starts here in FLASH */
	#define		kNVFlashBase	0xbfee0000		/* base of block we're using in FLASH */
#else
	extern void* kNVStorageBase;	/* set these pointers in */
	extern void* kNVFlashBase;		/* NVInit_Simulator() */
#endif

/*************** tags used by different objects ***************/

enum {
	kTellyTag			= QuadChar('t','l','l','y'), /* 0x746c6c79 */
	kBootTag			= QuadChar('b','o','o','t'), /* 0x626f6f74 */
	kHNameTag			= QuadChar('h','n','m','e'), /* 0x686e6d65 */
	kHPortTag			= QuadChar('h','p','r','t'), /* 0x68707274 */
	kPhoneTag			= QuadChar('F','O','N','E'), /* 0x464f4e45 */
	kConnectionLogTag	= QuadChar('c','l','o','g'), /* 0x636c6f67 */
	kFlashIPTag			= QuadChar('F','L','i','p'), /* 0x464c6970 */
	kFlashPortTag		= QuadChar('F','L','p','o'), /* 0x464c706f */
	kFlashPathTag		= QuadChar('F','L','t','h'), /* 0x464c7468 */
	kSecretTag			= QuadChar('P','s','s','t')  /* 0x         */
};

/************************** NVFlags ***************************/

const ulong	kUserReqFlashUpdate		= 0x00000001;
const ulong	kServerReqFlashUpdate	= 0x00000002;
const ulong kFlashDLFailed			= 0x00000004;
const ulong kPowerOnReboot			= 0x00000008;

/*********************** misc constants ***********************/

enum NVPhase {
	kNVPhaseSave = 0,
	kNVPhaseSaveFB = 1,
	kNVPhaseRestore = 2
};

const ulong kNVFlashAlignMask		= 3;

/************************* typedefs ***************************/

typedef struct NVHeader {
	ulong	checksum;
	ulong	flags;
	ulong	reserved1;
	ulong	reserved2;
} NVHeader;

typedef struct NVNode {
	long			len;
	ulong 			tag;
	uchar 			data[1];
} NVNode;

/************************** protos ****************************/

Boolean		NVSanityCheck(void);

void 		NVInit(NVPhase phase);
void 		NVCommit(void);
void 		NVWrite(uchar *data,long len,ulong tag);
uchar* 		NVRead(ulong tag,long *len);
ulong 		NVChecksum(ulong *base);

ulong		NVGetFlags(void);
void		NVSetFlags(ulong flags);
void 		NVJustSetFlags(ulong flags);

#endif /* __FLASHSTORAGE_H__ */