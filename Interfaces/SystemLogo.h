#ifndef __SYSTEMLOGO_H__
#define __SYSTEMLOGO_H__

#ifndef __LAYER_H__
#include "Layer.h"
#endif

class SystemLogo : public Layer {
public:
							SystemLogo();
	virtual 				~SystemLogo();
	
	virtual Layer*			GetTopLayer() const;

	virtual void			Draw(const Rectangle* invalid);
#ifdef FIDO_INTERCEPT
	virtual void			Draw(class FidoCompatibilityState& fidoCompatibility) const;
#endif
	void					DrawLogoWithTransparency(ushort transparency);
	virtual void			Idle();
	void					StartFading();
	
protected:
	class ImageData*		fLogoImage;
	ulong					fFadeStartTime;		// time logo first shown
	short					fCurrentLogoTransparency;
};

#endif /* __SYSTEMLOGO_H__ */
