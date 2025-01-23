// ===========================================================================
//	LoginPanel.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __AUTHORIZATIONL_H__
#include "Authorization.h"
#endif
#ifndef __LOGINPANEL_H__
#include "LoginPanel.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif

#include "PageViewer.h"
#include "System.h"

// =============================================================================

LoginPanel::LoginPanel()
{
	IsError(gLoginPanel != nil);
	gLoginPanel = this;
	fPageResource.SetURL(kLoginPanelURL);
}

LoginPanel::~LoginPanel()
{
	IsError(gLoginPanel != this);
	gLoginPanel = nil;
}

void
LoginPanel::Close(Boolean slide)
{
	if (!fIsVisible)
		return;
		
	Panel::Close(slide);
	
	if (!fDidLogin) {
		gPageViewer->BackInput();
		fDestination.SetStatus(kAuthorizationError);
	}
	
	if (fRealm != nil) {
		FreeTaggedMemory(fRealm, "LoginPanel::fRealm");
		fRealm = nil;
	}
}

void
LoginPanel::Login(const char* name, const char* password)
{
	gAuthorizationList->Add(&fDestination, fRealm, name, password);
	gPageViewer->ShowResource(&fDestination);
	fDidLogin = true;
	Close();
}

void
LoginPanel::Open()
{
	Panel::Open();
	fDidLogin = false;
}

void
LoginPanel::SetDestination(const Resource* resource)
{
	if (IsError(resource == nil))
		return;
	
	fDestination = *resource;
}

void
LoginPanel::SetRealm(const char* value)
{
	fRealm = CopyStringTo(fRealm, value, "LoginPanel::fRealm");
}

// =============================================================================

