#include "fidoCompatibility.h"
#ifndef HARDWARE
#define kCheckMark	0x12
#endif
#include "MacintoshMenus.h"


Boolean FidoCompatibilityState::FidoIsOff()
{
	MenuHandle menu = GetMenu(mHardware);
	short markChar;
	GetItemMark(menu, iFidoEnabled, &markChar);
	return markChar != kCheckMark;
}

void FidoCompatibilityState::Check(BitMapDevice& device)
{
	if (memcmp((const void*) &device, (const void*) &fdevice, sizeof(BitMapDevice)) != 0) {
		fdevice = device;
	}
}	

void FidoCompatibilityState::Check(XFont& font)
{
	if (font != ffont) {
		ffont = font;
		short theFont = (short)(font>>16);
		short theStyle = (short)((font >> 8) & 0xff);
		short theSize = (short)(font & 0xff);
		fontTypes::family fidoFamily = theFont == kMonaco ? fontTypes::monospaced :
			fontTypes::proportional;
		fontTypes::face fidoFace = theStyle == kNormalStyle ? fontTypes::plain : 
			theStyle == kItalicStyle ? fontTypes::italic :  theStyle == kBoldStyle ? fontTypes::bold : 
			theStyle == (kBoldStyle | kItalicStyle) ? fontTypes::boldItalic : fontTypes::plain;
		display.find(fidoFamily, fidoFace, theSize);
	}
}	

void FidoCompatibilityState::CheckColor(Color& color)
{
	if (color != fcolor) {
		fcolor = color;
		rgbPixel rgb = {0, color >> 16, color >> 8, color};
		yuvPixel yuv;
		rgb.toYUV(yuv);
		display.fillColor.setYAUV(yuv.y, display.fillColor.getAlpha(), yuv.u, yuv.v);
	}
}	

void FidoCompatibilityState::Check(Ordinate x, Ordinate y)
{
	if (x != fx) {
		fx = x;
		display.matrix.name.moveX = ff(x);
	}
	if (y != fy) {
		fy = y;
		display.matrix.name.moveY = ff(y);
	}
}		

void FidoCompatibilityState::CheckTransparency(ulong transparency)
{
	if (transparency != ftransparency) {
		ftransparency = transparency;
		display.fillColor.setAlpha(transparency);
	}
}
	
void FidoCompatibilityState::Check(const Rectangle* clip)
{
	if (clip && memcmp((const void*) clip, (const void*) &fclip, sizeof(Rectangle)) != 0)  {	
		// !!! (if characters are clipped, need to generate a mini record instead of a micro record)
		fclip = *clip;
		display.setClip(ff(clip->left), ff(clip->top),
			ff(clip->right), ff(clip->bottom));
	}
}

void FidoCompatibilityState::DrawImage(BitMapDevice& device, const BitMapDevice& source, const Rectangle& destR,
	ulong transparency, const Rectangle* clip, const Rectangle* )
{
	Assert(this);
	Check(device);
	Check(destR.left - source.bounds.left, destR.top - source.bounds.top);
	CheckTransparency(transparency);	// map the trasparency to alpha
	Check(clip);	// map the clip to a fido rectangle
	fidoBitMap::format format;
	codebook* clut;
	FidoTexture* bits = Translate(format, clut, const_cast<BitMapDevice&>(source));
	fidoBitMap map(bits, source.bounds.left, source.bounds.top, format,
		display.fillColor.getAlpha(), source.rowBytes, 0, 0, clut);
	display.add(map);
}

// !!! this needs quite a bit of work to make it real
void FidoCompatibilityState::DrawBorderImage(BitMapDevice& device, const BitMapDevice& source,
	const Rectangle& destR, const Rectangle& UNUSED(innerR), ulong transparency, 
	const Rectangle* clip)
{
	Assert(this);
	Check(device);
	Check(destR.left - source.bounds.left, destR.top - source.bounds.top);
	CheckTransparency(transparency);	// map the trasparency to alpha
	Check(clip);	// map the clip to a fido rectangle
	fidoBitMap::format format;
	codebook* clut;
	FidoTexture* bits = Translate(format, clut, const_cast<BitMapDevice&>(source));
	fidoBitMap map(bits, source.bounds.left, source.bounds.top, format,
		display.fillColor.getAlpha(), source.rowBytes, 0, 0, clut);
	display.add(map);
}


void FidoCompatibilityState::PaintText(BitMapDevice& device, XFont font, const char* text, ulong length, Color color,
	Ordinate x, Ordinate y, ulong transparency, Boolean UNUSED(antiAlias), const Rectangle* clip)
{	
	// map the device to a display list (assume a shallow copy is enough)
	Assert(this);
	Check(device);
	Check(font);	// map the font to fido font
	CheckColor(color);	// map the color to a fido color
	Check(x, y);	// map the coordinates to fixed point
	CheckTransparency(transparency);	// map the trasparency to alpha
	// Check(antiAlias); // ignore the antiAlias flag
	Check(clip);	// map the clip to a fido rectangle
	display.add(text, (int) length); 
}


void FidoCompatibilityState::PaintRectangle(BitMapDevice& device, const Rectangle& r, Color color,
	ulong transparency, const Rectangle* clip)
{
	Assert(this);
	Check(device);
	CheckColor(color);	// map the color to a fido color
	Check(0, 0);	// map the coordinates to (0, 0)
	CheckTransparency(transparency);	// map the trasparency to alpha
	Check(clip);	// map the clip to a fido rectangle
	fidoRectangle fixRect(ff(r.left), ff(r.top), ff(r.right), ff(r.bottom));
	display.add(fixRect);
}

FidoTexture* FidoCompatibilityState::Translate(fidoBitMap::format& fidoForm, codebook*& clut, BitMapDevice& bruceBitmap)
{
	switch (bruceBitmap.format) {
		case yuvFormat:
			Complain(("unsupported bitmap format"));
		break;
		case yuv422Format:
			Complain(("unsupported bitmap format"));
		break;
		case yuv422PlanarFormat:
			Complain(("unsupported bitmap format"));
		break;
		case gray8Format:
			Complain(("unsupported bitmap format"));
		break;
		case index1Format:
			Complain(("unsupported bitmap format"));
		break;
		case index2Format:
			Complain(("unsupported bitmap format"));
		break;
		case index4Format:
			Complain(("unsupported bitmap format"));
		break;
		case index8Format:
			fidoForm = fidoBitMap::indexed8;
			// create a clut
			Assert(bruceBitmap.colorTable && (bruceBitmap.colorTable->version == kRGB24 ||
				bruceBitmap.colorTable->version == kRGB15));
			bruceBitmap.clutType = kYUAV_32;
			clut = bruceBitmap.clutBase = AllocateArray(codebook, 256);
			if (bruceBitmap.colorTable->version == kRGB24) {
				uchar* rgb24 = (uchar*) &bruceBitmap.colorTable->data;
				for (int index = 0; index < 255; index++) {
	#ifdef DEBUG
					clut[index].type = codebook::yauvType;
	#endif
					rgbPixel rgb;
					rgb.r = *rgb24++;
					rgb.g = *rgb24++;
					rgb.b = *rgb24++;
					rgb.toYUV(clut[index].yauv);
				}
			} else if (bruceBitmap.colorTable->version == kRGB15) {
				short* rgb15 = (short*)(&bruceBitmap.colorTable->data);
				for (int index = 0; index < 255; index++) {
	#ifdef DEBUG
					clut[index].type = codebook::yauvType;
	#endif
					rgbPixel rgb;
					ushort pixel = *rgb15++;
					rgb.r = pixel>>7 & 0xf8 | pixel>>12 & 0x07; 
					rgb.g = pixel>>2 & 0xf8 | pixel>>7 & 0x07; 
					rgb.b = pixel<<3 & 0xf8 | pixel>>2 & 0x07;
					rgb.toYUV(clut[index].yauv);
				}
			}
			bruceBitmap.textureType = kYUAV_8;
			// create a bitmap
			bruceBitmap.textureBase = (FidoTexture*) bruceBitmap.baseAddress;
		break;
		case rgb16Format:
			Complain(("unsupported bitmap format"));
		break;
		case rgb32Format:
			Complain(("unsupported bitmap format"));
		break;
	}
	if (bruceBitmap.colorTable->version == kRGB24) 
		Complain(("unsupported clut format"));
	else if (bruceBitmap.colorTable->version == kRGB15)
		Complain(("unsupported clut format"));
	else
		Complain(("unsupported clut format"));
	return bruceBitmap.textureBase;
}


void FidoCompatibilityState::Update()
{
	display.build();
	display.draw();
}

