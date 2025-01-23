// ===========================================================================
// MessageWindow.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __MESSAGEWINDOW_H__
#define __MESSAGEWINDOW_H__

#ifndef __TEWINDOW_H__
#include "TEWindow.h"
#endif




// ===========================================================================

typedef const char* DebugMessageSender;

typedef enum 
{
	kPreviousImportance,
	kTrivialImportance,
	kNormalImportance,
	kHighImportance
} DebugMessageImportance;

class DebugMessageWindow : public TEWindow
{
	public:
							DebugMessageWindow(void);
		virtual				~DebugMessageWindow(void);

	virtual void			DoAdjustMenus(ushort modifiers);
	virtual Boolean			DoMenuChoice(long menuChoice, ushort modifiers);

	virtual void			Close(void);
	virtual void			HideWindow(void);
	virtual void			ShowWindow(void);

			void			AddMessage( DebugMessageSender sender,
										DebugMessageImportance priority,
									    char* message );
	protected:
			enum {kNumQueuedMessagesThreshhold = 5};
			int				fNumQueuedMessages;
};

extern DebugMessageWindow*	gDebugMessageWindow;

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include MessageWindow.h multiple times"
	#endif
#endif /* __MESSAGEWINDOW_H__ */
