// ===========================================================================
//	RecentPanel.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __RECENTPANEL_H__
#define __RECENTPANEL_H__

#ifndef __LIST_H__
#include "List.h"
#endif

#ifndef __PANEL_H__
#include "Panel.h"
#endif

#ifndef __RESOURCE_H__
#include "Resource.h"
#endif

//==================================================================================

class RecentURL : public Listable {
public:
							RecentURL();
	virtual					~RecentURL();

	char*					CopyURL(const char* tagName) const;
	const Resource*			GetResource() const;
	const Resource*			GetThumbnail() const;
	const char*				GetTitle() const;
	long					LastVisited() const;

	void					SetResource(const Resource*);
	void					SetThumbnail(const Resource*);
	void					SetTitle(const char*);
	void					Touch();

protected:
	Resource				fResource;
	Resource				fThumbnail;
	char*					fTitle;
	long					fLastVisited;
};

//==================================================================================

class Layer;

class RecentPanel : public Panel {
public:
							RecentPanel();
	virtual					~RecentPanel();
	
	void					AddPage(const Resource* resource, const char* title,
									const Resource* thumbnail);
	void					ClearPages();
	virtual void			ExecuteURL(const char* url, const char* formData);
	long					FindPage(const Resource* resource) const;
	long					GetCount() const;
	virtual void			Open();
	RecentURL*				RecentURLAt(long) const;
	const char*				TitleAt(long) const;
	void					WriteHTML();

protected:
	ObjectList				fPageList;
};

//==================================================================================

inline const Resource* RecentURL::GetResource() const		{ return &fResource; }
inline long RecentURL::LastVisited() const					{ return fLastVisited; }

//==================================================================================

const long  kInvalidPage = -1;

//==================================================================================

#endif /* __RECENTPANEL_H__ */
