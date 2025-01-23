/* ===========================================================================
	Utilities.h

	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
=========================================================================== */

#ifndef __UTILITIES_H__
#define __UTILITIES_H__

#ifndef NO_C_PLUS_PLUS

#ifndef __GRAPHICS_H__
#include "Graphics.h"
#endif

#endif	 /* NO_C_PLUS_PLUS */

class Resource;

/* ======================================================================== */
/* Useful macros. */
#define ABS(_x)				((_x) < 0 ? -(_x) : (_x))
#define IsPowerOfTwo(n)		(((n) & ((n) - 1)) == 0)
#define IsMultipleOf(n,m)	((n)/(m)*(m) == (n))
#define IsValidPtr(p)		((p) != nil && ((ulong)(p) & 0x1) == 0)
#define MIN(_x,_y)			(((_x) < (_y)) ? (_x) : (_y))
#define MAX(_x,_y)			(((_x) > (_y)) ? (_x) : (_y))
#define ZeroStruct(ptr)		{ Assert(sizeof(*ptr) > 4); memset(ptr, 0, sizeof(*ptr)); }

/* String functions. */
typedef const char*	(*StringFunction)();

#define kStringTag	"C String"

#ifndef MEMORY_TRACKING
	#define	CatStringTo(existing, addition, no_tag)		CatStringTo(existing, addition)
	#define	CopyString(source, no_tag)					CopyString(source)
	#define	CopyStringN(source, count, no_tag)			CopyStringN(source, count)
	#define	CopyStringTo(existing, source, no_tag)		CopyStringTo(existing, source)
	#define NewTruncatedStringWithEllipsis(title, font, encoding,maxWidth, no_tag)	NewTruncatedStringWithEllipsis(title, font, encoding,maxWidth)
	#define NewLocalURL(base, prefix, suffix, no_tag)	NewLocalURL(base, prefix, suffix)
#endif

char* CatStringTo(char* existing, const char* addition, const char* tag = kStringTag);
char* CopyString(const char* source, const char* tag = kStringTag);
char* CopyStringN(const char* source, ushort count, const char* tag = kStringTag);
char* CopyStringTo(char* existing, const char* source, const char* tag = kStringTag);
//const char* DateFunction(void);
char* DeletePrefix(char* s, size_t num);
Boolean EqualString(const char* string1, const char* string2);
Boolean EqualStringN(const char* string1, const char* string2, ulong length);
char* FindCharacter(const char* source, const char* target);
char* FindString(const char* source, const char* target, Boolean stopAtNewline = false);
char* FindStringLast(const char* source, const char* target);
const char* GetNextLine(const char *p);
DataType GuessDataType(const char* name);
void LowerCase(char*);
char* NewLocalURL(const Resource* base, const char* prefix, const char* suffix, const char* tag = kStringTag);
char* NewTruncatedStringWithEllipsis(const char* title, XFont font,CharacterEncoding encoding, long maxWidth, const char* tag = kStringTag);
ulong ParseAddress(const char*);
ulong ParsePort(const char*);
void RemoveCharacters(char* s, const char* remove);
void RemoveTrailingCharacters(char* s, const char* remove);
char* SkipCharacters(const char* s, const char* skip);
char* SkipString(const char* s, const char* skip);
//const char* TimeFunction(void);
#ifdef SIMULATOR
const char* UniqueName(const char* newName);
#endif
void UpperCase(char*);

void CopyStringIntoField(char* destField, const char* string, size_t fieldLength);
#define CopyStringIntoCharArray(destArray, source) \
	CopyStringIntoField(destArray, source, sizeof(destArray))

/* Time constants/conversions. */

const ulong kOneSecond = 60;
#define TicksPerMillisecond(m)		((m) * (kOneSecond/20) / 50)
#define MillsecondsPerTick(t)		((t) * 50 / (kOneSecond/20))
ulong Now();
void DelayFor(ulong ticks);

/* Sorting */

typedef int (*QuickSortCompare)(const void*, const void*);
void QuickSort(void* base, ulong count, ulong size, QuickSortCompare compare);

/* Base64 conversion */

char* NewBase64String(const char* ascii, const char* tag = kStringTag);
char* NewBase64StringFromBuf(const uchar* data, int count, const char* tag = kStringTag);
uchar* NewFromBase64(const char* base64, int *count, const char* tag = kStringTag);

/* Crc32 */

ulong InitCrc32();
ulong UpdateCrc32(ulong crc, const char* data, ulong length);

/* ======================================================================== */

#ifndef NO_C_PLUS_PLUS

class Stream;

/* ======================================================================== */

class HasAttributes {
public:
	virtual Boolean				SetAttribute(const char* name, char* value);
	Boolean						SetAttributeString(char* nameColonAttribute);
	virtual void				WriteAttributes(Stream*);
};

/* ======================================================================== */

class HasBounds {
public:
	virtual void			GetBounds(Rectangle* bounds) const;
	
	Ordinate				GetHeight() const;
	Ordinate				GetWidth() const;

	Ordinate				GetLeft() const;
	Ordinate				GetTop() const;
	Ordinate				GetBottom() const;
	Ordinate				GetRight() const;
	
	virtual void			SetBounds(const Rectangle* bounds);
	void					SetBounds(Ordinate left, Ordinate top, Ordinate right, Ordinate bottom);

	void					CenterBounds(const Rectangle* inBounds);
	void					InsetBounds(Ordinate x, Ordinate y);
	void					OffsetBounds(Ordinate x, Ordinate y);

protected:
	Rectangle				fBounds;
};

/* ======================================================================== */

class HasDebugModifiedTime {
#ifdef SIMULATOR
public:
				HasDebugModifiedTime(void)			{ fDebugModifiedTime = Now(); };
	void		SetDebugModifiedTime(void) 			{ fDebugModifiedTime = Now(); };
	void		SetDebugModifiedTime(ulong time)	{ fDebugModifiedTime = time; };
	ulong		GetDebugModifiedTime(void) const	{ return fDebugModifiedTime; };
protected:
	ulong		fDebugModifiedTime;
#else
public:
	void		SetDebugModifiedTime(void) 		{  }
	void		SetDebugModifiedTime(ulong)		{  }
	ulong		GetDebugModifiedTime(void)		{ return 0; };
#endif
};

#endif	 /* NO_C_PLUS_PLUS */

/* ======================================================================== */

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Utilities.h multiple times"
	#endif
#endif /* __UTILITIES_H__ */
