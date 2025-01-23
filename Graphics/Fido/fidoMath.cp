#include "fidoDebug.h"
#include "fidoMath.h"

fidoDivide::algorithmType fidoDivide::algorithm = fidoDivide::recip16S148;	// change this with fidoDivide::setAlgorithm, if necessary

const unsigned0dot16 u0dot16::one = u0dot16::fromInt(0);
const signed2dot30 s2dot30::one = fr(1);
const signed8dot8 s8dot8::one = s8dot8::fromInt(1 << s8dot8::shift);
const unsigned8dot8	u8dot8::one = u8dot8::fromInt(1 << u8dot8::shift);
const signed10dot10	s10dot10::one = s10dot10::fromInt(1 << s10dot10::shift);
const signed16dot16	s16dot16::one = ff(1);
const unsigned0dot16 u0dot16::divideBottom = u0dot16::fromInt(256);
const signed16dot16	s16dot16::angle360 = s16dot16::fromInt(1 << s16dot16::shift);
const signed16dot16	s16dot16::angle270 = s16dot16::fromInt(0x0000C000);
const signed16dot16	s16dot16::angle180 = s16dot16::fromInt(0x00008000);
const signed16dot16	s16dot16::angle90 = s16dot16::fromInt(0x00004000);
const signed16dot16	s16dot16::angle45 = s16dot16::fromInt(0x00002000);
const signed16dot16	s16dot16::angle90Mask = s16dot16::fromInt(0x04000 - 1);
const signed16dot16	s16dot16::min = s16dot16::fromInt(0x80000000);	// for overflow checks
const signed16dot16	s16dot16::max = s16dot16::fromInt(0x7FFFFFFF);
const signed8dot8 s8dot8::min = s8dot8::fromInt(0x8000);	// for overflow checks
const signed8dot8 s8dot8::max = s8dot8::fromInt(0x7FFF);
const signed10dot10 s10dot10::min = s10dot10::fromInt(0xFFFE0000);	// for overflow checks
const signed10dot10 s10dot10::max = s10dot10::fromInt(0x0001FFFF);
const unsigned8dot8 u8dot8::max = u8dot8::fromInt(0xFFFF);
#ifdef DEBUG
const unsigned0dot16 u0dot16::error = u0dot16::fromInt(2); // maximum allowable error for simple calculations
const unsigned0dot16 u0dot16::oneHalf = u0dot16::fromInt(0x7FFF);
const unsigned0dot16 u0dot16::min = u0dot16::fromInt(0);
const unsigned0dot16 u0dot16::max = u0dot16::fromInt(0xFFFF);
const signed0dot16 s0dot16::oneHalf = s0dot16::fromInt(0x7FFF);
const signed0dot16 s0dot16::min = s0dot16::fromInt(0x8000);
const signed0dot16 s0dot16::max = s0dot16::fromInt(0x7FFF);
const signed16dot16	s16dot16::error = s16dot16::fromInt(2);
const signed16dot16	s16dot16::errorAtan2 = s16dot16::fromInt(1);
const signed2dot30 s2dot30::error = s2dot30::fromInt(0x00005836);	// max of: 1 - sin^2 - cos^2, about 0.00002
#endif

#define sineTableSize 1025
#define tanTableSize 1025
extern const unsigned0dot16 sineTable[sineTableSize];
extern const unsigned0dot16 tanTable[tanTableSize];
static unsigned long inverseTable[64];			// 8.16: 00xx.xxxx
static unsigned short inverseSquaredTable[64];		// 16.0: 0000xxxx.

void fidoDivide::makeInverseTable()
{
	for (int i = 0; i <= 63; i++) {
		double recip = 256./(i + 64 << 1);
		inverseTable[i] = recip * (1 << 16) + 0.5;
		Assert(inverseTable[i] < 0x01000000);
		inverseSquaredTable[i] = (recip * recip) * (1 << 8) + 0.5;
		Assert(inverseSquaredTable[i] > 0);
	}
}

// test ranges from 0 to <1 == 0xFFFF; 0 means 1.0
unsigned8dot8 fidoDivide::reciprocal(unsigned0dot16 test)
{
	if (inverseTable[0] == 0)
		makeInverseTable();
	if (test == u0dot16::one)
		return u8dot8::one;
	if (test <= u0dot16::divideBottom)
		return u8dot8::max;
	Assert(test >= u0dot16::fromInt(256));
	long result;
	unsigned char inputMSB;
	unsigned short inputLSB;
	int leadingZeros = 0;							// shift left till MSbit is 1
	if ((test & u0dot16::fromInt(0xf000)) == u0dot16::fromInt(0x0000)) {
		test <<= 4;
		leadingZeros += 4;
	}
	if ((test & u0dot16::fromInt(0xc000)) == u0dot16::fromInt(0x0000)) {
		test <<= 2;
		leadingZeros += 2;
	}
	if ((test & u0dot16::fromInt(0x8000)) == u0dot16::fromInt(0x0000)) {
		test <<= 1;
		leadingZeros += 1;
	}
	Assert(test & 0x8000);
	inputMSB = u0dot16::asInt(test) >> 9 & 0x003f;
	inputLSB = u0dot16::asInt(test) & 0x01ff;
	result = (inverseTable[inputMSB] << 8) - inverseSquaredTable[inputMSB] * inputLSB; // 8.16
	result >>= 16 - leadingZeros;						// scale result appropriately
	return u8dot8::fromInt(result);
}

// !!! detect overflow?
long signed32dot32::divide(long opB, int shift)
{
	// !!! this should use MIPS (and PowerPC?) assembly to do real 64/32 divide
	Assert(opB != 0);
	if (opB == s16dot16::asInt(s16dot16::one))
		return round(48 - shift);
	char sign = hi < 0 ^ opB < 0;
	unsigned long dividend = hi < 0 ?  -hi - (lo > 0) : hi;
	unsigned long dividendLo = hi < 0 ? -lo : lo;
	unsigned long divisor = opB < 0 ? -opB : opB;
	unsigned long divisorLo = 0;
	long quotient = 0;
	unsigned long divisorTimes2 = divisor << 1;
// normalize dividend
	while ((dividend > divisorTimes2 || dividend == divisorTimes2 && dividendLo == 0) && ++shift < 31) {
		dividendLo >>= 1;
		if (dividend & 1)
			dividendLo |= 0x80000000;
		dividend >>= 1;
	}
	Assert(shift < 33); // overflow
	do {
	// normalize divisor
		while ((dividend < divisor || dividend == divisor && dividendLo < divisorLo) && --shift >= 0) {
			divisorLo >>= 1;
			if (divisor & 1)
				divisorLo |= 0x80000000;
			divisor >>= 1;
		}
		if (shift < 0)
			break;
		unsigned long temp = dividendLo;
		dividendLo -= divisorLo;
		dividend -= divisor + (dividendLo > temp);
		quotient |= 1L << shift;
	} while (1);
	return sign ? -quotient : quotient;	
}

signed32dot32& signed32dot32::multiply(long multiplicand)
{
	if (multiplicand == s16dot16::asInt(s16dot16::one)) {
		hi = (signed long) lo >= 0 ? 0 : -1;
		scale(s16dot16::shift);
		return *this;
	}
	bool sign = 0;
	if ((signed long) lo < 0) {
		sign = 1;
		lo = -lo;
	}
	unsigned long operand = multiplicand;
	if (multiplicand < 0) {
		sign ^= 1;
		operand = -multiplicand;
	}
	unsigned long lo32 = (unsigned short) lo * (unsigned short) operand;
	unsigned long mid32 = (unsigned short) lo * (unsigned short) (operand >> 16);
	unsigned long mid32b = (unsigned short) (lo >> 16) * (unsigned short) operand;
	hi = (unsigned short) (lo >> 16) * (unsigned short) (operand >> 16);
	hi += (mid32 >> 16) + (mid32b >> 16);
	lo = lo32 + (mid32 << 16);
	if (lo32 > lo)
		hi++;
	lo32 = lo;
	lo += mid32b << 16;
	if (lo32 > lo)
		hi++;
	if (sign) {
		lo = -lo;
		hi = -hi - (lo != 0);
	}
	return *this;
}


int signed32dot32::firstBit()
{
	int bitSet;
	long check = hi < 0 ? -hi : hi;
	if (check == 0) {
		bitSet = 32;
		check = hi < 0 ? -lo : lo;
		if (check == 0)
			return 64;
	} else
		bitSet = 0;
	while (check > 0) {
		bitSet++;
		check += check;
	}
	return bitSet;
}


long math::multiply(long opA, long opB, int shift)
{	
	Assert(shift >= 0 && shift < 64);
	signed32dot32 big;
	big.lo = opA;
	big.multiply(opB);
	long result;
	Assert(shift != 0 || (big.lo & 0x80000000) != 0); // overflow check
	if (shift <= 32) {
		if (shift > 0) {
	#if ROUND_MULTIPLY
			signed32dot32 half = {0, 1 << shift - 1};
			if (big.hi < 0)
				big.subtract(half);
			else
				big.add(half);
	#endif
		}
		Assert(shift == 0 || ((big.hi >= 0 ? big.hi : -big.hi) & -1 << shift - 1) == 0); // overflow check
		result = big.lo >> shift | big.hi << (32 - shift);
	} else {
#if ROUND_MULTIPLY
		signed32dot32 otherHalf = {1 << shift - 33, 0};
		big.add(otherHalf);
#endif
		result = big.hi >> shift - 32;
	}
	return result;
}


#if BUILD_SINE_TABLE
#include <math.h>
#include <stdio.h>

void u0dot16::buildSineTable(void)
{
	const int tableMax = sizeof(sineTable)/sizeof(sineTable[0]);
	FILE* sineTableFile = fopen("sineTableFile.c", "w+");
	fprintf(sineTableFile, "// this is generated by u0dot16::buildSineTable in fidoMath.c\n\n");
	fprintf(sineTableFile, "#include \"fidoMath.h\"\n\n");
	fprintf(sineTableFile, "extern const unsigned0dot16 sineTable[%d];\n\n", tableMax);
	fprintf(sineTableFile, "const unsigned0dot16 sineTable[%d] = {\n", tableMax);
	for (int counter = 0; counter < tableMax; counter++) {
		int value = sin(counter * _PI  / 2 / (tableMax - 1)) * 0x10000 + 0.5;
		if (value >= 0x8000)
			value -= 1;
		Assert(value >= 0 && value <= 0xFFFF);
		fprintf(sineTableFile, "0x%04x%c%c", value, counter < tableMax - 1 ? ',' : ' ', (counter & 7) == 7 ? '\n' : ' ');
	}
	fprintf(sineTableFile, "\n};\n\n");
	fclose(sineTableFile);
}

// note that this only builds tangent values with a range of 0¡ to 45¡.  This means that only 1/8th of the range of the
// tangent table is used (max is 0x2000 instead of 0xFFFF) but this is as much as can be represented by a 16.16 angle
void u0dot16::buildAtanTable(void)
{
	const int tableMax = sizeof(tanTable)/sizeof(tanTable[0]);
	FILE* sineTableFile = fopen("sineTableFile.c", "a+");
	fprintf(sineTableFile, "extern const unsigned0dot16 tanTable[%d];\n\n", tableMax);
	fprintf(sineTableFile, "const unsigned0dot16 tanTable[%d] = {\n", tableMax);
	for (int counter = 0; counter < tableMax; counter++) {
		double fraction = (double) counter / (tableMax - 1);
		int value = atan(fraction) / _PI / 2 * 0x10000 + 0.5;
		Assert(value >= 0 && value <= 0xFFFF);
		fprintf(sineTableFile, "0x%04x%c%c", value, counter < tableMax - 1 ? ',' : ' ', (counter & 7) == 7 ? '\n' : ' ');
	}
	fprintf(sineTableFile, "\n};\n");
	fclose(sineTableFile);
}
#endif


signed8dot8 s16dot16::checkTo8dot8(signed16dot16 a)
{
#if DEBUG_WITH_CLASSES == 0
	Assert(a >= s8dot8::min << s16dot16::shift - s8dot8::shift &&
		a <= s8dot8::max << s16dot16::shift - s8dot8::shift);
#endif
	return roundTo8dot8(a);
}
	

signed10dot10 s16dot16::checkTo10dot10(signed16dot16 a)
{
#if DEBUG_WITH_CLASSES == 0
	Assert(a >= s10dot10::min << s16dot16::shift - s10dot10::shift &&
		a <= s10dot10::max << s16dot16::shift - s10dot10::shift);
#endif
	return roundTo10dot10(a);
}
	

unsigned0dot16 s2dot30::checkToU0dot16(signed2dot30 a)
{
	Assert(a >= fr(0) && a <= fr(1));
	return roundToU0dot16(a);
}
	
signed0dot16 s2dot30::checkToS0dot16(signed2dot30 a)
{
#if DEBUG_WITH_CLASSES == 0
	Assert(a >= s0dot16::min << s2dot30::shift - s0dot16::shift &&
		a <= s0dot16::max << s2dot30::shift - s0dot16::shift);
#endif
	return roundToS0dot16(a);
}

signed2dot30 s16dot16::sine(signed16dot16 angle)
{
	signed16dot16 root = angle & s16dot16::angle90Mask;
	int quadrant = angle >> s16dot16::shift - 2 & 3;
	if (quadrant & 1)
		root = s16dot16::angle90 - root;
		// !!! replace numbers with constants
	int index = s16dot16::asInt(root) >> 4;
	Assert(index >= 0 && index < sizeof(sineTable)/sizeof(sineTable[0]));
	signed2dot30 value = u0dot16::to2dot30(sineTable[index]);
	int fraction = root & 0xf;
	if (fraction)	// !!! could round here
		value += (u0dot16::to2dot30(sineTable[index + 1]) - value) * fraction >> 4;
	if (value >= s2dot30::one/2)
		value = s2dot30::fromInt(s2dot30::asInt(value) + 0x4000);
	if (quadrant > 1)
		value = -value;
	return value;
}

signed16dot16 s16dot16::atan(signed16dot16 x, signed16dot16 y)
{
	signed16dot16 absX = x > 0 ? x : -x;
	signed16dot16 absY = y > 0 ? y : -y;
	Assert(absX != 0 || absY != 0);
	signed2dot30 divisor = absX < absY ? s2dot30::divide(absX, absY) : s2dot30::divide(absY, absX);
	Assert(divisor >= 0 && divisor <= s2dot30::one);
		// !!! replace numbers with constants
	int index = divisor >> 20;
	Assert(index >= 0 && index < sizeof(tanTable)/sizeof(tanTable[0]));
	signed16dot16 arcTangent = u0dot16::to16dot16(tanTable[index]);
	signed2dot30 fraction = divisor & 0x000FFFFF;
	if (fraction)		// !!! could round here
		arcTangent += (tanTable[index + 1] - arcTangent) * fraction >> 20;
	if (absY > absX)
		arcTangent = s16dot16::angle90 - arcTangent;
	if (x < 0)
		arcTangent = s16dot16::angle180 - arcTangent;
	if (y < 0)
		arcTangent = -arcTangent;
	return arcTangent;
}
	
	
signed16dot16 signed32dot32::root()
{
	unsigned long work = hi;
	unsigned long result = 0, compare = 0, next;
	for (int counter = 15; counter >= 0; --counter) {
		compare = work >> 30 | compare << 2;
		work <<= 2;
		result += result;
		next = result + result + 1;
		if (next <= compare) {
			compare -= next;
			result++;
		}
	}
	work = lo;
	for (int counter = 13; counter >= 0; --counter) {
		compare = work >> 30 | compare << 2;
	 	work <<= 2;
	 	result += result;
		next = result + result + 1;
	 	if (next <= compare) {
	 		compare -= next;
	 		result++;
	 	}
	}
	unsigned short next_lo, compare_low;
	for (int counter = 1; counter >= 0; --counter) {
		compare_low = compare >> 30;
		compare = work >> 30 | compare << 2;
		work <<= 2;
		result += result;
		next_lo = result >> 31;
		next = result + result + 1;
		if (next_lo >= compare_low && (compare_low != next_lo || next > compare))
			continue;
		if (next > compare)
			compare_low -= next_lo - 1;
		else 
			compare_low -= next_lo;
		compare -= next;
		result++;
	}
	if (compare_low != 0 || result < compare && result + 1 != 0)
		result++;
	return result;
}

fidoRectangle::fidoRectangle(fidoPoint* array, int size)
{
	left = top = s16dot16::max;
	right = bottom = s16dot16::min;
	for (int counter = 0; counter < size; counter++) {
		if (array[counter].x < left)
			left = array[counter].x;
		else if (array[counter].x > right)
			right = array[counter].x;
		if (array[counter].y < top)
			top = array[counter].y;
		else if (array[counter].y > bottom)
			bottom = array[counter].y;
	}
}
