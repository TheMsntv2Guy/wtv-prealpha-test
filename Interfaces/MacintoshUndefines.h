/* This file should be placed before the Mac .h files, to redirect
 * the names of the conflicting Mac names.  It's companion file is
 * MacintoshRedefines.h, which is placed after the .h files.
 */

#define Boolean		MacBoolean
#define Button		MacButton
#define Byte		MacByte
#define Cell		MacCell
#define Control		MacControl
#define Create		MacCreate
#ifndef Debugger
	#define Debugger	MacDebugger
	#define DefinedDebugger
#endif
#define Point		MacPoint
#define FixedPoint	MacFixedPoint
#define Region		MacRegion
