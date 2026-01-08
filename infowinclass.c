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
 * $Id: infowinclass.c,v 1.30.2.1 2023/05/20 20:42:27 piru Exp $
 */

#include "ambient.h"

/* public */
#include <utility/tagitem.h>
#include <libraries/asl.h>
#include <workbench/workbench.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <devices/trackdisk.h>
#include <mui/NumericString_mcc.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h> /* for get get() below.. */

/* private */
#include "appclass.h"
#include "infowin.h"
#include "mui_func.h"
#include "ambient_cat.h"
#include "threads.h"
#include "time_func.h"
#include "tooltypelist.h"
#include "dostype.h"
#include "iconio.h"
#include "name.h"
#include "screen.h"
#include "doslistcache.h"
#include "rexx.h"
#include "methodstack.h"
#include "file_func.h"
#include "device_func.h"
#include "capacity.h"
#include "examine64.h"
#include "info64.h"
#include "prefs_advanced.h"


#define TJ_GETSIZE   (1 << 0UL)
#define TJ_FINDVER   (1 << 1UL)
#define TJ_MD5SUM    (1 << 2UL)
#define TJ_ICONWRITE (1 << 3UL)


/* No idea where this comes from  -itix */
#define MAX_TT_LENGTH 256

struct infodata {
	struct devinfo64 inf;
	struct fileinfo64 fi;
	CONST_STRPTR type;
	ULONG hasdi64;
	ULONG hasfi64;
};


struct Data {
	ULONG closing;
	ULONG saved;
	APTR grp_content;

	TEXT wintitle[128];
	TEXT defaulttool[PATH_SIZE];
	STRPTR fname;

	struct infodata *infdata;

	APTR iconobj;
	APTR iconinfoobj;
	APTR txt_icontype;
	ULONG has_file; /* tells if there's a file/directory related with the icon */
	ULONG type;
	ULONG rxid;

	/* disk/device */
	APTR txt_disk;
	APTR txt_size;
	APTR txt_used;
	APTR txt_free;
	APTR txt_fs;
	APTR txt_type;
	APTR txt_blocksize;
	APTR txt_status;
	APTR txt_features;
	APTR str_defaulttool;

	/* drawer */
	APTR txt_files;
	APTR txt_dirs;
	APTR txt_hardlinks;
	APTR txt_softlinks;
	APTR txt_total;
	APTR bt_scan;

	/* project */
	APTR txt_blocks;
	APTR txt_bytes;
	APTR txt_bytesdisk;
	APTR txt_version;
	APTR txt_md5sum;
	APTR str_stack;
	APTR bt_version;
	APTR bt_md5sum;

	APTR chk_script;
	APTR chk_archived;
	APTR chk_readable;
	APTR chk_writable;
	APTR chk_executable;
	APTR chk_deletable;

	/* shared */
	APTR txt_date;
	APTR str_comment;
	APTR str_tooltype;
	APTR lv_tooltypes;
	APTR bt_new;
	APTR bt_del;
	APTR bt_save;
	APTR bt_cancel;
	APTR cyc_type;

	/* project/tool */
	APTR ogrp;
	APTR txt_defaulttool;
	APTR txt_date_label;
	APTR txt_comment;
	APTR txt_tooltypes;
	APTR grp_tooltypes;
	APTR space_tooltypes;

	/* filetype... */
#if USE_OLD_ACTIONEDITOR
	APTR edit_bt;
	APTR pagegrp;
#endif

	ULONG jobs;

	ULONG is_fast;
	BOOL  autogetver;
	BYTE  update_icon;
	BYTE  update_file;
};

MUI_HOOK(tt_dest, APTR pool UNUSED, STRPTR p)
{
	free(p);
	return (0);
}

static void adjust_layout(struct Data *data, APTR iconobj)
{
	/*
	 * We do some layout fixing depending on what kind of infowin it is...
	 */
	if (data->type == MV_Icon_Type_Disk || data->type == MV_Icon_Type_Device ||
	    data->type == MV_Icon_Type_Kick || (0 && getv(iconobj, MA_Icon_IsDefault)))
	{
		APTR o = RectangleObject, End;

		if (o)
			DoMethod(data->grp_content, OM_ADDMEMBER, o);
	}
}

static void set_icontype(struct Data *data)
{
	APTR iconobj;

	iconobj = (APTR)getv(data->iconinfoobj, MA_Infoicongroup_Child);
	CHECKOBJECT(iconobj);

	switch (getv(iconobj, MA_Icon_ImageType))
	{
		case MV_Icon_ImageType_Old:
		case MV_Icon_ImageType_Standard:
			set(data->txt_icontype, MUIA_Text_Contents, GSI(MSG_INFOWIN_OLD_AMIGA_ICON));
			break;

		case MV_Icon_ImageType_Newicon:
			set(data->txt_icontype, MUIA_Text_Contents, GSI(MSG_INFOWIN_NEWICON));
			break;

		case MV_Icon_ImageType_Glowicon:
			set(data->txt_icontype, MUIA_Text_Contents, GSI(MSG_INFOWIN_GLOWICON));
			break;

		case MV_Icon_ImageType_PNGicon:
			set(data->txt_icontype, MUIA_Text_Contents, GSI(MSG_INFOWIN_PNGICON));
			break;

		case MV_Icon_ImageType_DTicon:
			set(data->txt_icontype, MUIA_Text_Contents, GSI(MSG_INFOWIN_DATATYPEICON));
			break;
			
		case MV_Icon_ImageType_SVGicon:
			set(data->txt_icontype, MUIA_Text_Contents, GSI(MSG_INFOWIN_SVGICON));
			break;

		#ifdef DEBUG
		default:
			PDB(("unknown icon type %ld\n", getv(iconobj, MA_Icon_ImageType)));
			break;
		#endif
	}
}

static APTR make_flags(struct Data *data)
{
	APTR obj;

	obj = ColGroup(2),
		Child, data->chk_script   = MUICreateCheckbox( MSG_INFOWIN_SCRIPT, FALSE, ""),
		Child, MUICreateLabel( MSG_INFOWIN_SCRIPT, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
		Child, data->chk_archived = MUICreateCheckbox( MSG_INFOWIN_ARCHIVED, FALSE, ""),
		Child, MUICreateLabel( MSG_INFOWIN_ARCHIVED, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
		Child, data->chk_readable = MUICreateCheckbox( MSG_INFOWIN_READABLE, FALSE, ""),
		Child, MUICreateLabel( MSG_INFOWIN_READABLE, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
		Child, data->chk_writable = MUICreateCheckbox( MSG_INFOWIN_WRITABLE, FALSE, ""),
		Child, MUICreateLabel( MSG_INFOWIN_WRITABLE, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
		Child, data->chk_executable = MUICreateCheckbox( MSG_INFOWIN_EXECUTABLE, FALSE, ""),
		Child, MUICreateLabel( MSG_INFOWIN_EXECUTABLE, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
		Child, data->chk_deletable = MUICreateCheckbox( MSG_INFOWIN_DELETABLE, FALSE, ""),
		Child, MUICreateLabel( MSG_INFOWIN_DELETABLE, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
	End;

	return (obj);
}


static void set_update_notifications(APTR obj, struct Data *data)
{
	switch (data->type)
	{
		case MV_Icon_Type_Drawer:
		case MV_Icon_Type_Tool:
		case MV_Icon_Type_Project:
			{
				/*
				 * Changes
				 */
				DoMethod(data->chk_script, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
					obj, 2, MM_Infowin_Changed, MV_Infowin_Changed_Protection
				);
				DoMethod(data->chk_archived, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
					obj, 2, MM_Infowin_Changed, MV_Infowin_Changed_Protection
				);
				DoMethod(data->chk_readable, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
					obj, 2, MM_Infowin_Changed, MV_Infowin_Changed_Protection
				);
				DoMethod(data->chk_writable, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
					obj, 2, MM_Infowin_Changed, MV_Infowin_Changed_Protection
				);
				DoMethod(data->chk_executable, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
					obj, 2, MM_Infowin_Changed, MV_Infowin_Changed_Protection
				);
				DoMethod(data->chk_deletable, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
					obj, 2, MM_Infowin_Changed, MV_Infowin_Changed_Protection
				);
			}
			break;
	}
}


static void copy_filename(struct Data *data, STRPTR path)
{
	int len = strlen(path) + 1;
	data->fname = AllocVec(len, MEMF_ANY);

	if (data->fname)
		strcpy(data->fname, path);
}


DEFNEW
{
	struct Data *data;
	APTR iconobj = NULL, o = 0, o1 = 0, o2 = 0, o3 = 0; /* = 0 to make gcc shut up (sigh) */
	APTR grp_content, grp_icon, bt_icon;
#if USE_OLD_ACTIONEDITOR
	APTR pagegrp, edit_bt, actionlist;
	static const char * const titles[] = { "Info", "Filetype", NULL };
#endif
	STRPTR pname;
	ULONG rxid = rxid; /* shut up gcc */
	struct infodata *infdata = NULL;
	LONG is_default;

	static STRPTR typemodes[3];

#define TM_TOOL    0
#define TM_PROJECT 1

	typemodes[0] = GSI(MSG_INFOWIN_TOOL);
	typemodes[1] = GSI(MSG_INFOWIN_PROJECT);

	FORTAG(INITTAGS)
	{
		case MA_Infowin_Iconobj:
			iconobj = (APTR)tag->ti_Data;
			break;

		case MA_Infowin_RXID:
			rxid = tag->ti_Data;
			break;

		case MA_Infowin_Infodata:
			infdata = (struct infodata *)tag->ti_Data;
			break;
	}
	NEXTTAG

	if (!iconobj)
	{
		return (0);
	}

	pname = (STRPTR)getv(iconobj, MA_Icon_Path);

	obj = DoSuperNew(cl, obj,
		MUIA_Window_Screen, get_screen(),
		MUIA_Window_ScreenTitle, screentitle,
		MUIA_Window_LeftEdge, MUIV_Window_LeftEdge_Moused,
		MUIA_Window_TopEdge, MUIV_Window_TopEdge_Moused,
		MUIA_Window_Width, MUIV_Window_Width_MinMax(10), /* XXX: fishy fishy.. */
		MUIA_Window_Height, MUIV_Window_Height_MinMax(10),
		MUIA_Window_ShowIconify, FALSE,
		MUIA_Window_ShowPrefs, FALSE,
		MUIA_Window_ShowJump, FALSE,
		MUIA_Window_ShowAbout, FALSE,
		MUIA_Window_NoMenus, TRUE,
#if USE_OLD_ACTIONEDITOR
		WindowContents, pagegrp = RegisterGroup(titles),
			Child, grp_content = VGroup, End,
			Child, VGroup,
				Child, VSpace(0),
				Child, HGroup,
					Child, HSpace(0),
					Child, actionlist = NewObject(getactionlistclass(), NULL, MA_Actionlist_Iconobj, iconobj, End,
					Child, HSpace(0),
				End,
				Child, VSpace(0),
				Child, MUI_MakeObject(MUIO_HBar, 2),
				Child, HGroup,
					Child, HSpace(0),
					Child, edit_bt = SimpleButton("Edit filetype actions"),
					Child, HSpace(0),
				End,
			End,
		End,
#else
		WindowContents, grp_content = VGroup, End,
#endif
		//TAG_MORE, INITTAGS,
	End;

	if (!obj)
	{
		return (IPTR)(NULL);
	}

#if USE_OLD_ACTIONEDITOR
	/*
	 * We disable the button for editing the filetype if it is internal.
	 */
	if (getv(actionlist, MA_Actionlist_Internal))
		set(edit_bt, MUIA_Disabled, TRUE);
	else {
		DoMethod(edit_bt, MUIM_Notify, MUIA_Pressed, FALSE,
			obj, 1, MM_Infowin_OpenActionEditor
		);
	}
#endif

	data = INST_DATA(cl, obj);
	data->grp_content = grp_content;
	
	data->iconobj = iconobj;
	data->rxid = rxid;

#if USE_OLD_ACTIONEDITOR
	data->edit_bt = edit_bt;
	data->pagegrp = pagegrp;
#endif

	data->type = getv(iconobj, MA_Icon_Type);

	if (!data->type)
	{
		data->type = MV_Icon_Type_Tool; /* default */
	}

	/*
	 * Build the window (XXX: handle appicon.. and Garbage ?)
	 */
	copy_filename(data, pname);

	switch (data->type)
	{
		case MV_Icon_Type_Disk:
		case MV_Icon_Type_Device:
		case MV_Icon_Type_Kick:
			snprintf( data->wintitle, sizeof( data->wintitle ), GSI(MSG_INFOWIN_WINDOW_TITLE_VOLUME), data->fname );
			break;

		case MV_Icon_Type_Drawer:
		{
			STRPTR s = FilePart(data->fname);
			snprintf(data->wintitle, sizeof(data->wintitle), GSI(MSG_INFOWIN_WINDOW_TITLE_DRAWER), *s ? s : data->fname);
			break;
		}

		case MV_Icon_Type_Tool:
			snprintf(data->wintitle, sizeof(data->wintitle), GSI(MSG_INFOWIN_WINDOW_TITLE_TOOL), FilePart(data->fname));
			break;

		case MV_Icon_Type_Project:
			snprintf(data->wintitle, sizeof(data->wintitle), GSI(MSG_INFOWIN_WINDOW_TITLE_PROJECT), FilePart(data->fname));
			break;
	}

	set(obj, MUIA_Window_Title, data->wintitle);

	/*
	 * Top
	 */
	if ((is_default = getv(iconobj, MA_Icon_IsDefault)))
	{
		bt_icon = MUICreateButton(MSG_INFOWIN_CREATE_ICON_GAD, NULL);

		if (bt_icon)
		{
			DoMethod(bt_icon, MUIM_Notify, MUIA_Pressed, FALSE, obj, 2, MM_Infowin_Changed, MV_Infowin_Changed_Icon);
			DoMethod(bt_icon, MUIM_Notify, MUIA_Pressed, FALSE, bt_icon, 3, MUIM_Set, MUIA_Disabled, TRUE);
		}
	}
	else
	{
		bt_icon = MUICreateButton(MSG_INFOWIN_USE_DEFAULT_ICON_GAD, NULL);
	}

	grp_icon = VGroup,
		Child, data->iconinfoobj = NewObject(getinfoicongroupclass(), NULL,
			MA_Infoicongroup_Child, data->iconobj,
			MA_Infoicongroup_Editable, TRUE,
			TAG_DONE),
		Child, data->txt_icontype = text2(NULL),
		Child, bt_icon,
		TAG_DONE);

	if (!grp_icon)
	{
		CoerceMethod(cl, obj, OM_DISPOSE);
		return (IPTR)(NULL);
	}

	DoMethod(data->iconinfoobj, MUIM_Notify, MA_Infoicongroup_Changed, TRUE,
		obj, 2, MM_Infowin_Changed, MV_Infowin_Changed_Icon
	);

	if (!is_default)
	{
		DoMethod(bt_icon, MUIM_Notify, MUIA_Pressed, FALSE, data->iconinfoobj, 3, MM_Infoicongroup_SetIcon, data->fname, TRUE);
	}

	switch (data->type)
	{
		case MV_Icon_Type_Disk:   /* XXX: should there be proper checking at a higher level to not be foolded here ? iconclass.. */
		case MV_Icon_Type_Device:
		case MV_Icon_Type_Kick:
			o = HGroup,
					Child, ColGroup(2),
						Child, NSLabel(MSG_INFOWIN_DISK_DISK),
						Child, data->txt_disk = NewObject(getsmarttextclass(), NULL, MUIA_ShortHelp, GSI(MSG_INFOWIN_DISK_DISK_HELP), End,
						Child, NSLabel(MSG_INFOWIN_DISK_SIZE),
						Child, data->txt_size = NewObject(getsmarttextclass(), NULL, MUIA_ShortHelp, GSI(MSG_INFOWIN_DISK_SIZE_HELP), End,
						Child, NSLabel(MSG_INFOWIN_DISK_USED),
						Child, data->txt_used = NewObject(getsmarttextclass(), NULL, MUIA_ShortHelp, GSI(MSG_INFOWIN_DISK_USED_HELP), End,
						Child, NSLabel(MSG_INFOWIN_DISK_FREE),
						Child, data->txt_free = NewObject(getsmarttextclass(), NULL, MUIA_ShortHelp, GSI(MSG_INFOWIN_DISK_FREE_HELP), End,
						Child, NSLabel(MSG_INFOWIN_DISK_FILESYSTEM),
						Child, data->txt_fs = MUICreateTextNoFrame(MSG_INFOWIN_DISK_FILESYSTEM,NULL),
					End,

					Child, HSpace(0),
					Child, grp_icon,
					Child, HSpace(0),

					Child, ColGroup(2),
						Child, NSLabel(MSG_INFOWIN_DISK_TYPE),
						Child, data->txt_type = MUICreateTextNoFrame( MSG_INFOWIN_DISK_TYPE, NULL),
						Child, NSLabel(MSG_INFOWIN_DISK_BLOCKSIZE),
						Child, data->txt_blocksize = MUICreateTextNoFrame( MSG_INFOWIN_DISK_BLOCKSIZE, NULL),
						Child, NSLabel(MSG_INFOWIN_DISK_STATUS),
						Child, data->txt_status = MUICreateTextNoFrame( MSG_INFOWIN_DISK_STATUS, NULL),
						Child, NSLabel(MSG_INFOWIN_DISK_FEATURES),
						Child, data->txt_features = NewObject(getaddtextclass(), NULL, MUIA_ShortHelp, GSI(MSG_INFOWIN_DISK_FEATURES_HELP), End,
					End,
				End;
			break;

		case MV_Icon_Type_Drawer:
			o = HGroup,
					Child, ColGroup(2),
						Child, NSLabel(MSG_INFOWIN_FILES),
						Child, data->txt_files     = NewObject(getsmarttextclass(), NULL, MA_SmartText_ViewChars, 8, MUIA_ShortHelp, GSI(MSG_INFOWIN_FILES), End,
						Child, NSLabel(MSG_INFOWIN_DIRS),
						Child, data->txt_dirs      = NewObject(getsmarttextclass(), NULL, MA_SmartText_ViewChars, 8, MUIA_ShortHelp, GSI(MSG_INFOWIN_DIRS),End,
						Child, NSLabel(MSG_INFOWIN_HARDLINKS),
						Child, data->txt_hardlinks = NewObject(getsmarttextclass(), NULL, MA_SmartText_ViewChars, 8, MUIA_ShortHelp, GSI(MSG_INFOWIN_HARDLINKS_HELP),End,
						Child, NSLabel(MSG_INFOWIN_SOFTLINKS),
						Child, data->txt_softlinks = NewObject(getsmarttextclass(), NULL, MA_SmartText_ViewChars, 8, MUIA_ShortHelp, GSI(MSG_INFOWIN_SOFTLINKS_HELP),End,
						Child, data->bt_scan       = MUICreateButton( MSG_INFOWIN_SCAN, NULL),
						Child, data->txt_total     = NewObject(getsmarttextclass(), NULL, End,
					End,

					Child, HSpace(0),
					Child, grp_icon,
					Child, HSpace(0),

					Child, make_flags(data),
				End;
			break;

		case MV_Icon_Type_Tool:
		case MV_Icon_Type_Project:
			{
				o = HGroup,
						Child, ColGroup(2),
							Child, NSLabel(MSG_INFOWIN_TYPE_TOOL_PROJECT),
							Child, data->cyc_type      = MUICreateCycle(MSG_INFOWIN_TYPE_TOOL_PROJECT, typemodes, 0,0,""),
							Child, NSLabel(MSG_INFOWIN_BLOCKS),
							Child, data->txt_blocks    = MUICreateTextNoFrame( MSG_INFOWIN_BLOCKS, NULL),
							Child, NSLabel(MSG_INFOWIN_SIZE),
							Child, data->txt_bytes     = MUICreateTextNoFrame( MSG_INFOWIN_SIZE, NULL),
							Child, NSLabel(MSG_INFOWIN_SIZEONDISK),
							Child, data->txt_bytesdisk = MUICreateTextNoFrame( MSG_INFOWIN_SIZEONDISK, NULL),
							Child, NSLabel(MSG_INFOWIN_STACK),
							Child, HGroup,
								Child, data->str_stack = NumericStringObject,
									MUIA_FixWidthTxt, "00000000",
									MUIA_Numeric_Min, 256,
									MUIA_Numeric_Max, 1073741824,
									MUIA_Numeric_FormatFactor, 1024,
									MUIA_String_Integer, getv(iconobj, MA_Icon_StackSize),
									MUIA_ShortHelp, GSI(MSG_INFOWIN_STACK_HELP),
									MUIA_CycleChain, 1,
									MUIA_ControlChar, MUIGetUnderScore( MSG_INFOWIN_STACK ),
								End,
								Child, HSpace(0),
							End,
						End,

						Child, HSpace(0),
						Child, grp_icon,
						Child, HSpace(0),

						Child, make_flags(data),
					End;

				if (o)
				{
					#if 0
					DoMethod(data->str_stack, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
						obj, 2, MM_Infowin_Changed, MV_Infowin_Changed_Stack );
					#else
					DoMethod(data->str_stack, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
						obj, 2, MM_Infowin_Changed, MV_Infowin_Changed_Stack
					);
					#endif

					set( data->cyc_type, MUIA_Cycle_Active, ((data->type == MV_Icon_Type_Tool) ? TM_TOOL : TM_PROJECT));
				}
			}
			break;
	}

	if (!o)
	{
		CoerceMethod(cl, obj, OM_DISPOSE);
		return (IPTR)(NULL);
	}

	adjust_layout(data, iconobj);
	DoMethod(data->grp_content, OM_ADDMEMBER, o);

	set_icontype(data);

	/*
	 * Mid
	 */
	if (data->type == MV_Icon_Type_Tool || data->type == MV_Icon_Type_Project)
	{
		data->is_fast = doslistcache_fastdevice(pname);

		data->ogrp = ColGroup(2),
				Child, data->bt_version = MUICreateButton( MSG_INFOWIN_VERSION, NULL ),
				Child, data->txt_version = MUICreateMarkableTextNoFrame( MSG_INFOWIN_VERSION, NULL),
				Child, data->bt_md5sum = MUICreateButton( MSG_INFOWIN_MD5SUM, NULL ),
				Child, data->txt_md5sum = MUICreateMarkableTextNoFrame( MSG_INFOWIN_MD5SUM, NULL),
			End;

		if (data->ogrp)
		{
			DoMethod(data->bt_version, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_Infowin_Version);
			DoMethod(data->bt_md5sum, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_Infowin_MD5sum);

			DoMethod(data->cyc_type, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, MUIV_Notify_Application, 5,
				MUIM_Application_PushMethod, obj, 2,
				MM_Infowin_ChangeMode, MUIV_TriggerValue
			);

			DoMethod(data->cyc_type, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
				obj, 2, MM_Infowin_Changed, MV_Infowin_Changed_DefaultTool
			);
		}
	}
	else if (data->type == MV_Icon_Type_Disk || data->type == MV_Icon_Type_Device || data->type == MV_Icon_Type_Kick)
	{
		data->ogrp = ColGroup(2),
			Child, NSLabel(MSG_INFOWIN_CREATED),
			Child, data->txt_date = MUICreateTextNoFrame(MSG_INFOWIN_CREATED, NULL),
			End;
	}
	else
	{
		data->ogrp = ColGroup(2), End;
	}

	if (!(data->ogrp))
	{
		CoerceMethod(cl, obj, OM_DISPOSE);
		return (IPTR)(NULL);
	}

	DoMethod(data->grp_content, OM_ADDMEMBER, data->ogrp);

	switch (data->type)
	{
		case MV_Icon_Type_Drawer:
		case MV_Icon_Type_Tool:
		case MV_Icon_Type_Project:
			o = data->txt_date_label = NSLabel(MSG_INFOWIN_LAST_CHANGED);
			o1 = data->txt_date      = MUICreateTextNoFrame(MSG_INFOWIN_LAST_CHANGED, NULL);
			o2 = data->txt_comment   = NSLabel(MSG_INFOWIN_COMMENT);
			o3 = data->str_comment   = MUICreateString( MSG_INFOWIN_COMMENT, 79, NULL );
			if (o && o1 && o2 && o3)
			{
				DoMethod(data->ogrp, OM_ADDMEMBER, o);
				DoMethod(data->ogrp, OM_ADDMEMBER, o1);
				DoMethod(data->ogrp, OM_ADDMEMBER, o2);
				DoMethod(data->ogrp, OM_ADDMEMBER, o3);

				DoMethod(o3, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
					obj, 2, MM_Infowin_Changed, MV_Infowin_Changed_Comment
				);
			}
			else
			{
				MUI_DisposeObject(o);
				MUI_DisposeObject(o1);
				MUI_DisposeObject(o2);
				MUI_DisposeObject(o3);

				CoerceMethod(cl, obj, OM_DISPOSE);
				return (IPTR)(NULL);
			}
			break;
	}

	if (data->type == MV_Icon_Type_Project)
	{
		STRPTR p = (STRPTR)getv(iconobj, MA_Icon_DefaultTool);

		if ( p )
		{
			stccpy(data->defaulttool, p, sizeof(data->defaulttool));
		}
	}

	/*
	 * Bottom
	 */
	switch (data->type)
	{
		case MV_Icon_Type_Disk:
		case MV_Icon_Type_Device:
		case MV_Icon_Type_Kick:
			break;

		case MV_Icon_Type_Drawer:
		case MV_Icon_Type_Tool:
		case MV_Icon_Type_Project:
			if (1 || !getv(iconobj, MA_Icon_IsDefault))
			{
				data->txt_tooltypes = VGroup,
					Child, VSpace(0),
					Child, NSLabel(MSG_INFOWIN_TOOLTYPES),
					Child, VSpace(0),
				End;
				data->lv_tooltypes = NewObject(getttlistviewclass(), NULL, MUIA_CycleChain, 1,
						MUIA_ShortHelp, GSI( MSG_INFOWIN_TOOLTYPES_HELP ),
						MUIA_ControlChar, MUIGetUnderScore( MSG_INFOWIN_TOOLTYPES ),
						MUIA_Listview_List, NewObject(getttlistclass(), NULL,
						InputListFrame,
						MUIA_List_DestructHook, &tt_dest_hook,
					End,
				End;
				data->grp_tooltypes = HGroup,
					Child, data->str_tooltype = StringObject, MUIA_Weight, 500,
						StringFrame,
						MUIA_String_MaxLen, 255,
						MUIA_CycleChain, 1,
					End,
					Child, data->bt_new = MUICreateButton( MSG_INFOWIN_NEW, NULL ),
					Child, data->bt_del = MUICreateButton( MSG_INFOWIN_DEL, NULL ),
				End;
				data->space_tooltypes = HSpace(0);

				if (data->space_tooltypes && data->txt_tooltypes && data->lv_tooltypes && data->grp_tooltypes && data->str_tooltype)
				{
					DoMethod(data->ogrp, OM_ADDMEMBER, data->txt_tooltypes);
					DoMethod(data->ogrp, OM_ADDMEMBER, data->lv_tooltypes);
					DoMethod(data->ogrp, OM_ADDMEMBER, data->space_tooltypes);
					DoMethod(data->ogrp, OM_ADDMEMBER, data->grp_tooltypes);

					DoMethod(data->lv_tooltypes, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
						obj, 2, MM_Infowin_TTSetString, MUIV_TriggerValue
					);

					DoMethod(data->str_tooltype, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
						obj, 1, MM_Infowin_TTCopyString
					);

					DoMethod(data->bt_del, MUIM_Notify, MUIA_Pressed, FALSE,
						data->lv_tooltypes, 2, MUIM_List_Remove, MUIV_List_Remove_Active
					);

					DoMethod(data->bt_new, MUIM_Notify, MUIA_Pressed, FALSE,
						obj, 1, MM_Infowin_TTAdd
					);

					set(data->bt_del, MUIA_Disabled, TRUE);
					set(data->str_tooltype, MUIA_Disabled, TRUE);

					DoMethod(data->lv_tooltypes, MUIM_Notify, MA_TTList_Changed, TRUE,
						obj, 2, MM_Infowin_Changed, MV_Infowin_Changed_Tooltype
					);
					DoMethod(data->str_tooltype, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
						obj, 2, MM_Infowin_Changed, MV_Infowin_Changed_Tooltype
					);
				}
				else
				{
					if (data->txt_tooltypes)
						MUI_DisposeObject(data->txt_tooltypes);
					if (data->lv_tooltypes)
						MUI_DisposeObject(data->lv_tooltypes);
					if (data->grp_tooltypes)
						MUI_DisposeObject(data->grp_tooltypes);

					CoerceMethod(cl, obj, OM_DISPOSE);
					return (IPTR)(NULL);
				}
			}
			break;
	}

	o = MUI_MakeObject(MUIO_HBar, 2);
	o1 = HGroup,
			Child, data->bt_save = MUICreateButton( MSG_INFOWIN_SAVE, NULL ),
			Child, HSpace(0),
			Child, data->bt_cancel = MUICreateButton( MSG_INFOWIN_CANCEL, NULL ),
		End;

	if (o && o1)
	{
		adjust_layout(data, iconobj);
		DoMethod(grp_content, OM_ADDMEMBER, o);
		DoMethod(grp_content, OM_ADDMEMBER, o1);
	}
	else
	{
		if (o)
			MUI_DisposeObject(o);
		if (o1)
			MUI_DisposeObject(o1);

		CoerceMethod(cl, obj, OM_DISPOSE);
			return (IPTR)(NULL);
	}

	/*
	 * Fill-in all the fields.
	 */
	switch (data->type)
	{
		case MV_Icon_Type_Disk:
		case MV_Icon_Type_Device:
		case MV_Icon_Type_Kick:
			{
				UQUAD sz;
				TEXT temp[64]; /* should be enough for date etc (tm) */
				TEXT time[16];
				struct device_info *di;
				STRPTR devicename = "----"; /* e.g. RAMDISK: has no proper device */

				if( ( di = deviceinfo_build( data->fname, TRUE ) ) )
				{
					if( di->name && di->name[0] )
					{
						devicename = di->name;
					}
				}
				set( data->txt_disk, MUIA_Text_Contents, devicename );
				deviceinfo_delete( di ); /* no check needed here */

				if (infdata->hasdi64)
				{
					sz = infdata->inf.di_NumBlocks * infdata->inf.di_BytesPerBlock;
					capacity_format_size( temp, sizeof( temp ), sz );
					set(data->txt_size, MUIA_Text_Contents, temp);

					sz = infdata->inf.di_NumBlocks * infdata->inf.di_BytesPerBlock;
					set( data->txt_used, MA_CapacityText_Total, &sz );

					sz = infdata->inf.di_NumBlocksUsed * infdata->inf.di_BytesPerBlock;
					capacity_format_size(temp, sizeof(temp), sz);
					set(data->txt_used, MUIA_Text_Contents, temp);
					set(data->txt_free, MA_CapacityText_Total, &sz);

					sz = (infdata->inf.di_NumBlocks - infdata->inf.di_NumBlocksUsed) * infdata->inf.di_BytesPerBlock;
					capacity_format_size(temp, sizeof(temp), sz);
					set(data->txt_free, MUIA_Text_Contents, temp);

					snprintf(temp, sizeof(temp), GSI(MSG_INFOWIN_BYTES), (UQUAD)infdata->inf.di_BytesPerBlock);
					DoMethod(data->txt_blocksize, MUIM_SetAsString, MUIA_Text_Contents, temp);

					switch (infdata->inf.di_DiskState)
					{
						case ID_WRITE_PROTECTED:
							set(data->txt_status, MUIA_Text_Contents, GSI(MSG_INFOWIN_STATUS_WRITE_PROTECTED));
							break;

						case ID_VALIDATING:
							set(data->txt_status, MUIA_Text_Contents, GSI(MSG_INFOWIN_STATUS_VALIDATING));
							break;

						case ID_VALIDATED:
							set(data->txt_status, MUIA_Text_Contents, GSI(MSG_INFOWIN_STATUS_VALIDATED));
							break;
					}
				}

				if (infdata->hasdi64 && infdata->inf.di_Flags)
				{
					if (infdata->inf.di_Flags & DIF_64BIT)
					{
						set(data->txt_features, MA_AddText_Contents, GSI(MSG_INFOWIN_FLAGS_LARGE_FILES));
					}
					if (infdata->inf.di_Flags & DIF_CASE)
					{
						set(data->txt_features, MA_AddText_Contents, GSI(MSG_INFOWIN_FLAGS_CASE_SENSITIVE));
					}
				}
				else
				{
					set(data->txt_features, MA_AddText_Contents, GSI(MSG_INFOWIN_FLAGS_NONE));
				}
				if (infdata->type)
				{
					set(data->txt_type, MUIA_Text_Contents, infdata->type);
				}

				if (!infdata->hasdi64 || infdata->inf.di_DiskType == -1)
				{
					set(data->txt_fs, MUIA_Text_Contents, GSI(MSG_INFOWIN_TYPE_NOMEDIA));
				}
				else
				{
					struct dlcnode *dln, *dln2;

					if ((dln = doslistcache_find_dlcdevice_by_volumename(data->fname)) || (dln = doslistcache_find_dlcdevice_by_devicename(data->fname)))
					{
						CONST_STRPTR dostype_dl;

						if ( (dostype_dl = dostype_get(dln->disktype)) )
						{
							DoMethod(data->txt_fs, MUIM_SetAsString, MUIA_Text_Contents, "%s", dostype_dl);
						}
						else
						{
							TEXT str_dl[5];

							dostype_to_str(dln->disktype, str_dl);

							DoMethod(data->txt_fs, MUIM_SetAsString, MUIA_Text_Contents, "%s", str_dl);
						}

					}

					/* In theory could happen... */

					ASSERT(dln);

					if ( dln && (dln2 = doslistcache_find_dlcvolume_by_devicename(dln->name)) ) /* XXX: hm.. maybe we could use the mp_Task field.. */
					{
						if (datestamp_to_str(&dln2->volumedate, temp, time))
						{
							DoMethod(data->txt_date, MUIM_SetAsString, MUIA_Text_Contents, "%s, %s", temp, time);
						}
					}
				}
				data->has_file = TRUE;
			}
			break;

		case MV_Icon_Type_Drawer:
		case MV_Icon_Type_Tool:
		case MV_Icon_Type_Project:
			{
				if (infdata->hasfi64)
				{
					TEXT temp[64]; /* should be enough for date etc (tm) */
					TEXT time[16];
					UQUAD sz;

					if (infdata->hasdi64 && (infdata->inf.di_BytesPerBlock > 0))
					{
						sz = (infdata->fi.fi_Size + infdata->inf.di_BytesPerBlock - 1) / infdata->inf.di_BytesPerBlock * infdata->inf.di_BytesPerBlock;
					}
					else
					{
						sz = 0;
					}

					if (data->type != MV_Icon_Type_Drawer)
					{
						/*
						 * It could be argued that MSG_INFOWIN_BYTES could
						 * have %lu instead of %llu. However IMO it makes more
						 * sense to have 64bit formatting there aswell, in case
						 * we need it elsewhere (or change this logic somehow).
						 * - piru
						 */
						if (infdata->fi.fi_Size > 10240ULL)
						{
							TEXT fmt[16];

							capacity_format_size(fmt, sizeof(fmt), (UQUAD)infdata->fi.fi_Size);
							snprintf(temp, sizeof(temp), GSI(MSG_INFOWIN_MBYTES_BYTES), fmt, (UQUAD)infdata->fi.fi_Size);
							DoMethod(data->txt_bytes, MUIM_SetAsString, MUIA_Text_Contents, temp);
							if (sz)
							{
								capacity_format_size(fmt, sizeof(fmt), (QUAD)sz);
								snprintf(temp, sizeof(temp), GSI(MSG_INFOWIN_MBYTES_BYTES), fmt, (UQUAD)sz);
								DoMethod(data->txt_bytesdisk, MUIM_SetAsString, MUIA_Text_Contents, temp);
							}
						}
						else
						{
							snprintf(temp, sizeof(temp), GSI(MSG_INFOWIN_BYTES), (UQUAD)infdata->fi.fi_Size);
							DoMethod(data->txt_bytes, MUIM_SetAsString, MUIA_Text_Contents, temp);
							if (sz)
							{
								snprintf(temp, sizeof(temp), GSI(MSG_INFOWIN_BYTES), (UQUAD)sz);
								DoMethod(data->txt_bytesdisk, MUIM_SetAsString, MUIA_Text_Contents, temp);
							}
						}

						snprintf(temp, sizeof(temp), "%llu", (UQUAD)infdata->fi.fi_NumBlocks);
						DoMethod(data->txt_blocks, MUIM_SetAsString, MUIA_Text_Contents, temp);
					}

					if (!sz)
					{
						set(data->txt_bytesdisk, MUIA_Text_Contents, GSI(MSG_INFOWIN_SIZE_UNKNOWN));
					}

					/*
					 * Sigh, DOS really sucks. If I catch
					 * the moron who inverted *some* flags
					 * I'll kill him.
					 */
					set(data->chk_script, MUIA_Selected, infdata->fi.fi_Protection & FIBF_SCRIPT);
					set(data->chk_archived, MUIA_Selected, infdata->fi.fi_Protection & FIBF_ARCHIVE);
					set(data->chk_readable, MUIA_Selected, !(infdata->fi.fi_Protection & FIBF_READ));
					set(data->chk_writable, MUIA_Selected, !(infdata->fi.fi_Protection & FIBF_WRITE));
					set(data->chk_executable, MUIA_Selected, !(infdata->fi.fi_Protection & FIBF_EXECUTE));
					set(data->chk_deletable, MUIA_Selected, !(infdata->fi.fi_Protection & FIBF_DELETE));


					if (datestamp_to_str(&infdata->fi.fi_Date, temp, time))
					{
						DoMethod(data->txt_date, MUIM_SetAsString, MUIA_Text_Contents, "%s, %s", temp, time);
					}

					if (infdata->fi.fi_Comment)
					{
						set(data->str_comment, MUIA_String_Contents, infdata->fi.fi_Comment);
					}
					data->has_file = TRUE;
				}
			}

			/*
			 * Adds the tooltypes
			 */
			if (1 || !getv(iconobj, MA_Icon_IsDefault))
			{
				struct ttnode *tn;
				struct MinList *tl;
				STRPTR p;

				if ( (tl = (struct MinList *)getv(iconobj, MA_Icon_ToolTypeList)) )
				{
					ITERATELIST(tn, tl)
					{
						if ( (p = malloc(MAX_TT_LENGTH)) ) /* a bit of a waste but that's ok */
						{
							stccpy(p, tn->tt, MAX_TT_LENGTH);
							DoMethod(data->lv_tooltypes, MUIM_List_InsertSingle, p, MUIV_List_Insert_Bottom);
						}
						/* XXX: we should really fail there.. */
					}
				}
			}

			if (data->type == MV_Icon_Type_Drawer)
			{
				DoMethod(data->bt_scan, MUIM_Notify, MUIA_Pressed, FALSE,
					obj, 1, MM_Infowin_Scan
				);
			}
			else
			{
				ULONG autolimit = _aprefs(infowinautolimit); /* 0 == no limit check */

				if (data->is_fast && 
                   (autolimit ? ((infdata->fi.fi_Size < autolimit) ? TRUE : FALSE) : FALSE))
				{
					if (_aprefs(infowinautomd5sum))
					{
						data->autogetver = _aprefs(infowinautoversion) ? TRUE : FALSE; /* starts automatic version'ing *after* automatic md5sum'ing is done */
						methodstack_push(obj, 1, MM_Infowin_MD5sum);
					}
					else if (_aprefs(infowinautoversion))
					{
						methodstack_push(obj, 1, MM_Infowin_Version);
					}
				}
			}
			break;
	}

	DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
		MUIV_Notify_Application, 4, MUIM_Application_PushMethod, obj, 1, MM_Infowin_Close
	);

	DoMethod(data->bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE,
		MUIV_Notify_Application, 4, MUIM_Application_PushMethod, obj, 1, MM_Infowin_Close
	);

	DoMethod(data->bt_save, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1, MM_Infowin_Save
	);

	if (!data->has_file)
	{
		switch (data->type)
		{
			case MV_Icon_Type_Disk:
			case MV_Icon_Type_Device:
			case MV_Icon_Type_Kick:
				/* XXX: maybe more later.. */
				break;

			case MV_Icon_Type_Drawer:
			case MV_Icon_Type_Tool:
			case MV_Icon_Type_Project:
				if (data->type == MV_Icon_Type_Drawer)
				{
					set(data->bt_scan, MUIA_Disabled, TRUE);
				}
				else
				{
					DoMethod(data->ogrp, MUIM_Group_InitChange);
					DoMethod(data->ogrp, OM_REMMEMBER, data->txt_version);
					DoMethod(data->ogrp, OM_REMMEMBER, data->bt_version);
					DoMethod(data->ogrp, OM_REMMEMBER, data->txt_md5sum);
					DoMethod(data->ogrp, OM_REMMEMBER, data->bt_md5sum);
					DoMethod(data->ogrp, MUIM_Group_ExitChange);

					MUI_DisposeObject(data->txt_version);
					MUI_DisposeObject(data->bt_version);
					MUI_DisposeObject(data->txt_md5sum);
					MUI_DisposeObject(data->bt_md5sum);

					data->txt_version = NULL;
					data->bt_version = NULL;
					data->txt_md5sum = NULL;
					data->bt_md5sum = NULL;
				}

				set(data->str_comment, MUIA_Disabled, TRUE);
				set(data->chk_script, MUIA_Disabled, TRUE);
				set(data->chk_archived, MUIA_Disabled, TRUE);
				set(data->chk_readable, MUIA_Disabled, TRUE);
				set(data->chk_writable, MUIA_Disabled, TRUE);
				set(data->chk_executable, MUIA_Disabled, TRUE);
				set(data->chk_deletable, MUIA_Disabled, TRUE);
				break;
			#ifdef DEBUG
			default:
				PDB(("eek.. how did it end here ?\n"));
				break;
			#endif
		}
	}

	if (data->type == MV_Icon_Type_Tool)
	{
		DoMethod(obj, MM_Infowin_ChangeMode, TM_TOOL);
	}
	else if (data->type == MV_Icon_Type_Project)
	{
		DoMethod(obj, MM_Infowin_ChangeMode, TM_PROJECT);
	}

	set_update_notifications(obj, data);

	set(data->bt_save, MUIA_Disabled, TRUE);
	data->infdata = infdata;

	return ((ULONG)obj);
}


DEFDISP
{
	GETDATA;

	FreeVec(data->fname);

	if (data->infdata)
	{
		free(data->infdata);
	}
	return (DOSUPER);
}


DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_Window_ID:
			*msg->opg_Storage = 0;
			return (TRUE);

		case MA_Window_Path:
			*msg->opg_Storage = (IPTR)data->fname;
			return (TRUE);

		case MA_Window_Type:
			*msg->opg_Storage = MV_Window_Type_Info;
			return (TRUE);
	}
	return (DOSUPER);
}


DEFSET
{
	struct TagItem *ti;

	if ( (ti = FindTagItem(MA_Infowin_RXID, INITTAGS)) )
	{
		GETDATA;

		if (data->rxid)
		{
			rx_set_result(data->rxid, 5, NULL); /* that's like if the user canceled */
			rx_reply_id(data->rxid);
		}
		data->rxid = ti->ti_Data;

	}
	return (DOSUPER);
}


DEFTMETHOD(Infowin_Close)
{
	GETDATA;

	set(obj, MUIA_Window_Open, FALSE);
	data->closing = TRUE;
	threads_abort(obj, NULL);

	return (0);
}


DEFSMETHOD(Thread_Finished)
{
	GETDATA;

	switch (msg->action)
	{
		case TA_File_GetSize:
			data->jobs &= ~TJ_GETSIZE;
			break;

		case TA_File_FindVer:
			data->jobs &= ~TJ_FINDVER;
			break;

		case TA_File_MD5sum:
			data->jobs &= ~TJ_MD5SUM;
			break;

		case TA_Icon_Write:
			data->jobs &= ~TJ_ICONWRITE;
			break;
	}

	if (data->closing && !data->jobs)
	{
		if (data->rxid)
		{
			ULONG res;

			if (data->saved)
			{
				if (msg->status)
				{
					res = 0;
				}
				else
				{
					res = 21;
				}
			}
			else
			{
				res = 5;
			}

			rx_set_result(data->rxid, res, NULL);
			rx_reply_id(data->rxid);
		}
		set(obj, MUIA_Window_Open, FALSE);
		DoMethod(app, OM_REMMEMBER, obj);
		MUI_DisposeObject(obj);
	}
	return (0);
}


DEFSMETHOD(Infowin_TTSetString)
{
	GETDATA;
	STRPTR p;

	DoMethod(data->lv_tooltypes, MUIM_List_GetEntry, msg->entry, &p);
	if (p)
	{
		nnset(data->str_tooltype, MUIA_String_Contents, p);
	}
	else
	{
		nnset(data->str_tooltype, MUIA_String_Contents, "");
	}
	set(data->str_tooltype, MUIA_Disabled, !p);
	set(data->bt_del, MUIA_Disabled, !p);

	return (0);
}


DEFTMETHOD(Infowin_TTAdd)
{
	GETDATA;
	STRPTR p;

	if ( (p = malloc(MAX_TT_LENGTH)) )
	{
		*p = '\0';
		DoMethod(data->lv_tooltypes, MUIM_List_InsertSingle, p, MUIV_List_Insert_Bottom);
		set(data->lv_tooltypes, MUIA_List_Active, MUIV_List_Active_Bottom);
		set(obj, MUIA_Window_ActiveObject, data->str_tooltype);
	}
	/* XXX */
	return (0);
}


DEFTMETHOD(Infowin_TTCopyString)
{
	GETDATA;
	STRPTR p;

	DoMethod(data->lv_tooltypes, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &p);
	if (p)
	{
		stccpy(p, (STRPTR)getv(data->str_tooltype, MUIA_String_Contents), MAX_TT_LENGTH);
		DoMethod(data->lv_tooltypes, MUIM_List_Redraw, MUIV_List_Redraw_Active);
	}
	return (0);
}


DEFTMETHOD(Infowin_Scan)
{
	GETDATA;

	set(data->bt_scan, MUIA_Disabled, TRUE);
	set(data->txt_dirs, MUIA_Text_Contents, "0");
	set(data->txt_files, MUIA_Text_Contents, "0");
	set(data->txt_hardlinks, MUIA_Text_Contents, "0");
	set(data->txt_softlinks, MUIA_Text_Contents, "0");
	if (do_action(obj, TA_File_GetSize,
		TT_File_GetSize_Path, data->fname,
	TAG_DONE))
	{
		data->jobs |= TJ_GETSIZE;
	}
	return (0);
}


DEFSMETHOD(Infowin_UpdateSize)
{
	GETDATA;
	TEXT t[22];

	capacity_format_size(t,sizeof(t),*msg->size);
	set(data->txt_total, MUIA_Text_Contents, t);

	if (msg->numdirs)
	{
		sprintf(t, "%lu", msg->numdirs);
		set(data->txt_dirs, MUIA_Text_Contents, t);
	}

	sprintf(t, "%lu", msg->numfiles);
	set(data->txt_files, MUIA_Text_Contents, t);

	if (msg->numhardlinks)
	{
		sprintf(t, "%lu", msg->numhardlinks);
		set(data->txt_hardlinks, MUIA_Text_Contents, t);
	}

	if (msg->numsoftlinks)
	{
		sprintf(t, "%lu", msg->numsoftlinks);
		set(data->txt_softlinks, MUIA_Text_Contents, t);
	}

	if (msg->final)
	{
		set(data->bt_scan, MUIA_Disabled, FALSE);
	}
	return (0);
}


DEFSMETHOD(Infowin_UpdateVersion)
{
	GETDATA;

	ASSERT(msg->text);

	set(data->txt_version, MUIA_Text_Contents, msg->text);
	set(data->bt_version, MUIA_Disabled, FALSE);

	return (0);
}


DEFTMETHOD(Infowin_Version)
{
	GETDATA;

	set(data->bt_version, MUIA_Disabled, TRUE);
	set(data->txt_version, MUIA_Text_Contents, GSI(MSG_INFOWIN_VERSION_SEARCHING));

	if (do_action(obj, TA_File_FindVer,
		TT_File_FindVer_Path, data->fname,
	TAG_DONE))
	{
		data->jobs |= TJ_FINDVER;
	}
	return (0);
}


DEFSMETHOD(Infowin_UpdateMD5sum)
{
	GETDATA;

	ASSERT(msg->text);

	set(data->txt_md5sum, MUIA_Text_Contents, msg->text);
	set(data->bt_md5sum, MUIA_Disabled, FALSE);

	if (data->autogetver && !data->closing)
	{
		data->autogetver = FALSE;
		DoMethod(obj, MM_Infowin_Version);
	}

	return (0);
}


DEFTMETHOD(Infowin_MD5sum)
{
	GETDATA;
	set(data->bt_md5sum, MUIA_Disabled, TRUE);
	set(data->txt_md5sum, MUIA_Text_Contents, GSI(MSG_INFOWIN_MD5_COMPUTING));
	if (do_action(obj, TA_File_MD5sum,
		TT_File_MD5sum_Path, data->fname,
	TAG_DONE))
	{
		data->jobs |= TJ_MD5SUM;
	}
	return (0);
}


static void save(APTR obj, struct Data *data, STRPTR name)
{
	ULONG i, mode = TV_Icon_Write_Mode_NoIcon;
	STRPTR p;
	APTR iconobj;

	iconobj = (APTR)getv(data->iconinfoobj, MA_Infoicongroup_Child);

	CHECKOBJECT(iconobj);

	data->saved = TRUE;

	set(iconobj, MA_Icon_Type, data->type);

	if (data->update_icon)
	{
		data->update_icon = FALSE;
		mode = getv(iconobj, MA_Icon_IsDefault) ? TV_Icon_Write_Mode_DefIcon : TV_Icon_Write_Mode_FullIcon;
	}

	/*
	 * Update the icon as it contains all the data.
	 */
	if (data->type == WBPROJECT && data->str_defaulttool)
	{
		set(iconobj, MA_Icon_DefaultTool, (APTR)getv(data->str_defaulttool, MUIA_String_Contents));
	}

	/*
	 * Save possible tooltypes.
	 */
	if (data->lv_tooltypes)
	{
		DoMethod(iconobj, MM_Icon_ClearToolTypes);
		for (i = 0;;i++)
		{
			DoMethod(data->lv_tooltypes, MUIM_List_GetEntry, i, &p);
			if (!p)
			{
				break;
			}
			DoMethod(iconobj, MM_Icon_InsertToolType, strlen(p) + 1, p);
		}
	}

	if (data->type == MV_Icon_Type_Tool || data->type == MV_Icon_Type_Project)
	{
		set(iconobj, MA_Icon_StackSize, getv(data->str_stack, MUIA_String_Integer));
	}

	PDB(("Write mode: %ld\n", mode));

	if (data->update_file && (data->type == MV_Icon_Type_Drawer || data->type == MV_Icon_Type_Tool || data->type == MV_Icon_Type_Project))
	{
		ULONG flags = 0;

		if (getv(data->chk_script, MUIA_Selected))
		{
			flags |= FIBF_SCRIPT;
		}

		if (getv(data->chk_archived, MUIA_Selected))
		{
			flags |= FIBF_ARCHIVE;
		}

		if (!getv(data->chk_readable, MUIA_Selected))
		{
			flags |= FIBF_READ;
		}

		if (!getv(data->chk_writable, MUIA_Selected))
		{
			flags |= FIBF_WRITE;
		}

		if (!getv(data->chk_executable, MUIA_Selected))
		{
			flags |= FIBF_EXECUTE;
		}

		if (!getv(data->chk_deletable, MUIA_Selected))
		{
			flags |= FIBF_DELETE;
		}

		if (do_action(iconobj, TA_Icon_Write,
			TT_Object, obj,
			TT_Icon_Write_Comment, getv(data->str_comment, MUIA_String_Contents),
			TT_Icon_Write_Flags, flags,
			TT_Icon_Write_Mode, mode,
			TT_Icon_Write_Path, name,
		TAG_DONE))
		{
			data->jobs |= TJ_ICONWRITE;
		}
	}
	else
	{
		if (do_action(iconobj, TA_Icon_Write, TT_Object, obj, TT_Icon_Write_Mode, mode, TT_Icon_Write_Path, name, TAG_DONE))
		{
			data->jobs |= TJ_ICONWRITE;
		}
	}

	data->update_file = FALSE;
	data->closing = TRUE;

	set(obj, MUIA_Window_Sleep, TRUE);
	/* XXX: well, this is lame.. if there's an error.. the window could stay blocked forever */
}


DEFTMETHOD(Infowin_Save)
{
	GETDATA;
	STRPTR name;

	if (data->fname && (name = name_build_info(data->fname)))
	{
		save(obj, data, name);
		name_delete(name);
	}

	return 0;
}


DEFTMETHOD(Infowin_OpenActionEditor)
{
	GETDATA;

	APTR o = NewObject(getactioneditwinclass(), NULL, MA_ActioneditWin_MimeNode,
		(APTR) getv(data->iconobj, MA_Icon_MimeType), TAG_DONE
	);

	if (o)
	{
		DoMethod(o, MM_ActioneditWin_Open);
	}

	return 0;
}


DEFSMETHOD(Infowin_ChangeMode)
{
	GETDATA;

	switch (msg->mode)
	{
		case TM_TOOL:
			if (data->txt_defaulttool)
			{
				STRPTR p = (STRPTR)getv(data->str_defaulttool, MUIA_String_Contents);

				if ( p )
				{
					stccpy(data->defaulttool, p, sizeof(data->defaulttool));
				}
				DoMethod(data->ogrp, MUIM_Group_InitChange);
				DoMethod(data->ogrp, OM_REMMEMBER, data->txt_defaulttool);
				DoMethod(data->ogrp, OM_REMMEMBER, data->str_defaulttool);
				DoMethod(data->ogrp, MUIM_Group_ExitChange);
				MUI_DisposeObject(data->txt_defaulttool);
				MUI_DisposeObject(data->str_defaulttool);
				data->txt_defaulttool = NULL;
				data->str_defaulttool = NULL;
				data->type = WBTOOL;
			}
			break;

		case TM_PROJECT:
			if (!data->txt_defaulttool)
			{
				if ( (data->txt_defaulttool = NSLabel(MSG_INFOWIN_DEFAULT_TOOL)) )
				{
					data->str_defaulttool = PopaslObject,
						MUIA_Popasl_Type, ASL_FileRequest,
							MUIA_ShortHelp, GSI(MSG_INFOWIN_DEFAULT_TOOL_HELP),
							MUIA_Popstring_String, StringObject,
							StringFrame,
							MUIA_String_MaxLen, PATH_SIZE,
							MUIA_CycleChain, 1,
							MUIA_ControlChar, MUIGetUnderScore( MSG_INFOWIN_DEFAULT_TOOL ),
							data->defaulttool[0] ? MUIA_String_Contents : TAG_IGNORE, data->defaulttool,
						End,
						MUIA_Popstring_Button, MUICreatePopButton( MSG_INFOWIN_DEFAULT_TOOL, MUII_PopFile, NULL),
						ASLFR_TitleText, GSI(MSG_INFOWIN_DEFAULT_TOOL_SELECT),
						//MUIA_ControlChar, ToUpper( MUIGetUnderScore( MSG_INFOWIN_DEFAULT_TOOL )),
					End;

					if (data->str_defaulttool)
					{
						DoMethod(data->ogrp, MUIM_Group_InitChange);
						DoMethod(data->ogrp, MUIM_Family_Insert, data->txt_defaulttool, data->str_comment);
						DoMethod(data->ogrp, MUIM_Family_Insert, data->str_defaulttool, data->txt_defaulttool);
						DoMethod(data->ogrp, MUIM_Group_ExitChange);
						data->type = WBPROJECT;

						DoMethod(data->str_defaulttool, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
							obj, 2, MM_Infowin_Changed, MV_Infowin_Changed_DefaultTool
						);
					}
					else
					{
						errormsg(ERR_NOMEM);
						MUI_DisposeObject(data->txt_defaulttool);
						data->txt_defaulttool = NULL;
					}
				}
			}
			break;

		#ifdef DEBUG
		default:
			PDB(("blerk, no mode\n"));
			break;
		#endif
	}

	//DoMethod(obj, MM_Infowin_Changed, MV_Infowin_Changed_Mode);

	return (0);
}


DEFSMETHOD(Infowin_Changed)
{
	GETDATA;

	if (msg->mode == MV_Infowin_Changed_Icon)
	{
		set_icontype(data);
	}

	switch (msg->mode)
	{
		case MV_Infowin_Changed_Icon:
			set_icontype(data);
		case MV_Infowin_Changed_Stack:
		case MV_Infowin_Changed_Tooltype:
		case MV_Infowin_Changed_DefaultTool:
		case MV_Infowin_Changed_Mode:
			data->update_icon = TRUE;
			break;

		default:
			data->update_file = TRUE;
			break;
	}

	set(data->bt_save, MUIA_Disabled, FALSE);

	return (0);
}


/* itix: from now on we accept pathlist instead of single path...
 *
 * But how to handle such list?
 */

ULONG tr_openinfowin(APTR obj, CONST CONST_STRPTR *pathlist, ULONG wait, ULONG rxid)
{
//	  CONST CONST_STRPTR *pl = pathlist;
	STRPTR p;
	ULONG rc = FALSE;

	THREAD;
	CHECKOBJECT(obj);
	ASSERT(pathlist);
	ASSERT(pathlist[0]);

	/* XXX: this goto construct is used to avoid reindentation */
loop:
	/* Skip empty strings */
	if ( *pathlist[0] && (p = name_build_info(pathlist[0])) )
	{
		struct infodata *infdata;
		APTR iconobj;

		if ( (infdata = malloc(sizeof(*infdata))) )
		{
			if ( (iconobj = (APTR)methodstack_push_sync(app, 2, MM_Application_CreateIconinfo, FALSE)) )
			{
				ULONG is_assign, is_devicename;

				is_devicename = isdevicename(pathlist[0]);
				is_assign = FALSE;

				if (is_devicename)
				{
					doslistcache_lock();

					if (!doslistcache_find_dlcdevice_by_volumename(pathlist[0]) && !doslistcache_find_dlcvolume_by_devicename(pathlist[0]))
					{
						is_assign = TRUE;
						is_devicename = FALSE;
					}

					doslistcache_unlock();
				}

				if (icon_read(p, iconobj,
					ICONTAG_Ancillary, TRUE, /* XXX: needed for d&d.. implement d&d then :) hm.. perhaps not needed after all */
					ICONTAG_Deficon, TRUE,
					ICONTAG_Infowin, TRUE,
					ICONTAG_IsAssign, is_assign,
				TAG_DONE))
				{
					BPTR l;

					methodstack_push_sync(iconobj, 3,
						MUIM_Set,
						MA_Icon_Path, pathlist[0]
					);

					infdata->hasdi64 = FALSE;
					infdata->hasfi64 = FALSE;

					/* Set to zero so it's not left undfined for device icons outside of
					   device root. (MT#6687) */
					infdata->type = NULL;

					if ( (l = Lock(pathlist[0], ACCESS_READ)) )
					{
						infdata->hasdi64 = info64(pathlist[0], &infdata->inf);

						if (is_devicename)
						{
							methodstack_push(iconobj, 3, MUIM_Set, MA_Icon_Type, MV_Icon_Type_Device);

							if (infdata->hasdi64)
							{
								switch (infdata->inf.di_DeviceType)
								{
									case -1:
										infdata->type = GSI(MSG_INFOWIN_DEVICETYPE_NA);
										break;

									case DG_DIRECT_ACCESS:
										infdata->type = GSI(MSG_INFOWIN_DEVICETYPE_DIRECT_ACCESS);
										break;

									case DG_SEQUENTIAL_ACCESS:
										infdata->type = GSI(MSG_INFOWIN_DEVICETYPE_SEQUENTIAL_ACCESS);
										break;

									case DG_PRINTER:
										infdata->type = GSI(MSG_INFOWIN_DEVICETYPE_PRINTER);
										break;

									case DG_PROCESSOR:
										infdata->type = GSI(MSG_INFOWIN_DEVICETYPE_PROCESSOR);
										break;

									case DG_WORM:
										infdata->type = GSI(MSG_INFOWIN_DEVICETYPE_WORM);
										break;

									case DG_CDROM:
										infdata->type = GSI(MSG_INFOWIN_DEVICETYPE_CDROM);
										break;

									case DG_SCANNER:
										infdata->type = GSI(MSG_INFOWIN_DEVICETYPE_SCANNER);
										break;

									case DG_OPTICAL_DISK:
										infdata->type = GSI(MSG_INFOWIN_DEVICETYPE_OPTICAL_DISK);
										break;

									case DG_MEDIUM_CHANGER:
										infdata->type = GSI(MSG_INFOWIN_DEVICETYPE_MEDIUM_CHANGER);
										break;

									case DG_COMMUNICATION:
										infdata->type = GSI(MSG_INFOWIN_DEVICETYPE_NETWORK);
										break;

									#ifdef DG_GRAPHICS
									case DG_GRAPHICS:
									#else
									case 10:
									#endif
										infdata->type = GSI(MSG_INFOWIN_DEVICETYPE_GRAPHICS);

									default:
										infdata->type = GSI(MSG_INFOWIN_DEVICETYPE_UNKNOWN);
										break;
								}
							}
							else
							{
								infdata->type = GSI(MSG_INFOWIN_DEVICETYPE_NA);
							}

						}
						else
						{
							ULONG is_dir;

							if ( (infdata->hasfi64 = examine64(pathlist[0], &infdata->fi)) )
							{
								is_dir = infdata->fi.fi_Type > 0;
							}
							else
							{
								is_dir = isdir(pathlist[0]);
							}

							if (is_dir)
								methodstack_push(iconobj, 3, MUIM_Set, MA_Icon_Type, MV_Icon_Type_Drawer);
						}

						UnLock(l);
					}

					if (methodstack_push_sync(app, 4, MM_Application_CreateInfowin, iconobj, rxid, infdata))
					{
						/* XXX: succeeded infowin class constructor controls infdata now, so we don't free it ourself */

						infdata = NULL;

						/* Also avoid using the same rxid more than once lest we get use-after-free.
						   This should not happen anyway as you can only pass one path with the
						   'iconinfo' command. But lets play is safe... - Piru */
						rxid = 0;

						if (wait)
						{
							rc = ASYNC;
						}
						else
						{
							rc = TRUE;
						}
					}
				}
			}

			if ( infdata )
				free(infdata);
		}
		name_delete(p);
	}

	pathlist++;

	if (pathlist[0])
		goto loop;

	return (rc);
}


BEGINMTABLE
DECNEW
DECDISP
DECGET
DECSET
DECTMETHOD(Infowin_Close)
DECSMETHOD(Thread_Finished)
DECSMETHOD(Infowin_TTSetString)
DECTMETHOD(Infowin_TTCopyString)
DECTMETHOD(Infowin_TTAdd)
DECTMETHOD(Infowin_Scan)
DECSMETHOD(Infowin_UpdateSize)
DECSMETHOD(Infowin_UpdateVersion)
DECTMETHOD(Infowin_Version)
DECSMETHOD(Infowin_UpdateMD5sum)
DECTMETHOD(Infowin_MD5sum)
DECTMETHOD(Infowin_Save)
DECTMETHOD(Infowin_OpenActionEditor)
DECSMETHOD(Infowin_ChangeMode)
DECSMETHOD(Infowin_Changed)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, infowinclass)
