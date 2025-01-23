// ===========================================================================
//	Document.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __AUDIO_H__
#include "Audio.h"
#endif
#ifndef __CACHESTREAM_H__
#include "CacheStream.h"
#endif
#ifndef __CONTENTVIEW_H__
#include "ContentView.h"
#endif
#ifndef __CONTROL_H__
#include "Control.h"
#endif
#ifndef __DOCUMENT_H__
#include "Document.h"
#endif
#ifndef __DOCUMENTBUILDER_H__
#include "DocumentBuilder.h"
#endif
#ifndef __IMAGE_H__
#include "Image.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __PAGEVIEWER_H__
#include "PageViewer.h"
#endif
#ifndef __PERFDUMP_H__
#include "PerfDump.h"
#endif
#ifndef __RECENTPANEL_H__
#include "RecentPanel.h"
#endif
#ifndef __SONG_H__
#include "Song.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif
#ifndef __SYSTEM_H__
#include "System.h"
#endif
#ifndef __TEXT_H__
#include "Text.h"
#endif

#if defined(FOR_MAC)
	#ifndef __MACSIMULATOR_H__
	#include "MacSimulator.h"
	#endif
#endif

#if defined(HARDWARE)
	#ifndef __SYSTEM_H__
	#include "System.h"
	#endif
#endif


// ===========================================================================
//	locals/globals/consts
// ===========================================================================

PackedStyle gDefaultStyle = {3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
PackedColorStyle gDefaultColorStyle = { {3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, 0};

const ushort	Document::kDefaultHorizontalMargin = 8;
const ushort	Document::kDefaultVerticalMargin = 6;
const char		gServerMapName[] = "servermap";


// ===========================================================================
//	implementations
// ===========================================================================

Boolean
SelectionsEqual(Selection s1, Selection s2)
{
	return (s1.fPageIndex == s2.fPageIndex &&
			s1.fSelectableIndex == s2.fSelectableIndex);
}

Document::Document()
{
	fBackgroundColor = gSystem->GetDefaultBackgroundColor();
	fLinkColor = gSystem->GetDefaultLinkColor();
	fTextColor = gSystem->GetDefaultTextColor();
	fVisitedLinkColor = gSystem->GetDefaultVisitedLinkColor();
	
	fReloadTime = -1;
	fReloadExpirationTime = (ulong)-1;
	fDisplayOptions = true;
	fEnableOptions = true;
	fDisplayStatus = true;
	
	fHTileBackground = true;
	fVTileBackground = true;
	
	fTransitionEffect = kTransitionCrossFade;
	
	// Set larger increment on selectable list. There's usually quite a few.
	fSelectableList.SetListIncrement(32);
	
	// Prepare to add displayables.
	fRootDisplayable.Open();
}

Document::~Document()
{
	// Notify all forms that we're leaving this document
	long count = fFormList.GetCount();
	for (long i = 0; i < count; i++) {
		Form* form = (Form*)fFormList.At(i);
		if (!IsError(form == nil))
			form->LeaveDocument();
	}
	
	// fFragmentList, fImageList contents are deleted by fRootDisplayable.

	// Delete the ImageMapSelectables from the selectable list. The rest
	// of the selectables are deleted by fRootDisplayayble.
	Selection s;
	
	for (s.fPageIndex = 0; s.fPageIndex < 2; s.fPageIndex++) {
		
		count = GetSelectableCount(s.fPageIndex);
		for (s.fSelectableIndex = 0; s.fSelectableIndex < count; s.fSelectableIndex++)
		{
			Displayable*	selectable = GetSelectable(s);
			if (selectable->IsImageMapSelectable())
				delete(selectable);
		}
	}
	
	fFormList.DeleteAll();
	fImageMapList.DeleteAll();
	fPrefetchList.DeleteAll();
	
	if (GetPriority() > kBackground)
		SetPriority(kBackground);
	
	if (fBackgroundImage)
		delete(fBackgroundImage);

	if (fLogoImage)
		delete(fLogoImage);

	if (fSong != nil)
		delete(fSong);
	
	if (fAudio != nil)
		delete(fAudio);
		
	if (fSideBar != nil)
		delete(fSideBar);

	if (fTextStream != nil)
		delete(fTextStream);
	
	if (fTitle != nil)
		FreeTaggedMemory(fTitle, "Document::fTitle");
		
	if (fReloadURL != nil)
		FreeTaggedMemory(fReloadURL, "Document::fReloadURL");

	if (fHelpURL != nil)
		FreeTaggedMemory(fHelpURL, "Document::fHelpURL");

	if (fCreditsURL != nil)
		FreeTaggedMemory(fCreditsURL, "Document::fCreditsURL");
}

void Document::AddAnchor(Anchor* anchor)
{
	// Maintain a list of named anchors
	
	if (anchor->GetID() != nil)
		fFragmentList.Add(anchor);
}

void Document::AddControl(Control* control)
{
	// Maintain a list of named anchors
	
	if (control->GetID() != nil)
		fFragmentList.Add(control);
}

void Document::AddForm(Form* form)
{
	// Maintain a list of Forms
	
	fFormList.Add(form);
	form->SetDocument(this);
}

void Document::AddImage(Image* image)
{
	// Maintain a list of images
	
	fImageList.Add(image);
	
	image->Load(GetBaseResource());
}

void Document::AddPrefetcher(DocumentPrefetcher* prefetcher)
{
	if (IsError(prefetcher == nil))
		return;
		
	// If a prefetcher already exists for this resource, ignore this one.
	long count = fPrefetchList.GetCount();
	for (long i = 0; i < count; i++) {
		DocumentPrefetcher*	item = (DocumentPrefetcher*)fPrefetchList.At(i);
		if (!IsError(item == nil) && *item->GetResource() == *prefetcher->GetResource()) {
			delete(prefetcher);
			return;
		}
	}
	
	fPrefetchList.Add(prefetcher);
}

void Document::AddSelectable(Displayable* item)
{
	if (item->IsSelectable())
	{
		// The new selectable is inserted into the list in graphically sorted order.
		// Selectables are sorted on Y and then X. Moving through selectable list
		// provides a progression to the right and down.
		Rectangle	itemBounds;
		Rectangle	item1stBounds;
 		Rectangle	selectableBounds;
 		Rectangle	selectable1stBounds;
 		ObjectList*	selectableList = fInSideBarLayout ? &fSideBarSelectableList : &fSelectableList;
		
		item->GetBounds(&itemBounds);
		item->GetFirstRegionBounds(&item1stBounds);
		
		if (EmptyRectangle(&itemBounds))
			return;

		// We iterate backwards because with the exception of tables and image maps,
		// the selectables are already layed out in sorted order.		
		long	i ;
		for (i = selectableList->GetCount(); i > 0; i--)
		{
 			Displayable*	selectable = (Displayable*)selectableList->At(i-1);
 			
			selectable->GetBounds(&selectableBounds);
			
			// Tests for above and below require that the vertical overlap be less than 1/2
			// the height of the smaller item.
			long	minHeight = MIN(RectangleHeight(itemBounds),
									RectangleHeight(selectableBounds)) / 2;

			// First check for below.
			if ((selectableBounds.bottom - itemBounds.top) < minHeight)
				break;
				
			// Check for above
			if ((itemBounds.bottom - selectableBounds.top) < minHeight)
				continue;
			
			// If they also overlap horzontally, we need to use the first region bounds
			// for the spatial comparison.
			if (selectableBounds.right > itemBounds.left && itemBounds.right > selectableBounds.left)
			{
				// They overlap vertically, use left of 1st region for horizontal ordering.
				selectable->GetFirstRegionBounds(&selectable1stBounds);
			
				minHeight = MIN(RectangleHeight(item1stBounds),
								RectangleHeight(selectable1stBounds)) / 2;
	
				// First check for below.
				if ((selectable1stBounds.bottom - item1stBounds.top) < minHeight)
					break;
					
				// Check for above
				if ((item1stBounds.bottom - selectable1stBounds.top) < minHeight)
					continue;	

				if (item1stBounds.left > selectable1stBounds.left)
					break; 
			}	

			else if (itemBounds.left > selectableBounds.left)
				break; 
		}

		TrivialMessage(("Adding Selectable: %ld,%ld,%ld,%ld...",
				 itemBounds.top, itemBounds.left, itemBounds.bottom, itemBounds.right));
		TrivialMessage(("   ...after %ld,%ld,%ld,%ld.",
				 selectableBounds.top, selectableBounds.left, 
				 selectableBounds.bottom, selectableBounds.right));
		selectableList->AddAt(item, i);
	}
}

void Document::AddSideBar(SideBar* sidebar)
{
	// We only allow one sidebar for now. The builder guarantees this.
	if (IsError(fSideBar != nil))
		return;
	
	fSideBar = sidebar;
	SetMargins(fRootDisplayable.GetLeft(), fView->GetTopMargin());
}

void Document::AddSong(const char* songName, ulong loopCount)
{
	if (IsError(songName == nil))
		return;

	if (fSong != nil)
		delete(fSong);
			
	fSong = new(Song);
	fSong->SetAttributeStr(A_SRC, songName);
	fSong->SetLoopCount(loopCount);
	fSong->Load(GetBaseResource());
}

long Document::AddText(const char* text, long textCount)
{
	if (IsError(fTextStream == nil))
		return -1;
		
	long offset = 0;
	
	if (!TrueError(fTextStream->GetStatus())) {
		offset = fTextStream->GetPosition();
		
		// Bump the priority to one below kImmediate so that we can force out
		// other entries of priority lower than kImmediate.
		fTextStream->SetPriority(kSelectable);
		fTextStream->Write(text, textCount);
		fTextStream->SetPriority(Priority(0));
	}
	
	if (TrueError(fTextStream->GetStatus()))
		offset = -1;
		
	PostulateFinal(false);
	// this should be done on a per field basis since it is possible
	// to get mixed documents ( i.e. as a search result )
	// but those documents may not break the text correctly
	
	CharacterEncoding guess = ::GuessCharacterEncoding(text,textCount);
	if ( fCharacterEncoding == kUnknownEncoding )
		fCharacterEncoding = guess;
	else if ( fCharacterEncoding != guess ) 
		TrivialMessage(("Conflicting character encoding guess"));

	return offset;	
}

Boolean Document::CheckReload()
{
	long		delta;

	if (fReloadExpirationTime == (ulong)-1 || (delta = Now() - fReloadExpirationTime) < 0)
		return false;
		
	// time to reload a page, use me if no one else specified
	char* url = fReloadURL != nil ? CopyString(fReloadURL, "Document::CheckReload") : fResource.CopyURL("Document::CheckReload");
	ImportantMessage(("Document: client pull: %s", url));
	fView->ExecuteURL(url, nil);
	fReloadExpirationTime = (ulong)-1;
	FreeTaggedMemory(url, "Document::CheckReload");
	return true;
}


void Document::Draw(const Rectangle* invalid)
{
	// Position sidebar so that is at top of current screen.
	if (fSideBar != nil && fSideBar->IsLayoutComplete()) {
		fSideBar->SetTop(fView->GetScrollPosition() + fView->GetTopMargin());
		fSideBar->Draw(this, invalid);
	}
	
	fRootDisplayable.Draw(this, invalid);
}

#ifdef FIDO_INTERCEPT
void Document::Draw(class FidoCompatibilityState& fidoCompatibility) const
{
	// Position sidebar so that is at top of current screen.
	if (fSideBar != nil && fSideBar->IsLayoutComplete()) {
		fSideBar->SetTop(fView->GetScrollPosition() + fView->GetTopMargin());
		fSideBar->Draw(this, fidoCompatibility);
	}
	
	fRootDisplayable.Draw(this, fidoCompatibility);
}
#endif

void Document::DrawBackground(const Rectangle* invalid)
{
	Rectangle	bounds;
	
	if (!fView->ShouldPaintBackground())
		return;
	
	fHasDrawnBackground = true;
	
	fView->VisibleBackgroundBounds(bounds);
	
	Rectangle	screenBounds = bounds;
	fView->BackgroundToScreen(&screenBounds);
	::PaintRectangle(gScreenDevice, screenBounds, fBackgroundColor, 0, invalid);
					
	gPageBackColor = fBackgroundColor;	// ¥¥¥¥ HACK FOR NOTIFYING XDRAWTEXT OF BackgroundColor ¥¥¥¥
	
	if (fBackgroundImage != nil && fBackgroundImage->IsLayoutComplete()) {					
		Coordinate origin;
		origin.x = fXBackgroundOffset;
		origin.y = fYBackgroundOffset;
		if (!fHTileBackground)
			origin.y = bounds.top;

		fBackgroundImage->DrawTiled(this,bounds,origin,invalid,fVTileBackground,fHTileBackground);
		if (fHTileBackground)
			gPageBackColor = fBackgroundImage->AverageColor(fBackgroundColor);	// ¥¥¥¥ HACK FOR NOTIFYING XDRAWTEXT OF BackgroundColor ¥¥¥¥
	}
}

void 
Document::Finalize()
{
	if (fTextStream != nil && !TrueError(fTextStream->GetStatus()))
		fTextStream->SetStatus(kComplete);
		
	fRootDisplayable.Close();

	// Mark any local image maps we never found as complete.	
	long count = fImageMapList.GetCount();
	for (long i = 0; i < count; i++) 
	{		
		ImageMap* map = (ImageMap*)fImageMapList.At(i);
		if (!IsError(map == nil) && map->IsLocal() && !map->IsComplete())
			map->SetComplete();
	}
	
	fFinalized = true;
}

Displayable*
Document::FindDisplayableAtTextOffset(Displayable* root, long offset)
{
	Displayable*	child;
	
	if (root->IsText()) {
		Text*	text = (Text*)root;
		if (text->IsLayoutComplete() && text->ContainsOffset(offset))
			return root;
	}
		
	for (child = root->GetFirstChild(); child != nil; child = (Displayable*)child->Next()) {
		Displayable* found = FindDisplayableAtTextOffset(child, offset);
		if (found != nil)
			return found;
	}
		
	return nil;
}

Form*
Document::FindForm(const char* formName)
{
	Form* found = nil;
	
	long count = fFormList.GetCount();
	for (long i = 0; i < count && found == nil; i++)
	{
		Form*	form = (Form*)fFormList.At(i);
		if (!IsError(form == nil) && 
		    form->GetName() != nil && strcmp(formName, form->GetName()) == 0)
			found = form;
	}
	
	return found;
}

Displayable*
Document::FindFragment(const char* fragmentName)
{
	Displayable* found = nil;
	
	long count = fFragmentList.GetCount();
	for (long i = 0; i < count && found == nil; i++)
	{
		Displayable*	fragment = (Displayable*)fFragmentList.At(i);
		if (!IsError(fragment == nil) && 
		    fragment->GetID() != nil && strcmp(fragmentName, fragment->GetID()) == 0)
			found = fragment;
	}
	
	return found;
}

ImageMap* 
Document::FindOrAddImageMap(const char* mapName, ImageMapType type)
{
	if (IsError(mapName == nil))
		return nil;
		
	// We need to split the resource URL from the map name fragment.
	const char*	resourceName = mapName;
	Resource	resource;
	
	// Server map
	if (type == kServerMap) {
		// If its a server map, use the map name to find the resource.
		mapName = gServerMapName;			// Server maps are distinguished by resource, not name;
		
		resourceName = strstr(resourceName, "/imagemap/");
		if (resourceName == nil)
			resource = *GetBaseResource();	// Use local resource, no map to load.
		else {			
			resourceName += strlen("/imagemap/");
			resource.SetURL(resourceName, GetBaseResource());
		}
	}
	
	// Client map
	else {
		// If the name starts with '#' or has no '#', then it's a local map.
		mapName = strchr(resourceName, '#');
		if (mapName == nil)
			mapName = resourceName;	

		if (mapName == resourceName)
			resource = *GetBaseResource();
		else {
			char* resourceNameCopy = CopyString(resourceName, "Document::FindOrAddImage");
			*strchr(resourceNameCopy, '#') = '\0';	// Strip fragment.
			
			resource.SetURL(resourceNameCopy, GetBaseResource());
			FreeTaggedMemory(resourceNameCopy, "Document::FindOrAddImage");
		}	
		
		// skip the '#'
		if (*mapName == '#')
			mapName++;
			
		// If map name is empty, skip it.
		if (*mapName == '\0')
			return nil;							
	}
		
	ImageMap*	map = FindOrAddImageMap(&resource, mapName, type);
	
	return map;
}

ImageMap* 
Document::FindOrAddImageMap(const Resource* resource, const char* mapName, ImageMapType type)
{
	if (IsError(resource == nil || mapName == nil))
		return nil;
		
	ImageMap* found = nil;
	
	long count = fImageMapList.GetCount();
	for (long i = 0; i < count && found == nil; i++)
	{
		ImageMap*	map = (ImageMap*)fImageMapList.At(i);
		if (!IsError(map == nil) &&
			strcmp(mapName, map->GetName()) == 0 && *resource == *map->GetResource())
			found = map;
	}
	
	// Didn't find it? Add it.
	if (found == nil)
	{	
		found = new(ImageMap);
		found->SetResourceAndName(resource, mapName);
		found->SetType(type);
		fImageMapList.Add(found);
		
		// If map is from another resource, load it.
		if (!(*resource == *GetBaseResource()))
			found->Load();
			
		// If its a local server map, mark it complete, we couldn't find it.
		else if (found->GetType() == kServerMap)
			found->SetComplete();
	}
	
	return found;
}

Displayable*
Document::FindString(const char* target)
{
	if (IsError(fTextStream == nil || target == nil))
		return nil;
		
	// Find target string in document text, ignoring case.
	const char* text = fTextStream->GetData();
	long textLength = fTextStream->GetDataLength();
	long targetLength = strlen(target);
	long i;
	
	// Start at the result of the last find.
	for (i = fTextHighlightEnd; textLength - i >= targetLength; i++)
		if (EqualStringN(&text[i], target, targetLength))
			break;

	// If we're beyond a possible match, and we didn't start at the beginning, wrap.
	if (textLength - i < targetLength && fTextHighlightEnd > 0)
		for (i = 0; textLength - i >= targetLength; i++)
			if (EqualStringN(&text[i], target, targetLength))
				break;
		
	// If we're beyond a possible match, return nil. 
	if (textLength - i < targetLength)
		return nil;
		
	// We found the match! Save the highlight range and find the starting text 
	// displayable that contains the starting text offset.
	fTextHighlightStart = i;
	fTextHighlightEnd = i + targetLength;
	
	return FindDisplayableAtTextOffset(&fRootDisplayable, i);
}

const
Resource* Document::GetBaseResource() const
{
	return fBaseResource.HasURL() ? &fBaseResource : &fResource;
}

Color
Document::GetBackgroundColor() const
{
	return fBackgroundColor;
}

Color
Document::GetColor(PackedStyle style) const
{
	if (style.visited)
		return fVisitedLinkColor;
	else if (style.anchor)
		return fLinkColor;
	else
		return fTextColor;
}

XFont
Document::GetFont(PackedStyle style) const
{
	FontSize size = (style.monoSpaced ?
							gSystem->GetFontMonospacedSizeRecord() :
							gSystem->GetFontProportionalSizeRecord()
					) -> size[style.fontSize];
	FontStyle face = kNormalStyle;
	ulong font;

	if (style.bold)
		face |= kBoldStyle;

	if (style.italic)
		face |= kItalicStyle;

	if (style.underline)
		face |= kUnderlineStyle;
		
	if (style.shadow)
		face |= kShadowStyle;
		
	switch (style.effect) {
		case kReliefEffect:		face |= kReliefStyle;	break;
		case kEmbossEffect:		face |= kEmbossStyle;	break;
		case kOutlineEffect:	face |= kOutlineStyle;	break;
	}

	font = style.monoSpaced ? gSystem->GetFontMonospaced() : gSystem->GetFontProportional();

	return ::GetFont(font, size, face);
}

Form*
Document::GetForm(long index) const
{
	return (Form*)fFormList.At(index);
}

ulong
Document::GetFormCount() const
{
	return fFormList.GetCount();
}

long
Document::GetHeight() const
{
	// Return 0 for height if waiting for sidebar layout to complete.
	if (fSideBar != nil && !fSideBar->IsLayoutComplete())
		return 0;
		
	return fRootDisplayable.GetTop() + fRootDisplayable.GetHeight();
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
Color
Document::GetLinkColor() const
{
	return fLinkColor;
}
#endif

long
Document::GetNoScrollWidth() const
{
	if (fBackgroundImage != nil && !fHTileBackground)
		return fBackgroundImage->GetWidth();
	
	if (fSideBar != nil)
		return fSideBar->GetWidth();
		
	return 0;
}

uchar
Document::GetPercentComplete() const
{
	// Only count child resources. The actual html is counted
	// in ContentView because it knows about the resource...
	
	if (!fFinalized)
		return 0;
	
	ulong count = fImageList.GetCount();
	ulong total = 0;
	
	if (count == 0 && fBackgroundImage == nil)
		return 100;

	for (ulong i = 0; i < count; i++)
	{
		Image* image = (Image*)fImageList.At(i);
		total += image->GetPercentComplete(kDefaultImageExpectedSize);
	}
	
	if (fBackgroundImage != nil) {
		count++;
		total += fBackgroundImage->GetPercentComplete(kDefaultImageExpectedSize);
	}
		
	if (total == 0)
		return 0;

	return total / count;
}

RootPage*
Document::GetRootDisplayable() const
{
	Document* This = (Document*)this;
	return &This->fRootDisplayable;
}

Displayable*
Document::GetSelectable(Selection selection) const
{
	if (IsError(selection.fPageIndex > 1))
		return nil;
		
	if (selection.fSelectableIndex < 0)
		return nil;
		
	if (selection.fPageIndex == 1) {
		if (selection.fSelectableIndex >= fSideBarSelectableList.GetCount())
			return nil;
		return (Displayable*)fSideBarSelectableList.At(selection.fSelectableIndex);	
	}
	
	if (selection.fSelectableIndex >= fSelectableList.GetCount())
		return nil;
	
	return (Displayable*)fSelectableList.At(selection.fSelectableIndex);
}

ulong
Document::GetSelectableCount(long pageIndex) const
{
	if (IsError(pageIndex > 1))
		return 0;
	
	if (pageIndex == 1)
		return fSideBarSelectableList.GetCount();
		
	return fSelectableList.GetCount();
}

const char*
Document::GetText(long offset) const
{
	return fTextStream->GetData() + offset;
}

void
Document::GetTextHighlightRange(long* start, long* end) const
{
	*start = fTextHighlightStart;
	*end = fTextHighlightEnd;
}

Boolean
Document::GetSkipBack() const
{
	return fSkipBack || fReloadTime == 0;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
Color
Document::GetTextColor() const
{
	return fTextColor;
}
#endif

const char*
Document::GetTitle() const
{
	return fTitle;
}

char*
Document::CopyURL(const char* tagName) const
{
	return fResource.CopyURL(tagName);
}

ContentView*
Document::GetView() const
{
	return fView;
}

Error
Document::GetVisibleImageStatus(const Rectangle* visibleBounds) const
{
	if (fBackgroundImage != nil && fBackgroundImage->GetStatus() >= 0 && fBackgroundImage->GetStatus() != kComplete)
		return kPending;

	if (fSong != nil && fSong->GetStatus() != kComplete)
		return kPending;
					
	long count = fImageList.GetCount();
	for (long i = 0; i < count; i++) 
	{		
		Image* image = (Image *)fImageList.At(i);
		if (IsError(image == nil))
			continue;
		
		if (!image->IsLayoutComplete())		// Nothing layed out from here.
			break;
			
		Rectangle imageBounds;
		image->GetBoundsTight(&imageBounds);
		
		if (imageBounds.top >= visibleBounds->bottom)
			break;
			
		if (RectanglesIntersect(visibleBounds, &imageBounds))
			if (image->GetStatus() >= 0 && image->GetStatus() != kComplete)
				return kPending;
	}

	return kComplete;				
}


void
Document::AddAudio(const char *linkURL,const DataType dataType)
{
	if ( fAudio )
		delete fAudio;
	fAudio = nil;
	
	fAudio = AudioStream::NewAudioStream(dataType);
	
	if ( linkURL   )
	{
		Resource resource;
		
		resource.SetURL(linkURL, GetBaseResource());
		if (dataType == kDataTypeRealAudioProtocol) {
			resource.SetStatus(kComplete);
		} else {
			resource.SetPriority(kImmediate);
		}
		if ( fAudio )
		{
			fAudio->SetResource(&resource);
		}
	}
}
 
Boolean
Document::Idle(Layer* layer)
{
	PerfDump perfdump("Document::Idle");

	// Idle images and text fields
	
	Boolean changed = false;
	long count;
	long i;
	
	// If time for reload, don't bother with the rest.
	if (CheckReload())
		return false;
		
	UpdatePriorities();
	
	count = fImageList.GetCount();
	for (i = 0; i < count; i++) 
	{		
		Image* image = (Image *)fImageList.At(i);
		if (!IsError(image == nil))
			changed |= image->Idle(layer);
	}
	
	count = fImageMapList.GetCount();
	for (i = 0; i < count; i++) 
	{		
		ImageMap* map = (ImageMap*)fImageMapList.At(i);
		if (!IsError(map == nil))
			changed |= map->Idle();
	}
	
	if (fSong != nil)
		fSong->Idle();
				
	if ( fAudio )
		fAudio->Idle();
			
	count = fFormList.GetCount();
	for (i = 0; i < count; i++) 
	{		
		Form* form = (Form*)fFormList.At(i);
		if (!IsError(form == nil))
			form->Idle(layer);
	}
		
	count = fPrefetchList.GetCount();
	for (i = count-1; i >= 0; i--) // Count backwards to allow deletion.
	{		
		DocumentPrefetcher* prefetcher = (DocumentPrefetcher*)fPrefetchList.At(i);
		if (!IsError(prefetcher == nil)) {
			prefetcher->Idle();
			if (prefetcher->GetStatus() == kComplete)
				fPrefetchList.DeleteAt(i);
		}
	}
		
	IdleBackground(layer);

	if (fLogoImage != nil)
		fLogoImage->Idle(layer);

	return changed;
}

void
Document::IdleBackground(Layer* layer)
{
	if (fBackgroundImage != nil) {
		fBackgroundImage->Idle(layer);
	
		if (fBackgroundImage->ReadyForLayout(this) && !fBackgroundImage->IsLayoutComplete() &&
		    fBackgroundImage->GetStatus() == kComplete) {
			fBackgroundImage->Layout(this, nil);
			fBackgroundImage->LayoutComplete(this, nil);
			
			if (fHasDrawnBackground) {
				layer->InvalidateBounds();
				if (layer == (Layer*)gPageViewer) {
					gPageViewerTransitionType = kTransitionCrossFade;
					gPageViewerTransitionDelay = 1;
				}
			}
		}
		
		if (fBackgroundImage->GetStatus() == kComplete &&
			(fXBackgroundSpeed != 0 || fYBackgroundSpeed != 0) && fView->Completed()) {
			
			fXBackgroundOffset += fXBackgroundSpeed;
			fYBackgroundOffset += fYBackgroundSpeed;
			
			// Limit offsets by image size so we don't scroll off to infinite.
			Rectangle	imageBounds;
			fBackgroundImage->GetBoundsTight(&imageBounds);
			fXBackgroundOffset %= (imageBounds.right - imageBounds.left);
			fYBackgroundOffset %= (imageBounds.bottom - imageBounds.top);
			
			layer->InvalidateBounds();
		}
	}
}	

Boolean
Document::IsLayoutCurrent() const
{
	return (fRootDisplayable.IsLayoutCurrent(this) && 
			(fSideBar == nil || fSideBar->IsLayoutComplete()));
}

Error
Document::GetStatus() const
{
	if (!fFinalized)
		return kPending;

	if (fLogoImage != nil && fLogoImage->GetStatus() >= 0 && fLogoImage->GetStatus() != kComplete)
		return kPending;

	if (fBackgroundImage != nil && fBackgroundImage->GetStatus() >= 0 && fBackgroundImage->GetStatus() != kComplete)
		return kPending;

	if (fSong != nil && fSong->GetStatus() >= 0 && fSong->GetStatus() != kComplete)
		return kPending;
					
	ulong	count = fImageList.GetCount();		
	for (ulong i = 0; i < count; i++) 
	{		
		Image* image = (Image*)(fImageList.At(i));		
		Error status = image->GetStatus();
		
		if (status == kNoError)
			return kNoError;
		if (status == kPending)
			return kPending;
	}
	
	return IsLayoutCurrent() ? kComplete : kPending;
}

void
Document::Layout()
{
	if (fSideBar != nil && !fSideBar->IsLayoutComplete() && fSideBar->ReadyForLayout(this)) {
		fInSideBarLayout = true;
		fSideBar->Layout(this, nil);
		fSideBar->LayoutComplete(this, nil);
		fInSideBarLayout = false;
	}

	fRootDisplayable.Layout(this, nil);

	// Align the page vertically if it is smaller than the view.
	if (fRootDisplayable.GetVAlign() != AV_TOP && fFinalized && IsLayoutCurrent())
	{
		long pageHeight = fRootDisplayable.GetHeight();
		Rectangle	viewBounds;
		fView->VisibleContentBounds(viewBounds);
		long viewHeight = viewBounds.bottom - viewBounds.top;

		if (pageHeight < viewHeight)
		{
			long	newTop = fRootDisplayable.GetTop();

			switch(fRootDisplayable.GetVAlign())
			{
				case AV_CENTER:
					newTop = (viewHeight - pageHeight) / 2;
					break;
				case AV_BOTTOM:
					newTop = (viewHeight - pageHeight);
				default:
					break;
			}

			fRootDisplayable.SetTop(newTop);
		}
	}
}

void
Document::Relayout()
{
	if (IsError(fRootDisplayable.IsOpen()))
		return;
		
	// Delete the ImageMapSelectables from the selectable list. Layout will add them
	// again with their new positions.
	Selection s;
	
	for (s.fPageIndex = 0; s.fPageIndex < 2; s.fPageIndex++) {
		
		long count = GetSelectableCount(s.fPageIndex);
		for (s.fSelectableIndex = 0; s.fSelectableIndex < count; s.fSelectableIndex++)
		{
			Displayable*	selectable = GetSelectable(s);
			if (selectable->IsImageMapSelectable())
				delete(selectable);
		}
	}
	fSelectableList.RemoveAll();
	fSideBarSelectableList.RemoveAll();
	
	fRootDisplayable.ResetLayout(this);
	if (fSideBar != nil)
		fSideBar->ResetLayout(this);
	Layout();
}

void
Document::ResetResources()
{
	// Reset all the resources used by this document.
	
	long 		count;
	long 		i;
	Resource	resource;
	
	fResource.Purge();
	
	count = fImageList.GetCount();
	for (i = 0; i < count; i++) {
		Image* image = (Image *)fImageList.At(i);
		if (!IsError(image == nil))
			image->PurgeResource();
	}
	
	count = fImageMapList.GetCount();
	for (i = 0; i < count; i++){		
		ImageMap* map = (ImageMap*)fImageMapList.At(i);
		if (!IsError(map == nil)) {
			resource = *map->GetResource();
			resource.Purge();
		}
	}
	
	if (fSong != nil) {
		resource = *fSong->GetResource();
		resource.Purge();
	}
					
	if (fAudio != nil) {
		resource = *fAudio->GetResource();
		resource.Purge();
	}
				
	if (fBackgroundImage != nil) {
		resource = *fBackgroundImage->GetResource();
		resource.Purge();
	}
				
	if (fLogoImage != nil) {
		resource = *fLogoImage->GetResource();
		resource.Purge();
	}
}
	
void
Document::SetAttribute(Attribute attributeID, long value, Boolean)
{
	switch (attributeID) 
	{			
		case A_LINK:		if (value >= 0) fLinkColor = value; break;
		case A_VLINK:		if (value >= 0) fVisitedLinkColor = value; break;
		case A_BGCOLOR:		if (value >= 0) fBackgroundColor = value; break;
		case A_TEXT:		if (value >= 0) fTextColor = value; break;
		case A_XSPEED:		fXBackgroundSpeed = value; break;
		case A_YSPEED:		fYBackgroundSpeed = value; break;
		case A_HSPACE:
			if (fView != nil) {
				value = MAX(value, 0);
				SetMargins(value, fView->GetTopMargin());
			}
			break;
		case A_VSPACE: 		if (fView != nil) fView->SetTopMargin(MAX(value, 0)); break;
		case A_VALIGN:		if (value > 0) fRootDisplayable.SetVAlign((AttributeValue)value); break;
		case A_NOHTILEBG:	fHTileBackground = false; break;
		case A_NOVTILEBG:	fVTileBackground = false; break;
		case A_NOOPTIONS:	fEnableOptions = false; break;
		case A_HIDEOPTIONS:	fDisplayOptions = false; break;
		case A_NOSTATUS:	fDisplayStatus = false; break;
		case A_TRANSITION:	
			switch (value) {
				case AV_BLACKFADE:			fTransitionEffect = kTransitionBlackFade;		break;
				case AV_CROSSFADE:			fTransitionEffect = kTransitionCrossFade;		break;
				case AV_WIPELEFT:			fTransitionEffect = kTransitionWipeLeft;		break;
				case AV_WIPERIGHT:			fTransitionEffect = kTransitionWipeRight;		break;
				case AV_WIPEUP:				fTransitionEffect = kTransitionWipeUp;			break;
				case AV_WIPEDOWN:			fTransitionEffect = kTransitionWipeDown;		break;
				case AV_WIPELEFTTOP:		fTransitionEffect = kTransitionWipeLeftTop;		break;
				case AV_WIPERIGHTTOP: 		fTransitionEffect = kTransitionWipeRightTop;	break;
				case AV_WIPELEFTBOTTOM:		fTransitionEffect = kTransitionWipeLeftBottom;	break;
				case AV_WIPERIGHTBOTTOM:	fTransitionEffect = kTransitionWipeRightBottom;	break;
				case AV_SLIDELEFT:			fTransitionEffect = kTransitionSlideLeft;		break;
				case AV_SLIDERIGHT:			fTransitionEffect = kTransitionSlideRight;		break;
				case AV_SLIDEUP:			fTransitionEffect = kTransitionSlideUp;			break;
				case AV_SLIDEDOWN:			fTransitionEffect = kTransitionSlideDown;		break;
				case AV_PUSHLEFT:			fTransitionEffect = kTransitionPushLeft;		break;
				case AV_PUSHRIGHT:			fTransitionEffect = kTransitionPushRight;		break;
				case AV_PUSHUP:				fTransitionEffect = kTransitionPushUp;			break;
				case AV_PUSHDOWN:			fTransitionEffect = kTransitionPushDown;		break;
				case AV_ZOOMINOUT:			fTransitionEffect = kTransitionZoomInZoomOut;	break;
				case AV_ZOOMIN:				fTransitionEffect = kTransitionZoomIn;			break;
				case AV_ZOOMOUT:			fTransitionEffect = kTransitionZoomOut;			break;
				case AV_ZOOMINOUTH:			fTransitionEffect = kTransitionZoomInZoomOutH;	break;
				case AV_ZOOMINH:			fTransitionEffect = kTransitionZoomInH;			break;
				case AV_ZOOMOUTH:			fTransitionEffect = kTransitionZoomOutH;		break;
				case AV_ZOOMINOUTV:			fTransitionEffect = kTransitionZoomInZoomOutV;	break;
				case AV_ZOOMINV:			fTransitionEffect = kTransitionZoomInV;			break;
				case AV_ZOOMOUTV:			fTransitionEffect = kTransitionZoomOutV;		break;
				default:
					break;
			}
		case A_STAYTIME:	fStayTime = value; break;
		case A_CLEARBACK:	gPageViewer->ClearBackList();
							gRecentPanel->ClearPages();			// Fall through to skip-back also.
		case A_SKIPBACK:	fSkipBack = true; break;
		default:								break;
	}
}

void
Document::SetAttributeStr(Attribute attributeID, const char* value)
{
	if (value == nil)
		return;
	
	switch (attributeID)
	{		
		case A_BACKGROUND:
		{
			Image* image = new(Image);
			image->SetAttributeStr(A_SRC, value);
			image->SetIsBackground(true);
			image->Load(GetBaseResource());
			fBackgroundImage = image;
			fBackgroundImage->SetPriority(GetPriority());
			Message(("Background Image is %s", value));
		}
			break;
		case A_LOGO:
		{
			Image* logo = new(Image);
			logo->SetAttributeStr(A_SRC, value);
			logo->Load(GetBaseResource());
			fLogoImage = logo;
			fLogoImage->SetPriority(GetPriority());
			Message(("Logo Image is %s", value));
		}
			break;
		case A_HREF:
			fBaseResource.SetURL(value);
			break;
		case A_HELP:
			fHelpURL = CopyString(value, "Document::fHelpURL");
			break;
		case A_CREDITS:
			fCreditsURL = CopyString(value, "Document::fCreditsURL");
			break;
		default:
			break;
	}
}

void
Document::SetBackgroundColor(Color color)
{
	fBackgroundColor = color;
}

void
Document::SetLinkColor(Color color)
{
	fLinkColor = color;
}

void
Document::SetMargins(ushort horizontalMargin, ushort verticalMargin)
{
	if (IsError(fView == nil))
		return;
	
	fView->SetTopMargin(verticalMargin);
	fRootDisplayable.SetLeft(horizontalMargin + (fSideBar != nil ? fSideBar->GetWidth() : 0));
	fRootDisplayable.SetWidth(fView->GetWidth() - fRootDisplayable.GetLeft() - horizontalMargin);
}

void
Document::SetPriority(Priority priority)
{
	long count;
	long i;
	
	fPriority = priority;
	
	fResource.SetPriority(priority);
	
	if (fBackgroundImage != nil)
		fBackgroundImage->SetPriority(priority);
	
	if (fLogoImage != nil)
		fLogoImage->SetPriority(priority);
		
	count = fImageList.GetCount();
	for (i = 0; i < count; i++) 
	{		
		Image* image = (Image*)fImageList.At(i);
		if (image != nil && image->GetPriority() != kPersistent && image->GetPriority() > priority)
			image->SetPriority(priority);
	}

	count = fImageMapList.GetCount();
	for (i = 0; i < count; i++) 
	{		
		ImageMap* imageMap = (ImageMap*)fImageMapList.At(i);
		if (imageMap != nil && imageMap->GetPriority() != kPersistent && imageMap->GetPriority() > priority)
			imageMap->SetPriority(priority);
	}
	
	if (fSong != nil)
		fSong->SetPriority(priority);
}

void
Document::SetReloadTime(short newValue)
{
 	fReloadTime = newValue;
 	fReloadExpirationTime = (ulong)-1;
}
 
void
Document::SetReloadURL(const char* url)
{
	fReloadURL = CopyString(url, "Document::fReloadURL");
}

void
Document::SetResource(const Resource* resource)
{
	static ulong	gTextSequence;
	char	extension[32];

	fResource = *resource;
	
	// Create a unique extension for the text name, in case the same document is used in two views.
	snprintf(extension, sizeof(extension), ".%ld.%s", gTextSequence++, "text");
	
	// Create the text stream
	Resource	textResource;
	char*		textURL = NewLocalURL(resource, "cache:", extension, "Document::SetResource");
	textResource.SetURL(textURL);
	textResource.SetDataType(kDataTypeTEXT);
	fTextStream = textResource.NewStreamForWriting();
	
	IsError(fTextStream == nil);
	
	FreeTaggedMemory(textURL, "Document::SetResource");
}

Song*
Document::RemoveSong()
{
	Song*	song = fSong;
	fSong = nil;
	
	return song;
}

void
Document::SetTextColor(Color value)
{
	fTextColor = value;
}

void
Document::SetTitle(const char* value)
{
	fTitle = CopyStringTo(fTitle, value, "Document::fTitle");
}

void
Document::SetView(ContentView* view)
{
	fView = view;

	SetMargins(kDefaultHorizontalMargin, kDefaultVerticalMargin);
}

void
Document::SetVisitedLinkColor(Color color)
{
	fVisitedLinkColor = color;
}

void
Document::StartAnimations()
{
	long count = fImageList.GetCount();
	long i;
	Rectangle	visibleBounds;
	fView->VisibleContentBounds(visibleBounds);
		
	// Make a first pass through the images performing an initial draw on any animations, on 
	// screen or not.
	for (i = 0; i < count; i++) 
	{		
		Image* image = (Image *)fImageList.At(i);
		if (IsError(image == nil))
			continue;
		if (image->IsAnimation())
			image->Draw(this, &visibleBounds);
	}
			
}

void
Document::StartReloadTiming()
{
	if (fReloadTime >= 0) {
		fReloadExpirationTime = Now() + fReloadTime*kOneSecond;

		// Prefetch the page while we wait to load.
		if (fReloadURL != nil) {
			DocumentPrefetcher* prefetcher = new(DocumentPrefetcher);
			prefetcher->Load(fReloadURL, GetBaseResource());
			AddPrefetcher(prefetcher);
		}
		
		fDisplayStatus = false;
	}
}

void
Document::UpdatePriorities()
{
	if (GetPriority() <= kBackground)
		return;
			
	Rectangle visibleBounds;
	fView->VisibleContentBounds(visibleBounds);

	// Keep the background image at kImmediate.
	if (fBackgroundImage != nil)
		fBackgroundImage->SetPriority(kImmediate);
	
	// Nearby bounds include current screen, screen before, and screen after.	
	Rectangle nearbyBounds = visibleBounds;
	nearbyBounds.bottom += visibleBounds.bottom - visibleBounds.top;
	nearbyBounds.top -= visibleBounds.bottom - visibleBounds.top;
	nearbyBounds.top = MAX(nearbyBounds.top, 0);
	
	// We set the priorities of all layed out images to visible, nearby or
	// background as determined by their position relative to the current visible
	// content area of the screen. We set the priority of the next image
	// blocking layout to kImmediate.
	long count = fImageList.GetCount();
	long immediateCount = 2;
	long i;
		
	// Make a first pass through setting priorities to background, to avoid lowering priorities of 
	// images that occur multiple times in the document.
	for (i = 0; i < count; i++) 
	{		
		Image* image = (Image *)fImageList.At(i);
		if (IsError(image == nil))
			continue;
		if (image->GetPriority() < kPersistent)
			image->SetPriority(kBackground);
	}
	
	for (i = 0; i < count; i++) 
	{		
		Image* image = (Image *)fImageList.At(i);
		if (IsError(image == nil))
			continue;
			
		if (image->IsLayoutComplete())
		{
			Rectangle imageBounds;
			image->GetBoundsTight(&imageBounds);
			
			if (RectanglesIntersect(&visibleBounds, &imageBounds))
				image->SetPriority(MAX(image->GetPriority(), kVisible)); // Use MAX to avoid lowering priority set
			else if (RectanglesIntersect(&nearbyBounds, &imageBounds))	 // by earlier image using same resource.
				image->SetPriority(MAX(image->GetPriority(), kNearby));
		}
		else if (visibleBounds.bottom > GetHeight() && immediateCount != 0)
		{
			image->SetPriority(MAX(image->GetPriority(), kImmediate));
			immediateCount--;
		}
	}
}


CharacterEncoding 
Document::GetCharacterEncoding() const
{
	return fCharacterEncoding;
}


#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void 
Document::SetCharacterEncoding(CharacterEncoding charset ) 
{
	fCharacterEncoding = charset;
}
#endif

