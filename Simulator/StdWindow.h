// ===========================================================================
//	StdWindow.h
//  portions ©1995 Peter Barrett
//  © 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __STDWINDOW_H__
#define __STDWINDOW_H__

#ifndef __WTVTYPES_H__
#include "WTVTypes.h"
#endif

struct StdWindowPrefs
{
	struct MacPoint windowLocation;
	struct MacPoint windowSize;
	Boolean isVisible;
};

enum
{
	kStdWindowDefaultFontID = monaco,
	kStdWindowDefaultFontSize = 9,
	kStdWindowDefaultFontStyle = normal
};

const short kDefaultStdWindowProcID = zoomDocProc;

class StdWindow {
public:
							StdWindow();
	virtual					~StdWindow();
	
	virtual Boolean			HandleEvent(EventRecord* event);	// returns true if window took event
	virtual void			DoAdjustMenus(ushort modifiers);
	virtual Boolean			DoMenuChoice(long menuChoice, ushort modifiers);

	virtual Boolean			DoNullEvent(EventRecord* event);
	virtual Boolean			DoMouseDownEvent(EventRecord* event);
	virtual Boolean			DoMouseUpEvent(EventRecord* event);
	virtual Boolean			DoKeyDownEvent(EventRecord* event);
	virtual Boolean			DoKeyUpEvent(EventRecord* event);
	virtual Boolean			DoAutoKeyEvent(EventRecord* event);
	virtual Boolean			DoUpdateEvent(EventRecord* event);
	virtual Boolean			DoDiskEvent(EventRecord* event);
	virtual Boolean			DoActivateEvent(EventRecord* event);
	virtual Boolean			DoOSEvent(EventRecord* event);
	virtual Boolean			DoHighLevelEvent(EventRecord* event);

	virtual void			ChangeWindowProcID(short newProcID);
	virtual	long			GetWindowType();
	virtual	void			SetTitle(const char* title);
	virtual void			Click(struct MacPoint *where, ushort modifiers);
	virtual void			DragWindow(struct MacPoint where);
	virtual	void			Draw();
	virtual	void			Idle();
	virtual void			Close();

			void			SetUsePrefs(Boolean flag);
			Boolean			GetUsePrefs(void);
			void			SetPrefsName(const char* name);
			const char*		GetPrefsName(void);
//			Defines the layout of the window

			void			SetActiveScroll(Boolean activeScroll);
			void			SetContentColor(short r, short g, short b);
			void			SetDefaultFontID(short font);
			void			SetDefaultFontSize(short size);
			void			SetDefaultFontStyle(short style);
			void			SetFormat(short headerHeight, short trailerWidth, Boolean vScroll, Boolean hScroll);
			void			SetZoomRect(Rect* r);

			short			GetDefaultFontID(void);
			short			GetDefaultFontSize(void);
			short			GetDefaultFontStyle(void);
			void			GetHeaderRect(struct Rect *r);
			void			GetBodyRect(struct Rect *r);
			void			GetTrailerRect(struct Rect *r);
			void			GetVScrollRect(struct Rect *r);
			void			GetHScrollRect(struct Rect *r);
			Boolean			GetVisible(void);

//			Draws the various bits of the window

	virtual	void			DrawHeader(struct Rect *r);
	virtual	void			DrawTrailer(struct Rect *r);
	virtual	void			BodyCoordinates(struct Rect *r);
	virtual	void			DrawBody(Rect *r, short hScroll, short vScroll, Boolean scrolling);
			
//			Scrolling, zooming, and resizing

			void			ScrollAction(struct ControlRecord** c, short part);
	virtual void			ScrollBody(short h, short v);
			void			ActiveScroll(struct ControlRecord** c, MacPoint *where);

			//void			FitOnScreen(short wantWidth, short wantHeight);
			void			GetMinMaxRect(struct Rect *r);
			void			SetMinMaxRect(struct Rect *r);
			void			GrowWindow(struct MacPoint where);
	virtual void			HideWindow();
	virtual void			SelectWindow();
	virtual void			ShowWindow();
	virtual void			ZoomWindow(short part);
			void			PaintContent(struct Rect *r);
			void			RecalcScrollBars(void);
	virtual void			ResizeWindow(short width, short height);
			void			SetBodySize(long width, long height);

			Boolean			GetIsResizable(void);
			void			SetIsResizable(Boolean);

	static void				DeleteAll();
	static void				IdleAll();
	static void				SavePrefsAll();
	static StdWindow*		GetStdWindow(WindowPtr windowPtr);

			struct GrafPort *w;
			long			mWindowKind;

protected:
	virtual Boolean			SavePrefs(StdWindowPrefs* prefPtr = nil);
	virtual Boolean			RestorePrefs(StdWindowPrefs* prefPtr = nil);
	virtual long			GetPrefsSize(void);
			
protected:
			enum {
				kMinWindowWidth = 80,
				kMinWindowHeight = 64
			};
			
			Boolean			mUsePrefs;
			const char*		mPrefsName;
			short			mHeaderHeight;
			short			mTrailerWidth;
			
			Rect			mMinMaxRect;
			
			long			mWidth;			// Width of body
			long			mHeight;		// Height of body
			
			struct ControlRecord**	fVScroll;
			struct ControlRecord**	fHScroll;
			short			mLastHScroll;	// Last values scrolled to
			short			mLastVScroll;
			
			short			mHLineScroll;	// Size of line or page scroll
			short			mVLineScroll;
			
			short			fDefaultFontID;
			short			fDefaultFontSize;
			short			fDefaultFontStyle;
			
			short			mRed;
			short			mGreen;
			short			mBlue;
			
			Boolean			mActiveScroll;
			Boolean			mResizable;
};

#else
#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
#error "Attempted to #include StdWindow.h multiple times"
#endif
#endif /* __STDWINDOW_H__ */