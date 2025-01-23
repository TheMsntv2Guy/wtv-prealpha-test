// ===========================================================================
//	DocumentBuilder.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __AUDIO_SCOPE_H__
#include "AudioScope.h"
#endif
#ifndef __ANIMATION_H__
#include "Animation.h"
#endif
#ifndef __BUTTON_H__
#include "Button.h"
#endif
#ifndef __CLOCKFIELD_H__
#include "ClockField.h"
#endif
#ifndef __DOCUMENT_H__
#include "Document.h"
#endif
#ifndef __DOCUMENTBUILDER_H__
#include "DocumentBuilder.h"
#endif
#ifndef __EMBEDDED_H__
#include "Embedded.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __MENU_H__
#include "Menu.h"
#endif
#ifndef __PAGEVIEWER_H__
#include "PageViewer.h"
#endif
#ifndef __PERFDUMP_H__
#include "PerfDump.h"
#endif
#ifndef __RADIOBUTTON_H__
#include "RadioButton.h"
#endif
#ifndef __SERVICE_H__
#include "Service.h"
#endif
#ifndef __STREAM_H__
#include "Stream.h"
#endif
#ifndef __SYSTEM_H__
#include "System.h"
#endif
#ifndef __TEXT_H__
#include "Text.h"
#endif
#ifndef __TEXTFIELD_H__
#include "TextField.h"
#endif




// ===========================================================================
//	globals
// ===========================================================================

static const short kMaxMarginDepth = 12;

typedef struct {
	short		tagID;
	PackedStyle	style;
	Boolean		hasMargin;
	short		count;
	Boolean		permanent;
	Boolean 	marginOnly;
} StyleList;

static StyleList gStyleList[] = {
//   Tag			   Siz,Mno,Bld,Itc,Uln,Shw,Sub,Sup	Mrg,Move,Perm,MOnly
	{ 0, 			{	3, 	0, 	0, 	0, 	0,  0,  0,  0},  0,  0,   0,   0	}, 	// Default text
	
	{ T_TT,			{	0, 	1, 	0, 	0, 	0,  0,  0,  0},  0,  0,   0,   0	}, 	// Typographic
	{ T_B,			{	0, 	0, 	1, 	0, 	0,  0,  0,  0},  0,  0,   0,   0	},
	{ T_I,			{	0, 	0, 	0, 	1, 	0,  0,  0,  0},  0,  0,   0,   0	}, 
	{ T_U,			{	0, 	0, 	0, 	0, 	1,  0,  0,  0},  0,  0,   0,   0	}, 
	{ T_SHADOW,		{	0, 	0, 	0, 	0, 	0,  1,  0,  0},  0,  0,   0,   0	}, 
	{ T_SUB,		{	0, 	0, 	0, 	0, 	0,  0,  1,  0},  0,  0,   0,   0	}, 
	{ T_SUP,		{	0, 	0, 	0, 	0, 	0,  0,  0,  1},  0,  0,   0,   0	}, 
	
	{ T_H1,			{	6, 	0, 	1, 	0, 	0	}, 			 0,  0,   0,   0	}, 	// Headings
	{ T_H2,			{	5, 	0, 	1, 	0, 	0	},		 	 0,  0,   0,   0	}, 
	{ T_H3,			{	4, 	0, 	1, 	0, 	0	},		 	 0,  0,   0,   0	}, 
	{ T_H4,			{	3, 	0, 	1, 	0, 	0	},		 	 0,  0,   0,   0	}, 
	{ T_H5,			{	2, 	0, 	1, 	0, 	0	},		 	 0,  0,   0,   0	}, 
	{ T_H6,			{	1, 	0, 	1, 	0, 	0	}, 			 0,  0,   0,   0	}, 

	{ T_DL,			{	3, 	0, 	0, 	0, 	0	},			 1, +1,   1,   0	}, 	// Glossary list
	{ T_OL,			{	3, 	0, 	0, 	0, 	0	}, 			 1, +1,   1,   0	}, 	// Ordered list
	{ T_UL,			{	3, 	0, 	0, 	0, 	0	}, 			 1, +1,   1,   0	}, 	// Unordered list
	{ T_DIR,		{	2, 	0, 	1, 	0, 	0	}, 			 1, +1,   1,   0	}, 	// Directory list
	{ T_MENU,		{	3, 	0, 	0, 	0, 	0	}, 			 1, +1,   1,   0	}, 	// Menu list

	{ T_DT,			{	3, 	0, 	0, 	0, 	0	},		 	 1, -1,   0,   1	}, 	// Not permanent, margin only
	{ T_DD,			{	3, 	0, 	0, 	0, 	0	}, 			 1,  0,   0,   1	}, 	// See DocumentBuidler::MoveMargin
	{ T_LI,			{	3, 	0, 	0, 	0, 	0	},		 	 0,  0,   0,   1	}, 

	{ T_PRE,		{	2, 	1, 	0, 	0, 	0	}, 			 0,  0,   0,   0	}, 	// Preformatted
	{ T_XMP,		{	2, 	1, 	0, 	0, 	0	},		 	 0,  0,   0,   0	}, 
	{ T_LISTING,	{	2, 	1, 	0, 	0, 	0	}, 			 0,  0,   0,   0	},
	{ T_ADDRESS,	{	3, 	0, 	0, 	1, 	0	}, 			 1, +1,   1,   0	}, 
	{ T_BLOCKQUOTE,	{	3, 	0, 	0, 	0, 	0	}, 			 1, +1,   1,   0	}, 
	
	{ T_CODE,		{	2, 	1, 	0, 	0, 	0	}, 			 0,  0,   0,   0	}, 	// Code
	{ T_KBD,		{	2, 	1, 	0, 	0, 	0	},		 	 0,  0,   0,   0	}, 	// Keyboard
	{ T_SAMP,		{	2, 	1, 	0, 	0, 	0	}, 			 0,  0,   0,   0	}, 	// Sample

	{ T_EM,			{	3, 	0, 	0, 	1, 	0	}, 			 0,  0,   0,   0	}, 	// Emphasis
	{ T_CITE,		{	3, 	0, 	0, 	1, 	0	}, 			 0,  0,   0,   0	}, 	// Citation
	{ T_VAR,		{	3, 	0, 	0, 	1, 	0	}, 			 0,  0,   0,   0	}, 	// Variable
	
	{ T_STRONG,		{	3, 	0, 	1, 	0, 	0	}, 			 0,  0,   0,   0	}, 	// Strong
	{ T_DFN,		{	3, 	0, 	0, 	0, 	0	}, 			 0,  0,   0,   0	}, 	// Definition?

	{ -1, {0, 0, 0, 0, 0} , 0, 0, 0, 0 }
};




// ============================================================================
// Builder

Builder::Builder()
{
}

Builder::~Builder()
{
}
					
void Builder::Finalize(){}

void Builder::AddTag(short, const TagList*)
{
}

void Builder::AddText(const char*, short)
{
}

void Builder::SetDisplayableAttributes(HasHTMLAttributes* hasAttributes, const TagList* tagList)
{
	// Pass Attributes to displayable
	
	for (long i = 0; tagList != nil && i < tagList->GetCount(); i++) {
		const Tag*	tag = tagList->At(i);
		if (tag->fIsNumericValue)
			hasAttributes->SetAttribute(tag->fAttributeID, tag->fValue, tag->fIsPercentage);
		else
			hasAttributes->SetAttributeStr(tag->fAttributeID, tag->GetValueAsString());
	}
}

			
// ============================================================================
// DocumentBuilder

DocumentBuilder::DocumentBuilder()
{
	fDocument = new(Document);
	fDisplayable = fDocument->GetRootDisplayable();

	fBaseFont = 3;
	fCurrentAlign = AV_LEFT;
	fCurrentStyle = gDefaultColorStyle;
}

DocumentBuilder::~DocumentBuilder()
{
	// Don't leak tables.
	while (fTable != nil)
		CloseTable();
		
	CloseTextArea();
		
	if (fFrameSRC != nil)
		FreeTaggedMemory(fFrameSRC, "DocumentBuilder::fFrameSRC");
}

void DocumentBuilder::AbortAndFinalize()
{
	static char abortText[] = "This page can't be shown in its entirety, "
					  		  "because there's too much of it.";
					  		  
	// Close any open tables, anchors forms, etc.
	while (fTable != nil)
		CloseTable();
	
	CloseAnchor();
	CloseSelect();
	CloseForm();
	CloseImageMap();
	
	// Add message at bottom of document indicating content is missing.
	Align(AV_CENTER, true);
	AddNewLine(nil);
	fDisplayable->AddChild(new(Divider));
	AddNewLine(nil);
	fCurrentStyle = gDefaultColorStyle;
	fCurrentStyle.style.bold = true;
	AddText(abortText, sizeof(abortText) - 1);
	AddNewLine(nil);
	
	fDocument->Finalize();
}

void DocumentBuilder::AddArea(const TagList* tag)
{
	if (fImageMap != nil)
	{		
		Area*	area = new(Area);
		
		SetDisplayableAttributes(area, tag);
		fImageMap->AddArea(area);
	}
}
	
void DocumentBuilder::AddBackgroundSound(const TagList* tagList)
{
	const char*	url = nil;
	ulong		loopCount = 0;
	
	for (long i = 0; tagList != nil && i < tagList->GetCount(); i++) {
		const Tag* tag = tagList->At(i);
		switch (tag->fAttributeID) 
		{			
			case A_SRC:		url = tag->GetValueAsString(); break;
			case A_LOOP:	if (tag->GetValueAsString() == nil ||
								(loopCount = atoi(tag->GetValueAsString())) == 0)
								loopCount = 0xFFFFFFFF;
							break;
			default:		break;
		}
	}
	
	DataType	dataType = GuessDataType(url);
	switch (dataType) {
		case kDataTypeMPEGAudio:
		case kDataTypeRealAudioMetafile:
			fDocument->AddAudio(url, dataType);
			break;
		default:
			fDocument->AddSong(url, loopCount);
	}
}

void DocumentBuilder::AddBase(const TagList* tagList)
{
	SetDisplayableAttributes(fDocument, tagList);
}

void DocumentBuilder::AddBody(const TagList* tagList)
{
	SetDisplayableAttributes(fDocument, tagList);
}

void DocumentBuilder::AddBullet(const TagList* tagList)
{
	// Get The style of the current list (UL/OL etc ...)
	
	for (long i = 0; tagList != nil && i < tagList->GetCount(); i++) {
		const Tag* tag = tagList->At(i);
		switch (tag->fAttributeID) 
		{				
			case A_TYPE:
				if (tag->GetValueAsString() != nil)
					fListStack[fListDepth] = *tag->GetValueAsString();	// Kind is A|a|I|i|1 for ordered lists, d|c|s for UL
				break;
			case A_VALUE:
			{
				long value = fListIndex[fListDepth];
				if (tag->GetValueAsString() != nil)
					sscanf(tag->GetValueAsString(), "%ld", &value);
				fListIndex[fListDepth] = value;
				break;
			}
			default:
				break;
		}
	}

	// Add a dot to a list item
	// Kind is A|a|I|i|1 for ordered lists, d|c|s for UL
	// Index is its ordinal
	
	Bullet* bullet;

	if (fCurrentStyle.style.color || fCurrentStyle.transparency != 0)
	{
		ColorBullet*	colorBullet = new(ColorBullet);
		colorBullet->SetColor(fCurrentStyle.color);
		colorBullet->SetTransparency(fCurrentStyle.transparency);
		bullet = colorBullet;
	}
	else
		bullet = new(Bullet);
		
	bullet->SetKind(fListStack[fListDepth], fListIndex[fListDepth]);
	bullet->SetStyle(fCurrentStyle.style);
	fDisplayable->AddChild(bullet);

	fListIndex[fListDepth]++;
}

void DocumentBuilder::AddClock(const TagList* tagList)
{
	int type = kTimeField;
	
	if (fForm == nil)
		return;

	for (long i = 0; tagList != nil && i < tagList->GetCount(); i++) {
		switch (tagList->At(i)->fAttributeID) {
			case A_TIME:
				type = kTimeField;
				break;
			case A_DATE:
				type = kDateField;
				break;
			default:
				type = kTimeField;
				break;
		}
	}

	ClockField* control = new(ClockField);
	control->IClockField(type);
	SetDisplayableAttributes(control, tagList);
	fForm->AddControl(control);
	fDisplayable->AddChild(control);
}

void DocumentBuilder::AddDivider(const TagList* tagList)
{
	// Add a horizontal rule
	AttributeValue oldAlign = fCurrentAlign;

	Divider* rule = new(Divider);
	SetDisplayableAttributes(rule, tagList);

	Align(rule->GetHAlign());		
	fDisplayable->AddChild(rule);
	Align(oldAlign);
	AddNewLine(nil);
}

void DocumentBuilder::AddEmbedded(const TagList*)
{
#if 0 // Disabled. No embedded types supported.
	Embedded* g = new(Embedded);
	SetDisplayableAttributes(g, tagList);
	fDisplayable->AddChild(g);
//	fDocument->AddEmbedded(g); еее Document may need to track these for idle, DRA.
#endif
}

void DocumentBuilder::AddFrame(const TagList* tagList)
{	
	const Tag*	tag;
	
	// Save the first frame's url, to be executed if we have no other content.
	if (fFrameSRC == nil && tagList != nil && (tag = tagList->Find(A_SRC)) != nil)
		if (tag->GetValueAsString() != nil)
			fFrameSRC = CopyString(tag->GetValueAsString(), "DocumentBuilder::fFrameSRC");
}

void DocumentBuilder::AddImage(const TagList* tagList)
{
	Image*	image;
	
	if (tagList != nil && tagList->Find(A_ANI) != nil)
		image = new(Animation);
	else
		image = new(Image);
	
	SetDisplayableAttributes(image, tagList);
	if (image->IsFloating())
		fDisplayable->AddFloatingChild(image);
	else
		fDisplayable->AddChild(image);
	fDocument->AddImage(image);

	// Check for client or server image maps
	if (image->GetUseMap() != nil)
		image->SetImageMap(fDocument->FindOrAddImageMap(image->GetUseMap(), kClientMap));		
	else if (image->IsMap() && fAnchor != nil && fAnchor->HasURL() && fAnchor->GetImageMap() == nil)
		image->SetImageMap(fDocument->FindOrAddImageMap(fAnchor->GetHREF(), kServerMap));

	// We set the image in the anchor for both client maps as well as server maps to 
	// guarantee that the anchor does not get added as a selectable.
	if (fAnchor != nil && fAnchor->GetImageMap() == nil)
		fAnchor->SetMappedImage(image);
}

void DocumentBuilder::AddAudioScope(const TagList* tagList)
{
	AudioScope*	audioscope;
	
	audioscope = new(AudioScope);
	
	SetDisplayableAttributes(audioscope, tagList);

	audioscope->Setup();

	fDisplayable->AddChild(audioscope);
	fDocument->AddImage(audioscope);
}

void DocumentBuilder::AddInput(const TagList* tagList)
{
	// Should this be a factory function in Controls?
	
	short inputType = AV_TEXT;
	if (fForm == nil)
		return;
	
	// Determine what kind of input should be created
	const Tag* tag;	
	if (tagList != nil && (tag = tagList->Find(A_TYPE)) != nil && tag->fValue > 0) 
		inputType = tag->fValue;

	Control* control = CreateInput(inputType, tagList);
	
	if (control != nil)
	{
		SetDisplayableAttributes(control, tagList);
		fForm->AddControl(control);
		
		if (control->IsFloating())
			fDisplayable->AddFloatingChild(control);
		else
			fDisplayable->AddChild(control);

		// Handle image map for input images.		
		Image* image = control->GetMappedImage();
		if (image != nil) 
		{		
			fDocument->AddImage(image);
			ImageMap*	map = fDocument->FindOrAddImageMap(fDocument->GetBaseResource(),
														   gServerMapName, kServerMap);
			image->SetImageMap(map);
		}
	}
}

void DocumentBuilder::AddLink(const TagList* tagList)
{
	const char*	rel = nil;
	const char* href = nil;
	
	for (long i = 0; tagList != nil && i < tagList->GetCount(); i++) {
		const Tag* tag = tagList->At(i);
		switch (tag->fAttributeID) 
		{			
			case A_REL:		rel = tag->GetValueAsString(); break;
			case A_HREF:	href = tag->GetValueAsString(); break;
			default:		break;
		}	
	}

	// If this is a "Next" link, create a resource to start preloading of the page.
	if (EqualString(rel, "next") && href != nil) {
		DocumentPrefetcher* prefetcher = new(DocumentPrefetcher);
		prefetcher->Load(href, fDocument->GetBaseResource());
		fDocument->AddPrefetcher(prefetcher);
	}
}

void DocumentBuilder::AddMeta(const TagList* tagList)
{
	if (tagList == nil)
		return;

	const Tag*	httpTag = tagList->Find(A_HTTP_EQUIV);
	const Tag*	contentTag = tagList->Find(A_CONTENT);
			
	if (httpTag == nil || contentTag == nil)
		return;
		
	// Special case "refresh" for reload
	if (EqualString(httpTag->GetValueAsString(), "refresh")) {	
		long		value;
		if (sscanf(contentTag->GetValueAsString(), "%ld", &value) == 1) {
			fDocument->SetReloadTime(value > 0 ? value : 0);
		
			if ((contentTag = tagList->Find(A_URL)) != nil)
				fDocument->SetReloadURL(contentTag->GetValueAsString());
			
			// Reload immediately if time is 0
			if (fDocument->GetReloadTime() == 0 && fDocument->GetReloadURL() != nil)
				fDocument->StartReloadTiming();
		}
	}
	
	// Otherwise set the attribute on the resource.
	else {
		DataStream*	stream = fDocument->GetResource()->NewStream();
		if (!IsError(stream == nil)) {
			stream->SetAttribute(httpTag->GetValueAsString(),
								 (char*)contentTag->GetValueAsString());			
			delete(stream);
		}
	}
}

void DocumentBuilder::AddNewLine(const TagList* tagList)
{
	// Linebreaks include breaking left, right and all around floating displayables.
	
	short		breakType = LineBreak::kHard;
	const Tag*	tag = nil;
	
	if  (tagList != nil && (tag = tagList->Find(A_CLEAR)) != nil) { 
		switch (tag->fValue) 
		{						
			case AV_LEFT:	breakType = LineBreak::kHardLeft;	break;
			case AV_RIGHT:	breakType = LineBreak::kHardRight;	break;
			case AV_ALL:	breakType = LineBreak::kHardAll;	break;
		}
	}
	
	LineBreak* lineBreak = new(LineBreak);
	lineBreak->SetBreakType((LineBreak::BreakType)breakType);
	fDisplayable->AddChild(lineBreak);
}

void DocumentBuilder::AddTag(short tagID, const TagList* tagList)
{
	if (fInScript && -tagID != T_SCRIPT)
		return;
		
	if (tagID > 0) 
	{		
		switch (tagID) {		// Start tags <TAG>
			case T_HTML:
			case T_HEAD:										break;
			case T_LINK:		AddLink(tagList);				break;
			case T_META:		AddMeta(tagList);				break;
			case T_DISPLAY:		Display(tagList);				break;
						
			case T_TITLE:		fInTitle = true;				break;
			case T_BASE:		AddBase(tagList);				break;
			case T_BGSOUND:		AddBackgroundSound(tagList);	break;
			case T_BODY:		AddBody(tagList);				break;
			
			case T_DIV:			OpenDivision(tagList);			break;
			case T_CENTER:		OpenCenter(tagList);			break;
			case T_P:			OpenParagraph(tagList);			break;
			case T_BR:			AddNewLine(tagList);			break;
			
			case T_HR:			AddDivider(tagList);			break;
			
			case T_IMG:
			case T_IMAGE:		AddImage(tagList);				break;
			case T_FN:	
			case T_A:			OpenAnchor(tagList);			break;
			
			case T_FONT:		Font(tagList);					break;
			case T_BASEFONT:	BaseFont(tagList);				break;
			case T_BIG:			Big(tagList);					break;
			case T_SMALL:		Small(tagList);					break;
		
			case T_TABLE:		OpenTable(tagList);				break;
			case T_TR:			OpenTableRow(tagList);			break;
			case T_TH:			OpenTableCell(tagList, true);	break;	// Heading
			case T_TD:			OpenTableCell(tagList, false);	break;
			case T_CAPTION:		OpenCaption(tagList);			break;
			
			case T_FORM:		OpenForm(tagList);				break;
			case T_INPUT:		AddInput(tagList);				break;
			case T_CLOCK:		AddClock(tagList);				break;
			case T_AUDIOSCOPE:	AddAudioScope(tagList);			break;
			case T_SELECT:		OpenSelect(tagList);			break;
			case T_OPTION:		OpenOption(tagList);			break;
			case T_TEXTAREA:	OpenTextArea(tagList);			break;
			case T_NOBR:		/* ignore this request */		break;
			
			case T_EMBED:		AddEmbedded(tagList);			break;
			case T_MAP:			OpenImageMap(tagList);			break;
			case T_AREA:		AddArea(tagList);				break;
			case T_SIDEBAR:		OpenSideBar(tagList);			break;
			
			case T_SCRIPT:		fInScript = true;				break;
			
			case T_FRAME:		AddFrame(tagList);				break;
			case T_NOFRAME:		
			case T_NOFRAMES:	fNoFrame = true;				break;

			default:
				if (!IsStyleTag(tagID, tagList))
				{
					ImportantMessage(("Unhandled Tag: %d", tagID));
#if defined(FOR_MAC) && defined(LOG_BAD_TAGS_AND_ATTRIBUTES)
					gUnhandledTagLog.Add(fTag,
						(fResource == nil) ? kUnknownString : fResource->GetURL());
#endif // defined(FOR_MAC) && defined(LOG_BAD_TAGS_AND_ATTRIBUTES)
				}
		}
	}
	else
	{					// Stop tags </TAG>
		switch (-tagID) 
		{			
			case T_HTML:
			case T_HEAD:										break;
			
			case T_TITLE:		fInTitle = false;				break;
			case T_BODY:										break;

			case T_DIV:			CloseDivision();				break;
			case T_CENTER:		CloseCenter();					break;
			case T_P:			CloseParagraph();				break;

			case T_FN:
			case T_A:			CloseAnchor();					break;
			
			case T_FONT:
			case T_BIG:
			case T_SMALL:		PopStyle();						break;
	
			case T_TABLE:		CloseTable();					break;
			case T_TR:			CloseTableRow();				break;
			case T_TH:		
			case T_TD:			CloseTableCell();				break;
			case T_CAPTION:		CloseCaption();					break;
			
			case T_FORM:		CloseForm();					break;
			case T_SELECT:		CloseSelect();					break;
			case T_TEXTAREA:	CloseTextArea();				break;
			case T_NOBR:		/* ignore this request */		break;
			case T_MAP:			CloseImageMap();				break;
			case T_SIDEBAR:		CloseSideBar();					break;
			
			case T_SCRIPT:		fInScript = false;				break;
			
			case T_FRAME:
			case T_NOFRAME:
			case T_NOFRAMES:									break;

			default:
				if (!IsStyleTag(tagID, 0))
				{
					ImportantMessage(("Unhandled Tag: %d", tagID));
#if defined(FOR_MAC) && defined(LOG_BAD_TAGS_AND_ATTRIBUTES)
					gUnhandledTagLog.Add(fTag,
						(fResource == nil) ? kUnknownString : fResource->GetURL());
#endif // defined(FOR_MAC) && defined(LOG_BAD_TAGS_AND_ATTRIBUTES)
				}
		}
		
	}
}

void DocumentBuilder::AddText(const char* s, short count)
{
	if (count == 0)
		return;

	if (fInScript)
		return;
			
	if (fInTitle) {
		fDocument->SetTitle(s);
		ImportantMessage(((char*)s));
		return;
	}
	
	if (fTextArea != nil) {
		fTextArea->AddDefaultText(s);
		return;
	}
	
	// Let the form have first crack at the text, return if it handled it
	if (fForm && fForm->AddText(s, count))
		return;
	
	long offset = fDocument->AddText(s, count);	// Make the document remember text
	
	// Text couldn't be added, so don't create the displayable.
	PostulateFinal(false);	// Need to AbortAndTerminate.
	if (offset < 0)
		return;

	Text*	text;

	if (fCurrentStyle.style.color || fCurrentStyle.transparency != 0)
	{
		ColorText*	colorText = new(ColorText);
		colorText->SetColor(fCurrentStyle.color);
		colorText->SetTransparency(fCurrentStyle.transparency);
		text = colorText;
	}
	else
		text = new(Text);
		
	PackedStyle currentStyle = fCurrentStyle.style;
	if (fAnchor != nil && fAnchor->HasURL() && !fNoColorAnchor) {
		currentStyle.anchor = true;
		if (fVisitedAnchor)
			currentStyle.visited = true;
	}
	
	text->SetText(offset, count, currentStyle);
	fDisplayable->AddChild(text);
}

void DocumentBuilder::Align(AttributeValue align, Boolean force)
{
	if (align != fCurrentAlign || force) {
		Alignment* g = new(Alignment);	// Set the current alignment
		g->SetAlign(align);
		fDisplayable->AddChild(g);
		fCurrentAlign = align;
	}
}

void DocumentBuilder::BaseFont(const TagList* tagList)
{
	const Tag* tag;	
	if (tagList != nil && (tag = tagList->Find(A_SIZE)) != nil) {
		if (tag->fIsPercentage)
			fBaseFont = MIN(7, MAX(0, tag->fValue + fBaseFont));	// Relative
		else
			fBaseFont = MIN(7, MAX(0, tag->fValue));				// Absolute
		fCurrentStyle.style.fontSize = fBaseFont;
	}
}

void DocumentBuilder::Big(const TagList*)
{
	PushStyle();	
	fCurrentStyle.style.fontSize = MIN(7, fCurrentStyle.style.fontSize + 1);
}

void DocumentBuilder::BlankLine()
{
	// Add a Blank line if there isn't one there already
	
	Displayable* g, *gg;
	
	// Determine if this is the first line in document, ignoring floaters & anchors.
	g = fDisplayable->GetFirstChild();	
	while (g != nil && (g->IsFloating() || g->IsAnchor() || g->IsAnchorEnd()))
		g = (Displayable*)g->Next();
	
	// First in document, align only
	if (g == nil)	
		return;

	g = fDisplayable->GetLastChild();
	gg = (Displayable *)g->Previous();
	if (g->IsExplicitLineBreak()) 
	{		
		if (!(gg != nil && gg->IsExplicitLineBreak()))
			AddNewLine(nil);
	} 
	else {		
		AddNewLine(nil);
		AddNewLine(nil);
	}
}

void DocumentBuilder::CloseAnchor()
{
	if (fAnchor == nil)
		return;

	AnchorEnd*	anchorEnd = new(AnchorEnd);
	
	fDocument->AddAnchor(fAnchor);
	fDisplayable->AddChild(anchorEnd);
	
	fAnchor = nil;
	fNoColorAnchor = false;
	fVisitedAnchor = false;
}
						
void DocumentBuilder::CloseCaption()
{
	if (fTable == nil)
		return;
		
	fTable->CloseCaption();

	// Restore style to the stayle in effect at caption open.
	fStyleStackDepth = fDisplayable->GetStyleStackDepth();
	PopStyle();
	
	fDisplayable = (Page*)fTable->GetParent();
}

void DocumentBuilder::CloseCenter()
{
	PopAlignment();
	Align(fCurrentAlign, true);
}

void DocumentBuilder::CloseDivision()
{
	CloseParagraph();
	PopAlignment();
	Align(fCurrentAlign, true);
}

void DocumentBuilder::CloseForm()
{
	if (fForm == nil)
		return;
	
	CloseTextArea();
	CloseParagraph();
		
	fForm = nil;
		
	// Text following form must align.
	LineBreak*	lineBreak = new(LineBreak);
	lineBreak->SetBreakType(LineBreak::kHardAlign);
	fDisplayable->AddChild(lineBreak);
}

void DocumentBuilder::CloseHeading()
{
	BlankLine();
	PopAlignment();
	Align(fCurrentAlign);
}

void DocumentBuilder::CloseImageMap()
{
	if (fImageMap != nil)
	{
		fImageMap->SetComplete();		
		fImageMap = nil;
	}
}

void DocumentBuilder::CloseParagraph()
{
	if (fInParagraph) {
		PopAlignment();
		Align(fCurrentAlign);
		BlankLine();
		fInParagraph = false;
	}
}

void DocumentBuilder::CloseSelect()
{
	Menu* menu;
	
	if (fForm == nil)
		return;

	// add the form's current menu to the displayable list.
	menu = fForm->GetSelect();
	if (menu != nil)
		fDisplayable->AddChild(menu);
	
	fForm->CloseSelect();
}

void DocumentBuilder::CloseSideBar()
{
	if (fSideBar == nil)
		return;
		
	fSideBar->Close();
	fDocument->AddSideBar(fSideBar);
	fSideBar = nil;
	
	// Restore style to the style in effect at cell open.
	fStyleStackDepth = fDisplayable->GetStyleStackDepth();
	PopStyle();
	
	fDisplayable = fDocument->GetRootDisplayable();
}

void DocumentBuilder::CloseTable()
{
	if (fTable == nil)
		return;

	CloseTableRow();		
	fTable->Close();
	
	// Table must align.
	LineBreak*	lineBreak = new(LineBreak);
	lineBreak->SetBreakType(LineBreak::kHardAlign);
	fDisplayable->AddChild(lineBreak);

	if (fTable->IsFloating())
		fDisplayable->AddFloatingChild(fTable);
	else
		fDisplayable->AddChild(fTable);
	
	// Text following table must align.
	lineBreak = new(LineBreak);
	lineBreak->SetBreakType(LineBreak::kHardAlign);
	fDisplayable->AddChild(lineBreak);

	fTable = fTable->GetParentTable();
}

void DocumentBuilder::CloseTableCell()
{
	CloseAnchor();

	if (fTable == nil || !fTable->IsInCell())
		return;
	
	fTable->CloseCell();
	
	// Restore style to the style in effect at cell open.
	fStyleStackDepth = fDisplayable->GetStyleStackDepth();
	PopStyle();
	
	// Restore alignment to the alignment in effect at cell open.
	fAlignStackDepth = fDisplayable->GetAlignStackDepth();
	PopAlignment();
	
	fDisplayable = (Page*)fTable->GetParent();
}

void DocumentBuilder::CloseTableRow()
{
	if (fTable == nil)
		return;
		
	CloseTableCell();
	fTable->CloseRow();
}

void DocumentBuilder::CloseTextArea()
{
	if (fTextArea == nil)
		return;
	
	fForm->AddControl(fTextArea);
	fDisplayable->AddChild(fTextArea);
	fTextArea = nil;
}

Control* DocumentBuilder::CreateInput(short inputType, const TagList* tagList)
{
	Control* control = nil;
	const Tag*	tag = nil;
	
	switch (inputType) 
	{		
		case AV_TEXT:		control = new(TextField); break;
		case AV_PASSWORD:	control = new(InputPassword); break;
		case AV_IMAGE:		control = new(InputImage); break;
		case AV_CHECKBOX:	control = new(CheckBox); break;
		case AV_RADIO:		control = new(RadioButton); break;
		case AV_BUTTON:		control = new(SubmitButton); break;
		case AV_SUBMIT:		control = new(SubmitButton); break;
		case AV_RESET:		control = new(ResetButton); break;
		case AV_HIDDEN:		control = new(InputHidden); break;
		case T_SELECT:
			tag = (tagList != nil ? tagList->Find(A_SIZE) : nil);			
			if (tagList && (tagList->Find(A_MULTIPLE) || (tag != nil && tag->fValue > 1)))
				control = new(ScrollingList);
			else
				control = new(PopUpMenu);
			break;
		default:
			ImportantMessage(("Unknown input type %d", inputType));
			control = new(TextField);
			break;
	}

	return control;
}

void DocumentBuilder::Display(const TagList* tagList)
{
	SetDisplayableAttributes(fDocument, tagList);
}

void DocumentBuilder::Finalize()
{
	// Close any open tables, anchors forms, etc.
	while (fTable != nil)
		CloseTable();
	
	CloseAnchor();
	CloseSelect();
	CloseForm();
	CloseImageMap();
	
	fDocument->Finalize();
	
	// If we have a frame, and there was not a NOFRAME tag present, and we
	// don't have any content, show the first frame's page.
	if (fFrameSRC != nil && !fNoFrame && fDocument->GetHeight() == 0 && fDocument->GetImageCount() == 0) {
		fDocument->SetSkipBack(true);
		fDocument->SetReloadTime(0);
		fDocument->SetReloadURL(fFrameSRC);
		fDocument->StartReloadTiming();
	}
}

void DocumentBuilder::Font(const TagList* tagList)
{
	if (tagList != nil)
	{
		PushStyle();	
		for (long i = 0; i < tagList->GetCount(); i++) {
			const Tag* tag = tagList->At(i);
			switch (tag->fAttributeID) 
			{
				case A_COLOR:
					if (tag->fValue >= 0) {
						fCurrentStyle.color = tag->fValue;
						fCurrentStyle.style.color = true;
					}
					break;
				case A_SIZE:
					if (tag->fIsPercentage)
						fCurrentStyle.style.fontSize = MIN(7, MAX(0, tag->fValue + fBaseFont));	// Relative
					else
						fCurrentStyle.style.fontSize = MIN(7, MAX(0, tag->fValue));				// Absolute
					break;
				case A_EFFECT:
					switch (tag->fValue) {
						case AV_NONE:	fCurrentStyle.style.effect = kNoEffect;		break;
						case AV_RELIEF:	fCurrentStyle.style.effect = kReliefEffect;	break;
						case AV_EMBOSS:	fCurrentStyle.style.effect = kEmbossEffect;	break;
						case AV_OUTLINE:fCurrentStyle.style.effect = kOutlineEffect;break;
						default:		fCurrentStyle.style.effect = kNoEffect;		break;
					}
					break;
				case A_TRANSPARENCY:
					fCurrentStyle.transparency = MIN(MAX(tag->fValue, 0), 255);
					break;
				default:
					break;
			}
		}
	}
}

Document* DocumentBuilder::GetDocument() const
{
	return fDocument;
}

Boolean DocumentBuilder::IsStyleTag(short tagID, const TagList* tagList)
{
	// IsStyleTag determines if tag sets styles that define a block of text
	// A T_H2 tag is a style tag because it overrides any current style
	// a T_B tag isn't because its effects are local
	// Tags that can be defined in terms of linebreaks and styles?
	
	if (tagID > 0) 
	{		
		switch (tagID) 
		{			
			case T_H1:				// Headings
			case T_H2:
			case T_H3:
			case T_H4:
			case T_H5:
			case T_H6:
				OpenHeading(tagList);
				NewStyle(tagID);
				return true;
			
			case T_LI:				// List Item
				AddBullet(tagList);
				return true;
				
			case T_DL:				// Lists
			case T_OL:
			case T_UL:
			case T_DIR:
			case T_MENU:
				PushList(tagID, tagList);	// Fall thru
				NewStyle(tagID);
				return true;
				
			case T_PRE:
			case T_XMP:
			case T_LISTING:
				fInPRE = true;
			case T_BLOCKQUOTE:
				BlankLine();
				NewStyle(tagID);
				return true;

			case T_ADDRESS:
			case T_DT:				// Glossary List Term
			case T_DD:				// Glossary List Definition
			
			case T_TT:
			case T_B:
			case T_I:
			case T_U:
			case T_SHADOW:
			case T_SUB:
			case T_SUP:
			case T_KBD:
			case T_SAMP:
			case T_CODE:
			case T_EM:
			case T_CITE:
			case T_VAR:
			case T_STRONG:
			case T_DFN:
				NewStyle(tagID);
				return true;
		}
	}
	else 
	{		
		switch (-tagID) 
		{			
			case T_H1:				// Headings
			case T_H2:
			case T_H3:
			case T_H4:
			case T_H5:
			case T_H6:
				NewStyle(tagID);
				CloseHeading();
				return true;
				
			case T_DL:				// Lists
			case T_OL:
			case T_UL:
			case T_DIR:
			case T_MENU:
				PopList();			// Forget the kind of list
				NewStyle(tagID);
				return true;

			case T_PRE:
			case T_XMP:
			case T_LISTING:
				fInPRE = false;
			case T_BLOCKQUOTE:
				NewStyle(tagID);
				BlankLine();
				return true;

			case T_DT:				// Glossary List Term
			case T_DD:				// Glossary List Definition
			case T_ADDRESS:

			case T_TT:
			case T_B:
			case T_I:
			case T_U:
			case T_SHADOW:
			case T_SUB:
			case T_SUP:
			case T_KBD:
			case T_SAMP:
			case T_CODE:
			case T_EM:
			case T_CITE:
			case T_VAR:
			case T_STRONG:
			case T_DFN:
				NewStyle(tagID);
				return true;
		}
	}
	return false;
}	

void DocumentBuilder::MoveMargin(short count, Boolean permanent)
{
	// Reposition margin according to a tag
	// DD tags have a count of zero and are handled differently:
	// 	If a DT is not very wide (< 32 pixels) dont start the DD on a new line
	// 	DefinitionMargin checks the page and will not break if (fHPos - fLeftMargin) < 32;
	
	short depth = MAX(0, fMarginDepth + count);
	Margin* g = count ? new(Margin) : new(DefinitionMargin);	// See above
	g->SetMargin(MIN(depth, kMaxMarginDepth));
	fDisplayable->AddChild(g);
	if (permanent)
		fMarginDepth += count;
}

void DocumentBuilder::NewStyle(short tagID)
{
	// Lookup a tag in the style list, move margins and set style
	
	short t = (tagID < 0) ? -tagID : tagID;
	short i;
	
	for (i = 0; gStyleList[i].tagID != -1; i++)
		if (gStyleList[i].tagID == t)
			break;
	if (gStyleList[i].tagID == -1)
		i = 0;
	
	if (!gStyleList[i].marginOnly)	
		SetTagStyle(gStyleList[i].style, tagID > 0);	// Set or unset a style
		
	if (gStyleList[i].hasMargin) 
	{		
		if (tagID > 0)
			MoveMargin(gStyleList[i].count, gStyleList[i].permanent);
		else
			MoveMargin(-gStyleList[i].count, gStyleList[i].permanent);
	}
}

void DocumentBuilder::OpenAnchor(const TagList* tagList)
{
	// Close current anchor if one is open.
	CloseAnchor();
	
	Anchor* g = new(Anchor);
	SetDisplayableAttributes(g, tagList);
	fDisplayable->AddChild(g);
	fAnchor = g;
	
	if (tagList != nil && tagList->Find(A_NOCOLOR) != nil)
		fNoColorAnchor = true;
	
	// If we don't know the service, discard the HREF.	
	if (g->HasURL() && !gServiceList->HasService(fDocument->GetBaseResource(), g->GetHREF()))
		g->ResetHREF();
		
	if (g->HasURL() && gPageViewer->WasVisited(fDocument->GetBaseResource(), g->GetHREF()))
		fVisitedAnchor = true;
}

void DocumentBuilder::OpenCaption(const TagList* tagList)
{
	if (fTable == nil)
		return;

	Cell* g = new(Cell);
	SetDisplayableAttributes(g, tagList);

	fTable->OpenCaption(g);
	fDisplayable = g;

	// Save current style and stack depth to restore on table close.
	PushStyle();
	fDisplayable->SetStyleStackDepth(fStyleStackDepth);
	fCurrentStyle = gDefaultColorStyle;
}
			
void DocumentBuilder::OpenCenter(const TagList*)
{
	PushAlignment();
	Align(AV_CENTER, true);
}

void DocumentBuilder::OpenDivision(const TagList* tagList)
{
	const Tag*		tag = nil;
	AttributeValue	align = fCurrentAlign;
	
	if (tagList != nil && (tag = tagList->Find(A_ALIGN)) != nil)
		align = (AttributeValue)tag->fValue;
		
	PushAlignment();
	Align(align, true);
}

void DocumentBuilder::OpenForm(const TagList* tagList)
{
	Form* form = new(Form);

	// Note: Form's document must be set before adding tags.	
	fDocument->AddForm(form);
	fForm = form;

	SetDisplayableAttributes(form, tagList);
	
	// Forms must start new line.
	LineBreak*	lineBreak = new(LineBreak);
	lineBreak->SetBreakType(LineBreak::kHardAlign);
	fDisplayable->AddChild(lineBreak);
}


void DocumentBuilder::OpenHeading(const TagList* tagList)
{
	const Tag*	tag = nil;
	
	PushAlignment();
	if (tagList != nil && (tag = tagList->Find(A_ALIGN)) != nil)
		Align((AttributeValue)tag->fValue);
		
	BlankLine();		// Need at lest one space before heading
}

void DocumentBuilder::OpenImageMap(const TagList* tagList)
{
	CloseImageMap();

	const char*	name = nil;
	
	const Tag* tag;
	if (tagList != nil && (tag = tagList->Find(A_NAME)) != nil)
		name = tag->GetValueAsString();
		
	// A map of this name may already have been added by earlier access from an image. In that
	// case we'll just find it, otherwise a new one is added.	
	if (name != nil)
		fImageMap = fDocument->FindOrAddImageMap(fDocument->GetBaseResource(), name, kClientMap);
}

void DocumentBuilder::OpenOption(const TagList* tagList)
{
	if (fForm != nil) {
		fForm->Option();
		SetDisplayableAttributes(fForm, tagList);
	}
}

void DocumentBuilder::OpenParagraph(const TagList* tagList)
{
	const Tag*		tag = nil;
	
	CloseParagraph();
	fInParagraph = true;

	PushAlignment();
	if (tagList != nil && (tag = tagList->Find(A_ALIGN)) != nil)
		Align((AttributeValue)tag->fValue);

	if (!fInPRE)
		BlankLine();
}

void DocumentBuilder::OpenSelect(const TagList* tagList)
{
	Menu* menu;
	
	if (fForm == nil) 
		return;
		
	menu = (Menu*)CreateInput(T_SELECT, tagList);
	SetDisplayableAttributes(menu, tagList);
	fForm->OpenSelect(menu);
}

void DocumentBuilder::OpenSideBar(const TagList* tagList)
{
	CloseSideBar();

	if (fDocument->GetSideBar() == nil) {		
		fSideBar = new(SideBar);
		SetDisplayableAttributes(fSideBar, tagList);
		fSideBar->Open();
		
		fDisplayable = fSideBar;
	
		// Save current style and stack depth to restore on table close.
		PushStyle();
		fDisplayable->SetStyleStackDepth(fStyleStackDepth);
		fCurrentStyle = gDefaultColorStyle;
	}
}

void DocumentBuilder::OpenTable(const TagList* tagList)
{
	// If we're in a table and not currently in a cell, close the table.
	if (fTable != nil && !fTable->IsInCell())
		CloseTable();
		
	Table* table = new(Table);
	
	SetDisplayableAttributes(table, tagList);

	table->SetParentTable(fTable);
	table->SetParent(fDisplayable);	
	fTable = table;
	
	table->Open();
}

void DocumentBuilder::OpenTableRow(const TagList* tagList)
{
	if (fTable == nil) 
		return;
	
	CloseTableRow();
	fTable->OpenRow();
	SetDisplayableAttributes(fTable, tagList);	// Row attributes affect table
}

void DocumentBuilder::OpenTableCell(const TagList* tagList, Boolean isHeading)
{
	if (fTable == nil)
		return;
	
	CloseTableCell();
		
	Cell* g = fTable->NewCell(isHeading);
	SetDisplayableAttributes(g, tagList);

	fTable->OpenCell(g, isHeading);
	fDisplayable = g;

	// Save current style and stack depth to restore on table close.
	PushStyle();
	fDisplayable->SetStyleStackDepth(fStyleStackDepth);
	fCurrentStyle = gDefaultColorStyle;
	
	PushAlignment();
	fDisplayable->SetAlignStackDepth(fAlignStackDepth);
	fCurrentAlign = fDisplayable->GetPageAlign();
	
	if (isHeading)
		fCurrentStyle.style.bold = true;
}

void DocumentBuilder::OpenTextArea(const TagList* tagList)
{
	if (fForm == nil)
		return;
		
	TextField* control = new(TextField);
	control->SetIsTextArea(true);
	SetDisplayableAttributes(control, tagList);
	fTextArea = control;
}

void DocumentBuilder::PopList()
{
	if (fListDepth)
		fListDepth--;	// Forget the kind of list, fall thru
	if (fListDepth == 0)
		BlankLine();
}

void DocumentBuilder::PopStyle()
{
	if (fStyleStackDepth > 0)
	{
		fStyleStackDepth--;
		if (fStyleStackDepth < 100)
			fCurrentStyle = fStyleStack[fStyleStackDepth];
	}
}

void DocumentBuilder::PopAlignment()
{
	if (fAlignStackDepth > 0)
	{
		fAlignStackDepth--;
		if (fAlignStackDepth < 100)
			fCurrentAlign = fAlignStack[fAlignStackDepth];
	}
}

void DocumentBuilder::PushList(short tagID, const TagList* tagList)
{
	// Determine the kind of oranment to be used at this list depth
	
	short start = 1;
	short type;	// Default to dots
	
	if (tagID == T_OL)	// Ordered lists like numbers
		type = '1';
	else 
	{		
		type = 'd';		// Everybody else has dots
		if (fListDepth)
			switch (fListDepth % 3) 
			{				
				case 0:	type = 's';	break;
				case 1:	type = 'd';	break;
				case 2:	type = 'c';	break;
			}
	}
		
	for (long i = 0; tagList != nil && i < tagList->GetCount(); i++) {
		const Tag* tag = tagList->At(i);
		switch (tag->fAttributeID) 
		{				
			case A_TYPE:	
				const char *value;
				value = tag->GetValueAsString();
				type = value ? *value : 0;
				break;	// Kind is A|a|I|i|1 for ordered lists, d|c|s for UL
			case A_START:	if (tag->fValue >= 0) start = tag->fValue;	break;
			default:		break;
		}
	}

	if (fListDepth == 0)
		BlankLine();	// Add a blank line before first list 

	if (fListDepth < 50) 
	{		
		fListStack[++fListDepth] = type;
		fListIndex[fListDepth] = start;
	}
}

void DocumentBuilder::PushStyle()
{
	if (fStyleStackDepth < 100)
		fStyleStack[fStyleStackDepth] = fCurrentStyle;
	fStyleStackDepth++;
}

void DocumentBuilder::PushAlignment()
{
	if (fAlignStackDepth < 100)
		fAlignStack[fAlignStackDepth] = fCurrentAlign;
	fAlignStackDepth++;
}

void DocumentBuilder::SetResource(const Resource* resource)
{
	fDocument->SetResource(resource);
}

void DocumentBuilder::SetTagStyle(PackedStyle newStyle, Boolean set)
{
	if (set) {
		PushStyle();
		if (newStyle.fontSize != 0)
			fCurrentStyle.style.fontSize = newStyle.fontSize;
		if (newStyle.monoSpaced) fCurrentStyle.style.monoSpaced = newStyle.monoSpaced;
		if (newStyle.bold) fCurrentStyle.style.bold = true;
		if (newStyle.italic) fCurrentStyle.style.italic = true;
		if (newStyle.shadow) fCurrentStyle.style.shadow = true;
		if (newStyle.subscript) fCurrentStyle.style.subscript = true;
		if (newStyle.superscript) fCurrentStyle.style.superscript = true;
		if (newStyle.underline) fCurrentStyle.style.underline = true;
	} else
		PopStyle();
}

void DocumentBuilder::Small(const TagList*)
{
	PushStyle();	
	fCurrentStyle.style.fontSize = MAX(1, fCurrentStyle.style.fontSize - 1);
}


// ============================================================================
// ImageMapBuilder

ImageMapBuilder::ImageMapBuilder()
{
}

ImageMapBuilder::~ImageMapBuilder()
{
	if (fBaseURL != nil)
		FreeTaggedMemory(fBaseURL, "ImageMapBuilder::fBaseURL");
}

void ImageMapBuilder::AddArea(const TagList* tagList)
{
	if (fCurrentMap != nil)
	{		
		Area*	area = new(Area);
		
		SetDisplayableAttributes(area, tagList);
		fCurrentMap->AddArea(area);
	}
}
	
void ImageMapBuilder::AddBase(const TagList* tagList)
{
	const Tag*	tag;
	if (tagList != nil && (tag = tagList->Find(A_HREF)) != nil)
		fBaseURL = CopyStringTo(fBaseURL, tag->GetValueAsString(), "ImageMapBuilder::fBaseURL");
}

void ImageMapBuilder::AddTag(short tagID, const TagList* tagList)
{
	if (tagID > 0)
	{
		switch (tagID) 
		{
			case T_AREA:		AddArea(tagList);				break;
			case T_BASE:		AddBase(tagList);				break;
			case T_MAP:			OpenImageMap(tagList);			break;
		}
	}
	else
	{
		switch(-tagID)
		{
			case T_MAP:			CloseImageMap();				break;
		}
		
	}
}

void ImageMapBuilder::CloseImageMap()
{
	if (fCurrentMap != nil)
	{
		fCurrentMap = nil;	// Clear the current map
		fTargetMap = nil; 	// Clear the target map, we've built it!!
	}
}

void ImageMapBuilder::Finalize()
{
	CloseImageMap();
}

ImageMap* ImageMapBuilder::GetTargetMap() const
{
	return fTargetMap;
}

void ImageMapBuilder::OpenImageMap(const TagList* tagList)
{
	CloseImageMap();
	
	// If we already found our target, ignore this map.
	if (fTargetMap == nil)
		return;

	const char*	name = nil;
	
	const Tag* tag;
	if (tagList != nil && (tag = tagList->Find(A_NAME)) != nil)
		name = tag->GetValueAsString();
		
	// If the name matches the target, this is the one we're looking for.
	if (name != nil && strcmp(fTargetMap->GetName(), name ) == 0)
	{
		fCurrentMap = fTargetMap;
		if (fBaseURL != nil)
			fCurrentMap->SetBaseURL(fBaseURL);
	}
}

void ImageMapBuilder::OpenImageMap()
{
	fCurrentMap = fTargetMap;
}

void ImageMapBuilder::SetTargetMap(ImageMap* map)
{
	fTargetMap = map;
}
	
// ============================================================================
// PrefetchBuilder

PrefetchBuilder::PrefetchBuilder()
{
}

PrefetchBuilder::~PrefetchBuilder()
{
}

void PrefetchBuilder::AddResource(const TagList* tagList)
{
	const Tag*	tag;
	
	if (tagList != nil && (tag = tagList->Find(A_SRC)) != nil) {
		const char* src = tag->GetValueAsString();
		if (src != nil) {
			Resource*	resource = new(Resource);
			resource->SetURL(src, &fResource);
			resource->SetPriority(kBackground);
			delete(resource);
		}
	}
}
	
void PrefetchBuilder::AddTag(short tagID, const TagList* tagList)
{
	switch (tagID)
	{
		case T_IMG:
		case T_IMAGE:
		case T_BGSOUND:		AddResource(tagList); 		break;
	}
}

void PrefetchBuilder::SetResource(const Resource* resource)
{
	if (!IsError(resource == nil))
		fResource = *resource;
}

// =============================================================================

DocumentPrefetcher::DocumentPrefetcher()
{
	fState = kCreatePrefetchStream;
}

DocumentPrefetcher::~DocumentPrefetcher()
{
	Reset();
}

Error DocumentPrefetcher::GetStatus() const
{
	return fState >= kPrefetchDone ? kComplete : kPending;
}
	
void DocumentPrefetcher::Idle()
{
	PerfDump perfdump("DocumentPrefetcher::Idle");

	switch (fState)
	{
		case kCreatePrefetchStream:
			if (fResource.GetStatus() < 0) {
				fState = kPrefetchError;
				break;
			}
			if ((fStream = fResource.NewStream()) == nil) 
				break;
		
			fBuilder.SetResource(&fResource);
			fParser.SetBuilder(&fBuilder);
			fState = kPrefetchParse;
			
		case kPrefetchParse:
		{			
			Error status = fStream->GetStatus();
			
			if (status == kStreamReset) {
				Reset();
				break;
			}
			
			if (TrueError(status)) {
				delete(fStream);
				fStream = nil;
				fState = kPrefetchError;
				break;
			}
			
			fParser.Parse(fStream);

			// If the stream is complete, or target has been built, we're done.			
			if (fStream->GetStatus() == kComplete && fStream->GetPending() == 0)
			{
				delete(fStream);
				fStream = nil;
				fState = kPrefetchDone;
			}
			break;
		}
			
		case kPrefetchDone:
		case kPrefetchError:
			break;
			
		default:
			IsError(true); // Should never reach here.
			break;
	}
}

void DocumentPrefetcher::Load(const char* url, const Resource* parent)
{
	if (IsError(url == nil || parent == nil))
		return;
	
	fResource.SetURL(url, parent);
	fResource.SetPriority(kBackground);
	fState = kCreatePrefetchStream;
}

void DocumentPrefetcher::Reset()
{
	delete(fStream);
	fStream = nil;
	fState = kCreatePrefetchStream;
}

// =============================================================================

