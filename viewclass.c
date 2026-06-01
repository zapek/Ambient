/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2016 Ambient Open Source Team
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
 * $Id: viewclass.c,v 1.40 2025/08/16 14:04:55 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <devices/rawkeycodes.h>
#include <workbench/workbench.h>

/* private */
#include "ambient_cat.h"
#include "clipboard.h"
#include "keymap.h"
#include "keyshortcuts.h"
#include "menus.h"
#include "mimeuri.h"
#include "mui_func.h"
#include "name.h"
#include "prefs_desktop.h"
#include "view_func.h"
#include "threads.h"
#include "contextmenu.h"
#include "command.h"
#include "rexx.h"
#include "viewapi.h"
#include "viewclass.h"
#include "screen.h"
#include "trashcan.h"
#include "networksfs.h"

#include "listviewclass.h" // bitRocky

struct Data {
	/* menu stuff */
	APTR cmenu;
	APTR submenustrip;
	ULONG ownmenuclass;
	ULONG contextmenumode;

	/* Common keyboard handling code */
	struct MUI_EventHandlerNode ehnode;

	/* select count */
	ULONG numselected;

	/* menuitems */
	APTR edit_copy;
	APTR edit_cut;
	APTR edit_paste;
//	APTR edit_paste_into;
	APTR view_newdrawer;
	APTR view_connect;
	APTR view_sort;
	APTR icons_information;
	APTR icons_putaway;
	APTR icons_eject;
	APTR icons_rename;
	APTR icons_delete;
	APTR icons_format;
	APTR icons_trash;
	APTR icons_empty;
	APTR icons_restore;

	STRPTR str_edit_paste;
	STRPTR str_edit_paste_into;

	/* selected icons */
	ULONG files;
	ULONG drawers;
	ULONG volumes;
	ULONG appicons;
	ULONG mycomputer;
	ULONG shortcuts;

	ULONG pastemode;

	/* id */

	ULONG viewid;

	/* counts */
	ULONG totalfiles;   /* total number of files in directory */
	ULONG totaldirs;    /* total number of dirs in directory */
	ULONG links;
	ULONG iconfiles;
};

enum /* for pastemode */
{
	PASTE_NOTHING,
	PASTE,
	PASTE_INTO
};

DEFNEW
{
	obj = DoSuperNew(cl, obj,
		MUIA_InnerBottom, 0,
		MUIA_InnerLeft, 0,
		MUIA_InnerRight, 0,
		MUIA_InnerTop, 0,
		MUIA_ContextMenu, 1,
		TAG_MORE, INITTAGS,
	End;

	if (obj)
	{
		GETDATA;

		data->viewid = GetTagData(MA_Viewgroup_ID, MV_ViewID_Unknown, INITTAGS);

		data->ehnode.ehn_Object = obj;
		data->ehnode.ehn_Class = cl;
		data->ehnode.ehn_Events = IDCMP_RAWKEY | IDCMP_MOUSEBUTTONS;
		data->ehnode.ehn_Priority = 3;
		data->ehnode.ehn_Flags = MUI_EHF_GUIMODE;

		data->str_edit_paste = GSI(MSG_MENU_EDIT_PASTE);
		data->str_edit_paste_into = GSI(MSG_MENU_EDIT_PASTEINTO);
	}

	return ((ULONG)obj);
}


DEFDISP
{
	GETDATA;

	if (data->cmenu)
	{
		MUI_DisposeObject(data->cmenu);
	}

	return (DOSUPER);
}


DEFMMETHOD(Hide)
{
	GETDATA;
	DoMethod(_win(obj), MUIM_Window_RemEventHandler, (ULONG)&data->ehnode);
	return (DOSUPER);
}


DEFMMETHOD(Show)
{
	GETDATA;
	ULONG rc;

	rc = DOSUPER;

	if (rc)
	{
		APTR menu;

		if (!data->view_newdrawer && (menu = (APTR)getv(_win(obj), MUIA_Window_Menustrip)))
		{
			data->edit_copy = FINDMENU(MENU_EDIT_COPY);
			data->edit_cut = FINDMENU(MENU_EDIT_CUT);
			data->edit_paste = FINDMENU(MENU_EDIT_PASTE);
//			data->edit_paste_into = FINDMENU(MENU_EDIT_PASTEINTO);
			data->view_newdrawer = FINDMENU(MENU_VIEW_NEWDRAWER);
			data->view_connect = FINDMENU(MENU_VIEW_NETWORKSCONNECT);
			data->view_sort = FINDMENU(MENU_VIEW_SORT);
			data->icons_information = FINDMENU(MENU_ICONS_INFORMATION);
			data->icons_putaway = FINDMENU(MENU_ICONS_PUTAWAY);
			data->icons_eject = FINDMENU(MENU_ICONS_EJECT);
			data->icons_rename = FINDMENU(MENU_ICONS_RENAME);
			data->icons_delete = FINDMENU(MENU_ICONS_DELETE);
			data->icons_format = FINDMENU(MENU_ICONS_FORMAT);
			data->icons_trash = FINDMENU(MENU_ICONS_TRASH);
			data->icons_restore = FINDMENU(MENU_ICONS_RESTORE);
			data->icons_empty = FINDMENU(MENU_ICONS_EMPTY);

			/* Certain menu items are always disabled for root */
			if (getv(obj, MA_View_IsRoot))
			{
				set(data->edit_paste, MUIA_Menuitem_Title, data->str_edit_paste_into);

				DoMethod(obj, MUIM_MultiSet, MUIA_Menuitem_Enabled, FALSE,
					data->edit_copy,
					data->edit_cut,
					NULL);
			}
		}

		DoMethod(_win(obj), MUIM_Window_AddEventHandler, (ULONG)&data->ehnode);
	}

	return (rc);
}


/*
 * OM_GET forwarders (to the viewgroup upwards)
 */
DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_View_TotalFiles:
			*msg->opg_Storage = data->totalfiles;
			return (TRUE);

		case MA_View_TotalDirs:
			*msg->opg_Storage = data->totaldirs;
			return (TRUE);

		case MA_View_Links:
			*msg->opg_Storage = data->links;
			return (TRUE);

		case MA_View_IconFiles:
			*msg->opg_Storage = data->iconfiles;
			return (TRUE);

		case MA_View_SelectionMask:
		{
			ULONG mask = 0;

			if (data->files)
				mask |= FVS_FILES;
			if (data->drawers)
				mask |= FVS_DRAWERS;
			if (data->volumes)
				mask |= FVS_VOLUMES;
			if (data->appicons)
				mask |= FVS_APPICONS;
			if (data->mycomputer)
				mask |= FVS_SYSTEM;
			if (data->shortcuts)
				mask |= FVS_SHORTCUTS;

			*msg->opg_Storage = mask;
			return (TRUE);
		}

		case MA_View_NumSelected:
		{
			GETDATA;
			*msg->opg_Storage = data->numselected;
			return (TRUE);
		}

		case MA_Viewgroup_ID:
		{
			GETDATA;
			*msg->opg_Storage = data->viewid;
			return (TRUE);
		}

		FORWARD_GET(MA_View_IsRoot);
		FORWARD_GET(MA_View_Path);
		FORWARD_GET(MA_View_ShowDevices);
		FORWARD_GET(MA_View_MIME);
		FORWARD_GET(MA_View_BgPen);
		FORWARD_GET(MUIA_Scrollgroup_HorizBar);
		FORWARD_GET(MUIA_Scrollgroup_VertBar);
	}
	return (DOSUPER);
}

/*
 * OM_SET forwarders (to the viewgroup upwards).
 * Don't forget to TAG_IGNORE in viewgroupclass!
 */
DEFSET
{
	GETDATA;

	FORTAG(INITTAGS)
	{
		case MA_View_TotalFiles:
			data->totalfiles = tag->ti_Data;
			break;

		case MA_View_TotalDirs:
			data->totaldirs = tag->ti_Data;
			break;

		case MA_View_Links:
			data->links = tag->ti_Data;
			break;

		case MA_View_IconFiles:
			data->iconfiles = tag->ti_Data;
			break;

		case MA_View_NumSelected:
		{
			GETDATA;

			switch (tag->ti_Data)
			{
				case MV_View_NumSelected_Increase:
					data->numselected++;
					break;

				case MV_View_NumSelected_Decrease:
					data->numselected--;
					break;

				default:
					data->numselected = tag->ti_Data;
					break;
			}
			break;
		}

		FORWARD_SET(MA_View_Path);
	}
	NEXTTAG

	return (DOSUPER);
}


/*
 * Method forwarders (to the viewgroup upwards)
 */
FORWARD_METHOD(View_ReadArgs)
FORWARD_METHOD(View_SetWindowPosition)
FORWARD_METHOD(View_SetStatus)
FORWARD_METHOD(View_ContextMenuMerge)
FORWARD_METHOD(View_InvalidateMimeType)


DEFSMETHOD(View_IconSelect)
{
	GETDATA;

	if (msg->selection == MV_View_IconSelect_Clear)
	{
		data->volumes = 0;
		data->files = 0;
		data->drawers = 0;
		data->appicons = 0;
		data->mycomputer = 0;
		data->shortcuts = 0;
	}
	else
	{
		LONG add = msg->selection == MV_View_IconSelect_Select ? 1 : -1;

		if (msg->shortcut)
		{
			data->shortcuts += add;
			if (msg->type == MV_Icon_Type_Disk || msg->type == MV_Icon_Type_Device) data->volumes += add;
		}
		else
		switch (msg->type)
		{
			case MV_Icon_Type_Disk  :
			case MV_Icon_Type_Device:
				data->volumes += add;
				break;

			case MV_Icon_Type_Tool   :
			case MV_Icon_Type_Project:
				data->files += add;
				break;

			case MV_Icon_Type_Drawer:
				data->drawers += add;
				break;

			case MV_Icon_Type_AppIcon:
				data->appicons += add;
				break;

			case MV_Icon_Type_MyComputer:
				data->mycomputer += add;	/* err, yeah */
				break;
		}
DB(("after: msg->type = %ld, data->volumes = %ld, add = %ld\n", msg->type, data->volumes, add));
	}

	return (0);
}



DEFTMETHOD(View_Abort)
{
	threads_abort(obj, NULL); /* XXX: hm.. what if there's more than one thread ? */

	return (0);
}

DEFMMETHOD(ContextMenuBuild)
{
	if (!getv(_view(obj), MA_View_IsRoot) && !getv(_view(obj), MA_View_IsRootExtra))
	{
		GETDATA;
		ULONG flags;
		ULONG type;
		ULONG overridetitle = FALSE;
		APTR  ownmenu;
		APTR vo = _view(obj);
		struct viewnode *vn = viewapi_findbyid(getv(vo, MA_Viewgroup_ViewIndex));

		if (data->cmenu != NULL)
		{
			MUI_DisposeObject(data->cmenu);
		}

		flags = 0;
		flags |= (viewapi_getflags(vn) & VF_DEVICEVIEW) ? AS_DEVICEVIEW : 0;
		flags |= (viewapi_getflags(vn) & VF_FILEVIEW) ? (AS_HASICONS | AS_FILEVIEW | AS_ICONVIEW): 0; /* Select All/Invert should be available only if directory is non-empty... blah */

		/* Menu type. This should be done in nicer way... */

		switch(getv(obj, MA_View_Type))
		{
			case MV_View_Type_Icon:
				type = CM_ICONVIEW;
				break;
			case MV_View_Type_List:
				type = CM_LISTVIEW;
				break;
			case MV_View_Type_Image:
				type = CM_LISTVIEW; /* XXX:Add image view to locale! */
				overridetitle = TRUE;
				break;
			case MV_View_Type_Text:
				type = CM_LISTVIEW; /* XXX:Add text view to locale */
				overridetitle = TRUE;
				break;
			default:
				type = CM_LISTVIEW; /* This one defaults to 'file' label */
				overridetitle = TRUE;
				break;
		}

		if (type == CM_LISTVIEW)
		{
			flags &= ~AS_ICONVIEW;
			flags |= AS_LISTVIEW;
		}
		
		flags |= icon_flags_for_path((STRPTR) getv(obj, MA_View_Path), type);

		if ((ownmenu = contextmenu_build(type, flags)))
		{
			/* XXX: Why this check for iconview?? */
			if ((data->cmenu = contextmenu_build(CM_VIEW, getv(obj, MA_View_Type) == MV_View_Type_Icon ? AS_ICONVIEW : AS_LISTVIEW)))
			{
				ULONG index = index; /* shut up gcc */
				APTR menu = menu; /* shut up gcc */
			
				/* menustrip -> menu (sigh) */
				FORCHILD(data->cmenu, MUIA_Family_List)
				{
					menu = child;
					break;
				}
				NEXTCHILD;

				ASSERT(menu);

				if (!get(obj, MA_View_ModeIndex, &index))
				{
					index = 0;
				}
				contextmenu_addviews(obj, menu);
				contextmenu_addmodes(obj, menu, index); /* XXX (and retcode too) */

				DoMethod(obj, MM_View_ContextMenuMerge, ownmenu, data->cmenu);
			}

			MUI_DisposeObject(ownmenu);

			/* if we can't get proper menu then at least try to give it proper title */

			if (overridetitle)
			{
				set(data->cmenu, MUIA_Menu_Title, vn->label);
			}

			return ((ULONG)data->cmenu);
		}
		errormsg(ERR_NOMEM);
	}
	return (MUIV_ContextMenuBuild_Default);
}


DEFMMETHOD(ContextMenuChoice)
{
	struct command_menu *cm;

	if ((cm = (struct command_menu *)getv(msg->item, MA_Menuitem_Command)))
	{
		if (cm->name && *cm->name)
		{
			ULONG type;

			if (get(msg->item, MA_Menuitem_SubType, &type) && type)
			{
				struct viewnode *vn;
				TEXT viewcmd[128]; /* should be enough for everyone (tm) */

				if ((vn = viewapi_findbyclass(OCLASS(obj))))
				{
					switch (type)
					{
						case MV_Menuitem_SubType_Mode:
							snprintf(viewcmd, sizeof(viewcmd), "Viewmode mode=\"%s %s\"", vn->name, cm->args);
							break;

						case MV_Menuitem_SubType_View:
							snprintf(viewcmd, sizeof(viewcmd), "Viewmode mode=\"%s\"", cm->args);
							break;

						#ifdef DEBUG
						default:
							PDB(("missing type\n"));
							break;
						#endif
					}
					execute_command(obj, cm->type, viewcmd, NULL);
				}
				/* XXX */
			}
			else
			{
				DB(("Command:%s\n", cm->args));
				execute_command(obj, cm->type, cm->args, NULL);
			}
		}
	}
	return (TRUE);
}


DEFSMETHOD(Thread_Finished)
{
	if (msg->status == MV_Thread_Finished_Abort)
	{
		/* we need to push as the object might go away as a result of this */
		DoMethod(app, MUIM_Application_PushMethod, _view(obj), 1, MM_Viewgroup_Aborted); /* XXX: will that crash if I exit ambient at the same time ? don't think so */
	}

	return (0);
}


DEFSMETHOD(View_ParseWindowArgs)
{
	struct v_args args;
	ULONG rc;

	memset(&args, 0, sizeof(struct v_args));

	if ((rc = CoerceMethod(cl, obj, MM_View_ReadArgs, VIEW_TEMPLATE, &args)))
	{
		struct Screen *scr;
		LONG left, top, width, height, mode;

		left = _conf(window_default_left);
		top  = _conf(window_default_top);

		if (args.left || args.top)
		{
			left = args.left ? *args.left : 0;
			top = args.top ? *args.top : 0;
		}

		if ((scr = get_screen() ))
		{
			if ( left == -1 )
				left = scr->MouseX;

			if ( top == -1 )
				top = scr->MouseY;
		}

		width = args.width ? *args.width : _conf(window_default_width);
		height = args.height ? *args.height : _conf(window_default_height);

		mode = LVM_ICONS;
		*msg->modeset = FALSE;

		if (args.mode)
		{
			if(!stricmp(args.mode, "ICONS"))
			{
				mode = LVM_ICONS;
				*msg->modeset = TRUE;
			}
			else if(!stricmp(args.mode, "THUMBS"))
			{
				mode = LVM_THUMBS;
				*msg->modeset = TRUE;
			}
			else if(!stricmp(args.mode, "ALL"))
			{
				mode = LVM_SHOWALL;
				*msg->modeset = TRUE;
			}
		}

		*msg->mode = mode;
		*msg->type = args.type ? args.type : (STRPTR) "";
		*msg->width = width;
		*msg->height = height;
	
		// bitRocky: new sort options in URIs
		DB(("args.sortby = %s, args.sortorder = %s\n", args.sortby ? args.sortby : "NULL", args.sortorder ? args.sortorder : "NULL"));
		if (msg->sortset) *msg->sortset = FALSE;

		if (args.sortby && msg->sortby)
		{
			typedef struct { STRPTR name; ULONG num; } ColNode;
			// bitRocky: for file://
			static CONST ColNode fcols[] = {
				{ "NAME", LISTVIEW_FILE_COL_NAME },
				{ "SIZE", LISTVIEW_FILE_COL_SIZE },
				{ "DATE", LISTVIEW_FILE_COL_DATE },
				{ "ATTRS", LISTVIEW_FILE_COL_ATTRS },
				{ "COMMENT", LISTVIEW_FILE_COL_COMMENT },
				{ "ICON", LISTVIEW_FILE_COL_ICON },
				{ "FILETYPE", LISTVIEW_FILE_COL_FILETYPE },
				{ "VERSION", LISTVIEW_FILE_COL_VERSION },
				{ "MD5", LISTVIEW_FILE_COL_MD5 },
				{ "FULLDATE", LISTVIEW_FILE_COL_FULLDATE },
				{ "UID", LISTVIEW_FILE_COL_UID },
				{ "GID", LISTVIEW_FILE_COL_GID },
				{NULL, 0}
			};

			// bitRocky: for devices://
			static CONST ColNode dcols[] = {
				{ "VOLUME", LISTVIEW_DEVICE_COL_VOLUME-LVFORMAT_DEVICES_BASE},
				{ "DEVICE", LISTVIEW_DEVICE_COL_DEVICE-LVFORMAT_DEVICES_BASE},
				{ "FREE", LISTVIEW_DEVICE_COL_FREE-LVFORMAT_DEVICES_BASE},
				{ "TOTAL", LISTVIEW_DEVICE_COL_TOTAL-LVFORMAT_DEVICES_BASE},
				{ "DOSTYPE", LISTVIEW_DEVICE_COL_DOSTYPE-LVFORMAT_DEVICES_BASE},
				{NULL, 0}
			};
			// bitRocky: for iconview
			static CONST ColNode icols[] = {
				{ "NAME", IVS_NAME},
				{ "TYPE", IVS_TYPE},
				{ "SIZE", IVS_SIZE},
				{ "DATE", IVS_DATE},
				{NULL, 0}
			};

			ULONG i=0;
			ColNode *cols;

#warning "These strnicmp/Strnicmp calls are suspect. I mean is it really meant to work like this?"
			cols = (args.view && (strnicmp(args.view, "DLIST", strlen(args.view))==0)) ? (ColNode*)&dcols :
				 ( (args.view && (strnicmp(args.view, "LIST", strlen(args.view))==0)) ? (ColNode*)&fcols : (ColNode*)&icols );
			
			while (cols[i].name && (Strnicmp(cols[i].name, args.sortby, strlen(args.sortby)) != 0)) i++;
			
			DB(("msg->sortby = %ld (%s)\n", cols[i].num, cols[i].name)); 
			*msg->sortby = cols[i].num;
			*msg->sortset = TRUE;
		}
		if (args.sortorder && msg->sortorder)
		{
			*msg->sortorder = (Strnicmp(args.sortorder, "desc", strlen(args.sortorder))==0) ? -1 : 1;
			*msg->sortset = TRUE;
		}
		else if (msg->sortorder)
		{
			*msg->sortorder = 1;
		}

		CoerceMethod(cl, obj, MM_View_SetWindowPosition, left, top);
	}

	return (rc);
}


DEFTMETHOD(View_PickSelected)
{
	ULONG objcount;
	APTR array;

	/*
	 * Perform the action on each selected
	 * icon. XXX: hm.. might be a bit sucky.. we definitely need some actionlists here. Well, ATM it asks for everyfile which is not too bad :)
	 */

	objcount = 16;
	array = AllocVecTaskPooled((objcount + 1) * sizeof(APTR));

	if (array)
	{
		ULONG count = objcount, *ptr = array;

		FORCHILD(obj, MUIA_Group_ChildList)
		{
			if (getv(child, MA_Icon_Selected))
			{
				if (count == 0)
				{
					ULONG *n;

					count = objcount;
					objcount *= 2;

					n = AllocVecTaskPooled((objcount + 1) * sizeof(APTR));
	
					if (!n)
						break;

					ptr = &n[count];

					CopyMemQuick(array, n, count * sizeof(APTR));
					FreeVecTaskPooled(array);

					array = n;
				}

				*ptr++ = (ULONG)child;
				count--;
			}
		}
		NEXTCHILD

		*ptr = (size_t)NULL;
	}

	return ((ULONG)array);
}

STATIC LONG get_paste_mode(struct Data *data)
{
	LONG mode = PASTE_NOTHING;

	if (data->appicons == 0 && data->mycomputer == 0 && clipboard_menu_paste_enable())
	{
		mode = PASTE;

		if ((data->drawers + data->volumes == 1) && data->files == 0)
			mode = PASTE_INTO;
	}

	return mode;
}

DEFSMETHOD(View_ExecuteAction)
{
	GETDATA;
	ULONG isroot = getv(obj, MA_View_IsRoot);
	ULONG show_devices = getv(obj, MA_View_ShowDevices);

	switch (msg->ExecuteAction)
	{
		case MV_View_ExecuteAction_Copy:
			if (!show_devices && (!isroot || data->shortcuts))
			{
				execute_command_objarray(obj, AC_INTERNAL, "ClipboardCopy");
			}
			break;

		case MV_View_ExecuteAction_Cut:
			if (!show_devices && !isroot)
			{
				execute_command_objarray(obj, AC_INTERNAL, "ClipboardCut");
			}
			break;

		case MV_View_ExecuteAction_Paste:
			{
				LONG mode = get_paste_mode(data);

				if (mode != PASTE_NOTHING)
				{
					APTR o = obj;

					if (mode == PASTE_INTO)
					{
						o = (APTR)getv(obj, MA_View_SubObject);
					}
					else
					{
						if (isroot || show_devices)
							break;
					}

					execute_command(o, AC_INTERNAL, "ClipboardPaste", NULL);
				}
			}
			break;
	}

	return 0;
}

DEFMMETHOD(HandleEvent)
{
	struct IntuiMessage *imsg;
	ULONG rc;

	imsg = msg->imsg;
	rc = 0;

	if (msg->muikey > MUIKEY_NONE)
	{
		switch (msg->muikey)
		{
			case MUIKEY_COPY : DoMethod(obj, MM_View_ExecuteAction, MV_View_ExecuteAction_Copy ); return (MUI_EventHandlerRC_Eat);
			case MUIKEY_CUT  : DoMethod(obj, MM_View_ExecuteAction, MV_View_ExecuteAction_Cut  ); return (MUI_EventHandlerRC_Eat);
			case MUIKEY_PASTE: DoMethod(obj, MM_View_ExecuteAction, MV_View_ExecuteAction_Paste); return (MUI_EventHandlerRC_Eat);
		}
	}

	if (imsg)
	{
		GETDATA;

		switch (imsg->Class)
		{
			case IDCMP_RAWKEY:

				/* first we try internal/user shortcuts list, and then contextmenu shortcuts,
				 * or should it be the other way around ? Not sure it's smart having 2 shortcuts systems
				 */

				if(keyshortcut_handle(obj, imsg))
				{
					rc = MUI_EventHandlerRC_Eat;
				}
				else
				{
					ULONG qual = imsg->Qualifier;

					if (qual & (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND))
					{
						TEXT c;

						imsg->Qualifier = IEQUALIFIER_LSHIFT;
						c = keymap_vanilla(imsg);
						imsg->Qualifier = qual;

						if (c)
							contextmenu_execute_shortcut(obj, toupper(c), 0xffffffff);
					}
				}
				break;

			case IDCMP_MOUSEBUTTONS:
				if (imsg->Code == MENUDOWN)
				{
					BOOL isroot, selected, isfileview, /*paste_enabled,*/ istrashcan, hastrashcan, isnetworks;
					LONG pastemode;
					APTR vo = _view(obj);
					/* XXX: Is this != NULL check really needed here? */
					struct viewnode *vn = vo != NULL ? viewapi_findbyid(getv(vo, MA_Viewgroup_ViewIndex)) : NULL;

					isroot = getv(obj, MA_View_IsRoot);
					selected = data->numselected;
					// isfileview = !getv(obj, MA_View_ShowDevices); // @itix: This is wrong and deadly, it doesn't work for other views such as textview & imageview
					isfileview = vn != NULL ? (viewapi_getflags(vn) & VF_FILEVIEW) : FALSE;
					//paste_enabled = clipboard_menu_paste_enable();
					isnetworks = is_networksfs((STRPTR)getv(obj, MA_View_Path));
					istrashcan = is_trashcan((STRPTR)getv(obj, MA_View_Path));
					hastrashcan = trashcan_is_running() && !isnetworks;

					#warning "@itix: Add some coherency checks that also work for image & text views"
					set(data->view_sort, MUIA_Menuitem_Enabled, !isroot && isfileview);
					set(data->icons_information, MUIA_Menuitem_Enabled, selected &&(data->files || data->drawers || data->volumes));
					set(data->icons_putaway, MUIA_Menuitem_Enabled, isroot && selected >= 1 && data->shortcuts);
					set(data->icons_eject, MUIA_Menuitem_Enabled, selected == 1 && data->volumes);
					set(data->icons_rename, MUIA_Menuitem_Enabled, selected && (data->files || data->drawers || data->volumes || data->shortcuts));
					set(data->icons_delete, MUIA_Menuitem_Enabled, selected && (isfileview || isroot) && (data->files || data->drawers) && !data->shortcuts);
					set(data->icons_format, MUIA_Menuitem_Enabled, selected == 1 && data->volumes);
					set(data->icons_trash, MUIA_Menuitem_Enabled, selected && hastrashcan && !istrashcan);
					set(data->icons_restore, MUIA_Menuitem_Enabled, selected && hastrashcan && istrashcan);
					set(data->icons_empty, MUIA_Menuitem_Enabled, istrashcan);					

					/*
					 * Only "Paste into..." is allowed for root and system view
					 */
					pastemode = get_paste_mode(data);

					set(data->edit_paste, MUIA_Menuitem_Enabled, ((pastemode == PASTE_INTO) || (isfileview && pastemode == PASTE)) ? TRUE : FALSE);

					if (!isroot)
					{
						set(data->edit_paste, MUIA_Menuitem_Title, (pastemode == PASTE_INTO || !isfileview) ? data->str_edit_paste_into : data->str_edit_paste);

						set(data->edit_cut, MUIA_Menuitem_Enabled, isfileview && selected);
						set(data->edit_copy, MUIA_Menuitem_Enabled, selected && (isfileview || data->shortcuts));

						set(data->view_newdrawer, MUIA_Menuitem_Enabled, isfileview && !selected);
					}

					set(data->view_connect, MUIA_Menuitem_Enabled, isroot || isnetworks);
					
					//set(data->view_listmode, MUIA_Menuitem_Checked, getv(obj, MA_View_Type) != MV_View_Type_Icon);
				}
				else if (keyshortcut_handle(obj, imsg))
				{
					rc = MUI_EventHandlerRC_Eat;
				}

				break;
		}
	}

	return (rc);
}


DEFSMETHOD(Rexx_Snapshot)
{
	STRPTR path;

	path = (STRPTR)getv(obj, MA_View_Path);

	if (msg->window)
	{
		LONG x, y, w, h;
		LONG flag;

		/* this fiels can contain other flags, but for now it's only for viewmode */
		flag = getv(obj, MA_View_ViewMode);

		x = _window(obj)->LeftEdge;
		y = _window(obj)->TopEdge;
		w = _window(obj)->Width;
		h = _mheight(obj) + (_window(obj)->Height - _mbottom(obj));

		if (*path == 0 && !strcmp((STRPTR)getv(obj, MA_View_MIME), MIMETYPE_INTERNAL_VOLUMES))
		{
			dprefs_mymorphos_window_set(x, y, w, h, flag);
			return do_action(obj, TA_DesktopPrefs_Save, TAG_DONE);
		}
		else
		{
			do_action(obj, TA_Window_Snapshot,
				TT_Window_Snapshot_Path, path,
				TT_Window_Snapshot_X, x,
				TT_Window_Snapshot_Y, y,
				TT_Window_Snapshot_XS, w,
				TT_Window_Snapshot_YS, h,
				TT_Window_Snapshot_Flags, flag,
			TAG_DONE);
		}
	}

	/* XXX: A little ugly, and reloads all icons... Should be reworked later. */
	/*
	if ((n = name_build_info(path)))
	{
		ULONG len = strlen(n);

		if (len >= 9)
		{
			STRPTR tmpn = n + len - 9;

			if (stricmp(tmpn, "disk.info") == 0)
				DoMethod(app, MM_Application_ReloadIcons, TRUE);
		}

		name_delete(n);
	}
	*/
	return (0);
}

DEFSMETHOD(Rexx_Unsnapshot)
{
	if (msg->window)
	{
		do_action(obj, TA_Window_Snapshot,
			TT_Window_Snapshot_Path, getv(obj, MA_View_Path),
			TT_Window_Snapshot_X, NO_ICON_POSITION,
			TT_Window_Snapshot_Y, NO_ICON_POSITION,
			TT_Window_Snapshot_XS, 0,
			TT_Window_Snapshot_YS, 0,
			TT_Window_Snapshot_Flags, 0,
		TAG_DONE);
	}

	return (0);
}

BEGINMTABLE
DECNEW
DECDISP
DECGET
DECSET
DECSMETHOD(Rexx_Snapshot)
DECSMETHOD(Rexx_Unsnapshot)
DECMMETHOD(ContextMenuBuild)
DECMMETHOD(ContextMenuChoice)
DECFORWARDER(View_ReadArgs)
DECFORWARDER(View_SetWindowPosition)
DECFORWARDER(View_SetStatus)
DECFORWARDER(View_ContextMenuMerge)
DECFORWARDER(View_InvalidateMimeType)
DECTMETHOD(View_Abort)
DECSMETHOD(View_ExecuteAction)
DECTMETHOD(View_PickSelected)
DECTMETHOD(View_ParseWindowArgs)
DECTMETHOD(View_IconSelect)
DECSMETHOD(Thread_Finished)
DECMMETHOD(Show)
DECMMETHOD(Hide)
DECMMETHOD(HandleEvent)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Virtgroup, viewclass)
DECSUBCLASS_NC(MUIC_Group, gviewclass)
