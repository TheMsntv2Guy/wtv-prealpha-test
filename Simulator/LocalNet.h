// ===========================================================================
//	LocalNet.h
//	 LocalNet stores data associated with a URL to the local filesystem.
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __LOCALNET_H__
#define __LOCALNET_H__

#ifdef FOR_MAC

class CacheEntry;

// ===========================================================================

struct LocalNetPrefs
{
	Boolean	activeRead;
	Boolean	activeWrite;
	Boolean exclusiveRead;
	Boolean hasRoot;
	FSSpec	rootSpec;
};

class LocalNet
{
	enum {
		kLocalNetCreator = 'MPS ',
		kLocalNetFileType = 'TEXT'
		};

	public:
						LocalNet(void);
		virtual			~LocalNet(void);
		
		long			GetURLSize(const char* url);
		long			GetURLData(const char* url, void* buffer, long size);
		Boolean			GetURLInCache(const char* url);
		
		void			SaveCacheEntry(CacheEntry* cache);
		void			SaveURL(const char* URL, void* buffer, long size,
								OSType fileCreator = kLocalNetCreator,
								OSType fileType = kLocalNetFileType);
		
		Boolean			GetActiveRead(void);
		Boolean			GetActiveWrite(void);
		Boolean			GetExclusiveRead(void);
		Boolean			GetRoot(FSSpec* spec = nil);

		void			SetActiveRead(Boolean active);
		void			SetActiveWrite(Boolean active);
		void			SetExclusiveRead(Boolean exclusive);
		
		void			SetRoot(FSSpec* spec);
		void			SetRoot(void);
		
		Boolean			RestorePrefs(void);
		Boolean			SavePrefs(void);

	protected:
		Boolean			URLToFSSpec(const char* url, FSSpec* spec, Boolean createDirectory);
		//Boolean			PathToFSSpec(const char* url, FSSpec *spec);
		//char*				FSSpecToPath(FSSpec* spec);

	protected:
		Boolean			fActiveRead;
		Boolean			fActiveWrite;
		Boolean			fExclusiveRead;
		Boolean			fHasRoot;
		FSSpec			fRootFSSpec;
};

extern LocalNet* gLocalNet;

#endif /* FOR_MAC */

#endif /* __LOCALNET_H__ */