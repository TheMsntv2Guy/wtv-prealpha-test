#ifndef __HEADERS_H__
#define __HEADERS_H__

/* WTV includes */

#ifndef __WTVTYPES_H__
#include "WTVTypes.h"
#endif

#ifndef __DEBUG_H__
#include "Debug.h"
#endif

#ifndef __ERRORNUMBERS_H__
#include "ErrorNumbers.h"
#endif

#ifndef __LOG_H__
#include "Log.h"
#endif

#ifndef __STUBS_H__
#include "Stubs.h"
#endif

#ifndef __BOXANSI_H___
	#include "boxansi.h"
#endif

#ifdef FOR_MAC
	#ifndef __PROFILER__
	#include "profiler.h"
	#endif
#endif

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Headers.h multiple times"
	#endif
#endif /* __HEADERS_H__ */
