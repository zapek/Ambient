#ifndef AMBIENT_VIEWCLASS_H
#define AMBIENT_VIEWCLASS_H
/*
 * $Id: viewclass.h,v 1.7 2016/08/01 18:51:29 itix Exp $
 */

enum { LVM_ICONS, LVM_SHOWALL, LVM_THUMBS };

#define FVS_FILES     (1 << 0)
#define FVS_DRAWERS   (1 << 1)
#define FVS_VOLUMES   (1 << 2)
#define FVS_APPICONS  (1 << 3)
#define FVS_SYSTEM    (1 << 4)	/* System icons */
#define FVS_SHORTCUTS (1 << 5)

#define FVS_ALL            0xffffffff
#define FVS_FS_OBJECTS     (FVS_FILES | FVS_DRAWERS)
#define FVS_NONE           0

/**********************************************************************
 * MUI methods and definitions
 *********************************************************************/

struct MP_View_ExecuteAction {
	ULONG MethodID;
	LONG  ExecuteAction;
};

enum
{
	MV_View_ExecuteAction_Copy,
	MV_View_ExecuteAction_Cut,
	MV_View_ExecuteAction_Paste,
	MV_View_ExecuteAction_PasteAs,
	MV_View_ExecuteAction_Delete,
	MV_View_ExecuteAction_Rename,
	MV_View_ExecuteAction_Information,
	MV_View_ExecuteAction_ParentView,
	MV_View_ExecuteAction_LeaveOut,
	MV_View_ExecuteAction_PutAway,
	MV_View_ExecuteAction_SelectAll,
	MV_View_ExecuteAction_SelectNone,
	MV_View_ExecuteAction_SelectInvert,
	MV_View_ExecuteAction_SelectPattern,
	MV_View_ExecuteAction_CycleViewMode,
	MV_View_ExecuteAction_CycleSubViewMode,
	MV_View_ExecuteAction_FindFiles
};

#endif /* AMBIENT_VIEWCLASS_H */
