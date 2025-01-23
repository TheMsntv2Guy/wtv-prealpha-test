// ===========================================================================
//	OptionsPanel.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __OPTIONSPANEL_H__
#define __OPTIONSPANEL_H__

#ifndef __PANEL_H__
#include "Panel.h"
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"
#endif

class Animation;
class ImageData;
class Layer;

// ===========================================================================

class OptionsPanel : public Panel {
public:
							OptionsPanel();
	virtual					~OptionsPanel();

	virtual Boolean			BackInput();	
	virtual void			ChangeIdleState(ViewIdleState newState, ViewIdleState oldState);
	virtual void			Close(Boolean slide = true);	
	virtual void			Draw(const Rectangle*);
	void					DrawOdometer(const Rectangle*);
#ifdef FIDO_INTERCEPT
	virtual void			Draw(class FidoCompatibilityState& fidoCompatibility) const;
	void					DrawOdometer(class FidoCompatibilityState& fidoCompatibility) const;
#endif
	virtual Boolean			ExecuteInput();
	virtual Layer*			GetTopLayer() const;
	void					Idle();
	Boolean					IsEnabled() const;
	Boolean					IsShown() const;
	virtual void			Open();
	void					PhoneStatusChanged();
	void					ScrollStatusChanged();
	void 					SetMessage(const char* message, Boolean shouldCopy = false);
	void					SetPercentDone(ulong);
	void					SetShowAdvanced(Boolean);
	void					SetShown(Boolean);
	void					SetTarget(const Resource*);
	Boolean					ShowAdvanced() const;
	void					StartProgress();
	void					StopProgress();
	void					StartPhoneProgress();
	void					StopPhoneProgress();
	void					TitleChanged();
	void					SetIsPhoneInProgress(Boolean state);
	Boolean					GetIsPhoneInProgress();

protected:
	void					DrawProgress(const Rectangle*);
	void					DrawScrollArrows(const Rectangle*);
	void					DrawTitle(const Rectangle*);
#ifdef FIDO_INTERCEPT
	void					DrawProgress(class FidoCompatibilityState& fidoCompatibility) const;
	void					DrawScrollArrows(class FidoCompatibilityState& fidoCompatibility) const;
	void					DrawTitle(class FidoCompatibilityState& fidoCompatibility) const;
#endif

protected:						
	Resource				fTargetResource;
	ImageData*				fOptionsBar;
	ImageData*				fAdvancedBar;
	ImageData*				fOptionsDrawer;
	ImageData*				fTitleField;
	ImageData*				fPhoneConnected;
	ImageData*				fPhoneDisconnected;
	ImageData*				fScrollDownLight;
	ImageData*				fScrollUpLight;
	ImageData*				fScrollArrows;
	ImageData*				fCapsLock;
	Animation*				fPhoneInProgress;
	const char*				fMessage;
	ulong					fLastIdle;
	uchar					fPercentDone;
	
	unsigned				fRefreshSelections : 1;
	unsigned				fInProgress : 1;
	unsigned				fIsPhoneInProgress : 1;
	unsigned				fMessageCopied : 1;
	unsigned				fIsCapsLocked : 1;
	unsigned				fShown : 1;
	unsigned				fShowAdvanced : 1;
};

// ===========================================================================

inline Boolean
OptionsPanel::IsEnabled() const
{
	return IsVisible();
}

// ===========================================================================
#endif /* __OPTIONSPANEL_H__ */
