#include "Headers.h"
#include "BoxBoot.h"
#include "Caches.h"
#include "Exceptions.h"
#include "Interrupts.h"
#include "SiliconSerial.h"

// dummy file for linking
void	move_reset_stack(ulong) {}

// from BoxDebugger.c

extern "C" void returnFromDebugger(void);

void returnFromDebugger(void)
{
}

// from Box:Caches.s
void config_cache()
{
}

void FlushCache()
{
}

void ClearCache()
{
}

// from Box:Interrupts.s
void SetIntEnables(ulong ints) 
{
	ints = ints;		// don't complain that ints is not used
}

ulong GetIntEnables(void)
{
	return 0;
}

ulong EnableInts(ulong ints)
{
	ints = ints;		// don't complain that ints is not used
	return 0;
}

ulong DisableInts(ulong ints)
{
	ints = ints;		// don't complain that ints is not used
	return 0;
}

void ClearAllInts(void)
{
}

void InterruptDecode(void);
void InterruptDecode(void)
{
}

// from Box:SiliconSerial.s
static SiSerialNumber number = {0, 0};

SiSerialNumber *ReadSiliconSerialNumber(void)
{
	return &number;
}

SiSerialNumber *GetSiliconSerialNumber(void)
{
	return &number;
}

