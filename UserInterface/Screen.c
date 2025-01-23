// ===========================================================================
//	Screen.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __SCREEN_H__
#include "Screen.h"
#endif

#include "AlertWindow.h"
#include "BoxAbsoluteGlobals.h"
#include "HWDisplay.h"
#include "ImageData.h"
#include "InfoPanel.h"
#include "Keyboard.h"
#include "LoginPanel.h"
#include "MemoryManager.h"
#include "Menu.h"
#include "Network.h"
#include "OptionsPanel.h"
#include "PageViewer.h"
#include "RecentPanel.h"
#include "Status.h"
#include "System.h"
#include "SongData.h"
#include "SystemLogo.h"

#ifndef __PERFDUMP_H__
#include "PerfDump.h"
#endif

// ===========================================================================
//	Globals

Screen* 			gScreen					= nil;

Region*				gSelectionRegion		= nil;
#ifdef FIDO_INTERCEPT
Region*				gFidoSelectionRegion	= nil;
#endif

// ===========================================================================
//	Layers

AlertWindow* 		gAlertWindow			= nil;
ConnectIndicator*	gConnectIndicator		= nil;
ConnectWindow*		gConnectWindow			= nil;				
Panel*				gFindPanel				= nil;
Panel*				gAutoPanel				= nil;
Panel*				gGoPanel				= nil;
InfoPanel*			gInfoPanel				= nil;
Keyboard*			gKeyboard				= nil;
LoginPanel*			gLoginPanel				= nil;
MenuLayer*			gMenuLayer				= nil;
OptionsPanel*		gOptionsPanel			= nil;
PageViewer* 		gPageViewer				= nil;
PowerOffAlert*		gPowerOffAlert			= nil;
RecentPanel*		gRecentPanel			= nil;
Panel*				gSavePanel				= nil;
SplashWindow*		gSaveSplash				= nil;
Panel*				gSendPanel				= nil;
SplashWindow*		gSendSplash				= nil;
StatusIndicator*	gStatusIndicator		= nil;
SystemLogo* 		gSystemLogo				= nil;

// ===========================================================================
//	Borders

BorderImage* 		gAlertBorder 			= nil;
BorderImage*		gButtonBorder 			= nil;
BorderImage*		gMenuBorder				= nil;
BorderImage*		gPanelBorder			= nil;
BorderImage*		gRuleBorder				= nil;
BorderImage*		gSelectionBorder 		= nil;
BorderImage*		gTextFieldBorder		= nil;

// ===========================================================================
//	Images

ImageData* 			gAlertImage 			= nil;
ImageData* 			gCheckBoxImage 			= nil;
ImageData* 			gCheckedBoxImage 		= nil;
ImageData*			gContinueImage			= nil;
ImageData*			gMenuIconImage			= nil;
ImageData*			gMenuUpIconImage		= nil;
ImageData*			gMenuDownIconImage		= nil;
ImageData* 			gRadioButtonOffImage 	= nil;
ImageData* 			gRadioButtonOnImage 	= nil;
ImageData*			gSaveImage				= nil;

// ===========================================================================
//	ImageMapCursor

ImageMapCursor*		gImageMapCursorActive	= nil;
ImageMapCursor*		gImageMapCursorNormal	= nil;

// ===========================================================================
//	Sound Effects

SoundEffect* 		gEndLinkSound			= nil;
SoundEffect*		gErrorSound				= nil;
SoundEffect*		gLimitSound				= nil;
SoundEffect*		gModemSound				= nil;
SoundEffect*		gPopupSound				= nil;
SoundEffect*		gSaveSound				= nil;
SoundEffect*		gScroll1Sound			= nil;
SoundEffect*		gScroll2Sound			= nil;
SoundEffect*		gStartLinkSound			= nil;
SoundEffect*		gSubmitSound			= nil;
SoundEffect*		gTypeSound				= nil;

// ===========================================================================

Screen::Screen(void)
{
	gScreen = this;
	fCurrentPanel = nil;
	
	
	fBounds = gOnScreenDevice.bounds;

	CreateLayers();

	// Add screen globals to layer list (front to back).
	fLayerList = gSystemLogo;
	fLayerList->Add(gConnectWindow);
	fLayerList->Add(gSendSplash);
	fLayerList->Add(gSaveSplash);
	fLayerList->Add(gMenuLayer);
	fLayerList->Add(gStatusIndicator);
	fLayerList->Add(gConnectIndicator);
	fLayerList->Add(gAlertWindow);
	fLayerList->Add(gPowerOffAlert);
	fLayerList->Add(gKeyboard);
	fLayerList->Add(gOptionsPanel);
	fLayerList->Add(gRecentPanel);
	fLayerList->Add(gInfoPanel);
	fLayerList->Add(gGoPanel);
	fLayerList->Add(gAutoPanel);
	fLayerList->Add(gFindPanel);
	fLayerList->Add(gSavePanel);
	fLayerList->Add(gSendPanel);
	fLayerList->Add(gLoginPanel);
	fLayerList->Add(gPageViewer);
	
	CreateBorders();
	CreateImages();
	CreateSoundEffects();
	Layout();
		
	gPageViewer->Show();
}

Screen::~Screen(void)
{
	DestroyBorders();
	DestroySoundEffects();
	DestroyLayers();
}

void 
Screen::CloseAllPanels(void)
{
	gKeyboard->Close(true);
	SetCurrentPanel(nil);
	gOptionsPanel->Close();
}

void 
Screen::CreateBorders(void)
{
	IsError(nil == (gSelectionRegion = new(Region)));

	IsError(nil == (gAlertBorder		= BorderImage::NewBorderImage(kAlertBorderURL)));
	IsError(nil == (gButtonBorder		= BorderImage::NewBorderImage(kButtonBorderURL)));
	IsError(nil == (gMenuBorder			= BorderImage::NewBorderImage(kMenuBorderURL)));
	IsError(nil == (gPanelBorder		= BorderImage::NewBorderImage(kPanelBorderURL)));
	IsError(nil == (gRuleBorder			= BorderImage::NewBorderImage(kRuleBorderURL)));
	IsError(nil == (gSelectionBorder	= BorderImage::NewBorderImage(kSelectionBorderURL)));
	gSelectionBorder->SetAnimationRange(1,3);
	IsError(nil == (gTextFieldBorder	= BorderImage::NewBorderImage(kTextFieldBorderURL)));
}

void
Screen::CreateImages(void)
{
	// regular images
	
	IsError(nil == (gAlertImage				= ImageData::NewImageData(kAlertImageURL)));
	IsError(nil == (gCheckBoxImage			= ImageData::NewImageData(kCheckBoxImageURL)));
	IsError(nil == (gCheckedBoxImage		= ImageData::NewImageData(kCheckedBoxImageURL)));
	IsError(nil == (gContinueImage			= ImageData::NewImageData(kContinueImageURL)));
	IsError(nil == (gMenuIconImage			= ImageData::NewImageData(kMenuIconImageURL)));
	IsError(nil == (gMenuUpIconImage		= ImageData::NewImageData(kMenuUpIconImageURL)));
	IsError(nil == (gMenuDownIconImage		= ImageData::NewImageData(kMenuDownIconImageURL)));	
	IsError(nil == (gRadioButtonOffImage	= ImageData::NewImageData(kRadioButtonOffImageURL)));
	IsError(nil == (gRadioButtonOnImage		= ImageData::NewImageData(kRadioButtonOnImageURL)));
	IsError(nil == (gSaveImage				= ImageData::NewImageData(kSaveImageURL)));

	//еее Dave - temporary hack until we have real image.
	gMenuUpIconImage->SetFlip(true, true);
	
	// cursor images
	
	if (!IsError(nil == (gImageMapCursorNormal =
			ImageMapCursor::NewImageMapCursor(kImageMapCursorNormal)))) {
			gImageMapCursorNormal->SetHotspot(kImageMapCursorNormalHotspotX,
											  kImageMapCursorNormalHotspotY);
			gImageMapCursorNormal->GetStatus(); // Validate image type.
	}
	if (!IsError(nil == (gImageMapCursorActive =
			ImageMapCursor::NewImageMapCursor(kImageMapCursorActive)))) {
			gImageMapCursorActive->SetHotspot(kImageMapCursorActiveHotspotX,
											  kImageMapCursorActiveHotspotY);
			gImageMapCursorActive->GetStatus(); // Validate image type.
	}

}

void
Screen::CreateLayers(void)
{
	IsError(nil == (gAlertWindow 		= new(AlertWindow)));
	IsError(nil == (gConnectIndicator	= new(ConnectIndicator)));
	IsError(nil == (gConnectWindow		= new(ConnectWindow)));
	IsError(nil == (gFindPanel			= new(Panel)));
	IsError(nil == (gPowerOffAlert 		= new(PowerOffAlert)));
	IsError(nil == (gAutoPanel			= new(Panel)));
	IsError(nil == (gGoPanel			= new(Panel)));
	IsError(nil == (gInfoPanel			= new(InfoPanel)));
	IsError(nil == (gLoginPanel			= new(LoginPanel)));
	IsError(nil == (gKeyboard			= new(Keyboard)));
	IsError(nil == (gMenuLayer			= new(MenuLayer)));
	IsError(nil == (gOptionsPanel		= new(OptionsPanel)));
	IsError(nil == (gPageViewer			= new(PageViewer)));
	IsError(nil == (gRecentPanel		= new(RecentPanel)));
	IsError(nil == (gSavePanel			= new(Panel)));
	IsError(nil == (gSaveSplash			= new(SplashWindow)));
	IsError(nil == (gSendPanel			= new(Panel)));
	IsError(nil == (gSendSplash			= new(SplashWindow)));
	IsError(nil == (gStatusIndicator	= new(StatusIndicator)));
	IsError(nil == (gSystemLogo			= new(SystemLogo)));
	
	gAutoPanel->SetURL(kAutoPanelURL);
	gGoPanel->SetURL(kGoPanelURL);
	gFindPanel->SetURL(kFindPanelURL);
	gSaveSplash->SetURL(kSaveSplashURL);
	gSavePanel->SetURL(kSavePanelURL);
	gSendPanel->SetURL(kSendPanelURL);
	gSendSplash->SetURL(kSendSplashURL);
}

void 
Screen::CreateSoundEffects(void)
{
	IsError(nil == (gEndLinkSound	= SoundEffect::NewSoundEffect(kEndLinkSound, kEndLinkVolume, 	true)));
	IsError(nil == (gErrorSound		= SoundEffect::NewSoundEffect(kErrorSound, kErrorVolume,		true)));
	IsError(nil == (gLimitSound		= SoundEffect::NewSoundEffect(kLimitSound, kLimitVolume,		true)));
	IsError(nil == (gModemSound		= SoundEffect::NewSoundEffect(kModemSound, kModemVolume,		true)));
	IsError(nil == (gPopupSound		= SoundEffect::NewSoundEffect(kPopupSound, kPopupVolume,		true)));
	IsError(nil == (gSaveSound		= SoundEffect::NewSoundEffect(kSaveSound, kSaveVolume,		true)));
	IsError(nil == (gScroll1Sound	= SoundEffect::NewSoundEffect(kScroll1Sound, kScroll1Volume,	true)));
	IsError(nil == (gScroll2Sound	= SoundEffect::NewSoundEffect(kScroll2Sound, kScroll2Volume,	true)));
	IsError(nil == (gStartLinkSound	= SoundEffect::NewSoundEffect(kStartLinkSound, kStartLinkVolume,	true)));
	IsError(nil == (gSubmitSound	= SoundEffect::NewSoundEffect(kSubmitSound, kSubmitVolume,	true)));
	IsError(nil == (gTypeSound		= SoundEffect::NewSoundEffect(kTypeSound, kTypeVolume,		true)));
}

void 
Screen::DestroyBorders(void)
{
	delete(gAlertBorder);				gAlertBorder = nil;
	delete(gButtonBorder);				gButtonBorder = nil;
	delete(gMenuBorder);				gMenuBorder = nil;
	delete(gPanelBorder);				gPanelBorder = nil;
	delete(gRuleBorder);				gRuleBorder = nil;
	delete(gSelectionBorder);			gSelectionBorder = nil;
	delete(gTextFieldBorder);			gTextFieldBorder = nil;

	delete(gSelectionRegion);			gSelectionRegion = nil;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void
Screen::DestroyImages(void)
{
	delete(gAlertImage);				gAlertImage = nil;
	delete(gCheckBoxImage);				gCheckBoxImage = nil;
	delete(gCheckedBoxImage);			gCheckedBoxImage = nil;
	delete(gContinueImage);				gContinueImage = nil;
	delete(gMenuIconImage);				gMenuIconImage = nil;
	delete(gMenuUpIconImage);			gMenuUpIconImage = nil;
	delete(gMenuDownIconImage);			gMenuDownIconImage = nil;
	delete(gRadioButtonOffImage);		gRadioButtonOffImage = nil;
	delete(gRadioButtonOnImage);		gRadioButtonOnImage = nil;
	delete(gSaveImage);					gSaveImage = nil;

	delete(gImageMapCursorNormal);		gImageMapCursorNormal = nil;
	delete(gImageMapCursorActive);		gImageMapCursorActive = nil;
}
#endif

void
Screen::DestroyLayers(void)
{
	fLayerList->DeleteAll();

	if (IsError(gAlertWindow != nil))		gAlertWindow = nil;
	if (IsError(gConnectIndicator != nil))	gConnectIndicator = nil;
	if (IsError(gConnectWindow != nil))		gConnectWindow = nil;
	if (IsError(gFindPanel != nil))			gFindPanel = nil;
	if (IsError(gAutoPanel != nil))			gAutoPanel = nil;
	if (IsError(gGoPanel != nil))			gGoPanel = nil;
	if (IsError(gInfoPanel != nil))			gInfoPanel = nil;
	if (IsError(gLoginPanel != nil))		gLoginPanel = nil;
	if (IsError(gKeyboard != nil))			gKeyboard = nil;
	if (IsError(gMenuLayer != nil))			gMenuLayer = nil;
	if (IsError(gOptionsPanel != nil))		gOptionsPanel = nil;
	if (IsError(gPageViewer != nil))		gPageViewer = nil;
	if (IsError(gRecentPanel != nil))		gRecentPanel = nil;
	if (IsError(gSavePanel != nil))			gSavePanel = nil;
	if (IsError(gSaveSplash != nil))		gSaveSplash = nil;
	if (IsError(gSendPanel != nil))			gSendPanel = nil;
	if (IsError(gSendSplash != nil))		gSendSplash = nil;
	if (IsError(gStatusIndicator != nil))	gStatusIndicator = nil;
	if (IsError(gSystemLogo != nil))		gSystemLogo = nil;
}

void 
Screen::DestroySoundEffects(void)
{
	delete(gEndLinkSound);			gEndLinkSound = nil;
	delete(gErrorSound);			gErrorSound = nil;
	delete(gLimitSound);			gLimitSound = nil;
	delete(gModemSound);			gModemSound = nil;
	delete(gPopupSound);			gPopupSound = nil;
	delete(gSaveSound);				gSaveSound = nil;
	delete(gScroll1Sound);			gScroll1Sound = nil;
	delete(gScroll2Sound);			gScroll2Sound = nil;
	delete(gStartLinkSound);		gStartLinkSound = nil;
	delete(gSubmitSound);			gSubmitSound = nil;
	delete(gTypeSound);				gTypeSound = nil;
}

Boolean 
Screen::DispatchInput(Input* input)
{
	Layer* layer;
	
	// Note: this should be moved elsewhere
	for (layer = fLayerList; layer != nil; layer = (Layer*) layer->Next()) {
		layer->IdleLayer();
	}

	if (input->data != 0) {
		// Only visible layers can handle input.
		
		for (layer = fLayerList; layer != nil; layer = (Layer*) layer->Next())
			if (layer->IsVisible() && layer->DispatchInput(input))
				return true;
				
		// Screen gets last crack at it.
		if (HandlesInput::DispatchInput(input))
			return true;
			
	}
	
	return false;
}

void 
Screen::Draw(void)
{
	ProfileUnitEnter("Screen::Draw");
	PerfDump perfdump("Screen::Draw");
	PushDebugChildURL("Screen::Draw");

	Layer*	layer;
	Layer*	last;

	if (!gSystem->GetIsOn()) {
		// note: this is overkill to do every time
		PaintRectangle(gScreenDevice, gScreenDevice.bounds, kBlackColor, nil);
#ifndef FOR_MAC		// just too pianful for Mac
		UpdateScreenBits();
#endif
		PopDebugChildURL();
		ProfileUnitExit("Screen::Draw");
		return;
	}
	
	if (!fIsDirty || fLayerList == nil) {
		PopDebugChildURL();
		ProfileUnitExit("Screen::Draw");
		return;
	}
	
	IsError(GetInDrawingLoop());
	SetInDrawingLoop(true);

	//#if defined(FOR_MAC)
	//gMacSimulator->StartProfiling();
	//#endif
	
	last = (Layer*)fLayerList->Last();
	for (layer = last; layer != nil; layer = (Layer*)layer->Previous())
		layer->DrawLayer();
		
	//#if defined(FOR_MAC)
	//gMacSimulator->StopProfiling();
	//#endif

	IsError(!GetInDrawingLoop());
	SetInDrawingLoop(false);
	fIsDirty = false;

	PopDebugChildURL();
	ProfileUnitExit("Screen::Draw");
}

#ifdef INCLUDE_FINALIZE_CODE
void 
Screen::Finalize(void)
{
	if (gScreen != nil) {	
		delete(gScreen);
		gScreen = nil;
	}
}
#endif

Panel* 
Screen::GetCurrentPanel(void) const
{
	return fCurrentPanel;
}

TextField* 
Screen::GetKeyboardTarget(void) const
{
	if (fLayerList == nil)
		return nil;
	
	return fLayerList->GetKeyboardTarget();
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
ContentView* 
Screen::GetTopContentView(void) const
{
	if (fLayerList == nil)
		return nil;
	
	return fLayerList->GetTopContentView();
}
#endif

Layer* 
Screen::GetTopLayer(void) const
{
	if (fLayerList == nil)
		return nil;
	
	return fLayerList->GetTopLayer();
}

Boolean 
Screen::HomeInput(void)
{
	// Disable Home button if we do not yet have a ticket.
	if (!gNetwork->IsSecure())
		return true;
	
	gEndLinkSound->Play();
	CloseAllPanels();
	gPageViewer->ExecuteURL(gSystem->GetHomeURL(), nil);

	return true;
}

void 
Screen::Initialize(void)
{
	if (!IsError(gScreen != nil))
		gScreen = new(Screen);
}

void 
Screen::Layout(void)
{
	const short kFindSideMargin = 20;
	const short kFindTopMargin = 225;
	const short kInfoSideMargin = 20;
	const short kInfoTopMargin = 150;
	const short kRecentSideMargin = 20;
	const short kRecentTopMargin = 40;
	const short kGoSideMargin = 20;
	const short kGoTopMargin = 150;
	const short kSendSideMargin = 20;
	const short kSendTopMargin = 130;
	const short kSaveWidth = 280;
	const short kSaveHeight = 150;
	
	short saveStartX = (GetWidth() / 2) - kSaveWidth/2;
	short saveStartY = (GetHeight() / 2) - kSaveHeight/2 - gOptionsPanel->GetHeight();
	
	gFindPanel->SetBounds(kFindSideMargin, kFindTopMargin,
				GetWidth() - kFindSideMargin, 
				GetHeight() - gOptionsPanel->GetHeight());
				
	gAutoPanel->SetBounds(kGoSideMargin, kGoTopMargin,
				GetWidth() - kGoSideMargin, 
				GetHeight() - gOptionsPanel->GetHeight());
				
	gGoPanel->SetBounds(kGoSideMargin, kGoTopMargin,
				GetWidth() - kGoSideMargin, 
				GetHeight() - gOptionsPanel->GetHeight());
				
	gInfoPanel->SetBounds(kInfoSideMargin, kInfoTopMargin,
				GetWidth() - kInfoSideMargin, 
				GetHeight() - gOptionsPanel->GetHeight());
				
	gLoginPanel->SetBounds(kGoSideMargin, kGoTopMargin,
				GetWidth() - kGoSideMargin, 
				GetHeight() - gOptionsPanel->GetHeight());
				
	gRecentPanel->SetBounds(kRecentSideMargin, kRecentTopMargin,
				GetWidth() - kRecentSideMargin, 
				GetHeight() - gOptionsPanel->GetHeight());

	gSavePanel->SetBounds(kInfoSideMargin, kInfoTopMargin,
				GetWidth() - kInfoSideMargin,
				GetHeight() - gOptionsPanel->GetHeight());

	gSaveSplash->SetBounds(saveStartX, saveStartY, saveStartX + kSaveWidth, 
				saveStartY + kSaveHeight);
				
	gSendPanel->SetBounds(kInfoSideMargin, kSendTopMargin,
				GetWidth() - kSendSideMargin, 
				GetHeight() - gOptionsPanel->GetHeight());

	gSendSplash->SetBounds(saveStartX, saveStartY, saveStartX + kSaveWidth, 
				saveStartY + kSaveHeight);
}

Boolean
Screen::KeyboardInput(Input* input)
{
	// Handle global command keys.
	
	if (input->modifiers & kAltModifier) {
	
		switch (input->data) {
			case 'r':
			case 'R':
				ReloadPage();
				break;
				
			default:
				// Unhandled command keys get passed on.
				return false;
		}
		
		return true;
	
	} else
		return false;
}

Boolean 
Screen::OptionsInput(void)
{
	if (!gOptionsPanel->IsVisible())
		return false;

	gScroll1Sound->Play();
	gSystemLogo->Hide();
					
	if (gOptionsPanel->IsOpen()) {
			gOptionsPanel->Close();
	} else {
		// close any subpanels that are open.
#if 0
		Panel* panel = GetCurrentPanel();
		
		if (panel != nil && panel->IsOpen())
			panel->Close();
#endif
		CloseAllPanels();		
		gOptionsPanel->Open();
	}

	RedrawNow();		// prevent bouncing	
	FlushInput();

	return true;
}

Boolean 
Screen::RecentInput(void)
{
	if (IsError(gRecentPanel == nil))
		return true;
		
	if (gRecentPanel->IsVisible())
		gRecentPanel->Close();
	else  if (gOptionsPanel->IsVisible()) {
		if (gOptionsPanel->IsOpen())
			gOptionsPanel->Close();
		gRecentPanel->Open();
	}
		
	return true;
}

void 
Screen::RedrawNow(void)
{
	Draw();
	UpdateScreenBits();
}

void
Screen::ReloadPage()
{
	CloseAllPanels();
	gPageViewer->Reload();
	gPageViewer->InvalidateBounds();
	gOptionsPanel->InvalidateBounds();
	RedrawNow();
}

Boolean 
Screen::SearchInput(void)
{
	const char* url;
	
	// Ignore input if there is no search URL.
	if ((url = gSystem->GetSearchURL()) == nil)
		return true;
		
	gEndLinkSound->Play();
	CloseAllPanels();
	gPageViewer->ExecuteURL(url, nil);
	return true;
}

void 
Screen::SetCurrentPanel(Panel* panel)
{
	if (fCurrentPanel != nil && fCurrentPanel != panel && fCurrentPanel->IsOpen())
		fCurrentPanel->Close();
		
	fCurrentPanel = panel;
}

void 
Screen::SlideAreaDown(const Rectangle* bounds, ulong scrollIncrement,
					  ulong distance, ushort)
{
	PerfDump perfdump("Screen:SlideAreaDown");
	
	Rectangle copyOnRect;
	Rectangle scrolledRect, oldScrolledRect;
	Boolean firstTime = true;
	
	copyOnRect = *bounds;
	scrolledRect = copyOnRect;

	while (scrolledRect.top < bounds->top + distance) {
		long increment = MIN(scrollIncrement, bounds->top + distance - scrolledRect.top);

		oldScrolledRect = scrolledRect;
		scrolledRect.top += increment;
		oldScrolledRect.bottom -= increment;

		copyOnRect.bottom = copyOnRect.top + increment;

			
		CopyImage(gOnScreenDevice, gOnScreenDevice, oldScrolledRect, scrolledRect);
		if ( firstTime ) {
			Rectangle topEdge;
			SetRectangle(topEdge,oldScrolledRect.left,oldScrolledRect.top-1,oldScrolledRect.right,oldScrolledRect.top+1);
			CopyImage(gScreenDevice, gOnScreenDevice, topEdge, topEdge);
			firstTime = false;
		}
		gOnScreenDevice.filter = kCopyFilterOffset;
		CopyImage(gScreenDevice, gOnScreenDevice, copyOnRect, copyOnRect);
		gOnScreenDevice.filter = kNoFilter;
		copyOnRect.top += increment;
	}
}

void 
Screen::SlideAreaUp(const Rectangle* bounds, ulong scrollIncrement,
					ulong distance, ulong startOffset, ushort shadow)
{
	PerfDump perfdump("Screen:SlideAreaUp");

	Rectangle srcRect, destRect;
	
	destRect = srcRect = *bounds;
	
	srcRect.bottom = srcRect.top + startOffset;
	destRect.top = destRect.bottom - startOffset;

	while ((destRect.bottom - destRect.top) < (distance + startOffset)) {
		long increment = MIN(scrollIncrement,
		                     (distance + startOffset) - (destRect.bottom - destRect.top));
		srcRect.bottom += increment;
		destRect.top -= increment;
		gOnScreenDevice.filter = srcRect.top == destRect.top ? kCopyFilter : kCopyFilterOffset;
		CopyImage(gScreenDevice, gOnScreenDevice, srcRect, destRect);
		gOnScreenDevice.filter = kNoFilter;
		if (shadow > 0 && (destRect.bottom - destRect.top) > shadow) {
			Rectangle	shadowBounds;
			shadowBounds.top = destRect.top + shadow;
			shadowBounds.left = destRect.right;
			shadowBounds.bottom = MIN(shadowBounds.top + scrollIncrement, destRect.bottom);
			shadowBounds.right = shadowBounds.left + shadow;
			PaintRectangle(gOnScreenDevice, shadowBounds, kBlackColor, 120, nil);
		}
	}
}

void 
Screen::SlidePageAndAreaDown(const Rectangle* bounds, ulong scrollIncrement, ulong distance)
{
	Rectangle copySrcRect, copyDstRect;
	Rectangle scrollSrcRect, scrollDstRect;
		
	copySrcRect = copyDstRect = *bounds;;
	copySrcRect.top += distance;

	while (copySrcRect.top > copyDstRect.top) {
		long increment = MIN(scrollIncrement, copySrcRect.top - copyDstRect.top);
		
		scrollDstRect = scrollSrcRect = *bounds;	
		scrollSrcRect.bottom -= increment;
		scrollDstRect.top += increment;
	
		copySrcRect.top -= increment;		
		copySrcRect.bottom = copySrcRect.top + increment;
		copyDstRect.bottom = copyDstRect.top + increment;
		
		CopyImage(gOnScreenDevice, gOnScreenDevice, scrollSrcRect, scrollDstRect);
		gOnScreenDevice.filter = copySrcRect.top == copyDstRect.top ? kCopyFilter : kCopyFilterOffset;
		CopyImage(gScreenDevice, gOnScreenDevice, copySrcRect, copyDstRect);		
		gOnScreenDevice.filter = kNoFilter;
	}
}

void 
Screen::SlideViewAndAreaDown(const Rectangle* viewBounds, const Rectangle* areaBounds, 
							 ulong scrollIncrement, ulong distance)
{
	Rectangle copySrcRect, copyDstRect;
	Rectangle viewScrollSrcRect, viewScrollDstRect;
	Rectangle areaScrollSrcRect, areaScrollDstRect;

	areaScrollSrcRect = areaScrollDstRect = *areaBounds;
	viewScrollSrcRect = viewScrollDstRect = *viewBounds;
			

	while (distance > 0) {
		long increment = MIN(scrollIncrement, distance);

		// Scroll the area and view down by the increment.		
		OffsetRectangle(viewScrollDstRect, 0, increment);
		OffsetRectangle(areaScrollDstRect, 0, increment);
		areaScrollSrcRect.bottom -= increment;
		areaScrollDstRect.bottom -= increment;
		if (viewScrollDstRect.bottom > areaBounds->bottom) {
			viewScrollSrcRect.bottom -= viewScrollDstRect.bottom - areaBounds->bottom;
			viewScrollDstRect.bottom = areaBounds->bottom;
		}
			
		CopyImage(gOnScreenDevice, gOnScreenDevice, areaScrollSrcRect, areaScrollDstRect);
		CopyImage(gOnScreenDevice, gOnScreenDevice, viewScrollSrcRect, viewScrollDstRect);

		// Copy on the scrolled from area. This can require 3 copys. The area above the view,
		// the area left of the view, and the area right of the view.
		copySrcRect = viewScrollSrcRect;
		copySrcRect.bottom = copySrcRect.top + increment;
		copyDstRect = copySrcRect;
				
		gOnScreenDevice.filter = copySrcRect.top == copyDstRect.top ? kCopyFilter : kCopyFilterOffset;
		CopyImage(gScreenDevice, gOnScreenDevice, copySrcRect, copyDstRect);		
		
		if (areaBounds->left < viewBounds->left) {
			copySrcRect = areaScrollSrcRect;
			copySrcRect.bottom = copySrcRect.top + increment;
			copySrcRect.right = viewBounds->left;
			copyDstRect = copySrcRect;
		
			gOnScreenDevice.filter = copySrcRect.top == copyDstRect.top ? kCopyFilter : kCopyFilterOffset;
			CopyImage(gScreenDevice, gOnScreenDevice, copySrcRect, copyDstRect);		
		}
		
		if (areaBounds->right > viewBounds->right) {
			copySrcRect = areaScrollSrcRect;
			copySrcRect.bottom = copySrcRect.top + increment;
			copySrcRect.left = viewBounds->right;
			copyDstRect = copySrcRect;
		
			gOnScreenDevice.filter = copySrcRect.top == copyDstRect.top ? kCopyFilter : kCopyFilterOffset;
			CopyImage(gScreenDevice, gOnScreenDevice, copySrcRect, copyDstRect);		
		}
		
		gOnScreenDevice.filter = kNoFilter;
		
		viewScrollSrcRect = viewScrollDstRect;
		areaScrollSrcRect = areaScrollDstRect;
		
		distance -= increment;
	}
}

void 
Screen::SlideViewAndAreaUp(const Rectangle* viewBounds, const Rectangle* areaBounds,
						   ulong scrollIncrement, ulong distance)
{
	Rectangle copySrcRect, copyDstRect;
	Rectangle scrollSrcRect, scrollDstRect;
		
	copySrcRect = copyDstRect = *areaBounds;
	copyDstRect.top = viewBounds->bottom;
	copySrcRect.bottom = copySrcRect.top + RectangleHeight(copyDstRect);

	scrollDstRect = scrollSrcRect = *viewBounds;

	while (distance > 0) {
		long increment = MIN(scrollIncrement, distance);
		
		OffsetRectangle(scrollDstRect, 0, -increment);
		
		copyDstRect.top -= increment;
		copySrcRect.bottom += increment;

		CopyImage(gOnScreenDevice, gOnScreenDevice, scrollSrcRect, scrollDstRect);
		gOnScreenDevice.filter = copySrcRect.top == copyDstRect.top ? kCopyFilter : kCopyFilterOffset;
		CopyImage(gScreenDevice, gOnScreenDevice, copySrcRect, copyDstRect);
		gOnScreenDevice.filter = kNoFilter;

		scrollSrcRect = scrollDstRect;
		
		distance -= increment;
	}
}

void 
Screen::DisableOptionsPanel()
{
	if (!gOptionsPanel->IsVisible())
		return;
		
	// Set PageViewer size based on screen.
	gPageViewer->SetBounds(0, 0, GetWidth(), GetHeight());

	gOptionsPanel->Hide();
}

void 
Screen::EnableOptionsPanel()
{
	if (gOptionsPanel->IsVisible())
		return;

	// Set PageViewer size based on options panel.
	gPageViewer->SetBounds(0, 0, GetWidth(), GetHeight() - gOptionsPanel->GetHeight());
	
	gOptionsPanel->Show();
}


void 
Screen::HideOptionsPanel()
{
	gOptionsPanel->SetShown(false);
}

void 
Screen::ShowOptionsPanel()
{
	gOptionsPanel->SetShown(true);
}


// ===========================================================================


