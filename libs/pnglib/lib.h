#ifndef AMBIENT_PNGLIB_LIB_H
#define AMBIENT_PNGLIB_LIB_H
/*
 * $Id: lib.h,v 1.2 2005/07/04 23:06:11 laire Exp $
 */

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <ppcinline/exec.h>
#include <dos/dos.h>

extern struct ExecBase *SysBase;
extern struct Library *PNGLibBase;

int lib_init(struct ExecBase *SBase);
int lib_open(void);
void lib_cleanup(void);

struct LibBase
{
	struct Library Lib;
	BPTR SegList;
	struct ExecBase *SBase;
};

struct Library * LIB_Open(void);
ULONG LIB_Close(void);
ULONG LIB_Expunge(void);
ULONG LIB_GetQueryAttr(void);

#endif /* AMBIENT_PNGLIB_LIB_H */
