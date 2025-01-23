// Copyright(c) 1996 Artemis Research, Inc. All rights reserved.

#ifndef __CLOCKFIELD_H__
#define __CLOCKFIELD_H__

#ifndef __CONTROL_H__
#include "Control.h"
#endif

#ifndef __IMAGE_H__
#include "Image.h"
#endif

enum { kTimeField,kDateField };

// ===========================================================================

class ClockField : public Control 
{
public:
							ClockField();
	virtual 				~ClockField();
	virtual void			IClockField(int type);
	virtual void			Draw(const Document* document, const Rectangle* invalid);
	virtual Boolean			Idle(Layer* layer);
	virtual void			Layout(Document*, Displayable* parent);
	virtual Boolean			IsSelectable() const;
	
protected:
	ulong					fCurrentTime;
	Layer*					fLayer;
	int						fType;	// date or time
};

// ===========================================================================

#endif /*__CLOCKFIELD_H__ */