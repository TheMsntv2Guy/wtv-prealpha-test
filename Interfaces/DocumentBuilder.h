// ===========================================================================
//	DocumentBuilder.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __DOCUMENTBUILDER_H__
#define __DOCUMENTBUILDER_H__

#ifndef __DOCUMENT_H__
#include "Document.h"
#endif

#ifndef __TABLE_H__
#include "Table.h"
#endif

#ifndef __TAG_H__
#include "Tag.h"
#endif

class SideBar;

// ============================================================================
// Builder is the base class for all HTML source object builders.

class Builder 
{
public:
							Builder();
	virtual					~Builder();
					
	virtual void			AddTag(short tagID, const TagList* tagList);	
	virtual void			AddText(const char* text, short textCount);
			
	virtual void			Finalize();

protected:
	void					SetDisplayableAttributes(HasHTMLAttributes*, const TagList* tagList);
};

// ============================================================================
// The DocumentBuilder takes output from a Parser and builds a page

class DocumentBuilder : public Builder
{
public:
							DocumentBuilder();
	virtual					~DocumentBuilder();
						
	void					AbortAndFinalize();

	virtual void			AddTag(short tagID, const TagList* tagList);	
	virtual void			AddText(const char* text, short textCount);

	virtual void			Finalize();

	Document* 				GetDocument() const;
	void					SetResource(const Resource*);

protected:
	void					AddArea(const TagList* tagList);
	void					AddBackgroundSound(const TagList* tagList);
	void					AddBase(const TagList* tagList);
	void					AddBody(const TagList* tagList);
	void					AddBullet(const TagList* tagList);
	void					AddClock(const TagList* tagList);
	void					AddDivider(const TagList* tagList);
	void					AddEmbedded(const TagList* tagList);
	void					AddFrame(const TagList* tagList);
	void					AddImage(const TagList* tagList);
	void					AddAudioScope(const TagList* tagList);
	void					AddInput(const TagList* tagList);
	void					AddLink(const TagList* tagList);
	void					AddMeta(const TagList* tagList);
	void					AddNewLine(const TagList* tag);
	
	void					Align(AttributeValue align, Boolean force = false);
	void					BaseFont(const TagList* tagList);
	void					Big(const TagList* tagList);
	void					BlankLine();
	
	void					CloseAnchor();
	void					CloseCaption();
	void					CloseCenter();
	void					CloseDivision();
	void					CloseForm();
	void					CloseHeading();
	void					CloseImageMap();
	void					CloseParagraph();
	void					CloseSelect();
	void					CloseSideBar();
	void					CloseTable();
	void					CloseTableCell();
	void					CloseTableRow();
	void					CloseTextArea();
	
	Control*				CreateInput(short inputType, const TagList* list);	
	void					Display(const TagList* tagList);
	void					Font(const TagList* tagList);
	Boolean					IsStyleTag(short tagID, const TagList* tagList);
	void					MoveMargin(short count, Boolean permanent);
	void					NewStyle(short tagID);
	
	void					OpenAnchor(const TagList* tagList);
	void					OpenCaption(const TagList* tagList);
	void					OpenCenter(const TagList* tagList);
	void					OpenDivision(const TagList* tagList);
	void					OpenForm(const TagList* tagList);
	void					OpenHeading(const TagList* tagList);
	void					OpenImageMap(const TagList* tagList);
	void					OpenOption(const TagList* tagList);
	void					OpenParagraph(const TagList* tagList);
	void					OpenSelect(const TagList* tagList);
	void					OpenSideBar(const TagList* tagList);
	void					OpenTable(const TagList* tagList);
	void					OpenTableCell(const TagList* tag, Boolean isHeading);
	void					OpenTableRow(const TagList* tagList);
	void					OpenTextArea(const TagList* tagList);
	
	void					PopAlignment();
	void					PopStyle();
	void					PopList();
	void					PushAlignment();
	void					PushList(short tagID, const TagList* tagList);
	void					PushStyle();
	void					SetTagStyle(PackedStyle style, Boolean set);
	void					Small(const TagList* tagList);
			
protected:
	Anchor*					fAnchor;	
	Page*					fDisplayable;
	Document*				fDocument;
	Form*					fForm;
	char*					fFrameSRC;
	SideBar*				fSideBar;
	Table*					fTable;
	ImageMap*				fImageMap;
	TextField*				fTextArea;
	
	PackedColorStyle		fCurrentStyle;
	PackedColorStyle		fStyleStack[100];
	ushort					fStyleStackDepth;
	
	AttributeValue			fAlignStack[100];
	ushort					fAlignStackDepth;

	short					fListIndex[50];
	short					fListDepth;
	short					fListStack[50];
	short					fMarginDepth;

	short					fBaseFont;

	AttributeValue			fCurrentAlign;

	unsigned				fInParagraph : 1;	
	unsigned				fInPRE : 1;
	unsigned				fInScript : 1;
	unsigned				fInTitle : 1;
	unsigned				fNoColorAnchor : 1;
	unsigned				fNoFrame : 1;
	unsigned				fVisitedAnchor : 1;
};

// ============================================================================
// The ImageMapBuilder takes output from a Parser and builds an image map.

class ImageMapBuilder : public Builder
{
public:
							ImageMapBuilder();
							~ImageMapBuilder();
					
	virtual void			AddTag(short tagID, const TagList* tagList);	

	virtual void			Finalize();

	ImageMap*				GetTargetMap() const;
	void					OpenImageMap(); 		// Opens the target map.	
	void					SetTargetMap(ImageMap* targetMap);
	
protected:
	void					AddArea(const TagList* tagList);
	void					AddBase(const TagList* tagList);
	void					CloseImageMap();
	void					OpenImageMap(const TagList* tagList);

	char*					fBaseURL;
	ImageMap*				fCurrentMap;
	ImageMap*				fTargetMap;
};

// ============================================================================
// The PrefetchBuilder takes output from a Parser and prefetches embedded streams.

class PrefetchBuilder : public Builder
{
public:
							PrefetchBuilder();
							~PrefetchBuilder();
					
	virtual void			AddTag(short tagID, const TagList* tagList);
	
	void					SetResource(const Resource*);

protected:
	void					AddResource(const TagList* tagList);
	
	Resource				fResource;
};


enum PrefetchState {
	kCreatePrefetchStream = 0,
	kPrefetchParse,
	kPrefetchDone,
	kPrefetchError
};

// ============================================================================
// The DocumentPrefetcher prefetches a resource and embedded streams.

class DocumentPrefetcher : public Listable
{
public:
							DocumentPrefetcher();
							~DocumentPrefetcher();

	const Resource*			GetResource() const;					
	Error					GetStatus() const;
	void					Idle();
	void					Load(const char* url, const Resource* parent);
	
protected:
	void					Reset();
	
	PrefetchBuilder			fBuilder;
	HTMLParser				fParser;
	Resource				fResource;
	DataStream*				fStream;
	PrefetchState			fState;
};

// ============================================================================

inline const Resource*
DocumentPrefetcher::GetResource() const
{
	return &fResource;
}

// ============================================================================
#else
#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
#error "Attempted to #include DocumentBuilder.h multiple times"
#endif
#endif /*__DOCUMENTBUILDER_H__ */

