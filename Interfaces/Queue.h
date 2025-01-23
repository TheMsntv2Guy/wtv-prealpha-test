// ===========================================================================
//	Queue.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __QUEUE_H__
#define __QUEUE_H__

#ifndef __ERRORNUMBERS_H__
#include "ErrorNumbers.h"
#endif

#ifndef __WTVTYPES_H__
#include "WTVTypes.h"
#endif

//===========================================================================

typedef struct QHeader QHeader;
typedef struct QEntry QEntry;

struct QHeader {
	ulong	qSignature;
	QEntry	*next;
	QEntry	*prev;
	long	param1;
	long	param2;
	long	param3;
};

struct QEntry {
	QHeader	header;
	char	data[1];		// Start of any extra data
};

//===========================================================================

class Queue
{
public:
			Queue();
	virtual	~Queue();

	virtual	Error	CreateEntry(Boolean atHead, long parameter1, long parameter2 = 0, long parameter3 = 0);
	virtual	void	AddEntry(QEntry *q, Boolean atHead);
	virtual	Error	DestroyEntry(QEntry *q);
	
	virtual	QEntry	*GetHead();
	virtual	void	PrintElements();
	
protected:
	virtual	Error	RemoveEntry(QEntry *q);

	QEntry	*fQHead;
};

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Queue.h multiple times"
	#endif
#endif /* __QUEUE_H__ */
