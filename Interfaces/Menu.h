// ===========================================================================
//	Menu.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __MENU_H__
#define __MENU_H__

#ifndef __CONTROL_H__
#include "Control.h"
#endif
#ifndef __GRAPHICS_H__
#include "Graphics.h"
#endif
#ifndef __LAYER_H__
#include "Layer.h"
#endif
#ifndef __LINKABLE_H__
#include "Linkable.h"
#endif
#ifndef __LIST_H__
#include "List.h"
#endif
#ifndef __PARSER_H__
#include "Parser.h"
#endif

class Document;
class Menu;

// ===========================================================================

class MenuLayer : public Layer {
public:
							MenuLayer();
	virtual					~MenuLayer();
	
	void					SetTarget(Menu*, Document*);

	virtual Boolean			BackInput();
	virtual Boolean			DispatchInput(Input*);
	virtual Boolean			DownInput(Input*);
	virtual void			Draw(const Rectangle*);
#ifdef FIDO_INTERCEPT
	virtual void			Draw(class FidoCompatibilityState& fidoCompatibility) const;
#endif
	virtual Boolean			ExecuteInput();
	virtual void			Hide();
	virtual Boolean			KeyboardInput(Input*);
	virtual void			Show();
	virtual Boolean			UpInput(Input*);
	
protected:
	void					ComputeBounds();
	void					IncrementalSearch(Input*);

protected:
	Menu*					fMenu;	
	Document*				fDocument;
	ushort					fSelectedIndex;
	ushort					fFirstIndex;
	ushort					fVisibleCount;
	
	Boolean					fShouldDrawShadow;
};

// ===========================================================================

class MenuItem : public Linkable {
public:
							MenuItem();
	virtual					~MenuItem();
			
	void					Set(const char* name, const char* value, Boolean selected);

public:
	char*					fName;
	char*					fValue;
	Boolean					fSelected;
};

// ===========================================================================

class Menu : public Control {
public:
							Menu();
	virtual 				~Menu();

	void					AddItem(const char* name, const char* value, Boolean selected);

	ObjectList*				GetItems() const;
	ulong					GetMaxTextWidth(XFont, CharacterEncoding) const;
	long					IncrementalSearch(Input*) const;

	virtual void			SetAttribute(Attribute attributeID, long value, Boolean isPercentage);

protected:
	MenuItem*				GetItemFromName(const char* itemString) const;

protected:
	ObjectList				fItems;
	
	unsigned				fExclusive : 1;
};

// ===========================================================================

class PopUpMenu : public Menu {
public:
							PopUpMenu();
	virtual 				~PopUpMenu();

	virtual void			AddSubmission(DataStream*, Boolean force);
	virtual void			Draw(const Document*, const Rectangle*);
	virtual Boolean			ExecuteInput();
	MenuItem*				GetSelectedItem() const;
	virtual void			Layout(Document*, Displayable* parent);

	virtual void			RestoreFillinData(const char* data);
	virtual const char*		SaveFillinData() const;
	
protected:
};

// ===========================================================================

class ScrollingList : public Menu {
public:
							ScrollingList();
	virtual 				~ScrollingList();

	virtual void			AddSubmission(DataStream*, Boolean force);
	virtual void			Draw(const Document*, const Rectangle*);
	virtual Boolean			ExecuteInput();
	virtual AttributeValue	GetAlign() const;
	virtual Boolean			IsHighlightable() const;
	virtual Boolean			KeyboardInput(Input*);
	virtual void			Layout(Document*, Displayable* parent);

	virtual void			RestoreFillinData(const char* data);
	virtual const char*		SaveFillinData() const;
	
	virtual void			Select();
	virtual void			Deselect();
	
	virtual void			SetAttribute(Attribute attributeID, long value, Boolean isPercentage);

	virtual Boolean			UpInput();
	virtual Boolean			DownInput();
	
protected:
	void					InvalidateBounds();
	
protected:
	ushort					fFirstIndex;
	ushort					fSelectedIndex;
	ushort					fSize;
	
	unsigned				fActive : 1;
	unsigned				fMultiple : 1;
};

// ===========================================================================
#endif /*__MENU_H__ */