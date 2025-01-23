// ===========================================================================
//	TourGuide.c
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#include "Headers.h"

#ifndef __ALERTWINDOW_H__
#include "AlertWindow.h"
#endif
#ifndef __DOCUMENT_H__
#include "Document.h"
#endif
#ifndef __MACINTOSHUTILITIES_H__
#include "MacintoshUtilities.h"
#endif
#ifndef __MEMORYMANAGER_H__
#include "MemoryManager.h"
#endif
#ifndef __PAGEVIEWER_H__
#include "PageViewer.h"
#endif
#ifndef __TEWINDOW_H__
#include "TEWindow.h"
#endif
#ifndef __TOURGUIDE_H__
#include "TourGuide.h"
#endif




// ----------------------------------------------------------------------------
//  globals and constants
// ----------------------------------------------------------------------------

#define PROFILE_TOUR	1	// define this to profile a tour
TourGuide gTourGuide;

#ifdef PROFILE_TOUR
#ifndef __MACSIMULATOR_H__
#include "MacSimulator.h"
#endif
#endif

#ifdef FOR_MAC
extern void SetStartTourGuideMenu(Boolean setToStart); // "Functions.c"
#endif
// ----------------------------------------------------------------------------

Boolean
Tour::Load(const Document* document)
{
	Clear();
	if (document == nil)
	{
		Complain(("Can't load a tour from a nil document"));
		return false;
	}
	
	char*	url = document->CopyURL("Tour::Load");
	fParser.SetURL(url);
	FreeTaggedMemory(url, "Tour::Load");
	
	long numSelectables = document->GetSelectableCount(0);

	for (Selection	s = {0, 0}; s.fSelectableIndex < numSelectables; s.fSelectableIndex++)
	{
		const Displayable *selectable = document->GetSelectable(s);

		if (selectable->HasURL())
		{
			char* HREF = selectable->NewURL(nil, "Tour::Load");
			if (HREF != nil)
			{					
				char* URL = fParser.NewURL(HREF, 0);
			
				if (URL != nil)
				{
					if (!EqualStringN(URL, "mailto", 6))
					{	
						if (!Find(URL))		// if it's not already on our list of stuff
							Add(URL);
					}
					FreeTaggedMemory(URL, 0);
				}
				FreeTaggedMemory(HREF, "Tour::Load");
			}
		}
	}
	return true;
}

// ----------------------------------------------------------------------------

TourGuide::TourGuide()
{
	fActive = false;
	fTourLoaded = false;
	fLookTime = kLookTime;
	fTimeOut = kTimeOut;
	fDumpFile = nil;
	fDumpFilename = nil;
}

TourGuide::~TourGuide()
{
	if (fActive)
	{
		Stop();
		Reset();
	}
}

ulong
TourGuide::GetPageCount() const
{
	Assert(fTourLoaded && fActive);
	return fTour.GetCurrentIndex();
}

// ---------------------------------------- actions: Load/Start/Stop/Print/Idle
Boolean
TourGuide::Load()
{
	Reset();	// just to make sure we're clean
	
	if (gPageViewer == nil)
	{
		Complain(("TourGuide sees no active PageViewer"));
		return false;
	}
	
	Document *document = gPageViewer->GetDocument();
	if (document == nil)
	{
		Complain(("TourGuide has no initial document to start from"));
		return false;
	}
	
	const char* filename = GenerateDumpFilename("WebTV-Warrior TourGuide ");

	fDumpFile = nil;
	
	if (filename == nil)
	{
		Complain(("TourGuide cannot generate dumpfile name"));
	}
	else
	{	
		fDumpFile = fopen(filename, "w");
		if (fDumpFile != nil)
		{	
			fDumpFilename = (char*)NewSimulatorMemory(strlen(filename)+1);
			if (fDumpFilename != nil)
			{	strcpy(fDumpFilename, filename);
			}
		}
	}
	if (fDumpFile == nil)
	{
		
		Complain(("TourGuide cannot open '%s' as dumpFile", filename));
		return false;
	}
	
	// ------------- now we're committed to loading the tour
	
	fRootResource = *document->GetResource();
	
	char* docURL = document->CopyURL("TourGuide::Load");
	fprintf(fDumpFile, "This file is saved as: \"%s\"\n", fDumpFilename);
	fprintf(fDumpFile, "TourGuide tours the URL '%s' (%s; %s)\n",
			docURL, TimeFunction(), DateFunction());
	FreeTaggedMemory(docURL, "TourGuide::Load");

	fPagesVisited = 0;
	fPagesSkipped = 0;
	fPagesTotal = 0;

	fTour.Load(document);
	fTourLoaded = !fTour.AtEnd();
	fTour.Print(fDumpFile);
	return true;
}

void
TourGuide::Start()
{
	Assert(!fActive);

	if (fActive)			// if it was already running...
	{
		Complain(("TourGuide called while another tour was already running..."));
		return;
	}
	
	if (!fTourLoaded)
	{	
		Complain(("No tour loaded for TourGuide to take"));
		return;
	}
	GetFirst();
	gPageLapTimer.Start();
	gIdleLapTimer.Start();
	fActive = true;
#if defined __MWERKS__ && !defined FOR_MPW && __profile__ == 1 && defined(PROFILE_TOUR)
	fIsProfiling = false;
#endif // defined __MWERKS__ && !defined FOR_MPW && __profile__ == 1 && defined(PROFILE_TOUR)
}

void
TourGuide::Stop()
{
	if (!fActive)
		return;
	
#if defined __MWERKS__ && !defined FOR_MPW && __profile__ == 1 && defined(PROFILE_TOUR)
	if (fIsProfiling)
	{
		gMacSimulator->StopProfiling();
	}
#endif // defined __MWERKS__ && !defined FOR_MPW && __profile__ == 1 && defined(PROFILE_TOUR)
	
	gPageLapTimer.Stop();
	gIdleLapTimer.Stop();

	if (fTour.AtEnd())
	{
		fprintf(fDumpFile, "%s> End of tour.\n", TimeFunction());
		if (fRootResource.HasURL())
		{
			gPageViewer->ShowResource(&fRootResource);	// return to beginning of tour
		}
	}
	else
	{
		fprintf(fDumpFile, "%s> Tour aborted.\n", TimeFunction());
	}

	fActive = false;

	Print();
	fclose(fDumpFile);
	fDumpFile = nil;
	
	if (fDumpFilename != nil)
	{
#ifdef FOR_MAC
		TEWindow* tw = newMac(TEWindow);
		if (tw)
		{
			tw->SetTitle(fDumpFilename);
			tw->ResizeWindow(300, 200);
			tw->ReadFromFile(fDumpFilename);
			tw->ShowWindow();
		}
#endif // #ifdef FOR_MAC
	}
	Reset();
	return;
}

void
TourGuide::Print(FILE* fp)
{
	if (fp == nil)
	{
		fp = fDumpFile;
	}
	
	fprintf(fp, "TourGuide visited %d, skipped %d, out of %d total\n",
				fPagesVisited, fPagesSkipped, fPagesTotal);

								fprintf(fp, "\n");				
	gIdleLapTimer.Print(fp);
								fprintf(fp, "\n");
	gPageLapTimer.Print(fp);
								fprintf(fp, "\n");
#if defined(LOG_ALL_TAGS_AND_ATTRIBUTES)
	gKnownTagLog.Print(fp);
								fprintf(fp, "\n");
	gAttributeLog.Print(fp);
								fprintf(fp, "\n");
#endif // defined(LOG_ALL_TAGS_AND_ATTRIBUTES)
#if defined(LOG_BAD_TAGS_AND_ATTRIBUTES)
	gBadAttributeLog.Print(fp);
								fprintf(fp, "\n");
	gUnknownTagLog.Print(fp);
								fprintf(fp, "\n");
	gUnhandledTagLog.Print(fp);
								fprintf(fp, "\n");
#endif // defined(LOG_BAD_TAGS_AND_ATTRIBUTES)
}

void
TourGuide::AddMessage(const char* message)
{
	if (fActive)
	{
		fprintf(fDumpFile, "\t%s> %s\n", TimeFunction(), message);
	}
}

void
TourGuide::Idle()
{
	if (!fActive)
		return;
	
	gIdleLapTimer.Lap();

	if (gAlertWindow->IsVisible() && !fDocumentLoaded)
	{
#ifdef DEBUG
		fprintf(fDumpFile, "%s> *** TourGuide saw gAlertWindow (%s) => skipped URL '%s'\n",
							TimeFunction(), gAlertWindow->GetMessage(), fTour.GetCurrent());
#endif
		fDocumentLoaded = true;
		fLookTime = Now() + kAlertLookTime;
	}

	if (gPageViewer == nil)
		return;
	
	if (fDocumentLoaded)
	{
		if (Now() > fLookTime)
		{
			if (gAlertWindow->IsVisible())
				gAlertWindow->Hide();
			gPageLapTimer.Resume();
			gPageLapTimer.Lap();
			GetNext();
			fDocumentLoaded = false;
		}
	}
	else
	{
		if (Now() > fTimeOut)		// if we've waited too long, just give up
		{
			fprintf(fDumpFile, "%s> *** Skipped URL '%s'\n", TimeFunction(), fTour.GetCurrent());
			fPagesSkipped++;	// we've now "officially" skipped this page
			gPageLapTimer.Lap();
			GetNext();
		}
		else if (gPageViewer->Completed())
		{
			fDocumentLoaded = true;
			fPagesVisited++;		// we've now "officially" visited this page
			fprintf(fDumpFile, "%s> Got URL '%s'\n", TimeFunction(), fTour.GetCurrent());
			fLookTime = Now() + kLookTime;	// wait for a bit to just stare at this page
			gPageLapTimer.Pause();
		}
	}
}		


void TourGuide::Reset()
{
	Assert(!fActive);
	
	if (fDumpFile != nil)
	{
		fclose(fDumpFile);
		fDumpFile = nil;
	}
	
	if (fDumpFilename != nil)
	{
		DisposeSimulatorMemory(fDumpFilename);
		fDumpFilename = nil;
	}

	fTour.Clear();
}


void
TourGuide::OpenURL(void)
{
	Assert(gPageViewer);

	if (fTour.AtEnd())
	{
		Stop();
		return;
	}
	
	fprintf(fDumpFile, "%s> Opening URL '%s'\n", TimeFunction(), fTour.GetCurrent());
	fflush(fDumpFile);	// make sure this gets written to disk...

	gPageViewer->ExecuteURL(fTour.GetCurrent(), nil);
	fTimeOut = Now() + kTimeOut;	// just give up if it takes longer than this
	fDocumentLoaded = false;					// surely it ain't loaded!
}

void
TourGuide::GetFirst(void)
{
#if defined __MWERKS__ && !defined FOR_MPW && __profile__ == 1 && defined(PROFILE_TOUR)
	if (!fIsProfiling)
	{
		fIsProfiling = true;
		gMacSimulator->StartProfiling();
	}
#endif // defined __MWERKS__ && !defined FOR_MPW && __profile__ == 1 && defined(PROFILE_TOUR)
	fTour.GetFirst();
	OpenURL();
}

void
TourGuide::GetNext(void)
{
	fTour.GetNext();
	OpenURL();
}

