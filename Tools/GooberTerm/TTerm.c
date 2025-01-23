/*********************************************************************

	Buggy MiniEdit.c
	
	The sample application from Inside Macintosh (RoadMap p.15-17)
	beefed up a bit by Stephen Z. Stein, Symantec Corp.
	Use this file with the “MiniEdit” chapter of your manual.
	
	The resources used in this program are in the file MiniEdit.π.rsrc.
	
	In order for THINK C to find the resource file for this
	project, be sure you’ve named the project MiniEdit.π
		
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "mini.file.h"
#include "mini.windows.h"
#include "MiniEdit.h"
#include "parallelcrap.h"

WindowRecord	wRecord;
WindowPtr		myWindow;		
TEHandle		TEH;
int				linesInFolder;
Rect			dragRect = { 0, 0, 1024, 1024 };
MenuHandle		myMenus[4];
ControlHandle 	vScroll;
Cursor			editCursor;
Cursor			waitCursor;
char			dirty;

extern Str255 	theFileName;

#define	ours(w)		((myWindow != NULL) && (w == myWindow))
#define ECHO false

extern OSErr serialinit(void);
extern void serialwrite(char);
extern long serialcharsavail(void);
extern void getserialchars(long );
extern void displaybuff(char *,long );
extern void cleanup(void);
extern char interpret (char);
extern char *inbuf;

void oops(Str255 s,OSErr err);
void OutlineButton(DialogPtr aDialog,Rect *r);
void CheckSerialPort(void);
void SquirtOther(void);

void oops(Str255 s,OSErr err)
{
	Str255 str;
	
	NumToString(err,str);
	ParamText(s,str,"\p","\p");
	Alert(ErrorAlert,0L);
}

void main() 
{
	int		myRsrc;
	OSErr	err;
	
	InitGraf(&qd.thePort);
	InitFonts();
	FlushEvents(everyEvent, 0);
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(0L);
	InitCursor();
	MaxApplZone();

/*
 *  The following statement is included as a check to see if we can
 *	access our program's resources.  When the project is run from
 *	THINK C, the resource file <project name>.rsrc is automatically
 *  opened.  When an application is built, these resources are 
 * 	automatically merged with the application.
 *
 */
	
	if (GetResource('MENU', fileID)==0) {
		SysBeep(20);
		CantOpen();
		return;
	}
	
	SetUpFiles();
	SetUpCursors();
	SetUpMenus();
	SetUpWindows();
	
	FindCard();
	
	err = serialinit();
	if(err) {
		oops("\pError initializing serial port",err);
	} else {
		while (MainEvent()) ;
		cleanup();
	}
}

int DoMouseDown (int windowPart, WindowPtr whichWindow, EventRecord *myEvent)
{
	switch (windowPart) {
		case inGoAway:
			if (ours(whichWindow))
				if (TrackGoAway(myWindow, myEvent->where))
					DoFile(fmClose);
			break;

		case inMenuBar:
			return(DoCommand(MenuSelect(myEvent->where)));

		case inSysWindow:
			SystemClick(myEvent, whichWindow);
			break;

		case inDrag:
			if (ours(whichWindow))
				DragWindow(whichWindow, myEvent->where, &dragRect);
			break;

		case inGrow:
			if (ours(whichWindow))
				MyGrowWindow(whichWindow, myEvent->where);
			break;

		case inContent:
			if (whichWindow != FrontWindow())
				SelectWindow(whichWindow);
			else if (ours(whichWindow))
				DoContent(whichWindow, myEvent);
			break;
	}
}

void CheckSerialPort(void)
{
	long			count=0;

	if (count = serialcharsavail())
	{
		getserialchars(count);
		displaybuff(inbuf,count);
		dirty = 1;
	}
}
#define kCommandCode		55
#define kPeriodCode 		47

static Boolean KeyIsDown(KeyMap keys, short keyCode)
{
	return ((((char *)keys)[keyCode >> 3] & (char)(1 << (keyCode & 7))) != (char)0);
}

Boolean ShouldAbort(void)
{
	KeyMap keys;

	GetKeys(keys);
	return KeyIsDown(keys, kCommandCode) && KeyIsDown(keys, kPeriodCode);
}

int MainEvent(void) 
{
	EventRecord		myEvent;
	WindowPtr		whichWindow;
	short			windowPart;
	Rect			r;
	char			theChar;
	
	MaintainCursor();
	MaintainMenus();
	SystemTask();
	TEIdle(TEH);
	CheckSerialPort();
	if (GetNextEvent(everyEvent, &myEvent)) {
		switch (myEvent.what) {
		case mouseDown:
			windowPart = FindWindow(myEvent.where, &whichWindow);
			DoMouseDown(windowPart, whichWindow, &myEvent);
			break;

		case keyDown:
		case autoKey: 
			{
				
				theChar = myEvent.message & charCodeMask;
				if ((myEvent.modifiers & cmdKey) != 0) 
					return(DoCommand(MenuKey( theChar)));
				else 
				{
					if(theChar == 0x08)
					{
						TEKey(theChar, TEH);
						ShowSelect();
						dirty = 1;
					}
				}
			}
			break;

		case activateEvt:
			if (ours((WindowPtr)myEvent.message)) {
				if (myEvent.modifiers & activeFlag) {
					TEActivate(TEH);
					ShowControl(vScroll);
					DisableItem(myMenus[editM], undoCommand);
					TEFromScrap();
				}
				else {
					TEDeactivate(TEH);
					HideControl(vScroll);
					ZeroScrap();
					TEToScrap();
				}
			}
			break;

		case updateEvt: 
			if (ours((WindowPtr) myEvent.message))
					UpdateWindow(myWindow);
			break;
		case nullEvent:
			CheckSerialPort();
			break;
		} /* end of case myEvent.what */
	} /* if */
	return(1);
}

void OutlineButton(DialogPtr aDialog,Rect *r)
{
	PenState	oldPen;
	GrafPtr		oldPort;
	
	GetPort(&oldPort);
	GetPenState(&oldPen);
	SetPort(aDialog);
	PenSize(3,3);
	InsetRect(r,-4,-4);
	FrameRoundRect(r,16,16);
	SetPort(oldPort);
	SetPenState(&oldPen);
}

void SetUpMenus(void)
{
	int		i;
	
	myMenus[appleM] = NewMenu(appleID, "\p\024");
	AddResMenu(myMenus[appleM], 'DRVR');
	myMenus[fileM] = GetMenu(fileID);
	myMenus[editM] = GetMenu(editID);
	myMenus[downloadM] = GetMenu(downloadID);
	for ((i=appleM); (i<=downloadM); i++)
		InsertMenu(myMenus[i], 0) ;
	DrawMenuBar();
}

int DoCommand(long mResult)

{
	int		theItem;
	Str255	name;
	
	theItem = LoWord(mResult);
	switch (HiWord(mResult)) {
		case appleID:
			GetItem(myMenus[appleM], theItem, name);
			OpenDeskAcc(name);
			SetPort(myWindow);
			break;

		case fileID: 
			DoFile(theItem);
			break;

		case editID: 
			if (SystemEdit(theItem-1) == 0) {
				switch (theItem) {
					case cutCommand:
						ZeroScrap();
						TECut(TEH);
						TEToScrap();
						dirty = 1;
						break;
	
					case copyCommand:
						ZeroScrap();
						TECopy(TEH);
						TEToScrap();
						break;
		
					case pasteCommand:
						TEPaste(TEH);
						dirty = 1;
						break;
		
					case clearCommand:
						TEDelete(TEH);
						dirty = 1;
						break;
						
					case selAllCommand:
						TESetSelect(0,32767,TEH);
						dirty = 0;
						break;
				}
				ShowSelect();
			}
			break;
	    case downloadID:
	        switch(theItem)
	        {
	          case 1: SquirtIt("squirt approm bfe00000"); break;
	          case 2: SquirtIt("squirt bootrom bfc00000"); break;
	          case 3: SquirtIt("squirt unirom bfc00000"); break;
	          case 4: SquirtIt("squirt goobrom bfc00000"); break;
	          case 5: SquirtOther(); break;
	        }
	        break;
	}
	HiliteMenu(0);
	return(1);
}

void SquirtOther(void)
{
	DialogPtr d;
	short item;
	Handle h;
	short type;
	short hit;
	Rect r;
	char s[255];
	Str255 file;
	Str255 address;
	
	d = GetNewDialog(129,nil,(WindowPtr)-1);
	GetDItem(d,5,&type,&h,&r);
	SelIText(d,5,0,100);
	do {
		ModalDialog(nil,&hit);
	} while(hit > 2);
	if(hit == 1)
	{
		GetDItem(d,5,&type,&h,&r);
		GetIText(h,file);
		GetDItem(d,6,&type,&h,&r);
		GetIText(h,address);
		sprintf(s,"squirt %s %s",P2CStr(file),P2CStr(address));
	}
	
	DisposeDialog(d);
	
	if(hit == 1)
		SquirtIt(s);
}

void MaintainCursor(void)
{
	Point		pt;
	WindowPeek	wPtr;
	GrafPtr		savePort;
	
	if (ours((WindowPtr)(wPtr=(WindowPeek)FrontWindow()))) {
		GetPort(&savePort);
		SetPort((GrafPtr)wPtr);
		GetMouse(&pt);
		if (PtInRect(pt, &(**TEH).viewRect ) )
			SetCursor( &editCursor);
		else SetCursor(&qd.arrow);
		SetPort(savePort);
	}
}

void MaintainMenus(void)
{
	if ( !(*(WindowPeek)myWindow).visible || 
			!ours(FrontWindow()) ) {
		EnableItem(myMenus[fileM], fmNew);
		EnableItem(myMenus[fileM], fmOpen);
		DisableItem(myMenus[fileM], fmClose);
		DisableItem(myMenus[fileM], fmSave);
		DisableItem(myMenus[fileM], fmSaveAs);
		DisableItem(myMenus[fileM], fmRevert);
		DisableItem(myMenus[fileM], fmPrint);
		EnableItem(myMenus[editM], undoCommand);
		EnableItem(myMenus[editM], cutCommand);
		EnableItem(myMenus[editM], copyCommand);
		EnableItem(myMenus[editM], clearCommand);
	}
	else {
		DisableItem(myMenus[fileM], fmNew);
		DisableItem(myMenus[fileM], fmOpen);
		EnableItem(myMenus[fileM], fmClose);
		EnableItem(myMenus[fileM], fmSaveAs);
		EnableItem(myMenus[fileM], fmPrint);
		if (dirty && theFileName[0] != 0) {
			EnableItem(myMenus[fileM], fmRevert);
			EnableItem(myMenus[fileM], fmSave);
		}
		else {
			DisableItem(myMenus[fileM], fmRevert);
			DisableItem(myMenus[fileM], fmSave);
		}
		DisableItem(myMenus[editM], undoCommand);
		if ((**TEH).selStart==(**TEH).selEnd) {
			DisableItem(myMenus[editM], cutCommand);
			DisableItem(myMenus[editM], copyCommand);
			DisableItem(myMenus[editM], clearCommand);
		}
		else {
			EnableItem(myMenus[editM], cutCommand);
			EnableItem(myMenus[editM], copyCommand);
			EnableItem(myMenus[editM], clearCommand);
		}
	}
}

void SetUpCursors(void)
{
	CursHandle	hCurs;
	
	hCurs = GetCursor(1);
	editCursor = **hCurs;
	hCurs = GetCursor(watchCursor);
	waitCursor = **hCurs;
}


void CantOpen(void)
{
	Rect r;

	SetRect(&r, 152, 60, 356, 132);
	SetPort((myWindow = NewWindow( (Ptr) 0L, &r, "\p", true, dBoxProc, (WindowPtr) -1L, false, 0L)));
	TextFont(0);
	MoveTo(4, 20);
	DrawString("\pCan't open resource file.");
	MoveTo(4, 40);
	DrawString("\pClick mouse to exit.");
	do {
	} while (!Button());
}
