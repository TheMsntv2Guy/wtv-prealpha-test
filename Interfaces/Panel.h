// ===========================================================================
//	Panel.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __PANEL_H__
#define __PANEL_H__

#ifndef __CONTENTVIEW_H__
#include "ContentView.h"
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"
#endif

// =============================================================================

class Panel : public ContentView {
public:
							Panel();
	virtual					~Panel();

	Boolean					IsOpen() const;
	void					SetIsOpen(Boolean);
	void					SetURL(const char*);

	virtual Boolean			BackInput();
	virtual void			ChangeIdleState(ViewIdleState newState, ViewIdleState oldState);
	virtual void			Close(Boolean slide = true);
	virtual Boolean			DispatchInput(Input*);
	virtual void			Draw(const Rectangle*);
#ifdef FIDO_INTERCEPT
	virtual void			Draw(class FidoCompatibilityState& fidoCompatibility) const;
#endif
	virtual Boolean			ExecuteInput();
	virtual void			ExecuteURL(const char* url, const char* formData);
	virtual void			Open();					
	virtual void			WritePage();
	
protected:
	Resource				fPageResource;
	Boolean					fIsOpen;
};

// =============================================================================

#endif /* __PANEL_H__ */

