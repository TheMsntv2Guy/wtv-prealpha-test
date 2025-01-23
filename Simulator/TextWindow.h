// ===========================================================================


#ifndef __TEXTWINDOW_H__
#define __TEXTWINDOW_H__

#ifndef __STDWINDOW__
#include "StdWindow.h"
#endif

class TextWindow : public StdWindow{
public:
							TextWindow();
		virtual				~TextWindow();
					
	virtual	void			DrawBody(Rect *r, short hScroll, short vScroll, Boolean scrolling);
			void			RemoveText(ulong start, ulong length);
			void			AppendText(char* text, ulong length);
			void			ReadFile(const char* filename);
			void			SetText(char* text, ulong length);
			void			SetBodyFont(short face, short size);

protected:
			void			RecalculateBody();
	
protected:
			char**			fTextHandle;
			ulong			fHandleSize;
			ulong			fTextLength;
			short			fFontFace;
			short			fFontSize;
			short			fLineHeight;
			short			fCharWidth;
};

#else
#error "Attempted to #include TextWindow.h multiple times"
#endif // __TEXTWINDOW_H__
