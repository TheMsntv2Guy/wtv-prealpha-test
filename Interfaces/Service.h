// ===========================================================================
//	Service.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __SERVICE_H__
#define __SERVICE_H__

#ifndef __CLASSES_H__
#include "Classes.h"
#endif
#ifndef __ERRORNUMBERS_H__
#include "ErrorNumbers.h"
#endif
#ifndef __LIST_H__
#include "List.h"
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif

class Protocol;
class Service;
class ServiceList;
class ServicePointer;
class Socket;

// =============================================================================
// Class ServicePointer

class ServicePointer : public Listable {
public:
							ServicePointer();
							ServicePointer(const ServicePointer&);
							ServicePointer(Service*);
							~ServicePointer();
	
							operator Service*() const;
	Service&				operator*() const;
	Service*				operator->() const;
	ServicePointer&			operator=(const ServicePointer&);
	Boolean					operator==(const ServicePointer&) const;
	
	Boolean					HasService() const;
	Boolean					IsEqual(const Service*) const;
	void					Reset();
	void					SetService(Service*);

protected:
	Service*				fService;
};

inline
ServicePointer::operator Service*() const
{
	return fService;
}

inline Service*
ServicePointer::operator->() const
{
	return fService;
}

inline Boolean 
ServicePointer::operator==(const ServicePointer& other) const
{
	return fService == other.fService;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
inline Boolean
ServicePointer::HasService() const
{
	return fService != nil;
}
#endif

// =============================================================================
// Class Service

class Service : public HasDebugModifiedTime {
public:
							Service();
	virtual					~Service();

	ulong					GetHostAddress() const;
	const char*				GetHostName() const;
	ushort					GetHostPort() const;
	const char*				GetName() const;
	Service*				GetNext() const;
	ClassNumber				GetProtocolClassNumber() const;
	Boolean					IsActive() const;
	Boolean					IsEqual(const Service*) const;
	Boolean					IsNamed(const char*) const;
	Boolean					IsProxy() const;
	Boolean					IsEncrypted() const;
	
	void					SetHostAddress(ulong);
	void					SetHostName(const char* nameOrAddress);
	void					SetHostPort(ushort);
	void					SetIsProxy(Boolean);
	void					SetIsEncrypted(Boolean);
	void					SetName(const char*);
	void					SetProtocolClassNumber(ClassNumber);

	void					Append(Service*);
	void					BeginUse();
	void					EndUse();
	Protocol*				NewProtocol() const;
	Socket*					NewSocket(Error*);
	void					Suspend();
	
	static Service*			NewService(const Resource*);

#ifdef DEBUG_BOXPRINT
public:
	enum {
		kFirstBoxPrint = 1,
		kBoxPrintHostInfo		= kFirstBoxPrint << 0,
		kBoxPrintName			= kFirstBoxPrint << 1,
		kBoxPrintUserCount		= kFirstBoxPrint << 2,
		kLastBoxPrint = kBoxPrintUserCount
	};
	virtual void			BoxPrintDebug(long whatToPrint) const;
	static void				StaticBoxPrintDebug(const Service* service, long whatToPrint);
#endif

protected:
	void					ComputeProtocolClassNumber();

protected:
	ulong					fHostAddress;
	char*					fHostName;
	char*					fName;
	ServicePointer			fNext;
	long					fReactivateTime;
	
	ushort					fHostPort;
	short					fUserCount;
	
	ClassNumber				fProtocolClassNumber;

	Boolean					fIsProxy;
	Boolean					fIsEncrypted;
};

inline void
Service::BeginUse()
{
	fUserCount++;
}

#ifdef DEBUG_SERVICEWINDOW
inline ulong 
Service::GetHostAddress() const
{
	return fHostAddress;
}
#endif

inline const char*
Service::GetHostName() const
{
	return fHostName;
}

inline ushort 
Service::GetHostPort() const
{
	return fHostPort;
}

inline const char* 
Service::GetName() const
{
	return fName;
}

inline Boolean 
Service::IsNamed(const char* name) const
{
	return EqualString(fName, name);
}

inline Boolean 
Service::IsProxy() const
{
	return fIsProxy;
}

inline Boolean
Service::IsEncrypted() const
{
	return fIsEncrypted;
}

// =============================================================================
// Class ServiceList

class ServiceList : public HasAttributes, public HasDebugModifiedTime {
public:
							ServiceList();
	virtual					~ServiceList();
	
	long					GetCount() const;
	Boolean					HasService(const Resource* base, const char* partial) const;
	
	virtual Boolean			SetAttribute(const char* name, char* value);
	
	void					Add(Service*);
	void					Add(const char* name, const char* nameOrAddress, ushort port);
	Service*				At(long index) const;
	Service*				FindByName(const char* name) const;
	ServicePointer			FindByResource(const Resource*) const;
	ServicePointer			FindByURL(const char*) const;
	char*					NewSendURL(const Resource* base, const char* partial, const char* tag) const;
	char*					NewSendURL(const char* url, const char* tag) const;

#if defined(TEST_SIMULATORONLY) && defined(DEBUG)
	void					Test();
#endif
	
#ifdef DEBUG_BOXPRINT
	virtual void			BoxPrintDebug(long whatToPrint) const;
	static void				StaticBoxPrintDebug(const ServiceList* serviceList, long whatToPrint);
#endif

protected:
	Boolean					HasService(const char* url, Boolean isDataTrusted) const;

protected:
	ObjectList				fServiceList;
};

extern ServiceList* gServiceList;

inline Service* ServiceList::At(long index) const
{
	return (Service*)*(ServicePointer*)fServiceList.At(index);
}

inline long ServiceList::GetCount() const
{
	return fServiceList.GetCount();
}

// =============================================================================

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Service.h multiple times"
	#endif
#endif /* __SERVICE_H__ */
