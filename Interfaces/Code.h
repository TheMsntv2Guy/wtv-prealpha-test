// ===========================================================================
//	Song.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __CODE_H__
#define __CODE_H__

#ifndef __CACHEENTRY_H__
#include "CacheEntry.h"
#endif
#ifndef __ERRORNUMBERS_H__
#include "ErrorNumbers.h"			/* Error */
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"				/* Resource */
#endif




// =============================================================================

class Code : public Listable {
public:
	virtual					~Code();
	static void				NewCode(const Resource* resource);

	Boolean					Idle();
	static void				IdleAll();
	
protected:
	Resource				fResource;
};


// =============================================================================

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Code.h multiple times"
	#endif
#endif /* __CODE_H__ */