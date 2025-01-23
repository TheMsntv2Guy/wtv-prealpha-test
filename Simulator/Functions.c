// ===========================================================================
//	Functions.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __BOXPRINTDEBUG_H__
#include "BoxPrintDebug.h"
#endif
#ifndef __DISPLAYABLESWINDOW_H__
#include "DisplayablesWindow.h"
#endif
#ifndef __FUNCTIONS_H__
#include "Functions.h"
#endif
#ifndef __GIF_H__
#include "GIF.h"
#endif
#ifndef __IMAGEDATA_H__
#include "ImageData.h"
#endif
#ifndef __MACINTOSHDIALOGS_H__
#include "MacintoshDialogs.h"
#endif
#ifndef __MACINTOSHMENUS_H__
#include "MacintoshMenus.h"
#endif
#ifndef __MACSIMULATOR_H__
#include "MacSimulator.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __PAGEVIEWER_H__
#include "PageViewer.h"
#endif
#ifndef __SIMULATOR_RSRC_H__
#include "Simulator.rsrc.h"
#endif
#ifndef __TELLYIO_H__
#include "TellyIO.h"		/* for RunScript() */
#endif
#ifndef __DEFS_H__
#include "defs.h"
#endif
#ifndef TINYIP_H
#include "tinyip.h"
#endif
#ifndef _TINYTCP_
#include "tinytcp.h"
#endif
#ifndef __TELLYIO__
#include "TellyIO.h"
#endif
#ifndef __INTERPRETER__
#include "Interpreter.h"
#endif
#ifndef __SCANNER__
#include "Scanner.h"
#endif

#if defined FIDO_INTERCEPT
#include "FidoWindow.h"
#endif
#ifdef DEBUG_TOURISTWINDOW
	#ifndef __TOURISTWINDOW_H__
	#include "TouristWindow.h"
	#endif
#endif

// ===========================================================================

typedef void (MenuFunction)(short menuItem);

struct MenuFunctionEntry
{
	char*			name;
	MenuFunction*	function;
};
#define DeclareNewFunction(functionName)	{ #functionName, functionName }

// ===========================================================================
//	static prototypes
// ===========================================================================

static void 		FadeOut(short);
static void 		ShowDisplayables(short);
static void 		ShowFido(short);

#ifdef DEBUG_TOURISTWINDOW
static void 		ShowTourist(short);
#endif

static void 		SimulateCarrierLoss(short);
static void 		PrecompileTellyscript(short);

#ifdef DEBUG_BOXPRINT
static void 		Test_StaticBoxPrintDebug(short);
#endif

static void 		ExecuteSpiesTellyscript(short);
static void 		ExecuteRegistrationTellyscript(short);
static void 		TryOpeningAConnection(short);

static void 		ChangeFunctionsMenuItem(short menuItem, Str255 name, MenuFunction* function);
static void			CheckFunctionsMenuItem(short menuItem, const char mark);
static GWorldPtr	NewWorld(Rect *r);
static void			CopyWorldRects(GWorldPtr src, GWorldPtr dst, Rect *srcRect, Rect *dstRect);
static void			CopyWorld(GWorldPtr src, GWorldPtr dst);
static void			CopyToBox(GWorldPtr src, Rect *r, short transferMode);
static void			CopyFromWorld(GWorldPtr src, short transferMode);
static void			CopyBoxToWorld(GWorldPtr dst, Rect *r);
static void			CopyToWorld(GWorldPtr dst);

// ===========================================================================
//	static structs
// ===========================================================================

static MenuFunctionEntry		gMenuFunctionEntries[] =
{
	/* Add new functions here */
	DeclareNewFunction(FadeOut),
	DeclareNewFunction(ShowDisplayables),
#if defined FIDO_INTERCEPT
	DeclareNewFunction(ShowFido),
#endif
#ifdef DEBUG_TOURISTWINDOW
	DeclareNewFunction(ShowTourist),
#endif
	DeclareNewFunction(SimulateCarrierLoss),
#ifdef DEBUG_BOXPRINT
	DeclareNewFunction(Test_StaticBoxPrintDebug),
#endif
	DeclareNewFunction(PrecompileTellyscript),

	{ nil, nil }		// last entry is nil	
};




// ===========================================================================
//	implementations
// ===========================================================================

void
BuildFunctionsMenu(void)
{
	MenuFunctionEntry*		entry = gMenuFunctionEntries;
	ulong					cmdNumber = 1;
	
	for (; entry->name != nil; entry++, cmdNumber++)
	{
		insertmenuitem(GetMenu(mFunctions), entry->name, cmdNumber - 1);
		if (cmdNumber <= 9)
			SetItemCmd(GetMenu(mFunctions), cmdNumber, cmdNumber + '0');
	}
}

void
HandleFunctionsMenu(short menuItem)
{
	Assert(0 < menuItem && menuItem < sizeof(gMenuFunctionEntries)/sizeof(gMenuFunctionEntries[0]));
	Assert(gMenuFunctionEntries[menuItem-1].function != nil);
	
	(*gMenuFunctionEntries[menuItem-1].function)(menuItem);
}

static void
ChangeFunctionsMenuItem(short menuItem, Str255 name, MenuFunction* function)
{
	SetMenuItemText(GetMenu(mFunctions), menuItem, name);
	// nobody cares about this now...gMenuFunctionEntries[menuItem-1].name = name;
	gMenuFunctionEntries[menuItem-1].function = function;
}

static void
CheckFunctionsMenuItem(short menuItem, const char mark)
{
	SetItemMark(GetMenu(mFunctions), menuItem, mark);
}

static GWorldPtr
NewWorld(Rect *r)
{
	CGrafPtr	oldPort;
	GDHandle	oldGD;
	GWorldPtr	w;
	
	if (NewGWorld(&w,32,r,nil,nil,0)) return 0;
	LockPixels(w->portPixMap);

	GetGWorld(&oldPort,&oldGD);
	SetGWorld(w,nil);
	PaintRect(r);
	SetGWorld(oldPort,oldGD);
	return w;
}

//	Copy rectangles from one GWorld to another

static void
CopyWorldRects(GWorldPtr src, GWorldPtr dst, Rect *srcRect, Rect *dstRect)
{
	CGrafPtr	oldPort;
	GDHandle	oldGD;
	
	GetGWorld(&oldPort,&oldGD);
	SetGWorld(dst,nil);
	CopyBits(&((GrafPtr)(src))->portBits,&((GrafPtr)(dst))->portBits,srcRect,dstRect,64,nil);
	SetGWorld(oldPort,oldGD);
}

//	Copy from one gWorld to another

static void
CopyWorld(GWorldPtr src, GWorldPtr dst)
{
	CopyWorldRects(src,dst,&src->portRect,&dst->portRect);
}

//	Copy to the current port

static void
CopyToBox(GWorldPtr src, Rect *r, short transferMode)
{
	CopyBits(&((GrafPtr)(src))->portBits,&(qd.thePort)->portBits,&src->portRect,r,transferMode,nil);
}

static void
CopyFromWorld(GWorldPtr src, short transferMode)
{
	CopyToBox(src, &(qd.thePort)->portRect, transferMode);
}

//	Copy from the current port

static void
CopyBoxToWorld(GWorldPtr dst, Rect *r)
{
	CGrafPtr	oldPort;
	GDHandle	oldGD;
	
	GetGWorld(&oldPort,&oldGD);
	SetGWorld(dst,nil);
	CopyBits(&((GrafPtr)(oldPort))->portBits,&((GrafPtr)(dst))->portBits,r,&dst->portRect,64,nil);
	SetGWorld(oldPort,oldGD);
}

static void
CopyToWorld(GWorldPtr dst)
{
	CopyBoxToWorld(dst, &(qd.thePort)->portRect);
}

static void
FadeOut(short)
{
	int			i;
	PenState	penState, oldState;
	RGBColor	opColor = { 0xFFF, 0xFFF, 0xFFF };
	RGBColor	fadeInColor = { 0x3000, 0x3000, 0x3000 };
	RGBColor	textColor = { 0xBFFF, 0xFFFF, 0xFFFF };
	char*		msg1 = "Yahoo: The";
	char*		msg2 = "State of the Art";
	char*		msg3 = "Reference for the Web";
	char*		msg4 = "http://www.yahoo.com";
	char*		msg5 = "Author........................................................................................................Joe Shmoe Jr.";
	char*		msg6 = "Site.............................................................................................................Stanford University";
	char*		msg7 = "Created......................................................................................................October 29, 1994";
	char*		msg8 = "Last Updated...........................................................................................September 13, 1995";
	GWorldPtr	savedWorld;
	Rect		clipRect;
	ImageData*	logoImage;
	
	SetPort(gMacSimulator->GetWindow());
	
	RGBColor black = {0, 0, 0};
	RGBColor white = {0xffff, 0xffff, 0xffff};
	RGBForeColor(&black);
	RGBBackColor(&white);
	
	savedWorld = NewWorld(&(qd.thePort)->portRect);
	Assert(savedWorld != nil);
	CopyToWorld(savedWorld);

	GetPenState(&penState);
	oldState = penState;
	TextSize(36);

	logoImage = ImageData::NewImageData("file://ROM/Logo.gif");
	Assert(logoImage != nil);

	for (i = 0; i < 32; i++)
	{
		penState.pnMode = subPin;
		SetPenState(&penState);
		RGBForeColor(&opColor);
		PaintRect(&gMacSimulator->GetWindow()->portRect);

		RGBForeColor(&textColor);
		penState = oldState;
		SetPenState(&penState);
		MoveTo(60, 350);
		DrawText(msg4, 0, strlen(msg3));

#if 0	/* fucks up the color table */
		Rectangle logoBounds;
		gSystemLogo->LogoBounds(&logoBounds);
		logoImage->Draw(&logoBounds);
#endif
	}

	textColor.red = 0; textColor.green = 0; textColor.blue = 0;
	penState.pnSize.h = 8; penState.pnSize.v = 8;
	SetPenState(&penState);
	for (i = 0; i < 512; i++)
	{
		RGBForeColor(&textColor);
	
		/* stop original text from flashing */
		clipRect = (qd.thePort)->portRect;
		clipRect.bottom = 353;
		ClipRect(&clipRect);

		TextSize(48);
		MoveTo(20, 90); DrawText(msg1, 0, strlen(msg1));
		MoveTo(20, 140); DrawText(msg2, 0, strlen(msg2));
		MoveTo(20, 190); DrawText(msg3, 0, strlen(msg3));
		
		TextSize(12);
		MoveTo(20, 220); DrawText(msg5, 0, strlen(msg5));
		MoveTo(20, 235); DrawText(msg6, 0, strlen(msg6));
		MoveTo(20, 250); DrawText(msg7, 0, strlen(msg7));
		MoveTo(20, 265); DrawText(msg8, 0, strlen(msg8));
		
		ClipRect(&(qd.thePort)->portRect);
		MoveTo(60, 360); Line(406*i/512, 0);

		textColor.red += 0xBFFF/512;
		textColor.green += 0xFFFF/512;
		textColor.blue += 0xFFFF/512;
	}

	penState.pnMode = subPin;
	SetPenState(&penState);
	RGBForeColor(&opColor);

	// restore to previous state
	penState = oldState;
	SetPenState(&penState);
	RGBForeColor(&black);
	RGBBackColor(&white);
	ClipRect(&(qd.thePort)->portRect);
	
	OpColor(&fadeInColor);
	for (i = 0; i < 24; i++)
	{
		CopyFromWorld(savedWorld, blend);
		fadeInColor.red *= 5/4; fadeInColor.green *= 5/4; fadeInColor.blue *= 5/4;
	}
	CopyFromWorld(savedWorld, srcCopy);
	DisposeGWorld(savedWorld);
}

static void
ShowDisplayables(short)
{
#ifdef DEBUG_DISPLAYABLEWINDOW
	DisplayablesWindow* dw = newMac(DisplayablesWindow);
	
	dw->ResizeWindow(300, 500);
	dw->SetTitle((const char*)"Displayables");
	dw->ShowWindow();
#endif /* DEBUG_DISPLAYABLEWINDOW */
}

#if defined FIDO_INTERCEPT
static void ShowFido(short)
{
	FidoWindow* fw = newMac(FidoWindow);
	fw->ResizeWindow(540, 400);
	fw->SetTitle((const char*)"Fido Window");
	fw->ShowWindow();
}
#endif /* FIDO_INTERCEPT */

#ifdef DEBUG_TOURISTWINDOW
static void
ShowTourist(short)
{
	TouristWindow* tw = newMac(TouristWindow);
	
	tw->ResizeWindow(600, 280);
	tw->SetTitle((const char*)"Tourist Window");
	tw->ShowWindow();
}
#endif /* DEBUG_TOURISTWINDOW */

static void
SimulateCarrierLoss(short)
{
	SetScriptResult(kTellyConnecting);
}

static void
PrecompileTellyscript(short)
{
	StandardFileReply reply;
	short	refNum;

	StandardGetFile(nil, -1, nil, &reply);

	if (reply.sfGood)
		{
		static Byte heap[SCRIPTMAX];
		static char data[16384];
		long		count;
 
		FSpOpenDF(&reply.sfFile, fsCurPerm, &refNum);

		(void)GetEOF(refNum, &count);
		(void)FSRead(refNum, &count, data);
		(void)FSClose(refNum);

		NewScript((uchar *)data, count);		/* data == where the source is */
		SetupPointers(heap, SCRIPTMAX, 0);		/* heap == where the tokenized version goes */
		count = LoadTokenBuffer();

		StandardPutFile("\pSave tokenized script as:", "\pscript.tok", &reply);

		if (reply.sfGood)
			{
			if (FSpCreate(&reply.sfFile, 'ANDY', 'TELY', reply.sfScript) == noErr)
				{
				FSpOpenDF(&reply.sfFile, fsCurPerm, &refNum);
				(void)FSWrite(refNum, &count, heap);
				(void)FSClose(refNum);
				}
			}
		}
}

#ifdef DEBUG_BOXPRINT
static void
Test_StaticBoxPrintDebug(short)
{
	char buffer[256] = "gNetwork";
	buffer[0] = 0;
	paramtext("Enter an expression of the form: \"gVariable\" or \"0xADDBE55 Type\"","","","");
	if (QueryUser(rGenericQueryDialog, buffer)) {
		char parseType[256];
		char parseAddress[256];
		if (sscanf(buffer, "%s %s", parseAddress, parseType) == 2) {
			BoxPrintDebugGlobal(parseType, parseAddress);
		} else if (sscanf(buffer, "%s", parseAddress) == 1) {
			BoxPrintDebugGlobal(nil, parseAddress);
		} else {
			BoxPrint("Cannot parse '%s'", buffer);
		}
	}
}
#endif /* DEBUG_BOXPRINT */
