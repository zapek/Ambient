/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2016 Ambient Open Source Team
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * $Id: thread_dispatcher.c,v 1.12 2020/08/16 21:10:18 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>
#include <proto/utility.h>

/* private */
#include "dosreq.h"
#include "print.h"
#include "prefs_desktop.h"
#include "smartreq.h"
#include "threads.h"
#include "methodstack.h"
#include "mui_func.h"
#include "prefs_startup.h"
#include "scandir.h"
#include "background.h"
#include "threadtags.h"
#include "wbstartup.h"
#include "devices.h"
#include "iconio.h"
#include "taskdata.h"
#include "getdirsize.h"
#include "findver.h"
#include "movedir.h"
#include "movelist.h"
#include "deleteall.h"
#include "format.h"
#include "wbstart.h"
#include "ipc.h"
#include "snapshot.h"
#include "snapshotlist.h"
#include "rexx.h"
#include "md5sum.h"
#include "appmessage.h"
#include "prefsio.h"
#include "panelprefs.h"
#include "paneleffects.h"
#include "appicon.h"
#include "infowin.h"
#include "shortcuts.h"
#include "loaduri.h"
#include "rename.h"
#include "tags.h"
#include "playsound.h"
#include "loadimage.h"
#include "imageview.h"
#include "textview.h"
#include "boopsiview.h"
#include "makedir.h"
#include "makelink.h"
#include "fonts.h"
#include "infoicongroup.h"
#include "thumbs.h"
#include "metadata.h"
#include "actiondispatcherclass.h"
#include "typescanner.h"
#include "clipboard.h"
#include "dragdrop.h"
#include "listviewclass.h"
#include "debug.h"
#include "findclass.h"
#include "appclass.h"
#include "paneltags.h"
#include "threads.h"
#include "thread_dispatcher.h"
#include "trashcan.h"
#include "name.h"

ULONG thread_domsg(struct thread_msg *msg)
{
	#if USE_THREADPOOL
	ULONG action = msg->action;
	#else
	ULONG action = msg->tn ? msg->tn->action : msg->action;
	#endif

	D(PROC, bug("processing message action %ld..\n", action));

	switch (action)
	{
		case TA_Slack:
			return (TRUE);

		case TA_File_ScanDir:
			return (tr_scandir(msg->obj,
				(STRPTR)GetTagData(TT_File_ScanDir_Path, (ULONG)NULL, msg->taglist),
				GetTagData(TT_File_ScanDir_Mode, TV_File_ScanDir_Mode_Files, msg->taglist)
			));

		case TA_Background_Load:
			return (tr_background_load(msg->obj,
				GetTagData(TT_Background_Load_Type, 0, msg->taglist),
				GetTagData(TT_Background_Load_Mode, 0, msg->taglist),
				(STRPTR)GetTagData(TT_Background_Load_Path, (ULONG)NULL, msg->taglist)
			));

		case TA_WBStartup_Execute:
			return (tr_wbstartup_execute((STRPTR)GetTagData(TT_WBStartup_Execute_Path, (ULONG)NULL, msg->taglist)
			));

		case TA_Devices_Show:
			return (tr_devices_show(msg->obj,
				GetTagData(TT_Devices_Show_Assigns, FALSE, msg->taglist),
				GetTagData(TT_Devices_Show_IsRoot, FALSE, msg->taglist),
				FALSE
			));

		case TA_File_Delete:
			return (tr_deleteall(msg->obj,
				(APTR)GetTagData(TT_File_Delete_Refwin, (ULONG)NULL, msg->taglist),
				(STRPTR *)GetTagData(TT_File_Delete_PathList, (ULONG)NULL, msg->taglist),
				(ULONG)GetTagData(TT_File_Delete_NoIcon, (ULONG)NULL, msg->taglist)
			));

		case TA_File_Trash:
			return (tr_trashall(msg->obj,
				(APTR)GetTagData(TT_File_Delete_Refwin, (ULONG)NULL, msg->taglist),
				(STRPTR *)GetTagData(TT_File_Delete_PathList, (ULONG)NULL, msg->taglist),
				(ULONG)GetTagData(TT_File_Delete_NoIcon, (ULONG)NULL, msg->taglist)
			));

		case TA_File_Restore:
			return (tr_restoreall(msg->obj,
				(APTR)GetTagData(TT_File_Delete_Refwin, (ULONG)NULL, msg->taglist),
				(STRPTR *)GetTagData(TT_File_Delete_PathList, (ULONG)NULL, msg->taglist),
				(ULONG)GetTagData(TT_File_Delete_NoIcon, (ULONG)NULL, msg->taglist)
			));

		case TA_Icon_Snapshot:
			{
				struct MinList *l = (struct MinList *)GetTagData(TT_Icon_Snapshot_List, (ULONG)NULL, msg->taglist);

				if (l)
				{
					/* NOTE: tr_snapshot_list frees the nodes and the list! */
					return (tr_snapshot_list(l));
				}
				else
				{
					return (tr_snapshot_icon(
						(STRPTR)GetTagData(TT_Icon_Snapshot_Path, (ULONG)NULL, msg->taglist),
						(LONG)GetTagData(TT_Icon_Snapshot_X, 0, msg->taglist),
						(LONG)GetTagData(TT_Icon_Snapshot_Y, 0, msg->taglist)
					));
				}
			}
			break;

		case TA_Icon_Unsnapshot:
			return (tr_snapshot_icon(
				(STRPTR)GetTagData(TT_Icon_Snapshot_Path, (ULONG)NULL, msg->taglist),
				NO_ICON_POSITION, NO_ICON_POSITION
			));
			break;

		case TA_Window_Snapshot:
			return (tr_snapshot_window(
				(STRPTR)GetTagData(TT_Window_Snapshot_Path, (ULONG)NULL, msg->taglist),
				(LONG)GetTagData(TT_Window_Snapshot_X, 0, msg->taglist),
				(LONG)GetTagData(TT_Window_Snapshot_Y, 0, msg->taglist),
				GetTagData(TT_Window_Snapshot_XS, 0, msg->taglist),
				GetTagData(TT_Window_Snapshot_YS, 0, msg->taglist),
				GetTagData(TT_Window_Snapshot_Flags, 0, msg->taglist)
			));

		case TA_File_GetSize:
			return (tr_getdirsize(msg->obj,
				(STRPTR)GetTagData(TT_File_GetSize_Path, (ULONG)NULL, msg->taglist)
			));

		case TA_File_FindVer:
			return (tr_findver(msg->obj,
				(STRPTR)GetTagData(TT_File_FindVer_Path, (ULONG)NULL, msg->taglist)
			));

		case TA_File_Move:
			if (is_trashcan((STRPTR)GetTagData(TT_File_Move_DstPath, (ULONG)NULL, msg->taglist)))
			{
				ULONG rc = FALSE;
				struct MinList *ml = (struct MinList *)GetTagData(TT_File_Move_SrcList, (ULONG)NULL, msg->taglist);
				STRPTR *dirlist = NULL;
				STRPTR srcpath = (STRPTR)GetTagData(TT_File_Move_SrcPath, (ULONG)NULL, msg->taglist);
				ULONG numfiles = 1;
					
				if (ml)
				{
					struct dragdropnode *ddn = FIRSTNODE(ml);

					numfiles = 0;
					
					ITERATELIST(ddn, ml)
					{
						numfiles ++;
					}
				}
				
				dirlist = malloc((numfiles+1) * sizeof(ULONG));

				if (dirlist)
				{
					ULONG i = 0;
			
					if (ml)
					{
						struct dragdropnode *ddn = FIRSTNODE(ml);

						ITERATELIST(ddn, ml)
						{
							dirlist[i] = name_build(ddn->path);
							i++;
						}
						
						dirlist[i] = NULL;
					}
					else
					{
						dirlist[0] = name_build(srcpath);
						dirlist[1] = NULL;
					}

					rc = tr_trashall(msg->obj,
						(APTR)GetTagData(TT_File_Move_Refwin, (ULONG)NULL, msg->taglist),
						dirlist,
						(ULONG)GetTagData(TT_File_Move_NoIcon, (ULONG)NULL, msg->taglist));
	
					i = 0;
					do
					{
						name_delete(dirlist[i]);
						i++;
					} while (dirlist[i]);
					
					free(dirlist);					
				}
				
				if (ml)
				{
					struct dragdropnode *ddn, *nextddn;
					ITERATELISTSAFE(ddn, nextddn, ml)
					{
						free(ddn);
					}
					free(ml);
				}
				
				return rc;
			}
			else
			return(tr_move(msg->obj,
					(APTR)GetTagData(TT_File_Move_Refwin, (ULONG)NULL, msg->taglist),
					(struct MinList *)GetTagData(TT_File_Move_SrcList, (ULONG)NULL, msg->taglist),
					(STRPTR)GetTagData(TT_File_Move_SrcPath, (ULONG)NULL, msg->taglist),
					(STRPTR)GetTagData(TT_File_Move_DstPath, (ULONG)NULL, msg->taglist),
					GetTagData(TT_File_Move_Copy, FALSE, msg->taglist),
					GetTagData(TT_File_Move_NoIcon, FALSE, msg->taglist),
					GetTagData(TT_File_Move_Rename, FALSE, msg->taglist)
			));

		case TA_AppMsg_Send:
			return (tr_appmessage_send(
				GetTagData(TT_AppMsg_Send_Type, (ULONG)NULL, msg->taglist),
				(struct MsgPort *)GetTagData(TT_AppMsg_Send_MsgPort, (ULONG)NULL, msg->taglist),
				(struct Window *)GetTagData(TT_AppMsg_Send_Window, (ULONG)NULL, msg->taglist),
				(CONST_STRPTR)GetTagData(TT_AppMsg_Send_Path, (ULONG)NULL, msg->taglist),
				GetTagData(TT_AppMsg_Send_Class, (ULONG)NULL, msg->taglist),
				GetTagData(TT_AppMsg_Send_ID, (ULONG)NULL, msg->taglist),
				GetTagData(TT_AppMsg_Send_Userdata, (ULONG)NULL, msg->taglist),
				GetTagData(TT_AppMsg_Send_NumArgs, 0, msg->taglist),
				(struct WBArg *)GetTagData(TT_AppMsg_Send_WBArgList, (ULONG)NULL, msg->taglist),
				GetTagData(TT_AppMsg_Send_MouseX, 0, msg->taglist),
				GetTagData(TT_AppMsg_Send_MouseY, 0, msg->taglist)
			));

		case TA_Devices_Add:
			return (tr_devices_add(msg->obj,
				GetTagData(TT_Devices_Fade, TRUE, msg->taglist)
			));

		case TA_Devices_Remove:
			return (tr_devices_remove(msg->obj));

		case TA_Devices_RemoveAll:
			return (tr_devices_removeall(msg->obj));

		case TA_Devices_UpdateInfo:
			return (tr_devices_updateinfo(msg->obj));

		case TA_Disk_Format:
			return (tr_format(msg->obj,
				(STRPTR)GetTagData(TT_Disk_Format_Device, (ULONG)NULL, msg->taglist),
				(struct device_info *)GetTagData(TT_Disk_Format_DeviceInfo, (ULONG)NULL, msg->taglist),
				GetTagData(TT_Disk_Format_Mode, (ULONG)NULL, msg->taglist),
				GetTagData(TT_Disk_Format_FileSystem, (ULONG)NULL, msg->taglist),
				GetTagData(TT_Disk_Format_Flags, 0, msg->taglist)
			));

		case TA_Icon_Write:
			return tr_icon_update(msg->obj,
				(STRPTR)GetTagData(TT_Icon_Write_Path, (ULONG)NULL, msg->taglist),
				(STRPTR)GetTagData(TT_Icon_Write_Comment, (ULONG)NULL, msg->taglist),
				        GetTagData(TT_Icon_Write_Flags, 0, msg->taglist),
				        GetTagData(TT_Icon_Write_Mode, TV_Icon_Write_Mode_NoIcon, msg->taglist)
			);

		case TA_Icon_GetProperties:
			{
				struct ipcmessage *imsg = (struct ipcmessage *)GetTagData(TT_Icon_GetProperties_Message, (ULONG)NULL, msg->taglist);
				struct ipc_wbstartlib *wbsm = imsg->msgtype;
				ASSERT(imsg);
				ASSERT(wbsm);

				wbsm->status = get_icon_properties(wbsm->filename, wbsm->type, wbsm->pri, wbsm->stacksize, wbsm->defaulttool);
				ReplyMsg(&imsg->msg);
			}
			return (TRUE);

		case TA_File_MD5sum:
			{
				struct MinList *l = (struct MinList *)GetTagData(TT_File_MD5sum_List, (ULONG)NULL, msg->taglist);

				if (l)
				{
					return (tr_md5sum_list(msg->obj,
						l
					));
				}
				else
				{
					return (tr_md5sum(msg->obj,
						(STRPTR)GetTagData(TT_File_MD5sum_Path, (ULONG)NULL, msg->taglist)
					));
				}
			}
			break;

		case TA_Prefs_Save:
			return (tr_prefs_save(msg->obj,
				(APTR)GetTagData(TT_Prefs_Save_Ctx, (ULONG)NULL, msg->taglist)
			));

		case TA_Panels_LoadAll:
			return (tr_panels_loadall());

		case TA_Panels_Save:
			return (tr_panels_save(msg->obj,
				(APTR)GetTagData(TT_Panels_Save_Ctx, (ULONG)NULL, msg->taglist)
			));

		case TA_Panels_Delete:
			return (tr_panels_delete(msg->obj,
				(APTR)GetTagData(TT_Panels_Delete_Ctx, (ULONG)NULL, msg->taglist)
			));

		case TA_Panels_Move:
			return (tr_panels_move(msg->obj,
				(struct Window *)GetTagData(TT_Panels_Move_Window, (ULONG)NULL, msg->taglist),
				GetTagData(TT_Panels_Move_X, 0, msg->taglist),
				GetTagData(TT_Panels_Move_Y, 0, msg->taglist)
			));

		case TA_Panels_Zip:
			return (tr_panels_zip(msg->obj,
				(struct Window *)GetTagData(TT_Panels_Zip_Window, (ULONG)NULL, msg->taglist),
				GetTagData(TT_Panels_Zip_XS, 0, msg->taglist),
				GetTagData(TT_Panels_Zip_YS, 0, msg->taglist),
				GetTagData(TT_Panels_Zip_Reversed, FALSE, msg->taglist),
				GetTagData(TT_Panels_Zip_ZipSpeed, MV_Panel_ZipSpeed_Medium, msg->taglist)
			));

		case TA_Appicon_Read:
			{
				ULONG rc;

				struct ipcmessage *imsg = (struct ipcmessage *)GetTagData(TT_Appicon_Read_Message, (ULONG)NULL, msg->taglist);
				ASSERT(imsg);

				rc = tr_appicon_read(msg->obj, (APTR)GetTagData(TT_Appicon_Read_Object, (ULONG)NULL, msg->taglist), (struct ipc_appicon *)imsg->msgtype);
				return (rc);
			}

		case TA_Infowin_Open:
			return (tr_openinfowin(msg->obj,
				(CONST CONST_STRPTR *)GetTagData(TT_Infowin_Open_PathList, (ULONG)NULL, msg->taglist),
				GetTagData(TT_Infowin_Open_Wait, FALSE, msg->taglist),
				GetTagData(TT_Infowin_Open_RexxID, (ULONG)NULL, msg->taglist)
			));

		#if USE_SHORTCUTS
		case TA_Shortcuts_Add:
			{
				struct MinList *l = (struct MinList *)GetTagData(TT_Shortcuts_Add_List, (ULONG)NULL, msg->taglist);

				if (l)
				{
					/* NOTE: tr_shotrcuts_list frees the nodes and the list! */
					return (tr_shortcuts_list_add(msg->obj,
						l
					));
				}
				else
				{
					return (tr_shortcuts_add(msg->obj,
						(STRPTR)GetTagData(TT_Shortcuts_Add_Path, (ULONG)NULL, msg->taglist),
						(LONG)GetTagData(TT_Shortcuts_Add_X, NO_ICON_POSITION, msg->taglist),
						(LONG)GetTagData(TT_Shortcuts_Add_Y, NO_ICON_POSITION, msg->taglist),
						TRUE,
						(LONG)GetTagData(TT_Shortcuts_Add_Type, MV_Icon_FileType_None, msg->taglist)
					));
				}
			}
			break;

		case TA_Shortcuts_SaveAll:
			return (tr_shortcuts_save(msg->obj,
				TRUE
			));
		#endif

		case TA_URI_Load:
			return (tr_loaduri(
				(APTR)GetTagData(TT_URI_Load_Ctx, (ULONG)NULL, msg->taglist),
				(STRPTR)GetTagData(TT_URI_Load_URI, (ULONG)NULL, msg->taglist),
				GetTagData(TT_URI_Load_ID, 0, msg->taglist)
			));

		case TA_File_Rename:
			{
				ULONG reqflags = 0;
				return (tr_rename(
					(STRPTR)GetTagData(TT_File_Rename_Path, (ULONG)NULL, msg->taglist),
					(CONST_STRPTR *)GetTagData(TT_File_Rename_PathList, (ULONG)NULL, msg->taglist),
					(STRPTR)GetTagData(TT_File_Rename_Name, (ULONG)NULL, msg->taglist),
					GetTagData(TT_File_Rename_NoIcon, (ULONG) FALSE, msg->taglist),
					GetTagData(TT_File_Notify, TRUE, msg->taglist),
					(ULONG *)&reqflags
				));
			}

		case TA_Sound_Play:
			return (tr_playsound(
				(STRPTR)GetTagData(TT_Sound_Play_Path, (ULONG)NULL, msg->taglist),
				GetTagData(TT_Sound_Play_Mode, PS_VORBIS | PSF_QUEUED_IMMEDIATE, msg->taglist)
			));

		case TA_Imageview_Load:
			return (tr_loadimage(msg->obj,
				(STRPTR)GetTagData(TT_Imageview_Load_Path, (ULONG)NULL, msg->taglist)
			));

		case TA_Imageview_Scale:
			return (tr_scaleimage(msg->obj,
				(APTR)GetTagData(TT_Imageview_Scale_Dtp, (ULONG)NULL, msg->taglist),
				GetTagData(TT_Imageview_Scale_XS, (ULONG)NULL, msg->taglist),
				GetTagData(TT_Imageview_Scale_YS, (ULONG)NULL, msg->taglist)
			));

		case TA_Icon_Read:
			return (tr_icon_read(msg->obj,
				(STRPTR)GetTagData(TT_Icon_Read_Path, (ULONG)NULL, msg->taglist),
				GetTagData(TT_Icon_Read_NoInfo, FALSE, msg->taglist)
			));

		case TA_Imageview_ScanPics:
			return (tr_scanforpics(msg->obj,
				(STRPTR)GetTagData(TT_Imageview_ScanPics_Path, (ULONG)NULL, msg->taglist)
			));

		case TA_Textview_Load:
			return (tr_loadtext(msg->obj,
				(STRPTR)GetTagData(TT_Textview_Load_Path, (ULONG)NULL, msg->taglist)
			));

		case TA_Boopsiview_Load:
			return (tr_loaddatatype(msg->obj,
				(STRPTR)GetTagData(TT_Boopsiview_Load_Path, (ULONG)NULL, msg->taglist)
			));

		case TA_File_Makedir:
			return (tr_makedir(
				(CONST_STRPTR)GetTagData(TT_File_BaseDir, (ULONG)NULL, msg->taglist),
				(CONST_STRPTR)GetTagData(TT_File_Makedir_Path, (ULONG)NULL, msg->taglist),
				GetTagData(TT_File_Makedir_Icon, FALSE, msg->taglist)
			));

		case TA_File_Makelink:
			return (tr_makelink(
				(STRPTR)GetTagData(TT_File_Makelink_Object, (ULONG)NULL, msg->taglist),
				(STRPTR)GetTagData(TT_File_Makelink_Link, (ULONG)NULL, msg->taglist)
			));

		case TA_Font_Load:
			DB(("THREAD:LOAD_FONT:%s:%d\n",(STRPTR)GetTagData(TT_Font_Load_Path, (ULONG)NULL, msg->taglist),GetTagData(TT_Font_Load_Type, TV_Font_Load_Type_Root, msg->taglist)));
			return (tr_font_load(msg->obj,
				(STRPTR)GetTagData(TT_Font_Load_Path, (ULONG)NULL, msg->taglist),
				GetTagData(TT_Font_Load_Type, TV_Font_Load_Type_Root, msg->taglist)
			));

		case TA_WBStart:
			return (tr_wbstart(
				(STRPTR)GetTagData(TT_WBStart_Path, 0, msg->taglist),
				(LONG)GetTagData(TT_WBStart_Priority, 0, msg->taglist),
				GetTagData(TT_WBStart_Stack, 0, msg->taglist),
				(STRPTR)GetTagData(TT_WBStart_Argument, 0, msg->taglist),
				(struct MinList *)GetTagData(TT_WBStart_Argument_List, (ULONG)NULL, msg->taglist),
				GetTagData(TT_WBStart_FreeNames, FALSE, msg->taglist)
			));

		case TA_Infoicon_Load:
			return (tr_infoicon_load(msg->obj,
				(STRPTR)GetTagData(TT_Infoicon_Load_Path, 0, msg->taglist),
				        GetTagData(TT_Infoicon_Load_DefIcon, FALSE, msg->taglist)
			));

		case TA_Thumbnail_Create:
			{
				struct MinList *l = (struct MinList *)GetTagData(TT_Thumbnail_Create_List, (ULONG)NULL, msg->taglist);

				if (l)
				{
					return (tr_thumb_createicon_list(msg->obj,
						l,
						TCF_CREATE
					));
				}
				else
				{
					return (tr_thumb_createicon(msg->obj,
						(APTR)GetTagData(TT_Thumbnail_Create_Object, (ULONG)NULL, msg->taglist),
						(STRPTR)GetTagData(TT_Thumbnail_Create_Path, (ULONG)NULL, msg->taglist),
						TCF_CREATE
					));
				}
			}

		case TA_Metadata_Gather:
			return (tr_metadata_gather(msg->obj,
				(STRPTR)GetTagData(TT_Metadata_Gather_Path, (ULONG)NULL, msg->taglist),
				(STRPTR)GetTagData(TT_Metadata_Gather_MimeTypeDescription, (ULONG)NULL, msg->taglist)
			));

		case TA_ActionDispatcher_Execute:
			return (tr_actiondispatcher_execute(msg->obj
			));

		case TA_MimeType_Scan:
			{
				struct MinList *l = (struct MinList *)GetTagData(TT_MimeType_Scan_List, (ULONG)NULL, msg->taglist);
				ULONG matchfirst = GetTagData(TT_MimeType_Scan_MatchFirst, FALSE, msg->taglist);
				Object *mimeTypeClass = (Object *)GetTagData(TT_MimeType_Scan_MimetypeObject, FALSE, msg->taglist);
				ULONG rc;

				if ( matchfirst )
				{
					rc = tr_typescanner_scan_matchfirst(msg->obj, l, mimeTypeClass);
				}
				else
				{
					if ( l )
					{
						ULONG gen_thumbs = GetTagData(TT_MimeType_Scan_GenerateThumbs, FALSE, msg->taglist);
						ULONG gen_deficons = GetTagData(TT_MimeType_Scan_GenerateDefIcons, FALSE, msg->taglist);
						rc = tr_typescanner_scan_list(msg->obj, l, gen_deficons, gen_thumbs);
					}
					else
					{
						rc = tr_typescanner_scan_entry(msg->obj,
							(APTR)GetTagData(TT_MimeType_Scan_Path, (ULONG)NULL, msg->taglist),
							mimeTypeClass);
					}
				}
				
				DoMethod(mimeTypeClass, OM_RELEASE);
				return rc;
			}

		case TA_Clipboard_Paste:
			return (tr_clipboard_paste(
				(STRPTR) GetTagData(TT_Clipboard_Paste_DestPath , 0, msg->taglist),
				(LONG) GetTagData(TT_Clipboard_Paste_ViewID, 0, msg->taglist),
				(LONG) GetTagData(TT_Clipboard_Paste_As, FALSE, msg->taglist)
			));

		case TA_Listview_CreateIcons:
			return (tr_listview_createicons(msg->obj,
				(APTR) GetTagData(TT_Listview_Iconlist, (ULONG)NULL, msg->taglist),
				(APTR) GetTagData(TT_Listview_Data, (ULONG)NULL, msg->taglist)
			));

		case TA_Listview_ShowDevices:
			return (tr_listview_showdevices(msg->obj,
			    (APTR) GetTagData(TT_Listview_Data, (ULONG)NULL, msg->taglist)
			));

		case TA_Find:
			return (tr_find(msg->obj,
				(APTR) GetTagData(TT_Find_Window, (ULONG)NULL, msg->taglist)
			));

		case TA_DesktopPrefs_Save:
			return (tr_dprefs_save(msg->obj));

		case TA_Version_Find:
			return (tr_findver_list(msg->obj,
				(struct MinList *)GetTagData(TT_Version_Find_List, (ULONG)NULL, msg->taglist)
			));

		case TA_File_GetSizes:
			return (tr_getdirsizes(msg->obj,
				(struct MinList *) GetTagData(TT_File_GetSizes_List, (ULONG)NULL, msg->taglist)
			));

		case TA_Dispose_Objects:
			return (tr_dispose_objects(msg->obj,
				(APTR*)GetTagData(TT_Dispose_Objects_Array, 0, msg->taglist),
				(ULONG)GetTagData(TT_Dispose_Objects_FreeArray, TRUE, msg->taglist)
			));

		case TA_Listview_ShowPreview:
			return (tr_listview_showpreview(msg->obj,
				(APTR) GetTagData(TT_Listview_Entry, 0, msg->taglist),
				(APTR) GetTagData(TT_Listview_Data, 0, msg->taglist)
			));

		case TA_Iconview_ShowPreview:
			return (tr_iconview_showpreview(msg->obj,
				(APTR) GetTagData(TT_Iconview_ShowPreview_Object, 0, msg->taglist)
			));

		case TA_SmartReq_ExamineFiles:
			return (tr_smartreq_examine(msg->obj,
				(APTR) GetTagData(TT_SmartReq_Data, 0, msg->taglist),
				(APTR) GetTagData(TT_SmartReq_File1, 0, msg->taglist),
				(APTR) GetTagData(TT_SmartReq_File2, 0, msg->taglist)
			));

		case TA_StartupPrefs_Load:
			return (tr_sprefs_load(msg->obj));

		case TA_Print:
			return (tr_print(msg->obj,
				(APTR)GetTagData(TT_Print_Source, 0, msg->taglist),
				GetTagData(TT_Print_Type, PRT_NONE, msg->taglist)
			));

		default:
			D(PROC, bug("command %ld not supported yet\n", msg->action));
			return (TRUE);
	}
}
