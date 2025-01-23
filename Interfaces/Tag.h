// ===========================================================================
//	Tag.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __TAG_H__
#define __TAG_H__

#ifndef __LIST_H__
#include "List.h"
#endif

#ifndef __PARSER_H__
#include "Parser.h"
#endif

#ifndef __WTVTYPES_H__
#include "WTVTypes.h"
#endif

// ============================================================================
// Tag

class Tag 
{
public:
							Tag();
							~Tag();
					
	void					SetAttribute(Attribute attributeID, long value, Boolean isPercentage);
	void					SetAttributeStr(Attribute attributeID, const char* value);
	
	const char*				GetValueAsString() const;

public:		
	Attribute				fAttributeID;
	long					fValue;

	Boolean					fIsNumericValue;
	Boolean 				fIsPercentage;
};

// ============================================================================
// TagList

class TagList
{
public:
							TagList();
							~TagList();
					
	void					Add(Attribute attributeID, const char* value);
	void					Add(Attribute attributeID, long value, Boolean isPercentage);
	const Tag*				At(long index) const;
	const Tag*				Find(Attribute attributeID) const;
	void					RemoveAll();
	long					GetCount() const;
	Boolean					IsEmpty() const;

protected:
	DataList				fDataList;
};

// =============================================================================

inline const char* Tag::GetValueAsString() const
{
	return fIsNumericValue ? nil : (const char*)fValue;
}

inline const Tag* TagList::At(long index) const
{
	return (const Tag*)fDataList.At(index);
}

inline long TagList::GetCount() const
{
	return fDataList.GetCount();
}

inline Boolean TagList::IsEmpty() const
{
	return fDataList.IsEmpty();
}

inline void TagList::RemoveAll()
{
	fDataList.RemoveAll();
}


// =============================================================================

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Tag.h multiple times"
	#endif
#endif /*__TAG_H__ */
