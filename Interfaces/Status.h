// ===========================================================================
//	Status.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __STATUS_H__
#define __STATUS_H__

#ifndef __GRAPHICS_H__
#include "Graphics.h"		/* for Rectangle */
#endif
#ifndef __LAYER_H__
#include "Layer.h"			/* for Layer */
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"		/* for Resource */
#endif

class Animation;
class ImageData;

//==================================================================================

class Indicator : public Layer {
public:
							Indicator();
	virtual					~Indicator();
	
	virtual Layer*			GetTopLayer() const;
	Boolean					IsDisabled() const;
	
	void					SetDisabled(Boolean);
	void					SetMessage(const char*, Boolean shouldCopy = false);	// note: holds on to pointer
	void					SetPercentDone(long);
	void					SetTarget(const Resource*);

	virtual void			Draw(const Rectangle*);
#ifdef FIDO_INTERCEPT
	virtual void			Draw(class FidoCompatibilityState& fidoCompatibility) const;
#endif
	virtual void			Hide();
	virtual void			Show();

protected:
	virtual void			GetPercentDoneBounds(Rectangle*) const;
	const char*				fMessage;
	Resource				fTargetResource;

	Boolean					fDisabled;
	Boolean					fMessageCopied;
	char					fPercentDone;
};

class StatusIndicator : public Indicator {
public:
							StatusIndicator();
	virtual					~StatusIndicator();
	
	virtual void			Draw(const Rectangle*);
#ifdef FIDO_INTERCEPT
	virtual void			Draw(class FidoCompatibilityState& fidoCompatibility) const;
#endif
	virtual void			Hide();
	virtual void			Idle();
	virtual void			Show();

	virtual void			GetPercentDoneBounds(Rectangle*) const;

protected:
	ImageData*				fBackground;
	Animation*				fImages;
	ulong					fLastIdle;

	Boolean					fDrawShadowOnShow;
};

class ConnectIndicator : public Indicator {
public:
							ConnectIndicator();
	virtual					~ConnectIndicator();

	virtual void			Draw(const Rectangle*);
#ifdef FIDO_INTERCEPT
	virtual void			Draw(class FidoCompatibilityState& fidoCompatibility) const;
#endif
	
protected:
	virtual void			GetPercentDoneBounds(Rectangle*) const;
};

//==================================================================================

#endif /* __STATUS_H__ */
