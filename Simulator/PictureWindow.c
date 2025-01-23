
// Creates a window with the shape of the specified picture
//   and sets the picture as the background image

// requires WDEF 1000 to be present in the resource map

WindowPtr NewPictureWindow(short pictResID);
GrafPtr NewOffScreenBitMap(Rect *rectP);
void DeleteOffScreenBitMap(GrafPtr grafPtr);
GrafPtr GetPictureBitMap(short resID);

#define amorphDef		16000	/* WDEF id for window */
#define BRAINDAMAGE		3		/* used for multi-finder friendliness, Barf */

WindowPtr NewPictureWindow(short pictResID)
{
	GrafPtr		pictPort, maskPort, oldPort;
	RgnHandle	pictRgn;
	WindowPtr	window;

	pictPort = GetPictureBitMap(pictResID);				/* put picture in offscreen bitmap */
	if (pictPort == nil)
		return (nil);

	maskPort = NewOffScreenBitMap(&pictPort->portRect); /* create offscreen bitmap for mask */
	if (maskPort == nil)
		return (nil);
	
	CalcMask(pictPort->portBits.baseAddr,
			 maskPort->portBits.baseAddr,
			 pictPort->portBits.rowBytes,
			 maskPort->portBits.rowBytes,
			 pictPort->portBits.bounds.bottom - pictPort->portBits.bounds.top,
			 (pictPort->portBits.rowBytes + 1) / 2);	/* calculate mask for picture */

	GetPort(&oldPort);
	SetPort(pictPort);
	pictRgn = NewRgn();
	BitMapToRegion(pictRgn, &maskPort->portBits);		/* get region for bitmap */
	SetPort(oldPort);
	
	window = NewCWindow(nil, &pictPort->portRect, "\pTest", false, amorphDef + BRAINDAMAGE,
					    (WindowPtr) -1, false, (long) pictRgn);
	if (window == nil)
		return (nil);

	SetPort(window);
	SetOrigin((*pictRgn)->rgnBBox.left, (*pictRgn)->rgnBBox.top);

	PicHandle pictH = (PicHandle) GetResource('PICT', pictResID);
	DetachResource((Handle) pictH);
	SetWindowPic(window, pictH);

	DeleteOffScreenBitMap(pictPort);
	DeleteOffScreenBitMap(maskPort);

	return (window);
}

/*	=============================
		Bitmap Routines
	============================= */

GrafPtr NewOffScreenBitMap(Rect *rectP)
{
	GrafPtr			grafPtr, oldPort;
	long			pSize, rBytes;
	Ptr				myBits;
	
	rBytes = ((1 * rectP->right + 15) / 16) * 2;	/* calculate bytes needed */
	pSize = ((long) (rectP->bottom)) * rBytes;
	
	myBits = NewPtr(pSize);							/* get room for bitmap */
	if (myBits == nil)
		return (nil);
	
	grafPtr = (GrafPtr) NewPtr(sizeof(GrafPort));	/* get room for port record */
	if (grafPtr == nil)
		return (nil);
	
	GetPort(&oldPort);
	OpenPort(grafPtr);
	SetPort(oldPort);
	
	grafPtr->portBits.baseAddr = myBits;			/* point to our bitmap memory */
	grafPtr->portBits.rowBytes = rBytes;
	grafPtr->portBits.bounds = *rectP;

	grafPtr->portRect = *rectP;
/*
	SetRectRgn(grafPtr->visRgn, -30000, -30000, 30000, 30000);
	SetRectRgn(grafPtr->clipRgn, -30000, -30000, 30000, 30000);
*/
	return (grafPtr);							/* return pointer to port with bitmap in it */
}

void DeleteOffScreenBitMap(GrafPtr grafPtr)
{
	if (grafPtr == nil)
		return;
		
	DisposPtr(grafPtr->portBits.baseAddr);			/* delete bitmap memory */
	ClosePort(grafPtr);								/* delete port */
	DisposPtr((Ptr) grafPtr);						/* delete port record memory */
}

GrafPtr GetPictureBitMap(short resID)
{
	PicHandle	pictH;
	Rect		pictR;
	GrafPtr		pictPort, oldPort;
	
	pictH = (PicHandle) GetResource('PICT', resID);
	if (pictH == nil)
		return (nil);
		
	BlockMove((Ptr) *pictH + 2, (Ptr) &pictR, sizeof(Rect));	/* store boundary rectangle */
	OffsetRect(&pictR, -(pictR.left), -(pictR.top));			/* normalize it */
	
	pictPort = NewOffScreenBitMap(&pictR);						/* create offscreen bitmap for picture */
	if (pictPort == nil)
		return (nil);

	GetPort(&oldPort);
	SetPort(pictPort);

	EraseRect(&pictR);											/* clear offscreen bitmap */
	DrawPicture(pictH, &pictR);									/* draw picture to offscreen bitmap */

	SetPort(oldPort);
	ReleaseResource((Handle) pictH);

	return (pictPort);
}
