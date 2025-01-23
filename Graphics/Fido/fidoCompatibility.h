#ifndef __FIDO_COMPATIBILITY_H__

#ifndef __HEADERS_H__
#include "Headers.h"
#endif

#ifndef _FIDO_BITMAP_H_
#include "fidoBitmap.h"
#endif

#ifndef _FIDO_TEXT_H_
#include "fidoText.h"
#endif

#ifndef _FIDO_H_
#include "fido.h"
#endif

class FidoCompatibilityState {
// copies of last state sent through by client
// fido state stuff
public:
	Boolean FidoIsOff();
	void PaintText(BitMapDevice& device, XFont font, const char* text, ulong length, Color color,
		Ordinate x, Ordinate y, ulong transparency, Boolean antiAlias, const Rectangle* clip);
	void PaintRectangle(BitMapDevice& device, const Rectangle& r, Color color,
		ulong transparency, const Rectangle* clip);
	void DrawImage(BitMapDevice& device, const BitMapDevice& source, const Rectangle& destR,
		ulong transparency, const Rectangle* clip, const Rectangle* srcR);
	void DrawBorderImage(BitMapDevice& device, const BitMapDevice& source,
		const Rectangle& destR, const Rectangle& innerR, ulong transparency, const Rectangle* clip);
	displayList display;
private:
	BitMapDevice fdevice;
	XFont ffont;
	Color fcolor;
	Rectangle fclip;
	Ordinate fx;
	Ordinate fy;
	ulong ftransparency;
	void Check(BitMapDevice& device);
	void Check(XFont& font);
	void CheckColor(Color& color);
	void Check(Ordinate x, Ordinate y);
	void CheckTransparency(ulong transparency);
	void Check(const Rectangle* clip);
	FidoTexture* Translate(fidoBitMap::format&, codebook*& clut, BitMapDevice& );
	void Update();
};

#endif // __FIDO_COMPATIBILITY_H__