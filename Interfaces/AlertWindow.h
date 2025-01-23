// Copyright (c) 1995 Artemis Research, Inc. All rights reserved.

#ifndef __ALERTVIEWER_H__
#define __ALERTVIEWER_H__

#ifndef __CONTENTVIEW_H__
#include "ContentView.h"
#endif

//==================================================================================

class Window : public ContentView {
public:
							Window();
	virtual					~Window();
	
	virtual void			SetURL(const char*);

	virtual void			Draw(const Rectangle*);
#ifdef FIDO_INTERCEPT
	virtual void			Draw(class FidoCompatibilityState& fidoCompatibility) const;
#endif
	virtual void			Hide();
	
protected:
	void					DrawShadow();
#ifdef FIDO_INTERCEPT
	void					DrawShadow(class FidoCompatibilityState& fidoCompatibility) const;
#endif
	
protected:
	Resource				fPageResource;
	Boolean					fShouldDrawShadow;
};

//==================================================================================

typedef enum {
	kNoAction = 0,
	kStartup,
	kPreviousPage,
	kTargetURL,
	kPowerOff
} AlertAction;

class GenericAlertWindow : public Window {
public:
							GenericAlertWindow();
	virtual					~GenericAlertWindow();

	const char*				GetMessage() const;
	const char*				GetServerMessage() const;
	
	virtual void			SetAction(AlertAction);
	void					SetBackDisabled(Boolean);
	void					SetError(Error);
	void					SetErrorResource(const Resource*);
	void					SetServerMessage(const char*);
	void					SetTargetFragment(const char*);
	virtual void			SetURL(const char*);

	virtual Boolean			BackInput();
	virtual void			ChangeIdleState(ViewIdleState newState, ViewIdleState oldState);
	virtual Boolean			ExecuteInput();
	virtual void			Hide();
	virtual Boolean			OptionsInput();
	void					Reset();
	virtual void			Show();

protected:
	const char*				GetActionURL() const;
	virtual void			MakePage();
	
protected:
	char*					fServerMessage;
	char*					fTargetFragment;
	Resource				fErrorResource;
	AlertAction				fAction;
	Error					fError;
	Boolean					fShouldPlaySound;
	Boolean					fShouldMakePage;
	Boolean					fBackDisabled;
};

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
inline const char*
GenericAlertWindow::GetServerMessage() const
{
	return fServerMessage;
}
#endif

inline void		GenericAlertWindow::SetBackDisabled(Boolean value)	{ fBackDisabled = value; }

//==================================================================================

class SplashWindow : public Window {
public:
							SplashWindow();
	virtual					~SplashWindow();

	virtual Boolean			DispatchInput(Input*);
	virtual void			Idle();	
	virtual void			Show();

	void					SetSendResource(const Resource*);
	
protected:	
	Resource				fSendResource;
	ulong					fHideTime;
};

//==================================================================================

class AlertWindow : public GenericAlertWindow {
public:
							AlertWindow();
	virtual					~AlertWindow();

	virtual void			MakePage();
};

class ConnectWindow : public GenericAlertWindow {
public:
							ConnectWindow();
	virtual					~ConnectWindow();

	virtual Boolean			ExecuteInput();
	virtual void			MakePage();
};

class PowerOffAlert : public GenericAlertWindow {
public:
							PowerOffAlert();
	virtual					~PowerOffAlert();

	virtual void			MakePage();
};

//==================================================================================
#endif /* __ALERTVIEWER_H__ */
