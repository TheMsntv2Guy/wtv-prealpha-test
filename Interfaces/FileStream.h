// ===========================================================================
//	FileStream.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __FILESTREAM_H__
#define __FILESTREAM_H__
#ifdef SIMULATOR

#ifndef __CACHESTREAM_H__
#include "CacheStream.h"		/* CacheStream */
#endif




class Resource;

// =============================================================================

class FileStream : public CacheStream
{
public:
	virtual					~FileStream();
	
	Boolean					Initialize(const Resource*);

	static FileStream*		NewStream(const Resource*);

protected:
	Boolean					Load(const char*);
};

// =============================================================================

#endif /* SIMULATOR */
#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include FileStream.h multiple times"
	#endif
#endif /* __FILESTREAM_H__ */
