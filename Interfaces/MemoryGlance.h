// ===========================================================================
//	MemoryGlance.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __MEMORYGLANCE_H__
#define __MEMORYGLANCE_H__

#ifdef MEMORY_TRACKING

// ---------------------------------------------------------------------------

/* define MEMORYGLANCE_ACTIVE for hierarchical memory display window */
#define MEMORYGLANCE_ACTIVE	

#ifndef __HIERARCHICALDEBUGDISPLAY_H__
#include "HierarchicalDebugDisplay.h"
#endif

extern const ulong kRecentlyModifiedTime;	// mem op. considered "fresh"

struct MemoryTag;

// ---------------------------------------------------------------------------

class MemoryTrackerItem : public HierarchicalItem
{
	public:
									MemoryTrackerItem(void);
		virtual						~MemoryTrackerItem(void);

		void	 					DoTaggedAlloc(MemoryTag* tag); // on one node
		void						DoTaggedFree(MemoryTag* tag);
		void				 		DoTaggedNew(MemoryTag* tag);
		void				 		DoTaggedDelete(MemoryTag* tag);
		
		void						TaggedAlloc(MemoryTag* tag); // on all siblings
		void						TaggedFree(MemoryTag* tag);
		void						TaggedNew(MemoryTag* tag);
		void						TaggedDelete(MemoryTag* tag);
		
		virtual MacBoolean			IsSortedBefore(MemoryTrackerItem* item);
		
		void						AddSibling(MemoryTrackerItem* item);
		
		virtual MacBoolean			Match(MemoryTag* tag);
		virtual MemoryTrackerItem*	NewItem(MemoryTag* tag);
		MemoryTag*					FirstMatchingMemoryTag(void);
		MemoryTag*					NextMatchingMemoryTag(MemoryTag* tag);
		
		void						InitFromHeap(void);
	
		virtual HierarchicalItem*	DoClickNode(const MacPoint* where, ushort modifiers, HierarchicalWindow* destWindow);
		virtual void				DrawNode(const MacPoint* topLeft, HierarchicalWindow* destWindow);

		ulong						GetLastModified(void);
		MacBoolean					GetRecentlyModified(ulong timePeriod = kRecentlyModifiedTime);
		MacBoolean					GetPermanent(void);
		MacBoolean					GetSummaryItem(void);

		void						UpdateLastModified(ulong time);
		void						SetPermanent(MacBoolean permanent);
		void						SetSummaryItem(MacBoolean summary);
	
	protected:
		long						fNumBlocks;
		long						fLifetimeBlocks;
		long						fMaxBlocks;
		size_t						fSize;
		size_t						fLifetimeSize;
		size_t						fMaxSize;
		ulong						fLastModified;
		MacBoolean					fPermanent;
		MacBoolean					fSummary;
};

class TagItem : public MemoryTrackerItem
{
	public:
									TagItem(void);
		virtual						~TagItem(void);

		virtual MacBoolean			IsSortedBefore(MemoryTrackerItem* item);
		virtual MacBoolean			Match(MemoryTag* tag);
		virtual MemoryTrackerItem*	NewItem(MemoryTag* tag);

		virtual void				DrawNode(const MacPoint* topLeft, HierarchicalWindow* destWindow);
		virtual HierarchicalItem*	DoClickNode(const MacPoint* topLeft, ushort modifiers, HierarchicalWindow* destWindow);

	protected:
		void 						DrawNodeDefault(MacPoint* topLeft,
													short height, short width,
													MacBoolean expand);
		void 						DrawNodeDispatch(MacPoint* topLeft,
													short height, short width,
													MacBoolean expand);

	protected:
		MemoryTag*					fTag;
		MacBoolean					fMaxExpand;
};

class URLMemoryTrackerItem : public MemoryTrackerItem
{
	public:
									URLMemoryTrackerItem(void);
		virtual						~URLMemoryTrackerItem(void);

		virtual MacBoolean			IsSortedBefore(MemoryTrackerItem* item);
		virtual MacBoolean			Match(MemoryTag* tag);
		virtual MemoryTrackerItem*	NewItem(MemoryTag* tag);
		
		virtual void				DrawNode(const MacPoint* topLeft, HierarchicalWindow* destWindow);
		//virtual HierarchicalItem*	DoClickNode(const MacPoint* topLeft, ushort modifiers, HierarchicalWindow* destWindow);
		virtual void				SetNodeExpand(MacBoolean newExpand);

	protected:
		const char*					fURLName;
};

class BlockMemoryTrackerItem : public MemoryTrackerItem
{
	public:
									BlockMemoryTrackerItem(void);
		virtual						~BlockMemoryTrackerItem(void);

		virtual MacBoolean			IsSortedBefore(MemoryTrackerItem* item);
		virtual MacBoolean			Match(MemoryTag* tag);
		virtual MemoryTrackerItem*	NewItem(MemoryTag* tag);

		virtual void				DrawNode(const MacPoint* topLeft, HierarchicalWindow* destWindow);
		//virtual HierarchicalItem*	DoClickNode(const MacPoint* topLeft, ushort modifiers, HierarchicalWindow* destWindow);
		virtual void				SetNodeExpand(MacBoolean newExpand);
	
	protected:
		const char*					fBlockName;
};

// ---------------------------------------------------------------------------

struct MemoryGlanceWindowPrefs : public StdWindowPrefs
{
	MacBoolean		zeroBlocksVisible;
};

class MemoryGlanceWindow : public HierarchicalWindow
{
	public:
							MemoryGlanceWindow(void);
		virtual				~MemoryGlanceWindow(void);
		
		void 				DrawHeader(Rect*);
		virtual void		Close(void);
		void				Click(struct MacPoint *where, ushort modifiers);

		void				TaggedAlloc(MemoryTag* tag); // on all siblings
		void				TaggedFree(MemoryTag* tag);
		void				TaggedNew(MemoryTag* tag);
		void				TaggedDelete(MemoryTag* tag);
		
		void				SetZeroBlocksVisible(MacBoolean isVisible);
		MacBoolean			GetZeroBlocksVisible(void);

	protected:
		virtual Boolean		SavePrefs(StdWindowPrefs* prefPtr = nil);
		virtual Boolean		RestorePrefs(StdWindowPrefs* prefPtr = nil);
		virtual long		GetPrefsSize(void);

	protected:
		ControlHandle		fZeroBlocksControl;
		MacBoolean			fZeroBlocksVisible;
};


class URLGlanceWindow : public MemoryGlanceWindow
{
	public:
							URLGlanceWindow(void);
		virtual				~URLGlanceWindow(void);

		virtual void		DoAdjustMenus(ushort modifiers);
		virtual Boolean		DoMenuChoice(long menuChoice, ushort modifiers);

		virtual void		HideWindow(void);
		virtual void		ShowWindow(void);
};

class BlockGlanceWindow : public MemoryGlanceWindow
{
	public:
							BlockGlanceWindow(void);
		virtual				~BlockGlanceWindow(void);

		virtual void		DoAdjustMenus(ushort modifiers);
		virtual Boolean		DoMenuChoice(long menuChoice, ushort modifiers);

		virtual void		HideWindow(void);
		virtual void		ShowWindow(void);
};

class TagItemWindow : public MemoryGlanceWindow
{
	public:
							TagItemWindow(void);
		virtual				~TagItemWindow(void);

		virtual void		DoAdjustMenus(ushort modifiers);
		virtual Boolean		DoMenuChoice(long menuChoice, ushort modifiers);

		virtual void		HideWindow(void);
		virtual void		ShowWindow(void);
};


extern URLGlanceWindow*		gURLGlanceWindow;
extern BlockGlanceWindow*	gBlockGlanceWindow;
extern TagItemWindow*		gTagItemWindow;

#endif /* MEMORY_TRACKING */

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include MemoryGlance.h multiple times"
	#endif
#endif /* __MEMORYGLANCE_H__ */
