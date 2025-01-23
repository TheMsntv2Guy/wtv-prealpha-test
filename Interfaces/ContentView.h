// Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.

#ifndef __CONTENTVIEW_H__
#define __CONTENTVIEW_H__

#ifndef __DOCUMENT_H__
#include "Document.h"
#endif

#ifndef __RESOURCE_H__
#include "Resource.h"
#endif

#ifndef __SCREEN_H__
#include "Screen.h"
#endif

#ifndef __LAYER_H__
#include "Layer.h"
#endif


//#define SPATIAL_NAVIGATION 1

const long kInvalidScrollPosition = -1;
const long kPageOverlap = 35;


class Displayable;
class Document;
class DocumentBuilder;
class HTMLParser;
class SelectionLayer;

enum ViewIdleState {							
	kViewIdle = 0,					// Nothing to do.
	kWaitForDocument,				// Kicked off request, wait for document data.
	kBeginDocument,					// Receiving data, parse and wait for title or end.
	kDocumentTitleComplete,			// Received title, parse content and wait for end.
	kDocumentContentComplete,		// All content received, continue layout for images.
	kDocumentLayoutComplete,		// All layout complete. Continue loading images.
	kDocumentLoadingComplete		// All images loaded, document it complete. Do nothing.
};

enum DrawIdleState {
	kDrawIdle = 0,					// Nothing to do.
	kWaitForInitialScreen,			// Wait for the initial visible page of data to transition in.
	kWaitForFirstScreenDrawn,		// Wait for 1st screen to be drawing complete
	kWaitForCurrentScreen,			// Still laying out.
	kDrawingComplete				// Layout complete, no more drawing.
};

//==================================================================================

class ContentView : public Layer {
public:
							ContentView();
	virtual					~ContentView();

	// Getters.
	Boolean					CanScrollDown() const;
	Boolean					CanScrollUp() const;
	Boolean					Completed();
	const Resource*			GetBaseResource() const;
	ulong					GetCurrentInputTime() const;
	Displayable*			GetCurrentSelectable() const;
	Selection				GetCurrentSelection() const;
	Document*				GetDocument() const;
	Coordinate				GetMapCursorPosition() const;
	uchar					GetPercentComplete() const;
	const Resource*			GetResource() const;
	void					GetScrollBounds(Rectangle*) const;
	long					GetScrollPosition() const;
	SelectionLayer*			GetSelectionLayer() const;
	Boolean					GetShowSelection() const;
	short					GetTopMargin() const;
	virtual Boolean			IsContentView() const;
	Boolean					IsSelected(Displayable*) const;
	Boolean					ShouldPaintBackground() const;

	// Operations.	
	void					BackgroundToScreen(Rectangle* bounds) const;
	void					ContentToScreen(Rectangle* bounds) const;
	void					ContentToScreen(Region* region) const;
	virtual Boolean			DispatchInput(Input*);
	virtual Boolean			ExecuteInput();
	virtual void			ExecuteURL(const char* url, const char* formData = nil);
	virtual Boolean			DownInput(Input*);
	virtual void			Draw(const Rectangle* invalid);
#ifdef FIDO_INTERCEPT
	virtual void			Draw(class FidoCompatibilityState& fidoCompatibility) const;
#endif
	virtual Boolean			KeyboardDownInput(Input*);
	virtual Boolean 		KeyboardInput(Input*);
	virtual Boolean			KeyboardLeftInput(Input*);
	virtual Boolean			KeyboardRightInput(Input*);
	virtual Boolean			KeyboardUpInput(Input*);
	virtual Boolean			LeftInput(Input*);	
	void					PlaySongSoon();
	void					PostTargetFragment(const char* fragmentName);
	void					RefreshSelection();
	void					ReLayout();				
	void					Reload();
	void					ReloadAll();
	void					RequestLayout(Displayable* requestor);			
	virtual Boolean			RightInput(Input*);
	Boolean					ScrollToFragment(const char* fragmentName);
	void					ScreenToBackground(Rectangle* bounds) const;
	void					ScreenToContent(Rectangle* bounds) const;
	virtual void			SetBounds(const Rectangle*);
	void					SetBounds(Ordinate left, Ordinate top, Ordinate right, Ordinate bottom);
	void					SetCurrentSelectableWithScroll(Displayable*);
	void					SetShowSelection(Boolean);
	void					SetTopMargin(short);
	virtual void			ShowFragment(const char*);
	virtual void			ShowResource(const Resource*);
	void					StopSong();
	virtual Boolean			UpInput(Input*);	
	void					VisibleContentBounds(Rectangle&) const;
	void					VisibleBackgroundBounds(Rectangle&) const;
	
	virtual Boolean			ScrollDown(long suggestedIncrement, long minIncrement);
	virtual Boolean			ScrollUp(long suggestedIncrement, long minIncrement);

protected:

	virtual ViewIdleState	HandleNonHTMLResource(DataType);
	virtual ViewIdleState	HandleNonShowableResource(DataType);
	
	// Idle states.
	virtual void			Idle();
	virtual ViewIdleState	IdleWaitForDocument();
	virtual	ViewIdleState	IdleBeginDocument();
	virtual	ViewIdleState	IdleDocumentTitleComplete();
	virtual	ViewIdleState	IdleDocumentContentComplete(Boolean changed);
	virtual	ViewIdleState	IdleDocumentLayoutComplete();
	virtual	void			ChangeIdleState(ViewIdleState newState, ViewIdleState oldState);

	
	void					DeleteDocument();
	void					DeleteStream();

	virtual void			DocumentHeightChanged(long newHeight, long oldHeight);

	Resource*				NewHTMLResourceForImage(const Resource*);

	void					ParseDocument();
	void					PostTargetSelection(Selection selection);
	void					SetCurrentSelection(Selection selection);
	
	virtual	void			InitialScreenLayoutCompleted();
	virtual	void			ScreenLayoutCompleted();
	virtual	void			FirstScreenDrawingCompleted();
	
	
	Selection				NextAnchorWithScroll(Selection current, Boolean goHorizontal,
												 Boolean autoRepeat);
	Selection				NextVisibleAnchor(Selection current, Boolean wrap = false);
	Selection				NextVisibleAnchorBelow(Selection current, Boolean wrap = false,
												   Boolean requireSibling = true);
#ifdef SPATIAL_NAVIGATION
	virtual Selection		NextVisibleAnchorRight(Selection current, Boolean wrap = false);
#endif
	Selection				PreviousAnchorWithScroll(Selection current, Boolean goHorizontal,
													 Boolean autoRepeat);
	Selection				PreviousVisibleAnchor(Selection current, Boolean wrap = false);
	virtual Selection		PreviousVisibleAnchorAbove(Selection current, Boolean wrap = false,
												 	   Boolean requireSibling = true);
#ifdef SPATIAL_NAVIGATION
	virtual Selection		PreviousVisibleAnchorLeft(Selection current, Boolean wrap = false);
#endif
	void					RefreshSelection(Selection selection);
	void					SaveSelectionPosition();
	void					SaveSelectionHPosition();
	void					SaveSelectionVPosition();
	void					ScrollToPosition(long position);
	Boolean					ScrollToSelection(Selection selection);
	void					SelectFirstAnchor();
	void					SelectNextAnchor(Boolean goHorizontal, Boolean autoRepeat);
	void					SelectPreviousAnchor(Boolean goHorizontal, Boolean autoRepeat);
	void					SelectTopAnchor(Selection startHint = kInvalidSelection);
	Boolean					ShouldLeaveSideBar(Selection current);
	Boolean					ShouldEnterSideBar(Selection current);
	Selection				VisibleAnchorInNewPage(Selection current, long newPage);
 
protected:
	friend class			SelectionLayer;
	
	ulong					fCurrentInputTime;
	Song*					fCurrentSong;
	long					fScrollPosition;
	Document*				fDocument;
	DocumentBuilder*		fDocumentBuilder;
	ulong					fDocumentBeginTime;
	Displayable*			fRequestedLayout;
	Document*				fLastDocument;
	Parser*					fParser;
	Resource				fResource;
	DataStream*				fStream;
	char*					fTargetFragment;
	Selection				fTargetSelection;
	
	Selection				fCurrentSelection;
	Coordinate				fMapCursorPosition;
	Rectangle				fSelectionPosition;
	
	short					fTopMargin;

	ViewIdleState			fIdleState;
	DrawIdleState			fDrawIdleState;
	ViewIdleState			fLastIdleState;

	unsigned				fInsideImageMap : 1;
	unsigned				fNoHWrapSelection : 1;
	unsigned				fShouldPaintBackground : 1;
	unsigned				fShouldAutoScroll : 1;
	unsigned				fShowActiveSelection : 1;
	unsigned				fShowSelection : 1;
};

//==================================================================================

class SelectionLayer : public Layer {
public:
							SelectionLayer();
	virtual					~SelectionLayer();
	
	void					Initialize(ContentView*);

	virtual void			Draw(const Rectangle* invalid);
	void					DrawActiveNow();
	void					DrawSelection(Selection selection, Boolean turnOn, const Rectangle* invalid);
#ifdef FIDO_INTERCEPT
	virtual void			Draw(class FidoCompatibilityState& fidoCompatibility) const;
	void					DrawSelection(Selection selection, Boolean turnOn, class FidoCompatibilityState& fidoCompatibility) const;
#endif

	void					RestoreBehind(const Rectangle* invalid);
	void					SaveBehind(const Rectangle* bounds, const Rectangle* invalid);
	void					UpdateSavedBits(const Rectangle* drawn);
	void					DeleteBackBuffer();
	
protected:
	
	ContentView*			fView;
	BitMapDevice*			fBackBuffer;
	Boolean					fActive;
};


//==================================================================================

inline Selection 
ContentView::GetCurrentSelection() const
{
	return fCurrentSelection;
}

inline ulong
ContentView::GetCurrentInputTime() const
{
	return fCurrentInputTime;
}

inline long 
ContentView::GetScrollPosition() const
{
	return fScrollPosition;
}

inline SelectionLayer*
ContentView::GetSelectionLayer() const
{
	return (SelectionLayer*)fChild;
}

inline Boolean
ContentView::GetShowSelection() const
{
	return fShowSelection;
}

inline Coordinate 
ContentView::GetMapCursorPosition() const
{
	return fMapCursorPosition;
}

inline short 
ContentView::GetTopMargin() const
{
	return fTopMargin;
}

inline void
ContentView::SetShowSelection(Boolean show)
{
	fShowSelection = show;
}


//==================================================================================

#endif /* __CONTENTVIEW_H__ */
