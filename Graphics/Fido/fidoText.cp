#include "fido.h"
#include "fidoSim.h"

// find 4 bit already-built clut, or build one
const codebook4BitRamp* yMapBuilder::findClut(const codebook& match)
{
	Assert(this);
	CheapAssert(match.type == codebook::yType);
	if (clutList == nil || clutPointer >= &clutList->block[clutBlockList::max])
		grow(clutList);
	Assert(clutPointer >= clutList->block);
	Assert(clutPointer < &clutList->block[clutBlockList::max]);
	const codebook4BitRamp* last = clutPointer;
	clutBlockList* block = clutList;
	codebook4BitRamp* check = clutList->block;	// first
	while (last > check && check->equal(match) == false) {
		if (++check >= last) {
			if ((block = block->next) == nil)
				break;
			check = block->block;
			last = &check[clutBlockList::max];
		}
	}
	if (last <= check) {
		check = clutPointer;
		check->construct(match);
		clutPointer++;
	}
	return check;
}


signed16dot16 displayLeaf::measure(const char* charStream,  int length)
{
	Assert(this);
	if (length == 0)
		length = strlen(charStream);
	if (font->monospaced.fontType == fontTypes::monospaced)
		return ff(font->monospaced.advance * length);
	int width = 0;
	const unsigned char* advancePtr = font->proportional.advance;
	do {
		width += advancePtr[(unsigned char) *charStream++];
	} while (--length > 0);
	return ff(width);
}


struct glyphState {
	int yPosition;
	int minY;
	int maxY;
	const fidoRectangle* clip;
	const iso8859_1_font* isoFont;
};

// given a line of text, generate the appropriates cels and add them to the yMaps
void yMapBuilder::draw(const char* charStream,  int length, const fidoFont* font,
	codebook& textColor, signed16dot16 xPosition, signed16dot16 yPosition, const fidoRectangle& clip)
{
	Assert(this);
// given a list of text characters, build a y list
	const codebook4BitRamp* textClutPtr = findClut(textColor);
	bool deleteTextClut = firstClut(textClutPtr);
	glyphState state;
	state.clip = &clip;
	bool mono = font->monospaced.fontType == fontTypes::monospaced;
	int advance;
	const unsigned char* advancePtr;
	if (mono) {
		advance = font->monospaced.advance;
		state.isoFont = &font->monospaced.font;
	} else {
		advancePtr = font->proportional.advance;
		state.isoFont = &font->proportional.font;
	}
	state.yPosition = s16dot16::round(yPosition);
	state.minY = state.yPosition - state.isoFont->maxAscent;
	if (clip.bottom <= state.minY)
		return;
	state.maxY = state.yPosition + state.isoFont->maxDescent;
	if (clip.top >= state.maxY)
		return;
	bool writeYEntry = true;
	if (textClutPtr != lastClut.clutPointer || lastClut.top > state.minY || lastClut.bottom < state.maxY) {
		lastClut.clutPointer = textClutPtr;
		lastClut.top = state.minY;
		lastClut.bottom = state.maxY;
		celRecord* cel = celPool->nextMicro();
		cel->mode = celRecord::loadCodeBook;
		cel->set((texture*) textClutPtr->ramp);
		DebugCode(cel->set_xLeftStart(originX));	// to satisfy truncation bit checks
		DebugCode(cel->set_xRightStart(originX + s10dot10::one));
		cel->topOffset = 0;
		cel->bottomOffset = 0;
		add(cel, state.minY, state.maxY);
		writeYEntry = false;
	}
	celRecord* cel;
	int intX = s16dot16::round(xPosition);
	fidoRectangle bitBounds;
	bitBounds.left = ff(intX);
	bitBounds.top = ff(state.minY);
	bitBounds.bottom = ff(state.maxY);
	do {
		int charIndex = (unsigned char) *charStream++;
		int index = iso8859_1_glyph[0][charIndex];
		int width = mono ? advance : advancePtr[charIndex];
		if (index != fontDimensions::noVisibleGlyph) {
		// !!! we don't really have enough info to know pixel width
			cel = addCelGlyph(state, index, intX, width);
			if (writeYEntry != false) {
				writeYEntry = false;
				add(cel, state.minY, state.maxY);
			}
			index = iso8859_1_glyph[1][charIndex];
			if (index != fontDimensions::noVisibleGlyph)
				cel = addCelGlyph(state, index, intX, width);
		}
		intX += width;
	} while (--length > 0);
	cel->setLastCelBit();
	bounds.right = ff(intX);
	bounds.onion(bitBounds);
}


celRecord* yMapBuilder::addCelGlyph(texture* base, unsigned short bitsOffset, int rowLongs,
	int xPosition, int topOffset, int bottomOffset)
{
	Assert(this);
	celRecord* cel = celPool->nextMicro();
	cel->mode = (celRecord::celTypes) (cel->vq4MicroRLE | cel->bgAlpha1Minus);
	cel->topOffset = topOffset;
	cel->set_textureRowLongsMicro(rowLongs - 1);
	cel->set_xLeftStartMini(s16dot16::roundTo10dot10(ff(xPosition) + originX));
	cel->bottomOffset = bottomOffset;
	cel->set((texture*) (&base->forAggregateInitializer + bitsOffset));
	return cel;
}

// !!! this can only discard entire characters on the left; to partially discard a character, it would have to be rendered into a
// non run-length-encoded bitmap somewhere else
celRecord* yMapBuilder::addCelGlyph(glyphState& state, int index, int xPosition, int width)
{
	Assert(this);
	texture* base = (texture*) state.isoFont->bits;
	unsigned short nextOffset = state.isoFont->offsets[index + 1];
	int bitsOffset = state.isoFont->offsets[index];
	int height = state.isoFont->lines[index];
	int rowLongs = (nextOffset - bitsOffset) / height;
	int top = state.yPosition - state.isoFont->ascent[index];
	int bottom = top + height;
	if (top < state.minY) {
		bitsOffset += (state.minY - top) * rowLongs;
		top = state.minY;
	}
	if (bottom > state.maxY)
		bottom = state.maxY;
	int intLeft = xPosition - state.isoFont->kern[index];
	signed16dot16 left = ff(intLeft);
	signed16dot16 right = ff(xPosition + width);
	int topOffset = top - state.minY;
	bitsOffset -= topOffset * rowLongs;
	int bottomOffset = state.maxY - bottom + 1;
	if (left >= state.clip->left && right <= state.clip->right)
		return addCelGlyph(base, bitsOffset, rowLongs, intLeft, topOffset, bottomOffset);
	if (left >= state.clip->right)
		return nil;
	if (right <= state.clip->left)
		return nil;
	if (left < state.clip->left)
		Assert(0);	// !!! render cel as non-runlength encoded, then add mini-cel
		// must expand char to regular cel first
	if (right > state.clip->right)
		right = state.clip->right;
	celRecord* cel = celPool->nextMini();
	cel->mode = (celRecord::celTypes) (cel->vq4MiniRLE | cel->bgAlpha1Minus);
	cel->set((texture*) (&base->forAggregateInitializer + bitsOffset));
	cel->topOffset = topOffset;
	cel->set_xLeftStartMini(s16dot16::roundTo10dot10(left + originX));
	cel->bottomOffset = bottomOffset;
	cel->set_textureRowLongs(rowLongs - 1);
	cel->globalAlpha = 0xFF;
	// a hack since we can't really clip on the right when scaled
	cel->set_xRightStartMini(s16dot16::roundTo10dot10(right + originX));
	cel->set_dux(s8dot8::one);
	cel->uMask = -1;
	cel->vMask = -1;
	cel->set((codebook*) nil);
	return cel;
}


void yMapBuilder::draw(const char* ,  int , const fidoFont* ,
	codebook& , const fidoMapping& , const fidoRectangle& )
{
	Assert(0);
	// create offscreen with entire line of text, then draw it as a bitmap
	// it would be great if this could cache the offscreen and use it again with a different mapping
	// this means that a structure would have to keep track of charStream, length, font, textColor
}
