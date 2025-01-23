// ===========================================================================
//	Tourist.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __TOURIST_H__
#define __TOURIST_H__

#ifdef DEBUG_TOURIST

#ifndef __LIST_H__
#include "List.h"			/* for StringDictionary */
#endif
#ifndef __UTILITIES_H__
#include "Utilities.h"		/* for HasDebugModifiedTime */
#endif

// ===========================================================================

class Document;
class Stream;
class StringDictionary;
class Tourist;
class TouristSite;



// ===========================================================================

struct TouristSite : public Listable {

	protected:
								TouristSite();
								~TouristSite();

	public:
		static TouristSite*		NewTouristSite(const char* url = nil, long depth = 0);
		static void				DeleteTouristSite(TouristSite*& tourSite);
		
		
		TouristSite*			Duplicate();
		
		long					GetDepth() const { return fDepth; };
		long					GetRetries() const { return fRetries; };
		const char*				GetComment() const { return fComment; };
		const char*				GetURL() const { return fURL; };
		Boolean					GetVisited() const { return fVisited; };
		
		void					SetDepth(long depth) { fDepth = depth; };
		void					SetRetries(long retries) { fRetries = retries; };
		void					SetComment(const char* comment);
		void					SetURL(const char* URL);
		void					SetVisited(Boolean visited) { fVisited = visited; };
	
	protected:
		void			Set(const char* string);

	protected:
		long			fDepth;
		long			fRetries;
		char*			fComment;
		char* 			fURL;
		Boolean			fVisited;
};

// ===========================================================================

class TouristSiteList : public ObjectList {
	public:
		TouristSiteList() : fTotalSites(0), ObjectList() {};
		virtual 			~TouristSiteList();

		void				Add(const Listable* item);
		Boolean				Contains(TouristSite* tourSite);
		Boolean				Contains(const char* url);
		void				DeleteAll(void);
#ifdef _INCLUDE_DEAD_CODE_	// currently unused: this is never defined
		long				GetTotalSites() const { return fTotalSites; };
		void				SetTotalSites(long total) { fTotalSites = total; };
#endif
		virtual void		WriteToStream(Stream* stream);

	protected:
		long				fTotalSites;
};

class TouristSiteListIterator : public ObjectIterator {};

// ===========================================================================

class TouristJournal : public TouristSiteList {
	public:
						enum { kMonkeyMaxSites = 5 };
						enum { kDefaultMaxSites = 5 };
						TouristJournal() : fMaxSites(kDefaultMaxSites), TouristSiteList() {};

		long			GetMaxSites() { return fMaxSites; };
		void			SetMaxSites(Boolean maxSites) { fMaxSites = maxSites; };

		void			Add(const Listable* item);

	protected:
		long			fMaxSites;
};

class TouristItinerary : public TouristSiteList {
	public:
						enum { kMonkeyMaxBufferedSites = 50 };
						enum { kMonkeyMaxTotalSites = 0 };
						enum { kDefaultMaxBufferedSites = 50 };
						enum { kDefaultMaxTotalSites = 0 };
						TouristItinerary() : fMaxBufferedSites(kDefaultMaxBufferedSites),
											 fMaxTotalSites(kDefaultMaxTotalSites),
											 TouristSiteList() {};

		long			GetMaxBufferedSites()			{ return fMaxBufferedSites; };
		long			GetMaxTotalSites()				{ return fMaxTotalSites; };
		void			SetMaxBufferedSites(long max)	{ fMaxBufferedSites = max; };
		void			SetMaxTotalSites(long max)		{ fMaxTotalSites = max; };

		void			Add(const Listable* item);
		void			AddDocument(Document* document, long depth,
									Selection selection);

	protected:
		long			fMaxBufferedSites;
		long			fMaxTotalSites;
};

class TouristJournalIterator : public TouristSiteListIterator {};
class TouristItineraryIterator : public TouristSiteListIterator {};

// ===========================================================================

typedef enum {
	kTouristStateNone,
	kTouristStateInitialized,
	kTouristStatePaused,
	kTouristStateWaitCompleted,
	kTouristStateScrollDown,
	kTouristStateScrollUp,
	kTouristStateWaitShow,
	kTouristStateWaitMIDI,
	kTouristStateGiveUpSite,
	kTouristStateFinishSite,
	kTouristStateLoad,
	kTouristStateAlmostDone,
	kTouristStateDone
} TouristState;

#ifdef DEBUG_NAMES
const char* GetTouristStateString(TouristState touristState);
#endif

// ===========================================================================

class Tourist : public HasDebugModifiedTime {
	protected:
							Tourist();
							~Tourist();
	
	public:
		static void			Initialize();
		static void			Finalize();
		static void			Execute(const StringDictionary*);
		static void			ShowHelp();
		void				Idle();
		void				Start();
		void				Stop();
		void				Pause();
		void				Resume();
		void				Skip();
		
		void				Show();
		void				WriteToStream(Stream* stream);

		static long			GetDefaultDepth();
		static long			GetDefaultLoadTime();
		static long			GetDefaultMaxRetries();
		static long			GetDefaultMIDITime();
		static Boolean		GetDefaultScrollThrough();
		static long			GetDefaultShowTime();
		Boolean				GetIsActive() const			{ return fTouristState != kTouristStateAlmostDone && fTouristState != kTouristStateDone && fTouristState != kTouristStateNone && fTouristItinerary != nil; };
		long				GetLoadTime() const			{ return fLoadTime; };
		long				GetMaxRetries() const		{ return fMaxRetries; };
		long				GetMIDITime() const 		{ return fMIDITime; };
		Boolean				GetPaused() const			{ return fTouristState == kTouristStatePaused; };
		ulong				GetPageCount() const;
		Boolean				GetScrollThrough() const	{ return fScrollThrough; };
		long				GetShowTime() const			{ return fShowTime; };
		TouristItinerary*	GetTouristItinerary() const	{ return fTouristItinerary; };
		TouristJournal*		GetTouristJournal() const	{ return fTouristJournal; };
		TouristSite*		GetTouristSite() const		{ return fTouristSite; };
		TouristState		GetTouristState() const		{ return fTouristState; };
		
		TouristItinerary*	RemoveTouristItinerary();
		TouristJournal*		RemoveTouristJournal();
		TouristSite*		RemoveTouristSite();
		
		static void			SetDefaultDepth(long depth);
		static void			SetDefaultLoadTime(long loadTime);
		static void			SetDefaultMaxRetries(long maxRetries);
		static void			SetDefaultMIDITime(long MIDITime);
		static void			SetDefaultScrollThrough(Boolean scrollThrough);
		static void			SetDefaultShowTime(long showTime);
		void				SetLoadTime(long loadTime);
		void				SetMaxRetries(long retries);
		void				SetMIDITime(long MIDITime);
		void				SetScrollThrough(Boolean flag);
		void				SetShowTime(long showTime);
		void				SetTouristItinerary(TouristItinerary*);
		void				SetTouristJournal(TouristJournal*);
		void				SetTouristSite(TouristSite*);
		void				SetTouristState(TouristState);
	
	protected:
		time_t				fCheckpoint;
		long				fLoadTime;
		long				fMIDITime;
		long				fShowTime;
		long				fMaxRetries;
		char*				fOldURL;

		TouristItinerary*	fTouristItinerary;
		TouristJournal*		fTouristJournal;
		TouristSite*		fTouristSite;
		TouristState		fTouristState;

		Boolean				fScrollThrough;
};

extern Tourist* gTourist;

// ===========================================================================

#endif /* DEBUG_TOURIST */

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Tourist.h multiple times"
	#endif
#endif /* __TOURIST_H__ */
