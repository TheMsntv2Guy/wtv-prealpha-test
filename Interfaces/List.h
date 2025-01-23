#ifndef __LIST_H__
#define __LIST_H__

#ifndef __WTVTYPES_H__
#include "WTVTypes.h"
#endif

class DataList;
class DataIterator;
class Listable;
class ObjectList;
class ObjectIterator;
class StringList;

//===========================================================================
// List of data elements of a specified size.

class DataList {
public:
							DataList();
							~DataList();
	
	long					GetCount() const;
	short					GetDataSize() const;
	short					GetListIncrement() const;
	Boolean					IsEmpty() const;
	
	void					SetDataSize(short);
	void					SetListIncrement(short);
	
	void					Add(const void*);
	void					AddAll(const DataList*);
	void					AddAt(const void* data, long index);
	void*					At(long index) const;
	void					DeleteAll();
	DataIterator*			NewIterator() const;
	void					RemoveAt(long index);
	void					RemoveAll();
	void					RemoveAllAfter(long index);

#ifdef TEST_SIMULATORONLY
	static void				Test();
#endif /* TEST_SIMULATORONLY */

protected:
	long					fCount;
	long					fLength;
	uchar*					fList;

	short					fDataSize;
	short					fListIncrement;
};

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
inline short
DataList::GetListIncrement() const
{
	return fListIncrement;
}
#endif

//===========================================================================

class DataIterator {
public:
	virtual void*			GetFirst();
	virtual void*			GetNext();
	
public:
	void					SetTarget(const DataList*);

protected:
	const DataList*			fTarget;
	long					fNext;
};

//===========================================================================
// Objects that go into lists must descend from this class as the left-most
// base class.

class Listable {
public:
	virtual					~Listable();
};

//===========================================================================
// List of objects.

class ObjectList : public Listable {
public:
							ObjectList();
	virtual					~ObjectList();
							
	long					GetCount() const;
	short					GetListIncrement() const;
	Boolean					IsEmpty() const;
	
	void					SetListIncrement(short);
	
	void					Add(const Listable*);
	void					AddAll(const ObjectList*);
	void					AddAt(const Listable*, long index);
	Listable*				At(long index) const;
	void					Delete(Listable*);
	void					DeleteAll();
	void					DeleteAt(long index);
	long					Find(Listable*) const;	// returns -1 if not found
	virtual ObjectIterator*	NewIterator() const;
	Listable*				Remove(Listable*);
	void					RemoveAll();
	void					RemoveAllAfter(long index);
	Listable*				RemoveAt(long index);

#ifdef TEST_SIMULATORONLY
	static void				Test();
#endif /* TEST_SIMULATORONLY */

protected:
	DataList				fObjects;
};

//===========================================================================

class ObjectIterator {
public:
	virtual Listable*		GetFirst();
	virtual Listable*		GetNext();
	
public:
	void					SetTarget(const ObjectList*);

protected:
	const ObjectList*		fTarget;
	long					fNext;
};

//===========================================================================
// Dictionary of key/value strings.

class StringDictionary {
public:
							StringDictionary();
							~StringDictionary();
	
	long					GetCount() const;
	const char*				GetKeyAt(long index) const;
	const char*				GetValue(const char* key) const;
	const char*				GetValueAt(long index) const;
	long					GetValueLong(const char* key) const;
	long					GetValueLongAt(long index) const;
	Boolean					IsEmpty() const;

	void					Add(const char* key, const char* value);
	void					Add(const char* attributeString);	// e.g. name=John+Matheny&...
	void					Delete(const char* key);
	void					DeleteAll();

protected:
	void					DeleteAt(long index);

protected:
	DataList				fDataList;
};

//===========================================================================
// List of strings. The strings are copied and owned by the list.

class StringList {
public:
							StringList();
							~StringList();
	
	void					Add(const char* string);
	void					AddAt(const char* string, long index);
	char*					At(long index) const;
	void					Delete(char*);
	void					DeleteAll();
	void					DeleteAt(long index);
	long					Find(const char*) const;
	long					GetCount() const;
	Boolean					IsEmpty() const;
	char*					Remove(char*);
	char*					RemoveAt(long index);

#ifdef TEST_SIMULATORONLY
	static void				Test();
#endif /* TEST_SIMULATORONLY */

protected:
	DataList				fDataList;
};

//===========================================================================
// DataList Inlines

inline long DataList::GetCount() const
{
	return fCount;
}

//===========================================================================
// StringList Inlines

inline long StringList::GetCount() const
{
	return fDataList.GetCount();
}

inline Boolean StringList::IsEmpty() const
{
	return fDataList.IsEmpty();
}

//===========================================================================
// StringDictionary Inlines

inline long StringDictionary::GetCount() const
{
	return fDataList.GetCount();
}

inline Boolean StringDictionary::IsEmpty() const
{
	return fDataList.IsEmpty();
}

//===========================================================================
// ObjectList Inlines

inline void ObjectList::AddAt(const Listable* object, long index)
{
	fObjects.AddAt(&object, index);
}

inline void ObjectList::Add(const Listable* item)
{
	AddAt(item, fObjects.GetCount());
}

inline Listable* ObjectList::At(long index) const
{
	return *(Listable**)fObjects.At(index);
}

inline long ObjectList::GetCount() const
{
	return fObjects.GetCount();
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
inline short ObjectList::GetListIncrement() const
{
	return fObjects.GetListIncrement();
}
#endif

inline Boolean ObjectList::IsEmpty() const
{
	return fObjects.IsEmpty();
}

inline void ObjectList::SetListIncrement(short value)
{
	fObjects.SetListIncrement(value);
}

inline void ObjectList::RemoveAll()
{
	fObjects.RemoveAll();
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
inline void ObjectList::RemoveAllAfter(long index)
{
	fObjects.RemoveAllAfter(index);
}
#endif

inline Listable* ObjectList::RemoveAt(long index)
{
	Listable* result = At(index);
	fObjects.RemoveAt(index);
	return result;
}

//===========================================================================
#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include List.h multiple times"
	#endif
#endif /* __LIST_H__ */
