// ===========================================================================
//	HierarchicalDebugDisplay.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __HIERARCHICALDEBUGDISPLAY_H__
#define __HIERARCHICALDEBUGDISPLAY_H__

#ifndef __LINKABLE_H__
#include "Linkable.h"
#endif

#ifndef __STDWINDOW_H__
#include "StdWindow.h"
#endif




// --------------------------------------------------------------------------------

class HierarchicalWindow;

class HierarchicalItem : public Linkable
{
public:
								HierarchicalItem(void);
	virtual						~HierarchicalItem(void);

	virtual void				DrawNode(const MacPoint* topLeft, HierarchicalWindow* destWindow);
	void						Draw(MacPoint* topLeft, HierarchicalWindow* destWindow);
	
	virtual HierarchicalItem*	DoClickNode(const MacPoint* topLeft, ushort modifiers, HierarchicalWindow* destWindow);
	HierarchicalItem*			Click(MacPoint* topLeft, ushort modifiers, HierarchicalWindow* destWindow);

	MacBoolean					GetNodeExpand(void);
	virtual void				SetNodeExpand(MacBoolean newExpand);
	void						SetTreeExpand(MacBoolean newExpand);

	void						AddChild(HierarchicalItem* newChild);
	void						RemoveChild(HierarchicalItem* newChild);
	void						RemoveAllChildren(void);
	
	void						AddSibling(HierarchicalItem* newChild);
	void						AddAfterSibling(HierarchicalItem* newChild);
	void						RemoveSibling(HierarchicalItem* newChild);
	void						RemoveAllSiblings(void);
	
protected:
	HierarchicalItem*			fParent;
	HierarchicalItem*			fFirstChild;
	short						fHeight;		// my height
	MacBoolean					fExpand;		// whether or not to traverse children
};

class HierarchicalWindow : public StdWindow
{
	public:
								HierarchicalWindow(void);
		virtual					~HierarchicalWindow(void);
		
		virtual void			Click(struct MacPoint *where, ushort modifiers);
		virtual	void			DrawBody(struct Rect *r, short hScroll, short vScroll, Boolean scrolling);
		virtual void			Idle();
		void					RequestRedraw(void);
		
		void					AddItem(HierarchicalItem* newItem);
		HierarchicalItem*		RemoveItem(HierarchicalItem* oldItem);
		
		short					GetFont(void);
		short					GetFontSize(void);
		short					GetLineHeight(void);
		short					GetAscentHeight(void);
		short					GetCharWidth(void);
		HierarchicalItem*		GetRootItem(void);
		HierarchicalItem*		RemoveRootItem(void);

		void					SetFont(short font);
		void					SetFontSize(short size);
		void					SetRootItem(HierarchicalItem* rootItem);

	protected:
		void					SetFont(short font, short size);
		
	protected:
		enum { kDefaultFont = monaco };
		enum { kDefaultFontSize = 9 };

		HierarchicalItem*		fRootItem;
		short					fFont;
		short					fFontSize;
		short					fLineHeight;
		short					fAscentHeight;
		short					fCharWidth;
		MacBoolean				fRequestRedraw;
};

void TestHierarchicalWindow(void);

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include HierarchicalDebugDisplay.h multiple times"
	#endif
#endif /* __HIERARCHICALDEBUGDISPLAY_H__ */
