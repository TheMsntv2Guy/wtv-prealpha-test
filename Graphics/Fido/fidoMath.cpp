#if !defined _FIDO_DEBUG_H_
#include "fidoDebug.h"
#endif

#ifndef _FIDOMATH_H_
#include "fidoMath.h"
#endif

#ifndef _FIDOMATH_CPP_
#define _FIDOMATH_CPP_

#if _SIMULATE_FIDO_
inline void fidoDivide::setAlgorithm(algorithmType which)
{
	algorithm = which;
	if (which == recip16S148)
		makeInverseTable();
}
#endif

inline int math::firstBit(unsigned long number) 
{
	int bitSet = 0;
	while (number != 0) {
		bitSet++;
		number += number;
	}
	return bitSet;
}

inline int math::div255(int a)
{
	int i = a + 128;
	return i + (i >> 8) >> 8;
}

inline unsigned math::udiv255(int a)
{
	unsigned i = a + 128;
	return i + (i >> 8) >> 8;
}

inline long math::divide(long dividend, long divisor, int shift)
{
	signed32dot32 big;
	big.hi = dividend;
	big.lo = 0;
	return big.divide(divisor, shift);
}

inline long math::multiplyDivide(long a, long b, long c)
{
	signed32dot32 big;
	big.lo = a;
	big.multiply(b);
	return big.divide(c, 32);
}

inline signed8dot8 s8dot8::multiply(signed8dot8 a, signed8dot8 b)
{
	return a * b >> shift;
}

// !!! get rid of int divide, eventually, or at least qualify this and others as fido only
inline signed8dot8 s8dot8::divide0dot16(signed8dot8 a, unsigned0dot16 b)
{
	Assert(b == 0 || (a / 256.) / (b / 65536.) >= -128 && (a / 256.) / (b / 65536.) < 128);
	return fidoDivide::algorithm == fidoDivide::intDivide ?
		(a << 16) / b : a * fidoDivide::reciprocal(b) >> s8dot8::shift;
}

inline signed8dot8 s8dot8::multiplyDivide(signed8dot8 multiplicand, signed8dot8 multiplier, signed8dot8 divisor)
{
	return multiplicand * multiplier / divisor;
}
// #endif

inline signed16dot16 s8dot8::as16dot16(signed8dot8 f)
{
	return f << s16dot16::shift - shift;
}

inline int s10dot10::roundUp(signed10dot10 a)
{
	return a + one/2 >> shift;
}

inline int s10dot10::roundDown(signed10dot10 a)
{
	return a + one/2 - 1 >> shift;
}

inline signed16dot16 s10dot10::as16dot16(signed10dot10 f)
{
	return f << s16dot16::shift - shift;
}

#if DEBUG_WITH_CLASSES
inline s16dot16& u0dot16::to16dot16(u0dot16 operand)
{
	s16dot16 f;
	f.value = operand.value;
	return f;
}

inline s2dot30& u0dot16::to2dot30(u0dot16 operand)
{
	s2dot30 f;
	f.value = operand.value << f.shift - shift;
	return f;
}
#else
inline signed16dot16 u0dot16::to16dot16(unsigned0dot16 operand)
{
	return operand;
}

inline signed2dot30 u0dot16::to2dot30(unsigned0dot16 operand)
{
	return operand << s2dot30::shift - shift;
}
#endif

inline signed32dot32& signed32dot32::add(signed32dot32 &operand)
{
	unsigned long temp = lo;
	lo += operand.lo;
	hi += operand.hi + (temp > lo);
	return *this;
}

inline signed32dot32& signed32dot32::subtract(signed32dot32 &operand)
{
	unsigned long temp = lo;
	lo -= operand.lo;
	hi -= operand.hi - (temp < lo);
	return *this;
}
	
inline long signed32dot32::scale(int count)
{
	hi = (hi << count) | (lo >> 32 - count);
	lo <<= count;
	return lo;
}

inline long signed32dot32::round(int count)
{
	if (count < 0) return scale(-count);
	bool roundBit = count ? lo >> count - 1 & 1: 0;
	lo = ((lo >> count) | (hi << 32 - count)) + roundBit;
	hi >>= count;
	 return lo;
}

inline signed32dot32& signed32dot32::multiply(signed16dot16 a, signed16dot16 b)
{
	hi = 0;
	lo = s16dot16::asInt(a);
	multiply(s16dot16::asInt(b));
	return *this;
}

inline signed32dot32& signed32dot32::set16dot16(signed16dot16 num)
{
	hi = num >> s16dot16::shift;
	lo = num << s16dot16::shift;
	return *this;
}

inline signed16dot16 s16dot16::multiplyDivide(fidoPoint& pt1, fidoPoint& pt2, signed16dot16 divisor)
{
	signed32dot32 big;
	big.multiplyAdd(pt1.x, pt2.x, pt1.y, pt2.y);
	return s16dot16::fromInt(big.divide(s16dot16::asInt(divisor), 32));
}

inline signed16dot16 s16dot16::root(signed16dot16 x, signed16dot16 y)
{
	signed32dot32 big;
	big.multiplyAdd(x, x, y, y);
	return big.root();
}

inline signed16dot16 s16dot16::multiply(signed16dot16 a, signed16dot16 b)
{
	return s16dot16::fromInt(math::multiply(s16dot16::asInt(a), s16dot16::asInt(b), shift));
}

inline signed16dot16 s16dot16::divide(signed16dot16 dividend, signed16dot16 divisor)
{
	return s16dot16::fromInt(math::divide(s16dot16::asInt(dividend), s16dot16::asInt(divisor), shift));
}

#if DEBUG_WITH_CLASSES == 0
inline int s16dot16::round(signed16dot16 a)
{
	return a + one/2 >> shift;
}

inline signed16dot16 s16dot16::roundUp(signed16dot16 a)
{
	return a + one/2 & ~(one - 1) | one/2;
}

inline signed16dot16 s16dot16::roundDown(signed16dot16 a)
{
	return a - one/2 - 1 & ~(one - 1) | one/2;
}

inline signed8dot8 s16dot16::roundTo8dot8(signed16dot16 a)
{
	return a + (1 << shift - s8dot8::shift - 1) >> shift - s8dot8::shift;
}

inline signed10dot10 s16dot16::roundTo10dot10(signed16dot16 a)
{
	return a + (1 << shift - s10dot10::shift - 1) >> shift - s10dot10::shift;
}

inline signed2dot30 s16dot16::cosine(signed16dot16 angle)
{
	return sine(angle + s16dot16::angle90);
}

inline unsigned0dot16 s2dot30::roundToU0dot16(signed2dot30 a)
{
	return a + (1 << shift - u0dot16::shift - 1) >> shift - u0dot16::shift;
}

inline signed0dot16 s2dot30::roundToS0dot16(signed2dot30 a)
{
	return a + (1 << shift - s0dot16::shift - 1) >> shift - s0dot16::shift;
}
#endif

#if DEBUG_WITH_CLASSES
inline struct s16dot16& s2dot30::to16dot16(s2dot30 operand)
{
	s16dot16 f;
	f.value = operand >> shift - s16dot16::shift;
	return f;
}
#else
inline signed16dot16 s2dot30::to16dot16(signed2dot30 f)
{
	return f >> shift - s16dot16::shift;
}
#endif

inline signed2dot30 s2dot30::multiply(signed2dot30 a, signed2dot30 b)
{
	return s2dot30::fromInt(math::multiply(s2dot30::asInt(a), s2dot30::asInt(b), shift));
}

inline signed2dot30 s2dot30::divide(signed2dot30 dividend, signed2dot30 divisor)
{
	return s2dot30::fromInt(math::divide(s2dot30::asInt(dividend), s2dot30::asInt(divisor), shift));
}

inline fidoMapping::type fidoMapping::order() const
{
	return name.u != fr(0) || name.v != fr(0) || name.w != s2dot30::one ? perspective :
		name.skewX != -name.skewY || name.scaleX != name.scaleY ? affine :	// !!! not quite true (needs to also compare magnitude
		name.skewX != ff(0) || name.skewY != ff(0) ?  rotated :
		name.scaleX != s16dot16::one || name.scaleY != s16dot16::one ?
		name.scaleX != name.scaleY ? scaled : squareScaled : 
		name.moveX != ff(0) || name.moveY != ff(0) ? translated : identity;
}

#endif // _FIDOMATH_CPP_