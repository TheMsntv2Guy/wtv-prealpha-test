#ifndef _YLIST_H_
#include "yList.h"
#endif

#ifndef _YLIST_CPP_
#define _YLIST_CPP_

inline yEntry::hint celRecord::get(celRecord::celTypes mode)
{
	int celType = mode & celTypeMask;
	Assert(celType != reserved);	// E
				// 9 						2, 3, 6, 7, A, B, F
	return celType == specialCelType ? yEntry::micro : (celType & miniCelType) != 0 ? yEntry::mini :
				// C == C, D
		   (celType & microCelType) == microCelType ? yEntry::micro : yEntry::maxi;
}

inline yEntry::hint celRecord::get() const
{
	return get((celTypes) mode);
}


inline yEntry::yEntry()
{
}

inline yEntry::yEntry(int bottom, int top, hint size, unsigned int index)
{
	Assert(top < bottom);
	Assert(top >= -512);	// !!! use constants?
	Assert(bottom <= 511);
	Assert(size >= micro);
	Assert(size <= maxi);
	Assert(index < 1 << 10);
	full = (bottom << 22) | ((top & 0x3FF) << 12) | (size << 10) | index;
}

inline void yEntry::get(int& bottom, int& top, hint& size, unsigned int& index) const
{
	top = bits.top;
	bottom = bits.bottom;
	size = (hint) bits.size;
	index = bits.index;
}

inline void yEntry::swap()
{
	full ^= -1;	// flip y top and y bottom to disable the entry; reflip to enable
}

inline bool yEntry::terminate() const
{
	return (full & 0xFFF00000) == 0xFFF00000;
}

inline yEntryContainer::yEntryContainer(int s)	: size(s), lastCelBase(nil)
{
	end = map = AllocateArray(yEntry, s + 2);	// 1 extra slot to add cel terminator onto already drawn map
}

inline yEntryContainer::~yEntryContainer()
{
	delete(map);
}

#if 0
inline void yEntryContainer::addLast(int index)
{
	Assert(this);
	Assert(map < end);
	end->full = end[-1].full;
	end->bits.index = index;
	end++;
	// caller must check to see if map must be grown
}
#endif

// !!! these need to be restructured to support returning correct offsets
inline celRecord* celRecord::nextMini()
{
	return &this[1];
}

inline celRecord* celRecord::nextMicro()
{
	return &this[1];
}

inline void celRecord::set(const texture* basePtr)
{
#if USE_BIT_FIELDS
	unsigned long base = (unsigned long) basePtr;
	Assert((base & 0xFC000003) == 0);
	textureBase = base >> 2;
#else
	textureBase = basePtr;
#endif
}

inline void celRecord::set(const codebook* basePtr)
{
#if USE_BIT_FIELDS
	unsigned long base = (unsigned long) basePtr;
	Assert((base & 0xFC000003) == 0);
	codebookBase = base >> 2;
#else
	codebookBase = basePtr;
#endif
}

// !!! could make conversion a table lookup based on old and new sizes
inline void celRecord::convert(yEntry::hint size)
{
	yEntry::hint oldSize = get();
	DebugCode(int type = mode & celTypeMask);
	Assert(type != specialCelType && type != reserved);
	if (size == yEntry::micro) {
		if (oldSize == yEntry::mini) {
			Assert(type == vq4MiniYUAV || type == vq4MiniRLE);
			mode += 0x20;
		} else if (oldSize == yEntry::maxi) {
			Assert(type == vq4FullYUAV);
			mode +=  0x80;
		}
	} else if (size == yEntry::mini) {
		if (oldSize == yEntry::micro) {
			mode -=  0x20;
		} else if (oldSize == yEntry::maxi) {
			mode +=  0x20;
		}
	} else if (size == yEntry::maxi) {
		if (oldSize == yEntry::micro) {
			Assert(type == vq4MicroYUAV);
			mode -=  0x80;
		} else if (oldSize == yEntry::mini) {
			Assert(type != vq4MiniRLE && type != vq8MiniRLE);
			mode -=  0x20;
		}
	}
	Assert(get() == size);
}

inline const texture* celRecord::get(const texture*& basePtr) const
{
#if USE_BIT_FIELDS
	return basePtr = (const texture*) (textureBase << 2);
#else
	return basePtr = textureBase;
#endif
}

inline const codebook* celRecord::get(const codebook*& basePtr) const
{
#if USE_BIT_FIELDS
	return basePtr = (const codebook*) (codebookBase << 2);
#else
	return basePtr = codebookBase;
#endif
}

inline int celRecord::rowBytes() const
{
#if USE_BIT_FIELDS
	return ((get() == yEntry::micro ?  0 : textureRowLongsHigh << 4) | textureRowLongsLow) + 1 << 2;
#else
	return textureRowLongs + 1 << 2;
#endif
}

inline unsigned short celRecord::get_textureRowLongs() const
{
#if USE_BIT_FIELDS
	unsigned short textureRowLongs = textureRowLongsLow;
	if (get() >= yEntry::mini)
		textureRowLongs |= textureRowLongsHigh << 4;
#endif
	return textureRowLongs;
}

// !!! the assert is disallowed because the cel may be micro intended in the unscaled bitmap case
inline signed8dot8 celRecord::get_dux() const
{
//	Assert(get() != yEntry::micro);
#if USE_BIT_FIELDS
	signed8dot8 dux = duxdvRowAdjust << 4;
	if (get() == yEntry::maxi) {
		dux |= duxMSN << 12;
		dux |= duxLSN;
	}
#endif
	return dux;
}

inline signed10dot10 celRecord::get_xLeftStart() const
{
#if USE_BIT_FIELDS
	signed10dot10 result = xLeftStart << s10dot10::shift;
	if (get() == yEntry::maxi)
		result |= xLeftStartFrac;
	return result;
#else
	return xLeftStart;
#endif
}

inline signed10dot10 celRecord::get_xRightStart() const
{
#if USE_BIT_FIELDS
	signed10dot10 result = xRightStart << s10dot10::shift;
	if (get() == yEntry::maxi)
		result |= xRightStartFrac;
	return result;
#else
	return xRightStart;
#endif
}

// could add a micro version without the test
inline void celRecord::set_textureRowLongs(unsigned short rowLongs)
{
	Assert(rowLongs < 1 << 12);
#if USE_BIT_FIELDS
	textureRowLongsLow = rowLongs;
	textureRowLongsHigh = rowLongs >> 4;
#else
	textureRowLongs = rowLongs;
#endif
}

inline void celRecord::set_textureRowLongsMicro(unsigned short rowLongs)
{
	Assert(rowLongs < 1 << 4);
#if USE_BIT_FIELDS
	textureRowLongsLow = rowLongs;
#else
	textureRowLongs = rowLongs;
#endif
}

// !!! the assert is disallowed because the cel may be micro intended in the unscaled bitmap case
inline void celRecord::set_dux(signed8dot8 num)
{
//	Assert(get() == yEntry::maxi);
#if USE_BIT_FIELDS
	duxMSN = num >> 12;
	duxdvRowAdjust = num >> 4;
	duxLSN = num;
#else
	dux = num;
#endif
}

inline void celRecord::set_duxMini(signed8dot8 num)
{
	Assert(get() == yEntry::mini);
	Assert((num & 0xF000) == 0);
#if USE_BIT_FIELDS
	duxdvRowAdjust = num >> 4;
#else
	dux = num;
#endif
}

// !!! the assert is disallowed because the cel may be micro intended in the unscaled bitmap case
inline void celRecord::set_xLeftStart(signed10dot10 num)
{
//	Assert(get() == yEntry::maxi);
#if USE_BIT_FIELDS
	xLeftStart = num >> s10dot10::shift;
	xLeftStartFrac = num & s10dot10::one - 1;
#else
	xLeftStart = num;
#endif
}

inline void celRecord::set_xLeftStartMini(signed10dot10 num)
{
	Assert(get() <= yEntry::mini);
#if USE_BIT_FIELDS
	xLeftStart = num >> s10dot10::shift;
#else
	xLeftStart = num >> s10dot10::shift << s10dot10::shift;
#endif
}

inline void celRecord::set_xRightStart(signed10dot10 num)
{
//	Assert(get() == yEntry::maxi);
#if USE_BIT_FIELDS
	xRightStart = num >> s10dot10::shift;
	xRightStartFrac = num & s10dot10::one - 1;
#else
	xRightStart = num;
#endif
}

inline void celRecord::set_xRightStartMini(signed10dot10 num)
{
	Assert(get() == yEntry::mini);
#if USE_BIT_FIELDS
	xRightStart = num >> s10dot10::shift;
#else
	xRightStart = num >> s10dot10::shift << s10dot10::shift;
#endif
}

inline void celRecord::clearLastCelBit()
{
	mode = (enum celTypes) ((unsigned char) mode & ~lastCel);
}

inline void celRecord::setLastCelBit()
{
	mode = (enum celTypes) ((unsigned char) mode | lastCel);
}

inline bool celRecord::testLastCelBit() const
{
	return (mode & lastCel) != 0;
}

inline int celRecord::indexSize(yEntry::hint /* size */) const
{
	// !!! eventually,this should be:
#if 0
	Assert(size != yEntry::reserved);
	return size == yEntry::micro ? 1 : size == yEntry::mini ? 2 : 6;
#endif
	return sizeof(celRecord) >> 2;	
}

inline int celRecord::indexSize() const
{
	return indexSize(get());
}

// !!! this should eventually compile into simple long stuffs
// when I wrote this, the cel structure wasn't in place to attempt this
inline void celRecord::set(const yEntry* newBase) 
{
	mode = loadYMapBase;
	topOffset = 0; 
#if USE_BIT_FIELDS
	reserved0 = 0;
	textureRowLongsLow = 0;
#else
	textureRowLongs = 0;
#endif
	xLeftStart = 0;
	bottomOffset = 0; 
	set((const texture*) newBase);
}

// !!! this should eventually compile into simple long stuffs
// when I wrote this, the cel structure wasn't in place to attempt this
inline void celRecord::set(const celRecord* newBase) 
{
	mode = loadCelsBaseLast;
	topOffset = 0; 
#if USE_BIT_FIELDS
	reserved0 = 0;
	textureRowLongsLow = 0;
#else
	textureRowLongs = 0;
#endif
	xLeftStart = 0;
	bottomOffset = 0; 
	set((const texture*) newBase);
}

inline void celArrayPool::construct()
{
// !!! reserve extra space for two out-of-room switching cels
// for now we cheat and reserve a full size cel, wasting 32 bytes
	 last = cels = newArray(FidoCelRecord, size + 1);
	Assert(cels);
}

inline void celArrayPool::destroy()
{
	while (list) {
		celArrayPool* next = list->list;
		delete(list);
		list = next;
	}
	delete(cels);
}

inline celArrayPool::celArrayPool(yEntryContainer& yMap, int min) :
	size(min), list(nil), map(yMap)
{
//	DebugCode(switchCelBase = nil);
	construct();
}

inline void celArrayPool::init()
{
	destroy();
	construct();
}

// returns 8 byte index
inline int celArrayPool::getIndex() 
{
	return (char*) last - (char*) cels >> 2;
}

inline int celArrayPool::getIndex(celRecord* cel) 
{
	return (char*) cel - (char*) cels >> 2;
}

inline int celArrayPool::count() 
{
	return (char*) last - (char*) cels >> 2;
}


inline void yMapBuilder::destroy()
{
	while (clutList) {
		clutBlockList* next = clutList->next;
		delete(clutList);
		clutList = next;
	}
	while (fillList) {
		fillBlockList* next = fillList->next;
		delete(fillList);
		fillList = next;
	}
}

// !!! could only construct these blocks when used
inline void yMapBuilder::construct()
{
	if (fidoLimits::visibleLeft == 0)
		fidoLimits::setup(fidoLimits::ntsc);
	originX = ff(fidoLimits::visibleLeft);
	originY = fidoLimits::visibleTop;
	bounds.set(ff(fidoLimits::maxX), ff(fidoLimits::maxY), ff(fidoLimits::minX), ff(fidoLimits::minY));
}

inline yMapBuilder::yMapBuilder(celArrayPool* cels) :
	celPool(cels), clutList(nil), fillList(nil)
{
	lastClut.clutPointer = nil;
	construct();
}

inline yMapBuilder::~yMapBuilder()
{
	destroy();
}

inline void yMapBuilder::grow(clutBlockList* oldList)
{
	clutList = new(FidoClutBlockList);
	Assert(clutList);
	clutList->next = oldList;
	clutPointer = clutList->block;
	clutList->blockZeroReferenced = false;
}

inline void yMapBuilder::grow(fillBlockList* oldList)
{
	fillList = new(FidoFillBlockList);
	Assert(fillList);
	fillList->next = oldList;
	fillPointer = fillList->block;
	fillList->blockZeroReferenced = false;
}

inline bool yMapBuilder::firstClut(const codebook4BitRamp* textClutPtr)
{
	if (textClutPtr == clutList->block && clutPointer == &clutList->block[1] &&
		clutList->blockZeroReferenced == false)
	{
		clutList->blockZeroReferenced = true;
		return true;
	} else
		return false;
}

inline bool yMapBuilder::firstFill(const codebook* fillColorPtr)
{
	if (fillColorPtr == fillList->block && fillPointer == &fillList->block[1] &&
		fillList->blockZeroReferenced == false)
	{
		fillList->blockZeroReferenced = true;
		return true;
	} else
		return false;
}

inline void yMapBuilder::init()
{
	destroy();
	construct();
}


inline yEntry* yMapBuilder::terminate()
{
	Assert(this);
	Assert(celPool);
	yEntry* terminal = celPool->map.end;
	terminal->full = yEntry::termination;
	celPool->map.end++;
	return terminal;
}

#endif // _YLIST_CPP_
