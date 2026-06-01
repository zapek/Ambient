#ifndef MUICLASSES_H
#define MUICLASSES_H

#include <utility/tagitem.h>

void InitClasses(void);
void RemClasses(void);
struct IClass *GetClass(STRPTR classname);
struct MUI_CustomClass *GetMUIClass(STRPTR classname);
void fail(APTR app,char *str);
void init(void);




#define PATH_SIZE 256
//#define PANEL_PATH "PROGDIR:"

#define DIR_LEFTUP 1
#define DIR_RIGHTBOTTOM 2
#define DIR_BOTTOM_DOCK 3
#define DIR_LEFT_DOCK 4
#define DIR_RIGHT_DOCK 5

                    

#define TAGBASE (TAG_USER|((0xFED5L<<16)+0x100))
enum {
	Menu__dummy = 0,
	Menu_Title_Project,
	Menu_Item_About,
	Menu_Item_New,
	Menu_Item_Open,
	Menu_Item_OpenNew,
	Menu_Item_Save,
	Menu_Item_SaveAs,
	Menu_Item_ExportPNG,
	Menu_Item_Quit,
	Menu_Title_Settings,
	Menu_Item_SD_Settings,
	Menu_Item_MUI_Settings,
	MENU__dummyend
};
enum {
	MA__dummy = (int)(TAGBASE),
//	MA_ImageGroup_ImageName,
//	MA_Panelwin_PrefspoolStartIndex,
//	MM_ImageGroup_Dump,
	MM_ClassGroup_ActiveMethod,
	MM_ClassGroup_AddMethod,
	MM_Application_NewPanel,
	MM_Application_LoadAll,
	MM_Application_SaveMainprefs,
	MM_Application_StartZipping,
	MM_Application_ZipTimer,
	MM_Application_SavePanel,
	MM_Application_FindPanel,
	MM_Application_DeletePanel,
	MM_Application_SetObjectAttr,
	MM_Application_DeleteObject,	
	MM_Application_InsertObject,
	MM_Application_MoveObject,
	MM_Application_ScanDirectory,
	MM_Application_PrefsPoolUpdate,
	MM_Panelgroup_AppMessage,
	MM_PanelPrefs_SetGlobalPrefs,
	
	MM_PANELOBJ_TIMER,
//	MM_PanelObj_ParseRSS,
//	MM_PanelObj_OpenNews,
//	MM_Signal_NewNews,
	MM_Panelgroup_ZippingFinished,
	MA_Panel_Lock,
	MA_Panel_PrefsIndex,
	MM_PanelWindow_LoadPanel,
	MM_PanelWindow_SetupPanel,
	MA_Panelgroup_GridMode,
//	MM_Start,
	MA_Panel_Type,
	MA_Panel_Extern_DisplayName,
	MA_Panel_Extern_ClassName,
	MA_Panel_Extern_Author,
	MA_Panel_Extern_Description,
	MA_Panel_Extern_Version,
	MA_Panel_Extern_Revision,
	MA_Panel_Extern_Image,

//define MA_Panelextern_SupportObject   MA_AmbientPanel_SupportObject
//#define MA_Panelsupport_Object         MA_AmbientSupport_Object
//#define MA_Panelsupport_PPool          MA_AmbientSupport_PPool
//#define MA_Panelsupport_PItem          MA_AmbientSupport_PItem
	MA_Panelgroup_Size,
	MA_Panelgroup_Horiz,
	MM_Panelgroup_RefreshRect,
//	MM_Panelsupport_Saveconfig,
//	MM_Application_CreatePanelitem,
//	MM_PanelPrefs_ReadPrefs,
//	MM_PanelPrefs_FilesChanged,
//	MM_PanelPrefs_ObjectSetAttr,
//	MM_PanelListtree_AddPanel,
//	MM_PanelListtree_ReadPanel,
//	MM_PanelListtree_Rebuild,
///	MM_PanelPrefs_NewActive,
//	MM_PanelPrefs_ItemListChange,

//	MM_PanelPrefs_NewPanel,
//	MM_PanelPrefs_DeleteObject,
//	MM_PanelPrefs_RescanClasses,
//	MM_BasebuttonPrefs_ImagePath,
//	MA_PanelPrefs_ClassListObj,
//	MM_ClassListGetExternalObject,
//	MM_AmbientPanel_BuildSettingsPanel,
//	MM_Panel_Obsolete,
	MM_Panel_PrefsUpdate,
	MM_Panel_Drop,
	MM_Application_Modus,
	MA__dummyend
};

/*
struct MP_ClassListGetExternalObject
{
	ULONG MethodID;
	STRPTR class_name;
	APTR *panel_item;
};*/

struct MP_Application_NewPanel {
	ULONG MethodID;
	STRPTR p_name;
};
/*
struct MP_Application_CreatePanelitem {
	ULONG MethodID;
	ULONG type;
	APTR obj;
	APTR prefspool;
	ULONG index;
};
*/
struct MP_Application_StartZipping
{
	ULONG MethodID;
	Object *group_obj;
	ULONG type; /*leftup vs rightbottom*/
	ULONG direction; /* true zip false unzip*/
	ULONG width,height;
	ULONG drag; /*min width/height for draggadgets*/
};


struct MP_Application_DeleteObject
{
	ULONG MethodID;
	STRPTR panel_name;
	ULONG id;
};

struct MP_Application_InsertObject
{
	ULONG MethodID;
	STRPTR panel_name;
	LONG id;
	APTR obj;
	LONG pos;	
};

struct MP_Application_MoveObject
{
	ULONG MethodID;
	STRPTR dst_panel_name;
	LONG dst_id;
	LONG pos;
	STRPTR src_panel_name;
	LONG src_id;
};

struct MP_Application_SavePanel
{
	ULONG MethodID;
	STRPTR p_name;
};

struct MP_Application_FindPanel
{
	ULONG MethodID;
	STRPTR p_name;
};

struct MP_Application_DeletePanel
{
	ULONG MethodID;
	STRPTR p_name;
};

struct MP_Application_SetObjectAttr
{
	ULONG MethodID;
	STRPTR p_name;
	ULONG obj_id;
	ULONG ti_Tag,ti_Data;
};
struct MP_Application_ScanDirectory
{
	ULONG MethodID;
	STRPTR path;
};

struct MP_Panelgroup_AppMessage
{
	ULONG MethodID;
	APTR appmsg;	
};

struct MP_Panelgroup_ZippingFinished
{
	ULONG MethodID;
	ULONG direction;	
};


struct MP_Panelbutton_Launch
{
	ULONG MethodID;
	LONG NumArgs;
	struct WBArg *args;
};

struct MP_Panel_Drop
{
	ULONG MethodID;
	ULONG x,y;
	STRPTR path;
};

/*
struct MP_PanelListtree_AddPanel
{
    ULONG MethodID;
    STRPTR panelname;	
};

struct MP_PanelListtree_ReadPanel
{
    ULONG MethodID;
    APTR node;
};*/
/*
struct MP_PanelListtree_Rebuild
{
    ULONG MethodID;
    STRPTR panel_name;
};

struct MP_PanelPrefs_ObjectSetAttr
{
	ULONG MethodID;
	ULONG target;
	ULONG tag,data;	
};*/
/*
struct MP_PanelPrefs_SetGlobalPrefs
{
	ULONG MethodID;
	ULONG ID,value;	
};
*/

struct MV_BasebuttonPrefs_ImagePath
{
	ULONG MethodID;
	STRPTR image;
};
/*
struct MP_PanelPrefs_ItemListChange 
{
	ULONG MethodID;
	LONG val;
};*/

struct MP_Application_PrefsPoolUpdate
{
	ULONG MethodID;
	STRPTR panel_name;
	ULONG object_ID;
	ULONG prefs_ID;
};

struct MP_Panel_PrefsUpdate
{
	ULONG MethodID;
	APTR ppool;
	ULONG object_ID;
	ULONG prefs_ID;
};

struct MP_Application_Modus
{
	ULONG MethodID;
	STRPTR panel_name;
	ULONG modus;
};

#define MV_Application_InsertObject_PosNone		0
#define MV_Application_InsertObject_PosTail		-1
#define MV_Application_InsertObject_PosHead		-2
#define MV_Application_InsertObject_PosRootTail		-3
#define MV_Application_InsertObject_PosRootHead		-4

/*
#define MV_PanelPrefs_ObjectSetAttr_Target_Object	0
#define MV_PanelPrefs_ObjectSetAttr_Target_Window	1
#define MV_PanelPrefs_ObjectSetAttr_Target_Group	2
#define MV_PanelPrefs_ObjectSetAttr_Target_SubWindow	3
#define MV_PanelPrefs_ObjectSetAttr_Target_SubGroup		4*/
#endif
