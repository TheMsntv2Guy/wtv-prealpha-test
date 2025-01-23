// ===========================================================================
//	ImageMap.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __CONTENTVIEW_H__
#include "ContentView.h"
#endif
#ifndef __DOCUMENTBUILDER_H__
#include "DocumentBuilder.h"
#endif
#ifndef __IMAGE_H__
#include "Image.h"
#endif
#ifndef __IMAGEDATA_H__
#include "ImageData.h"
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
#ifndef __URLPARSER_H__
#include "URLParser.h"
#endif

// =============================================================================

static const long	kMinSelectionIncrement = 4;		// in pixels
static const long	kMaxSelectionIncrement = 32;	// in pixels
static const long	kAccelerationRate = 2;			// in pixels
static const long	kAccelerateTimeLimit = kOneSecond / 3;
static const long	kEdgeDecelerateZone = 12;		// in pixels

// =============================================================================

Area::Area()
{
	fShape = AV_RECT;
}

Area::~Area()
{
	if (fHREF != nil)
		FreeTaggedMemory(fHREF, "Area::fHREF");
}

	
void Area::GetBounds(Rectangle* bounds) const
{
	bounds->left = fLeft;
	bounds->top = fTop;
	bounds->right = fRight;
	bounds->bottom = fBottom;
}

void Area::SetAttribute(Attribute attributeID, long value, Boolean)
{
	if (attributeID == A_SHAPE && value > 0)
		fShape = (AttributeValue)value;
}

void Area::SetAttributeStr(Attribute attributeID, const char* value)
{
	// MHK: some documents use spaces instead of commas as separators
	// 	    it might be best if we just looked for not-numbers instead of specific separators
	// 		to compensate for whacky html docs
	
	switch (attributeID)
	{
		case A_COORDS:
			if (fShape == AV_RECT) {
				long left, top, right, bottom;
				if (sscanf(value, "%ld%*[ ,]%ld%*[ ,]%ld%*[ ,]%ld", &left, &top, &right, &bottom) == 4) {
					fLeft = left;
					fTop = top;
					fRight = right + 1;				// HTML bounds are inclusive.
					fBottom = bottom + 1;
				}
			}
			else if (fShape == AV_POLY || fShape == AV_POLYGON) {
				// For Polygons, we compute the bounding rectangle.
				
				long	x, y;
				
				fTop = fBottom = fLeft = fRight = 0;
				while (isspace(*value)) value++;				
				while (value != nil && sscanf(value, "%ld%*[ ,]%ld", &x, &y) == 2) {
					if (fTop == 0 && fBottom == 0 && fLeft == 0 && fRight == 0) {
						fTop = fBottom = y;
						fLeft = fRight = x;
					}
					else {
						fTop = MIN(fTop, y);
						fBottom = MAX(fBottom, y);
						fLeft = MIN(fLeft, x);
						fRight = MAX(fRight, x);
					}
					
					// Skip past 2 delimiters
					while (*value != '\0' && *value != ' ' && *value != ',') value++;
					while (*value != '\0' && (*value == ' ' || *value == ',')) value++;
					while (*value != '\0' && *value != ' ' && *value != ',') value++;
					while (*value != '\0' && (*value == ' ' || *value == ',')) value++;
				}
			}
			else if (fShape == AV_CIRCLE) {
			
				// according to "http://www.netscape.com/assist/net_sites/html_extensions_3.html"
				// it should be "centerx,centery,radius"
				
				long xCenter, yCenter, radius;
				if (sscanf(value, "%ld%*[ ,]%ld%*[ ,]%ld", &xCenter, &yCenter, &radius) == 3) {
					fTop = yCenter - radius;
					fBottom = yCenter + radius;
					fLeft = xCenter - radius;
					fRight = xCenter + radius;
				}
			}
			break;
		
		case A_HREF:
			fHREF = CopyStringTo(fHREF, value, "Area::fHREF");
			break;
			
		case A_NOHREF:
			// Do nothing. The lack of an HREF will signal this.
			break;
		default:
			break;
	}
}

// =============================================================================

ImageMap::ImageMap()
{
	fState = kWaitForLocalImageMap;	// Assume local until we load.
	fLocal = true;
}

ImageMap::~ImageMap()
{
	Reset();
	
	if (fName != nil)
		FreeTaggedMemory(fName, "ImageMap::fName");
}
	
void ImageMap::AddArea(Area* area)
{
	Rectangle	bounds;
	area->GetBounds(&bounds);
	
	// Delete the area if its empty. еее Need to handle "DEFAULT".
	if (!EmptyRectangle(&bounds))
		fAreaList.Add(area);
	else
		delete(area);
}

const Resource* ImageMap::GetBaseResource() const
{
	return fBaseResource.HasURL() ? &fBaseResource : &fResource;
}

void ImageMap::CleanupStream()
{
	delete(fParser); fParser = nil;
	delete(fBuilder); fBuilder = nil;
	delete(fStream); fStream = nil;
}
	

void ImageMap::HandleRedirect()
{
	DataStream*	stream = fResource.NewStream();
	if (stream != nil) {
		char*	url = CopyString(stream->GetDataAsString(), "ImageMap::HandleRedirect()");
		delete(stream);
		if (url != nil) {
			fResource.SetPriority((Priority)0);
			fResource.SetURL(url);
			FreeTaggedMemory(url, "ImageMap::HandleRedirect()");
			return;
		}
	}
	fState = kImageMapError;
}
			
Boolean ImageMap::Idle()
{
	PerfDump perfdump("ImageMap::Idle");

	Boolean wasComplete = IsComplete();
	
	switch (fState)
	{
		case kCreateImageMapStream:
			if (TrueError(fResource.GetStatus())) {
				fState = kImageMapError;
				break;
			}
			
			if (fResource.GetDataType() == 0)
				break;
			
			// Handle redirects
			if (fResource.GetDataType() == kDataTypeURL) {
				HandleRedirect();
				break;
			}
			
			if (fType == kClientMap && fResource.GetDataType() != kDataTypeHTML) {
				fState = kImageMapError;
				break;
			}
		
			if ((fStream = fResource.NewStream()) == nil) 
				break;
				
			fBuilder = new(ImageMapBuilder);
			fBuilder->SetTargetMap(this);
			if (fType == kClientMap)
				fParser = new(HTMLParser);
			else
			{
				fParser = new(ImageMapParser);
				fBuilder->OpenImageMap();		// Only one map for server maps, open it.
			}
			fParser->SetBuilder(fBuilder);
			fState = kParseImageMap;
			
		case kParseImageMap:			
			{
				Error status = fStream->GetStatus();
				
				if (status == kStreamReset) {
					Reset();
					fState = kCreateImageMapStream;
					break;
				}
				
				if (TrueError(status)) {
					CleanupStream();
					fState = kImageMapError;
					break;
				}
				
				fParser->Parse(fStream);
	
				// If the stream is complete, or target has been built, we're done.			
				if ((fStream->GetStatus() == kComplete && fStream->GetPending() == 0) ||
				    fBuilder->GetTargetMap() == nil) {
					fBuilder->Finalize();
					CleanupStream();
					fState = kImageMapDone;
				}
			}
			break;
			
		case kWaitForLocalImageMap:
		case kImageMapDone:
		case kImageMapError:
			break;
			
		default:
			IsError(true); // Should never reach here.
			break;
	}
	
	return (wasComplete != IsComplete());
}

void ImageMap::Load()
{
	if (IsError(fName == nil || !fResource.HasURL()))
		return;
	
	fLocal = false;
	
	fResource.SetPriority(kNearby); // Let's get these soon.
	fState = kCreateImageMapStream;
}

void ImageMap::Reset()
{
	CleanupStream();
	fAreaList.DeleteAll();
	fState = kCreateImageMapStream;
}
	
void ImageMap::SetBaseURL(const char* url)
{
	fBaseResource.SetURL(url);
}

void ImageMap::SetResourceAndName(const Resource* resource, const char* name)
{
	fResource = *resource;
	fName = CopyStringTo(fName, name, "ImageMap::fName");
}

// =============================================================================

ImageMapSelectable::ImageMapSelectable()
{
#ifdef DEBUG_CLASSNUMBER
	fClassNumber = kClassNumberImageMapSelectable;
#endif /* DEBUG_CLASSNUMBER */
}

ImageMapSelectable::~ImageMapSelectable()
{
}
	
Boolean ImageMapSelectable::BumpSelectionPosition(Coordinate* position, const Coordinate* delta,
									 	  		  ContentView* view)
{
	static ulong		lastInputTime;
	static long			lastIncrement;
	static Coordinate	lastDelta;
	
	if (IsError(position == nil || delta == nil))
		return false;
	
	if (fSelectable == nil || fSelectable->GetImageMap() == nil)
		return false;


	// Compute increment to use based on last increment and input delay.
	if (view->GetCurrentInputTime() - lastInputTime > kAccelerateTimeLimit ||
		lastDelta.x != delta->x || lastDelta.y != delta->y)
		lastIncrement = kMinSelectionIncrement;
	else
		lastIncrement = MIN(kMaxSelectionIncrement, lastIncrement * kAccelerationRate);
	lastInputTime = view->GetCurrentInputTime();
	lastDelta = *delta;
			
	Rectangle	bounds;
	Rectangle	cursorBounds;
	Rectangle	visibleBounds;
	
	// Invalidate the old cursor position
	cursorBounds = GetCursorBounds(position);
	view->ContentToScreen(&cursorBounds);
	view->InvalidateAbove(&cursorBounds);
	
	fSelectable->GetMappedImage()->GetBoundsTight(&bounds);
	long height = RectangleHeight(bounds);
	long width = RectangleWidth(bounds);
	
	// Update current position
	long oldX = position->x;
	long oldY = position->y;
	
	position->x += delta->x * lastIncrement;
	position->y += delta->y * lastIncrement;
	
	// Limit speed at edges.
	if (position->y < kEdgeDecelerateZone && position->y < oldY)
		position->y = MIN(kEdgeDecelerateZone, oldY - kMinSelectionIncrement);
	else if (height - position->y < kEdgeDecelerateZone && position->y > oldY)
		position->y = MAX(height - kEdgeDecelerateZone, oldY + kMinSelectionIncrement);
	else if (position->x < kEdgeDecelerateZone && position->x < oldX)
		position->x = MIN(kEdgeDecelerateZone, oldX - kMinSelectionIncrement);
	else if (width - position->x < kEdgeDecelerateZone && position->x > oldX)
		position->x = MAX(width - kEdgeDecelerateZone, oldX + kMinSelectionIncrement);
		
		
	// check for falling out vertically or horizontally
	if (position->y < 0 || position->y >= height ||
		position->x < 0 || position->x >= width) {
		view->InvalidateBounds(&cursorBounds);
		return false;
	}
	
	cursorBounds = GetCursorBounds(position);

	// Scroll if necessary
	view->VisibleContentBounds(visibleBounds);
	if (cursorBounds.top < visibleBounds.top)
		view->ScrollUp(kMaxSelectionIncrement*2, kMaxSelectionIncrement);
	else if (cursorBounds.bottom > visibleBounds.bottom)
		view->ScrollDown(kMaxSelectionIncrement*2, kMaxSelectionIncrement);

	// Invalidate the new cursor position
	view->ContentToScreen(&cursorBounds);
	view->InvalidateAbove(&cursorBounds);
	
	return true;
}

char* ImageMapSelectable::NewURL(const Coordinate* selectionPosition, const char* tag) const
{
	if (IsError(fSelectable == nil || fSelectable->GetImageMap() == nil))
		return nil;

	ImageMap*	imageMap = fSelectable->GetImageMap();

	// Client side image map. Build the url starting with the image map's document as the base.
	if (imageMap->GetType() == kClientMap)
	{
		if (fArea != nil && fArea->GetHREF() != nil)
		{
			char* url = imageMap->GetBaseResource()->CopyURL("ImageMapSelectable::NewURL");
			URLParser urlParser;
			urlParser.SetURL(url);
			FreeTaggedMemory(url, "ImageMapSelectable::NewURL");
			
			return urlParser.NewURL(fArea->GetHREF(), tag);
		}
		else			
			return nil;		
	}
		
	// Its a server map format the coordinates of the selection.
	if (IsError(selectionPosition == nil))
		return nil;
		
	Coordinate point = GetTargetPoint(*selectionPosition);

	char buffer[1 + 11 + 1 + 11 + 1]; // i.e., ('?' + %ld + ',' + %ld + NULL)
	snprintf(buffer, sizeof(buffer), "?%ld,%ld", point.x, point.y);
	
	char* mapURL = fSelectable->NewURL(selectionPosition, tag);
	
	return CatStringTo(mapURL, buffer, tag);
}

Boolean ImageMapSelectable::ExecuteInput()
{
	if (IsError(fSelectable == nil))
		return false;
	
	return fSelectable->ExecuteInput();
}

void ImageMapSelectable::GetBounds(Rectangle* bounds) const
{
	if (IsError(fSelectable == nil))
		return;

	Image*	image = fSelectable->GetMappedImage();
	if (IsError(image == nil))
		return;
	
	Rectangle	imageBounds;		
	image->GetBoundsTight(&imageBounds);

	if (fArea != nil)
	{
		fArea->GetBounds(bounds);
		
		// Deal with scaled images
		Rectangle	imageUBounds;		
		image->GetUnscaledBoundsTight(&imageUBounds);
		if ((imageBounds.right - imageBounds.left) != (imageUBounds.right - imageUBounds.left) ||
			(imageBounds.bottom - imageBounds.top) != (imageUBounds.bottom - imageUBounds.top))
		{
			bounds->top = bounds->top * (imageBounds.bottom - imageBounds.top) / 
									    (imageUBounds.bottom - imageUBounds.top);
			bounds->bottom = bounds->bottom * (imageBounds.bottom - imageBounds.top) / 
										      (imageUBounds.bottom - imageUBounds.top);
			bounds->left = bounds->left * (imageBounds.right - imageBounds.left) / 
										  (imageUBounds.right - imageUBounds.left);
			bounds->right = bounds->right * (imageBounds.right - imageBounds.left) /
										    (imageUBounds.right - imageUBounds.left);
		}
		OffsetRectangle(*bounds, imageBounds.left, imageBounds.top);
		
		// Clip data to image bounds
		if (bounds->bottom > imageBounds.bottom)
			bounds->bottom = imageBounds.bottom;
		if (bounds->right > imageBounds.right)
			bounds->right = imageBounds.right;
	}
	else
		*bounds = imageBounds;
}

Rectangle ImageMapSelectable::GetCursorBounds(const Coordinate* position) const
{
	Rectangle	cursorBounds;
	Rectangle	bounds;
	Coordinate	absPosition;
	Coordinate	hotspot;
	
	fSelectable->GetMappedImage()->GetBoundsTight(&bounds);

	// Determine absolute selection position from image bounds.	
	absPosition.y = bounds.top + position->y;
	absPosition.x = bounds.left + position->x;

	gImageMapCursorNormal->GetBounds(&cursorBounds);
	
	// Point bottom-left hot spot to selection position.
	OffsetRectangle(cursorBounds, absPosition.x, absPosition.y);

	gImageMapCursorNormal->GetHotspot(&hotspot);
	OffsetRectangle(cursorBounds, -hotspot.x, -hotspot.y);

	return cursorBounds;
}

Image* ImageMapSelectable::GetMappedImage() const
{
	if (IsError(fSelectable == nil))
		return nil;
	
	return fArea != nil ? nil : fSelectable->GetMappedImage();
}

Displayable* ImageMapSelectable::GetParent() const
{
	if (IsError(fSelectable == nil))
		return nil;
	
	return fSelectable->GetParent();
}

Coordinate ImageMapSelectable::GetTargetPoint(Coordinate selectionPosition) const
{
	// Its a server map. If we have map data use it.
	Coordinate	point = {selectionPosition.x, selectionPosition.y};
	Rectangle 	bounds;

	if (fSelectable == nil || fSelectable->GetMappedImage() == nil)
		return point;	
	
	Image*		image = fSelectable->GetMappedImage();

	if (fArea != nil)
	{
		// We have an area, use its center position.
		fArea->GetBounds(&bounds);
		
		point.x = (bounds.left + bounds.right) / 2;
		point.y = (bounds.top + bounds.bottom) / 2;
	}
	else
	{
		// Deal with scaled coordinates. We need to invert the scaling before sending points.
		image->GetBoundsTight(&bounds);
		Rectangle	uBounds;
		image->GetUnscaledBoundsTight(&uBounds);
		if ((bounds.right - bounds.left) != (uBounds.right - uBounds.left) ||
			(bounds.bottom - bounds.top) != (uBounds.bottom - uBounds.top))
		{
			point.y = point.y * (uBounds.bottom - uBounds.top) / (bounds.bottom - bounds.top);
			point.x = point.x * (uBounds.right - uBounds.left) / (bounds.right - bounds.left);
		}
	}
	
	return point;
}
	
Boolean ImageMapSelectable::HasURL() const
{
	if (IsError(fSelectable == nil))
		return false;
	
	if (fArea != nil && fArea->HasHREF())
		return true;
	
	return fSelectable->HasURL();
}

Boolean	ImageMapSelectable::IsImageMapSelectable() const
{
	return true;
}

Boolean ImageMapSelectable::IsInitiallySelected() const
{
	return fIsInitiallySelected;
}

Boolean ImageMapSelectable::IsSelectable() const
{
	return true;
}

Boolean ImageMapSelectable::KeyboardInput(Input* input)
{
	if (IsError(fSelectable == nil))
		return false;
	
	return fSelectable->KeyboardInput(input);
}

void ImageMapSelectable::GetSelectionRegion(Region* region) const
{
	if (IsError(region == nil))
		return;
	
	region->Reset();
	
	Rectangle bounds;	
	GetBounds(&bounds);
	region->Add(&bounds);
}


// =============================================================================

ImageMapCursor* ImageMapCursor::NewImageMapCursor(const char* url)
{
	ImageMapCursor* image = new(ImageMapCursor);
	
	image->fResource.SetURL(url);
	image->fResource.SetDataType(kDataTypeGIF);
	image->fAverageColor = (Color)-1;	
	return image;
}

// =============================================================================

