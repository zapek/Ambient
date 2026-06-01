#ifndef AMBIENT_THREADS_H
#define AMBIENT_THREADS_H
/*
 * $Id: threads.h,v 1.23 2025/08/13 16:17:01 jacadcaps Exp $
 */

#include <utility/tagitem.h>

struct Process;

ULONG threads_init(void);
void threads_cleanup(void);
ULONG threads_finish(ULONG loop);
ULONG v_do_action(APTR obj, int action, struct TagItem *tags);
ULONG v_do_action_sync(APTR obj, int action, struct TagItem *tags);
ULONG do_action(APTR obj, int action, ...);
ULONG do_action_sync(APTR obj, int action, ...);
void threads_handle(void);
void threads_abort(APTR obj, ...);
void threads_reclaim(void);
ULONG threads_check_abort(void);
ULONG threads_waitsig(ULONG usersigs, ULONG *aborted);

void thread_wait( void );
#if USE_THREADPOOL
ssize_t thread_get( void );
#else
APTR thread_get( void );
#endif
void thread_signal( APTR thread, BOOL force );

extern ULONG threadsig;

extern struct Task *maintask;
#define IS_MAINTASK (FindTask(NULL) == maintask)

/*
 * Thread states
 */
#define TS_IDLE 0
#define TS_BUSY 1
#define TS_WAITING 2



/*
 * Special tags.
 */
#define TAG_STRING (1 << 30)
#define TAG_WBLIST (1 << 29)


/*
 * Thread actions.
 */
enum {
	TA_Slack = 1,
	TA_Background_Load,
	TA_WBStartup_Execute,
	TA_Devices_Show,
	TA_Devices_Add,
	TA_Devices_Remove,
	TA_Devices_UpdateInfo,
	TA_File_Delete,
	TA_File_FindVer,
	TA_File_Move,
	TA_File_Rename,
	TA_File_Makedir,
	TA_File_Makelink,
	TA_File_GetSize,
	TA_File_ScanDir,
	TA_File_MD5sum,
	TA_Icon_Snapshot,
	TA_Icon_Unsnapshot,
	TA_Icon_Read,
	TA_Icon_Write,
	TA_Icon_GetProperties,
	TA_AppMsg_Send,
	TA_Disk_Format,
	TA_Window_Snapshot,
	TA_Prefs_Save,
	TA_Panels_LoadAll,
	TA_Panels_Save,
	TA_Panels_Delete,
	TA_Panels_Move,
	TA_Panels_Zip,
	TA_Appicon_Read,
	TA_Infowin_Open,
	TA_Shortcuts_Add,
	TA_Shortcuts_SaveAll,
	TA_URI_Load,
	TA_Sound_Play,
	TA_Imageview_Load,
	TA_Imageview_Scale,
	TA_Imageview_ScanPics,
	TA_Textview_Load,
	TA_Boopsiview_Load,
	TA_Font_Load,
	TA_WBStart,
	TA_Infoicon_Load,
	TA_Thumbnail_Create,
	TA_Metadata_Gather,
	TA_Devices_RemoveAll,
	TA_ActionDispatcher_Execute,
	TA_MimeType_Scan,
	TA_Clipboard_Paste,
	TA_Listview_CreateIcons,
	TA_Listview_ShowDevices,
	TA_Version_Find,
	TA_Find,
	TA_DesktopPrefs_Save,
	TA_File_GetSizes,
	TA_Dispose_Objects,
	TA_Listview_ShowPreview,
	TA_SmartReq_ExamineFiles,
	TA_StartupPrefs_Load,
	TA_Print,
	TA_Iconview_ShowPreview,
	TA_File_Trash,
	TA_File_Restore,
};

/*
 * General tags.
 */
#define TT_Priority 0xf001 /* priority of the thread */
#define TT_Object   0xf002 /* if set, send the MM_Thread_Finished there instead. use only in special cases */


/*
 * TA_Slack
 * --------
 * Goes to sleep.
 */

/*
 * TA_Background_Load
 * ------------------
 * Loads a background picture.
 */
#define TT_Background_Load_Path       (TAG_STRING | (TAG_USER + 1))
#define TT_Background_Load_Type                     (TAG_USER + 2)
 #define TV_Background_Load_Type_Root   0
 #define TV_Background_Load_Type_Window 1
#define TT_Background_Load_Mode                     (TAG_USER + 3)
 /* XXX: put them here or so */

/*
 * TA_WBStartup_Execute
 * --------------------
 * Starts the programs in WBStartup.
 */
#define TT_WBStartup_Execute_Path     (TAG_STRING | (TAG_USER + 1))

/*
 * TA_Devices_Show
 * ---------------
 * Adds devices to the root window.
 */
#define TT_Devices_Show_Assigns                     (TAG_USER + 1)
#define TT_Devices_Show_IsRoot                      (TAG_USER + 2)

/*
 * TA_Devices_Add
 * --------------
 * Adds a device.
 */

/*
 * TA_Devices_Remove
 * -----------------
 * Removes a device.
 */

/*
 * TA_Devices_UpdateInfo
 * -----------------
 * Updates info displayed under each device
 */

/*
 * TA_File
 * --------------
 * General tags
 */
#define TT_File_BaseDir               (TAG_STRING | (TAG_USER + 100))
#define TT_File_Notify                              (TAG_USER + 101)

/*
 * TA_File_Delete
 * --------------
 * Deletes a file.
 */
#define TT_File_Delete_PathList                     (TAG_USER + 1)
#define TT_File_Delete_Refwin                       (TAG_USER + 2)
#define TT_File_Delete_RexxID                       (TAG_USER + 3)
#define TT_File_Delete_RxDel                        (TAG_USER + 4)
#define TT_File_Delete_NoIcon                       (TAG_USER + 5)

/*
 * TA_File_FindVer
 * ---------------
 * Finds and returns the version in a file.
 */
#define TT_File_FindVer_Path          (TAG_STRING | (TAG_USER + 1))

/*
 * TA_File_Move
 * ------------
 * Moves/copies a file.
 */
#define TT_File_Move_SrcPath          (TAG_STRING | (TAG_USER + 1))
#define TT_File_Move_DstPath          (TAG_STRING | (TAG_USER + 2))
#define TT_File_Move_Copy                           (TAG_USER + 3)
#define TT_File_Move_Refwin                         (TAG_USER + 4)
#define TT_File_Move_SrcList                        (TAG_USER + 5)
#define TT_File_Move_Flags                          (TAG_USER + 6)
#define TT_File_Move_NoIcon                         (TAG_USER + 7)
#define TT_File_Move_Rename                         (TAG_USER + 8)

/*
 * TA_File_Rename
 * --------------
 * Renames a file/directory.
 */
#define TT_File_Rename_Path           (TAG_STRING | (TAG_USER + 1))
#define TT_File_Rename_Name           (TAG_STRING | (TAG_USER + 2))
#define TT_File_Rename_NoIcon                       (TAG_USER + 3)
#define TT_File_Rename_PathList                     (TAG_USER + 4)

/*
 * TA_File_Makedir
 * ---------------
 * Creates a directory.
 */
#define TT_File_Makedir_Path          (TAG_STRING | (TAG_USER + 1))
#define TT_File_Makedir_Icon                        (TAG_USER + 2)

/*
 * TA_File_Makelink
 * ---------------
 * Creates a directory.
 */
#define TT_File_Makelink_Object       (TAG_STRING | (TAG_USER + 1))
#define TT_File_Makelink_Link         (TAG_STRING | (TAG_USER + 2))

/*
 * TA_File_GetSize
 * --------------------
 * Gets the size of a directory (recursive). (XXX: make it work on files, or fail, or so)
 */
#define TT_File_GetSize_Path          (TAG_STRING | (TAG_USER + 1))

/*
 * TA_File_ScanDir
 * ---------------
 * Scans a directory.
 */
#define TT_File_ScanDir_Path          (TAG_STRING | (TAG_USER + 1))
#define TT_File_ScanDir_Mode                        (TAG_USER + 2)

 #define TV_File_ScanDir_Mode_Icons   0 /* add icons with .info */
 #define TV_File_ScanDir_Mode_NoIcons 1 /* add files without .info */
 #define TV_File_ScanDir_Mode_ShowAll 2 /* add all files with and without .info */
 #define TV_File_ScanDir_Mode_Files   3 /* add files, no icon processing */
 #define TV_File_ScanDir_Mode_Thumbs  4 /* adds all files with and without .info and thumbnails */
 #define TV_File_ScanDir_Mode_FilesNoIcons 5 /* adds all files except .info linked files */
 #define TV_File_ScanDir_Mode_FilesIcons   6 /* adds files with .info */
/*
 * TA_File_MD5sum
 * --------------
 * MD5sums a file.
 */
#define TT_File_MD5sum_Path           (TAG_STRING | (TAG_USER + 1))
#define TT_File_MD5sum_List                         (TAG_USER + 2)

/*
 * TA_Icon_Snapshot
 * ----------------
 * (Un)Snapshots an icon.
 */
#define TT_Icon_Snapshot_Path         (TAG_STRING | (TAG_USER + 1))
#define TT_Icon_Snapshot_X                          (TAG_USER + 2)
#define TT_Icon_Snapshot_Y                          (TAG_USER + 3)
#define TT_Icon_Snapshot_List                       (TAG_USER + 4)

/*
 * TA_Icon_Read
 * ------------
 * Reads an icon.
 */
#define TT_Icon_Read_Path             (TAG_STRING | (TAG_USER + 1))
#define TT_Icon_Read_NoInfo                         (TAG_USER + 2)

/*
 * TA_Icon_Write
 * -------------
 * Writes an icon.
 */
#define TT_Icon_Write_Path            (TAG_STRING | (TAG_USER + 1))
/* XXX: chose dynamically.. */
#define TT_Icon_Write_Flags                         (TAG_USER + 2) /* XXX: should that be flags really ? */
#define TT_Icon_Write_Comment         (TAG_STRING | (TAG_USER + 3))
#define TT_Icon_Write_Mode                          (TAG_USER + 4) /* Do not write icon image data */
 #define TV_Icon_Write_Mode_NoIcon   0
 #define TV_Icon_Write_Mode_DefIcon  1
 #define TV_Icon_Write_Mode_FullIcon 2

/*
 * TA_Icon_GetProperties
 * ---------------------
 * Get icon properties.
 */
#define TT_Icon_GetProperties_Message               (TAG_USER + 1)

/*
 * TA_AppMsg_Send
 * --------------
 * Sends an appmessage.
 */
#define TT_AppMsg_Send_Type                         (TAG_USER + 1)
#define TT_AppMsg_Send_MsgPort                      (TAG_USER + 2)
#define TT_AppMsg_Send_Window                       (TAG_USER + 3)
#define TT_AppMsg_Send_Path           (TAG_STRING | (TAG_USER + 4))
#define TT_AppMsg_Send_Class                        (TAG_USER + 5)
#define TT_AppMsg_Send_ID                           (TAG_USER + 6)
#define TT_AppMsg_Send_Userdata                     (TAG_USER + 7)
#define TT_AppMsg_Send_NumArgs                      (TAG_USER + 8)
#define TT_AppMsg_Send_WBArgList      (TAG_WBLIST | (TAG_USER + 9))
#define TT_AppMsg_Send_MouseX                       (TAG_USER + 10)
#define TT_AppMsg_Send_MouseY                       (TAG_USER + 11)

/*
 * TA_Disk_Format
 * --------------
 * Formats a disk.
 */
#define TT_Disk_Format_Device         (TAG_STRING | (TAG_USER + 1))
#define TT_Disk_Format_DeviceInfo                   (TAG_USER + 2)
#define TT_Disk_Format_Mode                         (TAG_USER + 3)
#define TT_Disk_Format_FileSystem                   (TAG_USER + 4)
#define TT_Disk_Format_Flags                        (TAG_USER + 5) /* XXX: hm.. why flags ? */

/*
 * TA_Window_Snapshot
 * ------------------
 * (Un)Snapshots a window.
 */
#define TT_Window_Snapshot_Path       (TAG_STRING | (TAG_USER + 1))
#define TT_Window_Snapshot_X                        (TAG_USER + 2)
#define TT_Window_Snapshot_Y                        (TAG_USER + 3)
#define TT_Window_Snapshot_XS                       (TAG_USER + 4)
#define TT_Window_Snapshot_YS                       (TAG_USER + 5)
#define TT_Window_Snapshot_Flags                    (TAG_USER + 6) /* XXX: change that ? */

/*
 * TA_Prefs_Save
 * -------------
 * Saves the prefspool.
 */
#define TT_Prefs_Save_Ctx                           (TAG_USER + 1)

/*
 * TA_Panels_LoadAll
 * -----------------
 * Loads all panels.
 */

/*
 * TA_Panels_Save
 * --------------
 * Saves the panel.
 */
#define TT_Panels_Save_Ctx                          (TAG_USER + 1)

/*
 * TA_Panels_Delete
 * ----------------
 * Deletes a panel.
 */
#define TT_Panels_Delete_Ctx                        (TAG_USER + 1)

/*
 * TA_Panels_Move
 * --------------
 * Moves a panel on screen.
 */
#define TT_Panels_Move_Window                       (TAG_USER + 1)
#define TT_Panels_Move_X                            (TAG_USER + 2)
#define TT_Panels_Move_Y                            (TAG_USER + 3)

/*
 * TA_Panels_Zip
 * -------------
 * Zip/Unzips a panel.
 */
#define TT_Panels_Zip_Window                        (TAG_USER + 1)
#define TT_Panels_Zip_XS                            (TAG_USER + 2)
#define TT_Panels_Zip_YS                            (TAG_USER + 3)
#define TT_Panels_Zip_Reversed                      (TAG_USER + 4)
#define TT_Panels_Zip_ZipSpeed                      (TAG_USER + 5)
#define TT_Panels_Zip_Direction                     (TAG_USER + 6)

/*
 * TA_Appicon_Read
 * ---------------
 * Reads an appicon.
 */
#define TT_Appicon_Read_Message                     (TAG_USER + 1)
#define TT_Appicon_Read_Object                      (TAG_USER + 2)

/*
 * TA_Infowin_Open
 * ---------------
 * Opens the infowindow with infos fully
 * gathered from the file.
 */
#define TT_Infowin_Open_PathList                    (TAG_USER + 1)
#define TT_Infowin_Open_Wait                        (TAG_USER + 2)
#define TT_Infowin_Open_RexxID                      (TAG_USER + 3)

/*
 * TA_Shortcuts_Add
 * ----------------
 * Adds a shortcut to the desktop.
 */
#define TT_Shortcuts_Add_Path         (TAG_STRING | (TAG_USER + 1))
#define TT_Shortcuts_Add_X                          (TAG_USER + 2)
#define TT_Shortcuts_Add_Y                          (TAG_USER + 3)
#define TT_Shortcuts_Add_List                       (TAG_USER + 4) /* XXX: merge the calls */
#define TT_Shortcuts_Add_Type                       (TAG_USER + 5)

/*
 * TA_Shortcuts_SaveAll
 * --------------------
 * Saves all shortcuts.
 */

/*
 * TA_URI_Load
 * -----------
 * Loads an URI.
 */
#define TT_URI_Load_Ctx                             (TAG_USER + 1)
#define TT_URI_Load_URI               (TAG_STRING | (TAG_USER + 2))
#define TT_URI_Load_ID                              (TAG_USER + 3)
#define TT_URI_Load_Newwin                          (TAG_USER + 4)
#define TT_URI_Load_Browser                         (TAG_USER + 5)
#define TT_URI_Load_Iconified                       (TAG_USER + 6)
#define TT_URI_Load_ToFront                         (TAG_USER + 7) // put the window to front&activate it
#define TT_URI_Load_Screen                          (TAG_USER + 8) // returned by get_screen_id

/*
 * TA_Sound_Play
 * -------------
 * Plays a sound.
 */
#define TT_Sound_Play_Path            (TAG_STRING | (TAG_USER + 1))
#define TT_Sound_Play_Mode                          (TAG_USER + 2) /* XXX: add a datatype mode, and merge */

/*
 * TA_Imageview_Load
 * -----------------
 * Loads an image into the imageview.
 */
#define TT_Imageview_Load_Path        (TAG_STRING | (TAG_USER + 1))

/*
 * TA_Imageview_Scale
 * ------------------
 * Scales an image in the imageview.
 */
#define TT_Imageview_Scale_Dtp                      (TAG_USER + 1)
#define TT_Imageview_Scale_XS                       (TAG_USER + 2)
#define TT_Imageview_Scale_YS                       (TAG_USER + 3)

/*
 * TA_Imageview_ScanPics
 * ---------------------
 * Scans a directory to gather pictures in it.
 */
#define TT_Imageview_ScanPics_Path    (TAG_STRING | (TAG_USER + 1))
#define TT_Imageview_ScanPics_Mode                  (TAG_USER + 2)
 #define TV_Imageview_ScanPics_Mode_Next 0
 #define TV_Imageview_ScanPics_Mode_Prev 1

/*
 * TA_Textview_Load
 * ----------------
 * Loads a text into the textview.
 */
#define TT_Textview_Load_Path         (TAG_STRING | (TAG_USER + 1))

/*
 * TA_Boopsiview_Load
 * ------------------
 * Loads a datatype into the boopsiview.
 */
#define TT_Boopsiview_Load_Path       (TAG_STRING | (TAG_USER + 1))

/*
 * TA_Font_Load
 * ------------
 * Loads a font.
 */
#define TT_Font_Load_Path             (TAG_STRING | (TAG_USER + 1))
#define TT_Font_Load_Type                           (TAG_USER + 2)
 #define TV_Font_Load_Type_Root   0
 #define TV_Font_Load_Type_Window 1
 #define TV_Font_Load_Type_Root_Small 2
 #define TV_Font_Load_Type_Lister 3

/*
 * TA_WBStart
 * ----------
 * Executes a file in "workbench" mode.
 */
#define TT_WBStart_Path               (TAG_STRING | (TAG_USER + 1))
#define TT_WBStart_Priority                         (TAG_USER + 2)
#define TT_WBStart_Stack                            (TAG_USER + 3)
#define TT_WBStart_Argument                         (TAG_USER + 4)
#define TT_WBStart_Argument_List                    (TAG_USER + 5 )
#define TT_WBStart_FreeNames                        (TAG_USER + 6)

/*
 * TA_Infoicon_Load
 * ----------------
 * Loads an icon into an infowin for further
 * saving.
 */
#define TT_Infoicon_Load_Path         (TAG_STRING | (TAG_USER + 1))
#define TT_Infoicon_Load_DefIcon                    (TAG_USER + 2)

/*
 * TA_Thumbnail_Create
 * -------------------
 * Turns a file into a thumbnail.
 */
#define TT_Thumbnail_Create_Path      (TAG_STRING | (TAG_USER + 1))
#define TT_Thumbnail_Create_List                    (TAG_USER + 2)
#define TT_Thumbnail_Create_Object                  (TAG_USER + 3)

/*
 * TA_Metadata_Gather
 * ------------------
 * Finds out some infos about a file/dir.
 */
#define TT_Metadata_Gather_Path                     (TAG_STRING | (TAG_USER + 1))
#define TT_Metadata_Gather_MimeTypeDescription      (TAG_STRING | (TAG_USER + 2))

/*
 * TA_Device_Add
 *--------------
 * Adds device/devices to a view.
 *
 */
#define TT_Devices_Fade                             (TAG_USER + 1)

/*
 * TA_ActionDispatcher_Execute
 * ------------------
 * Executes list of actions collected in actiondispatcher object.
 */

/*
 * TA_Mimetype_Scan
 * ------------------
 * Executes list of actions collected in actiondispatcher object.
 */
#define TT_MimeType_Scan_List                       (TAG_USER + 1)
#define TT_MimeType_Scan_Path         (TAG_STRING | (TAG_USER + 2))
#define TT_MimeType_Scan_MimetypeObject             (TAG_USER + 3)
#define TT_MimeType_Scan_GenerateDefIcons           (TAG_USER + 4)
#define TT_MimeType_Scan_GenerateThumbs             (TAG_USER + 5)
#define TT_MimeType_Scan_MatchFirst                 (TAG_USER + 6) /* will examine first entry on list and check if rest matches */

/*
 * Threads return:
 * TRUE: all fine
 * FALSE: error, couldn't complete
 * ABORTED: aborted by user
 */
/*
 * TA_Clipboard_Paste
 * ----------
 * Pastes the clipboard
 */
#define TT_Clipboard_Paste_DestPath   (TAG_STRING | (TAG_USER + 1))
#define TT_Clipboard_Paste_ViewID                   (TAG_USER + 2)
#define TT_Clipboard_Paste_As                       (TAG_USER + 3)


/*
 * Threads return:
 * TRUE: all fine
 * FALSE: error, couldn't complete
 * ABORTED: aborted by user
 */
/*
 * TA_Listview_CreateIcon
 * ----------
 * creates a listviewicon
 */
#define TT_Listview_Path              (TAG_STRING | (TAG_USER + 1))
#define TT_Listview_Selected                        (TAG_USER + 2)
#define TT_Listview_Data                            (TAG_USER + 3)
#define TT_Listview_Iconlist                        (TAG_USER + 4)
#define TT_Listview_Entry                           (TAG_USER + 5)
#define TT_Listview_Obj                             (TAG_USER + 6)
#define TT_Listview_Class                           (TAG_USER + 7)

/*
 * TA_Find
 * ----------
 * looks for files using various filters
 */
#define TT_Find_Window                              (TAG_USER + 1)

/*
 * TA_Version_Find
 * ----------
 * gets version for file (more generic than TA_File_FindVer)
 */
#define TT_Version_Find_List                        (TAG_USER + 1)

/*
 * TA_File_GetSizes
 * ----------
 * gets directories' sizes 
 */
#define TT_File_GetSizes_List                       (TAG_USER + 1)

/*
 * TA_Object_Dispose
 * ----------
 * Disposes given objects asynchronously. Use for bigger amount of them
 */
#define TT_Dispose_Objects_Array                    (TAG_USER + 1)
#define TT_Dispose_Objects_FreeArray                (TAG_USER + 2)

/*
 * TA_SmartReq_ExamineFiles
 * ----------
 * Get file information for replace requester.
 */
#define TT_SmartReq_File1             (TAG_STRING | (TAG_USER + 1))
#define TT_SmartReq_File2             (TAG_STRING | (TAG_USER + 2))
#define TT_SmartReq_Data                            (TAG_USER + 3)

/*
 * TA_Print
 * ----------
 * Print stuff.
 */
#define TT_Print_Source                             (TAG_USER + 1)
#define TT_Print_Type                               (TAG_USER + 2)

/*
 * TA_Iconview_ShowPreview
 * ----------
 * ShowPreview in iconview
 */
#define TT_Iconview_ShowPreview_Object              (TAG_USER + 1)

#define ABORTED -1L

#endif /* AMBIENT_THREADS_H */
