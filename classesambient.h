#ifndef CLASSES_AMBIENT_H
#define CLASSES_AMBIENT_H

/*
**
** This file contains all external headers required for any
** external ambient module type.
**
** Currently supported class types:
**
** Panel:
**       QUERYCLASS_AMBIENT / QUERYSUBCLASS_AMBIENT_PANEL
**
*/

#include <exec/types.h>

/************************************************************************/

#define MV_AmbientClassType_External   10

/* generic support object */

#define MA_AmbientSupport_Object              0xFECA2000
#define MA_AmbientSupport_PPool               0xFECA2001
#define MA_AmbientSupport_PItem               0xFECA2002

#define MM_AmbientSupport_WritePrefsStr       0xFECA2020
#define MM_AmbientSupport_WritePrefsLong      0xFECA2021
#define MM_AmbientSupport_ReadPrefsStr        0xFECA2022
#define MM_AmbientSupport_ReadPrefsLong       0xFECA2023

/************************************************************************/

struct MP_AmbientSupport_SaveConfig {
	ULONG   MethodID;
	APTR    pctx;
	APTR    pi;
};

struct MP_AmbientSupport_WritePrefsStr {
	ULONG   MethodID;
	ULONG   ID;
	STRPTR  value;
};

struct MP_AmbientSupport_WritePrefsLong {
	ULONG   MethodID;
	ULONG   ID;
	ULONG   value;
};
struct MP_AmbientSupport_ReadPrefsStr  {
	ULONG   MethodID;
	ULONG   ID;
	STRPTR *storage;
};

struct MP_AmbientSupport_ReadPrefsLong  {
	ULONG   MethodID;
	ULONG   ID;
	ULONG   *storage;
};

/************************************************************************/

/* panel attributes */

#define MA_AmbientPanel_Type                  0xFECA2040  /* (ULONG) must be MV_AmbientClassType_External */
#define MA_AmbientPanel_DisplayName           0xFECA2041  /* (STRPTR) class display name */
#define MA_AmbientPanel_ClassName             0xFECA2042  /* (STRPTR) class disk name */
#define MA_AmbientPanel_Author                0xFECA2043  /* (STRPTR) class developer */
#define MA_AmbientPanel_Description           0xFECA2044  /* (STRPTR) class description */
#define MA_AmbientPanel_Version               0xFECA2045  /* (ULONG)  class version */
#define MA_AmbientPanel_Revision              0xFECA2046  /* (ULONG)  class revision */
#define MA_AmbientPanel_Image                 0xFECA2047  /* (ULONG*) ARGB image shown in prefs */
#define MA_AmbientPanel_SupportObject         0xFECA2048  /* (Object*) object for methods below */
#define MA_AmbientPanel_Group_Size            0xFECA2049
#define MA_AmbientPanel_Group_Horiz           0xFECA204a
#define MA_AmbientPanel_SettingsPanel    	  0xFECA204b

/* panel methods */

#define MM_AmbientPanel_Group_RefreshRect     0xFECA2100  /* use on parent of your class only */
#define MM_AmbientPanel_SaveConfig            0xFECA2101  /* */
#define MM_AmbientPanel_BuildSettingsPanel    0xFECA2102  /* build class settings group */

/* method prototype */

struct MP_AmbientPanel_SaveConfig {
	ULONG   MethodID;
	APTR    pctx;
	ULONG   index;
};

/* defines */

#define AMBIENTPANEL_IMAGE_WIDTH  26
#define AMBIENTPANEL_IMAGE_HEIGHT 20

/************************************************************************/

#endif /* CLASSES_AMBIENT_H */
