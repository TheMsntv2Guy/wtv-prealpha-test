#ifndef __ANIMATION_H__
#define __ANIMATION_H__

#ifndef __DISPLAYABLE_H__
#include "Displayable.h"
#endif
#ifndef __IMAGE_H__
#include "Image.h"
#endif
#ifndef __IMAGEDATA_H__
#include "ImageData.h"
#endif

// =============================================================================

/* parsing state */

extern ulong	gPendingAnimationDissolvePeriod;
extern TransitionEffect gAnimationTransitionType;


typedef enum
{
	kAnimationCreateStream = 0,
	kAnimationReadHeader,
	kAnimationWaitForImages,
	kAnimationReady,
	kAnimationError
} AnimationState;

const ulong		kMaxCelImages = 32767;

struct MotionFrame
{
	ulong			fTicksToNext;
	Rectangle		fSourceBounds;
	Coordinate		fTranslation;
	Coordinate		fHotSpotOffset;		// offset from ULH to hot spot
	ushort			fScaleX;
	ushort			fScaleY;
	ushort			fDissolveIntoPeriod;
	ushort			fImageNumber;
};
typedef struct MotionFrame	MotionFrame;

struct LoopFrame
{
	ulong			fMaxCount;
	long			fCurrentCount;
	ulong			fLoopTopFrame;
	
};
typedef struct LoopFrame	LoopFrame;

struct SoundFrame
{
	ulong			soundIndex;
};
typedef struct SoundFrame	SoundFrame;

typedef enum
{
	kMotionFrame,
	kLoopFrame 
} FrameType;

struct Frame
{
	FrameType		fType;
	union
	{
		MotionFrame			motion;
		LoopFrame			loop;
	} u;
};
typedef struct Frame	Frame;

// =============================================================================

class Animation : public Image
{
public:
							Animation();
	virtual					~Animation();

	virtual void			Draw(const Document* document, const Rectangle* invalid);
#ifdef FIDO_INTERCEPT
	virtual void			Draw(const Document* document, class FidoCompatibilityState& fidoCompatibility) const;
#endif
	virtual long			GetLayoutWidth() const;				// overridden to affect layout
	virtual long			GetLayoutHeight() const;			// overridden to affect layout

	virtual Boolean			Idle(Layer*);
	virtual Boolean			IsAnimation() const;
	Boolean					IsRunning() const;
	Boolean					AnimationIdle(Layer*, Coordinate *delta, Coordinate* scale, ulong* dissolveIntoPeriod);

	virtual void			PurgeResource();
		
	virtual void			SetAttribute(Attribute attributeID, long value, Boolean isPercentage);
	virtual void			SetAttributeStr(Attribute attributeID, const char* value);
	void					SetShouldLoop(Boolean);
	virtual void			SetTop(long);
	virtual void			SetLeft(long);
	void					Start();
	void					Stop();

protected:
	void					Reset();
	Boolean					SetCurrentFrame(ulong newFrame, Rectangle *destinationBounds = nil);
	Boolean					ParseData();
	void					BuildLoopFrame();
	Boolean					PopLoopFrame(DataList* loopFrames);
	void					PushLoopFrame(DataList* loopFrames, ulong maxCount);

protected:
	AnimationState			fAnimationState;
	DataList				fFrameList;
	ObjectList				fImageList;
	ulong					fNextIdle;
	DataStream*				fStream;

	short					fCurrentFrame;

	signed					fDirectionX : 2;
	signed					fDirectionY : 2;
	unsigned				fBackwards: 1;
	unsigned				fRunning : 1;
	unsigned				fShouldLoop : 1;
	unsigned				fShouldSwing: 1;
	unsigned				fShouldHFlip: 1;
	unsigned				fShouldVFlip: 1;
	unsigned				fShouldHBounce: 1;
	unsigned				fShouldVBounce: 1;
	unsigned				fSkipLayout : 1;
};

// =============================================================================

inline Boolean Animation::IsRunning() const
{
	return fRunning;
}

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Animation.h multiple times"
	#endif
#endif /* __ANIMATION_H__ */
