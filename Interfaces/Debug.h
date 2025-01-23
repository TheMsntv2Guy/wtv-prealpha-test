// ===========================================================================
//	Debug.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifndef __BOXANSI_H__
#include "boxansi.h"
#endif

/*
 *
 * The following drops into the debugger after printing the message.
 * It does nothing in no-debug builds.  It requires double parentheses
 * so that we can do the proper work with the C preprocessor, which cannot
 * handle variable numbers of arguments to a macro.
 *
 *		Complain((message))			-	Always stop in debugger
 *
 * The following checks the assertion at runtime,
 * and drops in the debugger if it is false.
 * Assertions are not evaluated in no-debug versions.
 *
 *		Assert(assertion)			-	Complain if the assertion is false
 *
 * The following checks the postulation at build time.
 * It does nothing in no-debug builds (though it could, if we trusted
 * the implementation to do know code generation.
 *
 *		Postulate(postulation)		-	Do not build if the postulation is false
 *
 * The following drops into the debugger if ever it is reached.
 * It is non-existent in no-debug builds.  It should be placed in
 * code that should never be reached.
 *
 *		Trespass()					-	Complain if reached
 *
 * The following tracks stored pointers, so that they do not slip out from underneath
 *
 *		void NoteObserving(void* pointee, void* pointer, char* fieldName)
 *		int EachObservation(ObservingFunction function, void* parameters)
 *
 */
 
#define _void					((void)0)

#ifdef DEBUG
	#define DebugCode(a) a
	#if defined applec && defined SIMULATOR
		#define DebugBreakStr(x)	DebugBreak()
		pascal void DebugBreak(void) = 0xA9FF; 
	#elif defined GCC
		#define DebugBreakStr(x)		DebugMsg("Debugger")
		#define DebugBreak()			Debugger
	#elif defined FOR_MAC
		#if GENERATING68K
			#define DebugBreakStr(x)		SysBreakStr(x)
			#define DebugBreak()			SysBreakStr("\pDebugger")
		#else
			#define DebugBreakStr(x)		DebugStr(x)		
			#define DebugBreak()			DebugStr("\pDebugger")
		#endif
	#else
		#define DebugBreakStr(x)	printf("Hey, you called DebugBreakStr(%s)!\n", x);
		#define	DebugBreak()		printf("Hey, you called DebugBreak()!\n");
	#endif
	
	#ifdef FOR_MAC
		#define kSafeAddress			0x40800000
	#elif defined HARDWARE
		#define kSafeAddress			0xBFC00000
	#else
		#error "no safe address defined for debugging"
	#endif
	
	#ifdef FOR_MAC
		void								PrintMessage(const char *format, ...);
		void								PrintImportantMessage(const char *format, ...);
		void								PrintTrivialMessage(const char *format, ...);
		#define Message(message)			PrintMessage message
		#define ImportantMessage(message)	PrintImportantMessage message
		#define TrivialMessage(message)		PrintTrivialMessage message
	#endif
	
	extern Boolean gPreventDebuggerBreaks;

	#ifdef SIMULATOR
		void								PrologMessage(const char *file, int lineNumber);
		void								LogMessage(const char *format, ...);
		void								VLogMessage(const char *format, va_list list);
		void								RawLogMessage(const char *format, ...);
		void								VRawLogMessage(const char *format, va_list list);
		void								PrologComplaint(const char *file, int lineNumber);
		unsigned char *						PrepareComplaint(const char *format, ...);
		unsigned char *						VPrepareComplaint(const char *format, va_list list);
		FILE*								OpenLogFile(void);
		FILE*								GetLogFile(void);
		void								FlushLogFile(void);
		void								CloseLogFile(void);

		#define Log(message)				{ PrologMessage(__FILE__, __LINE__); LogMessage message; }
		#define RawLog(message)				RawLogMessage message
		#define Complain(message)			( LogMessage message, \
											  PrologComplaint(__FILE__, __LINE__), \
											  (gPreventDebuggerBreaks \
											  		? ((void)0) \
											  		: DebugBreakStr(PrepareComplaint message)), \
													  ((void)0))	
		#define DebugMsg(message)			_void					
	#else
		#ifdef __cplusplus
		extern "C" {
		#endif
		void CallDebugger(void);
		#ifdef __cplusplus
		}
		#endif
		#define Log(message)				_void
		#define RawLog(message)				_void
		#define Complain(message)			DebugMsg(message)
		#define Message(message)			printf message,printf("\n")
		#define LogMessage					printf
		#define ImportantMessage(message)	printf message,printf("\n")
		#define TrivialMessage(message)		_void
	#if defined(GOOBER) || defined(BOOTROM)
		#define DebugMsg(message)			printf message,(void) printf("\n")
	#else
		#define DebugMsg(message)			printf message,printf("\n"),(gPreventDebuggerBreaks ? (void)0 : CallDebugger())
	#endif
	#endif

	// Metrowerks compiler cannot handle logical expressions in
	// array size definitions
	#ifdef __MWERKS__
		#define Postulate(postulation)		Assert(postulation)
	#else
		#define Postulate(postulation)		{ extern int compilerStopsHereIfPostulationIsFalse[(!!(postulation))*2 - 1]; compilerStopsHereIfPostulationIsFalse[0] = 1; }
	#endif

	#define Assert(assertion)			((assertion) ? _void : (Complain(("assertion failed: %s\n", #assertion))))

#else	/* of #ifdef DEBUG */
	#define DebugCode(a)
	#define DebugMsg()					_void
	#define Complain(message)			_void
	#define Message(message)			_void
	#define ImportantMessage(message) 	_void
	#define TrivialMessage(message)		_void
	#define Log(message)				_void
	#define Assert(assertion)			_void
	#define Postulate(postulation)		_void
#endif /* of #else of #ifdef DEBUG */

#ifdef PRERELEASE
	#define PostulateFinal(postulation)	_void
#else
	#define PostulateFinal(postulation)	Postulate(postulation)
#endif /* PRERELEASE */

#ifdef HARDWARE
#define Trespass()					Complain(("%s,%d: this code should never be reached", __FILE__, __LINE__ ))
#else
#define Trespass()					Complain(("this code should never be reached"))
#endif
#define NotYetImplemented()			PostulateFinal(false)

typedef int (ObservingFunction)(const void* pointee, const void* pointer, const char* fieldName, void* parameters);

#ifdef DEBUG
	void NoteObserving(const void* pointer, const void* pointee, const char* fieldName);
	int EachObservation(ObservingFunction* function, void* parameters);
#else
	#define NoteObserving(pointer, pointee, fieldName)		_void
	#define EachObservation(function, parameters)			false
#endif /* DEBUG */

#ifdef DEBUG
	extern const char* 		gLastRequestedURL;
#endif /* DEBUG */
#ifdef DEBUG_MONKEY
	extern int				gMonkeyEnable;
	void DoTheMonkey(void);
#endif /* DEBUG_MONKEY */

#if defined(DEBUG) && defined(MEMORY_TRACKING)

	extern const char* gDebugParentURL;
	extern const char* gDebugChildURL;
	
	extern const char kBootingURLString[];
	extern const char kNoActiveURLString[];
	
	#define GetDebugURL()				((gDebugChildURL == (const char*) nil) ? gDebugParentURL : gDebugChildURL)
	#define GetDebugParentURL()			(gDebugParentURL)
	
	#define PushDebugChildURL(newUrl)										\
		const char* tempPushDebugChildURLMacroLongName = gDebugChildURL;	\
		gDebugChildURL = UniqueName(newUrl)
	
	#define PopDebugChildURL()												\
		gDebugChildURL = tempPushDebugChildURLMacroLongName
	
	#define SetDebugParentURL(newURL)										\
		gDebugParentURL = UniqueName(newURL);								\
		gDebugChildURL = nil
	
	#define new(_x)				(_x *)TaggedNew(new _x,										\
												DebugClassNumberToName(kClassNumber ## _x),	\
												(ulong)kClassNumber ## _x,					\
												GetDebugURL(), 								\
												__FILE__, __LINE__)
	#define newArray(_x,_n)		(_x *)TaggedNew(new _x[_n],									\
												DebugClassNumberToName(kClassNumber ## _x),	\
												(ulong)kClassNumber ## _x,					\
												GetDebugURL(),								\
												__FILE__, __LINE__)
	#define delete(_x)			{ TaggedDelete(_x, __FILE__, __LINE__); delete _x; }
	#define deleteArray(_x)		{ TaggedDelete(_x, __FILE__, __LINE__); delete[]  _x; }

#else /* #if defined(DEBUG) && defined(MEMORY_TRACKING) */

	#define GetDebugURL()					nil
	#define GetDebugParentURL()				nil
	
	#define PushDebugChildURL(newUrl)
	#define PopDebugChildURL()
	#define SetDebugParentURL(newUrl)

	#define newArray(_x,_n)		new _x[_n]
	#define deleteArray(_x)		delete[]  _x
#endif /* #else of #if defined(DEBUG) && defined(MEMORY_TRACKING) */

/* macros to handle unused parameters for functions */
#ifdef __cplusplus
	#define UNUSED(parameter)
	
	#ifdef FOR_MAC
		#define USED_FOR_MAC(parameter)				parameter
	#else
		#define USED_FOR_MAC(parameter)
	#endif
	
	#ifdef DEBUG
		#define USED_FOR_DEBUG(parameter)			parameter
		#define USED_FOR_NONDEBUG(parameter)
	#else
		#define USED_FOR_DEBUG(parameter)
		#define USED_FOR_NONDEBUG(parameter)		parameter
	#endif
	
	#ifdef HARDWARE
		#define USED_FOR_HARDWARE(parameter)		parameter
	#else
		#define USED_FOR_HARDWARE(parameter)
	#endif
	
	#ifdef MEMORY_TRACKING
		#define USED_FOR_MEMORY_TRACKING(parameter)	parameter
	#else
		#define USED_FOR_MEMORY_TRACKING(parameter)
	#endif
#else
	#define UNUSED(parameter)						parameter
	#define USED_FOR_MAC(parameter)					parameter
	#define USED_FOR_DEBUG(parameter)				parameter
	#define USED_FOR_NONDEBUG(parameter)			parameter
	#define USED_FOR_HARDWARE(parameter)			parameter
	#define USED_FOR_MEMORY_TRACKING(parameter)		parameter
#endif /* __cplusplus */

// =============================================================================

#ifdef FOR_MAC
	#define ValidWriteLocation(addr)		(!IsError(((long)(addr)) < 4096L))
	#define ValidWriteLocations(addr, size)	(!IsError((((long)(addr)) < 4096L) || ((ulong)(size) > 0x400008)))
	#define ValidReadLocation(addr)			(!IsError(((ulong)(addr)) < 4096L))
	#define ValidReadLocations(addr, size)	(!IsError((((ulong)(addr)) < 4096L) || ((ulong)(size) > 0x400008)))
#else
	#define ValidWriteLocation(addr)		(!IsError(((ulong)(addr)) < 0x800000ff))
	#define ValidWriteLocations(addr, size)	(!IsError((((ulong)(addr)) < 0x800000ff) || ((ulong)(size) > 0x400008)))
	#define ValidReadLocation(addr)			(!IsError(((ulong)(addr)) < 0x800000ff))
	#define ValidReadLocations(addr, size)	(!IsError((((ulong)(addr)) < 0x800000ff) || ((ulong)(size) > 0x400008)))
#endif /* FOR_MAC */

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include Debug.h multiple times"
	#endif
#endif /* __DEBUG_H__ */

