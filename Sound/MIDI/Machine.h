/*
************************************************************************
**
** “Machine.h”
**
**	Generalized Music Synthesis package. Part of SoundMusicSys.
**
**	© Copyright 1983-1996 by Steve Hales, All Rights Reserved.
**	Written by Jim Nitchals and Steve Hales
**	No portion of this program (whether in source, object, or other form)
**	may be copied, duplicated, distributed, altered, or otherwise utilized without
**	direct consent of the copyright holder.
**
**	Confidential-- Internal use only
**
** Overview
**	Machine dependent code and equates
**
**
** Modification History:
**
**	8/19/93		Split from Synthesier.c
**	11/7/95		Major changes, revised just about everything.
**
**
************************************************************************
*/
#ifndef MACHINE_H
#define MACHINE_H

#define INTEL				0
#define MACINTOSH			1
#define SGI				2
#define MAGIC				3

#define k68000			0
#define kRISC				1
#define k80X86			2

#ifdef _MSC_VER
	#define CODE_TYPE			INTEL
#else
	#define CODE_TYPE			MACINTOSH
#endif

#if CODE_TYPE == MACINTOSH
	#define FAR
	#undef _LO_BYTE_FIRST
	#define FP_OFF(x)			(x)
	#define PASCAL			pascal
	
	typedef unsigned char **	MEMREF;

	#if 1
		#define CPU_TYPE			kRISC
	#else
		#define CPU_TYPE			k68000
	#endif

	#ifdef __MWERKS__
	//	#define INLINE
		#define INLINE			inline
	#else
		#define INLINE
	#endif

	
#endif

#if CODE_TYPE == INTEL
	#include <windows.h>
//	#include <dos.h>
	#include <mmsystem.h>
//	#include <mmddk.h>

	#if WIN32
		#define FP_OFF(x)	(x)
	#endif

	#define _LO_BYTE_FIRST
	typedef HANDLE				MEMREF;

	#define INLINE			inline
	
	#define CPU_TYPE				k80X86
#endif

#if (MAX_CHUNK_SIZE%4) != 0
	#error "Bad Chunk Size (22K), Divisible by 4 only!" 
#endif
#if (MAX_CHUNK_SIZE%4) != 0
	#error "Bad Chunk Size (11K), Divisible by 4 only!" 
#endif

#undef TRUE
#undef FALSE
#define TRUE				1
#define FALSE				0

typedef unsigned char FAR *	G_PTR;

typedef char				BOOL_FLAG;
typedef unsigned char		BYTE;
typedef unsigned char		UBYTE;
typedef char				SBYTE;
typedef long				LOOPCOUNT;
typedef short				INT16;
typedef unsigned short		UINT16;
typedef long				INT32;
typedef unsigned long		UINT32;
typedef unsigned long		FIXED_VALUE;


long VX_GetMouse(void);

BOOL_FLAG IsOptionOn(void);

#include "X_API.h"

#endif
