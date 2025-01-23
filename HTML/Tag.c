// Copyright(c) 1995 Artemis Research, Inc. All rights reserved.

#include "Headers.h"
#include "Tag.h"

// ============================================================================
// Tags contain an attribute ID and a value

Tag::Tag()
{
}

Tag::~Tag()
{
}

void Tag::SetAttributeStr(Attribute attributeID, const char* value)
{
	fIsNumericValue = false;
	fAttributeID = attributeID;
	fValue = (long)value;
}

void Tag::SetAttribute(Attribute attributeID, long value, Boolean isPercentage)
{
	fIsNumericValue = true;
	fIsPercentage = isPercentage;
	fAttributeID = attributeID;
	fValue = value;
}

//===========================================================================

TagList::TagList()
{
	fDataList.SetDataSize(sizeof(Tag));
	fDataList.SetListIncrement(16);
}

TagList::~TagList()
{
	RemoveAll();
}

void TagList::Add(Attribute attributeID, long value, Boolean isPercentage)
{
	Tag	tag;
	tag.SetAttribute(attributeID, value, isPercentage);
	fDataList.Add(&tag);
}

void TagList::Add(Attribute attributeID, const char* value)
{
	Tag	tag;
	tag.SetAttributeStr(attributeID, value);
	fDataList.Add(&tag);
}

const Tag* TagList::Find(Attribute attributeID) const
{
	for (long i = 0; i < GetCount(); i++) {
		const Tag*	tag = At(i);	
		if (tag->fAttributeID == attributeID)
			return tag;
	}
	
	return nil;
}

// ============================================================================

