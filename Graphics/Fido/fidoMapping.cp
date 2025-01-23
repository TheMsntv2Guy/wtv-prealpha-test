#include "fidoDebug.h"
#include "fidoMath.h"

// !!! another flavor of this could be added (normalizeFast) that shifts instead of divides
// only the tests currently use this one; it is not used internally
fidoMapping& fidoMapping::normalize(fidoMapping* normalized)
{		
	if (normalized == nil)
		normalized = this;
	if (array[2][2] != s2dot30::one) {
		normalized->array[0][0] = s2dot30::divide(array[0][0], array[2][2]);
		normalized->array[0][1] = s2dot30::divide(array[0][1], array[2][2]);
		normalized->array[0][2] = s2dot30::divide(array[0][2], array[2][2]);
		normalized->array[1][0] = s2dot30::divide(array[1][0], array[2][2]);
		normalized->array[1][1] = s2dot30::divide(array[1][1], array[2][2]);
		normalized->array[1][2] = s2dot30::divide(array[1][2], array[2][2]);
		normalized->array[2][0] = s2dot30::divide(array[2][0], array[2][2]);
		normalized->array[2][1] = s2dot30::divide(array[2][1], array[2][2]);
	} else if (this != normalized)
		normalized = this;
	normalized->array[2][2] = s2dot30::one;
	return *normalized;
}


signed32dot32& signed32dot32::multiplyAdd(signed16dot16 a, signed16dot16 b, signed16dot16 c, signed16dot16 d, int shift)
{
	lo = a; 
	multiply(b);
	signed32dot32 temp;
	temp.lo = c;
	temp.multiply(d);
	if (shift)
		temp.round(shift);
	add(temp);
	return *this;
}


signed32dot32& signed32dot32::multiplyAdd(signed16dot16 a, signed16dot16 b, signed16dot16 c, signed16dot16 d, 
	signed16dot16 e, signed16dot16 f, int shift)
{
	multiplyAdd(a, b, c, d);
	signed32dot32 temp;
	temp.lo = e;
	temp.multiply(f);
	if (shift)
		temp.round(shift);
	add(temp);
	return *this;
}


signed32dot32& signed32dot32::multiplyAdd(signed16dot16 a, signed16dot16 b, int shift1, signed16dot16 c, signed16dot16 d, int shift2, 
	signed16dot16 e, signed16dot16 f)
{
	Assert(shift1 == shift2);
	NonDebugCode(shift2);	// suppress warning
	multiplyAdd(a, b, c, d);
	signed32dot32 temp;
	temp.lo = e;
	temp.multiply(f);
	if (shift1)
		round(shift1);
	add(temp);
	return *this;
}


// after inverse, affine is 18.46, perspective is 32.32
// after concatenation, affine is 32.32, perspective is 18.46
fidoMapping& bigMapping::normalize(fidoMapping* normal, int affineFractionBits, int perspectiveFractionBits)
{
 // either shift down so w < 1 or shift up keeping w < 1 and keeping more significant bits
	int perspectiveShift = 32 + 1 - big[2][2].firstBit();
	signed32dot32 bigTrialW = big[2][2];
	signed2dot30 trialW = bigTrialW.round(perspectiveShift);
	if (trialW != s2dot30::one && trialW != -s2dot30::one)
		perspectiveShift++;
	int shiftRequested = perspectiveShift + (affineFractionBits - perspectiveFractionBits + 14);
	for (int counter = 0; counter < 8; ++counter) {
		int shiftRequired = affineFractionBits + 1 - big[0][counter].firstBit();
		if (shiftRequired > shiftRequested)
			shiftRequested = shiftRequired;
	}
// !!! add check to see if w is too small and other numbers are too big
	perspectiveShift = shiftRequested - (affineFractionBits - perspectiveFractionBits + 14);
	for (int row = 0; row < 3; row++) {
		for (int column = 0; column < 2; column++)
			normal->array[row][column] = big[row][column].round(shiftRequested);
		normal->array[row][2] = big[row][2].round(perspectiveShift);
	}
// make w always positive
	if (normal->array[2][2] < 0) {
		for (int counter = 0; counter < 9; counter++)
			normal->array[0][counter]  = -normal->array[0][counter];
	}
	return *normal;
}


fidoMapping& fidoMapping::invert(fidoMapping* inverse) const
{		
	if (inverse == nil)
// !!! fix this before making embedded build with gcc
		inverse = const_cast<fidoMapping*>(this);
	switch(order()) {
		case identity:
			*inverse = *this;
		break;
		case translated:
			*inverse = *this;
			inverse->name.moveX = -inverse->name.moveX;
			inverse->name.moveY = -inverse->name.moveY;
		break;
		default:
			bigMapping temp;
			if (name.u == 0 && name.v == 0) {
				temp.big[0][0].multiply(+array[1][1], array[2][2]);
				temp.big[0][1].multiply(-array[0][1], array[2][2]);
				temp.big[0][2].lo = temp.big[0][2].hi = 0;
				temp.big[1][0].multiply(-array[1][0], array[2][2]);
				temp.big[1][1].multiply(+array[0][0], array[2][2]);
				temp.big[1][2].lo = temp.big[1][2].hi = 0;
			} else {
				temp.big[0][0].multiplyAdd(+array[1][1], array[2][2], -array[2][1], array[1][2]);
				temp.big[0][1].multiplyAdd(-array[0][1], array[2][2], +array[2][1], array[0][2]);
				temp.big[0][2].multiplyAdd(+array[0][1], array[1][2], -array[1][1], array[0][2]);
				temp.big[1][0].multiplyAdd(-array[1][0], array[2][2], +array[2][0], array[1][2]);
				temp.big[1][1].multiplyAdd(+array[0][0], array[2][2], -array[2][0], array[0][2]);
				temp.big[1][2].multiplyAdd(-array[0][0], array[1][2], +array[1][0], array[0][2]);
			}
		// !!! this throws away bits, but I don't see any easy alternative
			for (int counter = 0; counter < 6; ++counter)
				temp.big[0][counter].round(14);	// make them all 32.32s
			temp.big[2][0].multiplyAdd(+array[1][0], array[2][1], -array[2][0], array[1][1]);
			temp.big[2][1].multiplyAdd(-array[0][0], array[2][1], +array[2][0], array[0][1]);
			temp.big[2][2].multiplyAdd(+array[0][0], array[1][1], -array[1][0], array[0][1]);
			temp.normalize(inverse, 32, 32);
	}
	return *inverse;
}


// !!! needs optimization pass; but first, make it work
fidoMapping& fidoMapping::concatenate(const fidoMapping& add, fidoMapping* combined)
{
	if (combined == nil)
		combined = this;
	type thisOrder = order();
	type addOrder = add.order();
	if (addOrder == identity) {
		*combined = *this;
		return *combined;
	}
	if (thisOrder == identity) {
		*combined = add;
		return *combined;
	}
	bigMapping temp; // using temp allows combined to be add or this
	if (thisOrder == translated) {	// a common case
		temp.big[0][0].set16dot16(add.array[0][0]); 
		temp.big[0][1].set16dot16(add.array[0][1]);
		temp.big[0][2].set16dot16(add.array[0][2]);
		temp.big[1][0].set16dot16(add.array[1][0]);
		temp.big[1][1].set16dot16(add.array[1][1]);
		temp.big[1][2].set16dot16(add.array[1][2]);
		temp.big[2][0].multiplyAdd(array[2][0], add.array[0][0],
							array[2][1], add.array[1][0],
							s16dot16::one, add.array[2][0]);
		temp.big[2][1].multiplyAdd(array[2][0], add.array[0][1],
							array[2][1], add.array[1][1], 
							s16dot16::one, add.array[2][1]);
		temp.big[2][2].multiplyAdd(array[2][0], add.array[0][2],
							array[2][1], add.array[1][2],
							s16dot16::one, add.array[2][2]);
	} else if (addOrder == translated) {
		temp.big[0][0].set16dot16(array[0][0]);
		temp.big[0][1].set16dot16(array[0][1]);
		temp.big[0][2].set16dot16(array[0][2]);
		temp.big[1][0].set16dot16(array[1][0]);
		temp.big[1][1].set16dot16(array[1][1]);
		temp.big[1][2].set16dot16(array[1][2]);
		temp.big[2][0].multiplyAdd(array[2][0], s16dot16::one,
							  array[2][2], add.array[2][0], s2dot30::shift - s16dot16::shift);
		temp.big[2][1].multiplyAdd(array[2][1], s16dot16::one,
							  array[2][2], add.array[2][1], s2dot30::shift - s16dot16::shift);
		temp.big[2][2].multiply(array[2][2], add.array[2][2]).round(s2dot30::shift - s16dot16::shift);
	} else {
		// !!!? worth adding special case if [0][2], [1][2] == 0 in both matrices?
		temp.big[0][0].multiplyAdd(	array[0][0], add.array[0][0],
								array[0][1], add.array[1][0],
								array[0][2], add.array[2][0], s2dot30::shift - s16dot16::shift);
		temp.big[0][1].multiplyAdd(	array[0][0], add.array[0][1],
								array[0][1], add.array[1][1],
								array[0][2], add.array[2][1], s2dot30::shift - s16dot16::shift);
		temp.big[0][2].multiplyAdd(	array[0][0], add.array[0][2],
								array[0][1], add.array[1][2],
								array[0][2], add.array[2][2], s2dot30::shift - s16dot16::shift);
		temp.big[1][0].multiplyAdd(	array[1][0], add.array[0][0],
								array[1][1], add.array[1][0],
								array[1][2], add.array[2][0], s2dot30::shift - s16dot16::shift);
		temp.big[1][1].multiplyAdd(	array[1][0], add.array[0][1],
								array[1][1], add.array[1][1],
								array[1][2], add.array[2][1], s2dot30::shift - s16dot16::shift);
		temp.big[1][2].multiplyAdd(	array[1][0], add.array[0][2],
								array[1][1], add.array[1][2],
								array[1][2], add.array[2][2], s2dot30::shift - s16dot16::shift);
		temp.big[2][0].multiplyAdd(	array[2][0], add.array[0][0],
								array[2][1], add.array[1][0],
								array[2][2], add.array[2][0], s2dot30::shift - s16dot16::shift);
		temp.big[2][1].multiplyAdd(	array[2][0], add.array[0][1],
								array[2][1], add.array[1][1],
								array[2][2], add.array[2][1], s2dot30::shift - s16dot16::shift);
		temp.big[2][2].multiplyAdd(	array[2][0], add.array[0][2],
								array[2][1], add.array[1][2],
								array[2][2], add.array[2][2], s2dot30::shift - s16dot16::shift);
	}
	temp.normalize(combined, 32, 46);
	return *combined;
}

fidoMapping& fidoMapping::setIdentity()
{
	name.scaleX = s16dot16::one;	name.skewY = 0;			name.u = 0;
	name.skewX = 0;			name.scaleY = s16dot16::one;	name.v = 0;
	name.moveX = 0;			name.moveY =  0;			name.w = s2dot30::one;	
	return *this;
}

fidoMapping& fidoMapping::setTranslate(signed16dot16 aboutX, signed16dot16 aboutY)
{
	name.scaleX = s16dot16::one;	name.skewY = 0;			name.u = 0;
	name.skewX = 0;			name.scaleY = s16dot16::one;	name.v = 0;
	name.moveX = aboutX;		name.moveY =  aboutY;		name.w = s2dot30::one;
	return *this;
}

fidoMapping& fidoMapping::setScale(signed16dot16 scaleX, signed16dot16 scaleY, signed16dot16 aboutX, signed16dot16 aboutY)
{
	name.scaleX = scaleX;		name.skewY = 0;			name.u = 0;
	name.skewX = 0;			name.scaleY = scaleY;		name.v = 0;
	name.moveX = aboutX;		name.moveY =  aboutY;		name.w = s2dot30::one;
	return *this;
}

fidoMapping& fidoMapping::setRotate(signed16dot16 angle, signed16dot16 aboutX, signed16dot16 aboutY)
{
	name.scaleX = s2dot30::to16dot16(s16dot16::cosine(angle));
	name.skewY = s2dot30::to16dot16(s16dot16::sine(angle));	name.u = 0;
	name.skewX = -s2dot30::to16dot16(s16dot16::sine(angle));	
	name.scaleY = s2dot30::to16dot16(s16dot16::cosine(angle));	name.v = 0;
	name.moveX = aboutX;		name.moveY = aboutY;		name.w = s2dot30::one;
	return *this;
}

fidoMapping& fidoMapping::translate(signed16dot16 aboutX, signed16dot16 aboutY, fidoMapping* output)
{
	if (output == nil)
		output = this;
	fidoMapping translated;
	translated.setTranslate(aboutX, aboutY);
	concatenate(translated, output);
	return *output;
}

fidoMapping& fidoMapping::scale(signed16dot16 scaleX, signed16dot16 scaleY, signed16dot16 aboutX, signed16dot16 aboutY, fidoMapping* output)
{
	if (output == nil)
		output = this;
	fidoMapping translated;
	translated.setTranslate(-aboutX, -aboutY);
	translated.concatenate(*this, output);
	fidoMapping scaled;
	scaled.setScale(scaleX, scaleY, aboutX, aboutY);
	output->concatenate(scaled);
	return *output;
}

fidoMapping& fidoMapping::rotate(signed16dot16 angle, signed16dot16 aboutX, signed16dot16 aboutY, fidoMapping* output)
{
	if (output == nil)
		output = this;
	fidoMapping translated;
	translated.setTranslate(-aboutX, -aboutY);
	translated.concatenate(*this, output);
	fidoMapping rotated;
	rotated.setRotate(angle, aboutX, aboutY);
	output->concatenate(rotated);
	return *output;
}

const fidoMapping& fidoMapping::map(fidoPoint& point) const
{
	signed32dot32 scaleX, skewX, moveX;
	moveX.hi = name.moveX >> 16; moveX.lo = name.moveX << 16;
	scaleX.lo = point.x;
	scaleX.multiply(name.scaleX);
	skewX.lo = point.y;
	skewX.multiply(name.skewX);
	scaleX.add(skewX);
	scaleX.add(moveX);
	signed32dot32 scaleY, skewY, moveY;
	moveY.hi = name.moveY >> 16; moveY.lo = name.moveY << 16;
	scaleY.lo = point.y;
	scaleY.multiply(name.scaleY);
	skewY.lo = point.x;
	skewY.multiply(name.skewY);
	scaleY.add(skewY);
	scaleY.add(moveY);
	
	if (name.u || name.v) {
		signed32dot32 bigU;	// bigU is 18.46
		bigU.lo = point.x;
		bigU.multiply(name.u);
		signed32dot32 bigV;
		bigV.lo = point.y;
		bigV.multiply(name.v);
		bigU.add(bigV);
		signed32dot32 bigW;
		bigW.hi = name.w >> 16; bigW.lo = name.w << 16;
		bigU.add(bigW);
	// !!! is rounding the right thing to do?
		bigU.round(46-16);	// !!! need to add overflow check
		point.x = scaleX.divide(bigU.lo, 32);
		point.y = scaleY.divide(bigU.lo, 32);
	} else {
		Assert((scaleX.hi & 0xFFFF0000) == (scaleX.hi < 0 ? 0xFFFF0000 : 0));
		point.x = (scaleX.hi << 16) + (scaleX.lo >> 16); 
		Assert((scaleY.hi & 0xFFFF0000) == (scaleY.hi < 0 ? 0xFFFF0000 : 0));
		point.y = (scaleY.hi << 16) + (scaleY.lo >> 16); 
	}
	return *this;
}
