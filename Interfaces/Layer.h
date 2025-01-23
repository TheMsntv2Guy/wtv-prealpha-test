// ===========================================================================
//	Layer.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __LAYER_H__
#define __LAYER_H__

#ifndef __GRAPHICS_H__
#include "Graphics.h"
#endif
#ifndef __INPUT_H__
#include "Input.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif
#ifndef __LINKABLE_H__
#include "Linkable.h"
#endif
#ifndef __REGION_H__
#include "Region.h"
#endif

class ContentView;
class TextField;

// =============================================================================

class Layer : public Linkable,  public HandlesInput, public HasBounds {
public:
							Layer();
	virtual					~Layer();

	TextField*				GetKeyboardTarget() const;
	ContentView*			GetTopContentView() const;
	virtual Layer*			GetTopLayer() const;
	void					InvalidateBounds(const Rectangle* bounds = nil);
	void					InvalidateAbove(const Rectangle* bounds);
	void					InvalidateBehind(const Rectangle* bounds);
	virtual Boolean			IsContentView() const;
	Boolean					IsVisible() const;

	void					DrawBehind();
	void					DrawLayer();
#ifdef FIDO_INTERCEPT
	void					DrawLayer(class FidoCompatibilityState& fidoCompatibility) const;
#endif
	virtual void			Hide();
	void					IdleLayer();
	virtual void			Show();

	// еее <UNUSED_fLayerID> long					GetLayerID()	{ return (long)fLayerID; };

	Layer*					GetChild() const;
	Layer*					GetParent() const;	
	void					AddChild(Layer*, long layerID = 0);

protected:
	virtual void			Draw(const Rectangle* invalid);
#ifdef FIDO_INTERCEPT
	virtual void			Draw(class FidoCompatibilityState& fidoCompatibility) const;
#endif
	void					DrawLayerInBounds(const Rectangle*);
	virtual void			Idle();
	inline Boolean			IsWithinBounds(const Rectangle* bounds) const;
	inline Boolean			Contains(const Rectangle* bounds) const;
		
	Region					fInvalidRegion;
	Layer*					fChild;
	Layer*					fParent;
	Boolean					fIsVisible;
	Boolean					fPartiallyTransparent;
	// еее <UNUSED_fLayerID> short					fLayerID;
};

// =============================================================================

#endif /* __LAYER_H__ */
