// ===========================================================================
// Utilities.c
//
// Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif
#ifndef __PPP_H__
#include "ppp.h"
#endif

#include "BoxUtils.h"
#include "SystemGlobals.h"
#include "DateTime.h"
#include "Clock.h"
#include "Resource.h"

// =============================================================================

Boolean HasAttributes::SetAttribute(const char*, char*)
{
	return false;
}

Boolean HasAttributes::SetAttributeString(char* string)
{
	char* colon;
	char* value;
	Boolean result;
	
	PostulateFinal(false);	// remove this method  --JRM
	
	if (IsWarning(string == nil || *string == 0))
		return false;
	
	if ((colon = FindCharacter(string, ":")) == nil)
		return false;
	
	value = SkipCharacters(colon + 1, " ;");
	if (value == nil || *value == 0)
		return false;
	
	*colon = 0;
	result = SetAttribute(string, value);
	*colon = ':';
	return result;
}

void HasAttributes::WriteAttributes(Stream*)
{
}

// =============================================================================

void HasBounds::GetBounds(Rectangle *bounds) const
{
	*bounds = fBounds;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
Ordinate HasBounds::GetLeft() const
{
	return fBounds.left;
}
#endif

Ordinate HasBounds::GetTop() const
{
	return fBounds.top;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
Ordinate HasBounds::GetRight() const
{
	return fBounds.right;
}
#endif

Ordinate HasBounds::GetBottom() const
{
	return fBounds.bottom;
}

Ordinate HasBounds::GetHeight() const
{
	return fBounds.bottom - fBounds.top;
}

Ordinate HasBounds::GetWidth() const
{
	return fBounds.right - fBounds.left;
}

void HasBounds::SetBounds(const Rectangle* bounds)
{
	fBounds = *bounds;
}

void HasBounds::SetBounds(Ordinate left, Ordinate top, Ordinate right, Ordinate bottom)
{
	Rectangle	bounds = {top, left, bottom, right};
	SetBounds(&bounds);
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void HasBounds::CenterBounds(const Rectangle* inBounds)
{
	::CenterRectangle(fBounds, *inBounds);
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void HasBounds::InsetBounds(Ordinate x, Ordinate y)
{
	::InsetRectangle(fBounds, x, y);
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void HasBounds::OffsetBounds(Ordinate x, Ordinate y)
{
	::OffsetRectangle(fBounds, x, y);
}
#endif

// =============================================================================
// String functions.

char* CatStringTo(char* existing, const char* addition, const char* tag)
{
	// Free the existing string and return a new copy of source concatenated with addition.
	
	if (existing == nil)
		return CopyString(addition, tag);
	
	if (addition == nil || *addition == 0)
		return existing;
	
	char* s = (char*)AllocateTaggedMemory(strlen(existing) + strlen(addition) + 1, tag);
	strcpy(s, existing);
	strcat(s, addition);
	FreeTaggedMemory(existing, tag);
	return s;
}

char* CopyString(const char* source, const char* USED_FOR_MEMORY_TRACKING(tag))
{
	// Return a new copy of the source string.
	
	if (source == nil)
		return nil;
	
	char* s = (char*)AllocateTaggedMemory(strlen(source) + 1, tag);
	strcpy(s, source);
	return s;
}

void CopyStringIntoField(char* destField, const char* string, size_t fieldLength)
{
	if (IsError(destField == nil))
		return;

	if (string == nil || *string == 0) {
		*destField = 0;
		return;
	}

	strncpy(destField, string, fieldLength);
	destField[fieldLength-1] = 0;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
char* CopyStringN(const char* source, ushort count, const char* USED_FOR_MEMORY_TRACKING(tag))
{
	// Return a new copy of the source string.

	if (source == nil || count == 0)
		return nil;
	
	char* s = (char*)AllocateTaggedMemory(count + 1, tag);
	strncpy(s, source, count);
	s[count] = 0;
	return s;
}
#endif

char* CopyStringTo(char* existing, const char* source, const char* tag)
{
	// Free the existing string and return a copy of the source string.
	
	if (existing != nil)
		FreeTaggedMemory(existing, tag);
	return CopyString(source, tag);
}

char* DeletePrefix(char* s, ulong num)
{
	// Delete the first num characters from string, shift everything else down 
	
	if (IsError(strlen(s) < num))
		return s;

	char* dest = s;
	char* src = &(dest[num]);
	while ((*dest++ = *src++) != 0)
		;
	return s;
}

Boolean EqualString(const char* string1, const char* string2)
{
	if (string1 == string2)
		return true;
	
	if (string1 == nil || string2 == nil)
		return false;
	
	for (; *string1 && *string2; string1++, string2++)
		if (tolower(*string1) != tolower(*string2))
			return false;
	
	return *string1 == 0 && *string2 == 0;
}

Boolean EqualStringN(const char* string1, const char* string2, ulong length)
{
	if (string1 == string2)
		return true;
	
	if (string1 == nil || string2 == nil)
		return false;
	
	for (;; string1++, string2++, length--)
		if (length == 0)
			return true;
		else if (*string1 == 0 || *string2 == 0)
			break;
		else if (tolower(*string1) != tolower(*string2))
			return false;
	
	return *string1 == 0 && *string2 == 0;
}

char* FindCharacter(const char* source, const char* target)
{
	// Find target character in source string, ignoring case.
	
	for (; *source; source++)
		if (strchr(target, tolower(*source)))
			return (char*)source;
	
	return nil;
}

char* FindString(const char* source, const char* target, Boolean stopAtNewline)
{
	// Find target string in source string, ignoring case.
	
	const char* s;
	const char* t;
	
	if (IsError(source == nil || target == nil || *target == 0))
		return nil;
	
	for (; *source && !(stopAtNewline && (*source == '\n' || *source == '\r')) ; source++) {
		for (s = source, t = target; *s && *t; s++, t++)
			if (tolower(*s) != tolower(*t))
				goto fail;
		if (*t == 0)
			return (char*)source;
		fail:;
	}
	
	return nil;
}

char* FindStringLast(const char* source, const char* target)
{
	char* p = FindString(source, target);
	
	if (p != nil && strlen(p) > strlen(target))
		p = nil;
		
	return p;
}

const char* GetNextLine(const char *p)
{
	char	ch;
	
	while ((ch = *p++) != 0)
		if (ch == '\n' || ch == '\r')
			return (*p != 0) ? p : nil;
			
	return nil;
}

#ifdef SIMULATOR
ulong GetRandom()
{
	return (((ulong)Random()) << 16) + (ulong)Random();
}
#endif

DataType GuessDataType(const char* name)
{
	const char* lastPartOfURL = name;
	const char* counter = name;
	
	while (*counter != 0)
		if (*counter++ == '/')
			lastPartOfURL = counter;	// lastPartOfURL is pointing one past '/'
	
	counter = lastPartOfURL;
	while (*counter != 0)
		if (*counter++ == '.')
			lastPartOfURL = counter-1;	// lastPartOfURL is at '.'
	
	// NOTE: these should all be checked at the end of the string
	PostulateFinal(false);

	if (strncmp(name, "pnm:", 4) == 0)
		return kDataTypeRealAudioProtocol;
		
	// for image map data
	if (FindStringLast(lastPartOfURL, ".map"))
		return kDataTypeHTML;
	
	if (FindStringLast(lastPartOfURL,".bif"))
		return kDataTypeBorder;
		
	if (FindStringLast(lastPartOfURL, ".img"))
		return kDataTypeBitmap;

	if (FindStringLast(lastPartOfURL,".gif"))
		return kDataTypeGIF;

	if (FindStringLast(lastPartOfURL,".jpg"))
		return kDataTypeJPEG;

	if (FindStringLast(lastPartOfURL,".ani"))
		return kDataTypeAnimation;

	if (FindStringLast(lastPartOfURL,".mid"))
		return kDataTypeMIDI;
		
	if (FindStringLast(lastPartOfURL,".mp2"))
		return kDataTypeMPEGAudio;

	if (FindStringLast(lastPartOfURL,".mpa"))
		return kDataTypeMPEGAudio;

	if (FindStringLast(lastPartOfURL,".m1a"))
		return kDataTypeMPEGAudio;

	if (FindStringLast(lastPartOfURL,".ram"))
		return kDataTypeRealAudioMetafile;

	if (FindString(lastPartOfURL, ".htm"))
		return kDataTypeHTML;

	if (FindString(lastPartOfURL, ".txt"))
		return kDataTypeTEXT;
		
	if (FindString(lastPartOfURL, ".xbm"))
		return kDataTypeXBitMap;
		
	if (FindString(lastPartOfURL, ".fid"))
		return kDataTypeFidoImage;
		

	// <еее> Mick sez...guess we look for index.html in this directory?
	if (*name && !(*lastPartOfURL)) // name ended in "/"...a directory
		return kDataTypeHTML;

	return (DataType)0;
}

void LowerCase(char* str)
{
	// Convert a string to lower case
	
	if (IsError(str == nil))
		return;

	while (*str) {
		*str = tolower(*str);
		str++;
	}
}

// Create a new URL for a local resource from a base resource, and either a prefix, suffix,
// or both.
char* 
NewLocalURL(const Resource* base, const char* prefix, const char* suffix, const char* tag)
{
	if (IsError(base == nil))
		return nil;
		
	// Either the prefix or suffix must be non-nil.
	if (IsError(prefix == nil && suffix == nil))
		return nil;
		
	char* newURL;
	ulong postDataLength = base->GetPostDataLength();
	char* baseURL = base->CopyURL("NewLocalURL");

	newURL = CopyString(prefix, tag);
	newURL = CatStringTo(newURL, baseURL, tag);
	if (postDataLength != 0) {
		ulong	newLength = strlen(newURL) + postDataLength;
		newURL = (char*)ReallocateTaggedMemory(newURL, newLength + 1, tag);
		CopyMemory(base->GetPostData(), &newURL[strlen(newURL)], postDataLength);
		newURL[newLength] = '\0';
	}
	newURL = CatStringTo(newURL, suffix, tag);
	
	FreeTaggedMemory(baseURL, "NewLocalURL");
	
	return newURL;
}

// TruncateStringWithEllipsis copies a string and appends an ellipsis (...) if necessary.
// maxLength is the size of the largest title (including the terminating 0).

char* NewTruncatedStringWithEllipsis(const char* title, XFont font, CharacterEncoding encoding, long maxWidth,
									 const char* tagName)
{
	static	char	ellipsis[] = "...";
	long	ellipsisWidth;
	char*	shortTitle;
	long	titleLength;
	
	if (title == nil)
		return nil;
	
	titleLength = strlen(title);
	shortTitle = CopyString(title, tagName);
	
	if ( (long)TextMeasure(gScreenDevice, font, encoding, title, titleLength) <= maxWidth)
		return shortTitle;
	
	ellipsisWidth = TextMeasure(gScreenDevice, font,encoding, ellipsis, strlen(ellipsis));
	long i;
	long space = -1;
	for (i = 0; i < titleLength && (long)TextMeasure(gScreenDevice, font, encoding, title, i) <
										maxWidth - ellipsisWidth; i++) {
		if (isspace(title[i]))
			space = i;
		if ( IsThreeByte((uchar)title[i],encoding) )
			i += 2;
		else if ( IsTwoByte((uchar)title[i],encoding) )
			i++;
	}
	// Break on space if string won't be too short
	if (space > 5)
		i = space + 2;
	shortTitle[i - 1] = '\0';
	
	// the ellipsis should immediately follow a letter, not a space
	if (isspace(shortTitle[i - 2]))
		shortTitle[i - 2] = '\0';
		
	shortTitle = CatStringTo(shortTitle, ellipsis, tagName);
	
	return shortTitle;
}

ulong ParseAddress(const char* address)
{
	// Convert host addresses of the form a.b.c.d to an unsigned long.
	
	int a, b, c, d;
	
	if (IsError(address == nil || *address == 0))
		return 0;

	if (sscanf(address, "%d.%d.%d.%d", &a, &b, &c, &d) != 4) {
		IsWarning(address);
		return 0;
	}
	
	return (a << 24) | (b << 16) | (c << 8) | d;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
ulong ParsePort(const char* address)
{
	const char* s;
	ushort port;
	
	if (IsError(address == nil || *address == 0))
		return 0;
	
	s = strchr(address, ':');
	if (IsError(s == nil))
		return 0;
		
	if (sscanf(s + 1, "%hu", &port) != 1) {
		IsError(true);
		return 0;
	}
	
	return port;
}
#endif

#ifdef SIMULATOR
void RemoveCharacters(char* s, const char* remove)
{
	if (IsError(s == nil || remove == nil || *remove == 0))
		return;
	
	for (; *s != 0; s++)
		if (strchr(remove, *s) != nil)
			strcpy(s, s+1);
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void RemoveTrailingCharacters(char* s, const char* remove)
{
	if (IsError(s == nil || remove == nil || *remove == 0))
		return;
	
	char* p = s + strlen(s) - 1;
	
	for (; p >= s && strchr(remove, *p) != nil; p--)
		*p = 0;
}
#endif

char* SkipCharacters(const char* s, const char* skip)
{
	if (IsError(s == nil || skip == nil))
		return (char*)s;
		
	while (*s && strchr(skip, *s))
		s++;
	
	return (char*)s;
}

char* SkipString(const char* s, const char* skip)
{
	if (IsWarning(s == nil || skip == nil))
		return (char*)s;
	
	int skipLength = strlen(skip);
	
	if (!EqualStringN(s, skip, skipLength))
		return nil;
	
	return (char*) (s + skipLength);
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
const char* UniqueName(const char* newName)
{
	Complain(("UniqueName should only be used in SIMULATOR builds!!!"));
	return newName;
}
#endif

void UpperCase(char* str)
{
	// Convert a string to upper case

	while (*str) {
		*str = toupper(*str);
		str++;
	}
}

// =============================================================================
// Time functions.

ulong Now()
{
	// Return free running time in ticks.
#ifdef SIMULATOR
	return TickCount();
#else
	return gSystemTicks;
#endif
}

void DelayFor(ulong ticks)
{
	ticks += Now();
	while ((long)(Now() - ticks) < 0)
		TCPIdle(false);
}


// =============================================================================
// Parser accessible functions.

//const char*
//DateFunction()
//{
//	if (!(IsError(gCurrentDateTimeParser == nil) || IsError(gClock == nil))) {
//		gCurrentDateTimeParser->SetDateTime(gClock->GetDateTimeLocal());
//		return gCurrentDateTimeParser->GetDateText();
//	}
//	return kEmptyString;
//}

//const char*
//TimeFunction()
//{
//	if (!(IsError(gCurrentDateTimeParser == nil) || IsError(gClock == nil))) {
//		gCurrentDateTimeParser->SetDateTime(gClock->GetDateTimeLocal());
//		return gCurrentDateTimeParser->GetTimeText();
//	}
//	return kEmptyString;
//}

// =============================================================================
// Qsort routine from Bentley & McIlroy's "Engineering a Sort Function".

static inline char* med3(char *, char *, char *, QuickSortCompare);
static inline void swapfunc(char *, char *, int, int);

#define swapcode(TYPE, parmi, parmj, n) { 	\
	long i = (n) / sizeof (TYPE); 			\
	register TYPE *pi = (TYPE *) (parmi); 	\
	register TYPE *pj = (TYPE *) (parmj); 	\
	do { 									\
		register TYPE	t = *pi;			\
		*pi++ = *pj;						\
		*pj++ = t;							\
        } while (--i > 0);					\
}

#define SWAPINIT(a, es) swaptype = ((char *)a - (char *)0) % sizeof(long) || \
	es % sizeof(long) ? 2 : es == sizeof(long)? 0 : 1;

static inline void swapfunc(char* a, char* b, int n, int swaptype)
{
	if(swaptype <= 1) 
		swapcode(long, a, b, n)
	else
		swapcode(char, a, b, n)
}

#define swap(a, b)						\
	if (swaptype == 0) {				\
		long t = *(long *)(a);			\
		*(long *)(a) = *(long *)(b);	\
		*(long *)(b) = t;				\
	} else								\
		swapfunc(a, b, es, swaptype)

#define vecswap(a, b, n) 	if ((n) > 0) swapfunc(a, b, n, swaptype)

static inline char* med3(char* a, char* b, char* c, QuickSortCompare cmp)
{
	return cmp(a, b) < 0 ?
	       (cmp(b, c) < 0 ? b : (cmp(a, c) < 0 ? c : a ))
              :(cmp(b, c) > 0 ? b : (cmp(a, c) < 0 ? a : c ));
}

void QuickSort(void* a, ulong n, ulong es, QuickSortCompare cmp)
{
	char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
	int r,d, swaptype, swap_cnt;

loop:
	SWAPINIT(a, es);
	swap_cnt = 0;
	if (n < 7) {
		for (pm = (char*)a + es; pm < (char*)a + n * es; pm += es)
			for (pl = pm; pl > (char*)a && cmp(pl - es, pl) > 0;
			     pl -= es)
				swap(pl, pl - es);
		return;
	}
	pm = (char*)a + (n / 2) * es;
	if (n > 7) {
		pl = (char*)a;
		pn = (char*)a + (n - 1) * es;
		if (n > 40) {
			d = (n / 8) * es;
			pl = med3(pl, pl + d, pl + 2 * d, cmp);
			pm = med3(pm - d, pm, pm + d, cmp);
			pn = med3(pn - 2 * d, pn - d, pn, cmp);
		}
		pm = med3(pl, pm, pn, cmp);
	}
	swap((char*)a, pm);
	pa = pb = (char*)a + es;

	pc = pd = (char*)a + (n - 1) * es;
	for (;;) {
		while (pb <= pc && (r = cmp(pb, a)) <= 0) {
			if (r == 0) {
				swap_cnt = 1;
				swap(pa, pb);
				pa += es;
			}
			pb += es;
		}
		while (pb <= pc && (r = cmp(pc, a)) >= 0) {
			if (r == 0) {
				swap_cnt = 1;
				swap(pc, pd);
				pd -= es;
			}
			pc -= es;
		}
		if (pb > pc)
			break;
		swap(pb, pc);
		swap_cnt = 1;
		pb += es;
		pc -= es;
	}
	if (swap_cnt == 0) {  /* Switch to insertion sort */
		for (pm = (char*)a + es; pm < (char*)a + n * es; pm += es)
			for (pl = pm; pl > (char*)a && cmp(pl - es, pl) > 0; 
			     pl -= es)
				swap(pl, pl - es);
		return;
	}

	pn = (char*)a + n * es;
	r = MIN(pa - (char *)a, pb - pa);
	vecswap((char*)a, pb - r, r);
	r = MIN(pd - pc, pn - pd - es);
	vecswap(pb, pn - r, r);
	if ((r = pb - pa) > es)
		QuickSort(a, r / es, es, cmp);
	if ((r = pd - pc) > es) { 
		/* Iterate rather than recurse to save stack space */
		a = pn - r;
		n = r / es;
		goto loop;
	}
}

// =============================================================================
// Base64 conversion

static const char* kBase64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char* NewBase64String(const char* ascii, const char* tag)
{
	
	if (IsError(ascii == nil))
		return nil;
	
	return(NewBase64StringFromBuf((const uchar*)ascii, strlen(ascii), tag));

}

char* NewBase64StringFromBuf(const uchar* data, int count, const char* USED_FOR_DEBUG(tag))
{
	char* base64 = nil;
	long in;
	long out;
	
	if (IsError(data == nil))
		return nil;
	
	base64 = (char*)AllocateTaggedMemory((count + 2) * 4 / 3 + 1, tag);
	
	for (in = 0, out = 0; in < count; in += 3) {
		ulong input = data[in] << 16;
		
		if (in + 1 < count)
			input |= data[in+1] << 8;
		
		if (in + 2 < count)
			input |= data[in+2];
			
		base64[out++] = kBase64[(input >> 18) & 0x3F];
		base64[out++] = kBase64[(input >> 12) & 0x3F];
		base64[out++] = (in + 1 < count) ? kBase64[(input >> 6) & 0x3F] : '=';
		base64[out++] = (in + 2 < count) ? kBase64[input & 0x3F] : '=';
	}
	
	base64[out] = 0;
	return base64;
}

static unsigned char data_ascii2bin[128]={
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xE0,0xF0,0xFF,0xFF,0xF1,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xE0,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0x3E,0xFF,0xF2,0xFF,0x3F,
	0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,
	0x3C,0x3D,0xFF,0xFF,0xFF,0x00,0xFF,0xFF,
	0xFF,0x00,0x01,0x02,0x03,0x04,0x05,0x06,
	0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,
	0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,
	0x17,0x18,0x19,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,
	0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,
	0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,
	0x31,0x32,0x33,0xFF,0xFF,0xFF,0xFF,0xFF,
	};
	
#define conv_ascii2bin(a)	(data_ascii2bin[(a)&0x7f])

uchar* NewFromBase64(const char* base64, int *count, const char* USED_FOR_DEBUG(tag))
{
	uchar *buffer;
	uchar *pointer;
	int eof = 0;
	long i;
	long length;
	
	*count = 0;
	
	if (IsError(base64 == nil))
		return nil;
	
	length = strlen(base64);
	
	if (IsError((length%4) != 0))
		return nil;
		
	/* Find out how many = at the end */
	for (i=1; i<=2; i++) {
		if (base64[length-i] == '=')
			eof++;
	}	
		
	buffer = (uchar*)AllocateTaggedMemory((length / 4) * 3 + 2, tag);
	pointer = buffer;
	
	for (i=0; i<length; i+=4)
		{
		int a,b,c,d;
		unsigned long l;
		
		a=conv_ascii2bin(*(base64++));
		b=conv_ascii2bin(*(base64++));
		c=conv_ascii2bin(*(base64++));
		d=conv_ascii2bin(*(base64++));
		l=(	(unsigned long)(a<<18L)|
			(unsigned long)(b<<12L)|
			(unsigned long)(c<< 6L)|
			(unsigned long)(d     ));
		*(pointer++)=(unsigned char)(l>>16L)&0xff;
		*(pointer++)=(unsigned char)(l>> 8L)&0xff;
		*(pointer++)=(unsigned char)(l     )&0xff;
		*count+=3;
		}
	
	*count -= eof;
	return buffer;
}

// =============================================================================

// =============================================================================
// Crc32

/*
 * Table of CRC-32's of all single-byte values (made by makecrc.c)
 */
static const ulong crc_32_tab[] = {
  0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
  0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
  0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
  0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
  0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
  0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
  0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
  0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
  0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
  0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
  0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
  0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
  0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
  0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
  0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
  0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
  0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
  0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
  0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
  0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
  0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
  0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
  0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
  0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
  0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
  0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
  0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
  0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
  0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
  0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
  0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
  0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
  0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
  0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
  0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
  0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
  0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
  0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
  0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
  0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
  0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
  0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
  0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
  0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
  0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
  0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
  0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
  0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
  0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
  0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
  0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
  0x2d02ef8dL
};


/*
 * Initialize the CRC-32 to a standard value.
 */
ulong
InitCrc32(void)
{
    return (0xffffffffL);
}


/*
 * Run a set of bytes through the crc shift register.  Starting with
 * the crc value passed in, compute a CRC on the data.  Return the
 * updated CRC.
 *
 * The result returned by this routine is the same as the result
 * returned by gzip's routine, except that all the bits are inverted
 * (the final c ^ 0xffffffffL).  This has been verified across
 * multiple invocations of both; the advantage of this routine is
 * that it doesn't have local state.
 */
ulong
UpdateCrc32(ulong crc, const char* s, ulong n)
{
    if (n) do {
        crc = crc_32_tab[((int)crc ^ (*s++)) & 0xff] ^ (crc >> 8);
    } while (--n);

    return crc;
}

// =============================================================================

