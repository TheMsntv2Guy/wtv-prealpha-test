/*
    profiler.h

    The main entry points for the Profiler.
    
    Copyright � 1993-1994 Metrowerks Inc.  All rights reserved.
 */

#ifndef __PROFILER__
#define __PROFILER__

typedef enum { collectDetailed, collectSummary } ProfilerCollectionMethod;
typedef enum { ticksTimeBase, timeMgrTimeBase, microsecondsTimeBase, PPCTimeBase, bestTimeBase } ProfilerTimeBase;
	
#ifdef __cplusplus
extern "C" {
#endif

/*
  The following call turns on the Profiler and starts the underlying timebase.
 */
pascal OSErr ProfilerInit(ProfilerCollectionMethod method, ProfilerTimeBase timeBase, short numFunctions, short stackDepth);

/*
  Turn off the profiler and stop the timebase.  This releases the memory holding
  the recorded data.
 */
pascal void ProfilerTerm(void);

/*
  Turn on and off the profiler.  This pauses the timebase, and the recording of data.
 */
pascal void ProfilerSetStatus(short on);
pascal short ProfilerGetStatus(void);

/*
  Return the data buffer sizes that the profiler has currently used.  This is useful
  for tuning the buffer sizes passed to ProfilerInit.
 */

pascal void ProfilerGetDataSizes(long *functionSize, long *stackSize);

/*
  Dump the current buffer of profile information to the given file.  If it exists, append
  and increment a number to the filename.  This does not clear the profile information.
  The filename should be a Pascal string.
 */
pascal OSErr ProfilerDump(StringPtr filename);

/*
  Clear out the profile buffer.  This does not stop the recording of new information.
 */
pascal void ProfilerClear(void);

/*
  The actual function that the compiler calls to collect profile information.
 */
pascal void __PROFILE(char *functionName);

#ifdef __cplusplus
}
#endif

#endif