/* ===========================================================================
	WTVTypes.h

	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
=========================================================================== */
#ifndef __WTVTYPES_H__
#define __WTVTYPES_H__

typedef signed char			schar;
typedef unsigned char		uchar;
typedef unsigned char 		u_char;
typedef unsigned char		Byte;
typedef unsigned short		ushort;
typedef unsigned short 		u_short;
typedef unsigned long		ulong;
typedef unsigned long 		u_long;
typedef volatile ulong 		vulong;
typedef unsigned int		uint;
typedef unsigned int 		u_int;
typedef unsigned long 		size_t;
typedef unsigned long 		time_t;
typedef unsigned char 		Boolean;

typedef long				Fixed;
#define FixedValue(x)		((x)<<16)

#ifndef false
	#define false 0
#elif false != 0
    #error Incompatible 'false' already #defined
#endif

#ifndef true
	#define true 1
#elif true != 1
	#error Incompatible 'true' already #defined
#endif

#ifndef NULL
	#define NULL 0
#elif NULL != 0
	#error Incompatible 'NULL' already #defined
#endif

#ifndef nil
	#define nil 0
#elif nil != 0
	#error Incompatible 'nil' already #defined
#endif

/* ===========================================================================
	Enumerated types
=========================================================================== */
	
#define QuadChar(ch1,ch2,ch3,ch4) \
	((((ch1)&0x0ff)<<24) + (((ch2)&0x0ff)<<16) + (((ch3)&0x0ff)<<8) + ((ch4)&0x0ff))

typedef enum DataType
{
	kDataTypeAnimation		= QuadChar('A','N','I','M'),
	kDataTypeBitmap			= QuadChar('B','M','I','R'),
	kDataTypeBorder			= QuadChar('B','O','R','D'),
	kDataTypeFidoImage		= QuadChar('F','I','D','O'),
	kDataTypeGIF			= QuadChar('G','I','F','f'),
	kDataTypeHTML			= QuadChar('H','T','M','L'),
	kDataTypeImage			= QuadChar('U','I','M','G'),
	kDataTypeJPEG			= QuadChar('J','P','E','G'),
	kDataTypeMIDI			= QuadChar('M','I','D','I'),
	kDataTypeMIPSCode		= QuadChar('M','I','P','S'),
	kDataTypeMPEGAudio		= QuadChar('M','P','G','A'),
	kDataTypePPCCode		= QuadChar('P','P','C',' '),
	kDataTypeRealAudioMetafile = QuadChar('P','N','R','M'),
	kDataTypeRealAudioProtocol = QuadChar('P','N','R','P'),
	kDataTypeTEXT			= QuadChar('T','E','X','T'),
	kDataTypeTellyScript	= QuadChar('A','N','D','Y'),
	kDataTypeURL			= QuadChar('U','R','L',' '),
	kDataTypeUnprintable	= QuadChar('?','?','?','?'),
	kDataTypeXBitMap		= QuadChar('X','B','M','P')
} DataType;

#ifdef HARDWARE
	#define kBusError	((void*)0xa4900000) /* important that this is non-nil */
#else
	#define kBusError	((void*)0x89abcdef) /* important that this is non-nil */
#endif

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include WTVTypes.h multiple times"
	#endif
#endif /* __WTVTYPES_H__ */
