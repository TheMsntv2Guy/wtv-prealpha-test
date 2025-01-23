// ===========================================================================
//	HTTP.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"
#ifndef __ALERTWINDOW_H__
#include "AlertWindow.h"
#endif
#ifndef __AUTHORIZATION_H__
#include "Authorization.h"
#endif
#ifndef __CLOCK_H__
#include "Clock.h"
#endif
#ifndef __COOKIE_H__
#include "Cookie.h"
#endif
#ifndef __DATETIME_H__
#include "DateTime.h"
#endif
#ifndef __HTTP_H__
#include "HTTP.h"
#endif
#ifndef __LOGINPANEL_H__
#include "LoginPanel.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __PERFDUMP_H__
#include "PerfDump.h"
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"
#endif
#ifndef __SOCKET_H__
#include "Socket.h"
#endif
#ifndef __STATUS_H__
#include "Status.h"
#endif
#ifndef __SYSTEM_H__
#include "System.h"
#endif
#ifndef __URLPARSER_H__
#include "URLParser.h"
#endif

// ===========================================================================
// Global variables, local constants

MemoryStream* HTTPProtocol::gMemoryStream = nil;

// ===========================================================================
// Class HTTPCommand

HTTPCommand::HTTPCommand()
{
}

HTTPCommand::~HTTPCommand()
{
}

ClassNumber
HTTPCommand::GetClassNumber() const
{
	return kClassNumberHTTPCommand;
}

long
HTTPCommand::GetState() const
{
	return (long)fState;
}

#ifdef DEBUG_NAMES
const char*
HTTPCommand::GetStateAsString() const
{
	switch (fState) {
	case kHTTPCreateStream:				return "create stream";
	case kHTTPGetCookieBegin:			return "get cookie begin";
	case kHTTPGetCookieEnd:				return "get cookie end";
	case kHTTPWriteCommandBegin:		return "write command begin";
	case kHTTPWriteCommandEnd:			return "write command end";
	case kHTTPWriteBody:				return "write body";
	case kHTTPReadResponse:				return "read response";
	case kHTTPReadHeader:				return "read header";
	case kHTTPReadHeaderAndAuthorize:	return "read header/authorize";
	case kHTTPReadHeaderAndFail:		return "read header/fail";
	case kHTTPReadHeaderAndRedirect:	return "read header/redirect";
	case kHTTPReadBody:					return "read body";
	case kHTTPComplete:					return "complete";
	case kHTTPError:					return "error";
	}
	
	IsError("HTTPCommand: unknown state");
	return "unknown";					
}
#endif /* DEBUG_NAMES */

Boolean
HTTPCommand::Idle()
{
	PerfDump perfdump("HTTPCommand::Idle");
#ifdef DEBUG_CACHE_VALIDATE
	if (IsError(!IsValid()))
		return false;
#endif

	if (fStream != nil && fStream->GetStatus() != kPending)
		fState = kHTTPComplete;
	
	if (IsTimedOut()) {
		if (GetService() != nil)
			GetService()->Suspend();
		fResource.SetStatus(kTimedOut);
		fState = kHTTPError;
	}

	switch (fState) {
	case kHTTPCreateStream:				IdleCreateStream(); return true;
	case kHTTPGetCookieBegin:			IdleGetCookieBegin(); return true;
	case kHTTPGetCookieEnd:				IdleGetCookieEnd(); return true;
	case kHTTPWriteCommandBegin:		IdleWriteCommandBegin(); return true;
	case kHTTPWriteCommandEnd:			IdleWriteCommandEnd(); return true;
	case kHTTPWriteBody:				IdleWriteBody(); return true;
	case kHTTPReadResponse:				IdleReadResponse(); return true;
	case kHTTPReadHeaderAndAuthorize:
	case kHTTPReadHeaderAndFail:
	case kHTTPReadHeaderAndRedirect:
	case kHTTPReadHeader:				IdleReadHeader(); return true;
	case kHTTPReadBody:					IdleReadBody(); return true;
	case kHTTPComplete:					break;
	case kHTTPError:					break;
	default:							IsError(fState);
	}
	
	return false;					
}

void
HTTPCommand::IdleCreateStream()
{
	if (!CreateStream())
		return;
	
	fState = kHTTPGetCookieBegin;
	IdleGetCookieBegin();
}

void
HTTPCommand::IdleGetCookieBegin()
{
	if (!gCookieList->LoadCookie(&fResource)) {
		fState = kHTTPWriteCommandBegin;
		IdleWriteCommandBegin();
		return;
	}
	
	fState = kHTTPGetCookieEnd;
	IdleGetCookieEnd();
}
	
void
HTTPCommand::IdleGetCookieEnd()
{
	if (gCookieList->LoadCookie(&fResource))
		return;
		
	fState = kHTTPWriteCommandBegin;
	IdleWriteCommandBegin();
}

void
HTTPCommand::IdleReadBody()
{
	Error error;
	Socket* socket;
	
	if (CheckBodyComplete())
		return;
	
	socket = GetSocket();
	if (IsError(socket == nil)) {
		fStream->SetStatus(kStreamReset);
		fState = kHTTPComplete;
		return;
	}
	
	switch (error = socket->ReadIntoStream(fStream)) {
	case kNoError:
		ResetTimeout();
		break;
		
	case kPending:
		return;
	
	case kLostConnection:
		fStream->SetStatus(kStreamReset);
		fState = kHTTPComplete;
		return;
		
	case kNoConnection:
		Message(("HTTP: lost connection, assuming %s is complete", fResource.GetURL()));
		fStream->SetStatus(kComplete);
		fState = kHTTPComplete;
		return;

	default:
		fStream->SetStatus(error);
		fState = kHTTPError;
		return;
	}
	
	CheckBodyComplete();
}

void
HTTPCommand::IdleReadHeader()
{
	char header[512];
	Error error;
	
	while (fStream != nil && fStream->GetStatus() == kPending) {
		switch (error = GetSocket()->ReadLine(header, sizeof (header))) {
		case kNoError:
			ResetTimeout();
			break;

		case kPending:
			return;
			
		case kLostConnection:
		case kNoConnection:
			ImportantMessage(("HTTP: lost connection to %s", fResource.GetURL()));
			fStream->SetStatus(kStreamReset);
			fState = kHTTPError;
			return;

		default:
			fStream->SetStatus(error);
			fState = kHTTPError;
			return;
		}
		
		if (*header == 0) {
			if (fState == kHTTPReadHeaderAndFail) {
				fStream->SetStatus(kResponseError);
				fState = kHTTPError;
				return;
			}
			
			fState = kHTTPReadBody;
			IdleReadBody();
			return;
		}
		
		SetAttributeString(header);
	}
}

void
HTTPCommand::IdleReadResponse()
{
	char response[1024];
	Error error;
	int responseCode;
	
	switch (error = GetSocket()->ReadLine(response, sizeof (response))) {
	case kNoError:
		ResetTimeout();
		break;
		
	case kPending:
		return;
		
	case kLostConnection:
	case kNoConnection:
		Message(("HTTP: lost connection to %s", fResource.GetURL()));
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
	
	if (sscanf(response, "HTTP/1.0 %ld", &responseCode) != 1) {
		Message(("HTTP: ignoring %s", response));
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
		ImportantMessage(("HTTP: resource cannot be found"));
		error = kPageNotFound;
		break;
	default:
		ImportantMessage(("HTTP: server error %d", responseCode));
		error = kResponseError;
	}
	
	fStream->SetStatus(error);
	fState = kHTTPError;
}

void
HTTPCommand::IdleWriteBody()
{
	fState = kHTTPReadResponse;
}

void
HTTPCommand::IdleWriteCommandBegin()
{
	Trespass();
}

void
HTTPCommand::IdleWriteCommandEnd()
{
	Error error;
	long pending;
	long used;
	
	if ((pending = fCommandStream.GetPending()) != 0) {
		error = GetSocket()->Write(fCommandStream.ReadNext(0), pending, &used);
		fCommandStream.ReadNext(used);
		if (used > 0)
			ResetTimeout();
	} else
		error = GetSocket()->Idle();
		
	switch (error) {
	case kNoError:
		ResetTimeout();
		break;
		
	case kPending:
		return;
		
	case kLostConnection:
	case kNoConnection:
		ImportantMessage(("HTTP: lost connection to %s", fResource.GetURL()));
		fStream->SetStatus(kStreamReset);
		fState = kHTTPError;
		return;
	
	default:
		fStream->SetStatus(error);
		fState = kHTTPError;
		return;
	}
	
	fCommandStream.Reset();
	fState = kHTTPReadResponse;
	IdleReadResponse();
}

Boolean
HTTPCommand::CheckBodyComplete()
{
	long dataExpected;
	Error status = fStream->GetStatus();
	
	if (TrueError(status)) {
		fState = kHTTPError;
		return true;
	}
	
	if (status == kComplete) {
		fState = kHTTPComplete;
		return true;
	}
	
	if ((dataExpected = fStream->GetDataExpected()) != -1)
		if (fStream->GetDataLength() >= dataExpected) {
			fStream->SetStatus(kComplete);
			fState = kHTTPComplete;
			return true;
		}
	
	return false;
}

void
HTTPCommand::Redirect(const char* url)
{
	if (IsError(fStream == nil || url == nil))
		return;

	fStream->Rewind();
	fStream->WriteString(url);
	fStream->SetDataType(kDataTypeURL);
	fStream->SetStatus(kComplete);
}

static const char*
ParseWWWAuthenticate(char* input)
{
	// Input is "Basic Realm="WallyWorld". Returns a pointer to the realm without quotes.
	
	char* quote;
	
	if ((input = SkipString(input, "Basic")) == nil)
		return nil;
	
	input = SkipCharacters(input, " ");
	
	if ((input = SkipString(input, "realm")) == nil)
		return nil;
	
	input = SkipCharacters(input, " =");
	if (*input == 0)
		return nil;
		
	if (*input == '"') {
		if ((quote = strrchr(input, '"')) != nil)
			*quote = 0;
		input++;
	}
	
	return input;
}

Boolean
HTTPCommand::SetAttribute(const char* name, char* value)
{
	const char* realm;
	char* authorization;
	
	if (fStream != nil && fStream->SetAttribute(name, value))
		return true;
	
	if (EqualString(name, "date"))
		return true;
	
	if (EqualString(name, "location") && fState == kHTTPReadHeaderAndRedirect) {
		Redirect(value);
		return true;
	}
	
	if (EqualString(name, "server"))
		return true;
	
	if (EqualString(name, "WWW-Authenticate") && fState == kHTTPReadHeaderAndAuthorize) {
		if ((realm = ParseWWWAuthenticate(value)) == nil) {
			fStream->SetStatus(kResponseError);
			return true;
		}
		
		if ((authorization = gAuthorizationList->NewAuthorization(&fResource, realm, "HTTPCommand::SetAttribute")) != nil) {
			fResource.SetAuthorization(authorization);
			fStream->SetStatus(kStreamReset);
			FreeTaggedMemory(authorization, "HTTPCommand::SetAttribute");
			return true;
		}
		
		if (gLoginPanel->IsOpen()) {
			fStream->SetStatus(kStreamReset);
			return true;
		}

		fStream->SetPriority((Priority)0);
		fStream->SetStatus(kStreamReset);
		gStatusIndicator->Hide();
		gLoginPanel->SetDestination(&fResource);
		gLoginPanel->SetRealm(realm);
		gLoginPanel->Open();
		return true;
	}
	
	if (gCookieList->SetAttribute(name, value, &fResource))
		return true;

	return HasAttributes::SetAttribute(name, value);
}

void 
HTTPCommand::WriteAttributes(Stream* stream)
{
	const char* authorization;
	char* cookie;
	
	if (IsError(stream == nil))
		return;

	stream->WriteAttribute("Accept", "text/html");
	stream->WriteAttribute("Accept", "text/plain");
	stream->WriteAttribute("Accept", "image/gif");
	stream->WriteAttribute("Accept", "image/jpeg");
	stream->WriteAttribute("Accept", "image/xbm");
	stream->WriteAttribute("Accept", "image/x-bitmap");
	stream->WriteAttribute("User-Agent", gSystem->GetUserAgent());

	if (gSystem->GetUseJapanese()) {
		stream->WriteAttribute("Accept", "charset/ISO-2022-JP");
		stream->WriteAttribute("Accept", "charset/ISO-2022-JP-2");
	}
	
	if ((authorization = fResource.GetAuthorization()) != nil) {
		Message(("HTTP: Authorization: %s", authorization));
		stream->WriteString("Authorization: Basic ");
		stream->WriteString(authorization);
		stream->Write("\r\n", 2);
	}
	
	if ((cookie = gCookieList->NewCookie(&fResource, "cookie")) != nil) {
		Message(("HTTP: Cookie: %s", cookie));
		stream->WriteAttribute("Cookie", cookie);
		FreeTaggedMemory(cookie, "cookie");
	}	
}

// =============================================================================
// Class HTTPGetCommand

HTTPGetCommand::HTTPGetCommand()
{
}

HTTPGetCommand::~HTTPGetCommand()
{
}

ClassNumber
HTTPGetCommand::GetClassNumber() const
{
	return kClassNumberHTTPGetCommand;
}

void HTTPGetCommand::IdleWriteCommandBegin()
{
	const char* url = fStream->GetName();
	
	if (!GetService()->IsProxy()) {
		URLParser urlParser;
		urlParser.SetURL(url);
		urlParser.GetPathPlus();
	}
	
	if (IsError(url == nil || *url == 0)) {
		fStream->SetStatus(kGenericError);
		fState = kHTTPError;
		return;
	}
	
	fCommandStream.WriteString("GET ");
	fCommandStream.WriteString(url);
	fCommandStream.WriteString(" HTTP/1.0\r\n");
	WriteAttributes(&fCommandStream);
	fCommandStream.WriteString("\r\n");

	fState = kHTTPWriteCommandEnd;
	IdleWriteCommandEnd();
}

// =============================================================================
// Class HTTPGetCommand

HTTPPostCommand::HTTPPostCommand()
{
}

HTTPPostCommand::~HTTPPostCommand()
{
}

ClassNumber
HTTPPostCommand::GetClassNumber() const
{
	return kClassNumberHTTPPostCommand;
}

void
HTTPPostCommand::IdleWriteCommandBegin()
{
	const char* url = fStream->GetName();
	
	if (!GetService()->IsProxy()) {
		URLParser urlParser;
		urlParser.SetURL(url);
		url = urlParser.GetPathPlus();
	}
	
	if (IsError(url == nil || *url == 0)) {
		fStream->SetStatus(kGenericError);
		fState = kHTTPError;
		return;
	}
	
	fCommandStream.WriteString("POST ");
	fCommandStream.WriteString(url);
	fCommandStream.WriteString(" HTTP/1.0\r\n");
	WriteAttributes(&fCommandStream);
	fCommandStream.WriteAttribute("Content-type", "application/x-www-form-urlencoded");
	fCommandStream.WriteAttribute("Content-length", fResource.GetPostDataLength());
	fCommandStream.WriteString("\r\n");
	fCommandStream.Write(fResource.GetPostData(), fResource.GetPostDataLength());

	fState = kHTTPWriteCommandEnd;
	IdleWriteCommandEnd();
}

// =============================================================================
// Class HTTPProtocol

HTTPProtocol::HTTPProtocol()
{
}

HTTPProtocol::~HTTPProtocol()
{
}

#ifdef INCLUDE_FINALIZE_CODE
void
HTTPProtocol::Finalize()
{
	delete(gMemoryStream);
	gMemoryStream = nil;
}
#endif

ClassNumber
HTTPProtocol::GetClassNumber() const
{
	return kClassNumberHTTPProtocol;
}

void
HTTPProtocol::Initialize()
{
	// Create a shared memory stream that can be used to create data
	// buffers of arbritrary length. Reset must be called after each use.
	gMemoryStream = new(MemoryStream);
	IsError(gMemoryStream == nil);
}

ProtocolCommand*
HTTPProtocol::NewGetCommand(const Resource* resource)
{
	ProtocolCommand* command = new(HTTPGetCommand);
	command->SetResource(resource);
	return command;
}

ProtocolCommand*
HTTPProtocol::NewPostCommand(const Resource* resource)
{
	ProtocolCommand* command = new(HTTPPostCommand);
	command->SetResource(resource);
	return command;
}

// =============================================================================

#ifdef DEBUG_BOXPRINT
// -------------------------------------------------
#include "BoxPrintDebug.h"

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void
HTTPProtocol::StaticBoxPrintDebug(const HTTPProtocol* httpProtocol, long whatToPrint)
{
	if (httpProtocol == nil) {
		BoxPrint("HTTPProtocol: <nil>");
	} else {
		BoxPrint("HTTPProtocol: <%#06x>", httpProtocol);
		AdjustBoxIndent(1);
		httpProtocol->BoxPrintDebug(whatToPrint);
		AdjustBoxIndent(-1);
	}
}
#endif

void
HTTPProtocol::BoxPrintDebug(long whatToPrint) const
{
	if (whatToPrint == 0) {
		whatToPrint = kBoxPrintCommand | kBoxPrintResource | kBoxPrintState;
	}

	Protocol::BoxPrintDebug(whatToPrint);
	if (whatToPrint & kBoxPrintMemoryStream) {
		BoxPrint("MemoryStream: not implemented");
	}
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void
HTTPCommand::StaticBoxPrintDebug(const HTTPCommand* httpCommand, long whatToPrint)
{
	if (httpCommand == nil) {
		BoxPrint("HTTPCommand: <nil>");
	} else {
		BoxPrint("HTTPCommand: (%#06x)", httpCommand);
		AdjustBoxIndent(1);
		httpCommand->BoxPrintDebug(whatToPrint);
		AdjustBoxIndent(-1);
	}
}
#endif

void
HTTPCommand::BoxPrintDebug(long whatToPrint) const
{
	if (whatToPrint == 0) {
		whatToPrint = kBoxPrintState;
	}

	ProtocolCommand::BoxPrintDebug(whatToPrint);
	if (whatToPrint & kBoxPrintState) {
#ifdef DEBUG_NAMES
		BoxPrint("HTTPState: %s (%d)", GetStateAsString(), GetState());
#else /* of #ifdef DEBUG_NAMES */
		BoxPrint("HTTPState: %d", GetState());
#endif /* of #else of #ifdef DEBUG_NAMES */
	}
	if (whatToPrint & kBoxPrintCommandStream) {
		BoxPrint("CommandStream: not implemented");
	}
}

// -------------------------------------------------
#endif /* #ifdef DEBUG_BOXPRINT */
