// ===========================================================================
//	InfoPanel.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __INFOPANEL_H__
#define __INFOPANEL_H__

#ifndef __PANEL_H__
#include "Panel.h"
#endif

// =============================================================================

class InfoPanel : public Panel {
public:
							InfoPanel();
	virtual					~InfoPanel();
	
	virtual Boolean			ExecuteInput();
	virtual void			ExecuteURL(const char* url, const char* formData);
	virtual void			Open();
	virtual void			WritePage();
};

// =============================================================================

#endif /* __INFOPANEL_H__ */
