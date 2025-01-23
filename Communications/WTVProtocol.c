// ===========================================================================
//	WTVProtocol.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __ALERTWINDOW_H__
#include "AlertWindow.h"
#endif
#ifndef __CACHE_H__
#include "Cache.h"
#endif
#ifndef __CACHESTREAM_H__
#include "CacheStream.h"
#endif
#ifndef __CLOCK_H__
#include "Clock.h"
#endif
#ifndef __DATETIME_H__
#include "DateTime.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __NETWORK_H__
#include "Network.h"
#endif
#ifndef __PAGEVIEWER_H__
#include "PageViewer.h"
#endif
#ifndef __SOCKET_H__
#include "Socket.h"
#endif
#ifndef __STREAM_H__
#include "Stream.h"
#endif
#ifndef __SYSTEM_H__
#include "System.h"
#endif
#ifndef __URLPARSER_H__
#include "URLParser.h"
#endif
#ifndef __WTVPROTOCOL_H__
#include "WTVProtocol.h"
#endif

// =============================================================================
// Global variables

Resource* WTVPCommand::gWTVAttributeResource;
int	gWTVIncarnation = 1;

// =============================================================================
// Class WTVPCommand

WTVPCommand::WTVPCommand()
{
}

WTVPCommand::~WTVPCommand()
{
}

void
WTVPCommand::AddWTVAttribute(const char* name, const char* value)
{
	DataStream* stream = gWTVAttributeResource->NewStreamForAppend();
	
	if (IsError(stream == nil))
		return;
	
	Message(("WTVP: %s: %s", name, value));
	
	PostulateFinal(false); // check to see if name is already there
	stream->FastForward();
	stream->WriteString(name);
	stream->WriteString(":");
	stream->WriteString(value);
	stream->WriteString("\n");
	delete(stream);
}

#ifdef INCLUDE_FINALIZE_CODE
void
WTVPCommand::Finalize()
{
	delete(gWTVAttributeResource);
	gWTVAttributeResource = nil;
}
#endif /* INCLUDE_FINALIZE_CODE */

ClassNumber
WTVPCommand::GetClassNumber() const
{
	return kClassNumberWTVPCommand;
}

void
WTVPCommand::IdleReadBody()
{
	if (fIsEncrypted) {
		RC4Stream* stream = new(RC4Stream);
		RC4_KEY* key;
			
		((WTVProtocol*)GetProtocol())->GetSessionKeys(NULL,&key);
		stream->SetKey(key);
		stream->Add(fStream);
		fStream = stream;
		fIsEncrypted = false;
	}
	
	HTTPCommand::IdleReadBody();
}

void
WTVPCommand::IdleReadResponse()
{
	char response[512];
	Error error;
	int responseCode;
	char* url;
	char* serverMessage;
	int count;

	switch (error = GetSocket()->ReadLine(response, sizeof (response))) {
	case kNoError:
		ResetTimeout();
		break;
		
	case kPending:
		ResetTimeout();
		return;
		
	case kLostConnection:
	case kNoConnection:
		url = fResource.CopyURL("IdleReadResponse");
		ImportantMessage(("WTVP: lost connection to %s", url));
		FreeTaggedMemory(url, "IdleReadResponse");
		fStream->SetStatus(kStreamReset);
		fState = kHTTPError;
		return;
		
	default:
		fStream->SetStatus(error);
		fState = kHTTPError;
		return;
	}
		
	if (*response == 0)
		return;
		
	if (sscanf(response, "%ld%n", &responseCode, &count) != 1) {
		Message(("WTVP: ignoring %s", response));
		return;
	}
	
	if (responseCode >= 200 && responseCode < 300) {
		fState = kHTTPReadHeader;
		return;
	}
	
	if (responseCode >= 300 && responseCode < 400) {
		fState = kHTTPReadHeaderAndRedirect;
		return;
	}

	switch (responseCode) {
	case 401:
		fState = kHTTPReadHeaderAndAuthorize;
		return;
		
	case 404:
		url = fResource.CopyURL("IdleReadResponse");
		ImportantMessage(("WTVP: resource %s cannot be found", url));
		FreeTaggedMemory(url, "IdleReadResponse");
		break;

	case 508:
		url = fResource.CopyURL("IdleReadResponse");
		ImportantMessage(("WTVP: Expired ticket error 508: %s", url));
		FreeTaggedMemory(url, "IdleReadResponse");
		
			/* Mac : old ticket, need to follow visit to head-waiter */
		break;

	case 509:
		if (GetService()->IsNamed("wtv-head-waiter")) {
			url = fResource.CopyURL("IdleReadResponse");
			ImportantMessage(("WTVP: Strong authentication error 509 : %s", url));
			FreeTaggedMemory(url, "IdleReadResponse");
			
			/* Mac : need to go back to pre-register */
			break;
		}

	default:
		url = fResource.CopyURL("IdleReadResponse");
		ImportantMessage(("WTVP: server error %d for %s", responseCode, url));
		FreeTaggedMemory(url, "IdleReadResponse");
	}
	
	serverMessage = response + count;
	serverMessage = SkipCharacters(serverMessage, " ");
	gAlertWindow->Reset();
	gAlertWindow->SetServerMessage(serverMessage);
	fState = kHTTPReadHeaderAndFail;
}

void
WTVPCommand::Initialize()
{
	gWTVAttributeResource = new(Resource);
	gWTVAttributeResource->SetURL("cache:wtv-attributes");
	gWTVAttributeResource->SetDataType((DataType)0);
	gWTVAttributeResource->SetPriority(kPersistent);
	gWTVAttributeResource->SetStatus(kComplete);
}

void
WTVPCommand::RedirectWithData(const char* url)
{
	Resource resource;
	
	if (IsError(fStream == nil || url == nil))
		return;
	
	// Redirect current resource to new URL.
	Redirect(url);
	
	// Create cache entry for new URL.
	URLParser urlParser;
	urlParser.SetURL(url);
	resource.SetURL(urlParser.GetURLNoFragment());
	
	// If it is complete, we are done.
	if (resource.GetStatus() >= kComplete)
		return;
	
	// Otherwise, set to new URL.
	SetResource(&resource);
	if (!CreateStream()) {
		IsWarning("RedirectWithData: cannot create stream");
		fState = kHTTPError;
		return;
	}
	
	// Start receiving data for new URL.
	fStream->SetDataExpected(CacheEntry::kDataExpectedUnknown);
	fStream->SetStatus(kPending);
}	

Boolean
WTVPCommand::SetAttribute(const char* name, char* value)
{
	if (gSystem->SetAttribute(name, value))
		return true;
	
	if (gNetwork->SetAttribute(name, value))
		return true;
	
	if (gServiceList->SetAttribute(name, value))
		return true;
	
	if (gClock->SetAttribute(name, value))
		return true;

	if (gRAMCache->SetAttribute(name, value))
		return true;

	if (HTTPCommand::SetAttribute(name, value))
		return true;
	
	if (EqualString(name, "wtv-location")) {
		RedirectWithData(value);
		return true;
	}	
	
	if (EqualString(name, "wtv-field-name")) {
		gAlertWindow->SetTargetFragment(value);
		return true;
	}
	
	if (EqualString(name, "wtv-encrypted")) {
		fIsEncrypted = true;
		fResource.SetIsDataTrusted(true);
		return true;
	}
	
	if (EqualString(name, "wtv-trusted")) {
		fResource.SetIsDataTrusted(false);
		return true;
	}

	if (GetService()->IsNamed("wtv-1800")) {
		if (gNetwork->SetSpecialAttribute(name, value))
			return true;
	}
	
	if (EqualStringN(name, "wtv-", 4)) {
		AddWTVAttribute(name, value);
		return true;
	}
	
	return false;
}

void 
WTVPCommand::WriteAttributes(Stream* stream)
{
	DataStream* wtvStream;
	
	PostulateFinal(false);	// call fProtocol->ShouldWriteHeaders
		
	HTTPCommand::WriteAttributes(stream);
	gSystem->WriteAttributes(stream);
	gServiceList->WriteAttributes(stream);
	gNetwork->WriteAttributes(stream);	
	gClock->WriteAttributes(stream);
	
	if ((wtvStream = gWTVAttributeResource->NewStream()) != nil) {
		stream->Write(wtvStream->GetData(), wtvStream->GetDataLength());
		delete(wtvStream);
	}
}

// =============================================================================
// Class WTVPGetCommand

WTVPGetCommand::WTVPGetCommand()
{
}

WTVPGetCommand::~WTVPGetCommand()
{
}

ClassNumber
WTVPGetCommand::GetClassNumber() const
{
	return kClassNumberWTVPGetCommand;
}

void WTVPGetCommand::IdleWriteCommandBegin()
{
	Stream* stream;
	
	if (GetService()->IsEncrypted()) {
		RC4Stream* rc4stream = new(RC4Stream);
		RC4_KEY* key;
		
		((WTVProtocol*)GetProtocol())->GetSessionKeys(&key, NULL);
		rc4stream->SetKey(key);
		rc4stream->Add(&fCommandStream);
		stream = rc4stream;
	}
	else
		stream = &fCommandStream;
		
	
	stream->WriteString("GET ");
	stream->WriteString(fStream->GetName());
	stream->WriteString("\r\n");
	WriteAttributes(stream);
	stream->WriteString("\r\n");
	
	if (GetService()->IsEncrypted()) {
		stream->RemoveAllAfter();
		delete(stream);
	}
	
	fState = kHTTPWriteCommandEnd;
	IdleWriteCommandEnd();
}

// =============================================================================
// Class WTVPPostCommand

WTVPPostCommand::WTVPPostCommand()
{
}

WTVPPostCommand::~WTVPPostCommand()
{
}

ClassNumber
WTVPPostCommand::GetClassNumber() const
{
	return kClassNumberWTVPPostCommand;
}

void WTVPPostCommand::IdleWriteCommandBegin()
{
	Stream* stream;
	
	if (GetService()->IsEncrypted()) {
		RC4Stream* rc4stream = new(RC4Stream);
		RC4_KEY* key;
		
		((WTVProtocol*)GetProtocol())->GetSessionKeys(&key, NULL);
		rc4stream->SetKey(key);
		rc4stream->Add(&fCommandStream);
		stream = rc4stream;
	}
	else
		stream = &fCommandStream;

	stream->WriteString("POST ");
	stream->WriteString(fStream->GetName());
	stream->WriteString("\r\n");
	WriteAttributes(stream);
	stream->WriteAttribute("Content-type", "application/x-www-form-urlencoded");
	stream->WriteAttribute("Content-length", fResource.GetPostDataLength());
	stream->WriteString("\r\n");
	stream->Write(fResource.GetPostData(), fResource.GetPostDataLength());
	
	if (GetService()->IsEncrypted()) {
		stream->RemoveAllAfter();
		delete(stream);
	}
	
	fState = kHTTPWriteCommandEnd;
	IdleWriteCommandEnd();
}

// =============================================================================
// Class WTVProtocol

WTVProtocol::~WTVProtocol()
{
}

ClassNumber
WTVProtocol::GetClassNumber() const
{
	return kClassNumberWTVProtocol;
}

void
WTVProtocol::IdleCommandEnd()
{
	HTTPProtocol::IdleCommandEnd();

	// Close if socket does not look ok.
	if (fSocket->FIFOCount() != 0) {
		Message(("WTVProtocol: socket still contains %d bytes, closing", fSocket->FIFOCount()));
		SetState(kProtocolDisconnectBegin);
		return;
	}
	
	SetState(kProtocolSecureBegin);
}

void
WTVProtocol::IdleSecureBegin()
{
	if (GetService()->IsEncrypted()) {
		fSecureStream.WriteString("SECURE ON");
		fSecureStream.WriteString("\r\n");
		gNetwork->WriteAttributes(&fSecureStream);
		fSecureStream.WriteString("wtv-incarnation:");
		fIncarnation = gWTVIncarnation++;
		fSecureStream.WriteNumeric(fIncarnation); 
		fSecureStream.WriteString("\r\n");
		fSecureStream.WriteString("\r\n");
		SetState(kProtocolSecureEnd);
		IdleSecureEnd();
		return;
	}
	
	SetState(kProtocolReady);
	return;
}

void
WTVProtocol::IdleSecureEnd()
{	
	Error error;
	long pending;
	long used;
	
	if ((pending = fSecureStream.GetPending()) != 0) {
		error = GetSocket()->Write(fSecureStream.ReadNext(0), pending, &used);
		fSecureStream.ReadNext(used);
	} else
		error = GetSocket()->Idle();
	
	switch (error) {
	case kNoError:
		break;
		
	case kPending:
		return;
		
	case kLostConnection:
	case kNoConnection:
		SetState(kProtocolDisconnectBegin);
		return;
	
	default:
		SetState(kProtocolDisconnectBegin);
		return;
	}
	
	fSecureStream.Reset();
	SetSessionKeys();
	SetState(kProtocolReady);
}

Error
WTVProtocol::SetSessionKeys()
{
	uchar* key;
	
	key = gNetwork->GetSessionKey1(fIncarnation);
	RC4_set_key(&fSendKey, 16, key);
	
	key = gNetwork->GetSessionKey2(fIncarnation);
	RC4_set_key(&fReceiveKey, 16, key);

	return kNoError;

}

void
WTVProtocol::GetSessionKeys(RC4_KEY** SendKey, RC4_KEY** ReceiveKey)
{
	if (SendKey)
		*SendKey = &fSendKey;
	if (ReceiveKey)
		*ReceiveKey = &fReceiveKey;
}

ProtocolCommand*
WTVProtocol::NewGetCommand(const Resource* resource)
{
	ProtocolCommand* command = new(WTVPGetCommand);
	command->SetResource(resource);
	return command;
}

ProtocolCommand*
WTVProtocol::NewPostCommand(const Resource* resource)
{
	ProtocolCommand* command = new(WTVPPostCommand);
	command->SetResource(resource);
	return command;
}

//===========================================================================
