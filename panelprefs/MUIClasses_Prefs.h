#ifndef MUICLASSES_PREFS_H
#define MUICLASSES_PREFS_H

#include <utility/tagitem.h>
/*
void InitClasses(void);
void RemClasses(void);
struct IClass *GetClass(STRPTR classname);
struct MUI_CustomClass *GetMUIClass(STRPTR classname);
void fail(APTR app,char *str);
void init(void);


*/
/*
#define PATH_SIZE 256

#define DIR_LEFTUP 0
#define DIR_RIGHTBOTTOM 1
#define DIR_BOTTOM_DOCK 2
#define DIR_LEFT_DOCK 3
#define DIR_RIGHT_DOCK 4

      */              

#define PR_TAGBASE (TAG_USER|((0xFED5L<<16)+0x500))


enum {
	MA__PRdummy = (int)(PR_TAGBASE),
	MM_PanelPrefs_SetPrefsPool,
	MM_PanelListTree_Renumber,
	

	MM_PanelPrefs_ReadPrefs,
	MM_PanelPrefs_NewPanel,
	MM_PanelPrefs_DeleteObject,
	MM_PanelPrefs_RescanClasses,
	MM_BasebuttonPrefs_ImagePath,
	MA_PanelPrefs_ClassListObj,
	MM_ClassListGetExternalObject,
	MM_Panelprefs_BuildSettingsPanel, // ???
//	MM_PanelPrefs_FilesChanged,
//	MM_PanelPrefs_ObjectSetAttr,
	MM_PanelListtree_AddPanel,
//	MM_PanelListtree_ReadPanel,
	MM_PanelListtree_Rebuild,
	MM_PanelPrefs_NewActive,
	MM_PanelPrefs_ItemListChange,
	MA__PRdummyend
};

struct MP_PanelPrefs_SetPrefsPool
{
	ULONG MethodID;
	ULONG prefs_ID;
	APTR data;
	ULONG size;
};

struct MP_PanelListTree_Renumber
{
	ULONG MethodID;
	APTR start_node;
};

struct MP_PanelListtree_AddPanel
{
    ULONG MethodID;
    STRPTR panelname;	
};

struct MP_PanelListtree_Rebuild
{
    ULONG MethodID;
    STRPTR panel_name;
};

struct MP_ClassListGetExternalObject
{
	ULONG MethodID;
	STRPTR class_name;
	APTR *panel_item;
};

struct MP_PanelPrefs_ItemListChange 
{
	ULONG MethodID;
	LONG val;
};

struct MP_PanelPrefs_SetGlobalPrefs
{
	ULONG MethodID;
	ULONG ID,value;	
};
#endif
