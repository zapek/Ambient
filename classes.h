#ifndef AMBIENT_CLASSES_H
#define AMBIENT_CLASSES_H

/*
 * $Id: classes.h,v 1.61.2.1 2024/01/20 02:56:53 piru Exp $
 */

#include <utility/tagitem.h>
#include <dos/exall.h>

#include "sizes.h"
#include "iconview.h"

/* Custom methods, what's the matter ? */
#define MTAGBASE (TAG_USER|((0xFECAL<<16)+0))
#define MTAGBASE_PUBLIC (MTAGBASE + 4096)

/*
 * Yes, stuntzi gave me that serial number,
 * let's hope the project doesn't end up in
 * a big pile of shit :)
 *
 * Actually it did with all the mess that happened.
 * As usual it's all stuntzi's fault!
 */

/* Constructor/destructor stuff */
ULONG classes_init(void);
void classes_cleanup(void);

/* Classes */
#define DEFCLASS(s) ULONG create_##s##class(void); \
	APTR get##s##class(void); \
	APTR get##s##classroot(void); \
	void delete_##s##class(void)

DEFCLASS(window);
DEFCLASS(icon);
DEFCLASS(app);
DEFCLASS(iconview);
DEFCLASS(about);
DEFCLASS(aboutmos);
DEFCLASS(infowin);
DEFCLASS(capacitytext);
DEFCLASS(addtext);
DEFCLASS(execute);
DEFCLASS(ttlistview);
DEFCLASS(smartreq);
DEFCLASS(smarttext);
DEFCLASS(dropeffect);
DEFCLASS(progresswin);
DEFCLASS(formatwin);
DEFCLASS(renamewin);
DEFCLASS(format);
DEFCLASS(formatlist);
DEFCLASS(menuitem);
DEFCLASS(menu);
DEFCLASS(menustrip);
DEFCLASS(faststring);
DEFCLASS(makedirwin);
DEFCLASS(makelinkwin);
DEFCLASS(logo);
DEFCLASS(ttlist);
DEFCLASS(cxwin);
DEFCLASS(cxlist);
DEFCLASS(soundwin);
DEFCLASS(bgrender);
DEFCLASS(fasttitlegroup);
DEFCLASS(fasttitletext);
DEFCLASS(mimegroup);
DEFCLASS(mimelisttree);
DEFCLASS(mimeadjustwin);
DEFCLASS(mimeadjustgroup);
DEFCLASS(mimeadjustlist);
DEFCLASS(mimeactionwin);
DEFCLASS(mimeactiongroup);
DEFCLASS(infoicongroup);
DEFCLASS(graph);
DEFCLASS(sysinfowin);
DEFCLASS(navigation);
#if USE_CRAWLER
DEFCLASS(crawl);
#endif
DEFCLASS(editstring);
DEFCLASS(viewgroup);
DEFCLASS(viewsizegroup);
DEFCLASS(view);
DEFCLASS(gview);
#if USE_VIEW_IMAGE
DEFCLASS(imageview);
#endif
#if USE_VIEW_HEX
DEFCLASS(hexview);
#endif
DEFCLASS(textview);
DEFCLASS(listview);
DEFCLASS(listviewlist);
DEFCLASS(boopsiview);
DEFCLASS(panelwin);
DEFCLASS(panelgroup);
DEFCLASS(panelspacer);
DEFCLASS(panelseparator);
DEFCLASS(panelviewwatcher);
DEFCLASS(panelbookmarks);
DEFCLASS(panelitem_list);
DEFCLASS(panelslidersize);
DEFCLASS(prefswin_main);
DEFCLASS(prefswin_list);
DEFCLASS(prefswin_background);
DEFCLASS(prefswin_icondisplay);
DEFCLASS(prefswin_miscellaneous);
DEFCLASS(prefswin_bookmarks);
DEFCLASS(prefswin_clilaunch);
#if USE_DROP_EFFECT_PREFS
DEFCLASS(prefswin_dragdrop);
#endif
DEFCLASS(prefswin_lister);
DEFCLASS(prefswin_panel);
DEFCLASS(panelsliderspeed);
DEFCLASS(prefswin_mime);
DEFCLASS(prefswin_window);
DEFCLASS(prefswin_advanced);
DEFCLASS(prefswin_keyboard);
DEFCLASS(advancedprefsgroup);
DEFCLASS(advancedprefslist);
DEFCLASS(toolbutton);
DEFCLASS(toolbutton_action);
DEFCLASS(toolbutton_spacer);
DEFCLASS(toolbutton_history);
DEFCLASS(toolbutton_viewswitch);
DEFCLASS(toolbutton_bookmarks);
DEFCLASS(toolbar);
DEFCLASS(toolbargroup);
DEFCLASS(clickpath);
DEFCLASS(clickpathbutton);
DEFCLASS(statusbar);
DEFCLASS(virtgroup);
DEFCLASS(searchbar);
DEFCLASS(searchstring);
DEFCLASS(destselectwin);
DEFCLASS(actiondispatcher);
DEFCLASS(devicelist);
DEFCLASS(listviewentry);
DEFCLASS(actionlist);
DEFCLASS(actionedit);
DEFCLASS(actioneditwin);
DEFCLASS(notification);
DEFCLASS(selectwin);
DEFCLASS(findwin);
DEFCLASS(findresultlist);
DEFCLASS(columnslist);
DEFCLASS(viewselectwin);
DEFCLASS(keyshortcut);
DEFCLASS(actioneditor);
DEFCLASS(actioneditorwin);
DEFCLASS(bgrefreshdelayslider);
DEFCLASS(patternrenamewin);
DEFCLASS(bookmarklist);
DEFCLASS(addbookmarkwin);

DEFCLASS(panelbasebutton);
DEFCLASS(panelcommandbutton);
DEFCLASS(panelsubpanelbutton);
DEFCLASS(paneldirpanelbutton);
DEFCLASS(panelexternalsupport);
DEFCLASS(panelsubwin);
DEFCLASS(paneldrag);
DEFCLASS(panelitem_list);
DEFCLASS(panellisttree);
DEFCLASS(panelclasslist);
DEFCLASS(panelmessenger);
DEFCLASS(panelmessengerfamily);
DEFCLASS(iconzoomslider); // bitRocky
DEFCLASS(mimetype);

enum {
	MA_dummy = (int)(MTAGBASE),

	/* Icon  (warning, add methods at the bottom of it as it's shared with iconlib) */
	MA_Icon_Name,                     /* [IGS] STRPTR to the icon name displayed below the icon */
	MA_Icon_Path,                     /* [IGS] STRPTR to the full path of the object (for deficons: real object) */
	MA_Icon_PathInfo,                 /* [IGS] STRPTR to the full path of the .info object (or deficon's .info) */
	MA_Icon_Type,                     /* [IGS] ULONG Icon type (disk, drawer, etc..), see MV_Icon_Type_#? */
	MA_Icon_FileType,                 /* [IGS] ULONG filetype (device, directory, file, link, etc..) */
	MA_Icon_StackSize,                /* [IGS] ULONG stack size in bytes */
	MA_Icon_DefaultTool,              /* [IGS] STRPTR default tool */
	MA_Icon_Y,                        /* [IGS] ULONG Y-pos (in the icon, not real) */
	MA_Icon_X,                        /* [IGS] ULONG X-pos (in the icon, not real) */
	MA_Icon_HasPos,                   /* [IGS] ULONG has real position */
	MA_Icon_WindowTop,                /* [IGS] ULONG Window Y-pos */
	MA_Icon_WindowLeft,               /* [IGS] ULONG Window X-pos */
	MA_Icon_WindowWidth,              /* [IGS] ULONG Window width */
	MA_Icon_WindowHeight,             /* [IGS] ULONG Window height */
	MA_Icon_OffsetY,                  /* [IGS] ULONG Window scrollbar Y offset */
	MA_Icon_OffsetX,                  /* [IGS] ULONG Window scrollbar X offset */
	MA_Icon_HasDrawerData,
	MA_Icon_ViewMode,                 /* [IGS] ULONG viewmode, see MV_Icon_ViewMode_#? */
	MA_Icon_ImageType,                /* [IGS] ULONG imagetype, see MV_Icon_ImageType_#? */
	MA_Icon_ImageNormal,              /* [IGS] APTR normal image icon */
	MA_Icon_ImageSelected,            /* [IGS] APTR selected image icon */
	MA_Icon_ImageNormalWidth,         /* [.G.] */
	MA_Icon_ImageNormalHeight,        /* [.G.] */
	MA_Icon_ImageSelectedWidth,       /* [.G.] */
	MA_Icon_ImageSelectedHeight,      /* [.G.] */
	MA_Icon_DiskType,
	MA_Icon_MsgPort,                  /* [..S] struct MsgPort * for appicons */
	MA_Icon_AppAddress,               /* [..S] appmessage address to know which one to remove */
	MA_Icon_AppID,
	MA_Icon_AppUserData,
	MA_Icon_Selected,
	MA_Icon_Left,
	MA_Icon_Top,
	MA_Icon_TextColor,
	MA_Icon_TextBgColor,
	MA_Icon_AFont,
	MA_Icon_FontSpace,
	MA_Icon_ToolTypeList,            /* [.G.] struct MinList *; returns the tooltype list */
	MA_Icon_IsDefault,
	MA_Icon_ImmediateUpdate,
	MA_Icon_IsShortcut,              /* [IGS] TRUE if the icon is a shortcut icon */
	MA_Icon_Highlighted,
	MA_Icon_ViewID,                  /* [IG.] Each icon MUST have viewid assigned if you want to control it's size */
	MA_Icon_SizeAdjustment,          /* [IGS] */
	MM_Icon_AddBitMap,
	MM_Icon_AddImage,                /* iconlib build only */
	MM_Icon_ToolTypesNum,            /* iconlib build only, number of tooltypes */
	MM_Icon_InsertToolType,
	MM_Icon_ClearToolTypes,
	MM_Icon_End,
	MM_Icon_Select,
	MM_Icon_SetIcon,
	MM_Icon_ErrorString,
	MM_Icon_GetToolTypes,
	MM_Icon_AddAncillary,
	MM_Icon_GetAncillary,
	MM_Icon_CreateBitMap,
	MM_Icon_UpdateBitMapBuffer,
	MM_Icon_UpdateBitMapBuffer2,
	MM_Icon_Scale,
	/* add MA/MM_Icon stuff there to avoid iconlib rebuilds */
	MM_Icon_Click,                   /* called everytime an icon is clicked to detect double clicks */
	MM_Icon_FadeIn,
	MM_Icon_Generate,                /* should be called after change of imaginery or title */
	MA_Icon_SortMode,
	MA_Icon_DoubleClick,             /* set to TRUE when an icon had a double click condition */
	MA_Icon_Infowin,                 /* set to TRUE if that icon is one that is in an infowin window */
	MA_Icon_Fade,
	MA_Icon_NameInfo,
	MA_Icon_ThumbIt,
	MA_Icon_IsLink,                  /* simple way to handle links */
	MA_Icon_AdditionalText,          /* additional text displayed under icon label */
	MA_Icon_GaugePercent,            /* percentvalue of the spacegauge */
	MA_Icon_SmallFont,
	MA_Icon_FileSize,
	MA_Icon_FileDate,
	MA_Icon_MimeType,                /* pointer to mimetype class */
	MA_Icon_IconPath,
	MA_Icon_DeviceType,
	MA_Icon_ClickPosX,               /* [.SG] mouse click position relative to top left corner */
	MA_Icon_ClickPosY,               /* [.SG] mouse click position relative to top left corner */
	MA_Icon_Version,
	MA_Icon_MD5,
	MA_Icon_Refine,                  /* refine icon as it only got 'wild-guessed' image */
	MA_Icon_Image,
	MA_Icon_Bitmap,
	MA_Icon_BitmapObject,
	MA_Icon_Hash,
	MA_Icon_Reference,               /* reference icon from which image is taken */
	MA_Icon_ReferenceCount,          /* number of icons that use our's one as reference */

	/* About */
	MM_About_Close,
	MM_About_OpenMUI,
	MM_About_GotoURL,
	MM_About_Gag,
	MM_About_TriggerObscure,

	/* Application */
	MM_Application_CheckDelayedDialog,
	MM_Application_Cleanup,
	MM_Application_DisplayDelayedDialog,
	MM_Application_FindWindowByID,
	MM_Application_FindWindowByName,
	MM_Application_FindWindowByType,
	MM_Application_FindWindowByUserData,
	MM_Application_Open_AboutWindow,
	MM_Application_Open_AboutMorphOSWindow,
	MM_Application_Open_CxWindow,
	MM_Application_Open_SystemInfoWindow,
	#if !USE_LEGACY
	MM_Application_Open_SystemLog,
	#endif
	MM_Application_SavePrefs,
	MM_Application_LoadPrefs,
	MM_Application_CreateIcon,
	MM_Application_DisposeObject,
	MM_Application_DisposeObjectArray,
	MM_Application_AddAppIcon,
	MM_Application_DisplayUpdate,
	MM_Application_Delete_Ok,
	MM_Application_ChangeScreenTitle,
	MM_Application_RestoreScreenTitle,
	MM_Application_Open_ExecuteWindow,
	MM_Application_DOSNotify,
	MM_Application_DisposeWindow,
	MM_Application_CreateActionDispatcher,
	MM_Application_DeleteActionDispatcher,
	MM_Application_DeleteAppIcon,
	MM_Application_CreateProgresswin,
	MM_Application_CreateDestSelectorwin,
	MM_Application_CreatePatternRenamewin,
	MM_Application_CreateSmartReq,
	MM_Application_ReclaimThreads,
	MM_Application_CountWindows,
	MM_Application_NewShell,
	MM_Application_Shutdown,
	MM_Application_DoRexx,
	MM_Application_CreatePanelwin,
	MM_Application_CreatePanelitem,
	MM_Application_CreateInfowin,
	MM_Application_CreateIconinfo,
	MM_Application_CreateWindow,
	MM_Application_WindowDoMethodByAttr,
	MM_Application_RootDoMethodByAttr,
	MM_Application_AddBackground,
	MM_Application_LoadBackground,
	MM_Application_CenterBackground,
	MM_Application_SetFont,
	MM_Application_SetDebug,
	MM_Application_DoDebug,
	MM_Application_EnableDOSNotify,
	MM_Application_ReloadIcons,
	MM_Application_RemoveShortcut,
	MM_Application_CommitStorage,
	MM_Application_OpenDevicesWindow, /* aka MyComputer aka My MorphOS aka Oma MorphOS */
	MM_Application_ReloadBackgrounds,
	MM_Application_CleanupCache,
	MM_Application_SoundControl,      /* method to control sound (to be used from other tasks) */
	MM_Application_GetMimeType,
	MM_Application_GetDeficonPath,
	MA_Application_BackgroundRefreshDelay,
	MA_Application_NextID,
	MA_Application_RootWindow,
	MA_Application_PanelClassList,    /* Kronos */
	MM_Application_Trash_Ok,
	MM_Application_UpdateTransitionEffect,
	/* Window */
	MM_Window_Close,
	MM_Window_UpdateBackground,
	MM_Window_ReloadBackground,
	MM_Window_FillBackground,
	MM_Window_SetTitle,
	MM_Window_DoView,
	MM_Window_Aborted,
	MM_Window_LoadURI,
	MM_Window_UpdateURI,
	MM_Window_Reload,
	MM_Window_UpdateStatusbar,
	MM_Window_UpdateUI,
	MM_Window_MenuAction,
	MM_Window_Iconify,
	MM_Window_IsActive, /* bitRocky: called if MUIA_Window_Activate changes */
	MM_Window_ZoomIcons, // bitRocky: called when using the iconzoom slider
	MA_Window_LeftOffset,
	MA_Window_TopOffset,
	MA_Window_Path,                   /* [IGS] STRPTR to the fully qualified name or MV_Window_Path_Devices/Root */
	MA_Window_ID,
	MA_Window_Type,
	MA_Window_UserData,
	MA_Window_BgPen,
	MA_Window_IsRoot,
	MA_Window_MIMEctx,
	MA_Window_DoBackfill,
	MA_Window_DTP,
	MA_Window_BGMode,
	MA_Window_Viewobj,
	MA_Window_EnableDOSNotify,
	MA_Window_Browser,
	MA_Window_IsIconified,

	/* Iconview */
	MM_Iconview_RefreshLasso,
	MM_Iconview_Redraw,
	MM_Iconview_AllocPens,
	MM_Iconview_Layout_Find,
	MM_Iconview_Layout_CheckFit,
	MM_Iconview_Empty,
	MM_Iconview_RemoveByName,
	MM_Iconview_Unselect,
	MM_Iconview_RemoveDefaults,
	MM_Iconview_DoSort,
	MM_Iconview_BuildSnapshotlist,
	MM_Iconview_SelectLasso,
	MM_Iconview_SaveShortcuts,
	MM_Iconview_SaveShortcuts2,
	MM_Iconview_AddShortcut,
	MM_Iconview_FindShortcut,
	MM_Iconview_AbortLasso,
	MM_Iconview_AddIcon,
	MM_Iconview_DeleteIcon,
	MM_Iconview_DeleteAppIcon,
	MM_Iconview_DoLayout,
	MM_Iconview_ClearLayout,
	MM_Iconview_ClearBubble,
	MM_Iconview_UpdateDriveInfo,
	MM_Iconview_FocusIcon,
	MM_Iconview_SnapshotIcon,
	MM_Iconview_UnsnapshotIcon,
	MM_Iconview_StartScanner,
	MM_Iconview_Relayout,
	MM_Iconview_ShowPreview,
	MA_Iconview_IconTextColor,
	MA_Iconview_IconTextBgColor,
	MA_Iconview_IconTextEffect,
	MA_Iconview_Path,
	MA_Iconview_NumIcons,
	MA_Iconview_IsRoot,
	MA_Iconview_SortMode,
	MA_Iconview_Qualifier,           /* Only for usage in child classes  = icons. Returns current qualifier */
	MA_Iconview_ShowMyMorphos,


	/* Special thread methods */
	MM_Thread_Finished,              /* the process has finished its job and the object can safely go away if it wants to */

	/* Prefswin */
	MM_Prefswin_Store,
	MA_Prefswin_Object,

	MM_Prefswin_Main_SelectChange,
	MM_Prefswin_Main_SetPage,
	MM_Prefswin_Main_Close,
	
	/* Prefswin/ Icon */
	MM_Prefswin_Icondisplay_AdjustSize,

	/* Prefswin/ Miscellaneous */
	MM_Prefswin_Misc_SetDefIconPath,

	/* Prefswin/ Window */
	MM_Prefswin_Window_GetSizeFromView,
	MM_Prefswin_Window_GetSizeFromViewAck,
	MM_Prefswin_Window_AddStatusbarPlaceholder,

	/* Prefswin/ Keyboard */
	MM_Prefswin_Keyboard_AddShortcut,
	MM_Prefswin_Keyboard_RemoveShortcut,

	/* Prefswin/ Background */
	MM_Prefswin_Background_Invalidate_Delay,

	/* Prefswin/ Bookmarks */
	MM_Prefswin_Bookmarks_Selected,
	MM_Prefswin_Bookmarks_Modified,

	/* Prefswin/ Advanced */
	MM_Advancedprefsgroup_UpdateGadgets,
	MM_Advancedprefsgroup_ApplyValue,
	MM_Advancedprefsgroup_ResetValue,

	/* Prefswin/ Panel */   /* Kronos */
	MM_Prefswin_Panels_Selected,
	MM_Prefswin_Panels_Horiz,
	MM_Prefswin_Panels_Position,
	MM_Prefswin_Panels_Size,
	MM_Prefswin_Panels_Depth,
	MM_Prefswin_Panels_DragMode,
	MM_Prefswin_Panels_HideDragBar,
	MM_Prefswin_Panels_HasClosed,
	MM_Prefswin_Panels_Delete,
	MM_Prefswin_Panels_New,
	MM_Prefswin_Panels_BackMode,
	MM_Prefswin_Panels_BackColor,
	MM_Prefswin_Panels_RescanClasses,
	MM_Prefswin_Panels_ItemListChange,
	MM_Prefswin_Panels_UpdatePanels,

	MM_PanelListTree_Refresh,
	MM_PanelListTree_CreateItem,
	MM_PanelListTree_RefreshNode,
	MM_PanelItemList_RefreshList,
	MM_PanelListTree_FindUData,


	/* Infowin */
	MM_Infowin_Close,
	MM_Infowin_TTSetString,
	MM_Infowin_TTCopyString,
	MM_Infowin_TTAdd,
	MM_Infowin_Scan,
	MM_Infowin_UpdateSize,
	MM_Infowin_Save,
	MM_Infowin_UpdateVersion,
	MM_Infowin_Version,
	MM_Infowin_ChangeMode,
	MM_Infowin_UpdateMD5sum,
	MM_Infowin_MD5sum,
	MM_Infowin_Changed,
	MM_Infowin_OpenActionEditor,
	MA_Infowin_Iconobj,
	MA_Infowin_RXID,
	MA_Infowin_Infodata,

	/* CapacityText */
	MM_CapacityText_Build,
	MA_CapacityText_Total,

	/* Execute */
	MM_Execute_Close,
	MM_Execute_CheckContent,

	/* SmartReq */
	MM_SmartReq_Enqueue,
	MM_SmartReq_Change,
	MM_SmartReq_Pressed,
	MM_SmartReq_Ask,

	/* SmartText */
	MA_SmartText_ViewChars,

	/* DropEffect */
	MA_DropEffect_Mode,
	MA_DropEffect_TintVal,
	MA_DropEffect_BrightenVal,
	MA_DropEffect_DarkenVal,
	MA_DropEffect_TintfadeVal,
	MA_DropEffect_Label,

	/* Progresswin */
	MM_Progresswin_Update,      /* update the progresswin gauge/status */
	MM_Progresswin_Stop,
	MM_Progresswin_Close,
	MM_Progresswin_InitProgress,
	MA_Progresswin_Look,        /* [I..] sets the state of the progresswin, see the flags below */
	MA_Progresswin_Thread,      /* [I..] thread to send possible CTRL_C to */
	MA_Progresswin_Refwin,

	MA_DragDrop_Path,
	MA_DragDrop_Type,

	/* Format */
	MM_Format_ChangeOptions,
	MM_Format_Format,
	MM_Format_RemoveGauge,
	MM_Format_Busy,
	MA_Format_NameStatus,
	MA_Format_DeviceInfo,       /* [..G] struct device_info * */
	MA_Format_Formattable,      /* [..G] */

	/* Formatwin */
	MM_Formatwin_SetDevice,
	MM_Formatwin_Close,
	MM_Formatwin_Cancel,
	MM_Formatwin_SetText,
	MM_Formatwin_SetGauge,
	MM_Formatwin_Format,
	MM_Formatwin_FormatReally,

	/* Listview */
	MM_Listview_AddFile,
	MM_Listview_AddFiles,
	MM_Listview_RemoveByName,
	MM_Listview_RemoveFiles,
	MM_Listview_Sort,
	MM_Listview_ShowDevices,
	MM_Listview_ShowFiles,
	MM_Listview_UpdateView,
	MM_Listview_Clear,
	MM_Listview_JumpTo,
	MM_Listview_AllocPens,
	MM_Listview_FindIcon,
	MM_Listview_UpdateIconsSize,
	MM_Listview_SetupIcons,
	MM_Listview_CleanupIcons,
	MM_Listview_InvalidateIcon,
	MM_Listview_CreateDefaultImages,
	MM_Listview_DeleteDefaultImages,
	MM_Listview_RunIconThread,
	MM_Listview_RunFileTypeThread,
	MM_Listview_RunVersionThread,
	MM_Listview_RunMD5Thread,
	MM_Listview_RunDirSizeThread,
	MM_Listview_RunPreviewThread,
	MM_Listview_ShowPreview,
	MM_Listview_UpdateColumns,
	MM_Listview_UpdateEntriesPositions,
	MM_Listview_UpdateStatusBar,
	MM_Listview_IsColumnVisible,
	MM_Listview_GetColumnOrder,
	MA_Listview_VScroller,
	MA_Listview_HScroller,
	MA_Listview_Top,
	MA_Listview_Left,
	MA_Listview_GetSizes,
	MA_Listview_Version,
	MA_Listview_MD5,
	MA_Listview_SortDirection,
	MA_Listview_SortColumn,
	MA_Listview_IconSize,
	MA_Listview_Preview,
	MA_Listview_InEditMode, // TRUE, while inline-editing is active

	/* ListviewEntry */
	MM_ListviewEntry_Redraw,
	MA_ListviewEntry_ListObject,
	MA_ListviewEntry_Position,
	MA_ListviewEntry_Pool,
	MA_ListviewEntry_IsVisible,

	/* Listviewlist */
	MM_Listviewlist_DoubleClick,
	MM_Listviewlist_EntryClick,
	MM_Listviewlist_SelectChange,
	MA_Listviewlist_Path,
	MA_Listviewlist_Type,
	MA_Listviewlist_IconDisplay,

	/* Renamewin */
	MM_Renamewin_Rename,
	MM_Renamewin_Close,
	MA_Renamewin_Path,
	MA_Renamewin_Name,
	MA_Renamewin_FileType,
	MA_Renamewin_NoIcon,
	MA_Renamewin_IconType,       /* MV_Icon_Type_#? */

	/* Copymode */
	MM_Copymode_Change,
	MA_Copymode_Mode,
	MA_Copymode_Bufsize,
	MA_Copymode_Readaheadsize,

	/* Rexx icon/iconview shared methods */
	MM_Rexx_Snapshot,
	MM_Rexx_Unsnapshot,
	MM_Rexx_Rename,
	MM_Rexx_SetName,
	MM_Rexx_SetComment,

	/* Makedirwin */
	MM_Makedirwin_Close,
	MM_Makedirwin_Makedir,
	MA_Makedirwin_Path,
	MA_Makedirwin_Icon,

	/* Makelinkwin */

	MM_Makelinkwin_Close,
	MM_Makelinkwin_Makelink,
	MA_Makelinkwin_From,

	/* Logo */
	MM_Logo_Flash,
	MM_Logo_DoFlash,
	MA_Logo_Egg,
	MA_Logo_Flashing,
	MA_Logo_Type,

	/* TTListview */
	MA_TTList_IsList,
	MA_TTList_Changed,

	/* Cxwin */
	MM_Cxwin_Rescan,
	MM_Cxwin_Close,
	MM_Cxwin_SetStatus,

	/* Cxlist */
	MM_Cxlist_InitChange,
	MM_Cxlist_TryAdd,
	MM_Cxlist_ExitChange,
	MM_Cxlist_CxNotify,
	MM_Cxlist_SetStatus,

	/* Bgrender */
	MM_BGRender_Change,
	MA_BGRender_Mode,
	MA_BGRender_Color,
	MA_BGRender_Root,
	MA_BGRender_NeedsBackground,
	MA_BGRender_Label,

	/* Viewgroup */
	MA_Viewgroup_IsView,
	MA_Viewgroup_ID,
	MA_Viewgroup_MIMEctx,
	MA_Viewgroup_VScroller,
	MA_Viewgroup_HScroller,
	MA_Viewgroup_CurrentView,
	MM_Viewgroup_ChangeView,
	MM_Viewgroup_ChangeView2,
	MM_Viewgroup_Aborted,
	MM_Viewgroup_GetBgPen,
	MA_Viewgroup_ViewModeIndex, /* [..G.] */
	MA_Viewgroup_ViewIndex,     /* [..G.] */
	MA_Viewgroup_ViewChanged,	/* [...N] */

	/* View */
	MA_View_IsRoot,
	MA_View_NeedsBackfill,
	MA_View_Path,
	MA_View_URI,
	MA_View_PreviousPath,
	MA_View_HasBackground,
	MA_View_BgPen,               /* background pen. valid between MUIM_View_Setup/Cleanup */
	MA_View_ModeIndex,           /* index of the mode, starts at 1, 0 being default */
	MA_View_MIME,                /* [G] foo/bar */
	MA_View_Scheme,              /* [G] file: */
	MA_View_NewWin,              /* [G] tells if the view wants a new window */
	MA_View_TotalFiles,
	MA_View_TotalDirs,
	MA_View_Links,
	MA_View_IconFiles,
	MA_View_DiskUsage,
	MA_View_SelectedDiskUsage,
	MA_View_Type,
	MA_View_ShowSearchString,
	MA_View_ContextMenuMode,
	MA_View_ViewMode,            /* viewgroup -> subview : IVM_ICON, IVM_SHOWALL, IVM_THUMBS, IVM_LISTER */
	MA_View_ShowDevices,         /* viewgroup -> subview [..G] currently showing devices */
	MA_View_SubObject,           /* viewgroup -> subview : Object * to listobject or iconobject */
	MA_View_IsViewObject,
	MA_View_HandleIcons,         /* FileView: tells if icons should be handled during a file operation (copy/delete/...) */
	MA_View_NumSelected,
	MA_View_IconSize,
	MA_View_SelectionMask,       /* See viewclass.h for definitions */
	MM_View_LoadURI,             /* viewgroup -> subview */
	MM_View_ReadArgs,
	MM_View_SetWindowPosition,
	MM_View_SetStatus,
	MM_View_Abort,               /* viewgroup -> subview */
	MM_View_ContextMenuMerge,
	MM_View_Mode,                /* viewgroup -> subview */
	MM_View_Setup,               /* viewgroup -> subview */
	MM_View_Cleanup,             /* viewgroup -> subview */
	MM_View_Refresh,             /* viewgroup -> subview */
	MM_View_Select,              /* viewgroup -> subview */
	MM_View_DoMethod,            /* viewgroup -> subview */
	MM_View_GetSelectionList,	 /* FileView: fills supplied list with selected objects (icons in iconview, list entries in listview etc) */
	MM_View_GetSelectedNamesList,/* FileView: */
	MM_View_Focus,               /* FileView: focuses view on item with given name */
	MM_View_GetEntry,            /* FileView: returns object which shoudl be icon-compatible. at least for few selected attributes */
	MM_View_InvalidateMimeType,  /* FileView: for each entry in view invalidate selected mimetype */
	MM_View_RedrawEntry,         /* FileView: redraws view 'entry'. */
	MM_View_QueryDisplayArea,    /* FileView: filles passed Rect32 struct with currently visible view area */
	MM_View_CheckFocus,          /* FileView: returns 1 if entry is visible */
	MM_View_PickSelected,        /* Create an AllocVecTaskPooled() object array from selected objects */
	MM_View_ParseWindowArgs,
	MM_View_IconSelect,
	MM_View_ExecuteAction,       /* See viewclass.h for details */

	/* Viewsizegroup */
	MA_Viewsizegroup_IsRoot,

	/* Crawl */
	MM_Crawl_Tick,
	MA_Crawl_Content,
	MA_Crawl_PreParse,

	/* Menuitem */
	MA_Menuitem_MenuType,
	MA_Menuitem_SubType,
	MA_Menuitem_Command,
	MA_Menuitem_FreeCommand,

	/* EditString */
	MM_EditString_EditStop,
	MM_EditString_Report,
	MA_EditString_ReportObj,

	/* Imageview */
	MM_Imageview_AddImage,
	MM_Imageview_AddImageScaled,
	MM_Imageview_AddImageNode,
	MM_Imageview_LoadImage,
	MM_Imageview_Scroll,
	MM_Imageview_ScrollX,
	MM_Imageview_ScrollY,
	MM_Imageview_SetPos,
	MM_Imageview_Scale,
	MM_Imageview_AddError,

	/* Addtext */
	MA_AddText_Contents,

	/* Soundwin */
	MM_Soundwin_Close,
	MM_Soundwin_Play,
	MM_Soundwin_Stop,
	MM_Soundwin_Scroll,

	/* Boopsiview */
	MM_Boopsiview_AddGadget,
	MM_Boopsiview_RemoveGadget,
	MM_Boopsiview_AddDatatype,
	MM_Boopsiview_SetPos,
	MM_Boopsiview_Trigger,

	/* Mimelisttree */
	MM_Mimelisttree_ChangeEntry,
	MM_Mimelisttree_Refresh,
	MA_Mimelisttree_ActiveType,
	MA_Mimelisttree_MarkDefined,
	MA_Mimelisttree_ShowInternal,

	/* Mimegroup */
	MM_Mimegroup_ChangeButtons,
	MM_Mimegroup_Edit,
	MM_Mimegroup_Add,
	MM_Mimegroup_Remove,
	MM_Mimegroup_Mime_Ack,

	/* Mimeadjustwin */
	MM_Mimeadjustwin_Close,
	MA_Mimeadjustwin_MimeType,
	MA_Mimeadjustwin_MimeNode,
	MA_Mimeadjustwin_MimeID,
	MA_Mimeadjustwin_Edit,
	MA_Mimeadjustwin_Generic,

	/* Mimeactionwin */
	MM_Mimeactionwin_Close,

	/* Mimeadjustgroup */
	MM_Mimeadjustgroup_CheckMime,
	MM_Mimeadjustgroup_Action_Add,
	MM_Mimeadjustgroup_Action_Remove,
	MM_Mimeadjustgroup_Action_Move,
	MM_Mimeadjustgroup_Action_Ack,
	MM_Mimeadjustgroup_Selection_Change,
	MM_Mimeadjustgroup_Accept,
	MM_Mimeadjustgroup_SaveIcon,
	MM_Mimeadjustgroup_IconChanged,

	/* Infoicongroup */
	MM_Infoicongroup_AddDragIcon,
	MM_Infoicongroup_SetIcon,
	MA_Infoicongroup_Child,
	MA_Infoicongroup_Changed,
	MA_Infoicongroup_Editable,

	/* Metadata (standalone) */
	MM_Metadata_Info,

	/* Graph */
	MM_Graph_Add,
	MA_Graph_BackgroundColor,
	MA_Graph_GridColor,
	MA_Graph_LineColor,
	MA_Graph_ScaleValue, /* graph scales to this value. */

	/* Sysinfowin */
	MM_Sysinfowin_UpdateSensorData,
	MM_Sysinfowin_UpdateSystemInfo,
	MM_Sysinfowin_Close,

	/* Toolbutton */
	MM_Toolbutton_Execute,      /* may or may not be implemented in button class. called by button itself */
	MM_Toolbutton_Update,       /* may or may not be implemented in button class. called by button itself */
	MA_Toolbutton_Name,         /* short description (used in settings mode) */
	MA_Toolbutton_Label,        /* caption (used in window mode) */
	MA_Toolbutton_Help,         /* optional help (for internals) */
	MA_Toolbutton_Single,       /* allow only one copy of it */
	MA_Toolbutton_Args,         /* arguments for button */
	MA_Toolbutton_Viewflags,	/* viewflags for which this button is visible */
	MA_Toolbutton_Location,     /* one of 3 specialflags; defines where the button is actually used */

	/* Toolbar */
	MA_Toolbar_Path,

	MM_Toolbar_HistoryNext,
	MM_Toolbar_HistoryPrev,
	MM_Toolbar_HistoryMove,		/* flexible version of HistoryNext/Prev with adjustable number of steps */
	MM_Toolbar_HistoryParent,
	MM_Toolbar_HistoryAddURI,
	MM_Toolbar_HistoryUpdateURI,
	MM_Toolbar_NewPath,
	MM_Toolbar_LocationAcknowledge,
	MM_Toolbar_ChangeView,
	MM_Toolbar_ViewChanged,
	
	MA_Toolbar_Viewobj,         /* inittime only. needed for history context */
	MA_Toolbar_History,			/* returns history context (It's readonly!) */

	/* Toolbargroup */
	MA_Toolbargroup_Definition,
	MA_Toolbargroup_Draggable,    /* draggable buttons */
	MA_Toolbargroup_Source,       /* is it a source container? */
	MM_Toolbargroup_UpdateVisible,/* update buttons when needed (internal) */

	/* Navigation */
	MM_Navigation_InsertHistory,
	MM_Navigation_SaveHistory,
	MM_Navigation_LoadHistory,
	MA_Navigation_StringObject,   /* [..G] returns string object */
	MA_Navigation_HistoryList,    /* [..G] returns list object (popup) */
	MA_Navigation_MaxHistoryItems,/* [I..] */
	MA_Navigation_HistoryPath,
	MA_Navigation_PathPopup,      /* adds asl popup button on the right */
	MA_Navigation_KeepActive,     /* string gadget is not deactivated after confirmation */
	MA_Navigation_StorageID,	  /* specifies ID shich is used to store hisotry in the storage */
	MA_Navigation_DiskStorage,    /* TRUE if history needs to be saved on disk, else it will live in mem only */
	MA_Navigation_AutoFocus,      /* displays entry on list while typing */

	/* Statusbar */
	MM_Statusbar_UpdateBackground,
	MM_Statusbar_UpdateTriggers,
	MM_Statusbar_SetMode,
	MM_Statusbar_Refresh,
	MA_Statusbar_TotalFiles,
	MA_Statusbar_TotalDirs,
	MA_Statusbar_Links,
	MA_Statusbar_IconFiles,
	MA_Statusbar_DiskUsage,
	MA_Statusbar_SelectedDiskUsage,
	MA_Statusbar_NumSelected,


	/* Clickpath */
	MM_Clickpath_ScrollBack,
	MM_Clickpath_ScrollForward,
	MM_Clickpath_Parent,
	MA_Clickpath_Position,
	MA_Clickpath_Path,
	MA_Clickpath_FullPath,
	MA_Clickpath_PathArrows,

	/* ClickpathButton */

	MA_ClickpathButton_Path,
	MA_ClickpathButton_Position,

	/* Textview */
	MM_Textview_Search,

	/* Searchbar */
	MM_Searchbar_Search,
	MM_Search,                          /* should focus on searched entry. return TRUE is found */
	MA_Searchbar_Target,                /* object that MM_Search is sent to */
	MA_Searchbar_Flags,
	MA_Searchbar_AutoHide,              /* hide when user pressed enter in search string */

	/* Destination selector */
	MA_DestSelector_Thread,
	MM_DestSelector_SetPath,
	MM_DestSelector_Accept,
	MA_DestSelector_Path,
	MA_DestSelector_RefWin,

	/* Action dispatcher */
	MM_ActionDispatcher_Execute,
	MM_ActionDispatcher_AddURI,
	MM_ActionDispatcher_Terminate,
	MA_ActionDispatcher_Event,
	MA_ActionDispatcher_Qualifier,      /* ACTION_QUALIFIER */
	MA_ActionDispatcher_IQualifier,     /* intuition qualifier */
	MA_ActionDispatcher_SrcURI,
	MA_ActionDispatcher_DstURI,
	MA_ActionDispatcher_RefWin,         /* reference window. If possible progress indicator will be displayed in it */
	MA_ActionDispatcher_SrcID,          /* Window/Viewgroup ID of source entity */
	MA_ActionDispatcher_DstID,          /* Window/Viewgroup ID of destination entity */
	MA_ActionDispatcher_Action,         /* give action to be executed for each URI. type lookup won't be executed. released with object */

	/* Action list */
	MM_Actionlist_SetDefaultAction,
	MA_Actionlist_Iconobj,              /* [I.G] */
	MA_Actionlist_Internal,             /* [..G] */

	/* Action edit */
	MM_Actionedit_OpenCommands,
	MM_Actionedit_CloseCommands,
	MM_Actionedit_ChangeCommand,
	MM_Actionedit_SelectCommand,
	MM_Actionedit_SetCommand,
	MM_Actionedit_GetActions,
	MM_Actionedit_ShowAction,
	MM_Actionedit_AddAction,
	MM_Actionedit_RemAction,
	MM_Actionedit_AddCommand,
	MM_Actionedit_RemCommand,
	MM_Actionedit_MoveCommand,
	MA_Actionedit_MimeNode,
	MA_Actionedit_Commandset,

	MA_ActioneditWin_MimeNode,
	MA_ActioneditWin_Commandset,
	MM_ActioneditWin_Open,
	MM_ActioneditWin_Close,

	/* Notification */
	MA_Notification_Type,
	MA_Notification_Text,

	/* Select win */
	MA_Selectwin_Pattern,
	MM_Selectwin_Select,
	MM_Selectwin_Close,
	
	/* Find win */
	MA_Find_Pattern,
	MA_Find_Text,
	MA_Find_Type,
	MM_Find_Start,
	MM_Find_Stop,
	MM_Find_AddLocation,
	MM_Find_RemLocation,
	MM_Find_AddResult,     /* internal */
	MM_Find_Update,        /* internal */
	MM_Find_TypeSelected,  /* internal */
	
	/* View selector */
	MA_ViewSelector_Left,
	MA_ViewSelector_Top,
	MA_ViewSelector_Width,
	MA_ViewSelector_Height,
	MA_ViewSelector_Accepted,
	MM_ViewSelector_Accept,

	/* KeyShortcut */
	MM_KeyShortcut_Update,
	MM_KeyShortcut_EditAction,
	MM_KeyShortcut_EditAction_Ack,
	MA_KeyShortcut_Shortcut,
	MA_KeyShortcut_EditAction,

	/* Action Editor */
	MM_Actioneditor_ChangeCommand,
	MM_Actioneditor_ShowAction,
	MM_Actioneditor_AddCommand,
	MM_Actioneditor_RemCommand,
	MM_Actioneditor_MoveCommand,
	MM_Actioneditor_CheckCommand,
	MM_Actioneditor_Accept,
	MM_Actioneditor_UpdateEvent,
	MA_Actioneditor_ActionNode,
	MA_Actioneditor_ActionID,
	MA_Actioneditor_MimeAction,

	MA_ActioneditorWin_ActionNode,
	MM_ActioneditorWin_Open,
	MM_ActioneditorWin_Close,

	/* PatternRename window */
	MA_PatternRenamewin_Path,
	MA_PatternRenamewin_PathList,
	MA_PatternRenamewin_FileType,
	MA_PatternRenamewin_NoIcon,
	MA_PatternRenamewin_IconType,
	MA_PatternRenamewin_OldName,
	MA_PatternRenamewin_NewName,
	MA_PatternRenamewin_Result,
	MA_PatternRenamewin_Thread,
	MM_PatternRenamewin_Accept,

	/* AddBookmark window */

	MM_AddBookmarkwin_Add,
	MM_AddBookmarkwin_Close,
	MA_AddBookmarkwin_Uri,
	MA_AddBookmarkwin_Name,
	MA_AddBookmarkwin_Permanent,

	MA_Mimetype_Type,
	MA_Mimetype_TypeResolved,

	MA_dummyend /* do not add any tag/method below ! */
};

#ifndef DEPEND
#include "fastclasses.h"
#endif /* !DEPEND */

/*
 * Public stuff (XXX: to be checked!)
 */
enum {
	MA_dummy_public = (int)(MTAGBASE_PUBLIC),
	MA_Panel_Properties,
	MA_Panel_About,
	MA_Panel_Help,
	MM_Notify_Change,
};

struct MP_Notify_Change {
	ULONG MethodID;
	APTR ctx;
	/* XXX: we should extend this */
};

struct BitMap;

struct MP_Application_AddWindow {
	ULONG MethodID;
	APTR obj;
	ULONG id;
};

struct MP_Application_RemoveWindow {
	ULONG MethodID;
	APTR obj;
	ULONG force;
};

struct MP_Application_DeleteActionDispatcher {
	ULONG MethodID;
	APTR dispatcher;
};

struct MP_Application_FindWindowByID {
	ULONG MethodID;
	ULONG id;
};

struct MP_Application_FindWindowByUserData {
	ULONG MethodID;
	ULONG type;
	ULONG userdata;
};

struct MP_Application_FindWindowByName {
	ULONG MethodID;
	ULONG type;
	STRPTR name;
};

struct MP_Application_FindWindowByType {
	ULONG MethodID;
	ULONG type;
};

struct MP_Application_EnableDOSNotify {
	ULONG MethodID;
	STRPTR path;
	ULONG enable;
};

struct MP_Application_CreateDestSelectorwin {
	ULONG MethodID;
	APTR thread;
	APTR refwin;
};

struct MP_Application_CreatePatternRenamewin {
	ULONG MethodID;
	APTR thread;
	STRPTR from;
	STRPTR * pathlist;
};


struct MP_Application_DisposeWindow {
	ULONG MethodID;
	APTR win;
};

struct MP_Application_OpenDevicesWindow {
	ULONG MethodID;
	ULONG Moused;
};

struct MP_Application_SoundControl {
	ULONG MethodID;
	ULONG command;
};

struct MP_Application_Open_AboutMorphOSWindow {
	ULONG MethodID;
	ULONG open;
};

struct MP_Application_GetMimeType {
	ULONG MethodID;
	STRPTR path;
};

struct MP_Application_GetDeficonPath {
	ULONG MethodID;
	STRPTR path;
	STRPTR mimetype;
};

struct MP_Icon_InsertToolType {  /* changing this structure requires changes also in iconlib/fakemethod.c */
	ULONG MethodID;
	ULONG size;
	STRPTR tt;
};

struct MP_Icon_AddBitMap { /* changing this structure requires changes also in iconlib/fakemethod.c */
	ULONG MethodID;
#ifdef BUILD_ICONLIB
	struct BitMap *bm;
	ULONG width;
	ULONG height;
#else
	APTR bm;
#endif
	ULONG type;
	ULONG state;
};

struct MP_Thread_Finished {
	ULONG MethodID;
	ULONG action;
	LONG status;
	struct TagItem *taglist; /* can be NULL */
};

struct MP_Iconview_LoadFiles {
	ULONG MethodID;
	STRPTR path;
};

struct MP_Iconview_FocusIcon {
	ULONG MethodID;
	APTR obj;
};


struct MP_About_GotoURL {
	ULONG MethodID;
	STRPTR url;
};

struct MP_Prefswin_Main_SelectChange {
	ULONG MethodID;
	LONG listentry;
};

struct MP_Prefswin_Main_Close {
	ULONG MethodID;
	ULONG mode;
};

struct MP_Application_SavePrefs {
	ULONG MethodID;
	ULONG askfilename;
};

struct MP_Icon_ErrorString { /* changing this structure requires changes also in iconlib/fakemethod.c */
	ULONG MethodID;
	STRPTR error;
};

struct MP_Application_LoadPrefs {
	ULONG MethodID;
	ULONG id;
	ULONG quiet;
};

struct MP_Application_CreateIcon {
	ULONG MethodID;
	ULONG isroot;
	ULONG view;
};

struct MP_Application_DisposeObject {
	ULONG MethodID;
	APTR o;
};

struct MP_Application_DisposeObjectArray {
	ULONG MethodID;
	APTR *array;
	ULONG threaded;
	ULONG freearray;
};

struct MP_Application_AddObject {
	ULONG MethodID;
	APTR obj;
	APTR o;
};

struct MP_Application_AddAppIcon {
	ULONG MethodID;
	APTR o;
};

struct MP_Icon_GetToolTypes {
	ULONG MethodID;
	STRPTR *tt;
	LONG *var[0];
};

struct MP_Application_DisplayUpdate {
	ULONG MethodID;
	ULONG flags;
};

struct MP_Iconview_Layout_Find {
	ULONG MethodID;
	ULONG flags; /* XXX: use it */
	APTR obj;
};

struct MP_Iconview_Layout_CheckFit {
	ULONG MethodID;
	ULONG x;
	ULONG y;
	ULONG xs;
	ULONG ys;
};

struct MP_Application_Delete {
	ULONG MethodID;
	STRPTR name;
};

struct MP_Application_Delete_Ok {
	ULONG MethodID;
	LONG butnum;
	ULONG userdata;
};

struct MP_Application_Trash_Ok {
	ULONG MethodID;
	LONG butnum;
	ULONG userdata;
};

struct MP_CapacityText_Build {
	ULONG MethodID;
	QUAD *totalsize;
};

struct MP_Infowin_TTSetString {
	ULONG MethodID;
	LONG entry;
};

struct MP_Application_ChangeScreenTitle {
	ULONG MethodID;
	STRPTR title;
	ULONG flags;
};

struct MP_Execute_Close {
	ULONG MethodID;
	ULONG val;
};

struct MP_SmartReq_Enqueue {
	ULONG MethodID;
	struct reqnode *rn;
};

struct MP_SmartReq_Pressed {
	ULONG MethodID;
	ULONG methodid;
	ULONG flags;
	LONG butnum;
	LONG userdata;
	ULONG counter;
};

struct MP_SmartReq_Ask {
	ULONG MethodID;
	LONG butnum;
	LONG userdata;
};

struct MP_Application_DOSNotify {
	ULONG MethodID;
	ULONG id;
	STRPTR name;
};

struct MP_Infowin_UpdateSize {
	ULONG MethodID;
	ULONG numdirs;
	ULONG numfiles;
	ULONG numhardlinks;
	ULONG numsoftlinks;
	QUAD *size;
	ULONG final;
};

struct MP_Infowin_UpdateVersion {
	ULONG MethodID;
	STRPTR text;
};

struct MP_Infowin_UpdateMD5sum {
	ULONG MethodID;
	STRPTR text;
};

struct MP_Application_DeleteAppIcon {
	ULONG MethodID;
	APTR address;
};

struct MP_Iconview_DeleteAppIcon {
	ULONG MethodID;
	APTR address;
};

struct MP_Iconview_RemoveByName {
	ULONG MethodID;
	STRPTR name;
};

struct MP_Progresswin_Update {
	ULONG MethodID;
	STRPTR name; /* when NULL, only update the progress */
	UQUAD *size; /* when name is NULL, size is maxsize otherwise it's currentsize addition */
	ULONG count;
	UQUAD *totaldone;
};

struct MP_Progresswin_InitProgress {
	ULONG MethodID;
	UQUAD *size;
	ULONG files;
};

struct MP_Application_CreateProgresswin {
	ULONG MethodID;
	ULONG mode;
	APTR thread;
	APTR refwin;
};

struct MP_Format_ChangeOptions {
	ULONG MethodID;
	struct device_info *di;
};

struct MP_Format_Format {
	ULONG MethodID;
	ULONG mode;
};

struct MP_Formatwin_Format {
	ULONG MethodID;
	ULONG mode;
};

struct MP_Formatwin_SetText {
	ULONG MethodID;
	STRPTR txt;
};

struct MP_Formatwin_SetGauge {
	ULONG MethodID;
	ULONG val;
};

struct MP_Listview_AddFile {
	ULONG MethodID;
	struct ExAllData *ead;
	ULONG replace;
	ULONG mode;
	STRPTR target;
};

struct MP_Listview_AddFiles {
	ULONG MethodID;
	ULONG mode;
};

struct MP_Listview_RemoveByName {
	ULONG MethodID;
	STRPTR name;
	ULONG flags;
};

struct MP_Listview_RemoveFiles {
	ULONG MethodID;
	ULONG mode;
};

struct MP_Listview_Sort {
	ULONG MethodID;
	ULONG mode;
};

struct MP_Listview_UpdateView {
	ULONG MethodID;
	ULONG mode;
};

struct MP_Listview_RunIconThread {
	ULONG MethodID;
	ULONG enable;
	ULONG immediate;
};

struct MP_Listview_JumpTo {
	ULONG MethodID;
	STRPTR partialname;
};

struct MP_Listview_FindIcon {
	ULONG MethodID;
	APTR    entry;
	ULONG   type;
};

struct MP_Listview_IsColumnVisible {
	ULONG MethodID;
	ULONG col;
};

struct MP_Listview_GetColumnOrder {
	ULONG MethodID;
	ULONG columnid;
};

struct MP_Listview_InvalidateIcon {
	ULONG MethodID;
	APTR  entry;
};

struct MP_Listview_ShowPreview {
	ULONG MethodID;
	ULONG enable;
};

struct MP_Listviewlist_DoubleClick {
	ULONG MethodID;
	ULONG entry;
};

struct MP_Listviewlist_EntryClick {
	ULONG MethodID;
	LONG pos;
};

struct MP_Formatwin_FormatReally {
	ULONG MethodID;
	LONG butnum;
	ULONG userdata;
};

struct MP_Icon_AddAncillary { /* changing this structure requires changes also in iconlib/fakemethod.c */
	ULONG MethodID;
	ULONG type;
	ULONG size;
	APTR data;
};

struct MP_Icon_GetAncillary {
	ULONG MethodID;
	ULONG type;
	UBYTE **ptr;
	ULONG *size;
};

struct MP_Iconview_Unselect {
	ULONG MethodID;
	APTR o;
};

struct MP_Icon_CreateBitMap {
	ULONG MethodID;
	ULONG type;
	ULONG state;
};

struct MP_Rexx_Snapshot {
	ULONG MethodID;
	ULONG icons;
	ULONG window;
	ULONG selection;
};

struct MP_Rexx_Unsnapshot {
	ULONG MethodID;
	ULONG icons;
	ULONG window;
	ULONG selection;
};

struct MP_Infowin_ChangeMode {
	ULONG MethodID;
	ULONG mode;
};

struct MP_Iconview_DoSort {
	ULONG MethodID;
	ULONG mode;
	ULONG sort;
};

struct RastPort;

struct MP_Window_FillBackground {
	ULONG MethodID;
	struct RastPort *rp;
	ULONG x;
	ULONG y;
	ULONG xs;
	ULONG ys;
};

struct MP_Window_LoadURI {
	ULONG MethodID;
	STRPTR path;
	STRPTR mode;
	STRPTR options;
};

struct MP_Window_MenuAction {
	ULONG MethodID;
	LONG  Action;
};

struct MP_Format_Busy {
	ULONG MethodID;
	ULONG sleep;
};

struct MP_Application_Shutdown {
	ULONG MethodID;
	LONG butnum;
	ULONG userdata;
};

struct MP_Makedirwin_Makedir {
	ULONG MethodID;
	ULONG mode;
};

struct MP_Application_CreateSmartReq {
	ULONG MethodID;
	STRPTR obj;
	STRPTR winobj;
};

struct MP_Iconview_BuildSnapshotlist {
	ULONG MethodID;
	ULONG mode;
};

struct MP_Iconview_SnapshotIcon {
	ULONG MethodID;
	APTR obj;
};

struct MP_Iconview_UnsnapshotIcon {
	ULONG MethodID;
	APTR obj;
};

struct PrivateCxObj;

struct MP_Cxlist_TryAdd {
	ULONG MethodID;
	struct PrivateCxObj *mco;
};

struct MP_Cxlist_CxNotify {
	ULONG MethodID;
	ULONG cmd;
};

struct MP_Cxwin_SetStatus {
	ULONG MethodID;
	ULONG gui;
	ULONG control;
	ULONG remove;
	STRPTR title;
	STRPTR descr;
};

struct MP_Cxlist_SetStatus {
	ULONG MethodID;
	LONG val;
};

struct MP_Application_DoRexx {
	ULONG MethodID;
	ULONG internal;
	APTR obj;
	STRPTR str;
	LONG *retval;
	STRPTR *retstr;
	ULONG id;
	APTR *objlist;
	ULONG sync;
};

struct MP_Prefswin_Main_SetPage {
	ULONG MethodID;
	STRPTR name;
};

struct MP_Panelconfigwin_Size {
	ULONG MethodID;
	ULONG size;
};

struct MP_Panelconfigwin_Depth {
	ULONG MethodID;
	ULONG depth;
};

struct MP_Panelconfigwin_Horiz {
	ULONG MethodID;
	ULONG horiz;
};

struct MP_Panelgroup_ChangeDrag {
	ULONG MethodID;
	ULONG dragmode;
};

struct MP_Panelconfigwin_FrameSpec {
	ULONG MethodID;
	UBYTE *spec;
};

struct MP_Panelconfigwin_ImageSpec {
	ULONG MethodID;
	UBYTE *spec;
};

struct MP_Panelitemwin_ListChange {
	ULONG MethodID;
	LONG val;
};

struct MP_Panelgroup_Lock {
	ULONG MethodID;
	ULONG lock;
};

struct MP_Panelgroup_RemoveItem {
	ULONG MethodID;
	APTR o;
};

struct MP_BGRender_Change {
	ULONG MethodID;
	ULONG mode;
};

struct MP_Panelwin_Delete_Ok {
	ULONG MethodID;
	LONG butnum;
	ULONG userdata;
};

struct MP_Panelwin_FindObjectByType {
	ULONG MethodID;
	ULONG type;
};

struct MP_Application_CreatePanelwin {
	ULONG MethodID;
	APTR pctx;
};

struct MP_Panelgroup_WhichObject {
	ULONG MethodID;
	ULONG x;
	ULONG y;
};

struct MP_Application_CreatePanelitem {
	ULONG MethodID;
	ULONG type;
	APTR obj;
	APTR prefspool;
	ULONG index;
};

struct MP_Panelconfigwin_PosMode {
	ULONG MethodID;
	ULONG mode;
};

struct MP_Panelgroup_ToggleZip {
	ULONG MethodID;
	ULONG type; /* left/up or right/bottom paneldrag */
};

struct MP_Panelconfigwin_Zipping {
	ULONG MethodID;
	ULONG selected;
};

struct MP_Panelconfigwin_AutoZip {
	ULONG MethodID;
	ULONG selected;
};

struct MP_Panelbutton_AddIcon {
	ULONG MethodID;
	APTR obj;
};

struct MP_Iconview_SelectLasso {
	ULONG MethodID;
	ULONG select;
};

struct MP_Panelwin_AttachToBorder {
	ULONG MethodID;
	ULONG immediate_save; /* if the user moves a window to a border, it doesn't move afterwards but we want it to be saved still */
};

struct MP_Panelclasslist_ScanDirectory {
	ULONG MethodID;
	STRPTR path;
};

struct MP_Prefswin_Icondisplay_AdjustSize {
	ULONG MethodID;
	ULONG sizemode;
};

struct MP_Icon_Scale {
	ULONG MethodID;
	ULONG mode;
};

struct MP_Application_CreateInfowin {
	ULONG MethodID;
	APTR iconobj;
	ULONG rxid;
	APTR infodata;
};

struct MP_Iconview_SaveShortcuts2 {
	ULONG MethodID;
	APTR pctx;
};

struct MP_Iconview_FindShortcut {
	ULONG MethodID;
	STRPTR path;
};

struct MP_Panelgroup_WhichPosition {
	ULONG MethodID;
	LONG x;
	LONG y;
};

struct MP_Panelgroup_MoveMode_Start {
	ULONG MethodID;
	ULONG mode;
	LONG x;
	LONG y;
};

struct MP_Panelgroup_MoveMode_Stop {
	ULONG MethodID;
	ULONG mode;
};

struct MP_Window_SetTitle {
	ULONG MethodID;
	STRPTR name;
};

struct MP_Viewgroup_ChangeView {
	ULONG MethodID;
	STRPTR name;
};

struct MP_View_LoadURI {
	ULONG MethodID;
	STRPTR uri;
};

struct MP_View_InvalidateMimeType {
	ULONG MethodID;
	APTR mimetype;
};

struct MP_Application_CreateWindow {
	ULONG MethodID;
	ULONG type;
	APTR mimectx;
	ULONG browser;
};

struct MP_View_ReadArgs {
	ULONG MethodID;
	STRPTR templ;
	LONG *array;
};

struct MP_View_SetWindowPosition {
	ULONG MethodID;
	LONG left;
	LONG top;
};

struct MP_View_GetSelectionList {
	ULONG MethodID;
	APTR list;
};

struct MP_View_GetSelectedNamesList {
	ULONG MethodID;
	struct MinList **listptr;
};

struct MP_View_GetEntry {
	ULONG MethodID;
	ULONG pos;
};

struct MP_View_Focus {
	ULONG MethodID;
	ULONG partial;
	STRPTR name;
	LONG searchUp;
};

struct MP_View_RedrawEntry {
	ULONG MethodID;
	APTR entry;
};

struct MP_View_CheckFocus {
	ULONG MethodID;
	APTR entry;
};

struct MP_View_QueryDisplayArea {
	ULONG MethodID;
	struct Rect32 *rect;
};

struct MP_View_IconSelect {
	ULONG MethodID;
	LONG  selection; /* One of MV_View_IconSelect_#? */
	LONG  type;      /* One of MV_Icon_Type_#? */
	LONG  shortcut;  /* TRUE if shortcut */
};

struct MP_Iconview_AddIcon {
	ULONG MethodID;
	APTR o;
	ULONG unique;
};

struct MP_Iconview_DeleteIcon {
	ULONG MethodID;
	APTR o;
};

struct MP_Application_WindowDoMethodByAttr {
	ULONG MethodID;
	ULONG attr;
	ULONG val;
	ULONG args;
};

struct MP_Application_RootDoMethodByAttr {
	ULONG MethodID;
	ULONG attr;
	ULONG val;
	ULONG args;
};

struct MP_Application_AddBackground {
	ULONG MethodID;
	ULONG type;
	APTR dtp;
};

struct MP_Window_UpdateBackground {
	ULONG MethodID;
	APTR dtp;
	ULONG bgmode;
};

struct MP_Application_LoadBackground {
	ULONG MethodID;
	ULONG flags;
};

struct MP_Application_CenterBackground {
	ULONG MethodID;
	ULONG pen;
	APTR  screen;
};

struct MP_View_SetStatus {
	ULONG MethodID;
	ULONG flags;
	STRPTR fmt;
	ULONG val;
	/* ... */
};

struct MP_View_ContextMenuMerge {
	ULONG MethodID;
	STRPTR from;
	STRPTR to;
};

struct MP_View_Refresh {
	ULONG MethodID;
	ULONG flags;
	APTR dtp;
};

struct MP_View_Select {
	ULONG MethodID;
	ULONG mode;
	STRPTR pattern;
};

struct MP_View_ParseWindowArgs {
	ULONG MethodID;
	CONST_STRPTR *type;
	ULONG *mode;
	ULONG *modeset;
	ULONG *width;
	ULONG *height;
	ULONG *sortset; // bitRocky
	ULONG *sortby; // bitRocky
	LONG *sortorder; // bitRocky: 1 (asc) or -1 (desc)
};

struct MP_Window_Close {
	ULONG MethodID;
	ULONG fromroot;
};

struct MP_EditString_EditStop {
	ULONG MethodID;
	STRPTR s;
};

struct MP_EditString_Report {
	ULONG MethodID;
	ULONG abort;
};

struct MP_Window_DoView {
	ULONG MethodID;
	ULONG view;
	ULONG args;
};

// bitRocky
struct MP_Window_IsActive {
	ULONG MethodID;
	ULONG active;
};

// bitRocky
struct MP_Window_ZoomIcons {
	ULONG MethodID;
	ULONG level;
};

struct MP_View_DoMethod {
	ULONG MethodID;
	APTR args;
};

struct MP_Imageview_AddImage {
	ULONG MethodID;
	APTR dtp;
	LONG mode;
};

struct MP_Imageview_AddImageScaled {
	ULONG MethodID;
	APTR bm;
};

struct MP_Icon_Click {
	ULONG MethodID;
	ULONG seconds;
	ULONG micros;
};

struct MP_Icon_Generate {
	ULONG MethodID;
	ULONG force;
};

struct MP_Icon_Select {
	ULONG MethodID;
	LONG x;
	LONG y;
};

struct MP_Iconview_AddShortcut {
	ULONG MethodID;
	STRPTR path;
};

struct MP_Iconview_ShowPreview {
	ULONG MethodID;
	ULONG enable;
};

struct MP_Imageview_AddImageNode {
	ULONG MethodID;
	STRPTR path;
};

struct MP_Imageview_LoadImage {
	ULONG MethodID;
	STRPTR path;
};

struct MP_Imageview_Scroll {
	ULONG MethodID;
	ULONG pos_x;
	ULONG pos_y;
};

struct MP_Imageview_ScrollX {
	ULONG MethodID;
	ULONG pos_x;
};

struct MP_Imageview_ScrollY {
	ULONG MethodID;
	ULONG pos_y;
};

struct MP_Imageview_SetPos {
	ULONG MethodID;
	ULONG dir;
	ULONG quals;
};

struct dtobject;

struct MP_Boopsiview_AddDatatype {
	ULONG MethodID;
	struct dtobject *dtobj;
};

struct MP_Boopsiview_SetPos {
	ULONG MethodID;
	ULONG dir;
	ULONG quals;
};

struct MP_Boopsiview_Trigger {
	ULONG MethodID;
	ULONG type;
};

struct MP_Imageview_AddError {
	ULONG MethodID;
	LONG err;
};

struct MP_Panelgroup_Refresh {
	ULONG MethodID;
	ULONG hard;
};

struct MP_PanelListTree_CreateItem
{
	ULONG MethodID;
	APTR parent;
	APTR panelobject;
};
struct MP_PanelListTree_RefreshNode
{
	ULONG MethodID;
	APTR treenode;
};

struct MP_PanelListTree_FindUData
{
	ULONG MethodID;
	APTR UData;
};

struct MP_PanelListTree_CreateList
{
	ULONG MethodID;
	APTR treenode;
};


struct MP_Mimegroup_ChangeButtons {
	ULONG MethodID;
	ULONG val;
};

struct MP_Mimelisttree_ChangeEntry {
	ULONG MethodID;
	LONG entry;
};

struct MP_Mimegroup_Edit {
	ULONG MethodID;
	STRPTR mimetype;
	APTR   mimenode;
	ULONG edit;
	ULONG generic;
};

struct MP_Mimegroup_Mime_Ack {
	ULONG MethodID;
	APTR  mimeobj;
	ULONG edit;
};

struct MP_Mimegroup_Add {
	ULONG MethodID;
	ULONG edit;
};

struct MP_Mimeadjustgroup_Action_Add {
	ULONG MethodID;
	ULONG edit;
};

struct MP_Mimeadjustgroup_Action_Move {
	ULONG MethodID;
	LONG offset;
};

struct MP_Mimeadjustgroup_Action_Ack {
	ULONG MethodID;
	APTR  actionobj;
	ULONG edit;
};

struct MP_Mimeadjustgroup_Accept {
	ULONG MethodID;
	ULONG accept;
};

struct atextfont;

struct MP_Application_SetFont {
	ULONG MethodID;
	ULONG type;
	struct atextfont *at;
};

struct MP_Infoicongroup_AddDragIcon {
	ULONG MethodID;
	APTR o;
};

struct MP_Infoicongroup_SetIcon {
	ULONG MethodID;
	STRPTR path;
	LONG   deficon;
};

struct MP_Metadata_Info {
	ULONG MethodID;
	STRPTR txt;
};

struct MP_Infowin_Changed {
	ULONG MethodID;
	ULONG mode;
};

struct MP_Navigation_InsertHistory {
	ULONG MethodID;
	STRPTR txt;
};

struct MP_Clickpath_Clicked {
	ULONG MethodID;
	ULONG dist;
};

struct MP_Toolbar_HistoryMove {
	ULONG MethodID;
	LONG steps;
};

struct MP_Toolbar_HistoryAddURI {
	ULONG MethodID;
	STRPTR path;
	STRPTR mode;
};

struct MP_Toolbar_HistoryUpdateURI {
	ULONG MethodID;
	STRPTR path;
	STRPTR mode;
};

struct MP_Toolbargroup_DeleteButton {
	ULONG MethodID;
	APTR o;
};

struct MP_DestSelector_Accept {
	ULONG MethodID;
	ULONG accept;
};

struct MP_Textview_Search {
	ULONG MethodID;
	STRPTR str;
	ULONG direction;
	ULONG sensitive;
};

struct MP_Searchbar_Search {
	ULONG MethodID;
	ULONG direction;
};

struct MP_Search {
	ULONG MethodID;
	STRPTR string;
	ULONG direction;
	ULONG casesensitive;
};

struct MP_Graph_Add {
	ULONG MethodID;
	ULONG value;
};

struct MP_Application_ReloadIcons {
	ULONG MethodID;
	ULONG onlyroot;
};

struct MP_Application_RemoveShortcut {
	ULONG MethodID;
	APTR  icon;
};

#ifdef DEBUG
struct MP_Application_SetDebug {
	ULONG MethodID;
	ULONG type;
	ULONG value;
};

struct MP_Application_DoDebug {
	ULONG MethodID;
	ULONG type;
};
#endif /* DEBUG */

struct MP_ActionDispatcher_AddURI {
	ULONG MethodID;
	STRPTR uri;
	ULONG quotes;
};

struct MP_Actionlist_SetDefaultAction {
	ULONG MethodID;
	LONG value;
};

struct MP_Actionedit_ChangeCommand {
	ULONG MethodID;
	APTR selobj;
	ULONG modifylist;
};

struct MP_Actionedit_SetCommand {
	ULONG MethodID;
	STRPTR cmd;
};

struct MP_Statusbar_SetMode {
	ULONG MethodID;
	LONG mode;
};

struct MP_Actionedit_MoveCommand {
	ULONG MethodID;
	LONG offset;
};

struct MP_Find_Start {
	ULONG MethodID;
};

struct MP_Find_Stop {
	ULONG MethodID;
	ULONG restart;
};

struct MP_Find_AddLocation {
	ULONG MethodID;
	STRPTR location;
};

struct MP_Find_RemLocation {
	ULONG MethodID;
};

struct MP_Find_AddResult {
	ULONG MethodID;
	STRPTR result;
	STRPTR comment;
};

struct MP_ViewSelector_Accept {
	ULONG MethodID;
	ULONG accept;
};


/* Action Editor */
struct MP_Actioneditor_ChangeCommand {
	ULONG MethodID;
	STRPTR cmd;
	LONG  type;
};

struct MP_Actioneditor_MoveCommand {
	ULONG MethodID;
	LONG offset;
};

struct MP_Actioneditor_Accept {
	ULONG MethodID;
	ULONG accept;
};

/* Keyshortcut */
struct MP_KeyShortcut_EditAction_Ack {
	ULONG MethodID;
	APTR actionobj;
};

struct MP_Prefswin_Keyboard_RemoveShortcut {
	ULONG MethodID;
	APTR object;
};

struct MP_PatternRenamewin_Accept {
	ULONG MethodID;
	ULONG accept;
};

struct MP_Prefswin_Panels_Selected {
	ULONG MethodID;
	struct MUIS_Listtree_TreeNode *node;
};

struct MP_Prefswin_Panels_Horiz {
	ULONG MethodID;
	ULONG horiz;
};

struct MP_Prefswin_Panels_Position {
	ULONG MethodID;
	ULONG mode;
};

struct MP_Prefswin_Panels_Size {
	ULONG MethodID;
	ULONG size;
};

struct MP_Prefswin_Panels_Depth {
	ULONG MethodID;
	ULONG depth;
};

struct MP_Prefswin_Panels_DragMode {
	ULONG MethodID;
	ULONG mode;
};

struct MP_Prefswin_Panels_HideDragBar {
	ULONG MethodID;
	BOOL hide;
};

struct MP_Prefswin_Panels_HasChanged {
	ULONG MethodID;
	APTR panel;
};

struct MP_Prefswin_Panels_HasClosed {
	ULONG MethodID;
	APTR panel;
};
struct MP_Prefswin_Panels_BackMode {
	ULONG MethodID;
	ULONG mode;
};

struct MP_Prefswin_Panels_ItemListChange {
	ULONG MethodID;
	LONG val;
};
struct MP_Prefswin_Panels_UpdatePanels {
	ULONG MethodID;
	ULONG mode;
};


/* Special values */

#define MV_Icon_DeviceType_NotDevice     0
#define MV_Icon_DeviceType_Auxillary     1
#define MV_Icon_DeviceType_Fixed         2

#define MV_Icon_ReferenceCount_Increase  -1
#define MV_Icon_ReferenceCount_Decrease  -2

/* Icon (same as WB_#? values) */
#define MV_Icon_Type_None    0 /* that one is a special case used internally */
#define MV_Icon_Type_Disk    1
#define MV_Icon_Type_Drawer  2
#define MV_Icon_Type_Tool    3
#define MV_Icon_Type_Project 4
#define MV_Icon_Type_Garbage 5
#define MV_Icon_Type_Device  6
#define MV_Icon_Type_Kick    7
#define MV_Icon_Type_AppIcon 8
#define MV_Icon_Type_MyComputer 9  /* Duh... bad name choice! */
#define MV_Icon_Type_View    10  /* Duh... bad name choice! */
#define MV_Icon_Type_Bookmarks 11

#define MV_Icon_Type_Last    11 /* Don't put anything below that one. */

/* Icon filetypes (same as ST_#? values) */
#define MV_Icon_FileType_File               -3
#define MV_Icon_FileType_Directory           2 
#define MV_Icon_FileType_Device              1 
#define MV_Icon_FileType_None                0 /* no filetype associated (icon only) */
#define MV_Icon_FileType_Softlink            3 
#define MV_Icon_FileType_Hardlink_Directory  4 
#define MV_Icon_FileType_Hardlink_File      -4

#define MV_Icon_ViewMode_None       IVM_NONE
#define MV_Icon_ViewMode_Lister     IVM_LISTER
#define MV_Icon_ViewMode_Icon       IVM_ICON
#define MV_Icon_ViewMode_IconAll    IVM_SHOWALL
#define MV_Icon_ViewMode_Thumbs     IVM_THUMBS

#define MV_Icon_ImageType_Old       1  /* AmigaOS 1.0 (not used as not possible to detect properly) */
#define MV_Icon_ImageType_Standard  2  /* AmigaOS 2.0-3.1 */
#define MV_Icon_ImageType_Newicon   3  /* Newicons */
#define MV_Icon_ImageType_Glowicon  4  /* "OS" 3.5 crap */
#define MV_Icon_ImageType_PNGicon   5  /* The Real Thing (tm) */
#define MV_Icon_ImageType_DTicon    6  /* Datatypes icons */
#define MV_Icon_ImageType_Thumbicon 7  /* Thumbnail */
#define MV_Icon_ImageType_SVGicon   8  /* SVG */

/* AddBitMap types */
#define MV_Icon_BitMap_Standard     0
#define MV_Icon_BitMap_Newicon      1
#define MV_Icon_BitMap_Glowicon     2
#define MV_Icon_BitMap_PNGicon      3
#define MV_Icon_BitMap_DTicon       4
#define MV_Icon_BitMap_Thumbicon    5
#define MV_Icon_BitMap_SVGicon      6

/* iconlib only */
#define MV_Icon_AddImage_Normal     0
#define MV_Icon_AddImage_Selected   2

/* AddBitMap states */
#define MV_Icon_BitMap_Normal       0
#define MV_Icon_BitMap_Selected     2

/* Ancillary datatype */
#define MV_Icon_Ancillary_Gadget            0
#define MV_Icon_Ancillary_ImageNormal       1
#define MV_Icon_Ancillary_ImageSelected     2
#define MV_Icon_Ancillary_NewiconNormalTT   3
#define MV_Icon_Ancillary_NewiconSelectedTT 4
#define MV_Icon_Ancillary_Glowicon_Chunk    5
#define MV_Icon_Ancillary_PNGicon_Chunk     6
#define MV_Icon_Ancillary_PNGicon_Ctx       7
#define MV_Icon_Ancillary_SVGDoc            8
#define MV_Icon_Ancillary_VGObj             9

/* Drawerdata stuff */
#define MV_Icon_HasDrawerData_None    0
#define MV_Icon_HasDrawerData_Old     1
#define MV_Icon_HasDrawerData_LessOld 2

/* Scaling modes */
#define MV_Icon_Scale_Magnify   0
#define MV_Icon_Scale_Reduce    1
#define MV_Icon_Scale_Infowin   2
#define MV_Icon_Scale_Normalize 3

/* Sorting modes */
#define MV_Icon_SortMode_Name 0
#define MV_Icon_SortMode_Date 1
#define MV_Icon_SortMode_Size 2
#define MV_Icon_SortMode_Type 3 /* XXX: hm.. what the hell can a type be useful for ? */

/* Prefswin saving method */
#define MV_Prefswin_Main_Close_Cancel 0
#define MV_Prefswin_Main_Close_Use    1
#define MV_Prefswin_Main_Close_Save   2
#define MV_Prefswin_Main_Close_Test   3

/* Application prefs */
#define MV_Application_LoadPrefs_All 0xffffffff

/* Application window reinitializations */
#define MF_Application_DisplayUpdate_Windows           (1UL << 0)
#define MF_Application_DisplayUpdate_Root              (1UL << 1)
/* type */
#define MF_Application_DisplayUpdate_Fonts             (1UL << 2)
#define MF_Application_DisplayUpdate_Size              (1UL << 3)
#define MF_Application_DisplayUpdate_Background        (1UL << 4)
#define MF_Application_DisplayUpdate_UI                (1UL << 5)

/* Application sound control */
#define MV_Application_SoundControl_Play  1
#define MV_Application_SoundControl_Pause 2
#define MV_Application_SoundControl_Stop  3
#define MV_Application_SoundControl_OpenControl  4

/* Thread status */
#define MV_Thread_Finished_Ok     TRUE
#define MV_Thread_Finished_Error  FALSE
#define MV_Thread_Finished_Abort  -1L /* ABORTED */

/* Application ChangeScreenTitle */
#define MF_Application_ChangeScreenTitle_Delayed   (1UL << 0) /* appears only a few seconds */
#define MF_Application_ChangeScreenTitle_Temporary (1UL << 1) /* goes away on the next restore */

/* Execute Close */
#define MV_Execute_Close_Cancel 0
#define MV_Execute_Close_Ok     1

/* Iconview snapshot building list mode */
#define MV_Iconview_BuildSnapshotlist_Snapshot 0UL
#define MV_Iconview_BuildSnapshotlist_Unsnapshot 1UL
#define MV_Iconview_BuildSnapshotlist_Snapshot_Selection 2UL
#define MV_Iconview_BuildSnapshotlist_Unsnapshot_Selection 3UL

/* Progresswin Look */
#define MV_Progresswin_Look_Move       0
#define MV_Progresswin_Look_MoveMany   1
#define MV_Progresswin_Look_Copy       2
#define MV_Progresswin_Look_CopyMany   3
#define MV_Progresswin_Look_Delete     4
#define MV_Progresswin_Look_DeleteMany 5

/* DragDrop */
#define MV_DragDrop_Type_Iconview 0
#define MV_DragDrop_Type_Root     1
#define MV_DragDrop_Type_Icon     2

#define MV_DragDrop_Drop_None     0
#define MV_DragDrop_Drop_Shortcut 1
#define MV_DragDrop_Drop_Appwin   4

/* Window Path */
#define MV_Window_Path_None 0
#define MV_Window_Path_Any  1

/* Window types */
#define MV_Window_Type_Unknown          0
#define MV_Window_Type_Rootview         1
#define MV_Window_Type_View             2
#define MV_Window_Type_About            3
#define MV_Window_Type_Execute          4
#define MV_Window_Type_Format           5
#define MV_Window_Type_Info             6
#define MV_Window_Type_Prefs            7
#define MV_Window_Type_Progress         8
#define MV_Window_Type_SmartReq_Delete  9
#define MV_Window_Type_Rename          10
#define MV_Window_Type_Makedir         11
#define MV_Window_Type_Cx              12
#define MV_Window_Type_Panelconfig     13
#define MV_Window_Type_Panel           14
#define MV_Window_Type_Panelitem       15
#define MV_Window_Type_unused          16
#define MV_Window_Type_Sound           17
#define MV_Window_Type_MIMEAdjust      18
#define MV_Window_Type_MIMEAction      19
#define MV_Window_Type_SystemInfo      20
#define MV_Window_Type_Makelink        21
#define MV_Window_Type_DestSelector    22
#define MV_Window_Type_Select          23
#define MV_Window_Type_Find            24
#define MV_Window_Type_ViewSelector    25
#define MV_Window_Type_PatternRename   26
#define MV_Window_Type_AddBookmark     27

/* Formatwin Format */
#define MV_Format_Format_Quick  0
#define MV_Format_Format_Format 1
#define MV_Format_Format_Verify 2

/* Fastlist Sorting modes (& 0xffff) */
#define MV_Listview_Sort_Name       0
#define MV_Listview_Sort_Size       1
#define MV_Listview_Sort_Date       2
#define MV_Listview_Sort_Comment    3
#define MV_Listview_Sort_Protection 4
#define MV_Listview_Sort_Owner      5
#define MV_Listview_Sort_Group      6
#define MV_Listview_Sort_DeviceName 7

#define MV_Listview_AddFiles_All    0
#define MV_Listview_AddFiles_New    1

#define MV_Listview_AddFile_Normal  0
#define MV_Listview_AddFile_New     1

#define MV_Listview_UpdateView_Normal 0
#define MV_Listview_UpdateView_New    1

/* Fastlist Sorting directions (& 0xffff0000) */
#define MV_Listview_Sort_Incremental (0 << 16L)
#define MV_Listview_Sort_Decremental (1 << 16L)

/* Makedirwin creation modes */
#define MV_Makedirwin_Makedir_With    0
#define MV_Makedirwin_Makedir_Without 1

/* Format */
#define MV_Format_NameStatus_Reserved -1L
#define MV_Format_NameStatus_Empty     0
#define MV_Format_NameStatus_Valid     1

/* Prefswin_Icondisplay_AdjustSize */
#define MV_Prefswin_Icondisplay_AdjustSize_Min 0
#define MV_Prefswin_Icondisplay_AdjustSize_Max 1

/* Application CreateWindow */
#define MV_Application_CreateWindow_Rootview 0
#define MV_Application_CreateWindow_View     1

/* Application LoadBackground */
#define MF_Application_LoadBackground_Root        (1 << 0UL)
#define MF_Application_LoadBackground_Window      (1 << 1UL)
#define MF_Application_LoadBackground_ClearRoot   (1 << 2UL)
#define MF_Application_LoadBackground_ClearWindow (1 << 3UL)

/* View SetStatus */
#define MF_View_SetStatus_Window (1 << 0UL) /* message go to the window */
#define MF_View_SetStatus_Bar    (1 << 1UL) /* message go to the statusbar, if not present and MF_View_SetStatus_Window is set it goes to the window then */
#define MF_View_SetStatus_Busy   (1 << 2UL) /* this means the view is currently busy and Ambient can show that state */

/* View Refresh */
#define MF_View_Refresh_Fonts      (1 << 0UL) /* font changed (size, space or color) */
#define MF_View_Refresh_Size       (1 << 1UL) /* size of contained objects changed (eg. icons). XXX: change the name or so.. */
#define MF_View_Refresh_Background (1 << 2UL) /* background changed */

/* View Select */
#define MV_View_Select_All      1UL
#define MV_View_Select_None     2UL
#define MV_View_Select_Invert   3UL
#define MV_View_Select_Pattern  4UL
#define MV_View_Select_Files    5UL
#define MV_View_Select_Dirs     6UL
#define MV_View_Select_Name     7UL

/* View Type */
#define MV_View_Type_Icon		1UL
#define MV_View_Type_List		2UL
#define MV_View_Type_Image		3UL
#define MV_View_Type_Boopsi		4UL
#define MV_View_Type_Text		5UL
#define MV_View_Type_ToolTypes  6UL

/* View Icon Selection */
#define MV_View_IconSelect_Unselect -1
#define MV_View_IconSelect_Clear     0
#define MV_View_IconSelect_Select    1

/* View housekeeping */
#define MV_View_NumSelected_Increase -1L
#define MV_View_NumSelected_Decrease -2L

/* View special types for deficonpool. use unknown only when deficons won't be used! */

#define MV_ViewID_Root     0L
#define MV_ViewID_Info    -1L
#define MV_ViewID_Panel   -2L
#define MV_ViewID_Unknown -3L

/* Imageview SetPos */
#define MV_Imageview_SetPos_TopIncrease  -1UL
#define MV_Imageview_SetPos_TopDecrease  -2UL
#define MV_Imageview_SetPos_LeftIncrease -3UL
#define MV_Imageview_SetPos_LeftDecrease -4UL

/* MenuItem subtype */
#define MV_Menuitem_SubType_None 0
#define MV_Menuitem_SubType_Mode 1
#define MV_Menuitem_SubType_View 2

/* Boopsiview SetPos */
#define MV_Boopsiview_SetPos_TopIncrease  -1UL
#define MV_Boopsiview_SetPos_TopDecrease  -2UL
#define MV_Boopsiview_SetPos_LeftIncrease -3UL
#define MV_Boopsiview_SetPos_LeftDecrease -4UL

/* Mimelisttree MA_Mimelisttree_ActiveType */
#define MV_Mimelisttree_ActiveType_None             0UL
#define MV_Mimelisttree_ActiveType_Media            1UL /* eg. application */
#define MV_Mimelisttree_ActiveType_Mime             2UL /* eg. application/foobar */
#define MV_Mimelisttree_ActiveType_MediaOverloaded  4UL /* eg. application-* overloaded */
#define MV_Mimelisttree_ActiveType_MimeOverloaded   8UL /* eg. application/foobar overloaded */

/* Infowin mode */
#define MV_Infowin_Changed_None        0UL
#define MV_Infowin_Changed_Icon        1UL
#define MV_Infowin_Changed_Stack       2UL
#define MV_Infowin_Changed_Protection  3UL
#define MV_Infowin_Changed_Comment     4UL
#define MV_Infowin_Changed_Tooltype    5UL
#define MV_Infowin_Changed_DefaultTool 6UL
#define MV_Infowin_Changed_Mode        7UL

/* Textview */
#define MV_Textview_SearchCurrent       0UL
#define MV_Textview_SearchNext          1UL
#define MV_Textview_SearchPrev          2UL
#define MV_Textview_SearchTop           3UL
#define MV_Textview_SearchBottom        4UL

/* Searchbar */
#define MV_Search_SearchNext            0UL
#define MV_Search_SearchPrev            1UL
#define MV_Search_SearchCurrent         2UL

#define MV_Searchbar_Flags_ShowNext		1UL
#define MV_Searchbar_Flags_ShowPrev		2UL
#define MV_Searchbar_Flags_ShowCase		4UL


/* Toolbargroup */
#define MV_Toolbargroup_Definition_Updated 0UL /* used to trigger notifications...*/
#define MV_Toolbargroup_Definition_All     1UL
#define MV_Toolbargroup_Definition_Default 2UL

/* Toolbar */
#define MV_Toolbar_Path_NewPath 0
#define MV_Toolbar_Path_Refresh 1
#define MV_Toolbar_Path_Parent  2

/* Toolbutton */
#define MV_Toolbutton_Location_Toolbar   0  /* normal view toolbar           */
#define MV_Toolbutton_Location_Samplebar 1  /* sample toolbar in preferences */
#define MV_Toolbutton_Location_Storage   2  /* button storage in preferences */

/* Clickpath clicks */
#define MV_Clickpath_Clicked_Parent 1

/* Notification */
#define MV_Notification_Error   0L
#define MV_Notification_Help    1L /* default */
#define MV_Notification_Warning 2L

/* Actionedit commandsets */
#define MV_Actionedit_Commandset_Normal 0L

/* Statusbar modes */
#define MV_Statusbar_SetMode_Icon 0L
#define MV_Statusbar_SetMode_List 1L

#endif /* AMBIENT_CLASSES_H */
