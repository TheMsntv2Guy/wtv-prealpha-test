// ===========================================================================
//	Panel.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __PANEL_H__
#include "Panel.h"
#endif

#include "ImageData.h"
#include "MemoryManager.h"
#include "OptionsPanel.h"
#include "PageViewer.h"
#include "Screen.h"
#include "SongData.h"
#include "Sound.h"
#include "System.h"
#ifdef FIDO_INTERCEPT
#include "FidoCompatibility.h"
#endif

// =============================================================================

//const short kShadowWidth = 8; Work in progress - Dave.

const short kShadowWidth = 0;

#ifndef FOR_MAC
const ulong kPanelScrollRate = 9;
#else
const ulong kPanelScrollRate = 3;
#endif

// =============================================================================

Panel::Panel()
{
	fIsOpen = false;
	fIsVisible = false;
}

Panel::~Panel()
{
}

Boolean 
Panel::BackInput()
{
	if (!fIsVisible)
		return false;
		
	Close();
	
	return true;
}

void 
Panel::ChangeIdleState(ViewIdleState newState, ViewIdleState oldState)
{
	ContentView::ChangeIdleState(newState, oldState);

	// Don't show the panel until it is done laying out.
	if (newState == kDocumentLoadingComplete) {
		gScreen->CloseAllPanels();
		gScreen->SetCurrentPanel(this);
		gOptionsPanel->ScrollStatusChanged();
		Show();
		gPageViewer->RefreshSelection();
		InvalidateBounds();
		fIsOpen = true;
		
		gScroll2Sound->Play();
	
		Rectangle drawBounds = fBounds;
		drawBounds.right += kShadowWidth;
		Draw(&drawBounds);
	
		gScreen->SlideAreaUp(&fBounds, kPanelScrollRate, RectangleHeight(fBounds), 
								0, kShadowWidth);
	}
}

void 
Panel::Close(Boolean slide)
{
	if (!IsVisible())
		return;

	gPageViewer->InvalidateBounds(&fBounds);
	gOptionsPanel->ScrollStatusChanged();
	
	Hide();
	
	if (!slide)
		return;
		
	Boolean	oldShowSelection = gPageViewer->GetShowSelection();
	gPageViewer->SetShowSelection (false);
	DrawBehind();
	gPageViewer->SetShowSelection(oldShowSelection);
	gScroll2Sound->Play();

	gScreen->SlideAreaDown(&fBounds, kPanelScrollRate, RectangleHeight(fBounds), 
								kShadowWidth);

	gPageViewer->RefreshSelection();
	fIsOpen = false;

	// Delete the document, it will be regenerated next time.
	DeleteDocument();
	fIdleState = kViewIdle;
}

Boolean 
Panel::DispatchInput(Input* input)
{
	if (!IsOpen())
		return false;
	
	// Unknown keys cause the panel to close.		
	if (input->data == kScrollUpKey || input->data == kScrollDownKey) {
		Close();
		return true;
	}
		
	return HandlesInput::DispatchInput(input);
}

void 
Panel::Draw(const Rectangle* invalid)
{
	if (!fIsVisible)
		return;	
	
	fShouldPaintBackground = !gPanelBorder->GetDrawCenter();
	if ( gPanelBorder->GetDrawCenter() ) {
		gPageBackColor = gPanelBorder->AverageColor(fDocument->GetBackgroundColor());
		gPanelBorder->Draw(&fBounds, invalid);
		ContentView::Draw(invalid);
	}
	else {
		ContentView::Draw(invalid);
		gPanelBorder->Draw(&fBounds, invalid);
	}

	Rectangle	shadowBounds;
	shadowBounds.top = fBounds.top + kShadowWidth;
	shadowBounds.left = fBounds.right;
	shadowBounds.bottom = fBounds.bottom;
	shadowBounds.right = shadowBounds.left + kShadowWidth;
	PaintRectangle(gScreenDevice, shadowBounds, kBlackColor, 120, invalid);

}

#ifdef FIDO_INTERCEPT
void 
Panel::Draw(class FidoCompatibilityState& fidoCompatibility) const
{
	if (!fIsVisible)
		return;
	
	fShouldPaintBackground = !gPanelBorder->GetDrawCenter();
	if ( gPanelBorder->GetDrawCenter() ) {
		gPageBackColor = gPanelBorder->AverageColor(fDocument->GetBackgroundColor());
		gPanelBorder->Draw(&fBounds, fidoCompatibility);
		ContentView::Draw(fidoCompatibility);
	}
	else {
		ContentView::Draw(fidoCompatibility);
		gPanelBorder->Draw(&fBounds, fidoCompatibility);
	}

	Rectangle	shadowBounds;
	shadowBounds.top = fBounds.top + kShadowWidth;
	shadowBounds.left = fBounds.right;
	shadowBounds.bottom = fBounds.bottom;
	shadowBounds.right = shadowBounds.left + kShadowWidth;
	fidoCompatibility.PaintRectangle(gScreenDevice, shadowBounds, kBlackColor, 120, nil);

}
#endif

Boolean 
Panel::ExecuteInput()
{
	if (!IsOpen())
		return false;
		
	ContentView::ExecuteInput();
	
	return true;
}

void 
Panel::ExecuteURL(const char* url, const char* formData)
{
	char* panelURL = fPageResource.CopyURL("Panel::ExecuteURL");
	
	if (panelURL != nil && EqualString(url, panelURL)) {
		ContentView::ExecuteURL(url, formData);
		FreeTaggedMemory(panelURL, "Panel::ExecuteURL");
		return;
	}
	FreeTaggedMemory(panelURL, "Panel::ExecuteURL");

	if (IsError(gPageViewer==nil))
		return;

	// Copy url because Close deletes the document.
	char* urlCopy = CopyString(url, "Panel::ExecuteURL");
	gPageViewer->ExecuteURL(urlCopy, formData);
	GetSelectionLayer()->DrawActiveNow();
	Close();	

	FreeTaggedMemory(urlCopy, "Panel::ExecuteURL");
}

//const char*
//Panel::CopyURL() const
//{
//	return fResource.CopyURL();
//}
		

Boolean 
Panel::IsOpen() const
{
	return fIsOpen;
}

void 
Panel::Open()
{
	char* copyURL = fPageResource.CopyURL("Panel::Open");
	if (copyURL != nil)
		ExecuteURL(copyURL, nil);
	FreeTaggedMemory((char*)copyURL, "Panel::Open");
}

void 
Panel::SetIsOpen(Boolean value)
{
	fIsOpen = value;
}

void
Panel::SetURL(const char* url)
{
	fPageResource.SetURL(url);
}

void 
Panel::WritePage()
{
}
