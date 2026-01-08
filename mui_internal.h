#ifndef AMBIENT_MUI_INTERNAL_H
#define AMBIENT_MUI_INTERNAL_H
/*
 * $Id: mui_internal.h,v 1.16 2016/01/29 18:40:33 itix Exp $
 */

#include <sys/types.h>

/*
 * WARNING: do not ever use private MUI functions.
 * They are subject to change and the only people who can
 * use those are the ones close to MUI development. stuntzi
 * knows what/who uses them and can act accordingly.
 * Usual MUI apps don't need them anyway.
 */

#ifndef _isfloating
#define _isfloating(o) (getv(o, MUIA_Floating))
#endif

#ifndef MUIA_Window_Remember
#define MUIA_Window_Remember 0x8042f6b6
#endif

#ifndef MUIA_Window_ShowIconify
#define MUIA_Window_ShowIconify 0x8042bc26
#endif

#ifndef MUIA_Window_ShowAbout
#define MUIA_Window_ShowAbout 0x80429c1e
#endif

#ifndef MUIA_Window_Frontdrop
#define MUIA_Window_Frontdrop 0x80426411
#endif

#ifndef MUIA_Text_HiIndex
#define MUIA_Text_HiIndex 0x804214f5
#endif

#ifndef MUIM_Window_LockMode
#define MUIM_Window_LockMode 0x80423e12
#endif

#ifndef MUIM_Application_IdentifyLayer
#define MUIM_Application_IdentifyLayer 0x8042f5e1
#endif

#ifndef MUIM_WhichObject
#define MUIM_WhichObject 0x8042867c
#endif

#ifndef MUIA_Application_NoIconify
#define MUIA_Application_NoIconify 0x80426a3b
#endif

#ifndef MUIA_CustomBackfill
#define MUIA_CustomBackfill 0x80420a63
#endif

#ifndef MUIM_Application_SetPushMethodDelay
#define MUIM_Application_SetPushMethodDelay 0x80423713
#endif

#ifndef MUIA_Window_ShowPrefs
#define MUIA_Window_ShowPrefs 0x8042e262
#endif

#ifndef MUIA_Window_ShowJump
#define MUIA_Window_ShowJump 0x80422f40
#endif

#ifndef MUIA_Window_ShowSnapshot
#define MUIA_Window_ShowSnapshot 0x80423c55
#endif

#ifndef MUIA_Window_ShowPopup
#define MUIA_Window_ShowPopup 0x8042324e
#endif

#ifndef MUIA_Window_AllowTopMenus
#define MUIA_Window_AllowTopMenus 0x8042fe69
#endif

#ifndef MUIA_Window_BackfillHook
#define MUIA_Window_BackfillHook 0x80428863
#endif


#ifndef MADF_DROPABLE
#define MADF_DROPABLE (1<<13)
#endif

#ifndef MADF_VISIBLE
#define MADF_VISIBLE (1<<14)
#endif

#ifndef MADF_DISABLED
#define MADF_DISABLED (1<<15)
#endif

#ifndef _isvibible
#define _isvisible(obj) (_flags(obj) & MADF_VISIBLE)
#endif

#ifndef _isdisabled
#define _isdisabled(obj) (_flags(obj) & MADF_DISABLED)
#endif

#ifndef MUIM_Group_ExitChange2
#define MUIM_Group_ExitChange2 0x8042e541
#endif

#ifndef MUIM_Window_HandleRMB
#define MUIM_Window_HandleRMB 0x80421786
#endif

#ifndef MUIM_Backfill
#define MUIM_Backfill 0x80428d73
struct  MUIP_Backfill { ULONG MethodID; LONG left; LONG top; LONG right; LONG bottom; LONG xoffset; LONG yoffset; };
#endif

#define MUIMRI_RESIZEREDRAW (1<<5)

#ifndef MUIM_Application_KillPushMethod
#define MUIM_Application_KillPushMethod 0x80429954
#endif

#ifndef MADF_DRAWALL
#define MADF_DRAWALL ((1<<2) | (1<<11) | (1<<0))
#endif

#ifndef MUIA_Window_PanelWindow
#define MUIA_Window_PanelWindow 0x80429528
#endif

#ifndef MADF_SHOWME
#define MADF_SHOWME (1<< 9)
#endif

#ifndef MUIM_AdjustLayout
#define MUIM_AdjustLayout 0x80424d50
struct  MUIP_AdjustLayout { ULONG MethodID; LONG pass; };
#endif

#ifndef MUIC_Popfrimage
#define MUIC_Popfrimage "Popfrimage.mui"
#endif

#ifndef MUIA_Framedisplay_Spec
#define MUIA_Framedisplay_Spec 0x80421794
#endif

#ifndef MUIA_Imagedisplay_Spec
#define MUIA_Imagedisplay_Spec 0x8042a547
#endif

#ifndef MUIA_PopClosing
#define MUIA_PopClosing 0x8042ce8c
#endif

#ifndef MUIA_DoubleBufferClipped
#define MUIA_DoubleBufferClipped 0x8042f282
#endif

#ifndef MUIA_Floating
#define MUIA_Floating 0x80429753
#endif

#ifndef MUIA_String_DoBoopsiForward
#define MUIA_String_DoBoopsiForward 0x80427467
#endif

#ifndef MUIM_String_BoopsiForward
#define MUIM_String_BoopsiForward 0x80424137
struct  MUIP_String_BoopsiForward { ULONG MethodID; ULONG BoopsiID; };
#endif

#ifndef MUIA_List_TopPixel
#define MUIA_List_TopPixel 0x80429df3
#endif

#ifndef MUIA_List_TotalPixel
#define MUIA_List_TotalPixel 0x8042a8f5
#endif

#ifndef MUIA_List_VisiblePixel
#define MUIA_List_VisiblePixel 0x804273e9
#endif

#ifndef MUIM_List_Compare
#define MUIM_List_Compare                   0x80421b68 /* V20 */
struct  MUIP_List_Compare                   { ULONG MethodID; APTR entry1; APTR entry2; }; /* V20 */
#endif

#ifndef MUIM_List_EditEntry
#define MUIM_List_EditEntry                 0x804236a5 /* V20 */
struct  MUIP_List_EditEntry                 { ULONG MethodID; LONG pos; APTR editdata; LONG column; }; /* V20 */
#endif

#ifndef MUIM_List_EditStop
#define MUIM_List_EditStop                  0x8042ce3a /* V20 */
struct  MUIP_List_EditStop                  { ULONG MethodID; LONG pos; APTR editdata; }; /* V20 */
#endif

#ifndef MUIM_List_TitleChange
#define MUIM_List_TitleChange               0x80423791 /* V20 */
struct  MUIP_List_TitleChange               { ULONG MethodID; }; /* V20 */
#endif

#ifndef MUIV_List_EditEntry_Active
#define MUIV_List_EditEntry_Active      -1
#endif

#ifndef MUIA_Argstring_Contents
#define MUIA_Argstring_Contents             0x80429456 /* V20 [ISG] STRPTR */
#endif

#ifndef MUIA_Argstring_Template
#define MUIA_Argstring_Template             0x80422904 /* V20 [ISG] STRPTR */
#endif

#ifndef MUIA_Scrollgroup_AutoBars
#define MUIA_Scrollgroup_AutoBars           0x8042f50e /* V20 [ISG] BOOL */
#endif

#ifndef MUIF_PUSHMETHOD_VERIFY
#define MUIF_PUSHMETHOD_VERIFY       (1<<30UL)
#endif
#ifndef MUIF_PUSHMETHOD_NOTLAGGING
#define MUIF_PUSHMETHOD_NOTLAGGING   (1<<29UL)
#endif
#ifndef MUIF_PUSHMETHOD_SINGLE
#define MUIF_PUSHMETHOD_SINGLE       (1<<28UL)
#endif

#ifndef MUIM_FindObject
#define MUIM_FindObject                     0x8042038f /* private */ /* V13 */ /* stuntzi said it will go public */
#endif

#ifndef MUIM_Window_ActionIconify
#define MUIM_Window_ActionIconify           0x80422cc0 /* private */ /* V18 */
struct  MUIP_Window_ActionIconify           { ULONG MethodID; }; /* private */
#endif

#ifndef MUIM_List_QueryPosition
#define MUIM_List_QueryPosition             0x80420fff /* V20 */
struct  MUIP_List_QueryPosition             { ULONG MethodID; LONG entrynr; LONG column; LONG *x; LONG *y; }; /* private */
#endif

#ifndef MUIC_Title
#define MUIC_Title "Title.mui"
#endif

#ifndef MUIA_Group_PageMax
#define MUIA_Group_PageMax                  0x8042d777 /* V4  i.. BOOL              */ /* private */
#endif

#ifndef MUIA_Filepanel_Path
#define MUIA_Filepanel_Path                 0x80420792 /* V20 isg STRPTR            */
#endif

#ifndef MUIA_Panel_Terminate
#define MUIA_Panel_Terminate                0x804248e4 /* V20 .sg LONG              */
#endif

#ifndef MUIC_Filepanel
#define MUIC_Filepanel "Filepanel.mui"
#endif

#ifndef MUIA_Numeric_FormatFactor
#define MUIA_Numeric_FormatFactor           0x80420e5c /* V20 isg LONG              */ /* private */
#endif

#ifndef MUIM_List_CreateEditObject
#define MUIM_List_CreateEditObject          0x804219ae /* V21 */
#define MUIM_List_Edit                      0x8042843d /* V21 */
#define MUIM_List_EditDone                  0x80423ab3 /* V21 */
struct  MUIP_List_CreateEditObject          { ULONG MethodID; ssize_t row; ssize_t column; CONST_APTR entry; };
struct  MUIP_List_Edit                      { ULONG MethodID; ssize_t row; ssize_t column; };
struct  MUIP_List_EditDone                  { ULONG MethodID; ssize_t row; ssize_t column; CONST_APTR entry; Boopsiobject *editobj; ssize_t Aborted; };
#endif

#ifndef MUIA_List_Editable
#define MUIA_List_Editable                  0x8042f9b9 /* V21 isg BOOL              */
#endif

#ifndef MUIC_FSProtectionBits
#define FSProtectionBitsObject MUI_NewObject(MUIC_FSProtectionBits
#define MUIC_FSProtectionBits "FSProtectionBits.mui"
#define MUIA_FSProtectionBits_Flags         0x8042330c /* V21 isg size_t            */
#endif

#endif /* AMBIENT_MUI_INTERNAL_H */
