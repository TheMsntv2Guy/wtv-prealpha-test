// ===========================================================================
//	SetUIElement.h
//
//	Copyright (c) 1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __SETUIELEMENT_H__
#define __SETUIELEMENT_H__

class BorderImage;
class ImageData;
class ImageMapCursor;
class MIDI;
class Panel;
class StringDictionary;
class Window;


// interface for client function
void			SetUI(const StringDictionary*);

// interface for direct calls
void			SetUIElement(const char* name, const char* url);
void			ResetUIElement(const char* name);
void			ResetUIElements(void);

// helper functions...maybe these should be static...
Panel*			SetUIPanel(Panel* oldPanel, const char* newUrl, ulong);
Window*			SetUIWindow(Window* oldWindow, const char* newUrl, ulong);
BorderImage*	SetUIBorderImage(BorderImage* oldBorderImage, const char* newUrl, ulong);
ImageData*		SetUIImageData(ImageData* oldImageData, const char* newUrl, ulong);
ImageMapCursor*	SetUIImageMapCursor(ImageMapCursor* oldImageMapCursor, const char* newUrl, ulong hotspot);
SoundEffect*	SetUISound(SoundEffect* oldEffect, const char* newUrl, ulong volume);


#else

#endif /* __SETUIELEMENT_H__ */