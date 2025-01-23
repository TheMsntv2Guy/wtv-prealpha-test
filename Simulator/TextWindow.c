#include "Headers.h"

#include "MacintoshUtilities.h"
#include "TextWindow.h"

// ----------------------------------------------------------------------

#define kTextChunkSize	1024

//	Create a new window, don't show it yet

TextWindow::TextWindow()
{
	fTextHandle = NewHandleClear(kTextChunkSize);
	fHandleSize = kTextChunkSize;
	SetFormat(0, true, true, 0, false);	// Header, vscroll, hscroll, trailer, constrain
	SetBodyFont(kMonaco, 9);
	mVLineScroll = fLineHeight;
}

TextWindow::~TextWindow()
{
	if (fTextHandle)
		DisposeHandle(fTextHandle);
}

ulong LineLength(char* text)
{
	char* p = text;
	while (*p != 0 && *p != 0xa && *p != 0xd)
		p++;
	
	if ((*p == 0xd && *(p+1) == 0xa) || (*p == 0xa && *(p+1) == 0xd))
		p++;
	
	if (p-text > 1000)	// no more than 1000 chars per line
		return 1000;
	else
		return p-text;
}

void TextWindow::RecalculateBody()
{
	GrafPtr savePort;
	GetPort(&savePort);
	SetPort(w);
	
	ulong rows = 0;
	ulong columns;
	ulong maxColumns = 0;
	char* text = *fTextHandle;
	char* textEnd = text + fTextLength;
	
	while (text < textEnd)
	{
		columns = LineLength(text);
		maxColumns = MAX(columns, maxColumns);
		text += columns + 1;
		rows++;
	}
	Rect r;
	GetBodyRect(&r);
	long distFromBottom = mHeight - (r.bottom - r.top) - mLastVScroll;
	
	SetBodySize(maxColumns * fCharWidth + fCharWidth, rows * fLineHeight + fLineHeight);
	
	long newDistFromBottom = mHeight - (r.bottom - r.top) - mLastVScroll;
	if ((distFromBottom <= 0) && (newDistFromBottom > 0))
	{
		ScrollBody(0, -newDistFromBottom);
		short max = mLastVScroll + newDistFromBottom; // default max
		if (fVScroll != nil)
		{	
			max = GetCtlMax(fVScroll);	// but really want this for the control
			SetCtlValue(fVScroll, max);
		}
		mLastVScroll = max;
	}
	
	SetPort(savePort);
}

void TextWindow::DrawBody(Rect *r, short UNUSED(hScroll), short UNUSED(vScroll), Boolean UNUSED(scrolling))
{
	EraseRect(r);
	SetGray(0);
	BodyCoordinates(r);
	TextFont(fFontFace);
	TextSize(fFontSize);
	
	ulong lines = 0;
	HLock(fTextHandle);
	char* text = *fTextHandle;
	char* textEnd = text + fTextLength;
	ulong length;
	
	while (text < textEnd)
	{
		lines++;
		length = LineLength(text);
		MoveTo(4, lines * fLineHeight);
		DrawText(text, 0, length);
		text += length + 1;
	}
	
	HUnlock(fTextHandle);
	SetOrigin(0,0);
}

void TextWindow::RemoveText(ulong start, ulong length)
{
	if (start > fTextLength)
		return;
	if (start + length > fTextLength)
		length = fTextLength - start;

	HLock(fTextHandle);
	char* text = *fTextHandle + start;
	
	memmove(text, text+length, fTextLength - (start+length));
	fTextLength -= length;
	
	HUnlock(fTextHandle);
	RecalculateBody();
}

void TextWindow::AppendText(char* text, ulong length)
{
	if (fTextLength + length >= fHandleSize)
	{
		SetHandleSize(fTextHandle, fTextLength + length + kTextChunkSize);
		Assert(MemError() == 0);
		fHandleSize = fTextLength + length + kTextChunkSize;
	}
	memcpy((*fTextHandle) + fTextLength, text, length);
	fTextLength += length;

	RecalculateBody();
}

// Fills in the text body from the specified file

void TextWindow::ReadFile(const char* filename)
{
	short	refNum,Oops;
	long	count;

	FSSpec inFile;
	FSMakeFSSpec(0, 0, c2pstr((char*)filename), &inFile);
	p2cstr((unsigned char*)filename);

	if ((Oops = FSpOpenDF(&inFile,fsRdPerm,&refNum)) != 0)
	{
		Message(("Unable to open '%s': %d",inFile.name+1,Oops));
		return;
	}
	count = -1;
	GetEOF(refNum,&count);
	inFile.name[inFile.name[0] + 1] = 0;
	Message(("'%s' is %d bytes",inFile.name+1,count-512));
	
//	Read in the text

	Handle textH = NewHandle(count);
	if (!textH) {
		Message(("Not enough memory to open '%s'",inFile.name+1));
		FSClose(refNum);
		return;
	}
	FSRead(refNum,&count,*textH);
	FSClose(refNum);
	
	SetTitle((const char*)inFile.name+1);
	DisposeHandle(fTextHandle);
	fTextHandle = textH;
	fHandleSize = fTextLength = count;
	
	RecalculateBody();
}

void TextWindow::SetText(char* text, ulong length)
{
	fTextLength = 0;
	AppendText(text, length);
}

void TextWindow::SetBodyFont(short face, short size)
{
	GrafPtr savePort;
	GetPort(&savePort);
	SetPort(w);
	
	FontInfo	finfo;

	fFontFace = face;
	fFontSize = size;
	
	TextFont(fFontFace);
	TextSize(fFontSize);
	GetFontInfo(&finfo);
	
	fLineHeight = finfo.ascent + finfo.descent + finfo.leading + 1;
	fCharWidth = finfo.widMax;
	RecalculateBody();
	
	SetPort(savePort);
}
