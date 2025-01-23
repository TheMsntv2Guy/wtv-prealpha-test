// ===========================================================================
//	Menu.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __MENU_H__
#include "Menu.h"
#endif

#include "boxansi.h"
#include "ContentView.h"
#include "ImageData.h"
#include "MemoryManager.h"
#include "SongData.h"
#if defined FIDO_INTERCEPT
#include "FidoCompatibility.h"
#endif

//===================================================================================

static PackedStyle	gMenuStyle = { 2, 0, 0, 0, 0 };
static const long	kMenuMarginX = 10;
static const long	kMenuMarginY = 5;
static const long	kShadowWidth = 4;
static const ulong	kSearchTimeLimit = kOneSecond / 3;

static const long	kMinListSize = 4;
static const long	kMaxListSize = 8;
static const long	kListMarginX = 10;
static const long	kListMarginY = 5;
static const long	kListSpacingAbove = 1;
static const long 	kListSpacingBelow = 1;

//===================================================================================

MenuLayer::MenuLayer()
{
	IsError(gMenuLayer != nil);
	gMenuLayer = this;

	fBounds.left = 100;
	fBounds.right = 300;
	fBounds.top = 100;
	fBounds.bottom = 300;
	fShouldDrawShadow = true;
}

MenuLayer::~MenuLayer()
{
	IsError(gMenuLayer != this);
	gMenuLayer = nil;
}

Boolean MenuLayer::BackInput()
{
	if (!fIsVisible)
		return false;

	Hide();
	return true;
}

void 
MenuLayer::ComputeBounds()
{
	ContentView* contentView = fDocument->GetView();
	XFont font = fDocument->GetFont(gMenuStyle);
	CharacterEncoding encoding = fDocument->GetCharacterEncoding();
	Ordinate lineHeight = ::GetFontAscent(font,encoding) + ::GetFontLeading(font,encoding) + ::GetFontDescent(font,encoding) + 10;
	ObjectList* itemList = fMenu->GetItems();
	ulong count = itemList->GetCount();
	Rectangle viewBounds;
	ulong availableBelow;
	ulong availableAbove;

	fMenu->GetBounds(&fBounds);
	InsetRectangle(fBounds, 0, 2);
	OffsetRectangle(fBounds, 2, 0);
	contentView->ContentToScreen(&fBounds);
	contentView->GetBounds(&viewBounds);

	// Determine the maximum number of entries available below.
	availableBelow = (viewBounds.bottom - fBounds.top - 3) / (lineHeight - 3);
	availableAbove = (fBounds.bottom - viewBounds.top - 3) / (lineHeight - 3);
	fVisibleCount = count;
	
	// If all entries fit below, pop the menu down.
	if (availableBelow >= count)	
		fBounds.bottom = fBounds.top + (lineHeight-3) * count + 3;	// fit cells together
	
	// If all entries fit above, pop the menu up.
	else if (availableAbove >= count)
		fBounds.top = fBounds.bottom - (lineHeight-3) * count - 3;

	// If all entries fit in space above + below, use all below + additional above.
	else if ((availableAbove + availableBelow - 1) >= count) {
		fBounds.bottom = fBounds.top + (lineHeight-3) * availableBelow;
		fBounds.top = fBounds.top - (lineHeight-3) * (count - availableBelow) - 3;
	}
		
	// If doesn't fit, use all available space, and limit count
	else {
		fBounds.bottom = fBounds.top + (lineHeight-3) * availableBelow;
		fBounds.top = fBounds.top - (lineHeight-3) * (availableAbove - 1) - 3;
		fVisibleCount = availableBelow + availableAbove - 1;
	}
}

Boolean 
MenuLayer::DispatchInput(Input* input)
{
	Boolean handled = Layer::DispatchInput(input);

	// Hide the menu if we receive unhandled input.	
	if (!handled)
		Hide();
			
	return handled;
}

void 
MenuLayer::Draw(const Rectangle* invalid)
{
	ObjectList* itemList;
	XFont font;
	Ordinate leftMargin;
	Ordinate lineHeight;
	Ordinate ascent;
	ulong count;
	ulong i;
	Rectangle selectedBounds;
	Rectangle iconBounds;

	// Paint shadow.	
	if (fShouldDrawShadow) {		// Hack!  Should not know about this!
	
		Rectangle shadowBounds;
		
		// first draw the right shadow
		shadowBounds.top = fBounds.top + kShadowWidth;
		shadowBounds.left = fBounds.right;
		shadowBounds.bottom = fBounds.bottom;
		shadowBounds.right = fBounds.right + kShadowWidth;
		PaintRectangle(gScreenDevice, shadowBounds, kBlackColor, 120, nil);
		
		// then draw the bottom shadow
		shadowBounds.top = fBounds.bottom;
		shadowBounds.left = fBounds.left + kShadowWidth;
		shadowBounds.bottom = fBounds.bottom + kShadowWidth;
		shadowBounds.right = fBounds.right + kShadowWidth;
		PaintRectangle(gScreenDevice, shadowBounds, kBlackColor, 120, nil);
		
		fShouldDrawShadow = false;
	}

	// Paint background.
	
	if ( !gMenuBorder->GetDrawCenter() ) 
		PaintRectangle(gScreenDevice, fBounds, kLightGrayColor, 0, invalid);
	
	if (fMenu == nil || fDocument == nil)
		return;

	// Get size information.		
	font = fDocument->GetFont(gMenuStyle);
	CharacterEncoding encoding = fDocument->GetCharacterEncoding();
	ascent = ::GetFontAscent(font,encoding);
	lineHeight = ascent + ::GetFontLeading(font,encoding) + ::GetFontDescent(font,encoding) + 10;
	leftMargin = fBounds.left + kMenuMarginX;
	Rectangle cellBounds = fBounds;

	// Get Icon size.
	gMenuUpIconImage->GetBounds(&iconBounds);
		
	itemList = fMenu->GetItems();
	count = itemList->GetCount();

	long oldBackColor = gPageBackColor;
	gPageBackColor = kLightGrayColor;	// ееее HACK FOR NOTIFYING XDRAWTEXT OF BackgroundColor ееее

	// Draw each menu item.	
	for (i = fFirstIndex; i < (fFirstIndex + fVisibleCount); i++) {
		MenuItem* item = (MenuItem*)itemList->At(i);
		const char* name = item->fName;
		char* shortName = nil;
		
		cellBounds.bottom = cellBounds.top + lineHeight;

		// The first item is the default selected cell.
		// If another cell is selected, it overrides the first.
		if (i == fSelectedIndex)
			selectedBounds = cellBounds;
			
		gMenuBorder->Draw(&cellBounds, invalid);
			
		ulong availableTextWidth = GetWidth() - kMenuMarginX*2 - RectangleWidth(iconBounds) - 4;
		ulong textWidth = TextMeasure(gScreenDevice, font,encoding, name, strlen(name));
		
		if (textWidth > availableTextWidth) {
			shortName = NewTruncatedStringWithEllipsis(name, font, encoding, availableTextWidth, "MenuLayer::Draw");
			name = shortName;
		}		
		PaintText(gScreenDevice, font, encoding, name, strlen(name), kBlackColor, leftMargin,
								cellBounds.top + ascent + 8, 0, false, invalid);
		if (shortName != nil)
			FreeTaggedMemory(shortName, "MenuLayer::Draw");
								
		cellBounds.top += lineHeight - 3;
	}
	
	gPageBackColor = oldBackColor;

	// Draw the more-up/more-down icons.
	if (fFirstIndex > 0) {
		gMenuUpIconImage->GetBounds(&iconBounds);
		OffsetRectangle(iconBounds, fBounds.right - RectangleWidth(iconBounds) - 8,
						fBounds.top + kMenuMarginY + 3);
		gMenuUpIconImage->Draw(&iconBounds, invalid);
	
	}
	
	if ((count - fFirstIndex) > fVisibleCount) {	
		gMenuDownIconImage->GetBounds(&iconBounds);
		OffsetRectangle(iconBounds, fBounds.right - RectangleWidth(iconBounds) - 8, 
						fBounds.bottom - kMenuMarginY - 3 - RectangleHeight(iconBounds));
		gMenuDownIconImage->Draw(&iconBounds, invalid);
	}
	
	// Draw the selection.
	gSelectionBorder->Draw(&selectedBounds, invalid);

}

#ifdef FIDO_INTERCEPT
void 
MenuLayer::Draw(class FidoCompatibilityState& fidoCompatibility) const
{
	ObjectList* itemList;
	XFont font;
	Ordinate leftMargin;
	Ordinate lineHeight;
	Ordinate ascent;
	ulong count;
	ulong i;
	Rectangle selectedBounds;

	// Paint shadow.	
	{
	
		Rectangle shadowBounds;
		
		// first draw the right shadow
		shadowBounds.top = fBounds.top + kShadowWidth;
		shadowBounds.left = fBounds.right;
		shadowBounds.bottom = fBounds.bottom;
		shadowBounds.right = fBounds.right + kShadowWidth;
		fidoCompatibility.PaintRectangle(gScreenDevice, shadowBounds, kBlackColor, 120, nil);
		
		// then draw the bottom shadow
		shadowBounds.top = fBounds.bottom;
		shadowBounds.left = fBounds.left + kShadowWidth;
		shadowBounds.bottom = fBounds.bottom + kShadowWidth;
		shadowBounds.right = fBounds.right + kShadowWidth;
		fidoCompatibility.PaintRectangle(gScreenDevice, shadowBounds, kBlackColor, 120, nil);
	}

	// Paint background.
	if ( !gButtonBorder->GetDrawCenter() ) 
		fidoCompatibility.PaintRectangle(gScreenDevice, fBounds, kLightGrayColor, 0, nil);
	
	if (fMenu == nil || fDocument == nil)
		return;

	// Get size information.		
	font = fDocument->GetFont(gMenuStyle);
	CharacterEncoding encoding = fDocument->GetCharacterEncoding();
	ascent = ::GetFontAscent(font,encoding);
	lineHeight = ascent + ::GetFontLeading(font,encoding) + ::GetFontDescent(font,encoding) + 10;
	leftMargin = fBounds.left + kMenuMarginX;
	Rectangle cellBounds = fBounds;

	itemList = fMenu->GetItems();
	count = itemList->GetCount();

	// Draw each menu item.	
	for (i = fFirstIndex; i < (fFirstIndex + fVisibleCount); i++) {
		MenuItem* item = (MenuItem*)itemList->At(i);
		char* name = item->fName;
		
		cellBounds.bottom = cellBounds.top + lineHeight;

		// The first item is the default selected cell.
		// If another cell is selected, it overrides the first.
		if (i == fSelectedIndex)
			selectedBounds = cellBounds;
			
		gButtonBorder->Draw(&cellBounds, 0, fidoCompatibility);
			
		fidoCompatibility.PaintText(gScreenDevice, font, name, strlen(name), kBlackColor, leftMargin,
								cellBounds.top + ascent + 8, 0, false, nil);
								
		cellBounds.top += lineHeight - 3;
	}
	
	// Draw the more-up/more-down icons.
	Rectangle iconBounds;
	if (fFirstIndex > 0) {
		gMenuUpIconImage->GetBounds(&iconBounds);
		OffsetRectangle(iconBounds, fBounds.right - RectangleWidth(iconBounds) - 8,
						fBounds.top + kMenuMarginY + 3);
		gMenuUpIconImage->Draw(&iconBounds, 0, fidoCompatibility);
	
	}
	
	if ((count - fFirstIndex) > fVisibleCount) {	
		gMenuDownIconImage->GetBounds(&iconBounds);
		OffsetRectangle(iconBounds, fBounds.right - RectangleWidth(iconBounds) - 8, 
						fBounds.bottom - kMenuMarginY - 3 - RectangleHeight(iconBounds));
		gMenuDownIconImage->Draw(&iconBounds, 0, fidoCompatibility);
	}
	
	// Draw the selection.
	gSelectionBorder->Draw(&selectedBounds, 0, fidoCompatibility);
}
#endif

Boolean 
MenuLayer::DownInput(Input*)
{
	ObjectList* itemList;
	ulong count;
	
	if (!fIsVisible)
		return false;

	itemList = fMenu->GetItems();
	count = itemList->GetCount();

	if (fSelectedIndex < count-1) {
		fSelectedIndex++;
		if ((fSelectedIndex - fFirstIndex) >= fVisibleCount)
			fFirstIndex++;
		InvalidateBounds();
	}
	else
		gLimitSound->Play();
		
	// Reset search.
	fMenu->IncrementalSearch(nil);

	return true;	
}

Boolean 
MenuLayer::ExecuteInput()
{
	ObjectList* itemList;
	ulong count;
	ulong i;

	if (!IsVisible())
		return false;
		
	gPopupSound->Play();

	// Set the new selection.
	itemList = fMenu->GetItems();
	count = itemList->GetCount();

	for (i=0; i < count; i++) {
		MenuItem* item = (MenuItem*)itemList->At(i);

		if (fSelectedIndex == i)
			item->fSelected = true;		
		else 
		if (item->fSelected)
			item->fSelected = false;
	}

	Hide();
	
	return fMenu->Control::ExecuteInput();
}

Boolean 
MenuLayer::KeyboardInput(Input* input)
{
	if (!IsVisible())
		return false;
		
	if (input->data == '\r' || input->data == '\n')
		return ExecuteInput();
	
	IncrementalSearch(input);

	return true;
}

Boolean 
MenuLayer::UpInput(Input*)
{
	if (!IsVisible())
		return false;
	
	if (fSelectedIndex > 0) {
		fSelectedIndex--;
		if (fSelectedIndex < fFirstIndex)
			fFirstIndex--;
		InvalidateBounds();
	}
	else
		gLimitSound->Play();
	
	// Reset search.
	fMenu->IncrementalSearch(nil);

	return true;
}

void 
MenuLayer::Hide()
{
	Rectangle bounds = fBounds;
	
	InsetRectangle(fBounds, -kShadowWidth, -kShadowWidth);
	
	Layer::Hide();
	if (fDocument != nil) {
		ContentView* view = fDocument->GetView();
		view->InvalidateBounds(&bounds);
		view->RefreshSelection();
	}
	fDocument = nil;
}

void 
MenuLayer::IncrementalSearch(Input* input)
{
	if (IsError(fMenu == nil))
		return;
		
	long	newIndex = fMenu->IncrementalSearch(input);
	
 	if (newIndex != -1) {
 		fSelectedIndex = newIndex;
 		if (fSelectedIndex < fFirstIndex)
 			fFirstIndex = newIndex;
 		if (fSelectedIndex >= fFirstIndex + fVisibleCount)
 			fFirstIndex = fSelectedIndex - fVisibleCount + 1;
 		InvalidateBounds();
 	}
	else
		gLimitSound->Play();
}

void 
MenuLayer::SetTarget(Menu* menu, Document* document)
{
	ObjectList* itemList;
	ulong count;
	ulong i;

	// Setup the target menu and document.	
	fMenu = menu;
	fDocument = document;
	
	// Compute the bounds of the popup menu.
	ComputeBounds();
	
	// Set initial selection index.
	itemList = fMenu->GetItems();
	count = itemList->GetCount();

	fSelectedIndex = 0;
	for (i=0; i < count; i++) {
		MenuItem* item = (MenuItem*)itemList->At(i);
		
		if (item->fSelected) {
			fSelectedIndex = i;
			break;
		}
	}
	
	if (fSelectedIndex < fVisibleCount)
		fFirstIndex = 0;
	else
		fFirstIndex = fSelectedIndex - fVisibleCount + 1;
}

void 
MenuLayer::Show()
{
	gPopupSound->Play();
	fShouldDrawShadow = true;
	Layer::Show();
	fDocument->GetView()->RefreshSelection();
}

//===================================================================================

MenuItem::MenuItem()
{
}

MenuItem::~MenuItem()
{
	if (fName != nil)	
		FreeTaggedMemory(fName, "MenuItem::fName");

	if (fValue != nil) 
		FreeTaggedMemory(fValue, "MenuItem::fValue");
}

void 
MenuItem::Set(const char* itemStr, const char* valueStr, Boolean selected)
{
	fName = CopyStringTo(fName, itemStr, "MenuItem::fName");
	fValue = CopyStringTo(fValue, valueStr, "MenuItem::fValue");
	fSelected |= selected;
	
	// Strip trailing spaces from name.
	for (long i = strlen(fName) - 1; i > 0 && isspace(fName[i]); i--)
		fName[i] = '\0';
}

// =============================================================================

Menu::Menu()
{
#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberMenu;
#endif /* DEBUG_CLASSNUMBER */
}

Menu::~Menu()
{
	fItems.DeleteAll();
}

void 
Menu::AddItem(const char* itemString, const char* valueStr, Boolean selected)
// Add an item to the list
{
	MenuItem*	menuItem;
	
	// for exclusive menus, check for a match and just change attributes of the matching item, at most
	if (fExclusive && (menuItem = GetItemFromName(itemString)) != nil) {
		menuItem->fSelected = selected;
		return;
	}
		
	menuItem = new(MenuItem);
	menuItem->Set(itemString, valueStr, selected);
	fItems.Add(menuItem);
}

MenuItem* 
Menu::GetItemFromName(const char* itemString) const
{
	ulong	count = fItems.GetCount();
	
	for (ulong i = 0; i < count; i++) {		
		MenuItem* item = (MenuItem*)fItems.At(i);
		if (EqualString(item->fName, itemString))
			return item;
	}
	
	return nil;
}

ObjectList* 
Menu::GetItems() const
{
	return (ObjectList*)&fItems;
}

ulong
Menu::GetMaxTextWidth(XFont font, CharacterEncoding encoding) const
{
	ulong maxWidth = 0;
	long count = fItems.GetCount();
	
	// find the widest cell.	
	for (long i = 0; i < count; i++) {
		MenuItem* item = (MenuItem*)fItems.At(i);
		const char* name = item->fName;
		ulong width = TextMeasure(gScreenDevice, font,encoding, name, strlen(name));
		if (width > maxWidth)
			maxWidth = width;
	}
	
	return maxWidth;
}

long 
Menu::IncrementalSearch(Input* input) const
{
	static ulong			lastInputTime;
	static char				searchString[64];

	if (input == nil) {
		lastInputTime = 0;
		return -1;
	}
	
	// Reset search string if time limit has expired.
	if (input->time - lastInputTime > kSearchTimeLimit)
		searchString[0] = '\0';
	lastInputTime = input->time;
	
	// Add current character to search string;
	ulong	length = strlen(searchString);
	if (length < sizeof(searchString) - 1) {
		searchString[length++] = input->data;
		searchString[length] = '\0';
	}
	
	// Search for next entry that matches string, wrapping to start
	long		count = fItems.GetCount();
	long		newIndex = -1;
	
	for (long i = 0; i < count && newIndex == -1; i++) {
		MenuItem* item = (MenuItem*)fItems.At(i);
		if (EqualStringN(searchString, item->fName, strlen(searchString)))
			newIndex = i;
 	}
 	
 	return newIndex;
}

void 
Menu::SetAttribute(Attribute attributeID, long value, Boolean isPercentage)
{
	// The set attribute stuff is probably too general
	
	switch (attributeID) {		
		case A_EXCLUSIVE:	fExclusive = true;  break;	// Menu item names are unique
		default:			Control::SetAttribute(attributeID, value, isPercentage); break;
	}
}

//===================================================================================

PopUpMenu::PopUpMenu()
{
#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberPopUpMenu;
#endif /* DEBUG_CLASSNUMBER */
}

PopUpMenu::~PopUpMenu()
{
}

void 
PopUpMenu::AddSubmission(DataStream* stream, Boolean UNUSED(force))
{
	MenuItem*	item = GetSelectedItem();
	if (IsError(item == nil))
		return;
	
	char* v = item->fValue != nil ? item->fValue : item->fName;

	if (fName != nil && v != nil)
		stream->WriteQuery(fName, v);	
}

void 
PopUpMenu::Draw(const Document* document, const Rectangle* invalid)
{
	Rectangle r;
	Rectangle iconBounds;
	
	GetBounds(&r);
	document->GetView()->ContentToScreen(&r);
	
	gMenuIconImage->GetBounds(&iconBounds);

	// look for quick out
	if (invalid != nil && !RectanglesIntersect(&r, invalid))
		return;

	::InsetRectangle(r, 2, 2);

	// Paint background.
	if ( !gMenuBorder->GetDrawCenter() ) 
		PaintRectangle(gScreenDevice, r, kLightGrayColor, 0, invalid);

	long oldBackColor = gPageBackColor;
	gPageBackColor = kLightGrayColor;	// ееее HACK FOR NOTIFYING XDRAWTEXT OF BackgroundColor ееее

	// Draw border.
	gMenuBorder->Draw(&r, invalid);
	
	// Paint text.
	MenuItem*	item = GetSelectedItem();
	if (item != nil && item->fName != nil && *item->fName) {
		const char* name = item->fName;
		char* shortName = nil;
		XFont font = document->GetFont(gMenuStyle);
		CharacterEncoding encoding = document->GetCharacterEncoding();
		ulong ascent = ::GetFontAscent(font,encoding);
		Ordinate x = r.left + kMenuMarginX - 1;
		Ordinate y = r.top + kMenuMarginY + ascent;
		
		ulong availableTextWidth = GetWidth() - kMenuMarginX*2 - RectangleWidth(iconBounds) - 4;
		ulong textWidth = TextMeasure(gScreenDevice, font,encoding, name, strlen(name));
		
		if (textWidth > availableTextWidth) {
			shortName = NewTruncatedStringWithEllipsis(name, font, encoding, availableTextWidth, "PopUpMenu::Draw");
			name = shortName;
		}
		
		::PaintText(gScreenDevice, font ,encoding, name, strlen(name), kBlackColor, x, y, 0, false, invalid);
		
		if (shortName != nil)
			FreeTaggedMemory(shortName, "PopUpMenu::Draw");
	}
	
	// Paint menu icon.	
	OffsetRectangle(iconBounds, r.right - RectangleWidth(iconBounds) - 8, r.top + kMenuMarginY + 2);
	gMenuIconImage->Draw(&iconBounds, invalid);

	gPageBackColor = oldBackColor;
}

Boolean 
PopUpMenu::ExecuteInput()
{
	gMenuLayer->SetTarget(this, fForm->GetDocument());
	gMenuLayer->Show();
	
	return true;
}

MenuItem* 
PopUpMenu::GetSelectedItem() const
{
	ulong	count = fItems.GetCount();
	
	// Get the name of the selected item
	for (ulong i = 0; i < count; i++) {		
		MenuItem* item = (MenuItem*)fItems.At(i);
		if (item->fSelected)
			return item;
	}

	// If nothing is selected, return the first item.
	if (fItems.GetCount() > 0)
		return (MenuItem*)fItems.At(0);
		
	return nil;
}

void 
PopUpMenu::Layout(Document* document, Displayable* parent)
{
	Rectangle iconBounds;
	
	if (fItems.GetCount() == 0)
		return;
		
	gMenuIconImage->GetBounds(&iconBounds);
		
	XFont font = document->GetFont(gMenuStyle);
	CharacterEncoding encoding = document->GetCharacterEncoding();
	ulong ascent = ::GetFontAscent(font,encoding);
	ulong descent = ::GetFontDescent(font,encoding);
	ulong leading = ::GetFontLeading(font,encoding);
	long width;
	
	if (fKnownWidth > 0) {
		if (fIsWidthPercentage)
			width = (fKnownWidth * parent->GetWidth() + 50) / 100;
		else
			width = fKnownWidth;
	}
	else
		width = GetMaxTextWidth(font, encoding) + kMenuMarginX*2 + RectangleWidth(iconBounds) + 4;
	
	SetWidth(MIN(width, ((Page*)parent)->GetDefaultMarginWidth()));
	SetHeight(ascent + descent + (leading + kMenuMarginY) * 2);
}

void 
PopUpMenu::RestoreFillinData(const char* data)
{
	long selected = atol(data);
	
	for (long i = 0; i < fItems.GetCount(); i++) {
		MenuItem* item = (MenuItem*)fItems.At(i);

		if (selected == i)
			item->fSelected = true;		
		else 
			item->fSelected = false;
	}
}

const char* 
PopUpMenu::SaveFillinData() const
{
	
	// Return the index of the selected item
	for (long i = 0; i < fItems.GetCount(); i++) {		
		MenuItem* item = (MenuItem*)fItems.At(i);
		
		if (item->fSelected) {
			static char	buffer[12]; /* 11 for %ld, 1 for NULL */
			snprintf(buffer, sizeof(buffer), "%ld", i);
			return buffer;
		}
	}
		
	return nil;
}

// ==================================================================================

ScrollingList::ScrollingList()
{
#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberScrollingList;
#endif /* DEBUG_CLASSNUMBER */

	fSize = kMinListSize;
}

ScrollingList::~ScrollingList()
{
}

void 
ScrollingList::AddSubmission(DataStream* stream, Boolean UNUSED(force))
{
	if (fName == nil)
		return;		
	
	// Add strings for all selected items.	
	long count = fItems.GetCount();
	for (long i = 0; i < count; i++) {
		MenuItem*	item = (MenuItem*)fItems.At(i);
		if (!IsError(item == nil)) {
			char* v = item->fValue != nil ? item->fValue : item->fName;
	
			if (item->fSelected && v != nil)
				stream->WriteQuery(fName, v);
		}
	}
}

void
ScrollingList::Deselect()
{
	fActive = false;
}

Boolean 
ScrollingList::DownInput()
{
	if (!fActive)
		return false;
			
	long count = fItems.GetCount();

	if (fSelectedIndex < count-1) {
		fSelectedIndex++;
		if ((fSelectedIndex - fFirstIndex) >= fSize)
			fFirstIndex++;
		InvalidateBounds();
	}
	else
		gLimitSound->Play();
		
	// Reset search.
	IncrementalSearch(nil);

	return true;	
}

void 
ScrollingList::Draw(const Document* document, const Rectangle* invalid)
{
	XFont font;
	Ordinate leftMargin;
	Ordinate lineHeight;
	Ordinate ascent;
	Ordinate descent;
	ulong count;
	ulong i;
	Rectangle selectedBounds;
	Rectangle iconBounds;	
	Rectangle	bounds;
	
	GetBounds(&bounds);
	document->GetView()->ContentToScreen(&bounds);

	::InsetRectangle(bounds, 2, 2);
	
	// Paint background. еее Currently using button border
	if (!gMenuBorder->GetDrawCenter())
		PaintRectangle(gScreenDevice, bounds, kLightGrayColor, 0, invalid);

	Color oldBackColor = gPageBackColor;

	// Get size information.		
	font = document->GetFont(gMenuStyle);
	CharacterEncoding encoding = document->GetCharacterEncoding();
	ascent = ::GetFontAscent(font,encoding);
	descent = ::GetFontDescent(font, encoding);
	lineHeight = ascent + descent + kListSpacingAbove + kListSpacingBelow;
	leftMargin = bounds.left + kListMarginX;

	// Get Icon size.
	gMenuUpIconImage->GetBounds(&iconBounds);

	// Draw each menu item.
	Rectangle cellBounds = bounds;
	cellBounds.top += kListMarginY;
	
	count = fItems.GetCount();
	for (i = fFirstIndex; i < (fFirstIndex + fSize) && i < count; i++) {
		MenuItem* item = (MenuItem*)fItems.At(i);
		const char* name = item->fName;
		char* shortName = nil;
		
		cellBounds.bottom = cellBounds.top + lineHeight;

		gPageBackColor = kLightGrayColor;

		if (item->fSelected) {
			PaintRectangle(gScreenDevice, cellBounds, kGrayColor, 0, invalid);
			gPageBackColor = kGrayColor;
		}
			
		// The first item is the default selected cell.
		// If another cell is selected, it overrides the first.
		if (i == fSelectedIndex)
			selectedBounds = cellBounds;
			
		ulong availableTextWidth = GetWidth() - kMenuMarginX*2 - RectangleWidth(iconBounds) - 4;
		ulong textWidth = TextMeasure(gScreenDevice, font,encoding, name, strlen(name));
		
		if (textWidth > availableTextWidth) {
			shortName = NewTruncatedStringWithEllipsis(name, font, encoding, availableTextWidth, "ScrollingList::Draw");
			name = shortName;
		}		
		PaintText(gScreenDevice, font, encoding, name, strlen(name), kBlackColor, leftMargin,
								cellBounds.top + kListSpacingAbove + ascent,  0, false, invalid);								
		if (shortName != nil)
			FreeTaggedMemory(shortName, "ScrollingList::Draw");

		cellBounds.top = cellBounds.bottom;
	}
	
	// Draw border.
	gMenuBorder->Draw(&bounds, invalid);

	gPageBackColor = oldBackColor;

	// Draw the more-up/more-down icons.
	if (fFirstIndex > 0) {
		gMenuUpIconImage->GetBounds(&iconBounds);
		OffsetRectangle(iconBounds, bounds.right - RectangleWidth(iconBounds) - 8,
						bounds.top + kListMarginY + 3);
		gMenuUpIconImage->Draw(&iconBounds, invalid);	
	}
	
	if ((count - fFirstIndex) > fSize) {	
		gMenuDownIconImage->GetBounds(&iconBounds);
		OffsetRectangle(iconBounds, bounds.right - RectangleWidth(iconBounds) - 8, 
						bounds.bottom - kListMarginY - 3 - RectangleHeight(iconBounds));
		gMenuDownIconImage->Draw(&iconBounds, invalid);
	}
	
	// Draw the selection if active.
	if (fActive) {
		// Make bounds tight around text.
		selectedBounds.left += kListMarginX;
		selectedBounds.right -= kListMarginX;
		selectedBounds.top += kListSpacingAbove;
		selectedBounds.bottom -= kListSpacingBelow + descent / 2;
		gSelectionBorder->GetOuterBounds(&selectedBounds);
		gSelectionBorder->Draw(&selectedBounds, invalid);
	}
}

Boolean 
ScrollingList::ExecuteInput()
{
	if (!fActive) {
		fActive = true;
		gPopupSound->Play();
	}
	else if (fMultiple) {
		MenuItem*	item = (MenuItem*)fItems.At(fSelectedIndex);
		if (!IsError(item == nil))
			item->fSelected ^= true;
		gTypeSound->Play();
	}
	else {
		// Deselect every thing but the currently selected item.
		long	count = fItems.GetCount();
		for (long i = 0; i < count; i++) {
			MenuItem*	item = (MenuItem*)fItems.At(i);
			if (!IsError(item == nil))
				item->fSelected = (i == fSelectedIndex);
		}
		gPopupSound->Play();
		fActive = false;
	}
		
	InvalidateBounds();
	
	return true;
}

AttributeValue
ScrollingList::GetAlign() const 
{
	return AV_TOP;
}

void
ScrollingList::InvalidateBounds()
{
	ContentView*	view = GetView();
	
	if (view != nil)
		view->RefreshSelection();
}

Boolean
ScrollingList::IsHighlightable() const
{
	return !fActive && Menu::IsHighlightable();
}

Boolean 
ScrollingList::KeyboardInput(Input* input)
{
	if (input->data == '\r' || input->data == '\n')
		return ExecuteInput();
		
	if (input->data == '\t')
		return Control::KeyboardInput(input);
	
	long	newIndex = IncrementalSearch(input);
	if (newIndex >= 0) {
 		fSelectedIndex = newIndex;
 		if (fSelectedIndex < fFirstIndex)
 			fFirstIndex = newIndex;
 		if (fSelectedIndex >= fFirstIndex + fSize)
 			fFirstIndex = fSelectedIndex - fSize + 1;
 			
 		InvalidateBounds();
 	}
	else
		gLimitSound->Play();

	return true;
}

void 
ScrollingList::Layout(Document* document, Displayable* parent)
{
	Rectangle iconBounds;
	
	if (fItems.GetCount() == 0)
		return;
		
	gMenuIconImage->GetBounds(&iconBounds);
		
	XFont font = document->GetFont(gMenuStyle);
	CharacterEncoding encoding = document->GetCharacterEncoding();
	ulong ascent = ::GetFontAscent(font,encoding);
	ulong descent = ::GetFontDescent(font,encoding);
	ulong leading = ::GetFontLeading(font,encoding);
	long width;
		
	if (fKnownWidth > 0) {
		if (fIsWidthPercentage)
			width = (fKnownWidth * parent->GetWidth() + 50) / 100;
		else
			width = fKnownWidth;
	}
	else
		width = GetMaxTextWidth(font, encoding) + kListMarginX*2 + RectangleWidth(iconBounds) + 4;
	
	SetWidth(MIN(width, ((Page*)parent)->GetDefaultMarginWidth()));
	SetHeight((ascent + descent + kListSpacingAbove + kListSpacingBelow) * fSize + 
			  (leading + kListMarginY) * 2);
}

void 
ScrollingList::RestoreFillinData(const char* data)
{
	long selected = atol(data);
	
	for (long i = 0; i < fItems.GetCount(); i++) {
		MenuItem* item = (MenuItem*)fItems.At(i);

		if (selected == i)
			item->fSelected = true;		
		else 
			item->fSelected = false;
	}
}

const char* 
ScrollingList::SaveFillinData() const
{
	
	// Return the index of the selected item
	for (long i = 0; i < fItems.GetCount(); i++) {		
		MenuItem* item = (MenuItem*)fItems.At(i);
		
		if (item->fSelected) {
			static char	buffer[12]; /* 11 for %ld, 1 for NULL */
			snprintf(buffer, sizeof(buffer), "%ld", i);
			return buffer;
		}
	}
		
	return nil;
}

void
ScrollingList::Select()
{
	fActive = false;
}

void 
ScrollingList::SetAttribute(Attribute attributeID, long value, Boolean isPercentage)
{
	// The set attribute stuff is probably too general
	
	switch (attributeID) {		
		case A_MULTIPLE:	fMultiple = true; 										break;
		case A_SIZE:		fSize = MAX(MIN(value, kMaxListSize), kMinListSize);	break;	
		default:			Menu::SetAttribute(attributeID, value, isPercentage); 	break;
	}
}

Boolean 
ScrollingList::UpInput()
{
	if (!fActive)
		return false;
	
	if (fSelectedIndex > 0) {
		fSelectedIndex--;
		if (fSelectedIndex < fFirstIndex)
			fFirstIndex--;
		InvalidateBounds();
	}
	else
		gLimitSound->Play();
	
	// Reset search.
	IncrementalSearch(nil);

	return true;
}


