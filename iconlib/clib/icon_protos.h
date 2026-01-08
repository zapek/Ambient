#ifndef  CLIB_ICON_PROTOS_H
#define  CLIB_ICON_PROTOS_H

#ifndef  EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef  WORKBENCH_WORKBENCH_H
#include <workbench/workbench.h>
#endif
#ifndef DATATYPES_PICTURECLASS_H
#include <datatypes/pictureclass.h>
#endif

struct WBObject;

APTR GetWBObject(CONST_STRPTR name);
BOOL PutWBObject(CONST_STRPTR name, APTR object);
BOOL GetIcon(CONST_STRPTR name, struct DiskObject *icon, struct FreeList *fl);
BOOL PutIcon(CONST_STRPTR name, struct DiskObject *icon);
void FreeFreeList(struct FreeList *freelist);
void FreeWBObject(struct WBObject *wbo);
struct WBObject * AllocWBObject(void);
BOOL AddFreeList(struct FreeList *freelist, APTR mem, unsigned long size);
struct DiskObject *GetDiskObject(CONST UBYTE *name);
BOOL PutDiskObject(CONST UBYTE *name, struct DiskObject *diskobj);
void FreeDiskObject(struct DiskObject *diskobj);
UBYTE *FindToolType(UBYTE **toolTypeArray, CONST UBYTE *typeName);
BOOL MatchToolValue(UBYTE *typeString, UBYTE *value);
STRPTR BumpRevision(UBYTE *newname, UBYTE *oldname);
APTR FreeAlloc(struct FreeList *freelist, ULONG size, ULONG flags);
/*--- functions in V36 or higher (Release 2.0) ---*/
struct DiskObject *GetDefDiskObject(long type);
BOOL PutDefDiskObject(struct DiskObject *diskObject);
struct DiskObject *GetDiskObjectNew(UBYTE *name);
/*--- functions in V37 or higher (Release 2.04) ---*/
BOOL DeleteDiskObject(UBYTE *name);
/*--- "OS" 3.5 crap, not implemented ---*/
void FreeFree(struct FreeList *fl, APTR address);
struct DiskObject *DupDiskObjectA(struct DiskObject *diskObject, struct TagItem *tags);
struct DiskObject *DupDiskObject(struct DiskObject *diskObject, ...);
ULONG IconControlA(struct DiskObject *icon, struct TagItem *tags);
ULONG IconControl(struct DiskObject *icon, ...);
void DrawIconStateA(struct RastPort *rp, struct DiskObject *icon, CONST_STRPTR label, LONG leftOffset, LONG topOffset, ULONG state, struct TagItem *tags);
void DrawIconState(struct RastPort *rp, struct DiskObject *icon, CONST_STRPTR label, LONG leftOffset, LONG topOffset, ULONG state, ...);
BOOL GetIconRectangleA(struct RastPort *rp, struct DiskObject *icon, CONST_STRPTR label, struct Rectangle *rect, struct TagItem *tags);
BOOL GetIconRectangle(struct RastPort *rp, struct DiskObject *icon, CONST_STRPTR label, struct Rectangle *rect, ...);
struct DiskObject *NewDiskObject(LONG type);
struct DiskObject *GetIconTagList(CONST_STRPTR name, struct TagItem *tags);
struct DiskObject *GetIconTags(CONST_STRPTR name, ...);
BOOL PutIconTagList(CONST_STRPTR name, struct DiskObject *icon, struct TagItem *tags);
BOOL PutIconTags(CONST_STRPTR name, struct DiskObject *icon, ...);
BOOL LayoutIconA(struct DiskObject *icon, struct Screen *screen, struct TagItem *tags);
BOOL LayoutIcon(struct DiskObject *icon, struct Screen *screen, ...);
void ChangeToSelectedIconColor( struct ColorRegister *cr);
#endif	 /* CLIB_ICON_PROTOS_H */
