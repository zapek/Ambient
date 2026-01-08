/*
   ambientsupport.library internal library definitions.
   Generated with LibMaker 0.9.
*/

#ifndef AMBIENTSUPPORT_LIBRARY_H
#define AMBIENTSUPPORT_LIBRARY_H

#define __NOLIBBASE__

#include <exec/libraries.h>
#include <exec/semaphores.h>

#define UNUSED __attribute__((unused))

extern struct AmbientSupportBase *AmbientSupportBase;
extern struct ExecBase           *SysBase;
extern struct Library            *UtilityBase;
extern struct Library            *CyberGfxBase;
extern struct Library            *CGXDitherBase;
extern struct Library            *GfxBase;
extern struct Library            *LayersBase;
extern struct Library            *IntuitionBase;
extern struct Library            *DOSBase;


struct AmbientSupportBase
{
	struct Library          LibNode;
	APTR                    Seglist;
	struct SignalSemaphore  BaseLock;
	BOOL                    InitFlag;
/* library stuff */
	APTR                    MemoryPool;
	ULONG                   HasAltiVec;
#if USE_ALTIVEC
	ULONG                   UseAltiVec;
#endif
};

#include "internal/math_sqrt.h"
#include "internal/altivec.h"

#endif      /* AMBIENTSUPPORT_LIBRARY_H */
