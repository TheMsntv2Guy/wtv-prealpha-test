//
//	LocalNet.c
//
//	 LocalNet stores data associated with a URL to the local filesystem.
//

#include "Headers.h"

#ifdef FOR_MAC

#ifndef __CACHEENTRY_H__
#include "CacheEntry.h"
#endif
#ifndef __LOCALNET_H__
#include "LocalNet.h"
#endif
#ifndef __MACSIMULATOR_H__
#include "MacSimulator.h"
#endif

const char kPrefsLocalNet[] = "LocalNet Prefs";

// ---------------------------------------------------------------------------
//	variables and static prototypes

LocalNet* gLocalNet = nil;

static Boolean MakeDirectorySpec(FSSpec* spec, char* name, Boolean createDirectory);
static Boolean MakeCacheFileSpec(FSSpec* spec, char* name);

// ---------------------------------------------------------------------------

LocalNet::LocalNet(void)
{
	if (gLocalNet != nil)
		delete gLocalNet;
	
	gLocalNet = this;
	if (!RestorePrefs())
	{	fActiveRead = false;
		fActiveWrite = false;
		fHasRoot = false;
	}
}
LocalNet::~LocalNet(void)
{
	SavePrefs();
	gLocalNet = nil;
}

Boolean
LocalNet::RestorePrefs()
{
	LocalNetPrefs* prefPtr;
	unsigned long prefsSize;
	gMacSimulator->OpenPreferences();
	Boolean foundPrefs = gMacSimulator->GetPreference(kPrefsLocalNet, &prefPtr, &prefsSize);
	if (foundPrefs)
	{
		LocalNetPrefs prefs = *prefPtr;
		Assert(prefsSize == sizeof(LocalNetPrefs));
		if (prefsSize != sizeof(LocalNetPrefs))
		{	foundPrefs = false;
		}
		else
		{
			SetActiveRead(prefs.activeRead);
			SetActiveWrite(prefs.activeWrite);
			SetExclusiveRead(prefs.exclusiveRead);
			if (prefs.hasRoot)
			{	SetRoot(&(prefs.rootSpec));
			}
		}
	}
	gMacSimulator->ClosePreferences();
	return foundPrefs;
}

Boolean
LocalNet::SavePrefs()
{
	LocalNetPrefs prefs;
	prefs.activeRead  = fActiveRead;
	prefs.activeWrite = fActiveWrite;
	prefs.exclusiveRead  = fExclusiveRead;
	prefs.hasRoot     = fHasRoot;
	prefs.rootSpec    = fRootFSSpec;
	return gSimulator->SetPreference(kPrefsLocalNet, &prefs, sizeof(prefs));
}

// ---------------------------------------------------------------------------

Boolean
LocalNet::GetActiveRead(void)
{	return fActiveRead;
}

Boolean
LocalNet::GetActiveWrite(void)
{	return fActiveWrite;
}

Boolean
LocalNet::GetExclusiveRead(void)
{	return fExclusiveRead;
}

Boolean
LocalNet::GetRoot(FSSpec* spec)
{
	if (fHasRoot && (spec!=nil))
		*spec = fRootFSSpec;
	return fHasRoot;
}

void
LocalNet::SetActiveRead(Boolean active)
{
	fActiveRead = active;
	if (!fActiveRead)
	{	fExclusiveRead = false;
	}
}

void
LocalNet::SetActiveWrite(Boolean active)
{
	fActiveWrite = active;
}

void
LocalNet::SetExclusiveRead(Boolean exclusive)
{
	fExclusiveRead = exclusive;
	if (fExclusiveRead)
	{	fActiveRead = true;
	}
}

void
LocalNet::SetRoot(FSSpec* spec)
{
	if (spec==nil)
	{	fHasRoot = false;
	}
	else
	{
		OSErr err;
		
		err = FSMakeFSSpec(spec->vRefNum, spec->parID, spec->name, &fRootFSSpec);
	
		if (err != noErr)
		{	Complain(("LocalNet::SetRoot(FSSpec*)  FSMakeFSSpec() returned err==%d", err));
			fHasRoot = false;
		}
		else
		{	fHasRoot = true;
		}
	}
}


void
LocalNet::SetRoot(void)
{
	Boolean result = false;
	StandardFileReply reply;
	StandardPutFile("\pSave this in LocalNet's root folder", "\p¥Dummy Name¥", &reply);
	if (reply.sfGood)
	{	
		reply.sfFile.name[0] = 0;
		SetRoot(&(reply.sfFile));
		fHasRoot = true;
	}
}

// ---------------------------------------------------------------------------

Boolean
LocalNet::GetURLInCache(const char* url)
{
	if (!(fHasRoot && fActiveRead))
		return false;

	FSSpec spec;
	if (!URLToFSSpec(url, &spec, false))
		return false;
	FSSpec copySpec;
	return (noErr == FSMakeFSSpec(spec.vRefNum, spec.parID, spec.name, &copySpec));	
}

long
LocalNet::GetURLSize(const char* url)
{
	if (!(fHasRoot && fActiveRead))
		return 0;
	
	FSSpec spec;
	if (!URLToFSSpec(url, &spec, false))
	{
		Complain(("LocalNet::GetURLSize() couldn't create FSSpec for \"%s\"", url));
		return 0;
	}
	short refNum;
	OSErr err = FSpOpenDF(&spec, fsRdPerm, &refNum);
	if (err != noErr)
	{
		Complain(("LocalNet::GetURLSize() couldn't FSpOpenDF \"%s\" (err==%d)", url, err));
		return 0;
	}
	long logEOF;
	err = GetEOF(refNum, &logEOF);
	if (err != noErr)
	{
		Complain(("LocalNet::GetURLSize() couldn't GetEOF for \"%s\" (err==%d)", url, err));
		return 0;
	}
	return logEOF;
}
long
LocalNet::GetURLData(const char* url, void* buffer, long size)
{
	if (!(fHasRoot && fActiveRead))
		return 0;

	FSSpec spec;
	if (!URLToFSSpec(url, &spec, false))
	{
		Complain(("LocalNet::GetURLData() couldn't create FSSpec for \"%s\"", url));
		return 0;
	}
	short refNum;
	OSErr err = FSpOpenDF(&spec, fsRdPerm, &refNum);
	if (err != noErr)
	{
		Complain(("LocalNet::GetURLData() couldn't FSpOpenDF \"%s\" (err==%d)", url, err));
		return 0;
	}
	long count = size;
	err = FSRead(refNum, &count, buffer);
	if (err != noErr)
	{
		Complain(("LocalNet::GetURLData() couldn't FSRead from \"%s\" (err==%d)", url, err));
		return 0;
	}
	if (count != size)
	{
		Complain(("LocalNet::GetURLData() did partial read (%d of %d bytes)", count, size));
	}
	return count;
}
void
LocalNet::SaveCacheEntry(CacheEntry* cache)
{
	if ((cache != nil) && fHasRoot && fActiveWrite)
	{
		const char* url = cache->GetName();
		void* buffer = cache->GetData();
		long size = cache->GetDataLength();
		
		const kJPEGView			= QuadChar('J','V','W','R');
		const kTeachText		= QuadChar('t','t','x','t');
		const kMPWShell			= QuadChar('M','P','S',' ');
		const kMoviePlayer		= QuadChar('T','V','O','D');
		const kUnknownCreator	= QuadChar('?','?','?','?');
		const kRealAudioPlayer  = QuadChar('P','N','R','M');
		
		DataType dataType = cache->GetDataType();
		OSType fileCreator;
		OSType fileType = dataType;
		switch (dataType)
		{
			case kDataTypeHTML:			fileCreator = kMPWShell;	fileType = 'TEXT';		break;
			case kDataTypeTEXT:			fileCreator = kMPWShell;	break;
			case kDataTypeGIF:			fileCreator = kJPEGView;	break;
			case kDataTypeJPEG:			fileCreator = kJPEGView;	break;
			case kDataTypeXBitMap:		fileCreator = kTeachText;	break;
			case kDataTypeBitmap:		fileCreator = kTeachText;	break;
			case kDataTypeBorder:		fileCreator = kTeachText;	break;
			case kDataTypeURL:			fileCreator = kTeachText;	break;
			case kDataTypeImage:		fileCreator = kTeachText;	break;
			case kDataTypeFidoImage:	fileCreator = kTeachText;	break;
			case kDataTypeAnimation:	fileCreator = kMPWShell;	break;
			case kDataTypeMIDI:			fileCreator = kMoviePlayer;	break;
			case kDataTypeMIPSCode:		fileCreator = kUnknownCreator; 			fileType = dataType;				break;
			case kDataTypeMPEGAudio:	fileCreator = kMoviePlayer;	break;
			case kDataTypePPCCode:		fileCreator = kUnknownCreator; 			fileType = dataType;				break;
			case kDataTypeRealAudioMetafile:	fileCreator = kRealAudioPlayer;	fileType = 'TEXT';		break;
			case kDataTypeUnprintable:			fileCreator = kMPWShell;		fileType = 'TEXT';		break;
			case 0:								fileCreator = kMPWShell;		fileType = 'TEXT';		break;
			default:
				Complain(("You should add a case for DataType %lx (%c%c%c%c) to the switch statement in "
						  "LocalNet::SaveCacheEntry() to give this cache entry a decent file creator/type!",
						  dataType,
						  (char)((dataType>>24)&0x0ff),
						  (char)((dataType>>16)&0x0ff),
						  (char)((dataType>>8)&0x0ff),
						  (char)(dataType & 0x0ff)));
				fileCreator = kUnknownCreator;
				fileType = dataType;
				break;
		}
		SaveURL(url, buffer, size);
	}
}
void
LocalNet::SaveURL(const char* url, void* buffer, long size, OSType fileCreator, OSType fileType)
{
	if (!(fHasRoot && fActiveWrite))
		return;
	
	FSSpec spec;
	OSErr err;
	
	// only write stuff we got from the net, because thumbnails were getting
	// written on top of the html data file with the same name
	
	if ( strchr(url,':') != 0 && strncmp(url,"http",4) != 0 )
		return;
		
	if (!URLToFSSpec(url, &spec, true))
	{	Complain(("LocalNet couldn't make FSSpec from url \"%s\"", url));
		return;
	}
	
	err = FSpCreate(&spec, fileCreator, fileType, smSystemScript);
	if ((err != noErr) && (err != dupFNErr))
	{	Complain(("Couldn't create LocalNet file for url \"%s\" (err==%d)", url, err));
		return;
	}
	
	short refNum;
	err = FSpOpenDF(&spec, fsWrPerm, &refNum);
	if (err != noErr)
	{	Complain(("Couldn't open data fork of LocalNet file for url \"%s\" (err==%d)", url, err));
		return;
	}
	
	long count = size;
	err = FSWrite(refNum, &count, buffer);
	if (err != noErr)
	{	Complain(("Error in writing to LocalNet file for url \"%s\" (err==%d)", url, err));
	}
	else
	if (count != size)
	{	Complain(("Error in writing to LocalNet file for url \"%s\" (%ld of %ld written)",
				  url, count, size));
	}
	err = SetEOF(refNum, size);
	err = FSClose(refNum);
	if (err != noErr)
	{	Complain(("Error in closing LocalNet file for url \"%s\" (err==%d)", url, err));
	}
	return;
}

Boolean
LocalNet::URLToFSSpec(const char* url, FSSpec* spec, Boolean createDirectory)
{
	Boolean result = false;
	char *kDefaultHTMLName = "index.html";
	
	// make a copy for our own usage
	char* tempCopyMaster = NewPtr(strlen(url)+1 + strlen(kDefaultHTMLName));
	if (tempCopyMaster == nil)
	{	Complain(("LocalNet::URLToFSSpec():  couldn't alloc space for copy of url"));
		goto LocalNet__URLToFSSpec_cleanup;
	}
	strcpy(tempCopyMaster, url);

	// make sure root exists
	OSErr err = FSMakeFSSpec(fRootFSSpec.vRefNum, fRootFSSpec.parID,
							 fRootFSSpec.name, spec);
	if (err != noErr)
	{	Complain(("LocalNet::URLToFSSpec():  FSMakeFSSpec of root "
				  "(vRefNum=%d, parID=%d) failed (err==%d)",
				  fRootFSSpec.vRefNum, fRootFSSpec.parID, err));
		goto LocalNet__URLToFSSpec_cleanup;
	}
	
	char* segmentStart = tempCopyMaster;
	char* segmentEnd = tempCopyMaster;

	// skip the "http:" or whatever this starts with...
	Boolean hasScheme = false;
	while (((*segmentEnd >= 'a') && (*segmentEnd <= 'z')) ||
		   ((*segmentEnd >= 'A') && (*segmentEnd <= 'Z')) ||
		   (*segmentEnd == '+') ||
		   (*segmentEnd == '-') ||
		   (*segmentEnd == '.'))
	{
		segmentEnd++;
	}
	if (*segmentEnd==':')
	{
		*segmentEnd++ = '\0';
		hasScheme = true;
	}
	else
		segmentEnd = segmentStart;
	
	// determine if this thing has a netloc
	Boolean hasNetloc = false;
	if (*segmentEnd == '/')
	{	segmentEnd++;
		if (*segmentEnd == '/')
		{	hasNetloc = true;
			segmentEnd++;
		}
	}
	
	if (!hasNetloc)	// if no netloc, put something else in place of netloc
	{
		char* fakeNetloc = "no netloc specified";
		if (hasScheme)
			fakeNetloc = segmentStart;
		
		if (!MakeDirectorySpec(spec, fakeNetloc, createDirectory))
		{	goto LocalNet__URLToFSSpec_cleanup;
		}
	}
	
	// ------------
	
	while (!result)
	{
		segmentStart = segmentEnd;
		while ((*segmentEnd != '/')
				&& (*segmentEnd != '?')
				&& (*segmentEnd != ';')
				&& (*segmentEnd != '#')
				&& (*segmentEnd != '\0'))
		{
			if (*segmentEnd == ':')
			{
				if (hasNetloc)
					*segmentEnd = '\0'; // port number...kill it
				else
					*segmentEnd = '|'; // Mac file system hates ':' characters
			}
			segmentEnd++;
		}
		
		if ((*segmentEnd == '/') || ((*segmentEnd == '\0') && hasNetloc))
		{	
			Boolean noHTMLSpecified =  *segmentEnd == '\0' || ((*segmentEnd == '/') && (segmentEnd[1] == '\0') );
			if (segmentEnd != '\0')
				*segmentEnd++ = '\0';
			if (!MakeDirectorySpec(spec, segmentStart, createDirectory))
				goto LocalNet__URLToFSSpec_cleanup;
			hasNetloc = false;
			if ( noHTMLSpecified ) {
				segmentStart = segmentEnd;
				strcpy(segmentStart,kDefaultHTMLName);
			}
		}
		else
		{
			char tempChar = *segmentEnd;
			*segmentEnd = '\0';
			if (!MakeCacheFileSpec(spec, segmentStart))
				goto LocalNet__URLToFSSpec_cleanup;
			*segmentEnd = tempChar;
			result = true;
		}
	}
	
	
LocalNet__URLToFSSpec_cleanup:
	if (tempCopyMaster != nil)
		DisposePtr(tempCopyMaster);
	return result;	
}





// ---------------------------------------------------------------------------
//	Static helper functions
// ---------------------------------------------------------------------------
static Boolean
MakeDirectorySpec(FSSpec* spec, char* name, Boolean createDirectory)
{
	Boolean result = true;
	Str255 pascalName;
	pascalName[0] = spec->name[0]+2;
	pascalName[1] = ':';
	memcpy(&(pascalName[2]), &(spec->name[1]), spec->name[0]);
	pascalName[pascalName[0]] = ':';
	char* appendHere = (char*)&(pascalName[pascalName[0]+1]);
	long length = strlen(name);
	if (length > 31)
		length = 31;
	pascalName[0] += length;
	strncpy(appendHere, name, length);
	
	OSErr err = FSMakeFSSpec(spec->vRefNum, spec->parID, pascalName, spec);
	if ((err != noErr) && (err != fnfErr))
	{	Complain(("MakeDirectorySpec() couldn't find FSSpec for parent dir (err==%d)",
					err));
		result = false;
	}
	else if (err == fnfErr)
	{
		if (createDirectory)
		{
			long createdDirID;
			OSErr err = FSpDirCreate(spec, smSystemScript, &createdDirID);
			if (err != noErr)
				{	Complain(("MakeDirectorySpec() couldn't FSpDirCreate \"%s\" (err==%d)", name, err));
			result = false;
			}
			else
			{
				err = FSMakeFSSpec(spec->vRefNum, createdDirID, nil, spec);
				if (err != noErr)
				{	Complain(("MakeDirectorySpec() couldn't create FSSpec for child dir \"%s\" (err==%d)",
								name, err));
					result = false;
				}
			}
		}
		else
		{	result = false;
		}
	}
	return result;
}

static Boolean
MakeCacheFileSpec(FSSpec* spec, char* name)
{
	Boolean result = true;
	Str255 pascalName;
	pascalName[0] = spec->name[0]+2;
	pascalName[1] = ':';
	memcpy(&(pascalName[2]), &(spec->name[1]), spec->name[0]);
	pascalName[pascalName[0]] = ':';
	char* appendHere = (char*)&(pascalName[pascalName[0]+1]);
	long length = strlen(name);
	if (length > 31)
		length = 31;
	pascalName[0] += length;
	strncpy(appendHere, name, length);
	
	OSErr err = FSMakeFSSpec(spec->vRefNum, spec->parID, pascalName, spec);
	if ((err != noErr) && (err != fnfErr))
	{	Complain(("MakeCacheFileSpec() couldn't create FSSpec (err==%d)", err));
		result = false;
	}
	return result;
}

#endif /* FOR_MAC */





