// ===========================================================================
//	TEWindow.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __TEWINDOW_H__
#define __TEWINDOW_H__

#ifndef __STDWINDOW_H__
#include "StdWindow.h"
#endif



class TEWindow : public StdWindow
{
public:
							TEWindow();
	virtual					~TEWindow();
	
	virtual void			DoAdjustMenus(ushort modifiers);
	virtual Boolean			DoMenuChoice(long menuChoice, ushort modifiers);
	virtual Boolean			DoKeyDownEvent(EventRecord* event);
	virtual Boolean			DoActivateEvent(EventRecord* event);

			Boolean			DoTEKey(char ch, ushort modifiers);
	virtual Boolean			DoTEEnter(ushort modifiers);
	virtual Boolean			DoTEDelete(ushort modifiers);
	virtual Boolean			DoTETab(ushort modifiers);
	virtual Boolean			DoTEReturn(ushort modifiers);
	virtual void			Click(struct MacPoint *where, ushort modifiers);
	virtual	void			Idle(void);
	virtual	void			ShowWindow(void);
			
	virtual	Boolean			ReadFromFile(void);
	virtual	Boolean			ReadFromFile(const char* filename);
	virtual	Boolean			ReadFromFile(FSSpec* inFile);

	virtual	Boolean			SaveToFile(void);
	virtual	Boolean			SaveToFile(const char* filename);
	virtual	Boolean			SaveToFile(FSSpec* inFile);

	virtual void			DrawBody(Rect *r, short hScroll, short vScroll, Boolean scrolling);		
	virtual void			ScrollBody(short h, short v);
			void			RemoveText(ulong start, ulong length);
			void			AppendText(char* text, ulong length);
			int				printf(const char* format, ...);
			int				vprintf(const char* format, va_list list);

			void			GetBodyText(char** text, ulong* length);
			void			SetBodyText(char* text, ulong length);
			void			SetBodyFont(short font, short size, short style);
			void			AdjustBodyScroll();
	virtual void			ResizeWindow(short width, short height);

			void			SelectAll(void);
			void			GetBodySelection(char** textString, long* length);
			void			GetBodySelectionRange(long* selStart, long* selEnd);
			//void			SetBodySelection(char* textString, long length);
			void			SetBodySelectionRange(long selStart, long selEnd);
			
			void			FindForwardInBody(const char* text, ulong length);
			void			FindBackwardInBody(const char* text, ulong length);
			void			FindSelectionForwardInBody(void);
			void			FindSelectionBackwardInBody(void);

	virtual TEHandle		GetActiveTEHandle(void);
			TEHandle		GetBodyTEHandle(void);
			Boolean			GetBodyTEWriteProtect(void);
			
	virtual void			SetActiveTEHandle(TEHandle teHandle);
			void			SetBodyTEWriteProtect(Boolean writeProtect);
			void			SetBodyTEHandle(TEHandle teHandle);

protected:
	enum	{
				TE_ENTER_KEY = 3,
				TE_DELETE_CHAR = 8,
				TE_TAB_CHAR = 9,
				TE_CARRIAGE_RETURN = 13,
				kTEMaxSize = 0x7fff,
				kTECreator = 'MPS ',	// MPW
				//kTECreator = 'CWIE',	// CodeWarrior
				kTEFileType = 'TEXT'
			};
			
protected:

			TEHandle		fActiveTEHandle;
			Boolean			fIsActivated;	// has active been activated?
			TEHandle		fBodyTEHandle;
			Boolean			fBodyTEWriteProtect;
};

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include TEWindow.h multiple times"
	#endif
#endif /* __TEWINDOW_H__ */
