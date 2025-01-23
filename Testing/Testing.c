// ===========================================================================
//	Testing.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifdef TEST_SIMULATORONLY

#ifndef __AUTHORIZATION_H__
#include "Authorization.h"
#endif
#ifndef __CACHE_H__
#include "Cache.h"
#endif
#ifndef __CLOCK_H__
#include "Clock.h"
#endif
#ifndef __COOKIE_H__
#include "Cookie.h"
#endif
#ifndef __DATETIME_H__
#include "DateTime.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __PAGEVIEWER_H__
#include "PageViewer.h"
#endif
#ifndef __SERVICE_H__
#include "Service.h"
#endif
#ifndef __SOUND_H__
#include "Sound.h"
#endif
#ifndef __TESTING_H__
#include "Testing.h"
#endif
#ifndef __TESTING_PRIVATE_H__
#include "Testing.Private.h"
#endif
#ifndef __WTVPROTOCOL_H__
#include "WTVProtocol.h"
#endif
#ifndef __URLPARSER_H__
#include "URLParser.h"
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"
#endif

#include "CryptoInterface.h"

#if defined(FOR_MAC)
	#ifndef __MACSIMULATOR_H__
	#include "MacSimulator.h"
	#endif
#endif

// ===========================================================================

#define kLogMessage	"This is a test of the emergency broadcast system"

static void
TestBase64()
{
	char* base64;
	uchar* data;
	int count;
	
	Message(("Testing Base64..."));
	base64 = NewBase64String("Aladdin:open sesame", "TestBase64");
	Assert(strcmp(base64, "QWxhZGRpbjpvcGVuIHNlc2FtZQ==") == 0);
	data = NewFromBase64(base64, &count, "TestBase64");
	Assert(count == strlen("Aladdin:open sesame"));
	Assert(strcmp("Aladdin:open sesame", (const char*)data) == 0);
	FreeTaggedMemory(base64, "TestBase64");
	FreeTaggedMemory(data, "TestBase64");
}

static void
TestUtilities()
{
	Message(("Testing Utilities (1/5): DataList..."));
	DataList::Test();

	Message(("Testing Utilities (2/5): ObjectList..."));
	ObjectList::Test();

	Message(("Testing Utilities (3/5): StringList..."));
	StringList::Test();

	Message(("Testing Utilities (4/5): URLParser..."));
	URLParser::Test();

	Message(("Testing Utilities (5/5): DateTimeParser..."));
	DateTimeParser::Test();

	Message(("Testing cache..."));
	Cache::TestLocking();
	
	Message(("Testing client functions..."));
	gPageViewer->ExecuteURL("client:test?John=Matheny&Company=Artemis+Research");

	Message(("Testing logging"));
	SendLog(kLogTest, kLogMessage, strlen(kLogMessage), __FILE__, __LINE__);

	Message(("Testing CookieList..."));
	CookieList::Test();

	TestBase64();
	Message(("Done testing utilities."));

	Message(("Testing Authorizations..."));
	AuthorizationList::Test();

	Message(("Testing ServiceList..."));
	gServiceList->Test();
}

static void 
TestSecurity()
{
	unsigned int start, end, time;

	Message(("Test RC4"));
	RC4_Test();
	
	Message(("Test DES"));
	DES_Test();
	
	Message(("Test RSA"));
	start = Now();
	RSA_Test();
	end = Now();
	
	if (end > start)
		time = (end - start) / 60;
	else
		time = ((0xFFFFFFFF - start) + end) / 60;

	printf("total RSA time : %d seconds\n", time);


}




static Boolean
Splattered(BitMapDevice *dst,const Rectangle *drawBounds,BitMapDevice *src)
{

#ifdef	DEBUG
	VerifyMemory();
#endif
	if ( AverageImage(*src, 0,&src->bounds,true,1) == (Color)-1 ) {
		Message(("pixels changed in src"));
		return true;
	}

	// check for splatter
	
	Rectangle changedArea = *drawBounds;
	
	// one pixel slop because u and v get blended on half word transfers
	
	changedArea.right += 1;			
	changedArea.left -= 1;	
	PaintRectangle(*dst, changedArea, 0);
	return ( AverageImage(*dst, 0,&dst->bounds,true,1) == (Color)-1 );
	
}

static Boolean 
GraphicsTest(BitMapDevice *src,BitMapDevice *dst,Boolean shortTest,Boolean doSrcIndependantTests);

static Boolean 
GraphicsTest(BitMapDevice *src,BitMapDevice *dst,Boolean shortTest,Boolean doSrcIndependantTests)
{
	Color transparency = 0;
	Color color;
	Rectangle	srcRect,dstRect,drawBounds;
	int	i;
	BitMapDevice *tb;
	const colorTolerance = 40;		// because rgb->yuv->rgb blue is off by this much ( need to check the coeffecients again )
	Boolean	canResize = true;
	
	if ( src->format == yuv422Format ) {
		tb = NewThumbnail(src,8);
		if ( tb == nil ) {
			Message(("NewThumbnail failed"));
			goto failed;
		}
		DeleteBitMapDevice(tb);
		if ( !shortTest ) {
			tb = NewThumbnail(src,1);
			if ( tb == nil ) {
				Message(("NewThumbnail big failed"));
				goto failed;
			}
			DeleteBitMapDevice(tb);
		}
	}
	
	
	// clear to black, make sure no graphics ops write outside of drawBounds
	
	ClearBitMapDevice(src);
	if ( AverageImage(*src, 0,&src->bounds,true,1) != 0 ) {
		Message(("AverageImage on src failed"));
		goto failed;
	}
	PaintRectangle(*dst, dst->bounds, 0);		
	if ( AverageImage(*dst, 0,&dst->bounds,true,1) != 0 ) {
		Message(("AverageImage on dst failed"));
		goto failed;
	}
	
	if ( src->format == vqFormat )
		canResize = false;
	drawBounds = dst->bounds;
	
	InsetRectangle(drawBounds,4,4);
	for ( i=0; i < 7; i++ ) {
		srcRect = src->bounds;
		dstRect = drawBounds;
		switch ( i ) { 
		case 0:
			break;
		case 1:
			if ( !canResize )
				continue;
			InsetRectangle(srcRect,7,7);
			break;
		case 2:
			if ( !canResize )
				continue;
			InsetRectangle(dstRect,7,7);
			break;
		case 3:
			InsetRectangle(srcRect,7,7);
			InsetRectangle(dstRect,7,7);
			break;
		case 4:
			if ( shortTest )
				return 0;
			if ( !canResize )
				continue;
			SetRectangle(srcRect,0,0,1,1);
			break;
		case 5:
			if ( !canResize )
				continue;
			dstRect.right = dstRect.left+1;
			dstRect.bottom = dstRect.top+1;
			break;
		case 6:
			SetRectangle(srcRect,3,3,4,4);
			dstRect.right = dstRect.left+1;
			dstRect.bottom = dstRect.top+1;
			break;
		}
		for  ( color=0; color < 0xffffff; color = (color << 8) + 0xff  ) {
			for  ( transparency=0; transparency <= 256; transparency += 128 ) {
				if ( transparency == 256 )
					transparency = 255;
				PaintRectangle(*dst, dstRect, color);
				if ( AverageImage(*dst, color,&dstRect,true,colorTolerance) == (Color)-1 )  {
					Message(("PaintRectangle failed"));
					goto failed;
				}
				
				if ( doSrcIndependantTests ) {
					PaintAntiBevel(*dst, dstRect,&dstRect, transparency);
					if ( Splattered(dst,&drawBounds,src) ) {
						Message(("PaintAntiBevel drew outside of clip"));
						goto failed;
					}
					PaintBevel(*dst, dstRect, &dstRect,transparency);
					if ( Splattered(dst,&drawBounds,src) ) {
						Message(("PaintBevel drew outside of clip"));
						goto failed;
					}
					PaintRectangle(*dst, dstRect, color,transparency,&dstRect);
					if ( Splattered(dst,&drawBounds,src) ) {
						Message(("PaintRectangle drew outside of clip"));
						goto failed;
					}
					PaintLine(*dst, dstRect.left, dstRect.top, dstRect.right, dstRect.top, 1,  
						color, transparency,&dstRect);
					if ( Splattered(dst,&drawBounds,src) ) {
						Message(("PaintLine drew outside of clip"));
						goto failed;
					}
					PaintLine(*dst, dstRect.left, dstRect.top, dstRect.left, dstRect.bottom, 1,  
						color, transparency,&dstRect);
					if ( Splattered(dst,&drawBounds,src) ) {
						Message(("PaintLine 2 drew outside of clip"));
						goto failed;
					}
				}
				
				CopyImage(*src,*dst,srcRect,dstRect,transparency,&dstRect);
				if ( Splattered(dst,&drawBounds,src) ) {
					Message(("CopyImage 1 drew outside of clip"));
					goto failed;
				}
				CopyImage(*src,*dst,srcRect,dstRect,transparency,&dstRect,true);
				if ( Splattered(dst,&drawBounds,src) ) {
					Message(("CopyImage 2 drew outside of clip"));
					goto failed;
				}
				CopyImage(*src,*dst,srcRect,dstRect,transparency,&dstRect,false,true);
				if ( Splattered(dst,&drawBounds,src) ) {
					Message(("CopyImage 3 drew outside of clip"));
					goto failed;
				}
				CopyImage(*src,*dst,srcRect,dstRect,transparency,&dstRect,true,true);
				if ( Splattered(dst,&drawBounds,src) ) {
					Message(("CopyImage 4 drew outside of clip"));
					goto failed;
				}
				CopyImage(*src,*dst,srcRect,dstRect,transparency,&dstRect,false,false,true);
				if ( Splattered(dst,&drawBounds,src) ) {
					Message(("CopyImage 5 drew outside of clip"));
					goto failed;
				}
				if ( doSrcIndependantTests ) {
					CopyImage(*dst,*dst,srcRect,dstRect,transparency,&dstRect);
					if ( Splattered(dst,&drawBounds,src) ) {
						Message(("CopyImage 6 drew outside of clip"));
						goto failed;
					}
					CopyImage(*dst,*dst,srcRect,dstRect,transparency,&dstRect,false,false,true);
					if ( Splattered(dst,&drawBounds,src) ) {
						Message(("CopyImage 7 drew outside of clip"));
						goto failed;
					}
					{
						XFont font;
						const char *kTextTestString = "\n\tToyomi-chan ga dai suki desu yo!";
						const kFontSizes[8] = { 10,12,14,16,24,32,40,52 };
						int j;
						CharacterEncoding encoding;
						int face,style;
		
						for ( j=0; j < 256; j++ ) {
							if ( j & 0x80 )
								encoding = kSJIS;
							else
								encoding = kUnknownEncoding;
							if ( j & 0x40 )
								face = kHelvetica;
							else
								face = kMonaco;
							switch ( (j>>3) & 0x7 ) {
							case 1:
								style = kNormalStyle;
								break;
							case 2:
								style = kBoldStyle;
								break;
							case 3:
								style = kOutlineStyle;
								break;
							case 4:
								style = kShadowStyle;
								break;
							case 6:
								style = kReliefStyle;
								break;
							case 7:
								style = kEmbossStyle + kBoldStyle;
								break;
							}
							
							font = GetFont(face, kFontSizes[j & 7], style);
							GetFontAscent(font,encoding);
							GetFontDescent(font,encoding);
							GetFontLeading(font,encoding);
							TextMeasure(*dst,font,encoding,kTextTestString, sizeof(kTextTestString)-1);
							PaintText(*dst,font,encoding,kTextTestString, sizeof(kTextTestString)-1, color, dstRect.left, 
								dstRect.bottom,transparency,true,&dstRect);
							if ( Splattered(dst,&drawBounds,src) ) {
								Message(("PaintText %d drew outside of clip",j));
								goto failed;
							}
							if ( shortTest && j == 0x10 )
								break;
						}
					}
				}
			}
		}
	}	
	return 0;
failed:
	return 1;
}
void
TestGraphics();

void
TestGraphics()
{
	
	BitMapDevice *srcBitMap = nil,*dstBitMap = nil;
	Color transparent;
	CLUT *colorTable = nil;
	Gamma gamma;
	int	i;
	Boolean shortTest = false;
	
	const kSrcFormats = 7;
	static const BitMapFormat srcFormats[kSrcFormats] = {
		yuv422Format,
		index4Format,
		index8Format,
		alpha8Format,
		vqFormat,	
		antialias8Format,
		antialias4Format
	};
	
	short r,g,b;
	uchar dr,dg,db;
	short y,u,v;
	
	
	Message(("Testing graphics software"));
	for ( r =0; r < 256; r += 16 ) {
		for ( g =0; g < 256; g += 16 ) {
			for ( b =0; b < 256; b += 16 ) {
				rgbtoyuv(r,g,b,&y,&u,&v);
				yuvtorgb(y,u,v,&dr,&dg,&db);
				if ( ABS(dr-r) > 32 )
					goto fail;
				if ( ABS(dg-g) > 24 )
					goto fail;
				if ( ABS(db-b) > 40 )
					goto fail;
			}
		}
	}
	for ( gamma = 1; gamma < 3<<16; gamma += 0x8000 ) {
		colorTable = NewColorTable(kYUV24,256);
		if ( colorTable == nil )
			goto fail;
		if ( BuildGammaTable(gamma, 10, 200,kBlackY,kWhiteY) == nil )  {
			DeleteColorTable(colorTable);	
			goto fail;
		}
		GammaCorrect(colorTable,gamma, 0, 255);
		DeleteColorTable(colorTable);	
		colorTable = nil;
	}

	Rectangle rect;
	SetRectangle(rect,0,0,32,32);
	dstBitMap = NewBitMapDevice(rect,yuv422Format);
	if ( IsError(dstBitMap == nil) ) {
		goto fail;
	}

	transparent = 0;
	for ( i=0; i < kSrcFormats; ) {
		if ( i > 0 )
			transparent = 0;
		if ( srcFormats[i] == alpha8Format )
			colorTable = NewColorTable(kYUV32,256);
		srcBitMap = NewBitMapDevice(rect,srcFormats[i],colorTable,transparent);
		if ( IsError(srcBitMap == nil) ) {
			goto fail;
		}
		Message(("Test graphics src format %X transparent %X",(int)srcFormats[i],(int)transparent));
		if ( GraphicsTest(srcBitMap,dstBitMap,shortTest,i == 0 && transparent == 0 ) ) 
			goto fail;
		DeleteBitMapDevice(srcBitMap);
		srcBitMap = nil;
		if ( !shortTest && i== 0 && transparent == 0 ) {
			transparent = kTransparent;
			continue;
		}
		i++;
		if ( shortTest && i == 3 )
			break;
	}
	Message(("Graphics test completed"));
	return;
	
fail:
	if ( srcBitMap )
		DeleteBitMapDevice(srcBitMap);
	if ( dstBitMap )
		DeleteBitMapDevice(dstBitMap);
	Message(("!!! Graphics test failed"));
	
}



static void
TestAll(void)
{
	Message(("Executing all tests..."));
	
#if defined(FOR_MAC)
	gMacSimulator->StartProfiling();
#endif
	TestMemoryManager();
	TestObjectStore();
	TestUtilities();
	TestGraphics();
	Message(("All tests complete."));
#if defined(FOR_MAC)
	gMacSimulator->StopProfiling();
	gMacSimulator->ForceUpdateAll();
#endif	
}

void
ExecuteTest(ulong testNumber)
{
	switch (testNumber)
	{
		case 1: TestAll(); break;
		case 3: TestMemoryManager(); break;
		case 4: TestObjectStore(); break;
		case 5: TestUtilities(); break;
		case 6: break;
		case 7: /* TestTCP(); */ break;
		case 8: TestSecurity(); break;
		case 9: TestGraphics(); break;
		default: Trespass();
	}
	
	SystemBeep(20);
}





#endif /* TEST_SIMULATORONLY */