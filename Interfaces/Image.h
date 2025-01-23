// ===========================================================================
//	Image.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __IMAGE_H__
#define __IMAGE_H__

#ifndef __CACHEENTRY_H__
#include "CacheEntry.h"			/* for Priority */
#endif
#ifndef __DISPLAYABLE_H__
#include "Displayable.h"			/* Displayable, SpatialDisplayable */
#endif
#ifndef __ERRORNUMBERS_H__
#include "ErrorNumbers.h"			/* Error */
#endif
#ifndef __GRAPHICS_H__
#include "Graphics.h"				/* Color, Rectangle */
#endif
#ifndef __PARSER_H__
#include "Parser.h"					/* for Attribute */
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"				/* Resource */
#endif




class Document;
class ImageData;
class ImageMap;
class Layer;


// =============================================================================

enum ImageIdleState {
	kCreateImageData = 0,
	kDetermineImageSize,
	kWaitForImageLayout,
	kDrawImage,
	kImageError
};

class Image : public SpatialDisplayable 
{
public:
							Image();
	virtual 				~Image();

	virtual Color			AverageColor(ulong backgroundColor);
	virtual AttributeValue	GetAlign() const;
	virtual void			GetBoundsTight(Rectangle*) const;
	virtual long			GetHeight() const;
	ImageMap*				GetImageMap() const;
	virtual Image*			GetMappedImage() const;
	virtual short			GetMinUsedWidth(const Document*) const;
	virtual Displayable*	GetParent() const;
	virtual uchar			GetPercentComplete(ulong dataExpected) const;
	Priority				GetPriority() const;
	const Resource*			GetResource() const;
	Error					GetStatus() const;
	void					GetUnscaledBoundsTight(Rectangle*) const;
	const char*				GetUseMap() const;
	virtual long			GetWidth() const;
	virtual Boolean			IsAnimation() const;
	Boolean					IsBackground() const;
	virtual Boolean			IsImage() const;
	virtual Boolean			IsFloating() const;
	Boolean					IsDrawingComplete() const;
	Boolean					IsMap() const;
	Boolean					KnowSize() const;
	
	virtual void			SetAttribute(Attribute attributeID, long value, Boolean isPercentage);
	virtual void			SetAttributeStr(Attribute attributeID, const char* value);
	void					SetImageMap(ImageMap* map);
	void					SetIsBackground(Boolean background);
	void					SetKeepBitMap(Boolean keep);
	void					SetParent(Displayable* parent);
	void					SetPriority(Priority);

	virtual void			Draw(const Document* document, const Rectangle* invalid);
			void			DrawBorder(const Document* document, const Rectangle* invalid);
			void			DrawOutline(const Document* document, const Rectangle* invalid);
			
	void 					DrawTiled(const Document* document,Rectangle tileRect,Coordinate origin,const Rectangle* invalid = nil,Boolean tileVertical = true,Boolean tileHorizontal= true, ulong transparency = kNotTransparent);
	virtual Boolean			Idle(Layer* layer);
	void					Show();
	void					Hide();
	
	virtual Boolean			ReadyForLayout(const Document*);
	virtual void			Layout(Document*, Displayable* parent);
	virtual void			LayoutComplete(Document*, Displayable* parent);
	virtual Boolean			IsLayoutComplete() const;
	virtual void			ResetLayout(Document*);
	
	void					Load(const Resource* parent);
	virtual	void			PurgeResource();

protected:
	void					ConstrainWidth(long marginWidth, long maxWidth);
	void					InvalidateBounds(Layer*);
	
	char*					fALT;
	ImageData*				fImageData;
	ImageMap*				fImageMap;
	long					fKnownHeight;
	Displayable*			fParent;
	Resource				fResource;
	char*					fSRC;
	char*					fUseMap;

	short					fBorder;
	short					fHSpace;
	short					fVSpace;
	short					fImageMapSelection;
	short					fKnownWidth;

	AttributeValue			fAlign;
	char					fPercentageWidth;
	ImageIdleState			fState;
	uchar					fTransparency;

	unsigned				fCreatedImageData : 1;
	unsigned				fDrawEmpty : 1;
	unsigned				fIdleDrawEnabled : 1;
	unsigned				fIsBackground : 1;
	unsigned				fIsMap : 1;
	unsigned				fIsVisible : 1;
	unsigned				fLayoutComplete : 1;
	unsigned				fNoFilter : 1;
	unsigned				fSizeKnown : 1;
#if defined FIDO_INTERCEPT
public:
	virtual void			Draw(const Document* document, class FidoCompatibilityState& fidoCompatibility) const;
		    void			DrawBorder(const Document* document, class FidoCompatibilityState& fidoCompatibility) const;
#endif	
};

// =============================================================================

inline ImageMap* Image::GetImageMap() const
{
	return fImageMap;
}

inline Priority Image::GetPriority() const
{
	return fResource.GetPriority();
}

inline const Resource* Image::GetResource() const
{
	return &fResource;
}

inline void Image::SetPriority(Priority priority)
{
	fResource.SetPriority(priority);
}

inline void Image::SetImageMap(ImageMap* map)
{
	fImageMap = map;
}

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Image.h multiple times"
	#endif
#endif /*__IMAGE_H__ */
