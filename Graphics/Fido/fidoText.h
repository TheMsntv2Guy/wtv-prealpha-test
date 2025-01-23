#ifndef _FIDO_TEXT_H_
#define _FIDO_TEXT_H_

struct fontDimensions {
	enum {
		firstAscii = 32,
		lastAscii = 126,
		firstLatin = 160,
		lastLatin = 255,
		noVisibleGlyph = 255,
		iso8859characterCount = lastAscii-firstAscii+1 /* ascii */ +lastLatin-firstLatin+1, /* latin 1 extension */
		glyphPartCount = 125 // !!! needs to be tuned; currently around 125
	};
};

// this table describes how character codes are composited into glyphs
// !!! there may need to be 2 or more of these for different fonts, faces (but not sizes)
extern unsigned char iso8859_1_glyph[2][256]; 
// !!! have font constructor spit out the contents of this table

struct fontTypes {
	enum family {
		proportional,
		monospaced,
		builtInFontCount
	};
	enum face {
		plain,
		italic,
		bold,
		boldItalic =  italic + bold
	};
	// RAM fonts may be other sizes, but ROM fonts are always (and only) available in these sizes
	enum size {
		size0 = 12,
		size1 = 14,
		size2 = 16,
		size3 = 18,
		size4 = 20,
		size5 = 22,
		size6 = 24,
		size7 = 26,
		size8 = 28,
		size9 = 30,
		size10 = 32,
		size11 = 34,
		size12 = 36,
		size13 = 38,
		size14 = 40,
		size15 = 42
	};
};

struct iso8859_1_font {
	unsigned short version; // maybe could be a char
	fontTypes::family family; // maybe could be a char
	fontTypes::face style; // maybe could be a char
	fontTypes::size size;	// could be a char
	unsigned char maxAscent;
	unsigned char maxDescent;
	unsigned short glyphCount; // for built in ROM fonts, this is equal to fontDimensions::glyphPartCount
	unsigned char* glyphs; // indices of glyphs that compose character (normally iso8859_1_glyph)
	unsigned short offsets[fontDimensions::glyphPartCount + 1]; // where glyphs start, as a long offset (4x)
	char kern[fontDimensions::glyphPartCount]; // where to place glyph horizontally relative to baseline
	char ascent[fontDimensions::glyphPartCount]; // where to place glyphs vertically relative to baseline
//	unsigned char width[fontDimensions::glyphPartCount]; // how wide bits are (in row longs)
	unsigned char lines[fontDimensions::glyphPartCount]; // how tall bits are
	const unsigned long *bits; // any length is possible here; a pointer is used to allow (perhaps temporarily)
		// defining test fonts statically
};

// advances could be fractional, but only useful if alternate character with different
// alphas are available.

struct proportional_font {
	const unsigned char fontType;  // always proportional
	// !!! saves a little bit in TextWidth ; is it worth it?
	unsigned char advance[256]; // how far to move to next (every possible byte code is represented here)
	iso8859_1_font font;
};

struct monospaced_font {
	const unsigned char fontType;  // always monospaced
	unsigned char advance;
	iso8859_1_font font;
};
	
union fidoFont {
	proportional_font proportional;	
	monospaced_font monospaced;
};

struct fontDirectory {
	static const fidoFont* HoweverRAMBasedFontsWork(fontTypes::family, fontTypes::face, int size);
	static const proportional_font* proportional[4][16];
	static const monospaced_font* monospaced[4][16];
	static const fidoFont* find(fontTypes::family, fontTypes::face, int size);
};

#endif // _FIDO_TEXT_H_
