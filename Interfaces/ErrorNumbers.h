#ifndef __ERRORNUMBERS_H__
#define __ERRORNUMBERS_H__

// NOTE: Please update error message text in Alert::ShowError when changing enum Error.

enum Error
{
	kComplete = 3,
	kPending = 2,
	kStreamReset = 1,
	kNoError = 0,
	kGenericError = -1,
	kLowMemory = -2,
	kElementNotFound = -3,
	kHostNotFound = -4,
	kNoConnection = -5,
	kCannotListen = -6,
	kResponseError = -7,
	kCacheFull = -8,
	kCannotParse = -9,
	kTooManyConnections = -10,
	kUnknownService = -11,
	kFileNotFound = -12,
	kAborted = -13,
	kResourceNotFound = -14,
	kPageNotFound = -15,
	kNoCarrier = -16,
	kLineBusy = -17,
	kCannotClose = -18,
	kTimedOut = -20,
	kUnknownDataType = -21,
	kLostConnection = 22,			// NOTE: testing enum problem.
	kAuthorizationError = -23,
	kTruncated = -24,
	
	// simulator errors
	kCannotOpenResolver = -100,
	
	// JPEG
	kNoSOIMarkerErr	= -1024,	// No Start of Image marker, bad JPEG data
	kNoSOFMarkerErr	= -1025,	// No Start of Frame marker, bad JPEG data
	kNoSOSMarkerErr	= -1026,	// No Start of Scan marker, bad JPEG data
	kBadSOFMarkerErr = -1027,	// Bad Start of Frame marker, bad JPEG data
	kBadSOSMarkerErr = -1028,	// Bad Start of Scan marker, bad JPEG data
	kBadMarkerErr = -1029,		// Other marker that I don't want to know about
	kBadHuffmanErr = -1030,		// Bad huffman code
	kOutOfDataErr = -1031,		// Read past end of data
	
	kWrongImageType = -1234
};

#ifdef DEBUG_NAMES
const char* GetErrorString(Error error);	// implementation in Debug.c
#endif

#define TrueError(_error)	((signed)_error < 0)

#else
	#ifdef FIND_MULTIPLY_INCLUDED_HEADERS
	#error "Attempted to #include ErrorNumbers.h multiple times"
	#endif
#endif /* __ERRORNUMBERS_H__ */
