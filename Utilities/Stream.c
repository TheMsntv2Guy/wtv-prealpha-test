// ===========================================================================
//	Stream.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __DATETIME_H__
#include "DateTime.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __STREAM_H__
#include "Stream.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif

#include "rc4.h"

// =============================================================================
// Class DataStream

DataStream::DataStream()
{
}

DataStream::~DataStream()
{
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
char*
DataStream::CopyName(const char* USED_FOR_MEMORY_TRACKING(tagName)) const
{
	return CopyString(GetName(), tagName);
}
#endif

void
DataStream::FastForward()
{
	fPosition = GetDataLength();
}

char*
DataStream::GetData() const
{
	//Trespass(); // EMAC: removing all Trespass() to fix an issue
	return nil;
}

const char*
DataStream::GetDataAsString()
{
	//Trespass(); // EMAC: removing all Trespass() to fix an issue
	return nil;
}

long
DataStream::GetDataLength() const
{
	//Trespass(); // EMAC: removing all Trespass() to fix an issue
	return 0;
}

DataType
DataStream::GetDataType() const
{
	//Trespass(); // EMAC: removing all Trespass() to fix an issue
	return (DataType)0;
}

ulong
DataStream::GetExpires() const
{
	//Trespass(); // EMAC: removing all Trespass() to fix an issue
	return 0;
}

ulong
DataStream::GetLastModified() const
{
	//Trespass(); // EMAC: removing all Trespass() to fix an issue
	return 0;
}

const char*
DataStream::GetName() const
{
	//Trespass(); // EMAC: removing all Trespass() to fix an issue
	return nil;
}

uchar
DataStream::GetPercentComplete(ulong expected) const
{
	Error status = GetStatus();
	if (status == kComplete || TrueError(kComplete))
		return 100;
	
	long dataExpected = GetDataExpected();
	if (dataExpected == 0)
		dataExpected = expected;
	
	return (uchar)(100 * GetDataLength() / dataExpected);
}

Priority
DataStream::GetPriority() const
{
	//Trespass(); // EMAC: removing all Trespass() to fix an issue
	return (Priority)0;
}

Error
DataStream::GetStatus() const
{
	//Trespass(); // EMAC: removing all Trespass() to fix an issue
	return kNoError;
}

Boolean
DataStream::IsDataTrusted() const
{
	return false;
}

DataType
DataStream::ParseDataType(char* value)
{
	// Note:  remember to change GetDataTypeString when changing this
	
	typedef struct {
		const char* text;
		DataType type;
	} DataTypeDescriptor;
	
	static const DataTypeDescriptor kStandardType[] = {
		{"audio/midi",				kDataTypeMIDI},
		{"audio/x-midi",			kDataTypeMIDI},
		{"audio/x-mpeg",			kDataTypeMPEGAudio},
		{"audio/x-pn-realaudio",	kDataTypeRealAudioMetafile},
		{"image/fido",				kDataTypeFidoImage},
		{"image/gif",				kDataTypeGIF},
		{"image/jpeg",				kDataTypeJPEG},
		{"image/wtv-bitmap",		kDataTypeBitmap},
		{"image/x-bitmap",			kDataTypeXBitMap},
		{"image/x-xbitmap",			kDataTypeXBitMap},
		{"image/xbm",				kDataTypeXBitMap},
		{"text",					kDataTypeTEXT},
		{"text/html",				kDataTypeHTML},
		{"text/plain",				kDataTypeTEXT},
		{"text/tellyscript",		kDataTypeTellyScript},
		{"x-wtv-animation",			kDataTypeAnimation},
		{0}
	};
	
	static const DataTypeDescriptor kRestrictedType[] = {
		{"code/x-wtv-code-mips",	kDataTypeMIPSCode},
		{"code/x-wtv-code-ppc",		kDataTypePPCCode},
		{0}
	};
	
	char* p;
	long i;
	
	if ((p = FindCharacter(value, ";")) != nil)
		*p = 0;
		
	PostulateFinal(false); // hack until proxy returns correct type
	if (EqualString(value, "text/html") || EqualString(value, "text/plain")) {
		if (FindStringLast(GetName(), ".mid"))
			return kDataTypeMIDI;
		if (FindStringLast(GetName(), ".mpa"))
			return kDataTypeMPEGAudio;
		if (FindStringLast(GetName(), ".mp2"))
			return kDataTypeMPEGAudio;
		if (FindStringLast(GetName(), ".m1a"))
			return kDataTypeMPEGAudio;
		if (FindStringLast(GetName(), ".ram"))
			return kDataTypeRealAudioMetafile;
		if (FindStringLast(GetName(), ".native-code"))
			goto Fail;	// this should never come back
		
		if (IsDataTrusted()) {
			if (FindStringLast(GetName(), ".mips-code"))
		return kDataTypeMIPSCode;
			if (FindStringLast(GetName(), ".ppc-code"))
		return kDataTypePPCCode;
		}
	}
	
	for (i = 0; kStandardType[i].text != nil; i++)
		if (EqualString(kStandardType[i].text, value))
			return kStandardType[i].type;

	if (IsDataTrusted())
		for (i = 0; kRestrictedType[i].text != nil; i++)
			if (EqualString(kRestrictedType[i].text, value))
				return kRestrictedType[i].type;

Fail:	
	ImportantMessage(("DataStream: cannot parse %s", value));
	IsWarning("DataStream: unknown data type");
	return (DataType)0;
}

long
DataStream::Read(void* data, long length)
{
	long pending = GetPending();

	if (length > pending)
		length = pending;

	CopyMemory(ReadNext(length), data, length);
	return length;
}

const char*
DataStream::ReadNext(long length)
{
	long pending = GetPending();
	char* data;
	
	if (IsError(length < 0))
		return nil;
	
	if (IsError(length > pending))
		length = pending;

	data = GetData() + fPosition;
	fPosition += length;
	return data;
}

void
DataStream::Rewind()
{
	fPosition = 0;
}

Boolean
DataStream::SetAttribute(const char* name, char* value)
{
	DataType dataType;
	
	if (EqualString(name, "content-encoding")) {
		SetStatus(kUnknownDataType);
		return true;
	}
	
	if (EqualString(name, "content-length")) {
		SetDataExpected(atoi(value));
		return true;
	}
	
	if (EqualString(name, "content-type")) {
		if ((dataType = ParseDataType(value)) == (DataType)0)
			SetStatus(kUnknownDataType);
		SetDataType(dataType);
		return true;
	}
	
	if (EqualString(name, "expires")) {
		SetExpires(DateTimeParser::Parse(value));
		
		// Assume bad values are already expired.
		if (GetExpires() == 0)
			SetExpires(1);
		return true;
	}
	
	if (EqualString(name, "last-modified")) {
		SetLastModified(DateTimeParser::Parse(value));
		return true;
	}
	
	if (EqualString(name, "pragma") && EqualString(value, "no-cache")) {
		SetExpires(1);
		return true;
	}
	
	return HasAttributes::SetAttribute(name, value);
}

void
DataStream::SetDataType(DataType)
{
	//Trespass(); // EMAC: removing all Trespass() to fix an issue
}

void
DataStream::SetExpires(ulong UNUSED(value))
{
	//Trespass(); // EMAC: removing all Trespass() to fix an issue
}

void
DataStream::SetIsDataTrusted(Boolean)
{
	//Trespass(); // EMAC: removing all Trespass() to fix an issue
}

void
DataStream::SetLastModified(ulong UNUSED(value))
{
	//Trespass(); // EMAC: removing all Trespass() to fix an issue
}

void
DataStream::SetName(const char* UNUSED(value))
{
	//Trespass(); // EMAC: removing all Trespass() to fix an issue
}

void
DataStream::SetPriority(Priority UNUSED(value))
{
	//Trespass(); // EMAC: removing all Trespass() to fix an issue
}

void
DataStream::SetStatus(Error)
{
	//Trespass(); // EMAC: removing all Trespass() to fix an issue
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void
DataStream::Unread(long length)
{
	if (IsError(length > fPosition))
		length = fPosition;
		
	fPosition -= length;
}
#endif

void
DataStream::WriteQuery(const char* name, long value)
{
	char s[12]; /* 11 for %ld, 1 for NULL */
	snprintf(s, sizeof(s), "%ld", value);
	WriteQuery(name, s);
}

void
DataStream::WriteQuery(const char* name, const char* value)
{
	char ch;
	
	// Check whether this is the first query that has been written to the stream.
	if (GetDataLength() != 0) {
		char last = *(GetData() + GetDataLength() - 1);
		if (last != '?')
			Write(&(ch = '&'), 1);
	}
	
	// Write name and value.
	if (name != nil && *name != '\0') {
		WriteQueryString(name);
		Write(&(ch = '='), 1);
	}
	if (value != nil && *value != '\0')
		WriteQueryString(value);
}

void
DataStream::WriteQueryString(const char* input)
{
	long count;
	char* output;
	long out;
	long in;
	
	if (input == nil || *input == 0)
		return;
	
	count = 0;
	for (in = 0; input[in] != '\0'; in++)
		switch (input[in]) {
		case '\n':
		case '\r':
		case '%':
		case '&':
		case '+':
		case '=':
		case '?':
			count += 3;
			break;
		default:
			count++;
			break;
		}

	output = (char*)AllocateBuffer(count + 1);

	for (in = 0, out = 0; input[in] != 0; in++, out++)
		switch (input[in]) {
		case ' ':	output[out] = '+'; break;
		case '\n':	output[out++] = '%'; output[out++] = '0'; output[out] = 'A'; break;
		case '\r':	output[out++] = '%'; output[out++] = '0'; output[out] = 'D'; break;
		case '%':	output[out++] = '%'; output[out++] = '2'; output[out] = '5'; break;
		case '&':	output[out++] = '%'; output[out++] = '2'; output[out] = '6'; break;
		case '+':	output[out++] = '%'; output[out++] = '2'; output[out] = 'B'; break;
		case '=':	output[out++] = '%'; output[out++] = '3'; output[out] = 'D'; break;
		case '?':	output[out++] = '%'; output[out++] = '3'; output[out] = 'F'; break;
		default:	output[out] = input[in]; break;
		}
	
	Write(output, count);
	FreeBuffer(output, count+1);
}

// =============================================================================

MemoryStream::~MemoryStream()
{
	if (fData != nil)
		FreeTaggedMemory(fData, "MemoryStream::fData");

	if (fName != nil)
		FreeTaggedMemory(fName, "MemoryStream::fName");
}

char*
MemoryStream::GetData() const
{
	return fData;
}

const char*
MemoryStream::GetDataAsString()
{
	if (fData == nil)
		return nil;
	
	if (fData[fDataLength] != 0) {
		MakeAvailable(fDataLength + 1);
		fData[fDataLength++] = 0;
	}
	
	return fData;
}

long
MemoryStream::GetDataExpected() const
{
	return fDataExpected;
}

long
MemoryStream::GetDataLength() const
{
	return fDataLength;
}

DataType
MemoryStream::GetDataType() const
{
	return fDataType;
}

ulong
MemoryStream::GetExpires() const
{
	return fExpires;
}

ulong
MemoryStream::GetLastModified() const
{
	return fLastModified;
}

long
MemoryStream::GetLengthIncrement() const
{
	return 512;
}

long
MemoryStream::GetLengthInitial() const
{
	return 1024;
}

const char*
MemoryStream::GetName() const
{
	return fName;
}

Priority
MemoryStream::GetPriority() const
{
	return fPriority;
}

Error
MemoryStream::GetStatus() const
{
	return fStatus;
}

void
MemoryStream::MakeAvailable(long length)
{
	if (length <= fDataExpected)
		return;
	
	if (fDataExpected == 0) {
		fDataExpected = MAX(GetLengthIncrement(), length);
		fData = (char*)AllocateTaggedMemory(fDataExpected, "MemoryStream::fData");
		return;
	}
	
	fDataExpected += MAX(GetLengthIncrement(), length - fDataExpected);
	fData = (char*)ReallocateTaggedMemory(fData, fDataExpected, "MemoryStream::fData");
}

#ifdef DEBUG_CACHE_VIEWASHTML
char*
MemoryStream::RemoveData()
{
	char* data = fData;
	fData = nil;
	Reset();
	return data;
}
#endif

void
MemoryStream::Reset()
{
	if (fData)
		FreeTaggedMemory(fData, "MemoryStream::fData");
	
	if (fName)
		FreeTaggedMemory(fName, "MemoryStream::fName");

	fData = nil;
	fDataLength = 0;
	fDataExpected = 0;
	fName = nil;
	Rewind();
}

void
MemoryStream::SetDataType(DataType type)
{
	fDataType = type;
}

void
MemoryStream::SetExpires(ulong value)
{
	fExpires = value;
}

void
MemoryStream::SetLastModified(ulong value)
{
	fLastModified = value;
}

void
MemoryStream::SetName(const char* value)
{
	fName = CopyStringTo(fName, value, "MemoryStream::fName");
}

void
MemoryStream::SetPriority(Priority value)
{
	fPriority = value;
}

void
MemoryStream::SetStatus(Error value)
{
	fStatus = value;
	
	if (fStatus == kComplete) {
		if (fDataLength == 0 && fDataExpected != 0) {
			FreeTaggedMemory(fData, "MemoryStream::fData");
			fData = nil;
			fDataExpected = 0;
		} else if (fDataLength < fDataExpected) {
			fDataExpected = fDataLength;
			fData = (char*)ReallocateTaggedMemory(fData, fDataExpected, "MemoryStream::fData");
			return;
		}
		return;
	}
	
	if (TrueError(fStatus) && fDataExpected != 0) {
		FreeTaggedMemory(fData, "MemoryStream::fData");
		fData = nil;
		fDataLength = 0;
		fDataExpected = 0;
		return;
	}	
}		

void
MemoryStream::Write(const void* data, long length)
{
	MakeAvailable(fDataLength + length);
	CopyMemory(data, &fData[fDataLength], length);
	fDataLength += length;
}

// =============================================================================
// Class RC4Stream

RC4Stream::RC4Stream()
{
}

RC4Stream::~RC4Stream()
{
}

void
RC4Stream::SetKey(void* key)
{
	fKey = key;
}

long
RC4Stream::Read(void* data, long count)
{
	long data_len;
	 	
	data_len = DataStream::Read(data, count);
/*	RC4((RC4_KEY*)fKey,data_len,(uchar*)data,(uchar*)data); */
	return data_len;
}

void
RC4Stream::Write(const void* data, long count)
{
/*	RC4((RC4_KEY*)fKey,count,(uchar*)data,(uchar*)data); */
	DataStream::Write(data, count);
}

// =============================================================================
// Class Stream

Stream::Stream()
{
}

Stream::~Stream()
{
	// Delete the next in the chain, if any.
	delete(fNext);
}

long
Stream::GetDataExpected() const
{
	if (fNext == nil)
		return 0;
	
	return ((Stream*)fNext)->GetDataExpected();
}

Error
Stream::GetStatus() const
{
	if (fNext == nil)
		return kNoError;
	
	return ((Stream*)fNext)->GetStatus();
}

uchar
Stream::GetPercentComplete(ulong expectedSize) const
{
	if (fNext == nil)
		return 100;
	
	return ((Stream*)fNext)->GetPercentComplete(expectedSize);
}	

#ifdef TEST_CLIENTTEST
int
Stream::Printf(const char* format, ...)
{
	int result;
	va_list list;
	
	// just pretend to print it...see how long it would be
	va_start(list, format);
	result = VPrintf(format, list);
	va_end(list);

	return result;
}
#endif

#ifdef TEST_CLIENTTEST
int
Stream::VPrintf(const char* format, va_list list)
{
	// just pretend to print it...see how long it would be
	int result1 = ::vnullprintf(format, list);

	// now allocate a buffer and really write it to the stream
	int result2 = 0;
	char* buffer = (char*)AllocateBuffer(result1+1);
	if (buffer != nil) {
		result2 = ::vsnprintf(buffer, result1+1, format, list);
		IsError(result1 != result2);
		Write(buffer, result2);
		FreeBuffer(buffer, result1+1);
	}	
	return result2;
}
#endif

long
Stream::Read(void* data, long count)
{
	if (fNext == nil)
		return 0;
	
	return ((Stream*)fNext)->Read(data, count);
}

void
Stream::SetDataExpected(long value)
{
	if (fNext == nil)
		return;
	
	((Stream*)fNext)->SetDataExpected(value);
}

void
Stream::SetStatus(Error value)
{
	if (fNext == nil)
		return;
	
	((Stream*)fNext)->SetStatus(value);
}

void
Stream::Write(const void* data, long count)
{
	if (fNext == nil)
		return;
	
	((Stream*)fNext)->Write(data, count);
}

void
Stream::WriteAttribute(const char* name, long value)
{
	char s[12]; /* 11 for %ld, 1 for NULL */
	snprintf(s, sizeof(s), "%ld", value);
	WriteAttribute(name, s);
}

void
Stream::WriteAttribute(const char* name, const char* value)
{
	if (name == nil || *name == 0 || value == nil || *value == 0)
		return;
	
	Write(name, strlen(name));
	Write(": ", 2);
	Write(value, strlen(value));
	Write("\r\n", 2);
}

void
Stream::WriteNumeric(long value)
{
	char s[12]; /* 11 for %ld, 1 for NULL */
	snprintf(s, sizeof(s), "%ld", value);
	WriteString(s);
}

void
Stream::WriteString(const char* string)
{
	if (string == nil || *string == 0)
		return;

	Write(string, strlen(string));
}

void
Stream::WriteHTMLEscapedString(const char* string)
{
	if (string == nil || *string == '\0')
		return;

	// Write the string, escaping HTML special characters
	for ( ; *string != '\0'; string++) {
		switch(*string) {
			case '&':	WriteString("&amp;");		break;
			case '"':	WriteString("&quot;");		break;
			case '<':	WriteString("&lt;");		break;
			case '>':	WriteString("&gt;");		break;
			default:	Write(string, 1);			break;
		}
	}
}

// =============================================================================

