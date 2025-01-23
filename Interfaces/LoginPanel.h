// ===========================================================================
//	LoginPanel.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __LOGINPANEL_H__
#define __LOGINPANEL_H__

#ifndef __PANEL_H__
#include "Panel.h"
#endif

// =============================================================================

class LoginPanel : public Panel {
public:
							LoginPanel();
	virtual					~LoginPanel();
	
	virtual void			Close(Boolean slide = true);	
	virtual void			Open();
	void					SetDestination(const Resource*);
	void					SetRealm(const char*);
	
	void					Login(const char* name, const char* password);

protected:
	Resource				fDestination;
	Boolean					fDidLogin;
	char*					fRealm;
};

// =============================================================================

#endif /* __LOGINPANEL_H__ */
