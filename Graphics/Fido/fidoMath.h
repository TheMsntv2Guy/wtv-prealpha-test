#ifndef _FIDOMATH_H_
#define _FIDOMATH_H_

#define BUILD_SINE_TABLE 1	// set to 1 to build tables
#define ROUND_MULTIPLY 0
#define DEBUG_WITH_CLASSES 0

struct fidoPoint;

typedef char bool;
#if DEBUG_WITH_CLASSES
	#define signed8dot8 s8dot8
	#define unsigned8dot8 u8dot8
	#define signed10dot10 s10dot10
	#define unsigned0dot16 u0dot16
	#define signed0dot16 s0dot16
	#define signed16dot16 s16dot16
	#define signed2dot30 s2dot30
#else
	typedef long signed2dot30;			// for public interfaces to small numbers
	typedef long signed10dot10;			// for xLeft, xRight
	typedef unsigned short unsigned0dot16;	// for w
	typedef short signed0dot16;			// for dwx and dwRowAdjust
	typedef short signed8dot8;
	typedef unsigned short unsigned8dot8;	// internal to chip, result of reciprocal
	typedef short signed0dot16;
	typedef unsigned short unsigned0dot16;
	typedef long signed16dot16;			// for public interface to fixed numbers
#endif

struct u0dot16 {
#if DEBUG_WITH_CLASSES
	unsigned short value;
	int operator> (const u0dot16 operand) const
		{ return value > operand.value; }
	int operator>= (const u0dot16 operand) const
		{ return value >= operand.value; }
	int operator< (const u0dot16 operand) const
		{ return value < operand.value; }
	int operator<= (const u0dot16 operand) const
		{ return value <= operand.value; }
	int operator== (const u0dot16 operand) const
		{ return value == operand.value; }
	int operator!= (const u0dot16 operand) const
		{ return value != operand.value; }
	u0dot16 operator& (const u0dot16 operand) const
		{ u0dot16 f; f.value = value & operand.value; return f; }
	u0dot16 operator>> (int operand) const
		{ u0dot16 f; f.value = value >> operand; return f;  }
	u0dot16 operator<< (int operand) const
		{ u0dot16 f; f.value = value << operand; return f;  }
	u0dot16& operator<<= (int operand)
		{ value <<= operand; return *this; }
	static u0dot16 fromInt(int operand)
		{ u0dot16 f; f.value = operand; return f; }
	static int asInt(u0dot16 operand)
		{ return operand.value; }
	static struct s16dot16& to16dot16(u0dot16 operand);
	static struct s2dot30& to2dot30(u0dot16 operand);
#else
	static unsigned0dot16 fromInt(int operand)
		{ return operand; }
	static int asInt(unsigned0dot16 operand)
		{ return operand; }
	static signed16dot16 to16dot16(unsigned0dot16 operand);
	static signed16dot16 to2dot30(unsigned0dot16 operand);
#endif
	enum { shift = 16 };
	static const unsigned0dot16 error; // maximum allowable error for simple calculations
	static const unsigned0dot16 min;
	static const unsigned0dot16 max;
	static const unsigned0dot16 one;
	static const unsigned0dot16 oneHalf;
	static const unsigned0dot16 divideBottom;
#if BUILD_SINE_TABLE
	static void buildSineTable(void);
	static void buildAtanTable(void);
#endif
};
	
struct s0dot16 {
#if DEBUG_WITH_CLASSES
	short value;
	static s0dot16 fromInt(int operand)
		{ s0dot16 f; f.value = operand; return f; }
	static int asInt(s0dot16 operand)
		{ return operand.value; }
	static s2dot30 as2dot30(s0dot16 operand)
		{ s2dot30 f; f.value = operand; return f; }
#else
	static signed0dot16 fromInt(int operand)
		{ return operand; }
	static int asInt(signed0dot16 operand)
		{ return operand; }
	static signed2dot30 as2dot30(signed2dot30 operand)
		{ return operand; }
#endif
	enum { shift = 16 };
	static const signed0dot16 oneHalf;
	static const signed0dot16 min;
	static const signed0dot16 max;
};

struct s8dot8 {
#if DEBUG_WITH_CLASSES
	short value;
	static s8dot8 as8dot8(struct s16dot16& f);
	static s16dot16& as16dot16(struct s8dot8 f);
	static int asInt(s8dot8 f)
		{ return f.value; }
	static s8dot8 fromInt(int operand)
		{ s8dot8 f; f.value = operand; return f; }
#else
	static signed16dot16 as16dot16(signed8dot8 f);
	static signed8dot8 as8dot8(signed16dot16 f)
		{ return f; }
	static signed8dot8 fromInt(int f)
		{ return f; }
	static int asInt(signed8dot8 f)
		{ return f; }
#endif
	enum { shift = 8 };
	static const signed8dot8 one;
	static const signed8dot8 min;	// for overflow checks
	static const signed8dot8 max;
	static signed8dot8 multiply(signed8dot8 a, signed8dot8 b);
//	static signed8dot8 divide2dot14(signed8dot8 a, signed2dot14 b);
	static signed8dot8 divide0dot16(signed8dot8 a, unsigned0dot16 b);
	static signed8dot8 multiplyDivide(signed8dot8 multiplicand, signed8dot8 multiplier, signed8dot8 divisor);
};

struct u8dot8 {
#if DEBUG_WITH_CLASSES
	unsigned short value;
	static u8dot8 fromInt(int operand)
		{ u8dot8 f; f.value = operand; return f; }
#else
	static unsigned8dot8 fromInt(int operand)
		{ return operand; }
#endif
	enum { shift = 8 };
	static const unsigned8dot8 one;
	static const unsigned8dot8 min;	// for overflow checks
	static const unsigned8dot8 max;
};

struct s10dot10 {
#if DEBUG_WITH_CLASSES
	short value;
	static s16dot16& as16dot16(struct s10dot10 f);
	static int asInt(s10dot10 f)
		{ return f.value; }
	static s10dot10 fromInt(int operand)
		{ s10dot10 f; f.value = operand; return f; }
#else
	static signed16dot16 as16dot16(signed10dot10 f);
	static signed10dot10 fromInt(int f)
		{ return f; }
	static int asInt(signed10dot10 f)
		{ return f; }
#endif
	enum { shift = 10 };
	static const signed10dot10 one;
	static const signed10dot10 min;	// for overflow checks
	static const signed10dot10 max;
	static int roundDown(signed10dot10 a);
	static int roundUp(signed10dot10 a);
};

struct s2dot30 {
#if DEBUG_WITH_CLASSES
	long value;
	int operator> (const s2dot30 operand) const
		{ return value > operand.value; }
	int operator>= (const s2dot30 operand) const
		{ return value >= operand.value; }
	int operator< (const s2dot30 operand) const
		{ return value < operand.value; }
	int operator<= (const s2dot30 operand) const
		{ return value <= operand.value; }
	int operator== (const s2dot30 operand) const
		{ return value == operand.value; }
	int operator!= (const s2dot30 operand) const
		{ return value != operand.value; }
	static struct s16dot16& to16dot16(s2dot30 operand);
	static int asInt(s2dot30 f)
		{ return f.value; }
	static s2dot30 fromInt(int operand)
		{ s2dot30 f; f.value = operand; return f; }
#else
	static signed16dot16 to16dot16(signed2dot30 f);
	static int asInt(signed2dot30 f)
		{ return f; }
	static signed2dot30 fromInt(int operand)
		{ return operand; }
#endif
	enum { shift = 30 };
	static const signed2dot30	one;
	static const signed2dot30	error;
	static const signed2dot30	max;
	static unsigned0dot16 roundToU0dot16(signed2dot30 a);
	static signed0dot16 roundToS0dot16(signed2dot30 a);
	static unsigned0dot16 checkToU0dot16(signed2dot30 a);
	static signed0dot16 checkToS0dot16(signed2dot30 a);
	static signed2dot30 multiply(signed2dot30 a, signed2dot30 b);
	static signed2dot30 divide(signed2dot30 dividend, signed2dot30 divisor);
};

struct s16dot16 {
#if DEBUG_WITH_CLASSES
	long value;
	int operator> (const s16dot16 operand) const
		{ return value > operand.value; }
	int operator>= (const s16dot16 operand) const
		{ return value >= operand.value; }
	int operator< (const s16dot16 operand) const
		{ return value < operand.value; }
	int operator<= (const s16dot16 operand) const
		{ return value <= operand.value; }
	int operator== (const s16dot16 operand) const
		{ return value == operand.value; }
	int operator!= (const s16dot16 operand) const
		{ return value != operand.value; }
	s16dot16 operator+ (const s16dot16 operand) const
		{ s16dot16 f; f.value = value + operand.value; return f; }
	s16dot16 operator- (const s16dot16 operand) const
		{ s16dot16 f; f.value = value - operand.value; return f; }
	s16dot16 operator& (const s16dot16 operand) const
		{ s16dot16 f; f.value = value & operand.value; return f; }
	int operator>> (const int operand) const
		{ return value >> operand; }
	int operator<< (const int operand) const
		{ return value << operand; }
	s16dot16 operator- () const
		{ s16dot16 f; f.value = -value; return f; }
	static int asInt(s16dot16 f)
		{ return f.value; }
	static s16dot16 fromInt(int operand)
		{ s16dot16 f; f.value = operand; return f; }
#else
	static int asInt(signed16dot16 f)
		{ return f; }
	static signed16dot16 fromInt(int operand)
		{ return operand; }
#endif
	enum { shift = 16 };
	static const signed16dot16 one;
	static const signed16dot16 min;
	static const signed16dot16 max;
	static signed8dot8 roundTo8dot8(signed16dot16 a);
	static signed10dot10 roundTo10dot10(signed16dot16 a);
	static signed8dot8 roundTo2dot14(signed16dot16 a);
	static int round(signed16dot16 a);
	static signed16dot16 roundDown(signed16dot16 a);
	static signed16dot16 roundUp(signed16dot16 a);
	static signed16dot16 divide(signed16dot16 dividend, signed16dot16 divisor);
	static signed16dot16 multiply(signed16dot16 a, signed16dot16 b);
	static signed16dot16 atan(signed16dot16 a, signed16dot16 b);
	static signed8dot8 checkTo8dot8(signed16dot16 a);
	static signed10dot10 checkTo10dot10(signed16dot16 a);
	static signed8dot8 checkTo2dot14(signed16dot16 a);
	static unsigned0dot16 checkTo0dot16(signed16dot16 a);
	static signed16dot16 root(signed16dot16 x, signed16dot16 y);
	static signed16dot16 multiplyDivide(fidoPoint& pt1, fidoPoint& pt2, signed16dot16 divisor);
	static const signed16dot16	angle360;
	static const signed16dot16	angle270;
	static const signed16dot16	angle180;
	static const signed16dot16	angle90;
	static const signed16dot16	angle45;
	static const signed16dot16	angle90Mask; // is angle90 - 1
#ifdef DEBUG
	static const signed16dot16	error;
	static const signed16dot16	errorAtan2;
#endif
	static signed2dot30 sine(signed16dot16 angle);
	static signed2dot30 cosine(signed16dot16 angle);
};

inline signed16dot16 ff(int a)
{
	return s16dot16::fromInt(a << s16dot16::shift);
}

inline signed2dot30 fr(int a)
{
	return s2dot30::fromInt(a << s2dot30::shift);
}

inline signed16dot16 angle(int a)
{
	return s16dot16::fromInt((a << s16dot16::shift) / 360);
}

// !!! should this contain a type (identify, translate, scale, etc.) to specify the map and invert functions?
// also, add inverse so it can be carried around ?
struct fidoMapping {
	union {
		struct {	// this originally had 16 bit fields; this was not enough, especially for translate x and y
			signed16dot16 scaleX;	signed16dot16 skewY;		signed2dot30 u;
			signed16dot16 skewX;	signed16dot16 scaleY;		signed2dot30 v;
			signed16dot16 moveX;	signed16dot16 moveY;		signed2dot30 w;
		} name;
		long array[3][3];
	};
	fidoMapping& setIdentity();
	fidoMapping& setTranslate(signed16dot16 aboutX, signed16dot16 aboutY);
	fidoMapping& setScale(signed16dot16 scaleX, signed16dot16 scaleY, signed16dot16 aboutX, signed16dot16 aboutY);
	fidoMapping& setRotate(signed16dot16 angle, signed16dot16 aboutX, signed16dot16 aboutY);
	fidoMapping& normalize(fidoMapping* normalized = 0);
	const fidoMapping& map(fidoPoint& point) const;
	fidoMapping& translate(signed16dot16 aboutX, signed16dot16 aboutY, fidoMapping* output = 0);
	fidoMapping& scale(signed16dot16 scaleX, signed16dot16 scaleY,
		signed16dot16 aboutX = ff(0), signed16dot16 aboutY = ff(0), fidoMapping* output = 0);
	fidoMapping& rotate(signed16dot16 angle, signed16dot16 aboutX = ff(0), signed16dot16 aboutY = ff(0), 
		fidoMapping* output = 0);
	fidoMapping& invert(fidoMapping* inverse = 0) const;
	fidoMapping& concatenate(const fidoMapping& add, fidoMapping* combined = 0);
	enum type {
		identity,
		translated,
		squareScaled,
		scaled,
		rotated,
		affine,
		perspective,
		undetermined
	};
// probably want to cache this one day
	type order() const;
};

struct math {
//  jim blinn trick avoids divide (ieee computer graphics vol 15 no 6)
	static int div255(int a);
	static unsigned udiv255(int a);
	static int firstBit(unsigned long a);
	// shift == a - b + c where a == fraction bits in quotient, b == fraction bits in dividend, c == fraction bits in divisor
	static long multiply(long a, long b, int shift);
	static long divide(long dividend, long divisor, int shift);
	static long multiplyDivide(long a, long b, long c);
};

struct fidoPoint {
	signed16dot16 x;
	signed16dot16 y;
	bool lessThan(fidoPoint& other)
		{ return y < other.y || y == other.y && x < other.x; }
	fidoPoint& exchange(fidoPoint& other)
		{ fidoPoint temp = *this; *this = other; other = temp; return *this; }
};

struct fidoRectangle {
	signed16dot16 left;
	signed16dot16 top;
	signed16dot16 right;
	signed16dot16 bottom;
	fidoRectangle() {}
	fidoRectangle(signed16dot16 l, signed16dot16 t, signed16dot16 r, signed16dot16 b) :
		left(l), top(t), right(r), bottom(b) {}
	fidoRectangle(fidoPoint* array, int size);
	fidoRectangle& set(signed16dot16 l, signed16dot16 t, signed16dot16 r, signed16dot16 b) {
		left = l;  top = t; right = r; bottom = b; return *this; }
	void onion(fidoRectangle&);	// really union, but who can spell
};

struct signed32dot32 {				// for very large intermediate numbers
	signed long hi;
	unsigned long lo;
	signed32dot32& set16dot16(signed16dot16 num);
	long divide(long divisor, int shift);
	signed32dot32& multiply(long multiplicand);
	signed32dot32& add(signed32dot32 &operand);
	signed32dot32& subtract(signed32dot32 &operand);
	long scale(int count);
	long round(int count);
	signed32dot32& multiply(signed16dot16 a, signed16dot16 b);
	signed32dot32& multiplyAdd(signed16dot16 a, signed16dot16 b, signed16dot16 c, signed16dot16 d, int shift = 0);
	signed32dot32& multiplyAdd(signed16dot16 a, signed16dot16 b, signed16dot16 c, signed16dot16 d,
		signed16dot16 e, signed16dot16 f, int shift = 0);
	signed32dot32& multiplyAdd(	signed16dot16 a, signed16dot16 b, int shift1,
					signed16dot16 c, signed16dot16 d, int shift2,
					signed16dot16 e, signed16dot16 f);
	signed16dot16 root();
	int firstBit();
};

struct bigMapping {
	signed32dot32 big[3][3];
	fidoMapping& normalize(fidoMapping* normal, int affineFractionBits, int perspectiveFractionBits);
};

#if _SIMULATE_FIDO_
struct fidoDivide {
	static void makeInverseTable();
	static unsigned8dot8 reciprocal(unsigned0dot16 test);
	enum algorithmType {
		intDivide,
		recip16S148
	};
	static enum algorithmType algorithm;
	static int scale;
	static void setAlgorithm(algorithmType which);
};
#endif

#ifndef _FIDOMATH_CPP_
#include "fidoMath.cpp"
#endif

#endif // _FIDOMATH_H_
