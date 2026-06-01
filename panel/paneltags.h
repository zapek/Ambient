#ifndef PANEL_TAGS_H
#define PANEL_TAGS_H

/************************************************************************/

#define AMBIENT_FALLBACK

#include <exec/types.h>

//#include "classesambient.h"

/************************************************************************/

#define AMBIENTPANEL_AUTHORNAME_SIZEOF 80


#define SIZE_MICRO  16
#define SIZE_SMALL  24
#define SIZE_MEDIUM 32
#define SIZE_LARGE  48
#define SIZE_HUGE   64

/*
 * Icon sizes as saved in the
 * prefs and in the cycle object.
 */
 /*
enum {
	ICON_SIZE_MICRO,
	ICON_SIZE_SMALL,
	ICON_SIZE_MEDIUM,
	ICON_SIZE_LARGE,
	ICON_SIZE_HUGE,
};
*/


#ifndef MTAGBASE
#define MTAGBASE (TAG_USER|((0xFECAL<<16)+0))
#endif
/* to be used only with methods and attributes shared
   between Ambient and external classes. Use your own
   ID for everthing else */


#define MTAGBASE_PANEL (MTAGBASE + 0x2200)
enum {
	MA_paneldummy = (int)(MTAGBASE_PANEL),
		
	/* Panelwin */

	MA_Panelwin_Position,
	MA_Panelwin_Closed,     /* just for prefswin_panelclass to get notified on deleting a panel*/
	MA_Panelwin_ConfigFile,
	MA_Panelwin_Depth,
	MA_Panelwin_Group,
	MA_Panelwin_Name,
	MA_Panelwin_ParentObject,
	MA_Panelwin_PrefsNr,
	MA_Panel_PrefsPool,
	MA_Panelwin_Type,

	MM_Panelwin_AttachToBorder,
	MA_Panelgroup_Alpha,     	
	MM_Panelwin_BackPen,
	MM_Panelwin_Close,
	MM_Panelwin_CloseConfigwin,
	MM_Panelwin_Delete_Ok,
	MM_Panelwin_FindObjectByType,
	MM_Panelwin_FindRoot,
	MM_Panelwin_Placement,
	MM_Panelwin_SaveConfig,
	MM_Panelwin_Size,

	/* Panel (generic) */

	MA_Panel_HasMoved,
	MA_Panel_Highlighted,
	MA_Panel_Imagepath,
	MA_Panel_URI,
	MM_Panel_SaveConfig,

	/* Panelgroup */

	MA_Panelgroup_AutoZip,
	MA_Panelgroup_BackColor,
	MA_Panelgroup_BackRGB,
	MA_Panelgroup_Backdrop,
	MA_Panelgroup_BackMode,
	MA_Panelgroup_Configwin,
	MA_Panelgroup_DragBar1,
	MA_Panelgroup_DragBar2,
	MA_Panelgroup_DragMode,
	MA_Panelgroup_DockMode,
	MA_Panelgroup_FrameSpec,
	MA_Panelgroup_HasChanged,
	MA_Panelgroup_ImageSpec,
	MA_Panelgroup_Locked,
	MA_Panelgroup_Zipped,
	MA_Panelgroup_Zipping,
    MA_Panelgroup_HideDragBar,
    MA_Panelgroup_New,
	MA_Panelgroup_FixedMode,

	MM_Panelgroup_ChangeDrag,
	MM_Panelgroup_Clear,
	MM_Panelgroup_DelayedZip,
//	MM_Panelgroup_Delete,
	MM_Panelgroup_FindSubPanel,
	MM_Panelgroup_Lock,
	MM_Panelgroup_MoveMode_Start,
	MM_Panelgroup_MoveMode_Stop,
	MM_Panelgroup_Refresh,
//	MM_Panelgroup_RemoveItem,
	MM_Panelgroup_SaveConfig,
	MM_Panelgroup_ToggleLock,
	MM_Panelgroup_ToggleZip,
	MM_Panelgroup_WhichObject,
	MM_Panelgroup_WhichPosition,
	MM_Panelgroup_ToogleResize,
	MM_Panelgroup_CheckZipLock,
	MM_Panelgroup_StartZipTimer,
	MM_Panelgroup_FindID,
	MM_Panelgroup_OpenParent, // bitRocky
	/* panel subwin */

	MA_Panelsubwin_Dir,
	MA_Panelsubwin_Orientation,
	MA_Panelsubwin_StayOpen,

	MM_Panelsubwin_Open,
	MM_Panelsubwin_ParseDir,
	MM_Panelsubwin_ToggleStayOpen,
	MM_Panelsubwin_Timer,

	/* panel object */

	MA_PanelObject_Default_Imagepath,

	/* panel button */

	MA_Panelbutton_AttachedObject,

	MA_Panelbutton_DefaultImagePath,
	MA_PanelZipLock,
	MM_Panelbutton_AddIcon,
	MM_Panelbutton_Launch,
	MM_Panelbutton_Close,
	
	/* paneldragclass.c */

	MA_Paneldrag_LockMode,
	MA_Paneldrag_Type,
	MA_Paneldrag_Zipping,
	MM_Paneldrag_AddDragGad,
	MM_Paneldrag_RemoveDragGad,

	/* panelclasslist.c */

	MM_Panelclasslist_FreeClasslist,
	MM_Panelclasslist_RefreshClasslist,
	MM_Panelclasslist_ScanDirectory,

	/* Panelitem_list */

	MA_Panelitem_list_IsList,

	/* Panel popup base class */

	MM_PanelPopup_Popup,
	MA_PanelPopup_IconType,             /* one of MV_Icon_Type_xxx values. otherwise some default is used */
	MA_PanelPopup_MenuTitle,            /* displayed in context menu */

	/* subpanel */

	MA_SubPanel_ID,

	/* Panelbutton */

	MM_Panelbutton_ExtraBitmap,
	MM_Panelbutton_GetAttr,
	MM_Panelbutton_Callback,
	MA_Panel_Messenger,
	MM_Panelwin_FindObject,
	MA_Panelbutton_MsgPortJammed,
	MA_Panelmessenger_PanelObject,
	MM_Panelmessengerfamily_NewObject,
	MM_Panelmessengerfamily_RemObject,
	MM_Panelmessenger_PanelObjectDisposed,
	
	
	//MA_PANEL_ZIPSPEED,
	MA_PANEL_AUTOSAVE_DROP,
	MA_PANEL_AUTOSAVE_DELETE,
	MA_PANEL_AUTOSAVE_MOVE,
	MA_PANEL_AUTOSAVE_WINDOWPOS,
//	MA_Panelwin_DockMode,
/*	MA_PANEL_HIGHLIGHT_EFFECT  ,
	MA_PANEL_HIGHLIGHT_BRIGHTEN,
	MA_PANEL_HIGHLIGHT_DARKEN  ,
	MA_PANEL_DRAGDROP_EFFECT   ,
	MA_PANEL_DRAGDROP_BRIGHTEN ,
	MA_PANEL_DRAGDROP_DARKEN   , 
	MA_PANEL_HIGHLIGHT_TINT, 
	MA_PANEL_DRAGDROP_TINT,
	MA_PANEL_HIGHLIGHT_TINTFADE,
	MA_PANEL_DRAGDROP_TINTFADE,*/
	MA_PANEL_DROPTARGET,
	MA_PanelPrefs_Object,
	MM_PanelPrefs_ExternPrefsInit,
//	MM_Application_ObtainObject,
//	MM_Application_ReleaseObject,
	MA_Panelwin_FileName,
	MM_PanelPrefs_PanelMessage,
	MA_paneldummyend
};

struct MP_Panelsubwin_Open {
	ULONG MethodID;
	ULONG open;
};

struct MP_Panelwin_Placement
{
	ULONG MethodID;
	ULONG placement;
	ULONG width,height;
	ULONG attempt;
};

struct MP_Panelgroup_SaveConfig {
	ULONG MethodID;
	APTR panel_lock;
	ULONG id;
};
/*
struct MP_Panel_Setup {
	ULONG MethodID;
	APTR ppool;
	ULONG index;
};*/

struct MP_Panelgroup_FindSubPanel {
	ULONG MethodID;
	ULONG ID;
};

struct MP_Panelgroup_RefreshRect {
	ULONG MethodID;
	ULONG left,top;
	ULONG width,height;
	struct RastPort *rp;
};

struct MP_Panelgroup_FindID
{
	ULONG MethodID;
	ULONG objectID,startID;
	APTR *obj;
};

struct MP_Panelwin_BackPen {
	ULONG MethodID;
	struct MUI_PenSpec *penspec;
	APTR poppen;
};

struct MP_Panelwin_BackAlpha {
	ULONG MethodID;
	ULONG alpha;
};

struct MP_Panelwin_Size {
	ULONG MethodID;
	ULONG size;
};

struct MP_Panelwin_FindObject {
	ULONG MethodID;
	ULONG type; 
#warning type unused?
	APTR pattern;
	APTR *object;
};

struct MP_Panelbutton_GetAttr {
	ULONG MethodID;
	ULONG attrID;
	ULONG *storage;
};

/*
struct MP_Panelbutton_Callback {
	ULONG MethodID;
	struct TagItem    *ops_AttrList;
};

struct MP_Panelmessengerfamily_NewObject
{
	ULONG MethodID;
	APTR target;
};

struct MP_Panelmessengerfamily_RemObject
{
	ULONG MethodID;
	APTR object;
};*/
#if 0
struct MP_Panelwin_AttachToBorder {
	ULONG MethodID;
	ULONG immediate_save; /* if the user moves a window to a border, it doesn't move afterwards but we want it to be saved still */
};
#endif
struct MP_PanelPrefs_ExternPrefsInit {
	ULONG MethdoID;
	APTR ppool;
	ULONG pi;	
};

struct MP_Panelbutton_ExtraBitmap {
	ULONG MethdoID;
	struct BitMap *bm;
	ULONG width,height;	
};

/* Panel (Generic) */
#define MV_Panel_Type_Invalid					 0
#define MV_Panel_Type_Drag                       1
#define MV_Panel_Type_Button 	                 2
#define MV_Panel_Type_Spacer   	                 3
#define MV_Panel_Type_Separator                  4
#define MV_Panel_Type_ViewWatcher                5
#define MV_Panel_Type___________________         6 /* use for next internal class */
#define MV_Panel_Type_Bookmarks                  7
#define MV_Panel_Type_SubPanel                   8
#define MV_Panel_Type_DirPanel	                 9
#define MV_Panel_Type_External                  10

/* PanelWin */

#define MV_Panelwin_Type_Root 		             0
#define MV_Panelwin_Type_SubPanel	             1
#define MV_Panelwin_Type_DirPanel 	             2

#define MV_Panelwin_Position_Floating			 0
#define MV_Panelwin_Position_Attached			 1
#define MV_Panelwin_Position_Fixed				 2

#define MV_Panelsubwin_Default 	                 0
#define MV_Panelsubwin_Left                      101
#define MV_Panelsubwin_Right                     102
#define MV_Panelsubwin_Top                       103
#define MV_Panelsubwin_Bottom                    104

/* Panelgroup */

#define MV_Panelgroup_Size_Micro                 SIZE_MICRO
#define MV_Panelgroup_Size_Small                 SIZE_SMALL
#define MV_Panelgroup_Size_Medium                SIZE_MEDIUM
#define MV_Panelgroup_Size_Large                 SIZE_LARGE
#define MV_Panelgroup_Size_Huge                  SIZE_HUGE

#define MV_Panelgroup_Depth_Normal               0
#define MV_Panelgroup_Depth_Back                 1
#define MV_Panelgroup_Depth_Front                2

#define MV_Panelgroup_DragMode_None              0
#define MV_Panelgroup_DragMode_LeftUp            1
#define MV_Panelgroup_DragMode_RightBottom       2
#define MV_Panelgroup_DragMode_Both              ( MV_Panelgroup_DragMode_LeftUp | MV_Panelgroup_DragMode_RightBottom )

#define	MV_Panelgroup_BackMode_Color	         0
#define MV_Panelgroup_BackMode_Picture           1
#define MV_Panelgroup_BackMode_Clone             2

#define MV_Panelgroup_RemoveItem_Current         0

#define MV_Panelgroup_WhichPosition_None         -1UL

#define MV_Panelgroup_MoveMode_Start_Coordinates 0
#define MV_Panelgroup_MoveMode_Start_Standalone  1

#define MV_Panelgroup_MoveMode_Stop_Abort        0
#define MV_Panelgroup_MoveMode_Stop_Keep         1


#define MV_Panel_ZipSpeed_Slow                1500
#define MV_Panel_ZipSpeed_Medium               500
#define MV_Panel_ZipSpeed_Fast                 100
#define MV_Panel_ZipSpeed_Instant                0

/* Paneldrag */

#define MV_Paneldrag_Type_LeftUp                 0
#define MV_Paneldrag_Type_RightBottom            1

/* Panelconfig */

#define MV_Panelconfig_PosMode_Floating          0
#define MV_Panelconfig_PosMode_Attached          1

/* Prefwin_Panels */

#define	MV_Prefswin_Panels_UpdatePanels_Simple   0
#define	MV_Prefswin_Panels_UpdatePanels_Resize   1

#define MV_Panelwin_SaveConfig_Write			0x0001
#define MV_Panelwin_SaveConfig_Rename			0x0002
#define MV_Panelwin_SaveConfig_Add				0x0002
#define MV_Panelwin_SaveConfig_Delete			0x0004
#define MV_Panelwin_SaveConfig_MoveObject		0x0008
#define MV_Panelwin_SaveConfig_WindowPos		0x0010


/************************************************************************/
/************************************************************************/

/* DO NOT USE ANYMORE */

/************************************************************************/
/************************************************************************/

struct MP_Panel_SaveConfig {
	ULONG MethodID;
	APTR panel_lock;
	ULONG index;
};
/*
struct MP_Panelsupport_Saveconfig {
	ULONG MethodID;
	APTR pctx,pi;
};

struct MP_Panelsupport_WritePrefsStr {
	ULONG MethodID;
	ULONG ID;
	STRPTR value;
};

struct MP_Panelsupport_WritePrefsLong {
	ULONG MethodID;
	ULONG ID;
	ULONG value;
};
struct MP_Panelsupport_ReadPrefsStr  {
	ULONG MethodID;
	ULONG ID;
	STRPTR *storage;
};

struct MP_Panelsupport_ReadPrefsLong  {
	ULONG MethodID;
	ULONG ID;
	ULONG *storage;
};*/
struct MP_Panelwin_SaveConfig {
	ULONG MethodID;
	ULONG flags;
};
struct MP_Application_ObtainObject
{
	ULONG MethodID;
	struct TagItem *tags;
};
struct MP_Application_ReleaseObject
{
	ULONG MethodID;
	APTR pobj;
};


struct MP_PanelPrefs_PanelMessage
{
	ULONG MethodID;
	APTR pm;
};

//#endif

/************************************************************************/

#endif /* PANEL_TAGS_H */

