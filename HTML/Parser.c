// ===========================================================================
//	Parser.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __CLOCK_H__
#include "Clock.h"
#endif
#ifndef __DOCUMENTBUILDER_H__
#include "DocumentBuilder.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __SYSTEM_H__
#include "System.h"
#endif
#ifndef __PAGEVIEWER__H
#include "PageViewer.h"
#endif
#ifndef __PARSER_H__
#include "Parser.h"
#endif
#ifndef __STATUS_H__
#include "Status.h"
#endif
#ifndef __STREAM_H__
#include "Stream.h"
#endif
#ifndef __TELLYIO_H__
#include "TellyIO.h"
#endif
#ifndef __KEYBOARD_H__
#include "Keyboard.h"
#endif

#ifdef SIMULATOR
#ifndef __SIMULATOR_H__
#include "Simulator.h"
#endif
#endif

#ifndef SIMULATOR
#ifndef __BOXABSOLUTEGLOBALS_H__
#include "BoxAbsoluteGlobals.h"
#endif
#endif

// ===========================================================================
// "Hypertext Markup Language - 2.0" - Working draft, August 8, 1995
// Netscape Extensions
// IExplorer Extensions
// HTML 3.0 - March 28, 1995.
// WebTV Extensions.

// #define SINGLE_BYTE_PARSE 1
#ifdef SINGLE_BYTE_PARSE
const long kMaxParseChunk = 1;
#else
const long kMaxParseChunk = 512;
#endif // SINGLE_BYTE_PARSE

// NOTE: Dave, should we alphabetize these? It might be easier to find
// tags and attributes. It would also be useful when adding new ones to
// see immediately whether a name is already used.
//
// --Chris

static const char* gHTMLTags[] = {
	"BAD TAG",
	"HTML",
	"HEAD",
	"TITLE",
	"BASE",
	"LINK",
	"META",
	
	"BGSOUND",
	"BODY",
	"H1","H2","H3",
	"H4","H5","H6",
	"P",				// Block Structuring				5.5
	"PRE",
	"XMP","LISTING",
	"ADDRESS",
	"BLOCKQUOTE",
	"UL","LI",			// List Elements					5.6
	"OL",
	"DIR",
	"MENU",
	"DL","DT","DD",
	"CITE",				// Phrase Markup					5.7
	"CODE",
	"EM",
	"KBD",
	"SAMP",
	"STRONG",
	"VAR",
	"B",				// Typographic Elements			5.7.2
	"I",
	"TT",
	"U",
	"SUB",
	"SUP",
	"A",
	"BR",
	"HR",
	"IMG",
	"IMAGE",
	"FORM",
	"INPUT",
	"SELECT",
	"OPTION",
	"TEXTAREA",
	"TABLE",			// Tables							HTML 3.0
	"CAPTION",
	"DFN",
	"TH",
	"TD",
	"TR",
	"EMBED",
	"FN",
	"DIV",
	
	"MAP",				// Client side image maps			Netscape extensions
	"AREA",
	"FRAME",
	"NOFRAME",
	"NOFRAMES",
	
	"SHADOW",			// Wacky Artemis HTML
	"DISPLAY",
	"SIDEBAR",
	
	"CENTER",			// Wacky Netscape HTML
	"FONT",
	"BIG",
	"SMALL",
	"BASEFONT",
	"SCRIPT",
	"!--",
	"NOBR",
	"CLOCK",
	"AUDIOSCOPE",
	"UNKNOWN",
	0
};

static const char* gHTMLAttributes[] = {
	"BAD ATTRIBUTE",
	"HREF","NAME","TITLE","REL","REV","URN","METHODS",								// Attributes for anchors
	"ALIGN","ALT","ISMAP","USEMAP","SRC","WIDTH","HEIGHT","BORDER","HSPACE","VSPACE",	// Attributes for IMAGE
	"ACTION","METHOD","ENCTYPE",													// Attributes for FORM
	"TYPE","MAXLENGTH","SIZE","VALUE","CHECKED",									// Attributes for INPUT
	"MULTIPLE","EXCLUSIVE","SELECTED",
	"COLS","ROWS","COLSPAN","ROWSPAN",												// Attributes for TABLE
	"VALIGN","CELLSPACING","CELLPADDING","NOWRAP",
	"NOSHADE","CLEAR",
	"BACKGROUND","LOGO","TEXT","LINK","VLINK","ALINK",										// Attributes for BODY
	"BGCOLOR",
	"COLOR",																		// Attributes for FONT
	"EFFECT",
	"SHAPE","COORDS","NOHREF",														// Attributes for AREA
	"HTTP-EQUIV","CONTENT","URL",													// Attributes for META
	"ID",																			// Attribute for FN
	"START",
	"THING",
	"EXECUTEURL",																	// WebTV additions
	"AUTOSUBMIT",
	"TRANSPARENCY",
	"ANI",
	"ABSWIDTH", "ABSHEIGHT",
	"CELLBORDER",
	"XSPEED",
	"YSPEED",
	"NOHTILEBG",
	"NOVTILEBG",
	"NOSTATUS",
	"NOOPTIONS",
	"HIDEOPTIONS",
	"TRANSITION",
	"SKIPBACK",
	"CLEARBACK",
	"STAYTIME",
	"TIME",
	"DATE",
	"LOOP",
	"LAYER",
	"ANISTARTX",
	"ANISTARTY",
	"ONCLICK",
	"NOCOLOR",
	"USEFORM",
	"HELP",
	"CREDITS",
	"CURSOR",
	"AUTOCAPS",
	"NUMBERS",
	"FONT",
	"NOSOFTBREAKS",
	"GROWABLE",
	"BORDERIMAGE",
	"NOCURSOR",
	"NOHIGHLIGHT",
	"ALLCAPS",
	"SHOWKEYBOARD",
	"INVERTBORDER",
	"NOFILTER",
	"LEFTCOLOR",
	"RIGHTCOLOR",
	"LEFTOFFSET",
	"RIGHTOFFSET",
	"GAIN",
	"MAXLEVEL",
	"UNKNOWN",
	0																				
};

// Numeric attributes include valueless attributes. Should really be NonStringAttributes.
static const short gNumericAttributes[] = {
	A_ISMAP,A_WIDTH,A_HEIGHT,A_BORDER,A_HSPACE,A_VSPACE,
	A_MAXLENGTH,A_SIZE,A_CHECKED,A_MULTIPLE,A_EXCLUSIVE,A_SELECTED,
	A_COLS,A_ROWS,A_COLSPAN,A_ROWSPAN,
	A_CELLSPACING,A_CELLPADDING,A_NOWRAP,A_NOSHADE,
	A_TEXT,A_LINK,A_VLINK,A_ALINK,
	A_BGCOLOR,
	A_START,
	A_SELECTED,
	A_NOHIGHLIGHT,
	A_TRANSPARENCY,
	A_ABSWIDTH, A_ABSHEIGHT,
	A_CELLBORDER,
	A_XSPEED,
	A_YSPEED,
	A_NOHTILEBG,
	A_NOVTILEBG,
	A_NOOPTIONS,
	A_HIDEOPTIONS,
	A_NOSTATUS,
	A_STAYTIME,
	A_SKIPBACK,
	A_CLEARBACK,
	A_LAYER,
	A_ANISTARTX,
	A_ANISTARTY,
	A_CURSOR,
	A_INVERTBORDER,
	A_NOFILTER,
	A_LEFTCOLOR,
	A_RIGHTCOLOR,
	A_LEFTOFFSET,
	A_RIGHTOFFSET,
	A_GAIN,
	A_MAXLEVEL,
	0
};

static const char* gHTMLAttributeValues[] = {
	"BAD ATTRIBUTE VALUE",
	"LEFT","TOP","RIGHT","TEXTTOP","MIDDLE","ABSMIDDLE",
	"BASELINE","BOTTOM","ABSBOTTOM","CENTER","ALL",
	"BLEEDLEFT","BLEEDRIGHT",
	"BUTTON", "CHECKBOX","HIDDEN","IMAGE","PASSWORD","RADIO","RESET","SUBMIT","TEXT",
	"GET","POST",
	"RECT","POLYGON","POLY","CIRCLE","DEFAULT",
	"NONE","RELIEF","EMBOSS","OUTLINE",
	"ONLEAVE",
	"FIXED", "PROPORTIONAL",
	"BLACKFADE", "CROSSFADE",
	"WIPELEFT", "WIPERIGHT", "WIPEUP", "WIPEDOWN", 
	"WIPELEFTTOP", "WIPERIGHTTOP", "WIPELEFTBOTTOM", "WIPERIGHTBOTTOM",
	"SLIDELEFT", "SLIDERIGHT", "SLIDEUP", "SLIDEDOWN",
	"PUSHLEFT", "PUSHRIGHT", "PUSHUP", "PUSHDOWN",
	"ZOOMINOUT", "ZOOMIN", "ZOOMOUT",
	"ZOOMINOUTH", "ZOOMINH", "ZOOMOUTH",
	"ZOOMINOUTV", "ZOOMINV", "ZOOMOUTV",
	"ON", "OFF",
	"UNKNOWN",
	0
};

// ISO8859-1 charset

struct StringToCharacterMapping {
	short	fCharacterNumber;
	const char	*fMatchString;
};
typedef struct StringToCharacterMapping StringToCharacterMapping;

struct StringToFunctionMapping {
	StringFunction	fStringFunction;
	const char*			fMatchString;
};
typedef struct StringToFunctionMapping StringToFunctionMapping;


// 160 to 255


#ifdef	OLD_FONTS		// Backward compatible for old format ( Macintosh encoding) fonts

static const StringToCharacterMapping gISO8859Chars[] = {
	{' ', "nbsp"	},  {'┴', "iexcl"	},	// 160
	{'в', "cent"	},  {'г', "pound"	},
	{'█', "curren"	},  {'┤', "yen"		},
	{'|', "brkbar"	},  {'д', "sect"	},

	{'м', "uml"		},  {'й', "copy"	},	// 168
	{'╗', "ordf"	},  {'╟', "laquo"	},
	{'┬', "not"		},  {'╨', "shy"		},
	{'и', "reg"		},  {'°', "hibar"	},

	{'б', "deg"		},  {'▒', "plusmn"	},	// 176
	{'2', "sup2"	},  {'3', "sup3"	},
	{'л', "acute"	},  {'╡', "micro"	},
	{'ж', "para"	},  {'с', "middot"	},

	{'№', "cedil"	},  {'1', "sup1"	},	// 184
	{'╝', "ordm"	},  {'╚', "raquo"	}, 
	{'q', "frac14"	},  {'h', "frac12"	},
	{'t', "frac34"	},  {'└', "iquest"	},

	{'╦', "Agrave"	}, 	{'ч', "Aacute"	}, 	// 192
	{'х', "Acirc"	}, 	{'╠', "Atilde"	}, 
	{'А', "Auml"	}, 	{'Б', "Aring"	}, 
	{'о', "AElig"	}, 	{'В', "Ccedil"	}, 
	
	{'щ', "Egrave"	}, 	{'Г', "Eacute"	}, 	// 200
	{'ц', "Ecirc"	}, 	{'ш', "Euml"	}, 
	{'э', "Igrave"	}, 	{'ъ', "Iacute"	}, 
	{'ы', "Icirc"	}, 	{'ь', "Iuml"	}, 
	
	{'▌', "ETH"		}, 	{'Д', "Ntilde"	}, 	// 208
	{'ё', "Ograve"	}, 	{'ю', "Oacute"	}, 
	{'я', "Ocirc"	}, 	{'═', "Otilde"	}, 
	{'Е', "Ouml"	}, 	{'╫', "times"	}, 
	
	{'п', "Oslash"	}, 	{'Ї', "Ugrave"	}, 	// 216
	{'Є', "Uacute"	}, 	{'є', "Ucirc"	}, 
	{'Ж', "Uuml"	}, 	{'Y', "Yacute"	}, 	// еееее Not really the correct one
	{'▐', "THORN"	}, 	{'з', "szlig"	}, 
	
	{'И', "agrave"	}, 	{'З', "aacute"	}, 	// 224
	{'Й', "acirc"	}, 	{'Л', "atilde"	}, 
	{'К', "auml"	}, 	{'М', "aring"	}, 
	{'╛', "aelig"	}, 	{'Н', "ccedil"	}, 
	
	{'П', "egrave"	}, 	{'О', "eacute"	}, 	// 232
	{'Р', "ecirc"	}, 	{'С', "euml"	}, 
	{'У', "igrave"	}, 	{'Т', "iacute"	}, 
	{'Ф', "icirc"	}, 	{'Х', "iuml"	}, 
	
	{'▌', "eth"		}, 	{'Ц', "ntilde"	}, 	// 240
	{'Ш', "ograve"	}, 	{'Ч', "oacute"	}, 
	{'Щ', "ocirc"	}, 	{'Ы', "otilde"	}, 
	{'Ъ', "ouml"	}, 	{'╓', "divide"	}, 
	
	{'┐', "oslash"	}, 	{'Э', "ugrave"	}, 	// 248
	{'Ь', "uacute"	}, 	{'Ю', "ucirc"	}, 
	{'Я', "uuml"	}, 	{'р', "yacute"	}, 
	{'▀', "thorn"	}, 	{'╪', "yuml"	},
	{ 0, nil },		// *** Last entry
};
static StringToCharacterMapping gCommonChars[] = {
	{'"', "quot"	}, 	{'&', "amp"		}, 
	{'<', "lt"		}, 	{'>', "gt"		}, 
	{'й', "copy"	}, 	{'и', "reg"		}, 
	{' ', "nbsp"	},
	{ 0, nil },		// *** Last entry
};



#else


static const StringToCharacterMapping gISO8859Chars[] = {
	{160, "nbsp"	},  {161, "iexcl"	},	// 160
	{162, "cent"	},  {163, "pound"	},
	{164, "curren"	},  {165, "yen"		},
	{166, "brkbar"	},  {167, "sect"	},

	{168, "uml"		},  {169, "copy"	},	// 168
	{170, "ordf"	},  {171, "laquo"	},
	{172, "not"		},  {173, "shy"		},
	{174, "reg"		},  {175, "hibar"	},

	{176, "deg"		},  {177, "plusmn"	},	// 176
	{178, "sup2"	},  {179, "sup3"	},
	{180, "acute"	},  {181, "micro"	},
	{182, "para"	},  {183, "middot"	},

	{184, "cedil"	},  {185, "sup1"	},	// 184
	{186, "ordm"	},  {187, "raquo"	}, 
	{188, "frac14"	},  {189, "frac12"	},
	{190, "frac34"	},  {191, "iquest"	},

	{192, "Agrave"	}, 	{193, "Aacute"	}, 	// 192
	{194, "Acirc"	}, 	{195, "Atilde"	}, 
	{196, "Auml"	}, 	{197, "Aring"	}, 
	{198, "AElig"	}, 	{199, "Ccedil"	}, 
	
	{200, "Egrave"	}, 	{201, "Eacute"	}, 	// 200
	{202, "Ecirc"	}, 	{203, "Euml"	}, 
	{204, "Igrave"	}, 	{205, "Iacute"	}, 
	{206, "Icirc"	}, 	{207, "Iuml"	}, 
	
	{208, "ETH"		}, 	{209, "Ntilde"	}, 	// 208
	{210, "Ograve"	}, 	{211, "Oacute"	}, 
	{212, "Ocirc"	}, 	{213, "Otilde"	}, 
	{214, "Ouml"	}, 	{215, "times"	}, 
	
	{216, "Oslash"	}, 	{217, "Ugrave"	}, 	// 216
	{218, "Uacute"	}, 	{219, "Ucirc"	}, 
	{220, "Uuml"	}, 	{221, "Yacute"	}, 	// еееее Not really the correct one
	{222, "THORN"	}, 	{223, "szlig"	}, 
	
	{224, "agrave"	}, 	{225, "aacute"	}, 	// 224
	{226, "acirc"	}, 	{227, "atilde"	}, 
	{228, "auml"	}, 	{229, "aring"	}, 
	{230, "aelig"	}, 	{231, "ccedil"	}, 
	
	{232, "egrave"	}, 	{233, "eacute"	}, 	// 232
	{234, "ecirc"	}, 	{235, "euml"	}, 
	{236, "igrave"	}, 	{237, "iacute"	}, 
	{238, "icirc"	}, 	{239, "iuml"	}, 
	
	{240, "eth"		}, 	{241, "ntilde"	}, 	// 240
	{242, "ograve"	}, 	{243, "oacute"	}, 
	{244, "ocirc"	}, 	{245, "otilde"	}, 
	{246, "ouml"	}, 	{247, "divide"	}, 
	
	{248, "oslash"	}, 	{249, "ugrave"	}, 	// 248
	{250, "uacute"	}, 	{251, "ucirc"	}, 
	{252, "uuml"	}, 	{253, "yacute"	}, 
	{254, "thorn"	}, 	{255, "yuml"	},

	// alternate names ( ref: http://ppewww.ph.gla.ac.uk/~flavell/iso8859/iso8859-pointers.html )

	{166, "brvbar"	},
	{168, "die"		},
	{175, "macr"	},
	{175, "macron"	},
	{176, "degree"	},
	{184, "Cedilla"	}, 
	{208, "Dstrok"  },
	
	{ 0, nil },		// *** Last entry
};

static const StringToCharacterMapping gCommonChars[] = {
	{'"', "quot"	}, 	{'&', "amp"		}, 
	{'<', "lt"		}, 	{'>', "gt"		}, 
	{169, "copy"	}, 	{174, "reg"		}, 
	{' ', "nbsp"	},
	{ 0, nil },		// *** Last entry
};


#endif

//	Maps 8859-1 to Mac character set

static const char*
DateFunction()
{
	IsError(!gClock);
	return gClock->GetDateString();
}

static const char*
VersionFunction()
{
	IsError(!gSystem);
	return gSystem->GetVersion();
}

static const char* 
CPUSpeedFunction()
{
	static char	buffer[32];
	const char*	cpuTypeString;
	ulong	cpuSpeed;
	
#ifdef FOR_MAC
	long	response;
	
	Gestalt(gestaltNativeCPUtype, &response);
	switch (response)
	{
		case gestaltCPU601:
			cpuTypeString = "PowerPC 601";
			break;
		case gestaltCPU603:
			cpuTypeString = "PowerPC 603";
			break;
		case gestaltCPU604:
			cpuTypeString = "PowerPC 604";
			break;
		default:
			cpuTypeString = "PowerPC";
			break;
	}
	cpuSpeed = 120*1000*1000;
	
#else
	cpuTypeString = "MIPS 4640";
	cpuSpeed = READ_AG(agCPUSpeed);
#endif

	if ((cpuSpeed % 1000000) == 0)
		snprintf(buffer, sizeof(buffer), "%s (%d MHz)", cpuTypeString, cpuSpeed/1000000);
	else
		snprintf(buffer, sizeof(buffer), "%s (%d.%03d MHz)", cpuTypeString,
		       cpuSpeed/1000000,
		       (cpuSpeed % 1000000)/1000);
	       
	return buffer;
}

static const char* 
FontSizesFunction()
{
	const char*	sizeNames[] = { "small", "medium", "large"};
	
	return sizeNames[gSystem->GetFontSizes() + 1];
}

static const char* 
KeyboardNameFunction()
{
	const char*	keyboardNames[] = { "alphabetical", "standard" };
	long	keyboardNumber = gKeyboard->GetKeyboardType() - 1;
	
	if (keyboardNumber > 1)
		keyboardNumber = 1;
	return keyboardNames[keyboardNumber];
}

static const char* 
ModemSpeedFunction()
{
	static char			speed[32];
	ConnectionStats*	stats = GetConnectionStats();

	if (gSystem->GetUsePhone())
		snprintf(speed, sizeof(speed), "%ld bps (%scompressed)", stats->dcerate,
			(stats->compression != 0) ? "" : "un");
#ifdef SIMULATOR
	else
		{
		long limitNetSpeed = gSimulator->GetLimitNetSpeed();

		if (limitNetSpeed == 0)
			snprintf(speed, sizeof(speed), "10 mb/s (ethernet)");
		else
			snprintf(speed, sizeof(speed), "%ld bps", limitNetSpeed);
		}
#endif
	return speed;
}

static const char* 
PulseDialingFunction()			/* &pulse */
{
	return GetPhoneSettings()->usePulseDialing ? "on" : "off";
}

static const char* 
WaitForDialToneFunction()		/* &dtone */
{
	return GetPhoneSettings()->waitForTone ? "on" : "off";
}

static const char* 
AudibleDialingFunction()		/* &audio */
{
	return GetPhoneSettings()->audibleDialing ? "on" : "off";
}

static const char* 
DisableCallWaitingFunction()	/* &nowait */
{
	return GetPhoneSettings()->disableCallWaiting ? "on" : "off";
}

static const char* 
DisableCallWaitingString()		/* &wstr */
{
	return GetPhoneSettings()->callWaitingPrefix;
}

static const char* 
DialOutsideLineFunction()		/* &dout */
{
	return GetPhoneSettings()->dialOutsideLine ? "on" : "off";
}

static const char* 
DialOutsideLineString()			/* &outstr */
{
	return GetPhoneSettings()->dialOutsidePrefix;
}

static const char* 
AccessNumberString()			/* &anum */
{
	return GetPhoneSettings()->accessNumber;
}

static StringToFunctionMapping gFunctionMappings[] = {
	{ DateFunction, "date"	},
	{ VersionFunction, "vers"	},
	{ ModemSpeedFunction, "rate"	},
	{ PageURLFunction, "url"	},
	{ PageTitleFunction, "title"	},
	{ PageThumbnailURLFunction, "thumb"	},
	{ CPUSpeedFunction, "cpu"	},
	{ FontSizesFunction, "fsize"	},
	{ LastModifiedFunction, "mod"	},
	{ KeyboardNameFunction, "kbd"	},
	{ PulseDialingFunction, "pulse" },
	{ WaitForDialToneFunction, "dtone" },
	{ AudibleDialingFunction, "audio" },
	{ DisableCallWaitingFunction, "nowait" },
	{ DisableCallWaitingString, "wstr" },
	{ DialOutsideLineFunction, "dout" },
	{ DialOutsideLineString, "outstr" },
	{ AccessNumberString, "anum" },
	{ LastFindStringFunction, "find" },
	{ LastExecuteURLFunction, "exurl" },

	{ 0, nil },		// *** Last entry
};

// " < > & и й

#ifdef FOR_MAC
const char* kUnknownString = "<unknown>";
#endif

// ===========================================================================

static Boolean HasNumericValue(short attributeID)
{
	short i = 0;
	while (gNumericAttributes[i])
		if (attributeID == gNumericAttributes[i++])
			return true;

	return false;
}

static short LookupToken(char* str, const char** tokenList)
{
	if (str == nil)
		return 0;
		
	const char* s;
	
	// Lookup a Tag, Attribute or Attribute value
	
	short i = 0;
		
	UpperCase(str);
	while ((s = tokenList[++i]) != nil)
	{		
		if (!strcmp(str, s))
			return i;
	}

	Message(("Can't lookup %s", str));
	return 0;
}

// ============================================================================

Parser::Parser()
{
	// Help assure our HTML tables are in sync!!
	if ( IsError(sizeof(gHTMLTags)/sizeof(char*) != T_UNKNOWN+2) )
		;
	if ( IsError(sizeof(gHTMLAttributes)/sizeof(char*) != A_UNKNOWN+2) )
		;
	if ( IsError(sizeof(gHTMLAttributeValues)/sizeof(char*) != AV_UNKNOWN+2) )
		;
}

Parser::~Parser()
{
}

void
Parser::Parse(DataStream*)
{
}

void Parser::SetBuilder(Builder* builder)
{
	fBuilder = builder;
}

// ============================================================================

HTMLParser::HTMLParser()
{
	fTagList = new(TagList);
}

HTMLParser::~HTMLParser()
{
	delete(fTagList);
}

void HTMLParser::BuildTagList(short tagID)
{
	// Digest tag attributes
	
	Attribute attributeID;
	long value;
	char* attributeStr;
	char* valueStr;
	Boolean isPercentage;
	char* s = fTag;
	
	while((*s) && (!isspace(*s)))
		s++;

	while ((s = NextAttributeStrings(s, &attributeStr, &valueStr)) != nil) {
		attributeID = (Attribute)LookupToken(attributeStr, gHTMLAttributes);
		
#if defined FOR_MAC && defined LOG_TAGS_AND_ATTRIBUTES
		gAttributeLog.AddTagAndAttribute(gHTMLTags[tagID], attributeStr, fTag,
			(fResource == nil) ? kUnknownString : fResource->GetURL());
#endif
	
		// If value is numeric, convert letter 'o' to zero, and letter 'i' to one.
		if (HasNumericValue(attributeID) && valueStr != nil) {
			for (long i = 0; valueStr[i] != '\0'; i++) {
				switch (valueStr[i]) {
					case 'O':
						valueStr[i] = '0';
						break;
					case 'l':
						valueStr[i] = '1';
						break;
					default:
						break;
				}
			}
		}	
			
		switch (attributeID) 
		{			
			case A_ALIGN:				// Have values like "TOP", "LEFT" etc
			case A_VALIGN:
			case A_CLEAR:
			case A_METHOD:				// AV_GET, AV_POST
			case A_SHAPE:				// AV_RECT, AV_POLYGON, AV_CIRCLE
			case A_EFFECT:
			case A_FONT:				// AV_FIXED, AV_PROPORTIONAL
			case A_AUTOSUBMIT:			// AV_ONLEAVE
			case A_CHECKED:				// AV_ON, AV_OFF
			case A_TRANSITION:
				value = LookupToken(valueStr, gHTMLAttributeValues);
				fTagList->Add(attributeID, value, 0);
				break;
				
			case A_TYPE:				// AV_CHECKBOX, AV_HIDDEN, AV_IMAGE, AV_RADIO, AV_RESET, AV_SUBMIT, AV_TEXT
				switch (tagID) 
				{					
					case T_DL:			// Lists
					case T_OL:
					case T_UL:
					case T_LI:
						fTagList->Add(attributeID, valueStr);
						break;
					default:
						value = LookupToken(valueStr, gHTMLAttributeValues);
						fTagList->Add(attributeID, value, 0);
				}
				break;
				
			case A_BGCOLOR:
			case A_COLOR:
			case A_TEXT:
			case A_LINK:
			case A_ALINK:
			case A_VLINK:
				// Colors are treated special. Values are interpreted as hex always.
				value = -1;
				
				if (valueStr != nil)
				{
					long	R, G, B;
					
					// We need to read the R G B values individually to handle the form BGCOLOR="rr gg bb"
					if (sscanf(valueStr, "#%2lx%2lx%2lx", &R, &G, &B) != 3 &&
						sscanf(valueStr, "%2lx%2lx%2lx", &R, &G, &B) != 3)
						ImportantMessage(("HTMLParser::BuildTagList: cannot parse %s=%s", gHTMLAttributes[attributeID], valueStr));
					else
						value = (R << 16) + (G << 8) + B;
				}
				
				fTagList->Add(attributeID, value, false);
				break;
				
			case A_BAD:
#if defined(FOR_MAC) && defined(LOG_BAD_TAGS_AND_ATTRIBUTES)
				gBadAttributeLog.AddTagAndAttribute(gHTMLTags[tagID], attributeStr, fTag, 
					(fResource == nil) ? kUnknownString : fResource->GetURL());
#endif // defined(FOR_MAC) && defined(LOG_BAD_TAGS_AND_ATTRIBUTES)
				ImportantMessage(("###%s", fTag));
				ImportantMessage(("##BAD ATTRIBUTE>%s=%s", attributeStr,
								  valueStr != nil ? valueStr : ""));
				break;

			default:
				// Some numeric tags can also be used without a value, such as
				// BORDER, to indicate that the default value should be used. We set
				// the value to -1 to indicate the value was not present. If the "=" 
				// is present we set the default to 0.
				value = -1;
				
				if (HasNumericValue(attributeID)) 
				{
					if (valueStr != nil) {
						value = 0;
						if (sscanf(valueStr, "#%lx", &value) != 1 &&
							sscanf(valueStr, "%ld", &value) != 1)
							ImportantMessage(("HTMLParser::BuildTagList: cannot parse %s=%s", gHTMLAttributes[attributeID], valueStr));
					
						isPercentage = strchr(valueStr, '%') || strchr("+-", *valueStr);
					}
					else
						isPercentage = false;
					fTagList->Add(attributeID, value, isPercentage);
				} else
					fTagList->Add(attributeID, valueStr);
				break;
		}
	}
}

void HTMLParser::CheckTag(short tagID)
{
	if (tagID < 0)
		tagID = -tagID;

	// Set fStripSpaces to remove extra spaces before body text ...

	switch (tagID) 
	{
		case T_OPTION:
			fInOption = true;
		case T_HTML:
		case T_HEAD:
		case T_TITLE:
		case T_BASE:
		case T_BODY:
		case T_BGSOUND:
		case T_H1:		// Headings
		case T_H2:
		case T_H3:
		case T_H4:
		case T_H5:
		case T_H6:
		case T_DL:		// Glossary List
		case T_OL:		// Ordered List
		case T_UL:		// Unordered List
		case T_DIR:
		case T_MENU:
		case T_DT:		// Glossary List term
		case T_DD:		// Glossary List def
		case T_LI:		// List Item
		case T_P:
		case T_DIV:
		case T_BR:
		case T_HR:
		case T_TABLE:
		case T_TH:
		case T_TR:
		case T_TD:
		case T_FORM:
		case T_SELECT:
		case T_EMBED:
		case T_MAP:
		case T_AREA:
			fInBody = false;
			break;
			
		case T_IMG:
		case T_IMAGE:
		case T_PRE:
		case T_TEXTAREA:
			fInBody = true;
			break;
	}
}

Boolean HTMLParser::DispatchTag()
{
	// DispatchTag tokenizes a tag and sends it to the displayable builder

	fTag[fTagCount] = 0;
	short tagID = GetTagID();
	
	if (tagID == T_UNKNOWN)
		return false;

	CheckTag(tagID);	
	
	if (tagID > 0)
	{
		if (tagID == T_PRE || tagID == T_XMP || tagID == T_LISTING)
			fInPRE = true;
		if (tagID == T_TEXTAREA) {
			fInTextArea = true;
			fInPRE = true;
		}
		BuildTagList(tagID);	// Collect the attributes for this tag
		fBuilder->AddTag(tagID, fTagList);
		fTagList->RemoveAll();
	}
	else
	{
		if (-tagID == T_PRE || -tagID == T_XMP || -tagID == T_LISTING)
			fInPRE = false;
		if (-tagID == T_TEXTAREA) {
			fInTextArea = false;
			fInPRE = false;
		}
		fBuilder->AddTag(tagID, nil);
	}
	
	fTagCount = 0;
	return true;
}

void HTMLParser::DispatchText()
{
	// Write a chunk of text to the page
	
	if (fTextCount == 0)
		return;

	fText[fTextCount] = 0;
	fBuilder->AddText(fText, fTextCount);

	fTextCount = 0;
	fInOption = false;
}

short HTMLParser::GetTagID() const
{
	// Lookup tag name and determine ID
	
	short 	i, neg, tag;
	char	tagName[128];
	
	const char* tagStr = fTag;
	if ((neg = (*tagStr == '/')) != 0)
		tagStr++;
	
	i = 0;										// Just lookup name
	while (tagStr[i] && !isspace(tagStr[i])) 
	{		
		tagName[i] = tagStr[i];
		i++;
	}
	tagName[i] = 0;

	if ((tag = LookupToken(tagName, gHTMLTags)) != 0)
	{
#if defined FOR_MAC && defined LOG_TAGS_AND_ATTRIBUTES
		gKnownTagLog.Add(tagName, 
			(fResource == nil) ? kUnknownString : fResource->GetURL());
#endif
		return neg ? -tag : tag;
	}
#if defined(FOR_MAC) && defined(LOG_BAD_TAGS_AND_ATTRIBUTES)
	gUnknownTagLog.Add(tagName,
			(fResource == nil) ? kUnknownString : fResource->GetURL());
#endif // defined(FOR_MAC) && defined(LOG_BAD_TAGS_AND_ATTRIBUTES)
	
	ImportantMessage(("Parse::GetTagID: Don't know this tag: %s", tagName));
	return T_UNKNOWN;
}

const char* HTMLParser::InterpretEntityString(char* amp, long stringLength, short* terminator)
// convert the entity string to the resultant value
{
	const StringToCharacterMapping*	mapping;
	const StringToFunctionMapping*	functionMapping;
	
	// ignore the ampersand
	const char*					entity = amp + 1;
	stringLength--;
	
	*terminator = '\0';			// Assume no terminator to process.
	
	// If we have more than 6 characters, this cannot be a value & sequence. Return it as text.
	if (stringLength > 6)
		return amp;
			
	// Check for '#' numeric specifier.
	if (*entity == '#')
	{
		long	charNum = 0;
		
		*terminator = entity[stringLength-1];
		
		// Still getting digits.
		if (*terminator == '#' || isdigit(*terminator))
			return nil;
		
		// If we never received a digit, return the sequence as text.	
		if (sscanf(entity, "#%ld;", &charNum) <= 0) {
			*terminator = '\0';
			return amp;
		}
		
		if (charNum > 255)
			charNum = '?';
#ifdef	OLD_FONTS
		else if (charNum >= 160)
			charNum = gISO8859Chars[charNum-160].fCharacterNumber;
#endif
		amp[0] = charNum;
		amp[1] = '\0';
		return amp;
	}
	
	// If the current character not '#' and its a non-alpha character, this is not a valid sequence.
	// We strip the last character for individual processing, and return the rest as text.
	// MHK: digits are allowed or else &sup1, &sup2, &sup3 and &frac?? dontwork
	if (!isalnum(entity[stringLength - 1])) {
		*terminator = entity[stringLength - 1];
		amp[stringLength] = '\0';
		return amp;
	}

	// If we've gotten this far, we still have a valid ampersand sequence going. Look for matches with 
	// know sequences		
	// This is bound into character set considerations .... needs to be fixed.
	for (functionMapping = gFunctionMappings; functionMapping->fStringFunction != nil; functionMapping++)
		if (EqualStringN(entity, functionMapping->fMatchString, strlen(functionMapping->fMatchString)))
			return (*functionMapping->fStringFunction)();

	for (mapping = gCommonChars; mapping->fCharacterNumber != 0; mapping++)
		if (strncmp(entity, mapping->fMatchString, strlen(mapping->fMatchString)) == 0) {	// Common chars
			amp[0] = mapping->fCharacterNumber;
			amp[1] = '\0';
			return amp;
		}
	
	for (mapping = gISO8859Chars; mapping->fCharacterNumber != 0; mapping++)
		if (strncmp(entity, mapping->fMatchString, strlen(mapping->fMatchString)) == 0) {	// 160 to 255
			amp[0] = mapping->fCharacterNumber;
			amp[1] = '\0';
			return amp;
		}

	// We didn't find it, so give a case insensitive search a try.
	for (mapping = gCommonChars; mapping->fCharacterNumber != 0; mapping++)
		if (EqualStringN(entity, mapping->fMatchString, strlen(mapping->fMatchString))) {	// Common chars
			amp[0] = mapping->fCharacterNumber;
			amp[1] = '\0';
			return amp;
		}
	
	for (mapping = gISO8859Chars; mapping->fCharacterNumber != 0; mapping++)
		if (EqualStringN(entity, mapping->fMatchString, strlen(mapping->fMatchString))) {	// 160 to 255
			amp[0] = mapping->fCharacterNumber;
			amp[1] = '\0';
			return amp;
		}

	return nil;
}

char* HTMLParser::NextAttributeStrings(char* s, char** attributeStr, char** valueStr)
{
	// Break a tag string into pieces. The string is not copied, but rather attributeStr
	// and valueStr are pointed to the appriopriate points in the text, and '\0' characters
	// inserted in place of terminating characters.
	
	short c;
	Boolean singleQuote = false;
	Boolean doubleQuote = false;
	Boolean	openBrace = false;
	Boolean	openBrack = false;
	
	*attributeStr = nil;
	*valueStr = nil;
	
	while (isspace(c = *s)) s++;	// Strip leading spaces
	if (!c) return nil;				// End of tag
	
	*attributeStr = s;
	while ((c = *s++) != 0)
	{		
		if (isspace(c) || c == '=') 
		{			
			*(s-1) = '\0';					// End of attribute .. determine if there is a ' =value'
			
			while (isspace(c))				// Strip leading spaces before '='
				c = *s++;
			if (c != '=') return s-1;		// No '=' here
			while (isspace(c = *s)) s++;	// Strip leading spaces after '='
			
			*valueStr = s;			
			while ((c = *s++) != 0)
			{				
				switch (c) 
				{					
					case '\'':					// Strip quotes
						if (!doubleQuote) {
							*(s-1) = '\0';
							if (singleQuote) 
								return s;
							if (strlen(*valueStr) == 0)
								*valueStr = s;
							singleQuote = true;
						}
						break;
					case '"':
						if (!singleQuote) {
							*(s-1) = '\0';
							if (doubleQuote) 
								return s;
							if (strlen(*valueStr) == 0)
								*valueStr = s;
							doubleQuote = true;
						}
						break;
						
						// special handling for braces & brackets ( seen on net, works in Netscape )
						
					case ']':	
						if (openBrack)  
						{
							*(s-1) = '\0';
							return s;
						}
						break;
					case '[':
						if ( !singleQuote && !doubleQuote  && !openBrace )
						{
							if (strlen(*valueStr) == 0)
								*valueStr = s;
							openBrack = true;
						}
						break;
					case '}':	
						if (openBrace)  
						{
							*(s-1) = '\0';
							return s;
						}
						break;
					case '{':
						if ( !singleQuote && !doubleQuote && !openBrack )
						{
							if (strlen(*valueStr) == 0)
								*valueStr = s;
							openBrace = true;
						}
						break;

					case ';':		// Special handling for (CONTENT="5; URL="...").
						if (EqualString(*attributeStr, "CONTENT")) {
							*(s-1) = '\0';
							return s;
						}
						break;
					case ' ':
						if (!doubleQuote && !singleQuote && !openBrack && !openBrace ) 
						{							
							*(s-1) = '\0';
							return s;
						}
					default:
						break;
				}
			}
			return s-1;							// Leave s pointing at the null
		}
	}
	return s-1;
}


void HTMLParser::Parse(DataStream* stream)
{
	long pEnd = MAX(0, stream->GetPending() - kMaxParseChunk);
	short c;
	
	while (stream->GetPending() > pEnd)
	{
		// Next byte from stream	
		
		c = *stream->ReadNext(1);

		PostulateFinal(false);
		
		if ( gSystem->GetUseJapanese() ) {			// actually this should be on all the time, because if the user goes to a Japanese
														// page the parser may get confused and get stuck since it will misinterpret ascii codes
														// it's off for now because Dave doesn't like it because it has bugs
														
			// deal with JIS coded Japanese text
			
			PostulateFinal(false);
			// еее still need to check SJIS text, since if parser looks at second byte of a two-byte code
			// еее it may confuse it for a normal ascii code and break the parser
			
			if ( fSkipNext ) 
			{
				fSkipNext = false;
				continue;
			}
			if ( c == 0x1b ) 
			{
				fInEscape = true;
				continue;
			}
			
			if ( fInEscape ) 
			{
				fInEscape = false;
				if ( fInJapanese ) 
				{
					if ( c == '(' ) 
					{
						fJapaneseTextType = 0;
						// make sure the escape sequence terminating character gets skipped
						fSkipNext = true;		
						// switch back to single-byte character mode
						
						if ( !fInComment ) {
							const char *jt = fJapaneseText;
							long n = CleanJISText(&jt,fJapaneseTextCount,true);
							if ( fInTag ) 
							{
								if  ( n+fTagCount > kMaxTagLength) 
									n = kMaxTagLength - fTagCount;
								CopyMemory(jt,fTag+fTagCount,n);
								fTagCount += n;
							}
							else 
							{
								if  ( n+fTextCount > kMaxJapaneseTextLength)
									DispatchText();
								CopyMemory(jt,fText+fTextCount,n);
								fTextCount += n;
							}
						}
						fJapaneseTextCount = 0;
						fInJapanese = false;
						continue;
					} 
					else 
					{
						// escape embedded in 2-byte codes - leave in place
						fJapaneseText[fJapaneseTextCount++]	= 0x1b;
						fJapaneseText[fJapaneseTextCount++]	= c;
						continue;
					}
				}
				else 
				{
					if ( c == '$' || c == '&' ) 
					{
						
						DispatchText();
						fInJapanese = true;
						if ( fJapaneseTextType != '&' )
							fJapaneseTextType = c;
						fJapaneseTextCount = 0;
						fJapaneseText[fJapaneseTextCount++]	= 0x1b;
						fJapaneseText[fJapaneseTextCount++]	= c;
						continue;
					} else 
					{
						Message(("Unrecognized escape sequence: 0x%x",c));
						fJapaneseText[fJapaneseTextCount++] = 0x1b;
						fJapaneseText[fJapaneseTextCount++]	= c;
						continue;
					}
				}
			}
			if ( fInJapanese ) 
			{
				if ( IsError(fJapaneseTextCount >= kMaxJapaneseTextLength) )
					fJapaneseTextCount = 0;
				if ( fJapaneseTextCount == 2 )
				{
					switch( c ) 
					{
					case '@':			// JIS-C 6226-1978 or JIS-X 028-1990 (<esc>&@)
						if ( fJapaneseTextType == '$' )	
							fJapaneseTextType = c;
						break;
					case 'B':			// JIS-X 0208-1983
						fJapaneseTextType = c;
						break;
					case '(':			// ) JIS-X 0212-1990
						TrivialMessage(("JIS X 0212-1990 character set not supported"));
						fJapaneseTextType = c;
						break;
					default:
						Message(("Unrecognized escape sequence: 0x%x 0x%x",'$',c));
						break;
					}
				}
				fJapaneseText[fJapaneseTextCount++]	= c;
				if ( fJapaneseTextCount > kMaxJapaneseTextLength-8 ) 
				{
					const char *escSeq = kEmptyString;
					switch ( fJapaneseTextType ) {
					case '@':
						escSeq = "\033$@";
						break;
					case 'B':
						escSeq = "\033$B";
						break;
					case '&':
						escSeq = "\033&@\033$B";
						break;
					case '(':					// )
						escSeq = "\033$(D";		// )
						break;
					default:
						Trespass();
					}
					// if we are not in between characters flush what we have
					if ( ((fJapaneseTextCount - strlen(escSeq)) & 1) == 0 ) 
					{
						if ( !fInComment )
						{
							const char *jt = fJapaneseText;
								long n = CleanJISText(&jt,fJapaneseTextCount,true);
							if ( fInTag ) 
							{
								if  ( n+fTagCount > kMaxTagLength) 
									n = kMaxTagLength - fTagCount;
								CopyMemory(jt,fTag+fTagCount,n);
								fTagCount += n;
							}
							else
							{
								if ( (n + fTextCount) > kMaxJapaneseTextLength ) 
									DispatchText();
								CopyMemory(jt,fText+fTextCount,n);
								fTextCount += n;
							}
						}
						fJapaneseTextCount = strlen(escSeq);
						CopyMemory(escSeq,fJapaneseText,fJapaneseTextCount);
					}
				}
				continue;
			}
		}

		// 	Skip comments
		
		if (fInComment)	
		{	
			// Attempt to match Netscape's bizarre comment handling. If comment begins with
			// "<!" we only require ">" to end. If comment begins with "<!--" we require
			// "-->" to end.
			switch (c)
			{
				case '-':
					if (fCommentBeginCount >= 0 && fCommentBeginCount < 2)
						fCommentBeginCount++;
					else if (fCommentBeginCount >= 2 && fCommentEndCount < 2)
						fCommentEndCount++;
					break;
				case '>':
					if (fCommentBeginCount < 2 || fCommentEndCount >= 1)
						fInComment = false;
					break;
				case '!':
					if (fCommentBeginCount < 2 || fCommentEndCount >= 2)
						break;
				default:
					if (fCommentBeginCount < 2)
						fCommentBeginCount = -1;
					fCommentEndCount = 0;
					break;
			}
			continue;
		}
	
		// Ignore semicolon left over from '&'
		if (fIgnoreSemiColon)
		{
			fIgnoreSemiColon = false;
			if (c == ';') 
				continue;
		}
		
		// If the last character was a space, strip additional spaces
		if (fStripSpaces && isspace(c))
			 continue;
		fStripSpaces = false;
		
		// Strip white space if not preformatted
		if (!fInPRE || fInTag)
		{
			if (isspace(c))	
			{	
				fStripSpaces = true;			
				c = ' ';
			}
		}
		else
		{	// Tab to next 8 space alignment on tab in PRE.	
			if (c == '	')
			{
				// We use 7 because the last space is added below in default:
				for  (short tabCount = (7 - (fTextCount % 8)); tabCount > 0; tabCount--)
					fText[fTextCount++] = ' ';
				c = ' ';
			}
		}

		if (fInAmpersand)
		{
			// Attempt to interpret string with each character, because ';' may be missing.
			fAmpersand[fAmpersandCount++] = c;					
			fAmpersand[fAmpersandCount] = '\0';
			
			const char*		ampString = InterpretEntityString(fAmpersand, fAmpersandCount, &c);
			if (ampString != nil)
			{
				if (fInTag)
				{
					strcpy(fTag + fTagCount, ampString);
					fTagCount += strlen(ampString);
				}
				else
				{
					strcpy(fText + fTextCount, ampString);
					fTextCount += strlen(ampString);
					fInBody = true;
				}
				fInAmpersand = false;
				
				// No terminating character to process. Need to ignore the next semicolon if we get it.
				if (c == '\0')
					fIgnoreSemiColon = true;					
				if (c == '\0' || c == ';')
					continue;
			}
			else
				continue;
		}
		
		// 	Open and close tags, Dispatch text and process '&'
		switch (c) 
		{			
			case '<':
				DispatchText();

				// If we already started a tag, try to handle the tag. If its
				// unknown, dispatch it as text.
				if (fInTag && !DispatchTag()) {
					fText[fTextCount++] = '<';
					DispatchText();
					fTag[fTagCount] = '\0';
					fBuilder->AddText(fTag, fTagCount);
				}
				fTagCount = 0;
				fInTag = true;
				break;
			case '>':
				if (fInTag) {
					DispatchTag();
					fTagCount = 0; 
					fInTag = false; 
				}
				else
					fText[fTextCount++] = c;
				break;
			case '&':
				fInAmpersand = true;
				fAmpersandCount = 1;
				fAmpersand[0] = '&';
				fAmpersand[1] = '\0';
				break;
			case '\n':
			case '\r':
				if (!IsError(!fInPRE) && fInBody) {
					Boolean	inOption = fInOption;
					DispatchText();
					if (fInTextArea)
						fText[fTextCount++] = c;
					else if (!inOption)
						fBuilder->AddTag(T_BR, nil);
					else
						fInBody = false;
				}
				break;
			default:
				if (fInTag)
				{
					if (fTagCount == 0 && c == '!') {
						fInComment = true;
						fCommentBeginCount = 0;
						fCommentEndCount = 0;
						fInTag = false;
					}
					else if (fTagCount < kMaxTagLength)
						fTag[fTagCount++] = c;
					else {
						fTag[fTagCount++] = c;
						fTag[fTagCount] = '\0';
						fBuilder->AddText(fTag, fTagCount);
						fTagCount = 0;
						fInTag = false;
					}
				}
				else if (!fInPRE && !fInBody && isspace(c))
				{
					// strip more spaces
				}
				else 
				{
					// If the buffer is full, dispatch the text.
					if (fTextCount >= kMaxTextLength)
						DispatchText();						
					fText[fTextCount++] = c;
					fInBody = true;
				}
				break;
		}
	}
	
	// Dispatch any unhandled text at end of page. Page may be missing </Body>
	if (stream->GetPending() == 0  && stream->GetStatus() == kComplete)
		DispatchText();
}

#ifdef FOR_MAC
void HTMLParser::SetResource(Resource* USED_FOR_MAC(resource))
{
	fResource = resource;
}
#endif

// ============================================================================

PlainTextParser::PlainTextParser()
{
}

PlainTextParser::~PlainTextParser()
{
}

void PlainTextParser::DispatchText()
{
	// Write a chunk of text to the page
	
	if (fTextCount == 0)
		return;

	fText[fTextCount] = 0;
	fBuilder->AddText(fText, fTextCount);

	fTextCount = 0;
}

void PlainTextParser::Parse(DataStream* stream)
{
	long pEnd = MAX(0, stream->GetPending() - kMaxParseChunk);
	short c;
	
	// On first parse, set up the title and font style.
	if (stream->GetPosition() == 0 && stream->GetPending() > 0)
	{
		// Same font as <PRE>
		fBuilder->AddTag(T_PRE, nil);

		// Use the url leaf name for title.
		const char* name = stream->GetName();
		const char*	title;
		for (title = name + strlen(name); *(title-1) != '/' && title > name; title--)
		;
		
		fBuilder->AddTag(T_TITLE, nil);
		const char*	jt = title;
		long tl = strlen(title);
		tl = CleanJISText(&jt,tl,false);
		fBuilder->AddText(jt, tl);
		fBuilder->AddTag(-T_TITLE, nil);
	}	
		
	while (stream->GetPending() > pEnd)
	{
		// Next byte from stream	
		c = *stream->ReadNext(1);
		
		// Tab to next 8 space alignment on tab in PRE.
		switch(c)
		{
			case '\t':
				for  (long tabCount = (8 - (fTextCount % 8)); tabCount > 0; tabCount--)
					fText[fTextCount++] = ' ';
				break;
			case '\n':
			case '\r':
				DispatchText();
				fBuilder->AddTag(T_BR, nil);
				break;
			default:				
				// If the buffer is full, dispatch the text.
				if (fTextCount >= kMaxTextLength)
					DispatchText();						
				fText[fTextCount++] = c;
				break;
		}
	}
	
	// Dispatch any unhandled text at end of page. Page may be missing </Body>
	if (stream->GetPending() == 0  && stream->GetStatus() == kComplete)
		DispatchText();
}

// ============================================================================

ImageMapParser::ImageMapParser()
{
}

ImageMapParser::~ImageMapParser()
{
}

const char* ImageMapParser::CopyToEndOfLine(char* dst, const char* src, const char* end)
{
	*dst = '\0';
	while (src < end)
	{
		if (*src == '\n' || *src == '\r')
		{
			*dst = '\0';
			return src + 1;
		}
		*dst++ = *src++;
	}		
	return nil;
}

void ImageMapParser::Parse(DataStream* stream)
{
	char lineBuffer[1024];
	
	// Parse only when complete
	if (stream->GetStatus() != kComplete)
		return;

	const char* data = stream->ReadNext(stream->GetDataLength());	
	const char*	end = data + stream->GetDataLength(); 
	
	// use map data to guide us
	while (data != nil)
	{
		data = CopyToEndOfLine(lineBuffer, data, end);		
		const char*	line = lineBuffer;
		ulong shape = AV_BAD;
		
		if (EqualStringN(line, "rect ", 5))
			shape = AV_RECT;
		else if (EqualStringN(line, "poly ", 5))
			shape = AV_POLY;
		else if (EqualStringN(line, "circle ", 7))
			shape = AV_CIRCLE;
			
		if (shape != AV_BAD)
		{
			while (!isspace(*line))
				line++;			/* skip "rect" */
			while (isspace(*line))
				line++;			/* skip spaces after "rect" */
			while (!isspace(*line))
				line++;			/* skip target url */
			
			// Build the tag for the builder.
			TagList* tagList = new(TagList);
			tagList->Add(A_SHAPE, shape, 0);
			tagList->Add(A_COORDS, line);
			fBuilder->AddTag(T_AREA, tagList);
			delete(tagList);
		}
	}
}

// ============================================================================
