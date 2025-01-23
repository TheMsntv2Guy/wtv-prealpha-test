// ===========================================================================
//	Linkable.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __LINKABLE_H__
#define __LINKABLE_H__

#ifndef __LIST_H__
#include "List.h"		/* for Listable */
#endif

//==================================================================================
//	MixIn class to make common, household objects linkable

class Linkable : public Listable {
public:						
							Linkable();
	virtual					~Linkable();
							
	Linkable*				At(long index) const;
	long					Count() const;
	Linkable*				First() const;
	Linkable*				Last() const;
	Linkable*				Next() const;
	Linkable*				Previous() const;
							
	void					Add(Linkable *item);
	void					AddAfter(Linkable *item);
	void					DeleteAll();
	void					Remove();
	void					RemoveAllAfter();
							
protected:
	Linkable*				fNext;
	Linkable*				fPrevious;
};

#define AddOrSet(list, item)	if (list) (list)->Add(item); else (list) = (item);

//==================================================================================
//	Linked list class to manage a list of linkables.

class LinkedList {
public:						
							LinkedList();
							~LinkedList();
							
	Linkable*				At(long index) const;
	long					Count() const;
	Linkable*				First() const;
	Linkable*				Last() const;
							
	void					Add(Linkable *item);
	void					AddAfter(Linkable* item, Linkable* after);
	void					DeleteAll();
	void					Remove(Linkable*);
							
protected:
	Linkable*				fFirst;
	Linkable*				fLast;
};

//==================================================================================
// Linkable inlines

inline Linkable* Linkable::Next() const
{
	return fNext;
}

inline Linkable* Linkable::Previous() const
{
	return fPrevious;
}

//==================================================================================
		
#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Linkable.h multiple times"
	#endif
#endif /* __LINKABLE_H__ */
