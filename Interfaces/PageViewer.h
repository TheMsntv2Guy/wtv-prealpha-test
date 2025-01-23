// ===========================================================================
//    PageViewer.h
//
//    Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __PAGEVIEWER_H__
#define __PAGEVIEWER_H__

#ifndef __CONTENTVIEW_H__
#include "ContentView.h"
#endif
#ifndef __LIST_H__
#include "List.h"
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"
#endif

class BackURL;
class Displayable;
class StringList;
class Indicator;


//==================================================================================
// '&' formatting functions

extern const char* LastModifiedFunction();
extern const char* PageURLFunction();
extern const char* PageTitleFunction();
extern const char* PageThumbnailURLFunction();
extern const char* LastFindStringFunction();
extern void SetLastFindString(const char*);
extern const char* LastExecuteURLFunction();
extern void SetLastExecuteURL(const char*);

// ============================================================================
// VisitedURL

class VisitedURL  
{
public:
							VisitedURL();
							~VisitedURL();
							
	ulong					GetHash() const;
	ulong					LastVisited() const;
	
	void					SetURL(const Resource*, const char* fragment);
	void					Touch();

protected:		
	ulong					fHash;
	ulong					fLastVisited;
};

// ============================================================================
// VisitedList

class VisitedList
{
public:
							VisitedList();
							~VisitedList();
					
	void					Add(const Resource* base, const char* fragment);
	void					DeleteAll();
	VisitedURL*				Find(const Resource* base, const char* partial) const;

protected:
	DataList				fList;
};

//==================================================================================

class PageViewer : public ContentView {
public:
							PageViewer();
	virtual					~PageViewer();

	const Resource*			GetThumbnail();
	Resource*				NewThumbnail();

	virtual Boolean			BackInput();
	ulong					GetPageCount();
	void					ClearBackList();
	virtual void			Idle();
	void					RestoreFormData();
	long					ScrollBy(long offset);
	virtual Boolean			ScrollDownInput(Input*);
	virtual Boolean			ForwardInput();
	virtual Boolean			ScrollUpInput(Input*);

	virtual void			ShowFragment(const char*);
	virtual void			ShowResource(const Resource*);
	
	Boolean					WasVisited(const Resource* base, const char* partialURL) const;
#ifdef DEBUG
	void					ViewSource(void);
#endif /* DEBUG */
	Indicator*				GetQueuedShow() const							{ return fQueuedShow; };
	void					SetQueuedShow(Indicator *indicator)				{ fQueuedShow = indicator; };

protected:
	void					AddBack(const Resource*);
	void					AddRecent(const Resource*, const char* title);
	BackURL*				NewBackURL(const Resource*);
	
	long					AlignScrollPoint(Displayable* lineStart, long newScroll, long oldScroll);
	
	virtual void			ChangeIdleState(ViewIdleState newState, ViewIdleState oldState);
	virtual void			FirstScreenDrawingCompleted();
	virtual ViewIdleState	HandleNonShowableResource(DataType);
	virtual	void			InitialScreenLayoutCompleted();
	virtual void			ScreenLayoutCompleted();
	
	virtual Boolean			ScrollDown(long suggestedIncrement, long minIncrement);
	virtual Boolean			ScrollUp(long suggestedIncrement, long minIncrement);
	
protected:
	ObjectList				fBackPages;
	VisitedList				fVisitedPages;
	BackURL*				fPendingBackURL;
	BackURL*				fCancelBackURL;
	Indicator*				fQueuedShow;
};

//==================================================================================

class BackURL : public Listable {
public:
							BackURL();
	virtual					~BackURL();

	const StringList*		GetFormData() const;
	Coordinate				GetMapCursorPosition() const;
	const Resource*			GetResource() const;
	long					GetScrollPosition() const;
	Selection				GetSelection() const;
	Rectangle				GetSelectionPosition() const;
	Boolean					IsInImageMap() const;

	void					SetFormData(StringList*);
	void					SetInImageMap(Boolean);
	void					SetMapCursorPosition(Coordinate);
	void					SetResource(const Resource*);
	void					SetScrollPosition(long);
	void					SetSelection(Selection);
	void					SetSelectionPosition(Rectangle);

protected:
	StringList*				fFormData;
	Resource				fResource;
	Coordinate				fMapCursorPosition;
	Rectangle				fSelectionPosition;
	Selection				fSelection;
	long 					fScrollPosition;
	Boolean					fInImageMap;
};

//==================================================================================
// NOTE: perhaps these should get rolled into PageViewer.

extern short gPageViewerScrollDirection;
extern TransitionEffect gPageViewerTransitionType;
extern ulong gPageViewerTransitionDelay;
//==================================================================================

// Getters.
inline const StringList* BackURL::GetFormData() const			{ return fFormData;}
inline Coordinate BackURL::GetMapCursorPosition() const			{ return fMapCursorPosition;}
inline const Resource* BackURL::GetResource() const				{ return &fResource;}
inline long BackURL::GetScrollPosition() const					{ return fScrollPosition;}
inline Selection BackURL::GetSelection() const					{ return fSelection;}
inline Rectangle BackURL::GetSelectionPosition() const			{ return fSelectionPosition;}
inline Boolean BackURL::IsInImageMap() const					{ return fInImageMap;}

// Setters.
inline void BackURL::SetFormData(StringList* formData)			{ fFormData = formData;}
inline void BackURL::SetInImageMap(Boolean inMap)				{ fInImageMap = inMap;}
inline void BackURL::SetMapCursorPosition(Coordinate position)	{ fMapCursorPosition = position;}
inline void BackURL::SetResource(const Resource* resource)		{ fResource = *resource;}
inline void BackURL::SetScrollPosition(long position)			{ fScrollPosition = position;}
inline void BackURL::SetSelection(Selection selection)			{ fSelection = selection;}
inline void BackURL::SetSelectionPosition(Rectangle position)	{ fSelectionPosition = position;}

//==================================================================================

// Getters.
inline ulong VisitedURL::GetHash() const						{ return fHash;}
inline ulong VisitedURL::LastVisited() const					{ return fLastVisited;}

//==================================================================================

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include PageViewer.h multiple times"
	#endif
#endif /* __PAGEVIEWER_H__ */
