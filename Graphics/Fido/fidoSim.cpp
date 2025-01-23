#ifndef _FIDOSIM_CPP_
#define _FIDOSIM_CPP_

#ifndef _FIDOSIM_H_
#include "fidoSim.h"
#endif

inline celRecord* fidoData::get(int index) const
{
	Assert(index >= 0);
//	Assert(index < lastCel);
	return (celRecord*) &((long*) celBase)[index];
}

inline void fido::init(yEntry* map, celRecord* list)
{
	Assert(map);
	Assert(list);
	master.yMap = map;
	master.celBase = list;
}

inline void fido::loadCodebookCache(const celRecord* cel) 
{
	texture* base;
	memcpy(codebookCache, cel->get(base), sizeof(codebookCache));
	// !!! update stats accordingly
	stats.line[0].longRead += 16;
}

inline void fido::loadYMapBase(const celRecord* cel) 
{
	texture* base;
	working.yMap = (yEntry*) cel->get(base);
	yMapIndex = 0;
}

inline void fido::loadCelsBase(const celRecord* cel) 
{
	texture* base;
	working.celBase = (celRecord*) cel->get(base);
}

inline void fido::loadYMapMaster(const celRecord* cel) 
{
	texture* base;
	master.yMap = (yEntry*) cel->get(base);
	yMapIndex = 0;
}

inline void fido::loadCelsMaster(const celRecord* cel) 
{
	texture* base;
	master.celBase = (celRecord*) cel->get(base);
}

inline void fido::loadRAMAddressMSB(const celRecord* cel) 
{
	texture* base;
	ramAddressMSB = (long) cel->get(base);
}

inline void fido::loadROMAddressMSB(const celRecord* cel) 
{
	texture* base;
	romAddressMSB = (long) cel->get(base);
}

#endif // _FIDOSIM_CPP_