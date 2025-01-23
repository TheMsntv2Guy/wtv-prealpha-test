#include "fidoMath.h"

struct fido3dPoint {
	signed16dot16 x;
	signed16dot16 y;
	signed16dot16 z;
	subtract(const fido3dPoint& operand, fido3dPoint* result = 0);
	add(const fido3dPoint& operand, fido3dPoint* result = 0);
	cross(const fido3dPoint& operand, fido3dPoint* result = 0);
	negate(fido3dPoint* result = 0);
	length(fido3dPoint* result = 0);
	normalize(fido3dPoint* result = 0);
	set(signed16dot16, signed16dot16, signed16dot16);
	bool equal(const fido3dPoint& operand) const;
};

