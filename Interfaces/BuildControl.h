#ifndef __BUILDCONTROL_H__
#define __BUILDCONTROL_H__

/* Cannot specify command line options in CodeWarrior,
 * so add them in here.
 */

#ifndef HARDWARE
	#define REALAUDIO
	#define SIMULATOR
	#define WEBTV
	#define FOR_MAC
	#define PRERELEASE
	#define SUPPORTS_BITFIELDS
	#define IGOR_MIDI
	#define	APPROM
	#if GENERATING68K
		#undef IGOR_MIDI
	#endif
	#if defined(DEBUG)
		#define MEMORY_TRACKING
		#define BRUCES_UGLY_DIALING_HACK
	#endif
#endif

//#ifdef EXTERNAL_BUILD
//	#undef FOR_MAC
//	#undef MEMORY_TRACKING
//	#undef SIMULATOR
//#endif

/*
 * DEBUG and TEST flags
 */

#ifdef DEBUG
	#define DEBUG_BOXPRINT
	#define DEBUG_CACHE_SCRAMBLE
	#define DEBUG_CACHE_VALIDATE
	#define DEBUG_CACHE_VIEWASHTML
	#define DEBUG_CLASSNUMBER
	#define DEBUG_DISPLAYABLE
	#define DEBUG_MONKEY
	#define DEBUG_NAMES
	#define DEBUG_PERFDUMP
	#define DEBUG_TOURIST
	#define TEST_CLIENTTEST

	#ifdef SIMULATOR
		/* The following don't really _need_ DEBUG on */
		#define DEBUG_CACHEWINDOW
		#define DEBUG_DISPLAYABLEWINDOW
		#define DEBUG_LEDWINDOW
		#define DEBUG_MISCSTATWINDOW
		#define DEBUG_SERVICEWINDOW
		#define DEBUG_SOCKETWINDOW
		#define DEBUG_TOURISTWINDOW
		#define DEBUG_SIMULATORONLY
		#define INCLUDE_FINALIZE_CODE
		#define TEST_SIMULATORONLY
	
		/* these require MEMORY_TRACKING */
		#ifdef MEMORY_TRACKING
			#define DEBUG_MEMORYSEISMOGRAPH
			#define DEBUG_MEMORYWINDOW
		#endif
	#endif
#endif

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include BuildControl.h multiple times"
	#endif
#endif /* __BUILDCONTROL_H__ */
