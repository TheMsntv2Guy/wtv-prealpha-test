/* This file should be placed after the Mac .h files, to un-redirect
 * the names of the conflicting Mac names. It's companion file is
 * MacintoshUndefines.h, which is before after the .h files.
 */

#undef Boolean
#undef Button
#undef Byte
#undef Cell
#undef Control
#undef Create
#ifdef DefinedDebugger
#undef Debugger
#endif
#undef Point
#undef FixedPoint
#undef Region
