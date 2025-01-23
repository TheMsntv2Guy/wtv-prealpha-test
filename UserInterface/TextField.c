// ===========================================================================
//	TextField.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#include "ImageData.h"
#include "Keyboard.h"
#include "MemoryManager.h"
#include "PageViewer.h"
#include "Song.h"
#include "System.h"
#include "TextField.h"
#include "Service.h"


// =============================================================================

static const PackedStyle kFixedTextFieldStyle 			= { 2, 1, 0, 0, 0 };	// Mono font, size 2
static const PackedStyle kProportionalTextFieldStyle 	= { 3, 0, 0, 0, 0 };	// Proportional font, size 3
static const long	kDefaultHMargin = 2;
static const long	kDefaultVMargin	= 0;
static const uchar	kHardBreak 		= '\n';
static const uchar	kSoftBreak 		= '\r';
static const long 	kTextIncrement 	= 128;

// =============================================================================

TextField::TextField()
{
	fKnownColumns = 20;
	fRows = 1;
	fKnownRows = 1;
	fMaxLength = -1;
	fBackgroundColor = kWhiteColor;
	fTextColor = kBlackColor;
	fCursorColor = 0x003333aa;
	fText = nil;
	fAutoCaps = false;
	fAllCaps = false;
	fHasBorder = true;

#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberTextField;
#endif /* DEBUG_CLASSNUMBER */
}

TextField::~TextField()
{
	if (fText != nil)
		FreeTaggedMemory(fText, "TextField::fText");
}

void
TextField::AddDefaultText(const char* text)
{
	fValue = CatStringTo(fValue, text, "Control::fValue");
}

void
TextField::AddText(const char* text)
{
	if (IsError(text == nil))
		return;
		
	MakeSpaceAvailable(strlen(text));
	strncpy(fText, text, fTextLength);
}

void
TextField::Backspace()
{
	uchar character;
	
	if (fCursorPosition == 0)
		return;

	// Skip past soft break.
	if (fText[fCursorPosition-1] == kSoftBreak && fCursorPosition > 1)
		fCursorPosition--;
			
	character = fText[fCursorPosition - 1];
	CopyMemory(fText + fCursorPosition, fText + fCursorPosition - 1,
			   GetLength() + 1 - fCursorPosition);
	fCursorPosition--;
	
	HandleWordWrap();
	
	// If a row was removed, check need for scrolling.
	if (character == kHardBreak) {
		HandleRowChanged();
		if (fGrowable)
			GetView()->RequestLayout(this);
	}

	RefreshAfterInput();
}

void 
TextField::ClearText()
{
	ContentView* view = GetView();
	
	if (fText != nil)
		*fText = '\0';
	fCursorPosition = 0;
	fFirstPosition = 0;

	view->InvalidateBounds();
}

void
TextField::Deselect()
{
	fShouldDrawCursor = false;
	fFirstPosition = 0;
}

void 
TextField::Draw(const Document* document, const Rectangle* invalid)
{
	ContentView* view;
	Rectangle frameBounds;
	BorderImage*	border;
	
	if (IsError(document == nil))
		return;
	
	gSelectionBorder->Idle();

	view = document->GetView();
	GetBounds(&frameBounds);
	view->ContentToScreen(&frameBounds);
	border = GetBorder();
	
	// look for quick out
	if (invalid != nil && !RectanglesIntersect(&frameBounds, invalid))
		return;
	
	// Paint background.
	Rectangle	backgroundBounds = frameBounds;
	if (border != nil) {
		border->GetInnerBounds(&backgroundBounds);
		InsetRectangle(backgroundBounds, -1, -1);
	}
	::PaintRectangle(gScreenDevice, backgroundBounds, fBackgroundColor, 0, invalid);	

	long oldBackColor = gPageBackColor;
	gPageBackColor = fBackgroundColor;	// ¥¥¥¥ HACK FOR NOTIFYING XDRAWTEXT OF BackgroundColor ¥¥¥¥

	// Paint border.
	if (border != nil)
		border->Draw(&frameBounds, invalid);

	// Paint text.
	if (fText != nil && *fText != '\0') {
		XFont		font = GetFont(document);
		CharacterEncoding encoding = document->GetCharacterEncoding();
		Ordinate	x = frameBounds.left + GetLeftSpace();
		ulong		ascent = ::GetFontAscent(font,encoding);
		Ordinate	y = frameBounds.top + GetTopSpace() + ascent;

		// Scroll if there are more characters or rows than can fit within the field.			
		char* visibleText = GetVisibleText();
		if (fRows == 1)
			DrawText(font, encoding, visibleText, strlen(visibleText), x, y, invalid);
		else
			DrawTextInBox(font,encoding,  visibleText, strlen(visibleText), x, y, ascent + ::GetFontLeading(font,encoding) + ::GetFontDescent(font,encoding), invalid);
	}
	
	// Paint text cursor at end of text.
	//
	// NOTE: This will not work if there are two layers with textfields and
	// the software keyboard is used.
	
	if ((gScreen->GetTopLayer() == view || gScreen->GetTopLayer() == gKeyboard) 
								&& fShouldDrawCursor && view->IsSelected(this)) {
		Rectangle cursorBounds;
		GetCursorBounds(view, &cursorBounds);	
		::PaintRectangle(gScreenDevice, cursorBounds, fCursorColor, 0, invalid);
	}
	
	gPageBackColor = oldBackColor;
}

Boolean
TextField::IsShifted() const
{
	return fShift;
}

BorderImage*
TextField::GetBorder() const
{
	if (!fHasBorder)
		return nil;
	
	if (fBorderImage)
		return fBorderImage;

	return gTextFieldBorder;
}

ContentView* 
TextField::GetContainer() const
{
	return GetView();
}

Boolean
TextField::GetExecuteURL() const
{
	return fExecuteURL;
}

long 
TextField::GetLeftSpace() const
{
	long leftSpace = kDefaultHMargin;

	if (GetBorder() != nil)
		leftSpace += GetBorder()->PadBounds().left;
	
	return leftSpace;
}

long 
TextField::GetTopSpace() const
{
	long topSpace = kDefaultVMargin;

	if (GetBorder() != nil)
		topSpace += GetBorder()->PadBounds().top;
	
	return topSpace;
}

long 
TextField::GetRightSpace() const
{
	long rightSpace = kDefaultHMargin;

	if (GetBorder() != nil)
		rightSpace += GetBorder()->PadBounds().right;
	
	return rightSpace;
}

long 
TextField::GetBottomSpace() const
{
	long bottomSpace = kDefaultVMargin;

	if (GetBorder() != nil)
		bottomSpace += GetBorder()->PadBounds().bottom;
	
	return bottomSpace;
}

XFont
TextField::GetFont(const Document* document) const
{
	if (fProportionalFont)
		return document->GetFont(kProportionalTextFieldStyle);
	
	return document->GetFont(kFixedTextFieldStyle);
}

const char*
TextField::GetText() const
{
	return fText;
}

long
TextField::GetRowCount() const
{
	long 	count = 1;
	
	for (long i = 0; fText != nil && fText[i] != '\0'; i++)
		if (fText[i] == kSoftBreak || fText[i] == kHardBreak) 
			count++;
			
	return count;
}

char* 
TextField::GetVisibleText() const
{
	if (fText == nil)
		return nil;
		
	return fText + fFirstPosition;
}

void 
TextField::GetCursorBounds(ContentView* view, Rectangle* cursorBounds) const
{
	Document* document = view->GetDocument();
	
	// NOTE: Move this to GetCharacterWidth method.
	XFont font = GetFont(document);
	CharacterEncoding encoding = document->GetCharacterEncoding();
	ulong lineHeight = ::GetFontAscent(font,encoding) + ::GetFontLeading(font,encoding) + ::GetFontDescent(font,encoding);
	
	GetBounds(cursorBounds);
	view->ContentToScreen(cursorBounds);

	char* buffer = GetVisibleText();
	ulong height = 0;
	ulong lastLineIndex = buffer - fText;

	// If there is more than one row, compute the lastLineIndex and the
	// lineHeight to help in figuring out the cursorBounds...
		
	if (fRows > 1) {
		for (ulong i = buffer - fText; fText != nil && i < fCursorPosition; i++)
			if (fText[i] == kSoftBreak || fText[i] == kHardBreak) {
				lastLineIndex = i + 1;
				height += lineHeight;
			}
	}
	
	// The cursor bounds is determined by the vertical position of the last 
	// line and the horizontal position of the last character. The right
	// and bottom coordinates are dependent on the left and top, respectively...
	
	if (IsError(lastLineIndex > fCursorPosition))
		lastLineIndex = fCursorPosition;
	
	cursorBounds->left += GetLeftSpace() - 1;
	cursorBounds->left += ::TextMeasure(gScreenDevice, font, encoding, &fText[lastLineIndex], fCursorPosition - lastLineIndex);
	cursorBounds->right = cursorBounds->left + 3;
	cursorBounds->top += GetTopSpace() + 2;
	cursorBounds->top += height;
	cursorBounds->bottom = cursorBounds->top + lineHeight - 5;
}

void 
TextField::GetTextBounds(Rectangle* textBounds) const
{
	GetBounds(textBounds);

	// If only one row, text bounds are field bounds.
	if (fRows == 1)
		return;
	
	XFont font = GetFont(fForm->GetDocument());
	CharacterEncoding encoding = fForm->GetDocument()->GetCharacterEncoding();
	ulong lineHeight = ::GetFontAscent(font,encoding) + ::GetFontLeading(font,encoding) + ::GetFontDescent(font,encoding);
	
	// There is more than one row, compute the bottom of the text as the bottom
	// of the text bounds. Add space for one blank line above and below.
	ulong textTop = textBounds->top + GetTopSpace();
	for (ulong i = GetVisibleText() - fText; fText != nil && i < fCursorPosition; i++)
		if (fText[i] == kSoftBreak || fText[i] == kHardBreak)
			textTop += lineHeight;
	
	textBounds->top = MAX(textBounds->top, textTop - lineHeight);		
	textBounds->bottom = MIN(textBounds->bottom, textTop + lineHeight * 2);
}

void
TextField::MakeSpaceAvailable(ulong textCount)
{
	textCount++;	// Leave room for '\0'
	if (fText == nil) {
		fTextLength = MAX(kTextIncrement, textCount);
		fText = (char*)AllocateTaggedMemory(fTextLength, "TextField::fText");
		*fText = '\0';
	} else if (textCount > fTextLength) {
		fTextLength = MAX(fTextLength + kTextIncrement, textCount);
		fText = (char*)ReallocateTaggedMemory(fText, fTextLength, "TextField::fText");
	}
}

void 
TextField::DrawText(XFont font, CharacterEncoding encoding,char* text, ulong textLength, Ordinate x, Ordinate y,
																 const Rectangle* invalid)
{
	Ordinate	textWidthLimit = GetWidth() - GetLeftSpace() - GetRightSpace();
	
	while (::TextMeasure(gScreenDevice, font, encoding, text, textLength) > textWidthLimit)
		textLength--;
		
	::PaintText(gScreenDevice, font, encoding,text, textLength, fTextColor, x, y, 0, false, invalid);
}

void 
TextField::DrawTextInBox(XFont font, CharacterEncoding encoding, char* text, ulong textLength, Ordinate x, Ordinate y, 
												ulong lineHeight, const Rectangle* invalid)
{
	char*		checkText = text;
	char*		textEnd = text + textLength;
	long		row = 0;
	
	for (; checkText < textEnd && row < fRows; checkText++)
		if (*checkText == kSoftBreak || *checkText == kHardBreak) {
			// spit out a line and start to work on next
			DrawText(font, encoding,text, checkText - text, x, y,invalid);
			text = checkText + 1;
			y += lineHeight;
			row++;
		}
	
	// catch the last line, after the last newline	
	DrawText(font,encoding, text, checkText - text, x, y, invalid);
}

void 
TextField::HandleRowChanged()
{
	ContentView*	view = GetView();
	Rectangle		visibleBounds;
	Rectangle		textBounds;
	
	// Do nothing it this text field is not selected.
	if (!view->IsSelected(this))
		return;
					
	view->VisibleContentBounds(visibleBounds);
	GetTextBounds(&textBounds);
	
	// If the text is clipped by bottom of view, SetCurrentSelectableWithScroll
	// Will force the current line to be visible.
	if (textBounds.bottom > visibleBounds.bottom || textBounds.top < visibleBounds.top)
		view->SetCurrentSelectableWithScroll(this);
}

void 
TextField::HandleWordWrap()
{
	ulong length = GetLength();;
	long i, j;
	long softBreakDelta = 0;
	
	if (fText == nil || length == 0 || !fIsTextArea || fForm == nil)
		return;

	// Find start of previous line

	long breakCount = 0;
	for (i = fCursorPosition; i >= 0; i--)
		if (fText[i] == kSoftBreak || fText[i] == kHardBreak)
			if (++breakCount > 2)
				break;
	i++;
			
	// Remove all soft breaks from start of previous line to end of text.

	for (j = i; j < length;) {
		if (fText[j] == kSoftBreak) {
			CopyMemory(fText + j + 1, fText + j, length - j);
			softBreakDelta--;
			length--;
			if (j < fCursorPosition)
				fCursorPosition--;				
		}
		else
			j++;
	}
		
	// Now add soft breaks where necessary.
	
	XFont font = GetFont(fForm->GetDocument());
	Ordinate textWidthLimit = GetWidth() - GetLeftSpace() - GetRightSpace();			
	CharacterEncoding encoding = fForm->GetDocument()->GetCharacterEncoding();
	long lineStart = i;
	long lineLength = 0;
	long space = -1;
	
	for (; i < length; i++) {	
		if (isspace(fText[i]))
			space = i + 1;
		
		lineLength++;
		
		if (fText[i] == kHardBreak) {
			space = -1;
			lineStart = i + 1;
			lineLength = 0;
		}		
		else if (::TextMeasure(gScreenDevice, font, encoding, &fText[lineStart], lineLength) > textWidthLimit) {
			if (isspace(fText[i]))
				i++;
				
			if (space != -1) {
				lineStart = space;
				lineLength = i - space;
				space = -1;
			}
			else {
				lineStart = i;
				lineLength = 0;
			}
			
			MakeSpaceAvailable(length + 1);				
			CopyMemory(fText + lineStart, fText + lineStart + 1, length - lineStart + 1);	
			*(fText + lineStart) = kSoftBreak;
			length++;
			
			if (lineStart <= fCursorPosition)
				fCursorPosition++;
				
			lineStart++;	// Move past soft-break;
				
			softBreakDelta++;
		}	
	}
		
	if (softBreakDelta != 0) {
		HandleRowChanged();
		if (fGrowable)
			GetView()->RequestLayout(this);
	}
}

Boolean 
TextField::ExecuteInput()
{
	gKeyboard->Open();
	RefreshCapsIndicator(true);
	
	return true;
}

Boolean
TextField::HasNumbersFirst() const
{
	return fHasNumbersFirst;
}

Boolean 
TextField::Idle(Layer* layer)
{
	ContentView* view = (ContentView*)layer;
	Rectangle cursorBounds;

	gSelectionBorder->Idle();
	
	if ((gScreen->GetTopLayer() != view && gScreen->GetTopLayer() != gKeyboard) 
			&& !fShouldDrawCursor || !view->IsSelected(this))
		return false;

	// Check last idle time.
	if (fLastIdle != 0 && (Now() - fLastIdle) < 45)
		return false;
	fLastIdle = Now();
	fShouldDrawCursor = !fShouldDrawCursor;

	GetCursorBounds(view, &cursorBounds);
	view->InvalidateBounds(&cursorBounds);

	return true;
}

void
TextField::InsertChar(uchar character)
{
	ulong length = GetLength();

	if (fMaxLength != -1 && length >= fMaxLength)
		return;

	// Leave room for character.
	MakeSpaceAvailable(length + 1);
	
	// WARNING: this will currently insert control characters
	// into the buffer if they are received.
	PostulateFinal(false);

	CopyMemory(fText + fCursorPosition, fText + fCursorPosition + 1, 
			   length + 1 - fCursorPosition);

	// Convert to uppercase if shifted.
			
	if (fShift && islower(character))
		fText[fCursorPosition] = toupper(character);
	else
		fText[fCursorPosition] = character;
		
	SetShifted(false);
	fCursorPosition++;
		
	HandleWordWrap();

	// If a row was added, check need for scrolling.
	if (character == kHardBreak) {
		HandleRowChanged();
		if (fGrowable)
			GetView()->RequestLayout(this);
	}

	RefreshAfterInput();
}

Boolean
TextField::IsSelectable() const
{
	return Control::IsSelectable() || fExecuteURL;
}

Boolean	
TextField::IsTextField() const
{
	return true;
}

Boolean 
TextField::KeyboardInput(Input* input)
{
	uchar character = (uchar)input->data;
	
	// NOTE: this routine should use common character definitions so that
	// this is not compiler/environment dependent.
	PostulateFinal(false);

	switch (character) {
		case '\b':	Backspace();	break;
		case '\t':	input->modifiers & kShiftModifier ? Previous() : Next();	break;
		case '\r':	Return();		break;
		case '\n':	Return();		break;
		
		default:	
			InsertChar(character);
	}
	
	return true;
}

Boolean 
TextField::MoveCursorDown()
{
	if (fCursorPosition <= GetLength() && fText != nil) {
		ulong	xPosition;
		long	i;
			
		// Determine our current X position.
		for (i = fCursorPosition - 1; 
			 i >= 0 && fText[i] != kSoftBreak && fText[i] != kHardBreak; i--)
		;
		
		xPosition = fCursorPosition - i - 1;
		
		// Now find start of next line
		for (i = fCursorPosition;
			 fText[i] != '\0' && fText[i] != kSoftBreak && fText[i] != kHardBreak; i++)
		;
		
		// if we found a next line move as close to the previous xPosition as
		// possible.
		if (fText[i] != '\0') {
			for (fCursorPosition = ++i; 
				 fCursorPosition - i < xPosition && 
				 fText[fCursorPosition] != '\0' && fText[fCursorPosition] != kHardBreak && fText[fCursorPosition+1] != kSoftBreak;
				 fCursorPosition++)
			;
			HandleRowChanged();
			RefreshAfterInput();
			return true;
		}
	}
		
	return false;
}

Boolean
TextField::MoveCursorLeft()
{
	if (fCursorPosition > 0 && fText != nil) {
		fCursorPosition--;
		 if (fText[fCursorPosition] == kSoftBreak || fText[fCursorPosition] == kHardBreak)
			HandleRowChanged();
		if (fText[fCursorPosition] == kSoftBreak && fCursorPosition > 0)
			fCursorPosition--;
			
		RefreshAfterInput();
		return true;
	}
		
	return false;
}

Boolean
TextField::MoveCursorRight()
{
	if (fCursorPosition < GetLength() && fText != nil) {

		if (fText[fCursorPosition] == kHardBreak)
			HandleRowChanged();

		fCursorPosition++;

		if (fText[fCursorPosition] == kSoftBreak) {
			fCursorPosition++;
			HandleRowChanged();
		}
			
		RefreshAfterInput();
		return true;
	}
		
	return false;
}

Boolean
TextField::MoveCursorUp()
{
	if (fCursorPosition > 0) {
		ulong	xPosition;
		long 	i;
				
		// Determine our current X position.
		for (i = fCursorPosition - 1; 
			 i >= 0 && fText[i] != kSoftBreak && fText[i] != kHardBreak; i--)
		;
		
		// if there is a previous line move as close to the previous xPosition as
		// possible.
		if (i >= 0) {			
			xPosition = fCursorPosition - i - 1;
			
			// Now find start of the previous line
			for (i--;
				 i >= 0 && fText[i] != kSoftBreak && fText[i] != kHardBreak; i--)
			;
			
			for (fCursorPosition = ++i; 
				 fCursorPosition - i < xPosition && 
				 fText[fCursorPosition] != '\0' && fText[fCursorPosition] != kHardBreak && fText[fCursorPosition+1] != kSoftBreak;
				 fCursorPosition++)
			;
			HandleRowChanged();
			RefreshAfterInput();
			return true;
		}
	}
		
	return false;
}

void 
TextField::Layout(Document* document, Displayable* parent)
{
	if (IsError(document == nil))
		return;
	
	
	// We determine the width from the columns, always using the fixed character size
	// for width so that we get consistent results across fixed and proportional.
	XFont font = GetFont(document);
	CharacterEncoding encoding = document->GetCharacterEncoding();
	ulong ascent = ::GetFontAscent(font,encoding);
	ulong descent = ::GetFontDescent(font,encoding);
	ulong leading = ::GetFontLeading(font,encoding);
	ulong charWidth = TextMeasure(gScreenDevice, document->GetFont(kFixedTextFieldStyle) ,encoding, "m", 1);
	ulong width;
	
	if (fKnownWidth > 0) {
		if (fIsWidthPercentage)
			width = (fKnownWidth * parent->GetWidth() + 50) / 100;
		else
			width = fKnownWidth;
	}
	else {			
		long columns = MIN(fKnownColumns, (((Page*)parent)->GetMarginWidth() - GetLeftSpace() - GetRightSpace()) / charWidth);
		width = GetLeftSpace() + GetRightSpace() + columns * charWidth;
	}
	
	SetWidth(MIN(width, ((Page*)parent)->GetMarginWidth()));
	
	// Word wrap after width is known.
	HandleWordWrap();
	
	fRows = fGrowable ? MAX(fKnownRows, GetRowCount()) : fKnownRows;
	SetHeight(fRows * (ascent + descent + leading) - leading + GetTopSpace() + GetBottomSpace());
}

void
TextField::MakeActive()
{
	// Make sure that the keyboard is set up properly when going
	// between fields.
	if (gKeyboard->IsVisible())
		RefreshCapsIndicator(true);
}

void
TextField::MoveToNextField()
{
	ContentView* view = GetView();
	TextField* field = fForm->NextTextField(this);
	
	if (!IsError(field == nil))
		view->SetCurrentSelectableWithScroll(field);
}

void
TextField::Next()
{
	MoveToNextControl();
}

void
TextField::Previous()
{
	MoveToPreviousControl();
}

Boolean 
TextField::Reset()
{
	Boolean		result = true;
	
	if (fValue != nil) {
		MakeSpaceAvailable(strlen(fValue));
		
		// Copy characters, converting cr, lf & cr-lf into kHardBreak.
		char*	text = fText;
		if (text != nil) {
			for (long i = 0; fValue[i] != '\0'; i++, text++) {
				switch (fValue[i]) {
					case '\r':
						if (fValue[i+1] == '\n')
							i++;
					case '\n':
						*text = kHardBreak;
						break;
					default:
						*text = fValue[i];
						break;
				}
			}
			*text = '\0';
		}

		fCursorPosition = 0;
		HandleWordWrap();
		UpdateFirstPosition();
	} else {
		result = (fText != nil && *fText != '\0');
		if (fText != nil)
			*fText ='\0';
	}
	
	return result;
}

void 
TextField::RestoreFillinData(const char* data)
{
	if (IsWarning(data == nil))
		return;
		
	MakeSpaceAvailable(strlen(data));
	strcpy(fText, data);
	fCursorPosition = 0;
	HandleWordWrap();
	UpdateFirstPosition();
}

#define kHTTPShortPrefix		"http://"
#define kHTTPLongPrefix			"http://www."
#define kHTTPSuffix				".com"

static Boolean
MarkupURL(char* url)
// prepends "http://" if necessary.
// returns whether or not the string was marked up
// note: be careful about using this on strings that do
// not have enough space to be expanded by 7 bytes.
// NOTE: copy of this in Form::CreateSubmission().
{
	Assert(url != nil);
	
	if (strrchr(url, ':') != nil)
		return false;		/* assume some protocol */
	
	Boolean			longForm = (strrchr(url, '.') == nil);
	const char*		prefix = longForm ? kHTTPLongPrefix : kHTTPShortPrefix;
	CopyMemory(url, url + strlen(prefix), strlen(url) + 1);
	CopyMemory(prefix, url, strlen(prefix));
	if (longForm)
		strcat(url, kHTTPSuffix);
	
	return true;
}

void
TextField::Return()
{
	Boolean keyboardWasVisible = gKeyboard->IsVisible();
	
	// NOTE: does fRows == 0 ever mean something, such as a growable field?
	
	// Force hard break for multi-line textfields.
	if (fIsTextArea) {
		InsertChar(kHardBreak);
		return;
	}

	// Software keyboard only stays visible for Return within multi-line textfields.
	if (keyboardWasVisible)
		gKeyboard->Close();
		
	if (fForm->GetTextFieldCount() > 1) {

#ifdef DISABLE_RETURN
		// The error sound tells the user that Return has no effect on the 
		// hardware keyboard.
		if (!keyboardWasVisible)
			gErrorSound->Play();
#else
		// Return wraps between text fields.			
		MoveToNextField();
#endif

		return;
	} 
	
	// Show active selection feedback.
	ContentView* view = GetView();
	view->GetSelectionLayer()->DrawActiveNow();
	fShouldDrawCursor = false;
	
	// Treat return key as execute for forms with only one single-line field.
	if (fExecuteURL && fText != nil) {
		MakeSpaceAvailable(strlen(fText) + sizeof(kHTTPLongPrefix) + sizeof(kHTTPSuffix));
		if (MarkupURL(fText)) {
			InvalidateBounds(true);
		}
#ifndef DEBUG
		if (gServiceList->HasService(fForm->GetDocument()->GetBaseResource(), fText))
#endif	
		{
			SetLastExecuteURL(fText);
			view->ExecuteURL(fText, nil);
		}
		return;
	}
	
	if (fAction != nil) {
		Control::ExecuteInput();
		return;
	}
	
	if (IsError(fForm == nil))
		return;
	
	fForm->CreateSubmission(this);
	fForm->Submit();
}

const char* 
TextField::SaveFillinData() const
{
	if (fText == nil)
		return nil;

	if (fValue == nil || strcmp(fValue, fText) != 0)
		return fText;

	return nil;
}

void
TextField::Select()
{
	fCursorPosition = GetLength();
	fLastIdle = Now();
	fShouldDrawCursor = true;
	UpdateFirstPosition();
	HandleRowChanged();
}

void
TextField::SelectFirst()
{
	if (fShowKeyboard && !gSystem->GetUsingHardKeyboard()) {
		gKeyboard->Open();
		RefreshCapsIndicator(true);
	}
}

void 
TextField::SetShifted(Boolean value)
{
	fShift = value;

	// NOTE: This can be optimized to draw less.	
	if (gKeyboard->IsVisible())
		gKeyboard->InvalidateBounds();
}

void 
TextField::SetAttribute(Attribute attributeID, long value, Boolean isPercentage)
{
	switch (attributeID) {		
		case A_BORDER:		
			if (value == -1 || value > 0)
				fHasBorder = true;
			else
				fHasBorder = false;
			break;
			
		case A_BGCOLOR:		fBackgroundColor = value; break;
		case A_TEXT:		fTextColor = value; break;
		case A_CURSOR:		fCursorColor = value; break;
		case A_MAXLENGTH:	fMaxLength = value; break;
		case A_SIZE:
		case A_COLS:		fKnownColumns = value; break;
		case A_ROWS:		fKnownRows = value; break;
		case A_FONT:		if ((AttributeValue)value == AV_PROPORTIONAL) fProportionalFont = true;	break;
		default:			Control::SetAttribute(attributeID, value, isPercentage);
	}
}

void 
TextField::SetAttributeStr(Attribute attributeID, const char* value)
{
	Postulate(sizeof(kHTTPShortPrefix) == 8);
	
	switch (attributeID) {
		case A_ALLCAPS:
			fAllCaps = true;
			break;	
		case A_AUTOCAPS:	
			fAutoCaps = true; 
			break;
		case A_BORDERIMAGE:
			if (value != nil && *value != '\0') {
				fBorderImage = BorderImage::NewBorderImage(value);
				fHasBorder = true;
			}
			break;
		case A_EXECUTEURL:
			fExecuteURL = true;
			if (fValue == nil || *fValue == '\0') {
				Control::SetAttributeStr(A_VALUE, kHTTPShortPrefix);
				MakeSpaceAvailable(sizeof(kHTTPShortPrefix) - 1);
				strcpy(fText, kHTTPShortPrefix);
			}
			break;
		case A_NOSOFTBREAKS:
			fNoSoftBreaks = true;
			break;
		case A_GROWABLE:
			fGrowable = true;
			break;
		case A_NUMBERS:
			fHasNumbersFirst = true;
			break;
		case A_SHOWKEYBOARD:
			fShowKeyboard = true;
			break;
		case A_VALUE:
			if (value && *value) {
				Control::SetAttributeStr(attributeID, value);
				MakeSpaceAvailable(strlen(value));
				strcpy(fText, fValue);
			}
			break;
		default:
			Control::SetAttributeStr(attributeID, value);
	}
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void 
TextField::SetExecuteURL(Boolean value)
{
	fExecuteURL = value;
}
#endif

void 
TextField::AddSubmission(DataStream* stream, Boolean UNUSED(force))
{
	long	textLength = (fText != nil ? strlen(fText) : 0);
	long 	bufferLength = textLength + 1;
	long 	i;
	 		
	// Add room for expanding line-breaks into cr-lf.
	for (i = 0; i < textLength; i++)
		if (fText[i] == kSoftBreak || fText[i] == kHardBreak)
			bufferLength++;
	
	char* s = (char*)AllocateTaggedMemory(bufferLength, "TextField::AddSubmission");
	*s = '\0';
	
	char* s1 = s;
	for (i = 0; i < textLength; i++)
		switch(fText[i])
		{
			case kSoftBreak:	if (fNoSoftBreaks)			// If soft breaks allowed, fall through 
									break;
			case kHardBreak:	*s1++ = '\r'; *s1++ = '\n'; break;
			default:			*s1++ = fText[i];			break;
		}
	*s1 = '\0';
	
	stream->WriteQuery(fName, s);
	
	FreeTaggedMemory(s, "TextField::AddSubmission");
}

void
TextField::InvalidateBounds(Boolean drawNow)
{
	// update just me
	Rectangle bounds;
	GetBounds(&bounds);

	// Only invalidate our background size so that the document
	// background does not have to draw.
	if (GetBorder() != nil) {
		GetBorder()->GetInnerBounds(&bounds);
		InsetRectangle(bounds, -1, -1);
	}
	
	ContentView* view = GetView();
	view->ContentToScreen(&bounds);
	view->InvalidateBounds(&bounds);
	if (drawNow)
		gScreen->RedrawNow();
}

void
TextField::RefreshAfterInput()
{
	// Turn cursor and delay idle drawing on while typing.
	fShouldDrawCursor = true;
	fLastIdle = Now() + 1;

	InvalidateBounds(false);
	RefreshCapsIndicator();
	UpdateFirstPosition();
}

void
TextField::RefreshCapsIndicator(Boolean firstTime)
{
	// Handle AllCaps.
	if (fAllCaps && (gKeyboard->IsVisible() || firstTime)) {
		
		// Capitalize all letters.		
		SetShifted(true);
		return;
	}
	
	// Handle AutoCaps.
	if (fAutoCaps && (gKeyboard->IsVisible() || firstTime)) {
		ulong length = GetLength();
	
		// Capitalize the first character and any character that has a
		// space preceeding it.		
		if ((length == 0 && !fHasNumbersFirst) || (length > 1 &&  fText[length-1] == ' ')) {
			SetShifted(true);
		}
	}
}
	
void					
TextField::UpdateFirstPosition()
{
	if (fText == nil || fForm == nil)
		return;
		
	// Update the first position if necessary to keep cursor in view.

	if (!fIsTextArea) {
		if (fCursorPosition < fFirstPosition)
			fFirstPosition = fCursorPosition;
		else {
			XFont		font = GetFont(fForm->GetDocument());
			Ordinate	textWidthLimit = GetWidth() - GetLeftSpace() - GetRightSpace();			
			CharacterEncoding encoding = fForm->GetDocument()->GetCharacterEncoding();
			while (::TextMeasure(gScreenDevice, font, encoding, GetVisibleText(), fCursorPosition - fFirstPosition) > textWidthLimit)
				fFirstPosition++;
		}
	}
	else {
		// If cursor is before first, position first at last break before cursor;
		if (fCursorPosition < fFirstPosition) {
			for (fFirstPosition = fCursorPosition; fFirstPosition > 0; fFirstPosition--)
				if (fText[fFirstPosition - 1] == kSoftBreak || fText[fFirstPosition - 1] == kHardBreak) {
					break;
				}
		}
		// If the cursor is beyond the end of the field, position cursor at bottom.
		else if (fCursorPosition > 0 && !fGrowable) {
			long	rows = fRows;
			long	i;
			
			for (i = fCursorPosition - 1; i >= fFirstPosition; i--)
				if (fText[i] == kSoftBreak || fText[i] == kHardBreak) {
					if (--rows == 0) {
						fFirstPosition = i + 1;
						break;
					}
				}
		}
	}
	
	if (IsError(fFirstPosition > fCursorPosition))
		fFirstPosition = fCursorPosition;
}

// =============================================================================

InputPassword::InputPassword()
{
}

InputPassword::~InputPassword()
{
}

void 
InputPassword::DrawText(XFont font, CharacterEncoding encoding, char* UNUSED(text), ulong textLength, Ordinate x, Ordinate y, const Rectangle* invalid)
{
	char	buffer[256];
	ulong	i;
	
	textLength = MIN(textLength, sizeof(buffer)-1);
	for (i = 0; i < textLength; i++)
		buffer[i] = '\xad';
	buffer[i] = '\0';
	
	TextField::DrawText(font,encoding, buffer, textLength, x, y, invalid);
}

// =============================================================================
