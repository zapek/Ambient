#ifndef AMBIENT_ICON_LIB_H
#define AMBIENT_ICON_LIB_H
/*
 * $Id: lib.h,v 1.4 2006/04/12 14:01:58 fab Exp $
 */

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <workbench/workbench.h>
#include <proto/exec.h>

extern struct Library *IconBase;

int lib_init(struct ExecBase *SBase);
int lib_open(void);
void lib_cleanup(void);

struct LibBase
{
	struct Library Lib;
	BPTR SegList;
	struct ExecBase *SBase;
};

struct Library *LIB_Open(void);
BPTR LIB_Close(void);
BPTR LIB_Expunge(void);
ULONG LIB_GetQueryAttr(void);

/* private */
APTR LIB_GetWBObject(void);
BOOL LIB_PutWBObject(void);
BOOL LIB_GetIcon(void);
BOOL LIB_PutIcon(void);

/* public */
void LIB_FreeFreeList(void);

/* private */
void LIB_FreeWBObject(void);
struct WBObject * LIB_AllocWBObject(void);

/* public */
BOOL LIB_AddFreeList(void);
struct DiskObject * LIB_GetDiskObject(void);
BOOL LIB_PutDiskObject(void);
void LIB_FreeDiskObject(void);
UBYTE * LIB_FindToolType(void);
BOOL LIB_MatchToolValue(void);
STRPTR LIB_BumpRevision(void);

/* private */
APTR LIB_FreeAlloc(void);

/* public (V36) */
struct DiskObject * LIB_GetDefDiskObject(void);
BOOL LIB_PutDefDiskObject(void);
struct DiskObject * LIB_GetDiskObjectNew(void);

/* public (V37) */
BOOL LIB_DeleteDiskObject(void);

/* dummies for now (v44+) */
void LIB_FreeFree(void);
struct DiskObject * LIB_DupDiskObjectA(void);
ULONG LIB_IconControlA(void);
void LIB_DrawIconStateA(void);
BOOL LIB_GetIconRectangleA(void);
struct DiskObject * LIB_NewDiskObject(void);
struct DiskObject * LIB_GetIconTagList(void);
BOOL LIB_PutIconTagList(void);
BOOL LIB_LayoutIconA(void);
void LIB_ChangeToSelectedIconColor(void);


#endif /* AMBIENT_ICON_LIB_H */
