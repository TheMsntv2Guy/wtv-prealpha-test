// ===========================================================================
//	MemoryGlance.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifdef MEMORY_TRACKING

#ifndef __MACINTOSHMENUS_H__
#include "MacintoshMenus.h"
#endif
#ifndef __MEMORYGLANCE_H__
#include "MemoryGlance.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __MEMORYMANAGER_PRIVATE_H__
#include "MemoryManager.Private.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif



// ===========================================================================
static RGBColor kChangedColor 			= {0xffff, 0x0000, 0x0000};
static RGBColor kChildColor 			= {0x0000, 0x3f3f, 0x3f3f};
static RGBColor kPermanentColor			= {0x0000, 0x3f3f, 0x0000};
static RGBColor kIndividualTagColor		= {0x0000, 0x0000, 0x3f3f};
static RGBColor kTagItemColor			= {0x3f3f, 0x0000, 0x3f3f};
static RGBColor kTagEvenColor			= {0x0000, 0x7f7f, 0x1f1f};
static RGBColor kTagOddColor			= {0x0000, 0x1f1f, 0x7f7f};

const unsigned char kShowZeroBlocks[] = "\pShow Zero Blocks";
const ulong kRecentlyModifiedTime = 2 * kOneSecond;
static ulong lastMemoryOperation = 0;	// when was the last operation?

URLGlanceWindow*		gURLGlanceWindow = nil;
BlockGlanceWindow*		gBlockGlanceWindow = nil;
TagItemWindow*			gTagItemWindow = nil;
// ===========================================================================


MemoryTrackerItem::MemoryTrackerItem(void)
{
	//fNumBlocks = 0;
	//fMaxBlocks = 0;
	//fLifetimeBlocks = 0;
	//fSize = 0;
	//fMaxSize = 0;
	//fLifetimeSize = 0;
	HierarchicalItem::SetNodeExpand(false);
	UpdateLastModified(0);
	SetPermanent(false);
	SetSummaryItem(false);
}

MemoryTrackerItem::~MemoryTrackerItem(void)
{
}

void
MemoryTrackerItem::DoTaggedAlloc(MemoryTag* tag)
{
	fNumBlocks++;
	fLifetimeBlocks++;
	if (fNumBlocks > fMaxBlocks)
		fMaxBlocks = fNumBlocks;
	
	fSize += tag->fLength;
	fLifetimeSize += tag->fLength;
	if (fSize > fMaxSize)
		fMaxSize = fSize;
	
	UpdateLastModified(tag->fModifyTime);
	
	if (fFirstChild != nil)
	{	((MemoryTrackerItem*)fFirstChild)->TaggedAlloc(tag);
	}
}

void
MemoryTrackerItem::DoTaggedFree(MemoryTag* tag)
{
	Assert(fNumBlocks > 0);
	Assert(fSize >= tag->fLength);
	
	fNumBlocks--;
	fSize -= tag->fLength;

#if 0
	if ((fNumBlocks == 0) && (gDeleteZeroBlocks))
	{
		Assert(fSize == 0);
		if (!GetSummaryItem())
		{
			RemoveSibling(this);
			delete this;
		}
	}
	else
#endif
	{
		//UpdateLastModified(tag->fModifyTime);
	
		if (fFirstChild != nil)
		{	((MemoryTrackerItem*)fFirstChild)->TaggedFree(tag);
		}
	}
}
void
MemoryTrackerItem::DoTaggedNew(MemoryTag* tag)
{
	fNumBlocks++;
	fLifetimeBlocks++;
	if (fNumBlocks > fMaxBlocks)
		fMaxBlocks = fNumBlocks;
	
	fSize += tag->fLength;
	fLifetimeSize += tag->fLength;
	if (fSize > fMaxSize)
		fMaxSize = fSize;

	UpdateLastModified(tag->fModifyTime);

	if (fFirstChild != nil)
	{	((MemoryTrackerItem*)fFirstChild)->TaggedNew(tag);
	}
}
void
MemoryTrackerItem::DoTaggedDelete(MemoryTag* tag)
{
	Assert(fNumBlocks > 0);
	Assert(fSize >= tag->fLength);
	
	fNumBlocks--;
	fSize -= tag->fLength;

#if 0
	if ((fNumBlocks == 0) && (gDeleteZeroBlocks))
	{
		Assert(fSize == 0);
		if (!GetSummaryItem())
		{
			RemoveSibling(this);
			delete this;
		}
	}
	else
#endif
	{
		//UpdateLastModified(tag->fModifyTime);
	
		if (fFirstChild != nil)
		{	((MemoryTrackerItem*)fFirstChild)->TaggedDelete(tag);
		}
	}
}

void
MemoryTrackerItem::TaggedAlloc(MemoryTag* tag)
{
	Assert(tag != nil);
	if (tag == nil)
		return;
	if (tag->fName == kCPlusObjectTagString)	// C++ object...wait for TaggedNew
		return;

	MacBoolean gotMatch = false;
	MemoryTrackerItem* item = (MemoryTrackerItem*)First();
	
	while (item != nil)
	{
		MacBoolean isSummaryItem = item->GetSummaryItem();
		MemoryTrackerItem* next = (MemoryTrackerItem*)(item->Next());

		if (isSummaryItem || item->Match(tag))
		{
			item->DoTaggedAlloc(tag);
			if (!isSummaryItem)
				gotMatch = true;	// Match() must've returned true!
		}
		item = next;
	}
	if (!gotMatch)
	{	
		item = NewItem(tag);
		item->SetPermanent(GetPermanent());
		item->DoTaggedAlloc(tag);
		AddSibling(item);
	}
}
void
MemoryTrackerItem::TaggedFree(MemoryTag* tag)
{
	Assert(tag != nil);
	if (tag == nil)
		return;
	if (tag->fIsCPlus)	// if this is a C++ object, "delete" will handle it
		return;

	MacBoolean gotMatch = false;
	MemoryTrackerItem* item = (MemoryTrackerItem*)First();
	
	while (item != nil)
	{
		MacBoolean isSummaryItem = item->GetSummaryItem();
		MemoryTrackerItem* next = (MemoryTrackerItem*)(item->Next());

		if (isSummaryItem || item->Match(tag))
		{
			item->DoTaggedFree(tag);	// may delete the item...no more references to item!
			if (!isSummaryItem)
				gotMatch = true;
		}
		item = next;
	}
	Assert(gotMatch);	// free on something that wasn't allocated?!
	//if (!gotMatch)
	//{
	//	item = NewItem(tag);
	//	item->SetPermanent(GetPermanent());
	//	AddSibling(item);
	//	item->DoTaggedFree(tag);
	//}
}
void
MemoryTrackerItem::TaggedNew(MemoryTag* tag)
{
	Assert(tag != nil);
	if (tag == nil)
		return;
	Assert(tag->fIsCPlus);		// hey!  This should be a C++ object!!!
	if (!(tag->fIsCPlus))
		return;

	MacBoolean gotMatch = false;
	MemoryTrackerItem* item = (MemoryTrackerItem*)First();
	
	while (item != nil)
	{
		MacBoolean isSummaryItem = item->GetSummaryItem();
		MemoryTrackerItem* next = (MemoryTrackerItem*)(item->Next());

		if (isSummaryItem || item->Match(tag))
		{
			item->DoTaggedNew(tag);
			if (!isSummaryItem)
				gotMatch = true;
		}
		item = next;
	}
	if (!gotMatch)
	{	
		item = NewItem(tag);
		item->SetPermanent(GetPermanent());
		item->DoTaggedNew(tag);
		AddSibling(item);
	}
}
void
MemoryTrackerItem::TaggedDelete(MemoryTag* tag)
{
	Assert(tag != nil);
	if (tag == nil)
		return;
	Assert(tag->fIsCPlus);		// hey!  This should be a C++ object!!!
	if (!(tag->fIsCPlus))
		return;

	MacBoolean gotMatch = false;
	MemoryTrackerItem* item = (MemoryTrackerItem*)First();
	
	while (item != nil)
	{
		MacBoolean isSummaryItem = item->GetSummaryItem();
		MemoryTrackerItem* next = (MemoryTrackerItem*)(item->Next());

		if (isSummaryItem || item->Match(tag))
		{
			item->DoTaggedDelete(tag);	// may delete the item...no more references to item!
			if (!isSummaryItem)
				gotMatch = true;
		}
		item = next;
	}
	Assert(gotMatch);	// delete on something that wasn't new-ed?!
	//if (!gotMatch)
	//{
	//	item = NewItem(tag);
	//	item->SetPermanent(GetPermanent());
	//	AddSibling(item);
	//	item->DoTaggedDelete(tag);
	//}
}

MacBoolean
MemoryTrackerItem::IsSortedBefore(MemoryTrackerItem*)
{
	return true;	// silly, yes, but this way recent things are added close
					// to the front of the list
}

void
MemoryTrackerItem::AddSibling(MemoryTrackerItem* item)
{
	MemoryTrackerItem* justBeforeItem = (MemoryTrackerItem*)First();
	MemoryTrackerItem* nextItem = (MemoryTrackerItem*)(justBeforeItem->Next());
	while (nextItem != nil)
	{
		if (item->IsSortedBefore(nextItem))
			break;
		justBeforeItem = nextItem;
		nextItem = (MemoryTrackerItem*)(justBeforeItem->Next());
	}
	justBeforeItem->AddAfterSibling(item);
}


MacBoolean
MemoryTrackerItem::Match(MemoryTag* tag)
{
	 return tag->fInUse;	// EVERYTHING matches MemoryTrackerItem
}

MemoryTrackerItem*
MemoryTrackerItem::NewItem(MemoryTag*)
{
	MemoryTrackerItem* item = newMac(MemoryTrackerItem);
	return item;
}
		
MemoryTag*
MemoryTrackerItem::FirstMatchingMemoryTag(void)
{
	MemoryTag* tag = &(gTagSlots[0]);
	tag--;
	return NextMatchingMemoryTag(tag);
}

MemoryTag*
MemoryTrackerItem::NextMatchingMemoryTag(MemoryTag* tag)
{
	if (tag == nil)
		return nil;

	if (fParent == nil)
	{
		while (++tag != &(gTagSlots[kTagSlots]))
		{
			if (tag->fInUse && (Match(tag) || GetSummaryItem()))
				return tag;
		}
	}
	else
	{
		while ((tag = ((MemoryTrackerItem*)fParent)->NextMatchingMemoryTag(tag)) != nil)
		{
			if (tag->fInUse && (Match(tag) || GetSummaryItem()))
				return tag;
		}
	}
	return nil;
}

void
MemoryTrackerItem::InitFromHeap(void)
{
	fNumBlocks = 0;
	fLifetimeBlocks = 0;
	fMaxBlocks = 0;
	fSize = 0;
	fLifetimeSize = 0;
	fMaxSize = 0;

	MemoryTag* addTag = FirstMatchingMemoryTag();
	while (addTag != nil)
	{
		if (addTag->fIsCPlus)
			TaggedNew(addTag);
		else
			TaggedAlloc(addTag);
		addTag = NextMatchingMemoryTag(addTag);
	}
}

HierarchicalItem*
MemoryTrackerItem::DoClickNode(const MacPoint* where, ushort modifiers, HierarchicalWindow* UNUSED(destWindow))
{
	if ((where->v >= 0) && (where->v < fHeight))
	{
		if (modifiers & optionKey)
		{	SetTreeExpand(!GetNodeExpand());
		}
		else
		{	SetNodeExpand(!GetNodeExpand());
		}
		return this;
	}
	else
		return nil;
}

void
MemoryTrackerItem::DrawNode(const MacPoint* topLeft, HierarchicalWindow* destWindow)
{
	MoveTo(topLeft->h + (4*destWindow->GetCharWidth()), topLeft->v + destWindow->GetAscentHeight());

	char buffer[128];
	DrawText(buffer, 0, snprintf(buffer, sizeof(buffer),
								"Current: (%4d,%7d)"
								"  Hiwater: (%4d,%7d)"
								"  Lifetime: (%4d,%7d)",
								fNumBlocks, fSize,
								fMaxBlocks, fMaxSize,
								fLifetimeBlocks, fLifetimeSize));
	fHeight = destWindow->GetLineHeight();
}

ulong
MemoryTrackerItem::GetLastModified(void)
{
	return fLastModified;
}

MacBoolean
MemoryTrackerItem::GetRecentlyModified(ulong timePeriod)
{
	return ((lastMemoryOperation - fLastModified) < timePeriod);
}

MacBoolean
MemoryTrackerItem::GetPermanent(void)
{
	return fPermanent;
}

MacBoolean
MemoryTrackerItem::GetSummaryItem(void)
{
	return fSummary;
}

void
MemoryTrackerItem::UpdateLastModified(ulong time)
{
	fLastModified = time;
}

void
MemoryTrackerItem::SetPermanent(MacBoolean permanent)
{
	fPermanent = permanent;
}

void
MemoryTrackerItem::SetSummaryItem(MacBoolean summary)
{
	fSummary = summary;
}

// -----------------------------------------------------------------------------------

TagItem::TagItem(void)
{
	fTag = nil;
	fMaxExpand = false;
}

TagItem::~TagItem(void)
{
	fTag = nil;
}

MacBoolean
TagItem::IsSortedBefore(MemoryTrackerItem* item)
{
	return ((ulong)(fTag->fBase) < (ulong)(((TagItem*)item)->fTag->fBase));
}

MacBoolean
TagItem::Match(MemoryTag* tag)
{
	return (tag == fTag);
}

MemoryTrackerItem*
TagItem::NewItem(MemoryTag* tag)
{
	Assert(tag != nil);
	if (tag == nil)
		return nil;
	
	TagItem* newItem = newMac(TagItem);
	newItem->fTag = tag;
	
	return (TagItem*)newItem;
}

HierarchicalItem*
TagItem::DoClickNode(const MacPoint* where, ushort UNUSED(modifiers), HierarchicalWindow* destWindow)
{
	if ((where->v >= 0) && (where->v < fHeight))
	{
		short lineHeight = destWindow->GetLineHeight();
	
		if (where->v < lineHeight)
		{
			SetNodeExpand(!GetNodeExpand());
			fMaxExpand = false;
		}
		else if (where->v > fHeight - lineHeight)
		{
			fMaxExpand = !fMaxExpand;
		}
		else
		{
			fMaxExpand = false;
		}
		return this;
	}
	else
		return nil;
}

void
TagItem::DrawNode(const MacPoint* topLeft, HierarchicalWindow* destWindow)
{
	if (GetSummaryItem())
	{
		fHeight = 0;
	}
	else
	{
		MacPoint textPos = *topLeft;
		short lineHeight = destWindow->GetLineHeight();
		short indent = destWindow->GetCharWidth() * 4;
		
		textPos.v += destWindow->GetAscentHeight();
		MoveTo(textPos.h, textPos.v);
		char buffer[256];
		DrawText(buffer, 0, snprintf(buffer, sizeof(buffer),
									"%#x: (%7d bytes) allocated in (%s, line %d)",
									fTag->fBase, fTag->fLength,
									fTag->fSourceFile, fTag->fLineNumber));
		textPos.v += lineHeight;
		textPos.h += indent;
	
		RGBColor saveColor;
		GetForeColor(&saveColor);
		RGBForeColor(GetRecentlyModified() ? &kChangedColor : &kTagItemColor);
		DrawNodeDispatch(&textPos, lineHeight, destWindow->GetCharWidth(), fExpand);
		RGBForeColor(&saveColor);
	
		textPos.h -= indent;
		textPos.v -= destWindow->GetAscentHeight();
	
		fHeight = textPos.v - topLeft->v;
	}
}
 
void
TagItem::DrawNodeDefault(MacPoint* topLeft, short height, short width, MacBoolean UNUSED(expand))
{
	if (GetNodeExpand())
	{
		RGBColor saveColor;
		GetForeColor(&saveColor);

		int maxRows = 2;
		const int maxCols = 16;

		if (fTag->fLength <= maxCols*(maxRows+1))	// three is ok, otherwise just do two
		{	maxRows++;								// and reserve the third line for message
		}

		int rows=0;
		int colCounter = 0;
		int byteCounter = 0;
		unsigned char* data = (unsigned char*)(fTag->fBase);
		
		while ((byteCounter < fTag->fLength) && ((rows<maxRows) || fMaxExpand))
		{
			MacPoint rowStart = *topLeft;
			topLeft->v += height;
			rows++;
	
			int rowLength = fTag->fLength - byteCounter;
			if (rowLength > maxCols)
				rowLength = maxCols;
	
			MoveTo(rowStart.h, rowStart.v);
			char buffer[12];
			DrawText(buffer, 0, snprintf(buffer, sizeof(buffer),
										"%#6x:", byteCounter));
			rowStart.h += width*7;
			

			MacBoolean oddColor = true;

			for (colCounter = 0; colCounter < rowLength; colCounter++)
			{
				if (colCounter%4 == 0)
				{
					oddColor = !oddColor;
					RGBForeColor(oddColor ? &kTagOddColor : &kTagEvenColor);
				}
			
				MoveTo(rowStart.h, rowStart.v);
				if ((*data < 0x20) || (*data > 0xd9))
				{	char unprintable = '.';
					DrawText(&unprintable, 0, 1);
				}
				else
				{	DrawText(data, 0, 1);
				}
				MoveTo(rowStart.h + ((maxCols + ((5*colCounter)/4) + 3)*width), rowStart.v);
				DrawText(buffer, 0, snprintf(buffer, sizeof(buffer), "%02x", *data));
				data++;
				byteCounter++;
				rowStart.h += width;
			}
			RGBForeColor(GetRecentlyModified() ? &kChangedColor : &saveColor);
			
			buffer[0]=':';
			MoveTo(rowStart.h, rowStart.v);
			DrawText(buffer, 0, 1);
		}
		
		if (byteCounter < fTag->fLength)
		{
			const char moreString[] = "<click for all>";
			MoveTo(topLeft->h, topLeft->v);
			DrawText(moreString, 0, sizeof(moreString)-1);
			topLeft->v += height;
		}
	}
}


void
TagItem::DrawNodeDispatch(MacPoint* topLeft, short height, short width, MacBoolean expand)
{
	// for certain items, might draw complicated stuff here

	// if not, default is to do...
	DrawNodeDefault(topLeft, height, width, expand);
}

// -----------------------------------------------------------------------------------

URLMemoryTrackerItem::URLMemoryTrackerItem(void)
{
	fURLName = nil;
}
URLMemoryTrackerItem::~URLMemoryTrackerItem(void)
{
}

MacBoolean
URLMemoryTrackerItem::IsSortedBefore(MemoryTrackerItem* item)
{
	if ( fURLName == nil )
		return true;
	return (strcmp(fURLName, ((URLMemoryTrackerItem*)item)->fURLName) < 0);
}

MacBoolean
URLMemoryTrackerItem::Match(MemoryTag* tag)
{
	if (!(tag->fInUse))
		return false;
	if ( fURLName == nil )
		return true;
	if (tag->fOwnerURL == fURLName)
		return true;
	if (strcmp(tag->fOwnerURL, fURLName) == 0)
	{
		Complain(("Non-unique URL name \042%s\042", fURLName));	// 042 is octal 34 = doublequote 
		return true;
	}
	return false;
}

MemoryTrackerItem*
URLMemoryTrackerItem::NewItem(MemoryTag* tag)
{
	Assert(tag != nil);
	if (tag == nil)
		return nil;
	
	URLMemoryTrackerItem* newItem = newMac(URLMemoryTrackerItem);
	newItem->fURLName = tag->fOwnerURL;
	
	return (MemoryTrackerItem*)newItem;
}

void
URLMemoryTrackerItem::DrawNode(const MacPoint* topLeft, HierarchicalWindow* destWindow)
{
	fHeight = 0;

	if ((fNumBlocks == 0) && (!(((MemoryGlanceWindow*)destWindow)->GetZeroBlocksVisible())))
		return;

	RGBColor saveColor;
	GetForeColor(&saveColor);

	MacPoint textPos = *topLeft;
	textPos.v += destWindow->GetAscentHeight();
	short lineHeight = destWindow->GetLineHeight();
	
	char buffer[1024];
	const char* URLstring = fURLName;

	if (GetSummaryItem()) {	// summary item is mini-header 
		short saveFace = qd.thePort->txFace;
		TextFace(underline);
		
		MoveTo(textPos.h, textPos.v);
		DrawText(buffer, 0, snprintf(buffer, sizeof(buffer), "(Blocks,Bytes) URL"));
		fHeight += lineHeight;
		textPos.v += lineHeight;
		URLstring = "Total Usage";
		TextFace(saveFace);
	}
	
	int bufUsed = 0;
	if (GetPermanent())	{ // if permanent, max and lifetime are legitimate values
		bufUsed = snprintf(buffer, sizeof(buffer),
							"(%4d,%7d) Max=(%4d,%7d), Life=(%5d,%9d)",
							fNumBlocks, fSize,
							fMaxBlocks, fMaxSize,
							fLifetimeBlocks, fLifetimeSize);
		RGBForeColor(GetRecentlyModified() ? &kChangedColor : &kPermanentColor);
	} else {
		bufUsed = snprintf(buffer, sizeof(buffer), "(%4d,%7d)", fNumBlocks, fSize);
		RGBForeColor(GetRecentlyModified() ? &kChangedColor : &kChildColor);
	}
	
	bufUsed += snprintf(&(buffer[bufUsed]), sizeof(buffer) - bufUsed,
						"%.*s", sizeof(buffer) - bufUsed - 1, URLstring);
	MoveTo(textPos.h, textPos.v);
	DrawText(buffer, 0, bufUsed);
	fHeight += lineHeight;
	textPos.v += lineHeight;
	
	if (fExpand && (!fFirstChild)) {
		RGBForeColor(GetRecentlyModified() ? &kChangedColor : &kIndividualTagColor);
		short indent = 4 * destWindow->GetCharWidth();
		textPos.h += indent;
		MemoryTag* tag = FirstMatchingMemoryTag();
		while (tag != nil) {
			MoveTo(textPos.h, textPos.v);
			DrawText(buffer, 0, snprintf(buffer, sizeof(buffer),
										"0x%x: (%7d bytes) %s: %s (%s, line %d)",
										tag->fBase, tag->fLength,
										tag->fIsCPlus ? "Class" : "Block",
										tag->fName,
										tag->fSourceFile, tag->fLineNumber));
			
			fHeight += destWindow->GetLineHeight();
			textPos.v += lineHeight;
			tag = NextMatchingMemoryTag(tag);
		}
		textPos.h -= indent;
	}
	RGBForeColor(&saveColor);
}
 
void
URLMemoryTrackerItem::SetNodeExpand(MacBoolean newExpand)
{
	if (GetSummaryItem())	// don't let summary nodes expand!
		return;
	if (fExpand == newExpand)
		return;
	if (newExpand)
	{
		if (fFirstChild == nil)
		{
			MemoryTrackerItem* child = nil;
			if (fPermanent)
			{
				child = (MemoryTrackerItem*)newMac(BlockMemoryTrackerItem);
			}
			else
			{
				child = (MemoryTrackerItem*)newMac(TagItem);
			}
	
			Assert(child != nil);
			if (child == nil)
				return;
			child->SetSummaryItem(true);
			AddChild(child);
			
			// now we're ready to catch up these blocks
			
			MemoryTag* addTag = FirstMatchingMemoryTag();
			while (addTag != nil)
			{
				if (addTag->fIsCPlus)
					((MemoryTrackerItem*)fFirstChild)->TaggedNew(addTag);
				else
					((MemoryTrackerItem*)fFirstChild)->TaggedAlloc(addTag);
				addTag = NextMatchingMemoryTag(addTag);
			}
		}
	}
	else
	{
		if (fFirstChild != nil)
		{	// get rid of all non-permanent children
			MemoryTrackerItem* child = (MemoryTrackerItem*)fFirstChild;
			while (child != nil)
			{
				MemoryTrackerItem* next = (MemoryTrackerItem*)(child->Next());
				if (child->GetPermanent() == false)
				{
					delete child;
				}
				child = next;
			}
		}
	}
	MemoryTrackerItem::SetNodeExpand(newExpand);
}

// -----------------------------------------------------------------------------------

BlockMemoryTrackerItem::BlockMemoryTrackerItem(void)
{
	fBlockName = nil;
}
BlockMemoryTrackerItem::~BlockMemoryTrackerItem(void)
{
}

MacBoolean
BlockMemoryTrackerItem::IsSortedBefore(MemoryTrackerItem* item)
{
	if ( fBlockName == nil )
		return true;
	return (strcmp(fBlockName, ((BlockMemoryTrackerItem*)item)->fBlockName) < 0);
}

MacBoolean
BlockMemoryTrackerItem::Match(MemoryTag* tag)
{
	if (!(tag->fInUse))
		return false;
	if (tag->fName == fBlockName)
		return true;
	if ( fBlockName == nil )
		return true;
		
	if (strcmp(tag->fName, fBlockName) == 0)
	{
		//Complain(("Non-unique tag name \042%s\042", fBlockName));	// 042 is octal 34 = doublequote 
		return true;
	}
	return false;
}

MemoryTrackerItem*
BlockMemoryTrackerItem::NewItem(MemoryTag* tag)
{
	Assert(tag != nil);
	if (tag == nil)
		return nil;
	
	BlockMemoryTrackerItem* newItem = newMac(BlockMemoryTrackerItem);
	newItem->fBlockName = tag->fName;
	
	return (MemoryTrackerItem*)newItem;
}

void
BlockMemoryTrackerItem::DrawNode(const MacPoint* topLeft, HierarchicalWindow* destWindow)
{
	fHeight = 0;

	if ((fNumBlocks == 0) && (!(((MemoryGlanceWindow*)destWindow)->GetZeroBlocksVisible())))
		return;

	RGBColor saveColor;
	GetForeColor(&saveColor);

	MacPoint textPos = *topLeft;
	textPos.v += destWindow->GetAscentHeight();
	short lineHeight = destWindow->GetLineHeight();
	
	char buffer[1024];
	const char* tagString = fBlockName;

	if (GetSummaryItem()) {	// summary item is mini-header
	
		MoveTo(textPos.h, textPos.v);
		DrawText(buffer, 0, snprintf(buffer, sizeof(buffer), "(Blocks,  Bytes  ) Class/Tag"));
		fHeight += lineHeight;
		textPos.v += lineHeight;
		tagString = "Total Usage";
	}
	
	int bufUsed = 0;
	if (GetPermanent())	{ // if permanent, max and lifetime are legitimate values
		bufUsed = snprintf(buffer, sizeof(buffer),
							"(%4d,%7d) Max=(%4d,%7d), Life=(%5d,%9d) ",
							fNumBlocks, fSize,
							fMaxBlocks, fMaxSize,
							fLifetimeBlocks, fLifetimeSize);
		RGBForeColor(GetRecentlyModified() ? &kChangedColor : &kPermanentColor);
	} else {
		bufUsed = snprintf(buffer, sizeof(buffer),
							"(%4d,%7d) ", fNumBlocks, fSize);
		RGBForeColor(GetRecentlyModified() ? &kChangedColor : &kChildColor);
	}
	bufUsed += snprintf(&(buffer[bufUsed]), sizeof(buffer) - bufUsed,
						"%.*s", sizeof(buffer) - bufUsed - 1, tagString);
	MoveTo(textPos.h, textPos.v);
	DrawText(buffer, 0, bufUsed);
	
	fHeight += lineHeight;
	textPos.v += lineHeight;
	
	if (fExpand && (!fFirstChild)) {
		short indent = 4 * destWindow->GetCharWidth();
		textPos.h += indent;

		RGBForeColor(GetRecentlyModified() ? &kChangedColor : &kIndividualTagColor);
		MemoryTag* tag = FirstMatchingMemoryTag();
		while (tag != nil) {
			int bufUsed = snprintf(buffer, sizeof(buffer),
									"%#x: (%7d bytes) [%s, line %d]",
									tag->fBase, tag->fLength,
									tag->fSourceFile, tag->fLineNumber);
			bufUsed += snprintf(&(buffer[bufUsed]), sizeof(buffer) - bufUsed,
									"%.*s", sizeof(buffer) - bufUsed - 1, tag->fOwnerURL);

			MoveTo(textPos.h, textPos.v);
			DrawText(buffer, 0, bufUsed);
			
			fHeight += destWindow->GetLineHeight();
			textPos.v += lineHeight;
			tag = NextMatchingMemoryTag(tag);
		}

		textPos.h -= indent;
	}
	RGBForeColor(&saveColor);
}
 
void
BlockMemoryTrackerItem::SetNodeExpand(MacBoolean newExpand)
{
	if (GetSummaryItem())	// don't let summary nodes expand!
		return;
	if (fExpand == newExpand)
		return;
	if (newExpand)
	{
		if (fFirstChild == nil)
		{
			MemoryTrackerItem* child = nil;
			if (fPermanent)
			{
				child = (MemoryTrackerItem*)newMac(URLMemoryTrackerItem);
			}
			else
			{
				child = (MemoryTrackerItem*)newMac(TagItem);
			}
	
			Assert(child != nil);
			if (child == nil)
				return;
			child->SetSummaryItem(true);
			AddChild(child);
			
			// now we're ready to catch up these blocks
			
			MemoryTag* addTag = FirstMatchingMemoryTag();
			while (addTag != nil)
			{
				if (addTag->fIsCPlus)
					((MemoryTrackerItem*)fFirstChild)->TaggedNew(addTag);
				else
					((MemoryTrackerItem*)fFirstChild)->TaggedAlloc(addTag);
				addTag = NextMatchingMemoryTag(addTag);
			}
		}
	}
	else
	{
		if (fFirstChild != nil)
		{	// get rid of all non-permanent children
			MemoryTrackerItem* child = (MemoryTrackerItem*)fFirstChild;
			while (child != nil)
			{
				MemoryTrackerItem* next = (MemoryTrackerItem*)(child->Next());
				if (child->GetPermanent() == false)
				{
					delete child;
				}
				child = next;
			}
		}
	}
	MemoryTrackerItem::SetNodeExpand(newExpand);
}

// -----------------------------------------------------------------------------------

MemoryGlanceWindow::MemoryGlanceWindow(void)
{
	SetFormat(28, 0, true, true); // header, trailer, vscroll, hscroll

	Rect r;
	GetHeaderRect(&r);
	r.left += 4;
	r.right = r.left + 140;
	r.top += 4;
	r.bottom = r.top + 20;

	fZeroBlocksControl = NewControl(w, &r, kShowZeroBlocks, true,
									0, 0, 1, checkBoxProc, 0);
	SetZeroBlocksVisible(false);
}

MemoryGlanceWindow::~MemoryGlanceWindow(void)
{
	if (fZeroBlocksControl != nil)
	{
		DisposeControl(fZeroBlocksControl);
		fZeroBlocksControl = nil;
	}
}

void MemoryGlanceWindow::DrawHeader(Rect*)
{
}

void
MemoryGlanceWindow::Close(void)
{
	HideWindow();
}

void
MemoryGlanceWindow::Click(MacPoint *where, ushort modifiers)
{
	ControlHandle	c;
	
	if ( 	(fZeroBlocksControl != nil)
		 && FindControl(*where,w,&c)
		 &&	(c == fZeroBlocksControl)
		 &&	TrackControl(c, *where, nil))
	{	
		SetZeroBlocksVisible(!GetZeroBlocksVisible());
		RequestRedraw();
	}
	else
	{
		HierarchicalWindow::Click(where, modifiers);
	}
}

void
MemoryGlanceWindow::TaggedAlloc(MemoryTag* tag)
{
	lastMemoryOperation = Now();
	if (fRootItem != nil)
	{	((MemoryTrackerItem*)fRootItem)->TaggedAlloc(tag);
		RequestRedraw();
	}
}

void
MemoryGlanceWindow::TaggedFree(MemoryTag* tag)
{
	//lastMemoryOperation = Now();
	if (fRootItem != nil)
	{	((MemoryTrackerItem*)fRootItem)->TaggedFree(tag);
		RequestRedraw();
	}
}

void
MemoryGlanceWindow::TaggedNew(MemoryTag* tag)
{
	lastMemoryOperation = Now();
	if (fRootItem != nil)
	{	((MemoryTrackerItem*)fRootItem)->TaggedNew(tag);
		RequestRedraw();
	}
}

void
MemoryGlanceWindow::TaggedDelete(MemoryTag* tag)
{
	//lastMemoryOperation = Now();
	if (fRootItem != nil)
	{	((MemoryTrackerItem*)fRootItem)->TaggedDelete(tag);
		RequestRedraw();
	}
}

void
MemoryGlanceWindow::SetZeroBlocksVisible(MacBoolean isVisible)
{
	fZeroBlocksVisible = isVisible;
	if (fZeroBlocksControl != nil)
	{	SetControlValue(fZeroBlocksControl, fZeroBlocksVisible ? 1 : 0);
	}
}

MacBoolean
MemoryGlanceWindow::GetZeroBlocksVisible(void)
{
	if (fZeroBlocksControl != nil)
	{
		MacBoolean controlValue = GetControlValue(fZeroBlocksControl) ? true : false;
		Assert(fZeroBlocksVisible == controlValue);
	}
	return fZeroBlocksVisible;
}

Boolean
MemoryGlanceWindow::SavePrefs(StdWindowPrefs* prefPtr)
{
	MemoryGlanceWindowPrefs defaultPrefs;
	size_t prefSize = GetPrefsSize();
	
	if (prefPtr == nil)
	{
		prefPtr = &defaultPrefs;
		prefSize = MemoryGlanceWindow::GetPrefsSize();
	}
	
	((MemoryGlanceWindowPrefs*)prefPtr)->zeroBlocksVisible = GetZeroBlocksVisible();
	
	return HierarchicalWindow::SavePrefs(prefPtr);
}

Boolean
MemoryGlanceWindow::RestorePrefs(StdWindowPrefs* prefPtr)
{

	MemoryGlanceWindowPrefs defaultPrefs;
	size_t prefSize = GetPrefsSize();

	if (prefPtr == nil)
	{
		prefPtr = &defaultPrefs;
		prefSize = MemoryGlanceWindow::GetPrefsSize();
	}

	if (!StdWindow::RestorePrefs(prefPtr))
		return false;

	SetZeroBlocksVisible(((MemoryGlanceWindowPrefs*)prefPtr)->zeroBlocksVisible);
	return true;
}

long
MemoryGlanceWindow::GetPrefsSize(void)
{
	return sizeof(MemoryGlanceWindowPrefs);
}

// -----------------------------------------------------------------------------------

URLGlanceWindow::URLGlanceWindow(void)
{
	if (gURLGlanceWindow != nil)
	{
		delete gURLGlanceWindow;
	}
	gURLGlanceWindow = this;
	fRootItem = nil;
	SetTitle("URL list");
}

URLGlanceWindow::~URLGlanceWindow(void)
{
	gURLGlanceWindow = nil;
}

void
URLGlanceWindow::DoAdjustMenus(ushort modifiers)
{
	MemoryGlanceWindow::DoAdjustMenus(modifiers);

	MenuHandle menu = GetMenu(mMemory);
	EnableItem(menu, iGlanceByURL);
	SetItemMark(menu, iGlanceByURL, GetVisible() ? checkMark : ' ');
}

Boolean
URLGlanceWindow::DoMenuChoice(long menuChoice, ushort modifiers)
{
	Boolean handled = false;
	short theMenu = HiWord(menuChoice);
	short theItem = LoWord(menuChoice);

	if ((theMenu == mMemory) && (theItem == iGlanceByURL))
	{
		if (GetVisible())
			HideWindow();
		else
			ShowWindow();

		handled = true;
	}
	return handled || MemoryGlanceWindow::DoMenuChoice(menuChoice, modifiers);
}

void
URLGlanceWindow::HideWindow(void)
{
	if (fRootItem != nil)
	{
		delete fRootItem;
	}
	fRootItem = nil;
	MemoryGlanceWindow::HideWindow();
}

void
URLGlanceWindow::ShowWindow(void)
{
	if (fRootItem != nil)
		delete fRootItem;
	
	fRootItem = newMac(URLMemoryTrackerItem);
	if (fRootItem != nil)
	{
		((URLMemoryTrackerItem*)fRootItem)->SetSummaryItem(true);	
		((URLMemoryTrackerItem*)fRootItem)->SetPermanent(true);	
		((URLMemoryTrackerItem*)fRootItem)->InitFromHeap();
	}	
	MemoryGlanceWindow::ShowWindow();
}

// -----------------------------------------------------------------------------------

BlockGlanceWindow::BlockGlanceWindow(void)
{
	if (gBlockGlanceWindow != nil)
	{
		delete gBlockGlanceWindow;
	}
	gBlockGlanceWindow = this;
	fRootItem = nil;
	SetTitle("Class/Block list");
}

BlockGlanceWindow::~BlockGlanceWindow(void)
{
	gBlockGlanceWindow = nil;
}

void
BlockGlanceWindow::DoAdjustMenus(ushort modifiers)
{
	MemoryGlanceWindow::DoAdjustMenus(modifiers);

	MenuHandle menu = GetMenu(mMemory);
	EnableItem(menu, iGlanceByClass);
	SetItemMark(menu, iGlanceByClass, GetVisible() ? checkMark : ' ');
}

Boolean
BlockGlanceWindow::DoMenuChoice(long menuChoice, ushort modifiers)
{
	Boolean handled = false;
	short theMenu = HiWord(menuChoice);
	short theItem = LoWord(menuChoice);

	if ((theMenu == mMemory) && (theItem == iGlanceByClass))
	{
		if (GetVisible())
			HideWindow();
		else
			ShowWindow();

		handled = true;
	}
	return handled || MemoryGlanceWindow::DoMenuChoice(menuChoice, modifiers);
}

void
BlockGlanceWindow::HideWindow(void)
{
	if (fRootItem != nil)
	{
		delete fRootItem;
	}
	fRootItem = nil;
	MemoryGlanceWindow::HideWindow();
}

void
BlockGlanceWindow::ShowWindow(void)
{
	if (fRootItem != nil)
		delete fRootItem;
	
	fRootItem = newMac(BlockMemoryTrackerItem);
	if (fRootItem != nil)
	{
		((BlockMemoryTrackerItem*)fRootItem)->SetSummaryItem(true);	
		((BlockMemoryTrackerItem*)fRootItem)->SetPermanent(true);	
		((BlockMemoryTrackerItem*)fRootItem)->InitFromHeap();
	}	
	MemoryGlanceWindow::ShowWindow();
}

// -----------------------------------------------------------------------------------

TagItemWindow::TagItemWindow(void)
{
	if (gTagItemWindow != nil)
	{
		delete gTagItemWindow;
	}
	gTagItemWindow = this;
	fRootItem = nil;
	SetTitle("Tag list");
}

TagItemWindow::~TagItemWindow(void)
{
	gTagItemWindow = nil;
}

void
TagItemWindow::DoAdjustMenus(ushort modifiers)
{
	MemoryGlanceWindow::DoAdjustMenus(modifiers);
	
	MenuHandle menu = GetMenu(mMemory);
	EnableItem(menu, iGlanceByTag);
	SetItemMark(menu, iGlanceByTag, GetVisible() ? checkMark : ' ');
}

Boolean
TagItemWindow::DoMenuChoice(long menuChoice, ushort modifiers)
{
	Boolean handled = false;
	short theMenu = HiWord(menuChoice);
	short theItem = LoWord(menuChoice);

	if ((theMenu == mMemory) && (theItem == iGlanceByTag))
	{
		if (GetVisible())
			HideWindow();
		else
			ShowWindow();

		handled = true;
	}
	return handled || MemoryGlanceWindow::DoMenuChoice(menuChoice, modifiers);
}

void
TagItemWindow::HideWindow(void)
{
	if (fRootItem != nil)
	{
		delete fRootItem;
	}
	fRootItem = nil;
	MemoryGlanceWindow::HideWindow();
}

void
TagItemWindow::ShowWindow(void)
{
	if (fRootItem != nil)
		delete fRootItem;
	
	fRootItem = newMac(TagItem);
	if (fRootItem != nil)
	{
		((TagItem*)fRootItem)->SetSummaryItem(true);	
		((TagItem*)fRootItem)->SetPermanent(true);	
		((TagItem*)fRootItem)->InitFromHeap();
	}
	MemoryGlanceWindow::ShowWindow();
}

#endif /* MEMORY_TRACKING */