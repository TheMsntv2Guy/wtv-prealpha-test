#ifndef __AUDIO_SCOPE_H__
#define __AUDIO_SCOPE_H__

#ifndef __DISPLAYABLE_H__
#include "Displayable.h"
#endif
#ifndef __IMAGE_H__
#include "Image.h"
#endif
#ifndef __IMAGEDATA_H__
#include "ImageData.h"
#endif


const long kBGIndex		= 1;
const long kLeftIndex	= 2;
const long kRightIndex	= 3;

const long kWhiteIndex	= 15;
const long kGrayIndex 	= 14;
const long kLtGrayIndex	= 13;


class AudioScope : public Image
{
public:
							AudioScope();
	virtual					~AudioScope();

	virtual void			Draw(const Document* document, const Rectangle* invalid);
#ifdef FIDO_INTERCEPT
	virtual void			Draw(const Document* document, class FidoCompatibilityState& fidoCompatibility) const;
#endif
	virtual Boolean			Idle(Layer*);
		
	virtual void			SetAttribute(Attribute attributeID, long value, Boolean isPercentage);
	virtual void			SetAttributeStr(Attribute attributeID, const char* value);

	void 					Setup(void);
	void					SetColor(long index, long color);

protected:
	void					BlankScope(uchar colorIndex);
	void					DrawTrace(short *audio, uchar index, long offset);
	
	void					DrawHLine(long pos, uchar index);
	void					DrawMax(void);

	Rectangle				fBounds;	
	CLUT*					fCLUT;
	BitMapDevice*			fBitmapDevice;
	unsigned				fRunning : 1;
	unsigned				fDrawCalled : 1;
	long					fBGColor;
	long					fLeftColor;
	long					fRightColor;
	long					fLeftOffset;
	long					fRightOffset;
	long					fGain;
	
	unsigned				fMaxLevel : 1;
	long					fPos;
	long					fVel;
	long					fMax;
	long					fSettle;
};

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include AudioScope.h multiple times"
	#endif
#endif /* __AUDIO_SCOPE_H__ */
