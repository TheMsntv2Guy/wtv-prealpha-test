// ===========================================================================
//	TEWindow.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __FINDDIALOGBOX_H__
#include "FindDialogBox.h"
#endif
#ifndef __MACINTOSHMENUS_H__
#include "MacintoshMenus.h"
#endif
#ifndef __TEWINDOW_H__
#include "TEWindow.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif




// ===========================================================================

TEWindow::TEWindow(void)
{
	Rect r;
	GetBodyRect(&r);
	InsetRect(&r, 2, 2);

	GrafPtr savePort;
	GetPort(&savePort);
	SetPort(w);
	TEHandle teHandle = TENew(&r, &r);
	TEAutoView(true, teHandle);
	SetPort(savePort);

	SetFormat(0, 0, true, true);	// Header, trailer, vscroll, hscroll
	
	fBodyTEHandle = teHandle;
	fBodyTEWriteProtect = false;

	fActiveTEHandle = teHandle;
	fIsActivated = false;	// haven't called TEActivate on fActiveTEHandle, yet
	
	SetDefaultFontID(geneva);
	SetDefaultFontSize(9);
	SetDefaultFontStyle(normal);
	
	SetBodyFont(GetDefaultFontID(), GetDefaultFontSize(), GetDefaultFontStyle());
}

TEWindow::~TEWindow(void)
{
	if (fBodyTEHandle != nil)
		TEDispose(fBodyTEHandle);
	fBodyTEHandle = nil;
}

Boolean
TEWindow::DoKeyDownEvent(EventRecord* event)
{
	Boolean handled = StdWindow::DoKeyDownEvent(event);	// StdWindow will deal with menus...

	if (!handled)
	{	
		if ((FrontWindow() == w) && ((event->modifiers & cmdKey) == 0))	// ... if it wasn't a menu item ...
			handled = DoTEKey(event->message & charCodeMask, event->modifiers);
	}
	
	return handled;
}

Boolean
TEWindow::DoActivateEvent(EventRecord* event)
{
	if ((event->modifiers & activeFlag) != 0)
	{	TEHandle teHandle = GetActiveTEHandle();
		if (teHandle != nil)
		{
			TEActivate(teHandle);
			fIsActivated = true;
		}
	}
	else
	{
		TEHandle teHandle = GetActiveTEHandle();
		if (teHandle != nil)
		{
			TEDeactivate(teHandle);
			fIsActivated = false;
		}
	}
	
	(void)StdWindow::DoActivateEvent(event);
	return true;
}

void
TEWindow::DoAdjustMenus(ushort modifiers)
{
	StdWindow::DoAdjustMenus(modifiers);

	if (FrontWindow() == w)
	{	
		MenuHandle menu;

		menu = GetMenu(mFile);
		EnableItem(menu, iSave);
		EnableItem(menu, iSaveAs);
		
		menu = GetMenu(mEdit);
		EnableItem(menu, iCut);
		EnableItem(menu, iCopy);
		EnableItem(menu, iClear);
		EnableItem(menu, iPaste);
		EnableItem(menu, iSelectAll);
		EnableItem(menu, iFind);
		EnableItem(menu, iFindAgain);
		EnableItem(menu, iFindSelection);
	}
}

Boolean
TEWindow::DoMenuChoice(long menuChoice, ushort modifiers)
{
	Boolean result = false;

	if (FrontWindow() == w)
	{
		short theMenu = HiWord(menuChoice);
		short theItem = LoWord(menuChoice);
	
		if (theMenu == mFile)
		{
			switch(theItem)
			{
				case iSave:
					SaveToFile();
					result = true;
					break;
				case iSaveAs:
					SaveToFile();
					result = true;
					break;
			}
		}
		else if (theMenu == mEdit)
		{
			TEHandle teHandle = GetActiveTEHandle();
			TEHandle bodyHandle = GetBodyTEHandle();
			Boolean bodyWriteProtect = GetBodyTEWriteProtect();
			
			switch(theItem)
			{
				case iCut:
					if ((teHandle != nil) && (ZeroScrap() == noErr))
					{	if (!((teHandle == bodyHandle) && bodyWriteProtect))
						{	TECut(teHandle);
							if (TEToScrap() != noErr)
							{	ZeroScrap();
							}
							AdjustBodyScroll();
						}
					}
					result = true;
					break;
				case iCopy:
					if ((teHandle != nil) && (ZeroScrap() == noErr))
					{	TECopy(teHandle);
						if (TEToScrap() != noErr)
						{	ZeroScrap();
						}
					}
					result = true;
					break;		
				case iPaste:
					if (teHandle != nil)
					{	if (!((teHandle == bodyHandle) && bodyWriteProtect))
						{	if (TEFromScrap() == noErr)
							{	TEPaste(teHandle);
								AdjustBodyScroll();
							}
						}
					}
					result = true;
					break;		
				case iClear:
					if (teHandle != nil)
					{	TEDelete(teHandle);
						AdjustBodyScroll();
					}
					result = true;
					break;		
				case iSelectAll:
					SelectAll();
					result = true;
					break;
				case iFind:
					if (!DoFindDialog())
						break;
					modifiers &= (~shiftKey);
				case iFindAgain:
					const char* searchString = GetFindDialogSearchString();
					ulong searchStringLength = strlen(searchString);
					
					if (modifiers & shiftKey)
						FindBackwardInBody(searchString, searchStringLength);
					else
						FindForwardInBody(searchString, searchStringLength);
					result = true;
					break;
					
				case iFindSelection:
					if (modifiers & shiftKey)
						FindSelectionBackwardInBody();
					else
						FindSelectionForwardInBody();
					result = true;
					break;
			}
		}
	}
	return (result || StdWindow::DoMenuChoice(menuChoice, modifiers));
}

Boolean
TEWindow::DoTEKey(char ch, ushort modifiers)
{
	Boolean result = false;
	
	if (ch == TE_ENTER_KEY)
	{
		result = DoTEEnter(modifiers);
	}
	else if (ch == TE_DELETE_CHAR)
	{
		result = DoTEDelete(modifiers);
	}
	else if (ch == TE_TAB_CHAR)
	{
		result = DoTETab(modifiers);
	}
	else if (ch == TE_CARRIAGE_RETURN)
	{
		result = DoTEReturn(modifiers);
	}
	
	if (!result)
	{
		TEHandle teHandle = GetActiveTEHandle();
		if (teHandle != nil)
		{
			if (!((teHandle == GetBodyTEHandle()) && GetBodyTEWriteProtect()))
			{
				TEKey(ch, teHandle);
				if (teHandle == GetBodyTEHandle())
					AdjustBodyScroll();

				result = true;
			}
		}
	}
	return result;
}

Boolean
TEWindow::DoTEEnter(ushort UNUSED(modifiers))	// maybe want cmd-enter to be special?
{
	return false;
}
Boolean
TEWindow::DoTEDelete(ushort UNUSED(modifiers))	// maybe want delete to be special?
{
	return false;
}
Boolean
TEWindow::DoTETab(ushort UNUSED(modifiers))	// want shift-tab to go backwards?
{
	return false;
}
Boolean
TEWindow::DoTEReturn(ushort UNUSED(modifiers)) // maybe want cmd-return to be special?
{
	return false;
}

void
TEWindow::Click(struct MacPoint *where, ushort modifiers)
{
	TEHandle teHandle = GetActiveTEHandle();
	TEHandle bodyHandle = GetBodyTEHandle();
	
	if ((teHandle != nil) && (PtInRect(*where, &((**teHandle).viewRect))))
	{
		TEClick(*where, (modifiers & shiftKey) != 0, teHandle);
		if (teHandle == bodyHandle)
			AdjustBodyScroll();
	}
	else if ((bodyHandle != nil) && (PtInRect(*where, &((**bodyHandle).viewRect))))
		SetActiveTEHandle(bodyHandle);
	else
		StdWindow::Click(where, modifiers);
}

void
TEWindow::Idle(void)
{
	if (w == FrontWindow())
	{
		TEHandle teHandle = GetActiveTEHandle();
		if (teHandle != nil)
			TEIdle(teHandle);
	}
}

void
TEWindow::ShowWindow()
{
	if (!GetVisible())
	{
		GrafPtr savePort;
		GetPort(&savePort);
		SetPort(w);
		ResizeWindow(w->portRect.right - w->portRect.left, w->portRect.bottom - w->portRect.top);
		AdjustBodyScroll();
		StdWindow::ShowWindow();
		SetPort(savePort);
	}
}

void
TEWindow::DrawBody(Rect* r, short UNUSED(hScroll), short UNUSED(vScroll), Boolean UNUSED(scrolling))
{
	TEHandle teHandle = GetBodyTEHandle();
	if (teHandle != nil)
	{
		EraseRect(r);
		TEUpdate(r, teHandle);
	}
}

void
TEWindow::ResizeWindow(short width, short height)
{
	StdWindow::ResizeWindow(width, height);
	AdjustBodyScroll();
}

void
TEWindow::ScrollBody(short h, short v)
{
	TEHandle teHandle = GetBodyTEHandle();
	if (teHandle != nil)
	{
		TEScroll(h, v, teHandle);
	}
}
void
TEWindow::RemoveText(ulong start, ulong length)
{
	TEHandle teHandle = GetBodyTEHandle();
	if (teHandle != nil)
	{
		//long selStart = (**teHandle).selStart;
		//long selEnd = (**teHandle).selEnd;
		
		if (start < 0)
			start = 0;
		else if (start >= kTEMaxSize)
			return;
		
		if (start + length > kTEMaxSize)
			length = kTEMaxSize - start;
		
		TESetSelect(start, start+length, teHandle);
		TEDelete(teHandle);
		
		// try to re-establish old selection
		//if (selStart > start)
		//{
		//	if (selStart <= start+length)
		//		selStart = start;
		//	else
		//		selStart -= length;
		//}
		//
		//if (selEnd > start)
		//{
		//	if (selEnd <= start+length)
		//		selEnd = start;
		//	else
		//		selEnd -= length;
		//}
		//TESetSelect(selStart, selEnd, teHandle);
	}
}

#define kMinRemoveChunk	2048

void
TEWindow::AppendText(char* text, ulong length)
{
	TEHandle teHandle = GetBodyTEHandle();
	if (teHandle != nil)
	{
		long spaceLeft = kTEMaxSize - (**teHandle).teLength;
		
		if (length > kTEMaxSize)
		{	length = kTEMaxSize;
		}
		
		if (spaceLeft < length)
		{	
			long spaceWanted = length - spaceLeft;
			if (spaceWanted < kMinRemoveChunk)
				spaceWanted = kMinRemoveChunk;

			short saveViewRectBottom = (**teHandle).viewRect.bottom;
			(**teHandle).viewRect.bottom = (**teHandle).viewRect.top;
			RemoveText(0, spaceWanted);
			(**teHandle).viewRect.bottom = saveViewRectBottom;
		}
		
		TESetSelect((**teHandle).teLength, (**teHandle).teLength, teHandle);
		TEInsert(text, length, teHandle);
		
		TESelView(teHandle);
		AdjustBodyScroll();
	}
}

static char gTEWindowPrintfBuffer[2048];

int
TEWindow::printf(const char* format, ...)
{
	int result = 0;
	va_list list;
	va_start(list, format);
	result = vprintf(format, list);
	va_end(list);
	return result;
}

int
TEWindow::vprintf(const char* format, va_list list)
{
	int result = vsnprintf(gTEWindowPrintfBuffer, sizeof(gTEWindowPrintfBuffer), format, list);
	if (gTEWindowPrintfBuffer[result - 1] == '\n')
		result--;
	if (result > 0)
		AppendText(gTEWindowPrintfBuffer, result);
	return result;
}

Boolean
TEWindow::ReadFromFile(void)
{
	SFTypeList typeList = {
							('T'<<24)+('E'<<16) +('X'<<8)+'T',
							('?'<<24)+('?'<<16) +('?'<<8)+'?',
						  	0
						  };
	StandardFileReply reply;

	StandardGetFile(nil, 2, typeList, &reply);

	return (reply.sfGood) ? ReadFromFile(&reply.sfFile) : false;
}

Boolean
TEWindow::ReadFromFile(const char* filename)
{
	FSSpec inFile;
	OSErr err;

	err = FSMakeFSSpec(0, 0, c2pstr((char*)filename), &inFile);
	p2cstr((StringPtr)filename);

	if (err != noErr)
	{
		Message(("Unable to open '%s': %d", filename, err));
		return false;
	}

	return ReadFromFile(&inFile);
}

Boolean
TEWindow::ReadFromFile(FSSpec* inFile)
{
	short refNum;
	OSErr err = FSpOpenDF(inFile,fsRdPerm,&refNum);

	// ===== Open the data for the file
	
	if (err != noErr)
	{
		Message(("Unable to open '%s': %d",inFile->name+1, err));
		return false;
	}

	// Get the length of the file
	
	long	count;
	count = -1;
	err = GetEOF(refNum,&count);
	if (err != noErr)
	{
		Message(("Unable to get eof for '%s': %d",inFile->name+1, err));
		return false;
	}
	
	inFile->name[inFile->name[0] + 1] = 0;
	Message(("'%s' is %d bytes",inFile->name+1,count-512));
	
	// ===== Get (length) storage for the file's contents	
	
	Ptr textP = NewPtr(count);
	if (!textP) {
		Message(("Not enough memory to open '%s'",inFile->name+1));
		FSClose(refNum);
		return false;
	}
	
	// ===== Read in the text

	err = FSRead(refNum,&count,textP);
	if (err != noErr)
	{
		Message(("Unable to read %d bytes from '%s': %d",
				count, inFile->name+1, err));
		return false;
	}

	// ===== Close the file

	err = FSClose(refNum);
	if (err != noErr)
	{
		Message(("Error in closing file '%s': %d", inFile->name+1, err));
		return false;
	}

	// ===== Append the text to the window's TERec

	AppendText(textP, count);
	
	SetTitle((const char*)inFile->name+1);
	DisposePtr(textP);
	AdjustBodyScroll();
	
	return true; 
}

Boolean
TEWindow::SaveToFile(void)
{
	StandardFileReply reply;
	Str255 title;
	OSErr err;
	 
	GetWTitle(w, title);
	StandardPutFile("\pSave window contents as:", title, &reply);
	
	if (!reply.sfGood)
		return false;
	
	if (!reply.sfReplacing)
	{
		err = FSpCreate(&reply.sfFile, kTECreator, kTEFileType, smSystemScript);
		if (err != noErr)
		{
			p2cstr(title);
			Message(("Unable to create save file '%s': %d", (char*)title, err));
			return false;
		}
	}
	
	return (reply.sfGood) ? SaveToFile(&reply.sfFile) : false;
}
Boolean
TEWindow::SaveToFile(const char* filename)
{
	FSSpec outFile;
	OSErr err;

	err = FSMakeFSSpec(0, 0, c2pstr((char*)filename), &outFile);
	p2cstr((StringPtr)filename);
	
	if ((err != noErr) && (err != fnfErr))
	{
		Message(("Unable to make FSSpec out of '%s': %d", filename, err));
		return false;
	}
	
	if (err == fnfErr)
	{
		err = FSpCreate(&outFile, kTECreator, kTEFileType, smSystemScript);
		if (err != noErr)
		{
			Message(("Unable to create save file '%s': %d", filename, err));
			return false;
		}
	}

	return SaveToFile(&outFile);
}
Boolean
TEWindow::SaveToFile(FSSpec* outFile)
{
	short refNum;

	// open data fork of destination file

	OSErr err = FSpOpenDF(outFile, fsWrPerm, &refNum);

	if (err != noErr)
	{
		Message(("Error in opening data fork of '%s' for writing: %d",
				outFile->name+1, err));
	}
	else
	{
		// Get the body text, write it out
	
		char* text;
		ulong length;
		GetBodyText(&text, &length);
		
		err = FSWrite(refNum, (long*)&length, text);	

		if (err != noErr)
		{
			Message(("Error in writing contents to file '%s': %d",
					outFile->name+1, err));
		}
	}
	// Close the file

	err = FSClose(refNum);
	if (err != noErr)
	{
		Message(("Error in closing file '%s': %d", outFile->name+1, err));
		return false;
	}

	return false;
}

void
TEWindow::GetBodyText(char** text, ulong* length)
{
	*text = nil;	// in case get fails...
	*length = 0;	// ...this is default response
	
	TEHandle teHandle = GetBodyTEHandle();
	if (teHandle != nil)
	{
		CharsHandle cHandle = TEGetText(teHandle);
		if (cHandle != nil)
		{
			*text = (char*)*cHandle;
			*length = (**teHandle).teLength;
		}
	}
}

void
TEWindow::SetBodyText(char* text, ulong length)
{
	TEHandle teHandle = GetBodyTEHandle();
	if (teHandle != nil)
	{
		TESetText(text, length, teHandle);
		Handle hText = TEGetText(teHandle);
		if (hText != nil)
		{
			long hTextLength = (**teHandle).teLength;
			char* textPtr = *hText;
			while (hTextLength-- > 0)
			{
				if (*textPtr == 0x0a)	// convert lfs
					*textPtr = 0x0d;	// to carriage return
				textPtr++;
			}
		}
	}
}

void
TEWindow::SetBodyFont(short font, short size, short style)
{
	GrafPtr savePort;
	GetPort(&savePort);
	SetPort(w);
	
	FontInfo finfo;
	TextFont(font);
	TextSize(size);
	TextFace(style);
	GetFontInfo(&finfo);

	TEHandle teHandle = GetBodyTEHandle();
	if (teHandle != nil)
	{
		(**teHandle).txFont = font;
		(**teHandle).txSize = size;
		(**teHandle).txFace = style;
		(**teHandle).fontAscent = finfo.ascent;
		(**teHandle).lineHeight = finfo.ascent + finfo.descent + finfo.leading;
		mVLineScroll = (**teHandle).lineHeight;
		if (GetVisible())
		{	AdjustBodyScroll();
		}
	}
	
	SetPort(savePort);
}
void
TEWindow::AdjustBodyScroll()
{
	TEHandle teHandle = GetBodyTEHandle();
	if (teHandle != nil)
	{
		GrafPtr savePort;
		GetPort(&savePort);
		SetPort(w);
		
		Rect r;
		GetBodyRect(&r);
		InsetRect(&r, 2, 2);
		TECalText(teHandle);

		short bodyRectWidth = r.right - r.left;
		short bodyRectHeight = r.bottom - r.top;
		
		short teHeight = (**teHandle).destRect.bottom - (**teHandle).destRect.top;
		short teWidth = (**teHandle).destRect.right - (**teHandle).destRect.left;
		
		short viewDispVert = (**teHandle).destRect.top - (**teHandle).viewRect.top;
		short viewDispHorz = (**teHandle).destRect.left - (**teHandle).viewRect.left;

		short contentHeight = ((**teHandle).nLines + 1) * (**teHandle).lineHeight;

		
		if (bodyRectWidth != teWidth)	// if width has changed
		{
			// get point onscreen ==> convert to text offset
			MacPoint pt;
			pt.v = (**teHandle).viewRect.top;	// +2 ?
			pt.h = (**teHandle).viewRect.left;	// +2 ?
			
			short viewOffset = TEGetOffset(pt, teHandle);
			
			// change viewRect and destRect to current body rect, recalculate size
			(**teHandle).destRect = r;
			(**teHandle).viewRect = r;
			
			if (fHScroll != nil)
			{	SetCtlValue(fHScroll,0);
				mLastHScroll = 0;
			}
			if (fVScroll != nil)
			{	SetCtlValue(fVScroll,0);
				mLastVScroll = 0;
			}
			
			TECalText(teHandle);
			
			contentHeight = ((**teHandle).nLines + 1) * (**teHandle).lineHeight;
			
			SetBodySize(bodyRectWidth, contentHeight);
			//SetBodySize(bodyRectWidth,
			//		(**teHandle).nLines * (**teHandle).lineHeight + (teHeight - (**teHandle).lineHeight));
		
			// use text offset from above to get point in new destRect
			pt = TEGetPoint(viewOffset, teHandle);
			
			pt.v -= r.top;
			pt.v -= (**teHandle).lineHeight; // is this right?
			
			// set vertical of viewRect to that point (don't scroll horizontally!)
			(**teHandle).destRect.top -= pt.v;	// scroll off most of it
			(**teHandle).destRect.bottom = (**teHandle).destRect.top + contentHeight;
			InvalRect(&r);	// need to redraw!
		}
		else
		{
			SetBodySize(bodyRectWidth, contentHeight);
			(**teHandle).destRect.bottom = (**teHandle).destRect.top + contentHeight;
		}
		//else
		//{
		//	SetBodySize(bodyRectWidth,
		//		(**teHandle).nLines * (**teHandle).lineHeight + (teHeight - (**teHandle).lineHeight));
		//}
		
		if (fVScroll != nil)
		{
			mLastVScroll = (**teHandle).viewRect.top - (**teHandle).destRect.top;
			SetCtlMax(fVScroll, contentHeight - bodyRectHeight);
			SetCtlValue(fVScroll, mLastVScroll);
		}
		//if (fHScroll != nil)
		//{
		//	mLastHScroll = (**teHandle).viewRect.left - (**teHandle).destRect.left;
		//	SetCtlMax(fHScroll, bodyRectWidth);
		//	SetCtlValue(fHScroll, mLastHScroll);
		//}
		
		SetPort(savePort);
	}
}
void
TEWindow::SelectAll(void)
{
	TEHandle teHandle = GetActiveTEHandle();
	if (teHandle != nil)
	{
		TESetSelect(0, kTEMaxSize, teHandle);
	}
}

void
TEWindow::GetBodySelection(char** textString, long* length)
{
	TEHandle teHandle = GetBodyTEHandle();
	if ((teHandle == nil) || ((**teHandle).hText == nil))
	{
		*textString = nil;
		length = 0;
		return;
	}
	*textString = &((*((**teHandle).hText))[(**teHandle).selStart]);
	*length = (**teHandle).selEnd - (**teHandle).selStart;
}

void
TEWindow::GetBodySelectionRange(long* selStart, long* selEnd)
{
	TEHandle teHandle = GetBodyTEHandle();
	if ((teHandle == nil) || ((**teHandle).hText == nil))
	{
		selStart = 0;
		selEnd = 0;
		return;
	}
	*selStart = (**teHandle).selStart;
	*selEnd = (**teHandle).selEnd;
}
//void
//TEWindow::SetBodySelection(char* textString, long length)
//{
//}
void
TEWindow::SetBodySelectionRange(long selStart, long selEnd)
{
	TEHandle teHandle = GetBodyTEHandle();
	if (teHandle == nil)
		return;
	
	long teLength = (**teHandle).teLength;
	
	if (selStart > teLength)
		selStart = teLength;
	if (selEnd > teLength)
		selEnd = teLength;
	
	(**teHandle).selStart = selStart;
	(**teHandle).selEnd = selEnd;
}

void
TEWindow::FindForwardInBody(const char* text, ulong length)
{
	TEHandle teHandle = GetBodyTEHandle();
	if (teHandle == nil)
		return;

	int teLength = (**teHandle).teLength;
	int teStart = (**teHandle).selStart;
	int teEnd = (**teHandle).selEnd;
	Handle textHandle = (**teHandle).hText;

	if (textHandle == nil)
		return;
	
	char* textPtr = *textHandle;
	
	if (teStart + length == teEnd)
		teStart++;	// if curr selection IS find, don't count it
	
	int stopSearch = teLength - length;
	
	if (GetFindDialogWrapSearch())
	{
		stopSearch = teStart - 1;
		if (stopSearch < 0)
			stopSearch = teLength - length;
	}
	
	while (teStart != stopSearch)
	{
		// if not case sensitive, EqualStringN is ok, else use strncmp
		
		if (((!GetFindDialogCaseSensitive()) && EqualStringN(text, &(textPtr[teStart]), length))
			 || (strncmp(text, &(textPtr[teStart]), length) == 0))
		{
			// if entire word, check pre/post string to make sure they're non-alphanumeric characters
			
			if (!GetFindDialogEntireWord() ||
				(((teStart == 0) || (!isalnum(textPtr[teStart-1])))
				 && (teStart + length == teLength) || (!isalnum(textPtr[teStart+length]))))
			{
				// found a match! stop here
				TESetSelect(teStart, teStart+length, teHandle);
				TESelView(teHandle);
				AdjustBodyScroll();
				break;
			}
		}
		if (teStart == stopSearch)
			break;
		teStart++;
		if (teStart > teLength - length)
		{
			teStart = 0;
		}
	}	
}
void
TEWindow::FindBackwardInBody(const char* text, ulong length)
{
	TEHandle teHandle = GetBodyTEHandle();
	if (teHandle == nil)
		return;

	int teLength = (**teHandle).teLength;
	int teStart = (**teHandle).selStart;
	int teEnd = (**teHandle).selEnd;
	Handle textHandle = (**teHandle).hText;
	
	if (textHandle == nil)
		return;
	
	char* textPtr = *textHandle;

	if (teStart + length == teEnd)
		teStart--;	// if curr selection IS find, don't count it
	
	int stopSearch = 0;
	
	if (GetFindDialogWrapSearch())
	{
		stopSearch = teStart + 1;
		if (stopSearch > teLength - length)
			stopSearch = 0;
	}
	
	while (teStart != stopSearch)
	{
		// if not case sensitive, EqualStringN is ok, else use strncmp
		
		if (((!GetFindDialogCaseSensitive()) && EqualStringN(text, &(textPtr[teStart]), length))
			 || (strncmp(text, &(textPtr[teStart]), length) == 0))
		{
			// if entire word, check pre/post string to make sure they're non-alphanumeric characters
			
			if (!GetFindDialogEntireWord() ||
				(((teStart == 0) || (!isalnum(textPtr[teStart-1])))
				 && (teStart + length == teLength) || (!isalnum(textPtr[teStart+length]))))
			{
				// found a match! stop here
				TESetSelect(teStart, teStart+length, teHandle);
				TESelView(teHandle);
				AdjustBodyScroll();
				break;
			}
		}
		if (teStart == stopSearch)
			break;
		teStart--;
		if (teStart < 0)
		{
			teStart = teLength - length;
		}
	}	
}
void
TEWindow::FindSelectionForwardInBody(void)
{
	TEHandle teHandle = GetBodyTEHandle();
	if ((teHandle != nil) && ((**teHandle).selStart != (**teHandle).selEnd) && ((**teHandle).hText != nil))
	{
		char* textPtr;
		long textLength;
	
		GetBodySelection(&textPtr, &textLength);

		SetFindDialogSearchString(textPtr, textLength);
		FindForwardInBody(textPtr, textLength);
	}
}
void
TEWindow::FindSelectionBackwardInBody(void)
{
	TEHandle teHandle = GetBodyTEHandle();
	if ((teHandle != nil) && ((**teHandle).selStart != (**teHandle).selEnd) && ((**teHandle).hText != nil))
	{
		char* textPtr;
		long textLength;
	
		GetBodySelection(&textPtr, &textLength);

		SetFindDialogSearchString(textPtr, textLength);
		FindBackwardInBody(textPtr, textLength);
	}
}

TEHandle
TEWindow::GetActiveTEHandle(void)
{
	return fActiveTEHandle;
}
TEHandle
TEWindow::GetBodyTEHandle(void)
{
	Assert(this != nil && fBodyTEHandle != nil);
	return fBodyTEHandle;
}
Boolean
TEWindow::GetBodyTEWriteProtect(void)
{
	return fBodyTEWriteProtect;
}
		
void
TEWindow::SetActiveTEHandle(TEHandle teHandle)
{
	if (fIsActivated)
	{
		if (fActiveTEHandle != nil)
			TEDeactivate(fActiveTEHandle);
		TEActivate(teHandle);
		fIsActivated = true;
	}
	
	fActiveTEHandle = teHandle;
}
void
TEWindow::SetBodyTEWriteProtect(Boolean writeProtect)
{
	fBodyTEWriteProtect = writeProtect;
}
void
TEWindow::SetBodyTEHandle(TEHandle teHandle)
{
	fBodyTEHandle = teHandle;
}
