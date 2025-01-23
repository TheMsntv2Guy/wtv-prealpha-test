// ===========================================================================
//	Layer.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __LAYER_H__
#include "Layer.h"
#endif

#include "ContentView.h"
#include "PageViewer.h"

#if defined(FOR_MAC)
	#ifndef __MACSIMULATOR_H__
	#include "MacSimulator.h"
	#endif
#endif

// ===========================================================================
// implementation
//============================================================================

Layer::Layer()
{
	fIsVisible = false;
	fPartiallyTransparent = false;
}

Layer::~Layer()
{
}

TextField* 
Layer::GetKeyboardTarget() const
{
	if (fIsVisible && IsContentView()) {
		Displayable* selected = ((ContentView*)this)->GetCurrentSelectable();
		
		if (selected != nil && selected->IsTextField())
			return (TextField*)selected;
	}
	
	Layer* next = (Layer*)Next();
	
	if (next == nil)
		return nil;
	
	return next->GetKeyboardTarget();
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
ContentView* 
Layer::GetTopContentView() const
{
	if (fIsVisible && IsContentView())
		return (ContentView*)this;
	
	Layer* next = (Layer*)Next();
	
	if (next == nil)
		return nil;
	
	return next->GetTopContentView();
}
#endif

Layer* 
Layer::GetTopLayer() const
{
	if (fIsVisible)
		return (Layer*)this;
	
	Layer* next = (Layer*)Next();
	
	if (next == nil)
		return nil;
	
	return next->GetTopLayer();
}

void 
Layer::Hide()
{
	if (!fIsVisible)
		return;
		
	// Hide children as well.
	for (Layer* child = fChild; child; child = (Layer*)child->Next())
		child->Hide();

	Rectangle bounds;
	GetBounds(&bounds);
	fIsVisible = false;
	InvalidateBehind(&bounds);
}

void 
Layer::InvalidateBounds(const Rectangle* bounds)
{
	Rectangle layerBounds;
	
	if (!IsVisible())
		return;

	// Invalidate children as well. They are in front of this layer.
	for (Layer* child = fChild; child; child = (Layer*)child->Next())
		child->InvalidateBounds(bounds);
	
	GetBounds(&layerBounds);
	IsError(gScreen->GetInDrawingLoop());
	
	if (bounds == nil)
		bounds = &layerBounds;
	else {
		// intersect w/ region bounds
		Rectangle sectBounds = *bounds;

		IntersectRectangle(&sectBounds, &layerBounds);
		bounds = &sectBounds;
		if (EmptyRectangle(bounds))
			return;		// nothing to update for this layer
	}
	
	fInvalidRegion.Add(bounds);
	gScreen->SetIsDirty(true);
#if defined(FOR_MAC)
	gMacSimulator->ForceUpdate();
#endif
}

Boolean 
Layer::IsContentView() const
{
	return false;
}

void 
Layer::DrawBehind()
{
	Layer* layer;
	
	for (layer=gPageViewer; layer != nil && layer != this;  layer = (Layer*)layer->Previous())
		if (layer->fIsVisible)
			layer->DrawLayer();
}


void 
Layer::InvalidateAbove(const Rectangle* bounds)
{
	// temporary, until we stop overdrawing
	Layer*	layer = this;
	Boolean	saveInDrawingLoop = gScreen->GetInDrawingLoop();
	
	gScreen->SetInDrawingLoop(false);
	while ((layer = (Layer*)layer->Previous()) != nil)
		if (layer->fIsVisible)
			layer->InvalidateBounds(bounds);
	for (layer = fChild; layer != nil; layer = (Layer*)layer->Next())
		if (layer->fIsVisible)
			layer->InvalidateBounds(bounds);
	gScreen->SetInDrawingLoop(saveInDrawingLoop);
}

void 
Layer::InvalidateBehind(const Rectangle* bounds)
{
	Layer*	layer = this;
	
	while ((layer = (Layer*)layer->Next()) != nil)
		if (layer->fIsVisible)
			layer->InvalidateBounds(bounds);
}

Boolean 
Layer::IsVisible() const
{
	return fIsVisible;
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
inline Boolean
Layer::IsWithinBounds(const Rectangle* bounds) const
{
	return fInvalidRegion.IsWithin(bounds);
}
#endif

inline Boolean 
Layer::Contains(const Rectangle* bounds) const
{
	return RectangleContainedInRectangle(bounds, &fBounds);
}

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
Layer* 
Layer::GetChild() const
{
	return fChild;
}
#endif

#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
Layer* 
Layer::GetParent() const
{
	return fParent;
}
#endif

void 
Layer::AddChild(Layer* child, long UNUSED(layerID))
{
	AddOrSet(fChild, child);
	child->fParent = this;
	// еее <UNUSED_fLayerID> child->fLayerID = layerID;
}

void 
Layer::Draw(const Rectangle* /*invalidBounds*/)
{
}

#ifdef FIDO_INTERCEPT
void 
Layer::Draw(class FidoCompatibilityState&) const
{
}
#endif

void 
Layer::DrawLayerInBounds(const Rectangle* invalidBounds)
{
	Layer*		layer;
		
	// check if covered
	Boolean	saveInDrawingLoop = gScreen->GetInDrawingLoop();
	for (layer = (Layer*)Previous(); layer != nil; layer = (Layer*)layer->Previous())
		if (layer->fIsVisible) {
			gScreen->SetInDrawingLoop(false);
			layer->InvalidateBounds(invalidBounds);	// force guy above to draw it too
			gScreen->SetInDrawingLoop(saveInDrawingLoop);
			if (!layer->fPartiallyTransparent && layer->Contains(invalidBounds))
				return;								// someone else will handle it!
		}
	
	for (layer = fChild; layer != nil; layer = (Layer*)layer->Next())
		if (layer->fIsVisible) {
			gScreen->SetInDrawingLoop(false);
			layer->InvalidateBounds(invalidBounds);	// force child to draw it too
			gScreen->SetInDrawingLoop(saveInDrawingLoop);
		}
	
	if (this->fPartiallyTransparent) {	
		// draw everything below to get correct effect
		// If this layer has a parent, draw the parent, otherwise draw previous
		// layers in the list and their children.
		if (fParent != nil)
			fParent->DrawLayerInBounds(invalidBounds);
		else {
			for (layer = (Layer*)Last(); layer != nil && layer != this;
				 layer = (Layer*)layer->Previous())
				if (layer->fIsVisible) {
					layer->DrawLayerInBounds(invalidBounds);
					for (Layer* child = layer->fChild; child != nil; child = (Layer*)child->Next())
						if (child->fIsVisible)
							child->DrawLayerInBounds(invalidBounds);
				}
		}
	}
	
	Draw(invalidBounds);
}

void 
Layer::DrawLayer()
// DrawLayer makes a decision about the composition of the invalid region
// and then calls DrawLayerInBounds to deal with drawing to refresh
// the region.
{
	// forget me and children if invisible
	if (!fIsVisible)
		return;
		
	// Draw ourself before children.	
	if (!fInvalidRegion.IsEmpty()) {
		// for now just always do total bounds
		Rectangle	invalidBounds;
		fInvalidRegion.GetBounds(&invalidBounds);
		DrawLayerInBounds(&invalidBounds);
	}
			
	fInvalidRegion.Reset();
	
	// Draw children back to front.
	if (fChild != nil)
		for (Layer* child = (Layer*)fChild->Last(); child; child = (Layer*)child->Previous())
			child->DrawLayer();
}


#ifdef FIDO_INTERCEPT
void 
Layer::DrawLayer(FidoCompatibilityState& fidoCompatibility) const
// DrawLayer makes a decision about the composition of the invalid region
// and then calls DrawLayerInBounds to deal with drawing to refresh
// the region.
{
	// forget me and children if invisible
	if (!fIsVisible)
		return;
		
	Draw(fidoCompatibility);
	
	// Draw children back to front.
	if (fChild != nil)
		for (Layer* child = (Layer*)fChild->Last(); child; child = (Layer*)child->Previous())
			child->DrawLayer(fidoCompatibility);
}
#endif

void 
Layer::Idle()
{
}

void 
Layer::IdleLayer()
{
	// Idle children, and then ourself.
	for (Layer* child = fChild; child != nil; child = (Layer*)child->Next())
		child->IdleLayer();
	
	Idle();
}

void 
Layer::Show()
{
	if (fIsVisible)
		return;

	// Hide children as well.
	for (Layer* child = fChild; child; child = (Layer*)child->Next())
		child->Show();
		
	Rectangle bounds;
	GetBounds(&bounds);
	fIsVisible = true;
	InvalidateBounds(&bounds);
}
			
//===================================================================================

