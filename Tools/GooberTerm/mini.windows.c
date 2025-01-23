/*********************************************************************

	mini.windows.c
	
	window functions for MiniEdit
	
*********************************************************************/

#include "MiniEdit.h"
#include "mini.windows.h"
#include "mini.file.h"

extern WindowRecord	 wRecord;
extern WindowPtr	 myWindow;
extern ControlHandle vScroll;
extern TEHandle 	 TEH;
extern char			 dirty;
extern Str255 		 theFileName;
extern int			 linesInFolder;

	/**
	 **		Prototypes for private functions.
	 **		(They really should be static.)
	 **
	 **/
 
void AdjustText (void);
void SetVScroll(void);
void SetView (WindowPtr w);
pascal void ScrollProc (ControlHandle theControl, short theCode);



void SetUpWindows(void)
{
	Rect	destRect, viewRect;
	Rect	vScrollRect;
	FontInfo	myInfo;
	int		height;
	
	SetPort((myWindow = GetNewWindow(windowID, &wRecord, (WindowPtr) -1L)));
	TextFont(monaco);
	TextSize(9);
	vScrollRect = (*myWindow).portRect;
	vScrollRect.left = vScrollRect.right-15;
	vScrollRect.right += 1;
	vScrollRect.bottom -= 14;
	vScrollRect.top -= 1;
	vScroll = NewControl( myWindow, &vScrollRect, "\p", 1, 0, 0, 0,
		scrollBarProc, 0L);

	viewRect = qd.thePort->portRect;
	viewRect.right -= SBarWidth;
	viewRect.bottom -= SBarWidth;
	InsetRect(&viewRect, 4, 4);
	TEH = TENew(&viewRect, &viewRect);
	SetView(qd.thePort);
	dirty = 0;
}


void AdjustText (void)

{
	int		oldScroll, newScroll, delta;
	
	oldScroll = (**TEH).viewRect.top - (**TEH).destRect.top;
	newScroll = GetCtlValue(vScroll) * (**TEH).lineHeight;
	delta = oldScroll - newScroll;
	if (delta != 0)
	  TEScroll(0, delta, TEH);
}


void SetVScroll(void)
{
	register int	n;
	
	n = (**TEH).nLines-linesInFolder;

	if ((**TEH).teLength > 0 && (*((**TEH).hText))[(**TEH).teLength-1]=='\r')
		n++;

	SetCtlMax(vScroll, n > 0 ? n : 0);
}

void ShowSelect (void)

{
	register	int		topLine, bottomLine, theLine;
	
	SetVScroll();
	AdjustText();
	
	topLine = GetCtlValue(vScroll);
	bottomLine = topLine + linesInFolder;
	
	if ((**TEH).selStart < (**TEH).lineStarts[topLine] ||
			(**TEH).selStart >= (**TEH).lineStarts[bottomLine]) {
		for (theLine = 0; (**TEH).selStart >= (**TEH).lineStarts[theLine]; theLine++)
			;
		SetCtlValue(vScroll, theLine - linesInFolder / 2);
		AdjustText();
	}
}

void SetView (WindowPtr w)

{
	(**TEH).viewRect = w->portRect;
	(**TEH).viewRect.right -= SBarWidth;
	(**TEH).viewRect.bottom -= SBarWidth;
	InsetRect(&(**TEH).viewRect, 4, 4);

	linesInFolder = ((**TEH).viewRect.bottom-(**TEH).viewRect.top)/(**TEH).lineHeight;
	(**TEH).viewRect.bottom = (**TEH).viewRect.top + (**TEH).lineHeight*linesInFolder;
	(**TEH).destRect.right = (**TEH).viewRect.right;
	TECalText(TEH);
}

void UpdateWindow (WindowPtr theWindow)

{
	GrafPtr	savePort;
	
	GetPort(&savePort);
	SetPort(theWindow);

	BeginUpdate(theWindow);
	EraseRect(&theWindow->portRect);
	DrawControls(theWindow);
	DrawGrowIcon(theWindow);
	TEUpdate(&theWindow->portRect, TEH);
	EndUpdate(theWindow);

	SetPort(savePort);
}




pascal void ScrollProc (ControlHandle theControl, short theCode)

{
	int	pageSize;
	int	scrollAmt;
	int oldCtl;
	
	if (theCode == 0)
		return ;
	
	pageSize = ((**TEH).viewRect.bottom-(**TEH).viewRect.top) / 
			(**TEH).lineHeight - 1;
			
	switch (theCode) {
		case inUpButton: 
			scrollAmt = -1;
			break;
		case inDownButton: 
			scrollAmt = 1;
			break;
		case inPageUp: 
			scrollAmt = -pageSize;
			break;
		case inPageDown: 
			scrollAmt = pageSize;
			break;
	}

	oldCtl = GetCtlValue(theControl);
	SetCtlValue(theControl, oldCtl+scrollAmt);

	AdjustText();

}

void DoContent(WindowPtr theWindow, EventRecord *theEvent)

{
	int				cntlCode;
	ControlHandle 	theControl;
	int				pageSize;
	GrafPtr			savePort;
	
	GetPort(&savePort);
	SetPort(theWindow);

	GlobalToLocal(&theEvent->where);
	if ((cntlCode = FindControl(theEvent->where, theWindow, &theControl)) == 0) {
		if (PtInRect(theEvent->where, &(**TEH).viewRect))
			TEClick(theEvent->where, (theEvent->modifiers & shiftKey)!=0, TEH);
	} else if (cntlCode == inThumb) {
		TrackControl(theControl, theEvent->where, 0L);
		AdjustText();
	} else
		TrackControl(theControl, theEvent->where, NewControlActionProc(ScrollProc));
	SetPort(savePort);
}

void MyGrowWindow(WindowPtr w, Point p)

{
	GrafPtr	savePort;
	long	theResult;
	Rect	oldHorizBar;
	Rect 	r;
	
	GetPort(&savePort);
	SetPort(w);
	
	oldHorizBar = w->portRect;
	oldHorizBar.top = oldHorizBar.bottom - (SBarWidth+1);

	SetRect(&r, 80, 80, qd.screenBits.bounds.right, qd.screenBits.bounds.bottom);
	theResult = GrowWindow(w, p, &r);
	if (theResult == 0)
	  return;
	SizeWindow( w, LoWord(theResult), HiWord(theResult), false);

	InvalRect(&w->portRect);
	
	SetView(w);

	EraseRect(&oldHorizBar);
	
	MoveControl(vScroll, w->portRect.right - SBarWidth, w->portRect.top-1);
	SizeControl(vScroll, SBarWidth+1, w->portRect.bottom - w->portRect.top-(SBarWidth-2));
	r = (**vScroll).contrlRect;
	ValidRect(&r);


	SetVScroll();
	AdjustText();
	
	SetPort(savePort);
}



void CloseMyWindow(void)
{
	HideWindow(myWindow);
	TESetSelect(0, (**TEH).teLength, TEH);
	TEDelete(TEH);
	SetVScroll();
	SetUpFiles();
}
