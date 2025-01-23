// ===========================================================================
//	PerfDump.h
//
//	Copyright (c) 1995-1996 Artemis Research, Inc. All rights reserved.
// ===========================================================================

#ifndef __PERFDUMP_H__
#define __PERFDUMP_H__

// ===========================================================================
//	PerfDump
// ===========================================================================

#ifdef DEBUG_PERFDUMP

	enum PerfDumpEvent {
		kPerfDumpEventNone,
		kPerfDumpEventEnter,
		kPerfDumpEventExit,
		kPerfDumpEventMark,
		
		kNumPerfDumpEvents
	};

	void RecordPerfDumpEvent(const char* tag, PerfDumpEvent event, const char* file, int line);
	
	#define PerfDumpEnter(tag)	RecordPerfDumpEvent(tag, kPerfDumpEventEnter, __FILE__, __LINE__)
	#define PerfDumpExit(tag)	RecordPerfDumpEvent(tag, kPerfDumpEventExit, __FILE__, __LINE__)
	#define PerfDumpMark(tag)	RecordPerfDumpEvent(tag, kPerfDumpEventMark, __FILE__, __LINE__)
	
	void PerfDumpInitialize();
	void PerfDumpFinalize();
	Boolean GetPerfDumpActive();

#else

	#define PerfDumpEnter(tag)		((void)0)
	#define PerfDumpExit(tag)		((void)0)
	#define PerfDumpMark(tag)		((void)0)

	#define PerfDumpInitialize()	((void)0)
	#define PerfDumpFinalize()		((void)0)
	#define GetPerfDumpActive()		false

#endif /* PERFDUMP */

class PerfDump {
	public:
				PerfDump(const char* tag);
				~PerfDump(void);
	void		Mark(void);

#ifdef DEBUG_PERFDUMP	
	protected:
		const char* fTag;
#endif /* DEBUG_PERFDUMP */
};

#ifdef DEBUG_PERFDUMP
	inline PerfDump::PerfDump(const char* tag) : fTag(tag)	{ PerfDumpEnter(tag); }
	inline PerfDump::~PerfDump(void)						{ PerfDumpExit(fTag); }
	inline void PerfDump::Mark(void) 						{ PerfDumpMark(fTag); }
#else
	inline PerfDump::PerfDump(const char*)	{ }
	inline PerfDump::~PerfDump(void)		{ }
	inline void PerfDump::Mark(void) 		{ }
#endif /* DEBUG_PERFDUMP */

// ===========================================================================
//	ProfileUnit
// ===========================================================================

#ifdef SIMULATOR

	void		InitializeProfileUnit(void);
	void		FinalizeProfileUnit(void);
	Boolean		GetProfileUnitEnabled(void);
	void		SetProfileUnitEnabled(Boolean enabled);
	void		ProfileUnitEnter(int index);
	void		ProfileUnitEnter(const char* name);
	void		ProfileUnitExit(int index);
	void		ProfileUnitExit(const char* name);
	int			GetNumProfileUnits(void);
	const char*	GetProfileUnitName(int index);
	Boolean		GetProfileUnitOn(int index);
	Boolean		GetProfileUnitOn(const char* name);
	void		SetProfileUnitOn(int index, Boolean isOn);
	void		SetProfileUnitOn(const char* name, Boolean isOn);

#else
	#define InitializeProfileUnit()		((void)0)
	#define FinalizeProfileUnit()		((void)0)
	#define GetProfileUnitEnabled()		(false)
	#define SetProfileUnitEnabled()		((void)0)
	#define ProfileUnitEnter(value)		((void)0)
	#define ProfileUnitExit(value)		((void)0)
	#define GetNumProfileUnits()		(0)
	#define GetProfileUnitName()		(nil)
	#define GetProfileUnitOn(value)		(false)
	#define SetProfileUnitOn(value,on)	((void)0)

#endif

#endif // __PERFDUMP_H__