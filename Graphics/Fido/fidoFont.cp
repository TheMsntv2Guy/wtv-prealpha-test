#include "fidoDebug.h"
#include "fidoFont.h"
	
#ifdef FULL_SET_OF_FONTS
const proportional_font* fontDirectory::proportional[4][16] = {
	{&propPlain12, 	&propPlain14,		&propPlain16,		&propPlain18,		&propPlain20,
	&propPlain22,		&propPlain24,		&propPlain26,		&propPlain28,		&propPlain30,
	&propPlain32,		&propPlain34,		&propPlain36,		&propPlain38,		&propPlain40,		&propPlain42},
	{&propItalic12,		&propItalic14,		&propItalic16,		&propItalic18,		&propItalic20,
	&propItalic22,		&propItalic24,		&propItalic26,		&propItalic28,		&propItalic30,
	&propItalic32,		&propItalic34,		&propItalic36,		&propItalic38,		&propItalic40,		&propItalic42},
	{&propBold12,		&propBold14,		&propBold16,		&propBold18,		&propBold20,
	&propBold22,		&propBold24,		&propBold26,		&propBold28,		&propBold30,
	&propBold32,		&propBold34,		&propBold36,		&propBold38,		&propBold40,		&propBold42},
	{&propBldItal12,	&propBldItal14,	&propBldItal16,	&propBldItal18,	&propBldItal20,
	&propBldItal22,	&propBldItal24,	&propBldItal26,	&propBldItal28,	&propBldItal30,
	&propBldItal32,	&propBldItal34,	&propBldItal36,	&propBldItal38,	&propBldItal40,	&propBldItal42}
};
const monospaced_font* fontDirectory::monospaced[4][16] = {
	{&monoPlain12,	&monoPlain14,		&monoPlain16,		&monoPlain18,		&monoPlain20,
	&monoPlain22,		&monoPlain24,		&monoPlain26,		&monoPlain28,		&monoPlain30,	
	&monoPlain32,		&monoPlain34,		&monoPlain36,		&monoPlain38,		&monoPlain40,		&monoPlain42},
	{&monoItalic12,	&monoItalic14,		&monoItalic16,		&monoItalic18,		&monoItalic20,	
	&monoItalic22,		&monoItalic24,		&monoItalic26,		&monoItalic28,		&monoItalic30,
	&monoItalic32,		&monoItalic34,		&monoItalic36,		&monoItalic38,		&monoItalic40,		&monoItalic42},
	{&monoBold12,		&monoBold14,		&monoBold16,		&monoBold18,		&monoBold20,
	&monoBold22,		&monoBold24,		&monoBold26,		&monoBold28,		&monoBold30,
	&monoBold32,		&monoBold34,		&monoBold36,		&monoBold38,		&monoBold40,		&monoBold42},
	{&monoBldItal12, 	&monoBldItal14,	&monoBldItal16,	&monoBldItal18,	&monoBldItal20,
	&monoBldItal22, 	&monoBldItal24,	&monoBldItal26,	&monoBldItal28,	&monoBldItal30,
	&monoBldItal32, 	&monoBldItal34,	&monoBldItal36,	&monoBldItal38,	&monoBldItal40,	&monoBldItal42}
};
#else
const proportional_font* fontDirectory::proportional[4][16] = {
	{&propPlain12, 	&propPlain14,		0,			&propPlain18,		&propPlain20,
	0,				&propPlain24,		0,			&propPlain28,		0,
	&propPlain32,		0,				&propPlain36,	0,				0,			0},
	{&propItalic12,		&propItalic14,		0,			&propItalic18,		&propItalic20,
	0,				&propItalic24,		0,			&propItalic28,		0,
	&propItalic32,		0,				&propItalic36,	0,				0,			0},
	{&propBold12,		&propBold14,		0,			&propBold18,		&propBold20,
	0,				&propBold24,		0,			&propBold28,		0,
	&propBold32,		0,				&propBold36,	0,				0,			0},
	{&propBldItal12,	&propBldItal14,	0,			&propBldItal18,	&propBldItal20,
	0,				&propBldItal24,	0,			&propBldItal28,	0,
	&propBldItal32,	0,				&propBldItal36,	0,	0,	0}
};
const monospaced_font* fontDirectory::monospaced[4][16] = {
	{&monoPlain12,	&monoPlain14,		0,			&monoPlain18,		&monoPlain20,
	0,				&monoPlain24,		0,			&monoPlain28,		0,	
	&monoPlain32,		0,				&monoPlain36,	0,		0,		0},
	{&monoItalic12,	&monoItalic14,		0,		&monoItalic18,		&monoItalic20,	
	0,		&monoItalic24,		0,		&monoItalic28,		0,
	&monoItalic32,		0,		&monoItalic36,		0,		0,		0},
	{&monoBold12,		&monoBold14,		0,		&monoBold18,		&monoBold20,
	0,		&monoBold24,		0,		&monoBold28,		0,
	&monoBold32,		0,		&monoBold36,		0,		0,		0},
	{&monoBldItal12, 	&monoBldItal14,	0,	&monoBldItal18,	&monoBldItal20,
	0, 	&monoBldItal24,	0,	&monoBldItal28,	0,
	&monoBldItal32, 	0,	&monoBldItal36,	0,	0,	0}
};
#endif

const fidoFont* fontDirectory::find(fontTypes::family fontID, fontTypes::face style, int size)
{
	 if (fontID < fontTypes::builtInFontCount) {
	 	Assert(fontID >= fontTypes::proportional && fontID <= fontTypes::monospaced);
	 	Assert(style >= fontTypes::plain && style <= fontTypes::boldItalic);
	 	if (size & 1 || size < 12 || size > 42) {
	 		Assert(0);
	 		return nil; 
	 	}
	 	int sizeIndex = size - 12 >> 1; 
	 	return  fontID == fontTypes::proportional ? (const fidoFont*) proportional[style][sizeIndex] : (const fidoFont*) monospaced[style][sizeIndex];
	 }
	return HoweverRAMBasedFontsWork(fontID, style, size);
 }
 
 // !!!  unimplemented
 const fidoFont* fontDirectory::HoweverRAMBasedFontsWork(fontTypes::family, fontTypes::face, int)
 {
 	Assert(0);
 	return nil;
 }