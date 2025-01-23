// ===========================================================================
//	Document.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __DOCUMENT_H__
#define __DOCUMENT_H__

#ifndef __CACHEENTRY_H__
#include "CacheEntry.h"			/* for Priority */
#endif
#ifndef __ERRORNUMBERS_H__
#include "ErrorNumbers.h"		/* for Error */
#endif
#ifndef __GRAPHICS_H__
#include "Graphics.h"			/* for Color, Rectangle, XFont */
#endif
#ifndef __IMAGEMAP_H__
#include "ImageMap.h"			/* for ImageMapType */
#endif
#ifndef __LIST_H__
#include "List.h"				/* for ObjectList */
#endif
#ifndef __PAGE_H__
#include "Page.h"				/* for RootPage */
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"			/* for Resource */
#endif

class Anchor;
class AudioStream;
class ContentView;
class Displayable;
class DocumentPrefetcher;
class Form;
class Image;
class Layer;
class SideBar;
class Song;

// =============================================================================

enum {
	kNoEffect = 0,
	kReliefEffect,
	kEmbossEffect,
	kOutlineEffect
};
	
typedef struct {
	unsigned fontSize : 3;		// 1 .. 7 (0 == default font)
	unsigned monoSpaced : 1;	// 0 = proportional, 1 = mono
	unsigned bold : 1;
	unsigned italic : 1;
	unsigned underline : 1;
	unsigned shadow : 1;
	unsigned subscript : 1;
	unsigned superscript : 1;
	unsigned anchor : 1;
	unsigned visited : 1;
	unsigned effect : 2;
	unsigned color : 1;
	unsigned free : 1;
} PackedStyle;

typedef struct {
	PackedStyle	style;
	unsigned	color : 24;
	unsigned	transparency : 8;
} PackedColorStyle;

extern PackedStyle		gDefaultStyle;
extern PackedColorStyle	gDefaultColorStyle;
extern const char		gServerMapName[];

// =============================================================================
// Selection specifies a selection in an HTML document. The selection specifies
// page and selectable

struct Selection
{
	short				fPageIndex;
	short				fSelectableIndex;
};

Boolean	SelectionsEqual(Selection, Selection);

const long				kInvalidSelectable = -1;
const Selection	kInvalidSelection = {0, kInvalidSelectable};

// =============================================================================
// Document is the container for a built HTML document

class Document : public HasHTMLAttributes
{
public:
							Document();
	virtual 				~Document();

	char*					CopyURL(const char* tagName) const;

	Form*					FindForm(const char* formName);
	Displayable*			FindFragment(const char* fragmentName);
	Displayable*			FindString(const char* string);
	
	Color					GetBackgroundColor() const;
	const Resource*			GetBaseResource() const;
	Color					GetColor(PackedStyle) const;
	const char*				GetCreditsURL() const;
	Boolean					GetDisplaySelection() const;
	Boolean					GetDisplayStatus() const;
	Boolean					GetDisplayOptions() const;
	Boolean					GetEnableOptions() const;
	XFont					GetFont(PackedStyle) const;
	Form*					GetForm(long) const;
	ulong					GetFormCount() const;
	long					GetHeight() const;
	const char*				GetHelpURL() const;
	long					GetImageCount() const;
	Color					GetLinkColor() const;
	Image*					GetLogoImage() const;
	long					GetNoScrollWidth() const;
	uchar					GetPercentComplete() const;
	Priority				GetPriority() const;
	short					GetReloadTime() const;
	const char*				GetReloadURL() const;
	const Resource*			GetResource() const;
	RootPage*				GetRootDisplayable() const;
	Displayable*			GetSelectable(Selection) const;
	ulong					GetSelectableCount(long pageIndex = 0) const;
	Boolean					GetSkipBack() const;
	Error					GetStatus() const;
	Boolean					GetStayTime() const;
	const char*				GetText(long offset = 0) const;
	Color					GetTextColor() const;
	void					GetTextHighlightRange(long* start, long* end) const;
	const char*				GetTitle() const;
	TransitionEffect		GetTransitionEffect() const;
	ContentView*			GetView() const;
	Error					GetVisibleImageStatus(const Rectangle*) const;
	Color					GetVisitedLinkColor() const;
	Boolean					IsLayoutCurrent() const;
	CharacterEncoding		GetCharacterEncoding() const;
	
	void					ResetResources();

	virtual void			SetAttribute(Attribute attributeID, long value, Boolean isPercentage);
	virtual void			SetAttributeStr(Attribute attributeID, const char* value);

	void					SetBackgroundColor(Color);
	void					SetLinkColor(Color);
	void					SetLogoImage(Image* image);
	void					SetReloadTime(short newValue);
	void					SetReloadURL(const char* url);
	void					SetResource(const Resource* resource);
	void					SetSkipBack(Boolean);
	void					SetTextColor(Color);
	void					SetTitle(const char*);
	void					SetPriority(Priority);
	void					SetView(ContentView*);
	void					SetVisitedLinkColor(Color);
	void					SetMargins(ushort horizontal, ushort vertical);
	void					SetCharacterEncoding(CharacterEncoding);

	void					StartAnimations();
	void					StartReloadTiming();
	
	void					AddSong(const char* songName, ulong loopCount);
	Song*					RemoveSong();

	void					AddAudio(const char *urlName,const DataType type);

	
	void					Draw(const Rectangle* invalid);
	void					DrawBackground(const Rectangle* invalid);
#ifdef FIDO_INTERCEPT
	virtual void			Draw(class FidoCompatibilityState& fidoCompatibility) const;
	void					DrawBackground(class FidoCompatibilityState& fidoCompatibility) const;
#endif
	Boolean 				Idle(Layer* layer);
	void					IdleBackground(Layer* layer);
	void					Layout();
	void					Relayout();

	static const ushort		kDefaultHorizontalMargin;
	static const ushort		kDefaultVerticalMargin;

public: // DocumentBuilder only
	void					AddAnchor(Anchor* anchor);
	void					AddControl(Control* control);
	void					AddForm(Form* form);
	void					AddImage(Image* image);
	void					AddPrefetcher(DocumentPrefetcher*);
	void					AddSelectable(Displayable*);
	void					AddSideBar(SideBar*);
	long					AddText(const char* text, long textCount);	// returns offset
	void					Finalize();
	SideBar*				GetSideBar() const;
	Boolean					CheckReload();
	ImageMap*				FindOrAddImageMap(const char* mapName, ImageMapType);
	ImageMap*				FindOrAddImageMap(const Resource*, const char* mapName, ImageMapType);
			
protected:
	Displayable*			FindDisplayableAtTextOffset(Displayable* root, long textOffset);
	void					UpdatePriorities();

protected:
	ObjectList				fFormList;
	ObjectList				fFragmentList;
	ObjectList				fImageList;
	ObjectList				fImageMapList;
	ObjectList				fPrefetchList;
	RootPage				fRootDisplayable;
	ObjectList				fSelectableList;
	ObjectList				fSideBarSelectableList;
	SideBar*				fSideBar;
	DataStream*				fTextStream;			// Storage for plain text.

	Resource				fBaseResource;
	Resource				fResource;
	Color					fBackgroundColor;
	Image*					fBackgroundImage;
	Image*					fLogoImage;
	Color					fLinkColor;
	Song*					fSong;
	AudioStream*			fAudio;
	ulong					fStayTime;
	Color					fTextColor;
	long					fTextHighlightStart;
	long					fTextHighlightEnd;
	char*					fTitle;
	long					fVisitedLinkColor;
	ContentView*			fView;
	long					fXBackgroundOffset, fYBackgroundOffset;
	ulong					fReloadExpirationTime;
	char*					fReloadURL;				// name of next one to reload, or nil for me
	char*					fHelpURL;				// name of help page
	char*					fCreditsURL;			// name of credits page

	short					fReloadTime;
	short					fXBackgroundSpeed, fYBackgroundSpeed;
	
	Priority				fPriority;
	CharacterEncoding		fCharacterEncoding;
	TransitionEffect		fTransitionEffect;

	unsigned				fHTileBackground : 1;
	unsigned				fVTileBackground : 1;
	unsigned				fDisplayOptions : 1;
	unsigned				fEnableOptions : 1;
	unsigned				fDisplayStatus : 1;
	unsigned				fHasDrawnBackground : 1;
	unsigned				fInSideBarLayout : 1;
	unsigned				fFinalized : 1;
	unsigned				fSkipBack : 1;
};

// =============================================================================

inline Boolean 
Document:: GetDisplayStatus() const
{ 
	return fDisplayStatus;
}

inline Boolean 
Document::GetDisplayOptions() const
{
	return fDisplayOptions;
}

inline Boolean 
Document::GetEnableOptions() const
{
	return fEnableOptions;
}

inline long
Document::GetImageCount() const
{
	return fImageList.GetCount();
}

inline Image* 
Document::GetLogoImage() const
{
	return fLogoImage;
}

inline Priority 
Document::GetPriority() const
{
	return fPriority;
}

inline const char* 
Document::GetHelpURL() const
{
	return fHelpURL;
}

inline const char* 
Document::GetCreditsURL() const
{
	return fCreditsURL;
}

inline short 
Document::GetReloadTime() const
{
	return fReloadTime;
}

inline const char* 
Document::GetReloadURL() const
{ 
	return fReloadURL;
}

inline const Resource* 
Document::GetResource() const
{
	return &fResource;
}

inline SideBar*
Document::GetSideBar() const
{
	return fSideBar;
}

inline TransitionEffect
Document::GetTransitionEffect() const
{
	return fTransitionEffect;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
inline	Boolean	
Document::GetStayTime() const	
{
	return fStayTime;
}
#endif

inline void
Document::SetLogoImage(Image* image)
{
	fLogoImage = image;
}

inline void
Document::SetSkipBack(Boolean skip)
{
	fSkipBack = skip;
}

// =============================================================================

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Document.h multiple times"
	#endif
#endif /*__DOCUMENT_H__ */
