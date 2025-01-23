// ===========================================================================
//	HierarchicalDebugDisplay.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __HIERARCHICALDEBUGDISPLAY_H__
#include "HierarchicalDebugDisplay.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif





// ===========================================================================

HierarchicalItem::HierarchicalItem(void)
{
	fParent = nil;
	fFirstChild = nil;
	fHeight = 12;
	fExpand = false;
}

HierarchicalItem::~HierarchicalItem(void)
{
	// fix up me w/respect to parents
		if (fParent == nil)
		{
			// done with parent and siblings
		}
		else if (fParent->fFirstChild == this)	// if parent thinks I'm first child
		{	
			Assert(Previous() == nil);	// hey!  we weren't supposed to be the leftmost child!
			fParent->fFirstChild = (HierarchicalItem*)Next();	// next person moves up
		}
		fParent = nil;	// in any case, remove knowledge of parent
	
	// remove me from my siblings
		Remove();
	
	// delete my children
	if (fFirstChild != nil)
	{
		fFirstChild->DeleteAll();
		fFirstChild = nil;
	}
}

// ---------------------------------------------------------------------------

void
HierarchicalItem::DrawNode(const MacPoint* where, HierarchicalWindow* destWindow)
{
	MoveTo(where->h, where->v + destWindow->GetAscentHeight());
	
	int numChildren = (fFirstChild == nil) ? 0 : fFirstChild->Count();
	char text[64];
	DrawText(text, 0, snprintf(text, sizeof(text),
								"%cSampleNode: %d child%s%s expanded",
								(fFirstChild == nil) ? '-' : (fExpand ? '+' : '¥'),
								numChildren,
								(numChildren != 1) ? "ren are " : " is ",
								fExpand ? "" : "not "));
}

void
HierarchicalItem::Draw(MacPoint* where, HierarchicalWindow* destWindow)
{
	HierarchicalItem* node = this;
	short indent = 4 * destWindow->GetCharWidth();
	
	while ((node != nil))
	{
		node->DrawNode(where, destWindow);
		where->v += node->fHeight;
		where->h += indent;	// match this with outdent, below!
		if (node->GetNodeExpand())
		{	
			HierarchicalItem* child = node->fFirstChild;
			Assert((child == nil) || (child == (HierarchicalItem*)child->First()));	// should be first
	
			child->Draw(where, destWindow);
		}
		where->h -= indent;	// match this with indent, above!
		node = (HierarchicalItem*)(node->Next());
	}
}

// ---------------------------------------------------------------------------

HierarchicalItem*
HierarchicalItem::DoClickNode(const MacPoint* UNUSED(where), ushort modifiers, HierarchicalWindow* UNUSED(destWindow))
{
	if (modifiers & optionKey)
	{	SetTreeExpand(!GetNodeExpand());
	}
	else
	{	SetNodeExpand(!GetNodeExpand());
	}
	return this;
}

HierarchicalItem*
HierarchicalItem::Click(MacPoint* where, ushort modifiers, HierarchicalWindow* destWindow)
{
	HierarchicalItem* node = this;
	HierarchicalItem* result = nil;
	short indent = destWindow->GetCharWidth();
	
	while (node != nil)
	{
		if (where->v < 0)
			break;
		if ((where->v >= 0) && (where->v < node->fHeight))
		{
			result = node->DoClickNode(where, modifiers, destWindow);	// found it
			break;
		}
		where->v -= node->fHeight;
		where->h -= indent;	// match this with outdent, below!
		if (node->fExpand)
		{	
			HierarchicalItem* child = node->fFirstChild;
			Assert((child == nil) || (child == (HierarchicalItem*)child->First()));	// should be first

			result = child->Click(where, modifiers, destWindow);
			if (result != nil)
				break;	// one of the children found it
		}
		where->h += indent; // match this with indent, above!
		node = (HierarchicalItem*)(node->Next());
	}
	return result;
}
// ---------------------------------------------------------------------------
MacBoolean
HierarchicalItem::GetNodeExpand(void)
{
	return fExpand;
}

void
HierarchicalItem::SetNodeExpand(MacBoolean newExpand)
{
	fExpand = newExpand;
}

void
HierarchicalItem::SetTreeExpand(MacBoolean newExpand)
{
	SetNodeExpand(newExpand);
	
	HierarchicalItem* child = fFirstChild;
	while (child != nil)
	{	child->SetTreeExpand(newExpand);
		child = (HierarchicalItem*)(child->Next());
	}
}

void
HierarchicalItem::AddChild(HierarchicalItem* newChild)
{
	if (newChild != nil)
	{
		if (fFirstChild)
		{
			fFirstChild->AddSibling(newChild);
		}
		else
		{
			fFirstChild = newChild;
			newChild->fParent = this;
		}
	}
}

void
HierarchicalItem::RemoveChild(HierarchicalItem* oldChild)
{
	if (oldChild != nil)
	{
		if (fFirstChild)
		{
			fFirstChild->RemoveSibling(oldChild);
		}
	}
}

void
HierarchicalItem::RemoveAllChildren(void)
{
	if (fFirstChild)
	{
		fFirstChild->RemoveAllSiblings();
	}
}

void
HierarchicalItem::AddSibling(HierarchicalItem* newSibling)
{
	Add(newSibling);
	newSibling->fParent = fParent;
}

void
HierarchicalItem::AddAfterSibling(HierarchicalItem* newSibling)
{
	AddAfter(newSibling);
	newSibling->fParent = fParent;
}

void
HierarchicalItem::RemoveSibling(HierarchicalItem* oldSibling)
{
	if (oldSibling != nil)
	{
		if ((oldSibling->fParent != nil) && (oldSibling->fParent->fFirstChild == oldSibling))
		{
			oldSibling->fParent->fFirstChild = (HierarchicalItem*)(oldSibling->Next());
		}
		oldSibling->Remove();
		oldSibling->fParent = nil;
	}
}

void
HierarchicalItem::RemoveAllSiblings(void)
{
	HierarchicalItem* temp = (HierarchicalItem*)First();
	while (temp->Next())
	{
		temp->RemoveSibling((HierarchicalItem*)(temp->Next()));
	}
	temp->RemoveSibling(temp);
}

// ---------------------------------------------------------------------------

HierarchicalWindow::HierarchicalWindow() : StdWindow()
{
	SetFormat(0, 0, true, true);	// Header, trailer, vscroll, hscroll
	SetFont(kDefaultFont, kDefaultFontSize);
	fRootItem = nil;
	fRequestRedraw = false;
}

HierarchicalWindow::~HierarchicalWindow()
{
	if (fRootItem != nil)
	{
		delete(fRootItem);
		fRootItem = nil;
	}
}

const kHierarchicalWindowTopMargin = 4;
const kHierarchicalWindowLeftMargin = 4;

void
HierarchicalWindow::Click(struct MacPoint *where, ushort modifiers)
{
	struct Rect bodyRect;
	GetBodyRect(&bodyRect);
	if (PtInRect(*where, &bodyRect))
	{
		struct MacPoint pt;

		pt.h = where->h - kHierarchicalWindowLeftMargin;
		pt.v = where->v - kHierarchicalWindowTopMargin;
		
		pt.h -= bodyRect.left;	// take out of window coordinates
		pt.v -= bodyRect.top;

		BodyCoordinates(&bodyRect);		
		
		pt.h += bodyRect.left;	// put into body coordinates
		pt.v += bodyRect.top;

		if (fRootItem->Click(&pt, modifiers, this))
		{	InvalRect(&bodyRect);
		}

		SetOrigin(0,0);
	}
	else
	{
		StdWindow::Click(where, modifiers);
	}
}

void
HierarchicalWindow::DrawBody(struct Rect *r, short hScroll, short vScroll, Boolean UNUSED(scrolling))
{
	EraseRect(r);
	
	struct MacPoint pt;
	pt.h = kHierarchicalWindowLeftMargin + r->left - hScroll;	// scoot it in from margin
	pt.v = kHierarchicalWindowTopMargin + r->top - vScroll;	// scoot it down from top

	short saveFont = qd.thePort->txFont;
	short saveFontSize = qd.thePort->txSize;
	short saveVert = pt.v;
	
	TextFont(fFont);
	TextSize(fFontSize);
	fRootItem->Draw(&pt, this);
	TextFont(saveFont);
	TextSize(saveFontSize);
	
	short bodyHeight = pt.v + vScroll - r->top;
	SetBodySize(1024, bodyHeight);

	if ((r->bottom > pt.v) && (vScroll > 0))
	{
		short scrollAmount = r->bottom - pt.v;
		
		if (vScroll < scrollAmount)
			scrollAmount = vScroll;
	
		ScrollBody(0, scrollAmount);
	}

}

void
HierarchicalWindow::RequestRedraw(void)
{
	fRequestRedraw = true;
}


void
HierarchicalWindow::Idle(void)
{
	if (fRequestRedraw)
	{
		GrafPtr savePort;
		GetPort(&savePort);
		SetPort(w);
	
		InvalRect(&(w->portRect));
	
		//struct Rect bodyRect;
		//GetBodyRect(&bodyRect);
		//BodyCoordinates(&bodyRect);
		//InvalRect(&bodyRect);
	
		SetPort(savePort);
		fRequestRedraw = false;
	}
	StdWindow::Idle();
}

void
HierarchicalWindow::AddItem(HierarchicalItem* newItem)
{
	if (fRootItem == nil)
	{	fRootItem = newItem;
	}
	else
	{	fRootItem->AddSibling(newItem);
	}
}

HierarchicalItem*
HierarchicalWindow::RemoveItem(HierarchicalItem* oldItem)
{
	if (fRootItem != nil)
	{
		fRootItem->RemoveSibling(oldItem);
	}
	return oldItem;
}

short
HierarchicalWindow::GetFont(void)
{
	return fFont;
}

short
HierarchicalWindow::GetFontSize(void)
{
	return fFontSize;
}

short
HierarchicalWindow::GetLineHeight(void)
{
	return fLineHeight;
}

short
HierarchicalWindow::GetAscentHeight(void)
{
	return fAscentHeight;
}

short
HierarchicalWindow::GetCharWidth(void)
{
	return fCharWidth;
}

HierarchicalItem*
HierarchicalWindow::GetRootItem(void)
{
	return fRootItem;
}

HierarchicalItem*
HierarchicalWindow::RemoveRootItem(void)
{
	HierarchicalItem* result = fRootItem;
	fRootItem = nil;
	return result;
}

void
HierarchicalWindow::SetFont(short font)
{
	SetFont(font, fFontSize);
}

void
HierarchicalWindow::SetFontSize(short size)
{
	SetFont(fFont, size);
}

void
HierarchicalWindow::SetFont(short font, short size)
{
	fFont = font;
	fFontSize = size;
	TextFont(fFont);
	TextSize(fFontSize);

	FontInfo finfo;
	GetFontInfo(&finfo);

	fLineHeight = finfo.ascent + finfo.descent + finfo.leading + 1;
	fAscentHeight = finfo.ascent + 1;
	fCharWidth = finfo.widMax;

	mVLineScroll = fLineHeight;
	mHLineScroll = fCharWidth;
}

void
HierarchicalWindow::SetRootItem(HierarchicalItem* rootItem)
{
	fRootItem = rootItem;
}


// ---------------------------------------------------------------------------

void
TestHierarchicalWindow(void)
{
	HierarchicalWindow* hw = newMac(HierarchicalWindow);
	
	HierarchicalItem* item1 = newMac(HierarchicalItem);
	hw->AddItem(item1);

	HierarchicalItem* item2 = newMac(HierarchicalItem);
		HierarchicalItem* subItem2_1 = newMac(HierarchicalItem);
		HierarchicalItem* subItem2_2 = newMac(HierarchicalItem);
	hw->AddItem(item2);
		item2->AddChild(subItem2_1);
		item2->AddChild(subItem2_2);

	HierarchicalItem* item3 = newMac(HierarchicalItem);
		HierarchicalItem* subItem3_1 = newMac(HierarchicalItem);
	hw->AddItem(item3);
		item3->AddChild(subItem3_1);


	HierarchicalItem* item4 = newMac(HierarchicalItem);
		HierarchicalItem* subItem4_1 = newMac(HierarchicalItem);
		HierarchicalItem* subItem4_2 = newMac(HierarchicalItem);
			HierarchicalItem* subSubItem4_2_1 = newMac(HierarchicalItem);
			HierarchicalItem* subSubItem4_2_2 = newMac(HierarchicalItem);
		HierarchicalItem* subItem4_3 = newMac(HierarchicalItem);
		HierarchicalItem* subItem4_4 = newMac(HierarchicalItem);
			HierarchicalItem* subSubItem4_4_1 = newMac(HierarchicalItem);
	hw->AddItem(item4);
		item4->AddChild(subItem4_1);
		item4->AddChild(subItem4_2);
			subItem4_2->AddChild(subSubItem4_2_1);
			subItem4_2->AddChild(subSubItem4_2_2);
		item4->AddChild(subItem4_3);
		item4->AddChild(subItem4_4);
			subItem4_4->AddChild(subSubItem4_4_1);
	
	hw->ResizeWindow(480, 10);
	hw->SetTitle((const char*)"MEMORYGLANCE_ACTIVE not #defined in MemoryGlance.h");
	hw->ShowWindow();

	return;
}
