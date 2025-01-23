#ifndef __ANIMATIONDATA_H__
#define __ANIMATIONDATA_H__

#ifndef __IMAGEDATA_H__
#include "ImageData.h"
#endif

/* parsing state */

typedef enum
{
	kNewCelCreateStream = 0,
	kNewCelReadHeader,
	kNewCelDone,
	kNewCelError
} NewCelState;

const ulong		kMaxCelImages = 32767;

struct MotionFrame
{
	ulong			fTicksToNext;
	Rectangle		fSourceBounds;
	Coordinate		fTranslation;
	Coordinate		fHotSpotOffset;		// offset from ULH to hot spot
	ushort			fScale;
	ushort			fRotation;
	ushort			fImageNumber;
#ifdef SUPPORTS_BITFIELDS
	unsigned		fBlur: 1;
	unsigned		fHFlip: 1;
	unsigned		fVFlip: 1;
	unsigned		fHBounce: 1;
	unsigned		fVBounce: 1;
#else
	Boolean			fBlur;
	Boolean			fHFlip;
	Boolean			fVFlip;
	Boolean			fHBounce;
	Boolean			fVBounce;
#endif

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

class NewCelImage : public ImageData
{
public:
							NewCelImage();
	virtual					~NewCelImage();

	virtual void			Draw(Rectangle* r, const Rectangle* invalid, ulong transparency = 0);
	virtual Boolean			DrawingIdle(Rectangle* r);			// returns true if it drew something
	virtual Boolean			GetBounds(Rectangle*);				// returns true if bounds is known
	virtual Error			GetStatus() const;

	virtual Boolean			Idle(Layer*, Coordinate *delta, ulong* scale);
	virtual Boolean			PushData(Boolean sizeOnly = false, Boolean alpha = false);
	
	virtual void			SetShouldLoop(Boolean);
	virtual void			SetFlip(Boolean horizontal,Boolean vertical);
	virtual void			Start();
	virtual void			Stop();

protected:
	void					Reset();
	Boolean					SetCurrentFrame(ulong newFrame, Rectangle *destinationBounds = nil);
	Boolean					ParseData();
	void					BuildLoopFrame();
	Boolean					PopLoopFrame(DataList* loopFrames);
	void					PushLoopFrame(DataList* loopFrames, ulong maxCount);

protected:
	DataList				fFrameList;
	ObjectList				fImageList;
	GIFImage*				fImageData;
	ulong					fNextIdle;
	NewCelState				fState;

	short					fCurrentFrame;
	Boolean					fRunning;
	Boolean					fShouldLoop;

#ifdef SUPPORTS_BITFIELDS
	unsigned				fShouldSwing: 1;
	unsigned				fShouldHFlip: 1;
	unsigned				fShouldVFlip: 1;
	unsigned				fShouldHBounce: 1;
	unsigned				fShouldVBounce: 1;
	unsigned				fBackwards: 1;
#else
	Boolean					fShouldSwing;
	Boolean					fShouldHFlip;
	Boolean					fShouldVFlip;
	Boolean					fShouldHBounce;
	Boolean					fShouldVBounce;
	Boolean					fBackwards;
#endif
};

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include AnimationData.h multiple times"
	#endif
#endif /* __ANIMATIONDATA_H__ */
