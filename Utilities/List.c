// ===========================================================================
//	List.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __LIST_H__
#include "List.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif

//===========================================================================
// Class DataIterator.

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void* DataIterator::GetFirst()
{
	fNext = 0;
	return GetNext();
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void* DataIterator::GetNext()
{
	if (IsError(fTarget == nil))
		return nil;

	if (fNext >= fTarget->GetCount())
		return nil;
	
	return fTarget->At(fNext++);
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void DataIterator::SetTarget(const DataList* target)
{
	fTarget = target;
	fNext = 0;
}
#endif

//===========================================================================

DataList::DataList()
{
	fDataSize = 1;
	fListIncrement = 8;
}

DataList::~DataList()
{
	if (fList != nil)
		FreeTaggedMemory(fList, "DataList::fList");
}

void DataList::Add(const void* data)
{
	AddAt(data, fCount);
}

#ifdef TEST_SIMULATORONLY
void DataList::AddAll(const DataList* list)
{
	long i;
	
	if (IsWarning(list == nil))
		return;
	
	if (IsError(list->fDataSize != fDataSize))
		return;
	
	for (i = 0; i < list->fCount; i++)
		Add(list->At(i));
}
#endif

void DataList::AddAt(const void* data, long index)
{
	if (IsError(index < 0 || index > fCount))
		return;
	
	if (fList == nil) {
		fLength = fListIncrement;
		fList = (uchar*)AllocateTaggedMemory(fLength * fDataSize, "DataList::fList");
	} else if (fCount == fLength) {
		uchar* list = (uchar*)AllocateTaggedMemory((fLength + fListIncrement) * fDataSize, "DataList::fList");
		CopyMemory(fList, list, fLength * fDataSize);
		FreeTaggedMemory(fList, "DataList::fList");
		fList = list;
		fLength += fListIncrement;
	}
	
	if (index != fCount)
		CopyMemory(&fList[index*fDataSize], &fList[(index+1)*fDataSize], (fCount-index)*fDataSize);
	CopyMemory(data, &fList[index*fDataSize], fDataSize);
	fCount++;
}

void* DataList::At(long index) const
{
	if (IsError(index < 0 || index >= fCount))
		return nil;

	return &fList[index*fDataSize];
}

void DataList::DeleteAll()
{
	fCount = 0;
	fLength = 0;
	if (fList != nil) {
		FreeTaggedMemory(fList, "DataList::fList");
		fList = nil;
	}
}

#ifdef TEST_SIMULATORONLY
short DataList::GetDataSize() const
{
	return fDataSize;
}
#endif

Boolean DataList::IsEmpty() const
{
	return fCount == 0;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
DataIterator* DataList::NewIterator() const
{
	DataIterator* iterator = new(DataIterator);
	iterator->SetTarget(this);
	return iterator;
}
#endif

void DataList::RemoveAll()
{
	fCount = 0;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void DataList::RemoveAllAfter(long index)
{
	if (IsError(index < 0 || index >= fCount))
		return;
		
	fCount = index + 1;
}
#endif

void DataList::RemoveAt(long index)
{
	if (IsError(index < 0 || index >= fCount))
		return;
	
	if (fCount-index-1 != 0)
		CopyMemory(&fList[(index+1)*fDataSize], &fList[index*fDataSize], (fCount-index-1)*fDataSize);
	fCount--;
}

void DataList::SetDataSize(short value)
{
	if (IsError(value <= 0 || fCount != 0))
		return;
		
	fDataSize = value;
}

void DataList::SetListIncrement(short value)
{
	if (IsError(value <= 0 || fCount != 0))
		return;

	fListIncrement = value;
}

#ifdef TEST_SIMULATORONLY
void DataList::Test()
{
	{	// Test 1-byte list.
		DataList*	data = new(DataList);
		char value1 = 0x11;
		char value2 = 0x22;
	
		Assert(data->fCount == 0 && data->fList == 0 && data->fLength == 0 && data->fDataSize == 1);
	
		data->Add(&value1);
		Assert(data->fCount == 1);
		Assert(*(char*)data->At(0) == value1);
		
		data->Add(&value2);
		Assert(data->fCount == 2);
		Assert(*(char*)data->At(0) == value1);
		Assert(*(char*)data->At(1) == value2);
		
		data->RemoveAt(0);
		Assert(data->fCount == 1);
		Assert(*(char*)data->At(0) == value2);
	
		data->RemoveAt(0);
		Assert(data->fCount == 0);
		delete(data);
	}

	{	// Test 4-byte list.
		DataList*	data = new(DataList);
		long		value1 = 0x11111111;
		long		value2 = 0x22222222;
	
		data->SetDataSize(4);
		Assert(data->fCount == 0 && data->fList == 0 && data->fLength == 0 && data->fDataSize == 4);
	
		data->Add(&value1);
		Assert(data->fCount == 1);
		Assert(*(long*)data->At(0) == value1);
		
		data->Add(&value2);
		Assert(data->fCount == 2);
		Assert(*(long*)data->At(0) == value1);
		Assert(*(long*)data->At(1) == value2);
		
		data->RemoveAt(0);
		Assert(data->fCount == 1);
		Assert(*(long*)data->At(0) == value2);
	
		data->RemoveAt(0);
		Assert(data->fCount == 0);

		delete(data);
	}

	{	// Test list extending.
		DataList*	data = new(DataList);
		short		value1 = 0x1111;
		short		value2 = 0x2222;
		int			i;
		
		data->SetDataSize(2);
		Assert(data->fCount == 0 && data->fLength == 0 && data->fDataSize == 2);
		
		for (i = 0; i < data->fListIncrement; i++)
			data->Add(&value1);
		Assert(data->fCount == data->fListIncrement && data->fLength == data->fListIncrement && data->fDataSize == 2);
		
		data->AddAt(&value2, 0);
		Assert(data->fCount == (data->fListIncrement+1) && data->fLength == (2 * data->fListIncrement) && data->fDataSize == 2);
		Assert(*(short*)data->At(0) == value2);
		for (i = 1; i < data->fListIncrement+1; i++)
			Assert(*(short*)data->At(i) == value1);

		delete(data);
	}
	
	{	// Test AddAll.
		DataList*	data1 = new(DataList);
		DataList*	data2 = new(DataList);
		char		value1 = 0x11;
		char		value2 = 0x22;
		
		data1->Add(&value1);
		data1->Add(&value1);
		data2->Add(&value2);
		data2->Add(&value2);
		data1->AddAll(data2);
		
		Assert(data1->fCount == 4);
		Assert(*(char*)data1->At(0) == value1);
		Assert(*(char*)data1->At(1) == value1);
		Assert(*(char*)data1->At(2) == value2);
		Assert(*(char*)data1->At(3) == value2);
		
		delete(data1);
		delete(data2);
	}
}
#endif /* TEST_SIMULATORONLY */
	
//===========================================================================

Listable::~Listable()
{
}

//===========================================================================

Listable* ObjectIterator::GetFirst()
{
	fNext = 0;
	return GetNext();
}

Listable* ObjectIterator::GetNext()
{
	if (IsError(fTarget == nil))
		return nil;

	if (fNext >= fTarget->GetCount())
		return nil;
	
	return fTarget->At(fNext++);
}

void ObjectIterator::SetTarget(const ObjectList* target)
{
	fTarget = target;
	fNext = 0;
}

//===========================================================================

ObjectList::ObjectList() 
{
	fObjects.SetDataSize(sizeof (Listable*));
}

ObjectList::~ObjectList()
{
}

#ifdef TEST_SIMULATORONLY
void ObjectList::Delete(Listable* object)
{
	long where = Find(object);
	
	if (where != -1)
		DeleteAt(where);
}
#endif

void ObjectList::DeleteAll()
{
	long count;
	
	for (count = GetCount(); count != 0; count--)
		DeleteAt(count-1);
}

void ObjectList::DeleteAt(long index)
{
	Listable* object = At(index);
	RemoveAt(index);
	delete(object);
}

long ObjectList::Find(Listable* object) const
{
	long count = GetCount();
	long i;
	
	for (i = 0; i < count; i++)
		if (At(i) == object)
			return i;
	
	return -1;
}

ObjectIterator* ObjectList::NewIterator() const
{
	ObjectIterator* iterator = new(ObjectIterator);
	iterator->SetTarget(this);
	return iterator;
}

Listable* ObjectList::Remove(Listable* object)
{
	long where = Find(object);
	
	if (where == -1)
		return nil;
	
	return RemoveAt(where);
}

#ifdef TEST_SIMULATORONLY
void ObjectList::Test()
{
	ObjectList* list1 = new(ObjectList);
	ObjectList* list2 = new(ObjectList);
	ObjectList* list3 = new(ObjectList);
	
	Assert(list1->GetCount() == 0 && list1->fObjects.GetDataSize() == sizeof (list1));
	
	list1->Add(list2);
	Assert(list1->GetCount() == 1);
	Assert(list1->At(0) == list2);
	Assert(list1->Find(list2) == 0);
	list1->Remove(list2);
	Assert(list1->GetCount() == 0);
	
	list1->Add(list2);
	Assert(list1->GetCount() == 1);
	Assert(list1->At(0) == list2);
	Assert(list1->Find(list2) == 0);
	list1->Delete(list2);
	Assert(list1->GetCount() == 0);
	Assert(list1->Find(list2) == -1);
	
	list1->Add(list3);
	list1->DeleteAt(0);
	Assert(list1->GetCount() == 0);
	
	delete(list1);
}
#endif /* TEST_SIMULATORONLY */

//===========================================================================
// Dictionary of key/value strings. The key and value are combined into a
// single string and added to a DataList.

StringDictionary::StringDictionary()
{
	fDataList.SetDataSize(sizeof (char*));
}

StringDictionary::~StringDictionary()
{
}

void
StringDictionary::Add(const char* key, const char* value)
{
	if (IsError(key == nil || *key == 0 || value == nil))
		return;
	
	long keyLength = strlen(key);
	long valueLength = strlen(value);
	char* data = (char*)AllocateTaggedMemory(keyLength + 1 + valueLength + 1, "StringDictionary");
	
	CopyMemory(key, &data[0], keyLength);
	data[keyLength] = 0;
	CopyMemory(value, &data[keyLength+1], valueLength);
	data[keyLength+1+valueLength] = 0;
	fDataList.Add(&data);
}

#ifdef DEBUG
Boolean		gDebugDictionary = false;
#endif

void
StringDictionary::Add(const char* input)
{
	// Add a set of key/value pairs of the form "name=John+Matheny&eyes=blue".
	
	long inputCharacters;
	char* in;
	const char* key;
	const char* value;
	char* p;
	long hex;

	if (input == nil || *input == 0)
		return;
	
	// Create modifiable input string.
	inputCharacters = strlen(input);
	in = (char*)AllocateBuffer(inputCharacters + 1);
	strcpy(in, input);
	
	// Add each key/value pair found in the input string. 
	for (p = in, key = in, value = ""; *p; p++)
		switch (*p) {
			case '+':	*p = ' '; break;
			case '=':	*p = 0; value = p + 1; break;
			
			case '%':
				if (sscanf(p+1, "%2x", &hex) == 1) {
					*p = hex;
					strcpy(p+1, p+3);
				}
				break;
			
			case '&':
				*p = 0;
				if (*key) {
#ifdef DEBUG
					if (gDebugDictionary)
						TrivialMessage(("Adding (\"%s\",\"%s\") to dictionary %x", key, value));
#endif
					Add(key, value);
				}
				key = p + 1;
				value = "";
				break;
		}
	
	if (*key) {
#ifdef DEBUG
		if (gDebugDictionary)
			Message(("Adding (\"%s\",\"%s\") to dictionary %x", key, value));
#endif
		Add(key, value);
	}
	
	FreeBuffer(in, inputCharacters + 1);
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void
StringDictionary::Delete(const char* key)
{
	long count = fDataList.GetCount();
	long i;
	
	for (i = 0; i < count; i++)
		if (EqualString(key, *(char**)fDataList.At(i))) {
			DeleteAt(i);
			break;
		}
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void
StringDictionary::DeleteAll()
{
	long count = fDataList.GetCount();
	
	for (; count != 0; count--)
		DeleteAt(count-1);
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void
StringDictionary::DeleteAt(long index)
{
	char* data = *(char**)fDataList.At(index);
	fDataList.RemoveAt(index);
	FreeTaggedMemory(data, "StringDictionary");
}
#endif

const char*
StringDictionary::GetKeyAt(long index) const
{
	if (index >= fDataList.GetCount())
		return nil;
	
	 const char* data = *(const char**)fDataList.At(index);
	 return data;
}

const char*
StringDictionary::GetValue(const char* key) const
{
	long count = fDataList.GetCount();
	long i;
	
	for (i = 0; i < count; i++) {
		const char* data = *(const char**)fDataList.At(i);
		if (EqualString(data, key))
			return data + strlen(data) + 1;
	}

	return nil;
}

const char*
StringDictionary::GetValueAt(long index) const
{
	if (index >= fDataList.GetCount())
		return nil;
	
	const char* data = *(const char**)fDataList.At(index);
	data += strlen(data) + 1;
	return data;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
long
StringDictionary::GetValueLong(const char* key) const
{
	const char* value;
	
	if ((value = GetValue(key)) == nil)
		return 0;
	
	if (!isdigit(*value))
		return 0;
	
	return atol(value);
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
long
StringDictionary::GetValueLongAt(long index) const
{
	const char* value = GetValueAt(index);
	
	if (value == nil)
		return 0;
	
	return atol(value);
}
#endif

//===========================================================================

StringList::StringList()
{
	fDataList.SetDataSize(sizeof (char*));
}

StringList::~StringList()
{
	DeleteAll();
}

void StringList::Add(const char* string)
{
	char* s = CopyString(string, "StringList");
	fDataList.Add(&s);
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void StringList::AddAt(const char* string, long index)
{
	char* s = CopyString(string, "StringList");
	fDataList.AddAt(&s, index);
}
#endif

char* StringList::At(long index) const
{
	return *(char**)fDataList.At(index);
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void StringList::Delete(char* string)
{
	long where = Find(string);
	
	if (where != -1)
		DeleteAt(where);
}
#endif

void StringList::DeleteAll()
{
	while (GetCount() != 0)
		DeleteAt(GetCount()-1);
}

void StringList::DeleteAt(long index)
{
	char* string = At(index);
	RemoveAt(index);
	if (string != nil)
		FreeTaggedMemory(string, "StringList");
}

long StringList::Find(const char* string) const
{
	long i;
	long count = GetCount();
	for (i = 0; i < count; i++)
		if (strcmp(string, (char*)At(i)) == 0)
			return i;
	
	return -1;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
char* StringList::Remove(char* string)
{
	long where = Find(string);
	
	if (where == -1)
		return nil;
	
	return RemoveAt(where);
}
#endif

char* StringList::RemoveAt(long where)
{
	char*	string = At(where);
	fDataList.RemoveAt(where);
	return string;
}

#ifdef TEST_SIMULATORONLY
void StringList::Test()
{
}
#endif /* TEST_SIMULATORONLY */

//===========================================================================

