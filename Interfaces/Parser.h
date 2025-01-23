// ===========================================================================
//	Parser.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __PARSER_H__
#define __PARSER_H__

class Builder;
class DataStream;
class Tag;
class TagList;

// =============================================================================

class Resource;

// Parser is the base class for all parsers producing tag-based output from the HTML tables.

class Parser {
public:
							Parser();
	virtual					~Parser();

	virtual void			Parse(DataStream*);
	void					SetBuilder(Builder* builder);

protected:	
	Builder*				fBuilder;	// Destination for parsed info
};

// =============================================================================

// HTMLParser parses full HTML source to produce tag-based output from the HTML tables.

class HTMLParser : public Parser {
public:
							HTMLParser();
	virtual					~HTMLParser();
			
	short					GetTagID() const;

	void					SetResource(Resource* resource);

	void					BuildTagList(short tagID);
	void					CheckTag(short tagID);
	Boolean					DispatchTag();
	void					DispatchText();
	void					DispatchTextAreaValue();
	static const char*		InterpretEntityString(char* amp, long stringLength, short* terminator);
	char*					NextAttributeStrings(char* s, char** attributeStr, char** valueStr);
	virtual void			Parse(DataStream*);

protected:	
	enum {
		kMaxAmpersandLength = 10,
		kMaxTagLength = 4096,
		kMaxTextLength = 1024,
		kMaxJapaneseTextLength = 512
	};


#if defined(FOR_MAC)
	Resource*				fResource;	// Who dispatched us?
#endif
	TagList*				fTagList;
	short					fAmpersandCount;
	short					fTagCount;
	short					fTextCount;
	
	char					fTag[kMaxTagLength+1];
	char					fText[kMaxTextLength+1];
	char					fAmpersand[kMaxAmpersandLength];
	char					fCommentBeginCount;
	char					fCommentEndCount;

	short					fJapaneseTextCount;
	char					fJapaneseText[kMaxJapaneseTextLength+1];
	char					fJapaneseTextType;

	unsigned				fIgnoreSemiColon : 1;
	unsigned				fInAmpersand : 1;
	unsigned				fInBody : 1;
	unsigned				fInComment : 1;
	unsigned				fInOption : 1;
	unsigned				fInPRE : 1;
	unsigned				fInTag : 1;
	unsigned				fInTextArea : 1;
	unsigned				fStripSpaces : 1;
	unsigned				fInEscape : 1;
	unsigned				fInJapanese : 1;
	unsigned				fSkipNext : 1;
};

// =============================================================================

// PlainTextParser parses full plain text source to produce tag-based output.

class PlainTextParser : public Parser {
public:
							PlainTextParser();
	virtual					~PlainTextParser();
			
	void					DispatchText();
	virtual void			Parse(DataStream*);
	
protected:	
	enum {
		kMaxTextLength = 1024
	};

	char					fText[kMaxTextLength+1];
	short					fTextCount;
};

// =============================================================================

// ImageMapParser parses server image maps to produde tag-based output from the HTML tables.

class ImageMapParser : public Parser {
public:
							ImageMapParser();
	virtual					~ImageMapParser();
			
	virtual void			Parse(DataStream*);

protected:	
	const char*				CopyToEndOfLine(char* dest, const char* source, const char* end);
};

// =============================================================================
// HTML Tags and Attributes

#define kHTMLStyleFonts	16
#define kHTMLStyles		22

enum {
	T_BAD = 0,			// Bad Tag
	T_HTML,				// 
	T_HEAD,				// Document meta info				5.2
	T_TITLE,			// Document Title					5.2.1
	T_BASE,				// Document Base URL				5.2.2
	T_LINK,				// Relavent link for document		5.2.4
	T_META,				// Meta information for document	5.2.5
	
	T_BGSOUND,			// Background sound (Explorer 2.0)
	T_BODY,				// Displayed text body				5.3
	T_H1,T_H2,T_H3,		// Headings							5.4
	T_H4,T_H5,T_H6,
						// Block Structuring				5.5
	T_P,				// Paragraph						5.5.1
	T_PRE,				// Preformatted Text				5.5.2
	T_XMP,T_LISTING,	// Example and listing				5.5.2.1
	T_ADDRESS,			// Provide Address info				5.5.3
	T_BLOCKQUOTE,		// Block Quotations					5.5.4
	
						// List Elements					5.6
	T_UL,T_LI,			// Unordered list, elem				5.6.1
	T_OL,				// Ordered list						5.6.2
	T_DIR,				// Directory List					5.6.3
	T_MENU,				// Menu List						5.6.4
	T_DL,T_DT,T_DD,		// Definition list					5.6.5
	
						// Phrase Markup					5.7
	T_CITE,				// Citations						5.7.1.1
	T_CODE,				// Typed code						5.7.1.2
	T_EM,				// Emphasis							5.7.1.3
	T_KBD,				// Keyboard							5.7.1.4
	T_SAMP,				// Literal characters				5.7.1.5
	T_STRONG,			// Strong Emphasis					5.7.1.6
	T_VAR,				// A Variable						5.7.1.7
	
						// Typographic Elements				5.7.2
	T_B,				// Bold								5.7.2.1
	T_I,				// Italics							5.7.2.2
	T_TT,				// Typewriter Font					5.7.2.3
	T_U,				// Underline						5.7.2 (optional)
	T_SUB,				// Subscript						Netscape
	T_SUP,				// Superscript						Netscape
	T_A,				// Anchor							5.7.3	HREF,NAME,TITLE,REL,REV,URN,METHODS
	T_BR,				// Line break						5.8
	T_HR,				// Horizontal rule					5.9
	T_IMG,				// Image							5.10	ALIGN,ALT,ISMAP,SRC,HSPACE,BORDER,WIDTH,HEIGHT,ANI
	T_IMAGE,
		
	T_FORM,				// Forms							8.1.1	ACTION,METHOD,ENCTYPE
	T_INPUT,			// Text,Password,Check,Radio		8.1.2	TYPE,NAME,MAXLENGTH,SIZE,VALUE,CHECKED,IMAGE,HIDDEN,SUBMIT,RESET
	T_SELECT,			// Select							8.1.3	MULTIPLE,NAME,SIZE
	T_OPTION,			// Someting to select from			8.1.3.1	SELECTED,VALUE
	T_TEXTAREA,			// Multi-line text field			8.1.4	COLS,NAME,ROWS
	
	T_TABLE,			// Tables							p148, 	BORDER, "HTML Sourcebook"
	T_CAPTION,			// Table Caption
	T_DFN,				//	Definition
	T_TH,				// Table heading					p150	ALIGN,COLSPAN,ROWSPAN
	T_TD,				// Table Element					p153	ALIGN,COLSPAN,ROWSPAN
	T_TR,				// Table end of row
	T_EMBED,			// Embedded document				http://home.netscape.com/assist/net_sites/embed_tag.html
						//									SRC, WIDTH, HEIGHT
	T_FN,				// Footnote							HTML 3.0
	T_DIV,				// Division							HTML 3.0
	T_MAP,				// Client-side image map			http://home.netscape.com/assist/net_sites/embed_tag.html
	T_AREA,				//									SRC, WIDTH, HEIGHT
	T_FRAME,
	T_NOFRAME,
	T_NOFRAMES,

						// Wacky Artemis HTML	
	T_SHADOW,
	T_DISPLAY,
	T_SIDEBAR,
	
						// Wacky Netscape HTML
	T_CENTER,
	T_FONT,				// 									SIZE
	T_BIG,
	T_SMALL,
	T_BASEFONT,
	T_SCRIPT,
	T_COMMENT,
	T_NOBR,				// No line breaks					
	T_CLOCK,
	T_AUDIOSCOPE,
	T_UNKNOWN
};


// Attributes

typedef enum Attribute {
	A_BAD = 0,
	A_HREF,A_NAME,A_TITLE,A_REL,A_REV,A_URN,A_METHODS,								// Attributes for anchors
	A_ALIGN,A_ALT,A_ISMAP,A_USEMAP,A_SRC,A_WIDTH,A_HEIGHT,A_BORDER,A_HSPACE,A_VSPACE,	// Attributes for images
	A_ACTION,A_METHOD,A_ENCTYPE,													// Attributes for FORM
	A_TYPE,A_MAXLENGTH,A_SIZE,A_VALUE,A_CHECKED,									//	Attributes for INPUT
	A_MULTIPLE,A_EXCLUSIVE,A_SELECTED,
	A_COLS,A_ROWS,A_COLSPAN,A_ROWSPAN,												// Attributes for TABLE
	A_VALIGN,A_CELLSPACING,A_CELLPADDING,A_NOWRAP,
	A_NOSHADE,A_CLEAR,																// Attributes for hr,br
	A_BACKGROUND,A_LOGO,A_TEXT,A_LINK,A_VLINK,A_ALINK,										// Attributes for BODY
	A_BGCOLOR,
	A_COLOR,																		// Attributes for FONT
	A_EFFECT,		
	A_SHAPE, A_COORDS, A_NOHREF,													// Attributes for AREA
	A_HTTP_EQUIV,A_CONTENT,A_URL,													// Attributes for META
	A_ID,																			// Attributes for FN
	A_START,
	A_THING,
	A_EXECUTEURL, A_AUTOSUBMIT,														// WebTV Additions
	A_TRANSPARENCY,A_ANI,
	A_ABSWIDTH, A_ABSHEIGHT, A_CELLBORDER,											// WebTV Additions
	A_XSPEED,A_YSPEED,A_NOHTILEBG,A_NOVTILEBG,										// WebTV Additions
	A_NOSTATUS,A_NOOPTIONS,A_HIDEOPTIONS,A_TRANSITION,								// WebTV Additions
	A_SKIPBACK,A_CLEARBACK,A_STAYTIME,												// WebTV Additions
	A_TIME,A_DATE,																	// WebTV Additions - clock
	A_LOOP,																			// Internet Explorer 2.0
	A_LAYER,																		// WebTV Additions
	A_ANISTARTX,																	// WebTV Additions
	A_ANISTARTY,																	// WebTV Additions
	A_ONCLICK,
	A_NOCOLOR,
	A_USEFORM,
	A_HELP,
	A_CREDITS,
	A_CURSOR,
	A_AUTOCAPS,
	A_NUMBERS,
	A_FONT,
	A_NOSOFTBREAKS,
	A_GROWABLE,
	A_BORDERIMAGE,
	A_NOCURSOR,
	A_NOHIGHLIGHT,
	A_ALLCAPS,
	A_SHOWKEYBOARD,
	A_INVERTBORDER,
	A_NOFILTER,
	A_LEFTCOLOR,
	A_RIGHTCOLOR,
	A_LEFTOFFSET,
	A_RIGHTOFFSET,
	A_GAIN,
	A_MAXLEVEL,
	A_UNKNOWN
} Attribute;

// Attribute values

typedef enum AttributeValue {
	AV_BAD = 0,
	AV_LEFT,AV_TOP,AV_RIGHT,AV_TEXTTOP,AV_MIDDLE,AV_ABSMIDDLE,
	AV_BASELINE,AV_BOTTOM,AV_ABSBOTTOM,AV_CENTER,AV_ALL,
	AV_BLEEDLEFT,AV_BLEEDRIGHT,
	AV_BUTTON,AV_CHECKBOX,AV_HIDDEN,AV_IMAGE,AV_PASSWORD,AV_RADIO,AV_RESET,AV_SUBMIT,AV_TEXT,
	AV_GET,AV_POST,AV_RECT,AV_POLYGON,AV_POLY,AV_CIRCLE,AV_DEFAULT,
	AV_NONE,AV_RELIEF,AV_EMBOSS,AV_OUTLINE,
	AV_ONLEAVE,
	AV_FIXED, AV_PROPORTIONAL,
	AV_BLACKFADE, AV_CROSSFADE,
	AV_WIPELEFT, AV_WIPERIGHT, AV_WIPEUP, AV_WIPEDOWN, 
	AV_WIPELEFTTOP, AV_WIPERIGHTTOP, AV_WIPELEFTBOTTOM, AV_WIPERIGHTBOTTOM,
	AV_SLIDELEFT, AV_SLIDERIGHT, AV_SLIDEUP, AV_SLIDEDOWN,
	AV_PUSHLEFT, AV_PUSHRIGHT, AV_PUSHUP, AV_PUSHDOWN,
	AV_ZOOMINOUT, AV_ZOOMIN, AV_ZOOMOUT,
	AV_ZOOMINOUTH, AV_ZOOMINH, AV_ZOOMOUTH,
	AV_ZOOMINOUTV, AV_ZOOMINV, AV_ZOOMOUTV,
	AV_ON, AV_OFF,
	AV_UNKNOWN
} AttributeValue;

// =============================================================================

#endif /* __PARSER_H__ */
