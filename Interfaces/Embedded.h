// ===========================================================================
//	Embedded.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __EMBEDDED_H__
#define __EMBEDDED_H__

#ifndef __DISPLAYABLE_H__
#include "Displayable.h"			/* SpatialDisplayable */
#endif
#ifndef __GRAPHICS_H__
#include "Graphics.h"				/* Rectangle */
#endif
#ifndef __PARSER_H__
#include "Parser.h"					/* Attribute */
#endif
#ifndef __RESOURCE_H__
#include "Resource.h"				/* Resource */
#endif




// =============================================================================

class DataStream;
class Document;
class Layer;

class Embedded : public SpatialDisplayable 
{
public:
							Embedded();
	virtual 				~Embedded();
	
	virtual void			SetAttribute(Attribute attributeID, long value, Boolean isPercentage);
	virtual void			SetAttributeStr(Attribute attributeID, const char* value);

	virtual void			Draw(const Document* document, const Rectangle* invalid);
	
	Boolean					Idle(Layer* layer);
	void					Layout(Document*, Displayable* parent);
	void					Load(const Resource* parent);

protected:
	Resource				fResource;
	char*					fSRC;
	DataStream*				fStream;	
};

// =============================================================================

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Embedded.h multiple times"
	#endif
#endif /*__EMBEDDED_H__ */
