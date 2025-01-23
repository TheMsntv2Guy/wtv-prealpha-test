// ===========================================================================
//	ImageMap.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __IMAGEMAP_H__
#define __IMAGEMAP_H__

#ifndef __CACHEENTRY_H__
#include "CacheEntry.h"
#endif
#ifndef __DISPLAYABLE_H__
#include "Displayable.h"
#endif
#ifndef __GRAPHICS_H__
#include "Graphics.h"
#endif
#ifndef __IMAGEDATA_H__
#include "ImageData.h"
#endif
#ifndef __LIST_H__
#include "List.h"
#endif
#ifndef __PARSER_H__
#include "Parser.h"
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"
#endif

class ContentView;
class DataStream;
class ImageMapBuilder;
class Image;

// =============================================================================

enum ImageMapIdleState {
	kCreateImageMapStream = 0,
	kParseImageMap,
	kWaitForLocalImageMap,
	kImageMapDone,
	kImageMapError
};
	
class Area : public HasHTMLAttributes {
public:
							Area();
	virtual		 			~Area();
	
	virtual void			GetBounds(Rectangle*) const;
	const char*				GetHREF() const;
	AttributeValue			GetShape() const;
	Boolean					HasHREF() const;
	
	virtual void			SetAttribute(Attribute attributeID, long value, Boolean isPercentage);
	virtual void			SetAttributeStr(Attribute attributeID, const char* value);
	
protected:
	char*					fHREF;

	short					fLeft;
	short					fTop;
	short					fRight;
	short					fBottom;

	AttributeValue			fShape;	
};

// =============================================================================

enum ImageMapType {
	kClientMap = 0,
	kServerMap
};

class ImageMap : public Listable {
public:
							ImageMap();
	virtual		 			~ImageMap();
	
	void					AddArea(Area*);
	Area*					AreaAt(long);
	long					GetAreaCount() const;	
	const Resource*			GetBaseResource() const;
	const char*				GetName() const;
	Priority				GetPriority() const;
	const Resource*			GetResource() const;
	ImageMapType			GetType() const;
	
	Boolean					Idle();
	Boolean					IsComplete() const;
	Boolean					IsLocal() const;
	void					Load();

	void					SetBaseURL(const char* url);
	void					SetComplete();				
	void					SetPriority(Priority);
	void					SetResourceAndName(const Resource*, const char*);
	void					SetType(ImageMapType);
	
protected:
	void 					CleanupStream();
	void					HandleRedirect();
	void					Reset();

	ObjectList				fAreaList;
	Resource				fBaseResource;
	Resource				fResource;
	ImageMapBuilder*		fBuilder;
	char*					fName;
	Parser*					fParser;
	ImageMapIdleState		fState;
	DataStream*				fStream;
	
	ImageMapType			fType;	
	Boolean					fLocal;
};

// =============================================================================

class ImageMapSelectable : public Displayable {
public:
							ImageMapSelectable();
	virtual		 			~ImageMapSelectable();
	
	virtual Boolean			BumpSelectionPosition(Coordinate*, const Coordinate*,
												  ContentView*);											    
	virtual Boolean			ExecuteInput();
	
	virtual Image*			GetMappedImage() const;
	virtual void			GetBounds(Rectangle*) const;

	Area*					GetArea();
	Rectangle				GetCursorBounds(const Coordinate* position) const;
	virtual Displayable*	GetParent() const;
	Displayable*			GetSelectable();
	virtual void			GetSelectionRegion(Region*) const;
	Coordinate				GetTargetPoint(Coordinate) const;
		
	virtual Boolean			HasURL() const;
	virtual Boolean			IsImageMapSelectable() const;
	virtual Boolean			IsInitiallySelected() const;
	virtual	Boolean			IsSelectable() const;

	virtual Boolean			KeyboardInput(Input*);

	virtual char*			NewURL(const Coordinate*, const char* tag) const;

	void					SetArea(Area*);
	void					SetSelectable(Displayable*);
	void					SetInitiallySelected(Boolean selected = true);
	
protected:
	
	Area*					fArea;
	Displayable*			fSelectable;

	Boolean					fIsInitiallySelected;
};

// =============================================================================

class ImageMapCursor : public GIFImage {
public:
	void					GetHotspot(Coordinate*) const;
	void					SetHotspot(const Coordinate*);
	void					SetHotspot(ulong x, ulong y);
	
	static ImageMapCursor*	NewImageMapCursor(const char* url);
	
protected:
	Coordinate				fHotspot;
};


// =============================================================================

inline const char* Area::GetHREF() const
{
	return fHREF;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
inline AttributeValue Area::GetShape() const
{
	return fShape;
}
#endif

inline Boolean Area::HasHREF() const
{
	return fHREF != nil;
}

inline Area* ImageMap::AreaAt(long index)
{
	return (Area*)fAreaList.At(index);
}

inline long ImageMap::GetAreaCount() const
{
	return fAreaList.GetCount();
}

inline const char* ImageMap::GetName() const
{
	return fName;
}

inline Priority ImageMap::GetPriority() const
{
	return fResource.GetPriority();
}

inline const Resource* ImageMap::GetResource() const
{
	return &fResource;
}

inline ImageMapType ImageMap::GetType() const
{
	return fType;
}

inline Boolean ImageMap::IsComplete() const
{
	return fState >= kImageMapDone;
}

inline Boolean ImageMap::IsLocal() const
{
	return fLocal;
}

inline void ImageMap::SetComplete()
{
	if (fState == kWaitForLocalImageMap) fState = kImageMapDone;
}

inline void ImageMap::SetPriority(Priority priority)
{
	fResource.SetPriority(priority);
}

inline void ImageMap::SetType(ImageMapType type)
{
	fType = type;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
inline Displayable* ImageMapSelectable::GetSelectable()
{
	return fSelectable;
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
inline Area* ImageMapSelectable::GetArea()
{
	return fArea;
}
#endif

inline void ImageMapSelectable::SetSelectable(Displayable* selectable)
{
	fSelectable = selectable;
}

inline void ImageMapSelectable::SetArea(Area* area)
{
	fArea = area;
}

inline void ImageMapSelectable::SetInitiallySelected(Boolean selected)
{
	fIsInitiallySelected = selected;
}

// =============================================================================

inline void ImageMapCursor::GetHotspot(Coordinate* point) const		{*point = fHotspot; }
inline void ImageMapCursor::SetHotspot(const Coordinate* point)		{fHotspot = *point; }
inline void ImageMapCursor::SetHotspot(ulong x, ulong y)			{fHotspot.x = x; fHotspot.y = y; }

// =============================================================================

#endif /* __IMAGEMAP_H__ */
