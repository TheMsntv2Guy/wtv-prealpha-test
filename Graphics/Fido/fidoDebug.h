#ifndef _FIDO_DEBUG_H_
#define _FIDO_DEBUG_H_

#if !defined FIDO_STANDALONE
	#if !defined __HEADERS_H__
		#include "Headers.h"
	#endif
	#define celRecord FidoCelRecord  
	#define clutBlockList FidoClutBlockList
	#define fillBlockList FidoFillBlockList
	#define texture FidoTexture
	#define AllocateArray(a, b) (a*) AllocateTaggedMemory(sizeof(a) * (b), "Fido "#a, 0) 
	#define _SIMULATE_FIDO_ 1
	#define _KEEP_RGB_ 0
#else
	#include <Types.h>
	#include <string.h>
	#define newArray(a, b) new a[b]
	#define FidoCelRecord celRecord 
	#define FidoClutBlockList clutBlockList 
	#define FidoFillBlockList fillBlockList 
	#define FidoTexture texture
	#define AllocateArray(a, b) new a[b]
#endif

#ifndef DEBUG
	#define CheapAssert(code)
	#define DebugCode(code)
	#define NonDebugCode(code) code
	#if defined FIDO_STANDALONE
		#define Assert(assertion)
	#endif
#else
	#define DebugCode(code) code
	#define NonDebugCode(code)
	#if defined FIDO_STANDALONE
		#if !defined powerc && !defined __powerc
			#define Assert(assertion) do { if ((assertion) == 0) SysBreak(); } while (0)
		#else
			#define Assert(assertion) do { if ((assertion) == 0) Debugger(); } while (0)
		#endif
	#endif
	#define CheapAssert(code) Assert(code)
#endif

#endif // _FIDO_DEBUG_H_
