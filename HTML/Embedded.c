// ===========================================================================
//	Embedded.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __CONTENTVIEW_H__
#include "ContentView.h"
#endif
#ifndef __DOCUMENT_H__
#include "Document.h"
#endif
#ifndef __EMBEDDED_H__
#include "Embedded.h"
#endif
#ifndef __LAYER_H__
#include "Layer.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __PERFDUMP_H__
#include "PerfDump.h"
#endif
#ifndef __STREAM_H__
#include "Stream.h"
#endif




// =============================================================================

Embedded::Embedded()
{
#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberEmbedded;
#endif /* DEBUG_CLASSNUMBER */
}

Embedded::~Embedded()
{
	delete(fStream);
	
	if (fSRC != nil)
		FreeTaggedMemory(fSRC, "Embedded::fSRC");
}


#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
void Embedded::Draw(const Document* document, const Rectangle* invalid)
{
	Rectangle r;

	GetBounds(&r);
	document->GetView()->ContentToScreen(&r);
	
	::PaintRectangle(gScreenDevice, r, kBlueColor, 0, invalid);
}
#endif

Boolean Embedded::Idle(Layer* layer)
{
	PerfDump perfdump("Embedded::Idle");

	// Idles the resource

	if (fStream == nil)
		return false;

	// Check whether there is an error on the stream.
	if (TrueError(fStream->GetStatus())) 
	{
		delete(fStream);
		fStream = nil;
		return false;
	}
	
	// Check whether the stream is still coming in.
	if (fStream->GetStatus() == kComplete)
	{
		delete(fStream);
		fStream = nil;

		Rectangle	bounds;
		GetBounds(&bounds);
		layer->InvalidateBounds(&bounds);	// make sure it draws
	}
	
	return false;
}

void Embedded::Load(const Resource*)
{
#if 0 /* еее DRA Handle NewStream() returning nil. */
	if (IsError(parent == nil))
		return;
		
	if (fSRC == nil) {
		fState = kImageError;		
		return;
	}
	
	fResource.SetURL(fSRC, parent);
	FreeTaggedMemory(fSRC, "Image::fSRC");
	fSRC = nil;
#endif
}

void Embedded::Layout(Document*, Displayable*)
{
}

void Embedded::SetAttribute(Attribute attributeID, long value, Boolean isPercentage)
{
	switch (attributeID) 
	{		
		case A_WIDTH:			fWidth = MAX(value, 0);		break;
		case A_HEIGHT:			fHeight = MAX(value, 0);	break;

		default:				Displayable::SetAttribute(attributeID, value, isPercentage);
	}
}

void Embedded::SetAttributeStr(Attribute attributeID, const char* value)
{	
	switch (attributeID) 
	{		
		case A_SRC:	
			if (fSRC != nil)
				break;	// already have one
			TrivialMessage(("Setting %x->SRC = '%s'", (ulong)this, value));
			fSRC = CopyStringTo(fSRC, value, "Embedded::fSRC");
			break;
		default:
			Displayable::SetAttributeStr(attributeID, value);
	}
}

// =============================================================================

