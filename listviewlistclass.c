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
 * $Id: listviewlistclass.c,v 1.60 2026/01/25 17:36:24 kronos Exp $
 */

#include "ambient.h"

/* public */
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/locale.h>
#include <proto/wb.h>
#include <intuition/pointerclass.h>
#include <devices/rawkeycodes.h>

/* private */
#include "ambient_cat.h"
#include "listviewclass.h"
#include "locale.h"
#include "mui_func.h"
#include "command.h"
#include "rexx.h"
#include "dragdrop.h"
#include "threads.h"
#include "notify.h"
#include "contextmenu.h"
#include "mimetype.h"
#include "actiondispatcherclass.h"
#include "action.h"
#include "typescanner.h"
#include "file_func.h"
#include "methodstack.h"
#include "prefs.h"
#include "capacity.h"
#include "prefs.h"
#include "gfx_pen.h"
#include "hash.h"
#include "deficonpool.h"
#include "name.h"
#include "viewapi.h"
#include "mimeuri.h"
#include "str.h"
#include "time_func.h"
#include "iconio.h"
#include "fonts.h"
#include "ipc.h"
#include "wbarg.h"

#include "wblib/AppWindow.h"

#define LISTTITLE_BUFFER_SIZE 64

const ULONG  LVTITLES_FILES[] =
{
	MSG_CMENU_LVTITLES_NAME,
	MSG_CMENU_LVTITLES_SIZE,
	MSG_CMENU_LVTITLES_DATE,
	MSG_CMENU_LVTITLES_ATTRS,
	MSG_CMENU_LVTITLES_COMMENT,
	0,						/* There is no name for LISTVIEW_FILE_COL_ICON, because we use an image */
	MSG_CMENU_LVTITLES_FILETYPE,
	MSG_CMENU_LVTITLES_VERSION,
	MSG_CMENU_LVTITLES_FULLDATE,
	MSG_CMENU_LVTITLES_UID,
	MSG_CMENU_LVTITLES_GID,
	MSG_CMENU_LVTITLES_MD5
};

const ULONG  LVTITLES_DEVICES[] =
{
	MSG_CMENU_LVTITLES_VOLUMENAME,
	MSG_CMENU_LVTITLES_NAME,
	MSG_CMENU_LVTITLES_FREE,
	MSG_CMENU_LVTITLES_TOTAL,
	MSG_CMENU_LVTITLES_DOSTYPE,
	0                       /* There is no name for LISTVIEW_DEVICE_COL_ICON, because we use an image */
};

static const int LVTITLESFILESNUM   = sizeof(LVTITLES_FILES)/sizeof(ULONG);
static const int LVTITLESDEVICESNUM = sizeof(LVTITLES_DEVICES)/sizeof(ULONG);


struct dirty_node
{
	struct MinNode n;
	LONG pos;
	LONG dirty;
};

struct Data {
	ULONG type;      /* FLT_DEVICES or FLT_FILES */
	STRPTR path;
	APTR tempstorage;
	APTR tempstorage2;

	LONG  numselected;

	/* pens */
	LONG pen_file_fg;
	LONG pen_file_bg;
	LONG pen_file_sel_fg;
	LONG pen_file_sel_bg;
	LONG pen_directory_fg;
	LONG pen_directory_bg;
	LONG pen_directory_sel_fg;
	LONG pen_directory_sel_bg;
	LONG pen_softlink_fg;
	LONG pen_softlink_bg;
	LONG pen_hardlink_fg;
	LONG pen_hardlink_bg;
	LONG pen_volume_fg;
	LONG pen_volume_bg;
	LONG pen_assign_fg;
	LONG pen_assign_bg;
	LONG pen_source_fg;
	LONG pen_source_bg;
	LONG pen_destination_fg;
	LONG pen_destination_bg;
	LONG pen_column_fg;
	LONG pen_drop_fg;

	/* font styles */
	LONG style_file;
	LONG style_file_sel;
	LONG style_directory;
	LONG style_directory_sel;
	LONG style_softlink;
	LONG style_hardlink;
	LONG style_volume;
	LONG style_assign;
	LONG style_column;

	LONG sort_col;
	LONG prev_sort_col;
	LONG sort_direction;

	STRPTR * titles_files;
	STRPTR * titles_devices;

	ULONG icondisplay;
	ULONG alternated_rows;
	ULONG bold_directories;
	ULONG hilighted_sorting_column;

	/* inline edit stuff */
	LONG   edit_column_type;
	ULONG  edit_cursor_pos;

	/* drag'n'drop */
	LONG dropmark;
	
	/* store appwindow during dragevents */
	struct ipc_appwindow appwin;
};


static void doset(APTR obj, struct Data *data, struct TagItem *tags)
{
	FORTAG(tags)
	{
		case MUIA_List_TitleClick:
		{
			if(tag->ti_Data != LISTVIEW_FILE_COL_ICON && tag->ti_Data != LISTVIEW_DEVICE_COL_ICON)
			{
				data->sort_col = tag->ti_Data;

				if(data->prev_sort_col == data->sort_col)
				{
					data->sort_direction = -1 * data->sort_direction;
				}

				data->prev_sort_col = data->sort_col;

				/* give back sort state to listview class */
				SetAttrs(_parent(obj),
					MA_Listview_SortColumn, data->sort_col,
					MA_Listview_SortDirection, data->sort_direction,
					TAG_DONE
				);
			}

			break;
		}

		case MA_Listviewlist_Path:
		{
			data->path = (STRPTR)tag->ti_Data;
			break;
		}

		case MA_Icon_MimeType:
		{
			struct aline *selected;

			DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ULONG *)&selected);

			if(selected)
			{
				set(selected->obj, MA_Icon_MimeType, tag->ti_Data);
			}
			break;
		}

		case MA_Listviewlist_Type:
		{
			data->type = (ULONG)tag->ti_Data;
			break;
		}

		case MA_Listviewlist_IconDisplay:
		{
			data->icondisplay = tag->ti_Data;
			break;
		}

		case MA_Listview_SortDirection:
		{
			data->sort_direction = tag->ti_Data;
			break;
		}

		case MA_Listview_SortColumn:
		{
			data->sort_col = tag->ti_Data;
			break;
		}
	}
	NEXTTAG
}

DEFNEW
{
	static TEXT background[PATH_SIZE];
	ULONG bgmode = getprefslong(DSI_BACKGROUND_WINDOW_BGRENDER);
	STRPTR fontname = getprefsstr(DSI_FASTLIST_FONT);

	if(bgmode)
	{
		snprintf(background, sizeof(background), "5:%s", getprefsstr(DSI_BACKGROUND_WINDOW));
	}

	obj = DoSuperNew(cl, obj,
		MUIA_List_Format, LVFORMAT_FILES,
		MUIA_List_Editable, TRUE,
		MUIA_List_Title, TRUE,
		MUIA_List_DragType, 1,
		MUIA_List_ScrollerPos, MUIV_List_ScrollerPos_None,
		MUIA_Scrollgroup_UseWinBorder, TRUE,
		MUIA_Dropable, TRUE,
		MUIA_List_ShowDropMarks, FALSE,
		MUIA_List_PoolThreshSize, 16384,
		MUIA_List_PoolPuddleSize, 32768,
		MUIA_CycleChain, FALSE,
		getprefslong(DSI_FASTLIST_WINDOW_BG)?MUIA_Background:TAG_IGNORE, bgmode?background:getprefs(DSI_BACKGROUND_WINDOW_BGCOLOR),
		(fontname && *fontname)?MUIA_Font:TAG_IGNORE, _conf(fl_lister_font)->tf,
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		ULONG rc = TRUE;
		GETDATA;

		data->path = NULL;
		data->tempstorage  = NULL;
		data->tempstorage2 = NULL;

		data->type = FLT_UNKNOWN;
		data->sort_col = LISTVIEW_FILE_COL_NAME;
		data->prev_sort_col = LISTVIEW_FILE_COL_NAME;
		data->sort_direction = 1;

		data->dropmark = -1;

		data->edit_column_type = -1;
		data->edit_cursor_pos = 0;

		data->alternated_rows = getprefslong(DSI_FASTLIST_ALTERNATED_ROWS);
		data->hilighted_sorting_column = getprefslong(DSI_FASTLIST_HILIGHTED_SORTING_COLUMN);
		data->bold_directories = getprefslong(DSI_FASTLIST_BOLD_DIRECTORIES);
		data->appwin.window = NULL;
		
		set(obj, MUIA_String_MaxLen, 106); // huh? why that?

		DoMethod(obj, MUIM_Notify, MUIA_List_TitleClick, MUIV_EveryTime,
					obj, 2, MM_Listviewlist_EntryClick, MUIV_TriggerValue );

		DoMethod(obj, MUIM_Notify, MUIA_Listview_DoubleClick, MUIV_EveryTime,
				 obj, 2, MM_Listviewlist_DoubleClick, MUIV_TriggerValue);

		/*
		DoMethod(obj, MUIM_Notify, MUIA_Listview_AgainClick, MUIV_EveryTime,
				 obj, 1, MM_Rexx_Rename);
		*/
		/*
		DoMethod(obj, MUIM_Notify, MUIA_Listview_AgainClick, MUIV_EveryTime,
				 obj, 3, MUIM_Set, MUIA_List_Active,MUIV_List_Active_Off);
		*/

		DoMethod(obj, MUIM_Notify, MUIA_Listview_SelectChange, MUIV_EveryTime,
				 obj, 1, MM_Listviewlist_SelectChange);

		DoMethod(obj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
				 obj, 2, MM_Listview_ShowPreview, TRUE);

		data->titles_files = (STRPTR *) malloc(LVTITLESFILESNUM*sizeof(STRPTR));
		memset(data->titles_files, 0, LVTITLESFILESNUM*sizeof(STRPTR));
		if(data->titles_files)
		{
			int i;
			for(i=0; i<LVTITLESFILESNUM; i++)
			{
				data->titles_files[i] = (STRPTR) malloc(LISTTITLE_BUFFER_SIZE);

				if(!data->titles_files[i])
				{
					rc = FALSE;
				}
			}
		}
		else
		{
			rc = FALSE;
		}

		data->titles_devices = (STRPTR *) malloc(LVTITLESDEVICESNUM*sizeof(STRPTR));
		memset(data->titles_devices, 0, LVTITLESDEVICESNUM*sizeof(STRPTR));
		if(data->titles_devices)
		{
			int i;
			for(i=0; i<LVTITLESDEVICESNUM; i++)
			{
				data->titles_devices[i] = (STRPTR) malloc(LISTTITLE_BUFFER_SIZE);

				if(!data->titles_devices[i])
				{
					rc = FALSE;
				}
			}
		}
		else
		{
			rc = FALSE;
		}

		if(rc == FALSE)
		{
			CoerceMethod(cl, obj, OM_DISPOSE);
			return (0);
		}
	}
	return ((ULONG)obj);
}


DEFDISP
{
	GETDATA;
	int i;

	free(data->tempstorage);
	free(data->tempstorage2);

	for(i=0; i<LVTITLESFILESNUM; i++)
	{
		if(data->titles_files[i])
		{
			free(data->titles_files[i]);
		}
	}

	free(data->titles_files);

	for(i=0; i<LVTITLESDEVICESNUM; i++)
	{
		if(data->titles_devices[i])
		{
			free(data->titles_devices[i]);
		}
	}

	free(data->titles_devices);

	return (DOSUPER);
}


DEFSET
{
	GETDATA;

	doset(obj, data, INITTAGS);

	return (DOSUPER);
}


DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MUIA_List_TitleClick:
			return (TRUE);

		case MA_View_Path:
			*msg->opg_Storage = getv(_parent(obj), MA_View_Path);
			return (TRUE);

		case MA_Listviewlist_Path:
			*msg->opg_Storage = (ULONG)data->path;
			return (TRUE);

		case MA_Icon_Type:
		{
			struct aline *selected;

			DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ULONG *)&selected);

			if (!selected || !selected->obj)
				return (FALSE);

			*msg->opg_Storage = (ULONG) getv(selected->obj, MA_Icon_Type);
			return (TRUE);
		}

		case MA_Icon_FileType:
		{
			struct aline *selected;

			DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ULONG *)&selected);

			if (!selected || !selected->obj)
				return (FALSE);

			*msg->opg_Storage = (ULONG) getv(selected->obj, MA_Icon_FileType);
			return (TRUE);
		}

		case MA_Icon_Name:
		{
			struct aline *selected;

			DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ULONG *)&selected);

			if (!selected)
				return (FALSE);

			*msg->opg_Storage = (ULONG)selected->name;
			return (TRUE);
		}

		case MA_Icon_Path:
		{
			struct aline *selected;

			DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ULONG *)&selected);

			if (!selected || !selected->obj)
				return (FALSE);

			*msg->opg_Storage = (ULONG) getv(selected->obj, MA_Icon_Path);

			return (TRUE);
		}

		/* we should probably just mimic iconclass */
		case MA_Icon_PathInfo:
		{
			ULONG isdefault;
			struct aline *selected;
			STRPTR path;
			STRPTR p;

			DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ULONG *)&selected);

			if (!selected || !selected->obj)
				return (FALSE);

			path = (STRPTR) getv(selected->obj, MA_Icon_Path);

			if(path)
			{
				p = deficonpool_get_icon_name(getv(_parent(obj), MA_Viewgroup_ID), path, &isdefault);

				if(p)
				{
					if(data->tempstorage2)
						free(data->tempstorage2);
					data->tempstorage2 = p;
					*msg->opg_Storage = (ULONG)data->tempstorage2;
				}
				else
				{
					return (FALSE);
				}
			}
			else
			{
				return (FALSE);
			}

			return (TRUE);

			break;
		}

		case MA_DragDrop_Type:
			*msg->opg_Storage = MV_DragDrop_Type_Icon; //MV_DragDrop_Type_Iconview;
			return (TRUE);

		case MA_View_Type:
			*msg->opg_Storage = MV_View_Type_List;
			return (TRUE);

		case MA_DragDrop_Path:
			*msg->opg_Storage = getv(_win(obj), MA_Window_Path);
			return (TRUE);

		case MA_Iconview_Qualifier:
		{
			*msg->opg_Storage = getv(_parent(obj), MA_Iconview_Qualifier);
			return (TRUE);
		}

		case MA_View_IsViewObject:
			*msg->opg_Storage = FALSE;
			return (FALSE);

		case MA_View_HandleIcons:
			*msg->opg_Storage = getv(_parent(obj), MA_View_HandleIcons);
			return (TRUE);

		case MA_View_NumSelected:
			return (get(_parent(obj), msg->opg_AttrID, msg->opg_Storage));

		case MA_View_ShowDevices:
			*msg->opg_Storage = data->type == FLT_DEVICES ? TRUE : FALSE;
			return (TRUE);
	}
	return (DOSUPER);
}


DEFMMETHOD(List_Display)
{
	GETDATA;
	APTR parent = _parent(obj);

	if(data->type == FLT_DEVICES)
	{
		if (msg->entry)
		{
			struct aline *al = (struct aline *)msg->entry;
			ULONG color = 0;
			LONG pos = (LONG) msg->array[-1];

			if( data->alternated_rows && pos%2)
			{
				msg->array[ -9 ] = (STRPTR) 10;
			}

			if(data->hilighted_sorting_column)
			{
				msg->array[-7] = (STRPTR) data->sort_col;
			}

			switch(al->type)
			{
				case DLT_VOLUME:
					if(data->dropmark == pos && pos >= 0)
					{
						color = data->pen_drop_fg;
					}
					else
					{
						color = data->pen_volume_fg;
					}
					break;
				case DLT_DIRECTORY:
					if(data->dropmark == pos && pos >= 0)
					{
						color = data->pen_drop_fg;
					}
					else
					{
						color = data->pen_assign_fg;
					}
					break;
			}

			if(DoMethod(parent, MM_Listview_IsColumnVisible, LISTVIEW_DEVICE_COL_ICON))
			{
				static TEXT icon[32];

				if(data->icondisplay)
				{
					snprintf(icon, sizeof(icon), "\033O[%08lx]", DoMethod(parent, MM_Listview_FindIcon, al->obj, al->type));
				}
				else
				{
					icon[0] = ' ';
					icon[1] = '\0';
				}
				msg->array[ LISTVIEW_DEVICE_COL_ICON - LVFORMAT_DEVICES_BASE ] = icon;
			}

			if(DoMethod(parent, MM_Listview_IsColumnVisible, LISTVIEW_DEVICE_COL_VOLUME))
			{
				static TEXT name[VOLUME_SIZE + 64];

				snprintf(name, sizeof(name), "\033l\033P[%06lx]\033-%s", color, al->name);
				msg->array[ LISTVIEW_DEVICE_COL_VOLUME - LVFORMAT_DEVICES_BASE ] = name;
			}

			if(DoMethod(parent, MM_Listview_IsColumnVisible, LISTVIEW_DEVICE_COL_DEVICE))
			{
				static TEXT type[VOLUME_SIZE + 64];

				if(al->un.dev.devname && *al->un.dev.devname)
				{
					snprintf(type, sizeof(type), "\033l\033P[%06lx]\033-%s", color, al->un.dev.devname);
				}
				else
				{
					snprintf(type, sizeof(type), "\033l\033P[%06lx]\033-%s", color, GSI(MSG_LISTVIEWLISTCLASS_ASSIGN));
				}
				msg->array[ LISTVIEW_DEVICE_COL_DEVICE - LVFORMAT_DEVICES_BASE ] = type;
			}

			if(DoMethod(parent, MM_Listview_IsColumnVisible, LISTVIEW_DEVICE_COL_FREE))
			{
				static TEXT free[64];

				if(al->type == DLT_VOLUME)
				{
					TEXT t[32];
					capacity_format_size(t, sizeof(t), al->un.dev.total - al->un.dev.full);
					snprintf( free, sizeof(free), "\033r\033P[%06lx]%s", color, t);
				}
				else
				{
					free[0] = 0;
				}
				msg->array[ LISTVIEW_DEVICE_COL_FREE - LVFORMAT_DEVICES_BASE ] = free;
			}

			if(DoMethod(parent, MM_Listview_IsColumnVisible, LISTVIEW_DEVICE_COL_TOTAL))
			{
				TEXT size[64];
				static TEXT total[64];

				if(al->type == DLT_VOLUME)
				{
					capacity_format_size(size, sizeof(size), al->un.dev.total);
				}
				else
				{
					size[0] = 0;
				}

				snprintf( total, sizeof(total), "\033r\033P[%06lx]%s", color, size);

				msg->array[ LISTVIEW_DEVICE_COL_TOTAL - LVFORMAT_DEVICES_BASE ] = total;
			}

			if(DoMethod(parent, MM_Listview_IsColumnVisible, LISTVIEW_DEVICE_COL_DOSTYPE))
			{
				if(al->un.dev.dostype)
				{
					static TEXT dos[32];
					TEXT dostype[13], *dt = dostype;
					ULONG dtype = al->un.dev.dostype;
					int shift;

					for (shift = 24; shift >= 0; shift -= 8)
					{
						UBYTE val = dtype >> shift;

						if (isprint(val))
						{
							*dt++ = val;
						}
						else
						{
							*dt++ = '\\';
							*dt++ = __hex[val >> 4];
							*dt++ = __hex[val & 0xf];
						}
					}
					*dt = '\0';

					snprintf(dos, sizeof(dos), "\033P[%06lx][\033-%s]", color, dostype);
					msg->array[ LISTVIEW_DEVICE_COL_DOSTYPE - LVFORMAT_DEVICES_BASE ] = dos;
				}
				else
				{
					msg->array[ LISTVIEW_DEVICE_COL_DOSTYPE - LVFORMAT_DEVICES_BASE ] = "";
				}
			}
		}
		else
		{
			int i;
			STRPTR *array = msg->array;

			for (i = 0; i < LVTITLESDEVICESNUM; i++)
			{
				if( i == (LISTVIEW_DEVICE_COL_ICON - LVFORMAT_DEVICES_BASE))
				{
					strcpy( data->titles_devices[i], "\033\033\033I[6:23]" );
				}
				else
				{
					if(i == data->sort_col )
					{
						snprintf(data->titles_devices[i], LISTTITLE_BUFFER_SIZE, "\033P[%06lx]%s %s",  data->pen_column_fg, GSI( LVTITLES_DEVICES[i]) ,
								 (data->sort_direction > 0) ? "\033I[6:38]" : "\033I[6:39]");
					}
					else
					{
						snprintf(data->titles_devices[i], LISTTITLE_BUFFER_SIZE, "\033P[%06lx]%s",  data->pen_column_fg, GSI( LVTITLES_DEVICES[i]) );
					}
				}
				*array++ = data->titles_devices[i];
			}
		}
	}
	else
	{
		if (msg->entry)
		{
			struct aline *al = (struct aline *)msg->entry;
			ULONG color = 0;
			STRPTR style = "";
			LONG pos = (ULONG) msg->array[-1];

			if( data->alternated_rows && pos%2)
			{
				msg->array[ -9 ] = (STRPTR) 10;
			}

			if(data->hilighted_sorting_column)
			{
				msg->array[-7] = (STRPTR) data->sort_col;
			}

			switch(al->type)
			{
				case ST_USERDIR:
					if(data->dropmark == pos && pos >= 0)
					{
						color = data->pen_drop_fg;
					}
					else
					{
						color = al->active ? data->pen_directory_sel_fg : data->pen_directory_fg;
					}

					if(data->bold_directories)
					{
						style = "\033b";
					}
					break;
				case ST_FILE:
					color = al->active ? data->pen_file_sel_fg : data->pen_file_fg;
					style = "";
					break;
				case ST_LINKDIR:
				case ST_LINKFILE:
					color  = data->pen_hardlink_fg;
					style  = "\033i";
					break;
				case ST_SOFTLINK:
					color  = data->pen_softlink_fg;
					style  = "\033i";
					break;
			}

			if(DoMethod(parent, MM_Listview_IsColumnVisible, LISTVIEW_FILE_COL_ICON))
			{
				static TEXT icon[32];

				if(data->icondisplay)
				{
					snprintf(icon, sizeof(icon), "\033O[%08lx]", DoMethod(parent, MM_Listview_FindIcon, al->obj, al->type));
				}
				else
				{
					icon[0] = ' ';
					icon[1] = '\0';
				}
				msg->array[ LISTVIEW_FILE_COL_ICON ] = icon;
			}

			if(DoMethod(parent, MM_Listview_IsColumnVisible, LISTVIEW_FILE_COL_NAME))
			{
				static TEXT name[PATH_SIZE + 256];

				snprintf(name, sizeof(name), "%s\033P[%06lx]\033-%s", style, color, al->name);
				msg->array[ LISTVIEW_FILE_COL_NAME ] = name;
			}

			if(DoMethod(parent, MM_Listview_IsColumnVisible, LISTVIEW_FILE_COL_SIZE))
			{
				static TEXT size[64];
				switch(al->type)
				{
					case ST_USERDIR:
					{
						TEXT t[64];
						UQUAD *sizep;

						sizep = (UQUAD *) getv(al->obj, MA_Icon_FileSize);

						if(sizep)
						{
							if(_conf(fl_compact_size_display))
							{
								capacity_format_size(t, sizeof(t), *sizep);
							}
							else
							{
								if(*sizep)
								{
									capacity_format_size_separated(t, sizeof(t), *sizep);
								}
								else
								{
									strcpy(t, GSI( MSG_LISTVIEWLISTCLASS_EMPTY ));
								}
							}
						}
						else if(getv(parent, MA_Listview_GetSizes))
						{
							strcpy(t, "...");
						}
						else
						{
							strcpy(t, GSI( MSG_LISTVIEWLISTCLASS_DIRECTORY ));
						}
						snprintf( size, sizeof(size), "%s\033r\033P[%06lx]%s", style, color, t);
						break;
					}
					case ST_FILE:
					{
						TEXT t[64];

						if(al->un.file.size > 0)
						{
							if(_conf(fl_compact_size_display))
							{
								capacity_format_size(t, sizeof(t), al->un.file.size);
							}
							else
							{
								capacity_format_size_separated(t, sizeof(t), al->un.file.size);
							}
						}
						else
						{
							strcpy(t, GSI( MSG_LISTVIEWLISTCLASS_EMPTY ));
						}
						snprintf( size, sizeof(size), "%s\033r\033P[%06lx]%s", style, color, t);
						break;
					}
					case ST_LINKDIR:
					case ST_LINKFILE:
					case ST_SOFTLINK:
						snprintf( size, sizeof(size), "\033r\033P[%06lx]%s", color, GSI(MSG_LISTVIEWLISTCLASS_LINK));
						break;
				}
				msg->array[ LISTVIEW_FILE_COL_SIZE ] = size;
			}

			if(DoMethod(parent, MM_Listview_IsColumnVisible, LISTVIEW_FILE_COL_DATE))
			{
				static TEXT date[64];
				snprintf( date, sizeof(date), "%s\033P[%06lx]%s", style, color, al->date);
				msg->array[ LISTVIEW_FILE_COL_DATE ] = date;
			}

			if(DoMethod(parent, MM_Listview_IsColumnVisible, LISTVIEW_FILE_COL_FULLDATE))
			{
				static TEXT fulldate[64];
				snprintf( fulldate, sizeof(fulldate), "%s\033r\033P[%06lx]%s %s", style, color, al->date, al->time);
				msg->array[ LISTVIEW_FILE_COL_FULLDATE ] = fulldate;
			}

			if(DoMethod(parent, MM_Listview_IsColumnVisible, LISTVIEW_FILE_COL_ATTRS))
			{
				static TEXT protection[32];
				static TEXT prot[ 8 ];
				prot[ 0 ] = (al->un.file.protection & FIBF_SCRIPT ) ? 'S' : '-';
				prot[ 1 ] = (al->un.file.protection & FIBF_PURE ) ? 'P' : '-';
				prot[ 2 ] = (al->un.file.protection & FIBF_ARCHIVE ) ? 'A' : '-';
				prot[ 3 ] = !(al->un.file.protection & FIBF_READ ) ? 'R' : '-';
				prot[ 4 ] = !(al->un.file.protection & FIBF_WRITE ) ? 'W' : '-';
				prot[ 5 ] = !(al->un.file.protection & FIBF_EXECUTE ) ? 'E' : '-';
				prot[ 6 ] = !(al->un.file.protection & FIBF_DELETE ) ? 'D' : '-';
				prot[ 7 ] = 0;

				snprintf( protection, sizeof(protection), "%s\033P[%06lx]%s", style, color, prot);
				msg->array[ LISTVIEW_FILE_COL_ATTRS ] = protection;
			}

			if(DoMethod(parent, MM_Listview_IsColumnVisible, LISTVIEW_FILE_COL_COMMENT))
			{
				static TEXT comment[PATH_SIZE+256];
				if(al->un.file.target)
				{
					snprintf( comment, sizeof(comment), "\033P[%06lx]\033-%s", color, al->un.file.target);
				}
				else
				{
					snprintf( comment, sizeof(comment), "%s\033P[%06lx]\033-%s", style, color, al->un.file.comment ? al->un.file.comment : (STRPTR) "");
				}

				msg->array[ LISTVIEW_FILE_COL_COMMENT ] = comment;
			}

			if(DoMethod(parent, MM_Listview_IsColumnVisible, LISTVIEW_FILE_COL_FILETYPE))
			{
				static TEXT mimetype[128];
				APTR t = (APTR) getv(al->obj, MA_Icon_MimeType);

				snprintf( mimetype, sizeof(mimetype), "%s\033l\033P[%06lx]\033-%s", style, color, t?((struct internal_mimetype_node *)t)->description:(STRPTR) "");
				msg->array[ LISTVIEW_FILE_COL_FILETYPE ] = mimetype;
			}

			if(DoMethod(parent, MM_Listview_IsColumnVisible, LISTVIEW_FILE_COL_VERSION))
			{
				static TEXT version[64];
				TEXT tmp[64];
				STRPTR v = (STRPTR) getv(al->obj, MA_Icon_Version);

				if(!v)
				{
					if(getv(parent, MA_Listview_Version) && al->type != ST_USERDIR)
					{
						stccpy(tmp, "...",sizeof(tmp));
						v = tmp;
					}
					else
					{
						tmp[0] = '\0';
						v = tmp;
					}
				}

				snprintf( version, sizeof(version), "%s\033P[%06lx]\033-%s", style, color, v);
				msg->array[ LISTVIEW_FILE_COL_VERSION ] = version;
			}

			if(DoMethod(parent, MM_Listview_IsColumnVisible, LISTVIEW_FILE_COL_UID))
			{
				static TEXT uid[32];
				snprintf( uid, sizeof(uid), "%s\033P[%06lx]\033-%s", style, color, al->un.file.uid ? al->un.file.uid : (STRPTR) "nobody");
				msg->array[ LISTVIEW_FILE_COL_UID ] = uid;
			}

			if(DoMethod(parent, MM_Listview_IsColumnVisible, LISTVIEW_FILE_COL_GID))
			{
				static TEXT gid[32];
				snprintf( gid, sizeof(gid), "%s\033P[%06lx]\033-%s", style, color, al->un.file.gid ? al->un.file.gid : (STRPTR) "nogroup");
				msg->array[ LISTVIEW_FILE_COL_GID ] = gid;
			}

			if(DoMethod(parent, MM_Listview_IsColumnVisible, LISTVIEW_FILE_COL_MD5))
			{
				static TEXT md5[64];
				TEXT tmp[64];
				STRPTR t = (STRPTR) getv(al->obj, MA_Icon_MD5);

				if(!t)
				{
					if(getv(parent, MA_Listview_MD5) && al->type != ST_USERDIR)
					{
						stccpy(tmp, "...",sizeof(tmp));
						t = tmp;
					}
					else
					{
						tmp[0] = '\0';
						t = tmp;
					}
				}

				snprintf( md5, sizeof(md5), "%s\033l\033P[%06lx]%s", style, color, t);
				msg->array[ LISTVIEW_FILE_COL_MD5 ] = md5;
			}
		}
		else
		{
			int i;
			STRPTR * array = msg->array;

			for (i = 0; i < LVTITLESFILESNUM; i++)
			{
				if( i == LISTVIEW_FILE_COL_ICON)
				{
					//strcpy( data->titles_files[i], "\033\033\033I[4:PROGDIR:images/menu_dummy.mbr]" );
					strcpy( data->titles_files[i], "\033\033\033I[6:22]" );
				}
				else
				{
					if(i == data->sort_col )
					{
						snprintf(data->titles_files[i], LISTTITLE_BUFFER_SIZE, "\033P[%06lx]%s %s", data->pen_column_fg, GSI( LVTITLES_FILES[i]) ,
							     (data->sort_direction>0) ? "\033I[6:38]" : "\033I[6:39]");
					}
					else
					{
						snprintf(data->titles_files[i], LISTTITLE_BUFFER_SIZE, "\033P[%06lx]%s", data->pen_column_fg, GSI( LVTITLES_FILES[i]) );
					}

				}
				*array++ = data->titles_files[i];
			}
		}
	}
	return 0;
}


static LONG compareEntries( struct Data * data, struct aline *e1, struct aline *e2, LONG col, LONG rev UNUSED )
{
	if(data->type == FLT_DEVICES)
	{
		col += LVFORMAT_DEVICES_BASE;

		if ( col == LISTVIEW_DEVICE_COL_VOLUME )
		{
			/* column 0 - name */

			return StrnCmp(locale, e1->name, e2->name, -1, SC_ASCII);
		}
		else if ( col == LISTVIEW_DEVICE_COL_DEVICE )
		{
			/* column 1 - device name */
			if ( !e2->un.dev.devname && !e1->un.dev.devname  )
				return StrnCmp(locale, e1->name, e2->name, -1, SC_ASCII);
			else if ( !e2->un.dev.devname )
				return 1;
			else if ( !e1->un.dev.devname )
				return -1;
			else
				return Stricmp( e1->un.dev.devname, e2->un.dev.devname );
		}
		else if ( col == LISTVIEW_DEVICE_COL_FREE)
		{
			/* column 2 - free */

			if ( (e1->un.dev.total - e1->un.dev.full)  > (e2->un.dev.total - e2->un.dev.full) )
				return 1;
			else if ( (e1->un.dev.total- e1->un.dev.full ) < (e2->un.dev.total - e2->un.dev.full) )
				return -1;
			else
				return StrnCmp(locale, e1->name, e2->name, -1, SC_ASCII);
		}
		else if ( col == LISTVIEW_DEVICE_COL_TOTAL )
		{
			/* column 3 - total */

			if ( e1->un.dev.total > e2->un.dev.total )
				return 1;
			else if ( e1->un.dev.total < e2->un.dev.total )
				return -1;
			else
				return StrnCmp(locale, e1->name, e2->name, -1, SC_ASCII);
		}
		else if ( col == LISTVIEW_DEVICE_COL_DOSTYPE )
		{
			/* column 4 - dostype */

			if ( e1->un.dev.dostype > e2->un.dev.dostype )
				return 1;
			else if ( e1->un.dev.dostype < e2->un.dev.dostype )
				return -1;
			else
				return StrnCmp(locale, e1->name, e2->name, -1, SC_ASCII);
		}
		else if ( col == LISTVIEW_DEVICE_COL_ICON )
		{
			return 0;
		}
		else
		{
			return 0;
		}

	}
	else
	{
		if ( col == LISTVIEW_FILE_COL_NAME )
		{
			/* column 0 - name */

			return StrnCmp(locale, e1->name, e2->name, -1, SC_ASCII);
		}
		else if ( col == LISTVIEW_FILE_COL_SIZE )
		{
			/* column 1 - size */
			if(e1->type == ST_USERDIR)
			{
				UQUAD *sizep1, *sizep2;

				sizep1 = (UQUAD *) getv(e1->obj, MA_Icon_FileSize);
				sizep2 = (UQUAD *) getv(e2->obj, MA_Icon_FileSize);

				if(sizep1 && sizep2)
				{
					UQUAD size1, size2;

					size1 = *sizep1;
					size2 = *sizep2;

					if ( size1 > size2 )
						return 1;
					else if ( size1 < size2 )
						return -1;
					else
						return StrnCmp(locale, e1->name, e2->name, -1, SC_ASCII);
				}
				else
				{
					return StrnCmp(locale, e1->name, e2->name, -1, SC_ASCII);
				}
			}
			else
			{
				if ( e1->un.file.size > e2->un.file.size )
					return 1;
				else if ( e1->un.file.size < e2->un.file.size )
					return -1;
				else
					return StrnCmp(locale, e1->name, e2->name, -1, SC_ASCII);
			}
		}
		else if ( col == LISTVIEW_FILE_COL_DATE || col == LISTVIEW_FILE_COL_FULLDATE )
		{
			/* column 2 - date */

			if ( e1->un.file.date > e2->un.file.date )
				return 1;
			else if ( e1->un.file.date < e2->un.file.date )
				return -1;
			else
				return StrnCmp(locale, e1->name, e2->name, -1, SC_ASCII);
		}
		else if ( col == LISTVIEW_FILE_COL_ATTRS )
		{
			/* column 3 - protection bits */

			if ( e1->un.file.protection > e2->un.file.protection )
				return 1;
			else if ( e1->un.file.protection < e2->un.file.protection )
				return -1;
			else
				return StrnCmp(locale, e1->name, e2->name, -1, SC_ASCII);
		}
		else if ( col == LISTVIEW_FILE_COL_COMMENT )
		{
			/* column 4 - comment */

			if ( !e2->un.file.comment && !e1->un.file.comment  )
				return 0;
			else if ( !e2->un.file.comment )
				return 1;
			else if ( !e1->un.file.comment )
				return -1;
			else
				return StrnCmp(locale, e1->un.file.comment, e2->un.file.comment, -1, SC_ASCII);
		}
		else if ( col == LISTVIEW_FILE_COL_FILETYPE )
		{
			/* column 6 - filetype */
			APTR t1 = (APTR) getv(e1->obj, MA_Icon_MimeType);
			APTR t2 = (APTR) getv(e2->obj, MA_Icon_MimeType);
			STRPTR mimetype1 = t1?((struct internal_mimetype_node *) t1)->description:NULL;
			STRPTR mimetype2 = t2?((struct internal_mimetype_node *) t2)->description:NULL;
			int ret = 0;

			if ( !mimetype1 && !mimetype2  )
				return StrnCmp(locale, e1->name, e2->name, -1, SC_ASCII);
			else if ( !mimetype2 )
				return 1;
			else if ( !mimetype1 )
				return -1;
			else
			{
				ret = Stricmp( mimetype1, mimetype2 );
				
				if(ret)
					return ret;
				else
					return StrnCmp(locale, e1->name, e2->name, -1, SC_ASCII);
			}
		}
		else if ( col == LISTVIEW_FILE_COL_VERSION )
		{
			/* column 7 - version */
			STRPTR v1 = (APTR) getv(e1->obj, MA_Icon_Version);
			STRPTR v2 = (APTR) getv(e2->obj, MA_Icon_Version);
			int ret = 0;

			if ( !v1 && !v2  )
				return StrnCmp(locale, e1->name, e2->name, -1, SC_ASCII);
			else if ( !v2 )
				return 1;
			else if ( !v1 )
				return -1;
			else
			{
			    ret = Stricmp( v1, v2 );
				
				if(ret)
					return ret;
				else
					return StrnCmp(locale, e1->name, e2->name, -1, SC_ASCII);
			}
		}
		else if ( col == LISTVIEW_FILE_COL_UID)
		{
			/* column 9 - uid */

			if ( e1->un.file.uid > e2->un.file.uid )
				return 1;
			else if ( e1->un.file.uid < e2->un.file.uid )
				return -1;
			else
				return StrnCmp(locale, e1->name, e2->name, -1, SC_ASCII);
		}
		else if ( col == LISTVIEW_FILE_COL_GID)
		{
			/* column 9 - uid */

			if ( e1->un.file.gid > e2->un.file.gid )
				return 1;
			else if ( e1->un.file.gid < e2->un.file.gid )
				return -1;
			else
				return StrnCmp(locale, e1->name, e2->name, -1, SC_ASCII);
		}
		else if ( col == LISTVIEW_FILE_COL_MD5 )
		{
			/* column 7 - version */
			STRPTR v1 = (APTR) getv(e1->obj, MA_Icon_MD5);
			STRPTR v2 = (APTR) getv(e2->obj, MA_Icon_MD5);
			int ret = 0;

			if ( !v1 && !v2  )
				return StrnCmp(locale, e1->name, e2->name, -1, SC_ASCII);
			else if ( !v2 )
				return 1;
			else if ( !v1 )
				return -1;
			else
			{
			    ret = Stricmp( v1, v2 );
				
				if(ret)
					return ret;
				else
					return StrnCmp(locale, e1->name, e2->name, -1, SC_ASCII);
			}
		}
		else if ( col == LISTVIEW_FILE_COL_ICON )
		{
			return 0;
		}
		else
		{
			return 0;
		}
	}
}

DEFMMETHOD(List_Compare)
{
	GETDATA;

	struct aline *e1 = (struct aline *)msg->entry1;
	struct aline *e2 = (struct aline *)msg->entry2;


	/* get clicked col and reverse state*/
	LONG col = data->sort_col;
	LONG rev = data->sort_direction;

	LONG result;

	/* first check file<->directory */

	if(data->type == FLT_DEVICES)
	{
		if ((e1->type == DLT_VOLUME) && (e2->type == DLT_DIRECTORY))
			return -1;

		if ((e1->type == DLT_DIRECTORY) && (e2->type == DLT_VOLUME))
			return 1;

	}
	else
	{
		#if USE_LEGACY
		LONG type1 = (ULONG)e1->un.file.size == (ULONG)-1 ? ST_USERDIR : e1->type;	  /* in scandir.c we mark dirs with size of -1 */
		LONG type2 = (ULONG)e2->un.file.size == (ULONG)-1 ? ST_USERDIR : e2->type;
		#else
		LONG type1 = e1->un.file.size == NO_FILESIZE ? ST_USERDIR : e1->type;	  /* in scandir.c we mark dirs with size of NO_FILESIZE */
		LONG type2 = e2->un.file.size == NO_FILESIZE ? ST_USERDIR : e2->type;
		#endif

		if ((type1 == ST_USERDIR) && (type2 != ST_USERDIR))
			return -1;

		if ((type1 != ST_USERDIR) && (type2 == ST_USERDIR))
			return 1;
	}

	/* compare for each column */
	result = compareEntries( data, e1 , e2 , col , rev );

	result *= rev;

	return result;
}

DEFTMETHOD(Listviewlist_SelectChange)
{
	int i;
	int num = 0;
	int selected;
	struct aline * active;

	DoMethod(_view(obj), MM_View_IconSelect, MV_View_IconSelect_Clear, 0, FALSE);
	DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ULONG *)&active);

	for(i=0;;i++)
	{
		struct aline * al;
		DoMethod(obj, MUIM_List_GetEntry, i, &al);

		if(al)
		{
			DoMethod(obj, MUIM_List_Select, i, MUIV_List_Select_Ask, &selected);

			if(selected)
			{
				num++;
			}
			else if(active == al)
			{
				num++;
				selected = TRUE;
			}

			if(selected)
			{
				DoMethod(_view(obj), MM_View_IconSelect, MV_View_IconSelect_Select, getv(al->obj, MA_Icon_Type), FALSE);
			}

			if((selected && !al->active) || (!selected && al->active))
			{
				al->active = selected;

				DoMethod(obj, MUIM_List_Redraw, i);
			}
		}
		else
		{
			break;
		}
	}

	set(_parent(obj), MA_View_NumSelected, num);
	DoMethod(_parent(obj), MM_Listview_UpdateStatusBar);

	return 0;
}

DEFSMETHOD(Listviewlist_EntryClick)
{
	if(msg->pos == LISTVIEW_DEVICE_COL_ICON || msg->pos == LISTVIEW_FILE_COL_ICON)
	{
		DoMethod(_parent(obj), MM_Listview_UpdateIconsSize);
	}
	else
	{
		LONG activepos = getv(obj, MUIA_List_Active);
		struct aline * activeentry = NULL;

		/* Save active entry address */
		if(activepos != MUIV_List_Active_Off)
		{
			DoMethod(obj, MUIM_List_GetEntry, activepos, (ULONG *) &activeentry);
		}

		/* Sort */
		DoMethod(obj, MUIM_List_Sort);
		DoMethod(_parent(obj), MM_Listview_UpdateEntriesPositions);

		/* Restore active entry after sort (as selected, as setting active entry changes focus) */
		if(activeentry)
		{
			LONG pos, first=-1, state;
			struct aline * entry;

			for(pos=0;;pos++)
			{
				DoMethod(obj, MUIM_List_GetEntry, pos, (ULONG *) &entry);
				if (first == -1)
				{
					DoMethod(obj, MUIM_List_Select, pos, MUIV_List_Select_Ask, &state); // bitRocky
					if (state == MUIV_List_Select_On) first = pos;
				}

				if(entry == activeentry)
				{
					set(obj, MUIA_List_Active, MUIV_List_Active_Off);
					//DB(("Test: Select entry %ld, first = %ld\n", pos, first)); // bitRocky
					DoMethod(obj, MUIM_List_Select, pos, MUIV_List_Select_On, NULL);
					DoMethod(obj, MM_Listviewlist_SelectChange);
				}
				else if(!entry)
				{
					break;
				}
			}
			if (first > -1) DoMethod(obj, MUIM_List_Jump, first); // bitRocky: make the first selected entry visible after col-sort
		}
	}

	return 0;
}

DEFSMETHOD(Listviewlist_DoubleClick)
{
	GETDATA;

	UBYTE buf[PATH_SIZE + 256];
	struct aline *al;
	STRPTR mode = NULL;
	struct viewnode * vn = viewapi_findbyname("List");

	if(vn && data->type == FLT_FILES)
	{
		mode = viewapi_getmodename(vn, getv(_parent(obj), MA_View_ModeIndex));
	}

	DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &al);

	if(al && al->obj)
	{
		ULONG isdefault = FALSE;
		ULONG defaulttool = FALSE;
		STRPTR p;
		ULONG type = getv(al->obj, MA_Icon_Type);

		if(type != MV_Icon_Type_Device && type != MV_Icon_Type_Drawer)
		{
			ULONG viewid = getv(_parent(obj), MA_Viewgroup_ID);
			/* Check if icon has a default tool, sigh */
			p = deficonpool_get_icon_name(viewid, (STRPTR) getv(al->obj, MA_Icon_Path), &isdefault);

			if(p)
			{
				APTR iconobj;

				if ((iconobj = (APTR) DoMethod(app, MM_Application_CreateIcon, FALSE, viewid)))
				{
					if(icon_read(p, iconobj, ICONTAG_Deficon, TRUE, TAG_DONE))
					{
						STRPTR tool = (STRPTR) getv(iconobj, MA_Icon_DefaultTool);
						defaulttool = tool != NULL && *tool;
					}

					DoMethod(app, MM_Application_DisposeObject, iconobj);
				}

				free(p);
			}
		}

		if(!defaulttool || isdefault)
		{
			APTR dispatcher = (APTR)DoMethod( app, MM_Application_CreateActionDispatcher );

			if ( dispatcher )
			{
				SetAttrs(dispatcher,
					MA_ActionDispatcher_IQualifier, getv( _parent(obj), MA_Iconview_Qualifier ),  /* XXX: fix */
					MA_ActionDispatcher_Event, ACTION_EVENT_DOUBLECLICK,
					MA_ActionDispatcher_SrcURI, getv( obj , MA_Listviewlist_Path ),
					MA_ActionDispatcher_SrcID, getv( _win( obj ), MA_Window_ID ),
					MA_ActionDispatcher_RefWin, _win( obj ),
					TAG_DONE
				);

				{
					STRPTR path = (STRPTR) getv(al->obj, MA_Icon_Path);
					UBYTE encpath[mimeuri_encodepath(NULL, 0, path)];

					mimeuri_encodepath(encpath, sizeof(encpath), path);

					snprintf(buf, sizeof(buf), "file:///%s?view=List", encpath);
					if(mode)
					{
						strncat(buf, "&mode=", sizeof(buf));
						strncat(buf, mode, sizeof(buf));
					}
				}

				DoMethod( dispatcher, MM_ActionDispatcher_AddURI, buf, TRUE );
				DoMethod( dispatcher, MM_ActionDispatcher_Execute );
			}
		}
		else
		{
			/*  XXX: what if the path contains quotes itself?
			 */
			STRPTR path = (STRPTR) getv(al->obj, MA_Icon_Path);

			if(name_isinfo(path))
			{
				STRPTR pathnoinfo = name_build_noinfo(path);
				snprintf(buf, sizeof(buf), "Run \"%s\"", pathnoinfo ? pathnoinfo : (STRPTR) "");
				name_delete(pathnoinfo);
			}
			else
			{
				snprintf(buf, sizeof(buf), "Run \"%s\"", path);
			}

			execute_command(obj, AC_INTERNAL , buf, NULL);
		}
	}

	return 0;
}

DEFSMETHOD(View_GetSelectionList)
{
	LONG pos = MUIV_List_NextSelected_Start;
	struct dragdropnode *ddn;

	for(pos=0;;pos++)
	{
		struct aline *entry;

		DoMethod(obj, MUIM_List_GetEntry, pos, (ULONG *)&entry);

		if (entry && entry->obj && entry->active)
		{
			STRPTR path = (STRPTR) getv(entry->obj, MA_Icon_Path);

			if(path)
			{
				ULONG pathlen = strlen(path)+1;

				if ((ddn = malloc(sizeof(*ddn) + pathlen )))
				{
					strcpy(ddn->path, path);

					ddn->type = getv(entry->obj, MA_Icon_FileType);
					ddn->x = 0;
					ddn->y = 0;

					ADDTAIL(msg->list, ddn);
				}
			}
		}	 
		else if(!entry)
		{
			break;
		}
	}

	return 0;
}

DEFMMETHOD(DragQuery)
{
	LONG ft = getv(msg->obj, MA_Icon_FileType);

	if (ft == MV_Icon_FileType_File || ft == MV_Icon_FileType_Directory || ft == MV_Icon_FileType_Device)
	{
		return (MUIV_DragQuery_Accept);
	}
	return (MUIV_DragQuery_Refuse);
}

DEFMMETHOD(DragDrop)
{
	GETDATA;
	UBYTE path[PATH_SIZE];
	struct aline *entry = NULL;

	stccpy(path, data->path, sizeof(path));

	/* get drop entry, if any, and redraw it normally */
	if(data->dropmark != -1)
	{
		ULONG i;
		DoMethod(obj, MUIM_List_GetEntry, data->dropmark, &entry);

		i = data->dropmark;
		data->dropmark = -1;

		DoMethod(obj, MUIM_List_Redraw, i);
	}

	/* avoid action when dropping on source list (when it's not a dir) */
	if((msg->obj == obj) && ((entry && (entry->type != ST_USERDIR && entry->type != DLT_VOLUME && entry->type != DLT_DIRECTORY )) || entry == NULL) )
	{
		return (0);
	}

	/* if there's a drop entry and it's a dir */
	if (entry)
	{
		if(data->type == FLT_FILES)
		{
			if(entry->type == ST_USERDIR)
			{
				AddPart(path, entry->name, sizeof(path));
			}
		}
		else if(entry->type == DLT_VOLUME || entry->type == DLT_DIRECTORY)
		{
			AddPart(path, entry->name, sizeof(path));
			strncat(path, ":", sizeof(path));
		}
	}
	else if(data->type == FLT_DEVICES)
	{
		if (getv(_parent(msg->obj), MA_View_NumSelected) <= 1)
		{
			STRPTR filepath = (STRPTR)getv(msg->obj, MA_Icon_Path);

			if(filepath)
			{
				TEXT buffer[PATH_SIZE+64];
				UBYTE encpath[mimeuri_encodepath(NULL, 0, filepath)];

				mimeuri_encodepath(encpath, sizeof(encpath), filepath);
				snprintf(buffer, sizeof(buffer), "LoadURI \"file://%s?view=list\" VIEWID=%ld", encpath, getv(_win(obj), MA_Window_ID));

				execute_command(NULL, AC_INTERNAL, buffer, NULL);
			}
		}

		return 0;
	}

	if (getv(_parent(msg->obj), MA_View_NumSelected) > 1) /* multiple drop */
	{
		struct MinList ml;
		APTR dispatcher = (APTR)DoMethod( app, MM_Application_CreateActionDispatcher );

		if ( dispatcher )
		{
			struct dragdropnode *ddn, *nextddn;

			SetAttrs( dispatcher,
				MA_ActionDispatcher_IQualifier, getv(msg->obj, MA_Iconview_Qualifier),
				MA_ActionDispatcher_Event, ACTION_EVENT_DRAGNDROP,
				MA_ActionDispatcher_DstURI, path,
				MA_ActionDispatcher_DstID, getv(_win(obj), MA_Window_ID),
				MA_ActionDispatcher_SrcURI, getv(_win(msg->obj), MA_Window_Path),
				MA_ActionDispatcher_RefWin, _win(msg->obj),
				TAG_DONE
			);

			NEWLIST(&ml);
			DoMethod(msg->obj, MM_View_GetSelectionList, &ml);

			ITERATELISTSAFE(ddn, nextddn, &ml)
			{
				ULONG skip = isdir(ddn->path) && is_path_contained(path, ddn->path);

				if(!skip)
				{
					DoMethod( dispatcher, MM_ActionDispatcher_AddURI, ddn->path, TRUE );
				}
				else
				{
					SDB(("Recursive drop, skipping\n"));
				}
				free(ddn);
			}

			DoMethod( dispatcher, MM_ActionDispatcher_Execute );
		}
	}
	else
	{
		LONG filetype;

		filetype = getv(msg->obj, MA_Icon_FileType);
		if (filetype == MV_Icon_FileType_Directory || filetype == MV_Icon_FileType_File) /* XXX: and devices too, yep */
		{
			STRPTR filepath = (STRPTR)getv(msg->obj, MA_Icon_Path);
			ULONG skip = isdir(filepath) && is_path_contained(path, filepath);

			if(!skip)
			{
				APTR dispatcher = (APTR)DoMethod( app, MM_Application_CreateActionDispatcher );

				if ( dispatcher )
				{
					SetAttrs(dispatcher,
						MA_ActionDispatcher_IQualifier, getv( _parent( msg->obj ) , MA_Iconview_Qualifier ),
						MA_ActionDispatcher_Event, ACTION_EVENT_DRAGNDROP,
						MA_ActionDispatcher_DstURI, path,
						MA_ActionDispatcher_DstID, getv( _win(obj), MA_Window_ID ),
						MA_ActionDispatcher_SrcURI, getv( _win(msg->obj), MA_Window_Path ),
						MA_ActionDispatcher_RefWin, _win(msg->obj),
						TAG_DONE
					);

					DoMethod( dispatcher, MM_ActionDispatcher_AddURI, filepath, TRUE );
					DoMethod( dispatcher, MM_ActionDispatcher_Execute );
				}
			}
			else
			{
				SDB(("Recursive drop, skipping\n"));
			}
		}
		#if 0
		// This looks so wrong... -itix
		else if (filetype == MV_Icon_FileType_None)
		{
			do_action(NULL, TA_File_Move,
				TT_File_Move_SrcPath, (STRPTR)getv(msg->obj, MA_Icon_PathInfo),
				TT_File_Move_DstPath, path,
				TT_File_Move_Refwin, _win(obj),
				(msg->mode == MV_DragDrop_Drop_Copy) ? TT_File_Move_Copy : TAG_IGNORE, TRUE,
				TT_File_Move_NoIcon, TRUE,
			TAG_DONE);
		}
		#endif
	}

	return (0);
}

DEFMMETHOD(DragReport)
{
	GETDATA;
	struct MUI_List_TestPos_Result res;

	/* Autoscroll list, when mouse is in upper or lower area */
	if(_isinobject(msg->x, msg->y))
	{
		if( msg->y + 10 > _top(obj) + _height(obj))
		{
			if(!msg->update)
			{
				return MUIV_DragReport_Refresh;
			}

			set(obj, MUIA_List_TopPixel, getv(obj, MUIA_List_TopPixel) + 10);
			return (MUIV_DragReport_Continue);
		}
		else if(_top(obj) + 20 > msg->y)
		{
			if(getv(obj, MUIA_List_TopPixel) >= 10)
			{
				if(!msg->update)
				{
					return MUIV_DragReport_Refresh;
				}

				set(obj, MUIA_List_TopPixel, getv(obj, MUIA_List_TopPixel) - 10);
				return (MUIV_DragReport_Continue);
			}
		}
	}
	else
	{
		return MUIV_DragReport_Continue;
	}

	DoMethod(obj, MUIM_List_TestPos, msg->x, msg->y, &res);

	if(res.entry != -1 && res.column == DoMethod(_parent(obj), MM_Listview_GetColumnOrder, (data->type == FLT_FILES) ? LISTVIEW_FILE_COL_NAME : LISTVIEW_DEVICE_COL_VOLUME))
	{
		if (res.entry == data->dropmark)
			return MUIV_DragReport_Continue;

		if(!msg->update)
		{
			return MUIV_DragReport_Refresh;
		}

		if(data->dropmark != -1)
		{
			int entry = data->dropmark;
			data->dropmark = - 1;
			DoMethod(obj, MUIM_List_Redraw, entry);
		}

		data->dropmark = res.entry;
		DoMethod(obj, MUIM_List_Redraw, res.entry);
	}
	else
	{
		if(data->dropmark != -1)
		{
			if(!msg->update)
			{
				return MUIV_DragReport_Refresh;
			}
			else
			{
				int entry = data->dropmark;
				data->dropmark = - 1;
				DoMethod(obj, MUIM_List_Redraw, entry);
			}		 
		}
	}

	return MUIV_DragReport_Continue;
}

DEFMMETHOD(DragBegin)
{
	return 0;
}

DEFMMETHOD(DragFinish)
{
	GETDATA;

	if(!msg->dropfollows) /* drop was not for this view, reset dropmark */
	{
		data->dropmark = -1;
	}

	DoMethod(obj, MUIM_List_Redraw, MUIV_List_Redraw_All);

	return 0;
}

DEFSMETHOD(View_Select)
{
	GETDATA;

	if(data->type == FLT_DEVICES)
		return (0);

	switch (msg->mode)
	{
		case MV_View_Select_Invert:
			set(obj, MUIA_List_Active, MUIV_List_Active_Off);
			DoMethod(obj, MUIM_List_Select, MUIV_List_Select_All, MUIV_List_Select_Toggle, NULL);
			break;

		case MV_View_Select_All:
			DoMethod(obj, MUIM_List_Select, MUIV_List_Select_All, MUIV_List_Select_On, NULL);
			break;

		case MV_View_Select_None:
			DoMethod(obj, MUIM_List_Select, MUIV_List_Select_All, MUIV_List_Select_Off, NULL);
			set(obj, MUIA_List_Active, MUIV_List_Active_Off);
			break;

        case MV_View_Select_Files:
        case MV_View_Select_Dirs:
        {
            int i;
            int num = 0;
			int selectfiles = msg->mode == MV_View_Select_Files;

            set(obj, MUIA_List_Active, MUIV_List_Active_Off);
            DoMethod(obj, MUIM_List_Select, MUIV_List_Select_All, MUIV_List_Select_Off, NULL);

            for(i=0;;i++)
            {
                struct aline * al;
                DoMethod(obj, MUIM_List_GetEntry, i, &al);

                if(al)
                {
                    int select = al->type == ST_USERDIR;

                    if(selectfiles)
                    {
						select = !select;
                    }

                    if (select)
                    {
						al->active = TRUE;
						num++;

						DoMethod(obj, MUIM_List_Select, i, MUIV_List_Select_On, NULL);
						DoMethod(obj, MUIM_List_Redraw, i);
                    }
                }
                else
                {
					break;
                }
            }

            set(_parent(obj), MA_View_NumSelected, num);
            break;
        }

		case MV_View_Select_Pattern:
		case MV_View_Select_Name:
		{
			STRPTR parsepat = NULL;
			ULONG patsize = strlen(msg->pattern) * 2 + 2;

			if ((parsepat = malloc(patsize)))
			{
				if ((ParsePatternNoCase(msg->pattern, parsepat, patsize) > -1))
				{
					int i;
					int num = 0;

					set(obj, MUIA_List_Active, MUIV_List_Active_Off);
					if (msg->mode == MV_View_Select_Pattern)
					{
						DoMethod(obj, MUIM_List_Select, MUIV_List_Select_All, MUIV_List_Select_Off, NULL);
					}

					for(i=0;;i++)
					{
						struct aline * al;
						DoMethod(obj, MUIM_List_GetEntry, i, &al);

						if(al)
						{
							if (MatchPatternNoCase(parsepat, al->name))
							{
								al->active = TRUE;
								num++;

								DoMethod(obj, MUIM_List_Select, i, MUIV_List_Select_On, NULL);
								DoMethod(obj, MUIM_List_Redraw, i);
							}
						}
						else
						{
							break;
						}
					}

					set(_parent(obj), MA_View_NumSelected, num);
				}

				free(parsepat);
			}
			break;
		}

	}

	DoMethod(obj, MM_Listviewlist_SelectChange);

	return (0);
}//MM_View_Select


DEFMMETHOD(Setup)
{
	ULONG rc;

	if (( rc = (DOSUPER && _win(obj)) ))
	{
		DoMethod(obj, MM_Listview_AllocPens);
	}
	return (rc);
}


DEFMMETHOD(DragEvent)
{
	GETDATA;
	// objwindow is a temporary ptr, unless msg->obj is non-NULL
	// in our case, we just want to find out if the window is an appwindow
	// in case the msg->obj is NULL (meaning the win doesn't belong to us)
	// do not that we might get one of our app's windows here too

	if (msg->objwindow && !msg->obj)
	{
		struct ipc_appwindow *appwin;
		if ( (data->appwin.window  != msg->objwindow) )
		{
	 		if ( (appwin = (APTR)AppWindowObtain(msg->objwindow)) )
			{
				data->appwin = *appwin;
				// use a normal mouse ptr, means we can drop stuff
				msg->mouseptrtype = POINTERTYPE_NORMAL;
				// tell MUI we can drop stuff and that we've changed the mouse ptr
				msg->flags |= MUIF_DRAGEVENT_FOREIGNDROP | MUIF_DRAGEVENT_MOUSECHANGED;
			}
			else
			{
				if((data->appwin.message_types  & AM_CLASS_MOUSEEXIT) && (data->appwin.window ))
				{
					struct wbargs *wba;
					if ( (wba = wba_create(obj)) )
					{
						do_action(app, TA_AppMsg_Send,/* XXX: I *think* 'app' is ok here.. check */
							TT_AppMsg_Send_Type,      AMTYPE_APPWINDOW,
							TT_AppMsg_Send_Window,    data->appwin.window,
							TT_AppMsg_Send_Path,      wba->basepath,
							TT_AppMsg_Send_ID,        data->appwin.id,
							TT_AppMsg_Send_Userdata,  data->appwin.userdata,
							TT_AppMsg_Send_NumArgs,   wba->count,
							TT_AppMsg_Send_WBArgList, wba->wba,
							TT_AppMsg_Send_Class,     AM_CLASS_MOUSEEXIT,
							TT_AppMsg_Send_MouseX,    data->appwin.window->WScreen->MouseX,
							TT_AppMsg_Send_MouseY,    data->appwin.window->WScreen->MouseY,
							TAG_DONE);

						wba_delete(wba);
					}
					
				}

				data->appwin.window = NULL;
				data->appwin.message_types = 0;
			}

			if((data->appwin.message_types & AM_CLASS_MOUSEENTER) && (data->appwin.window ))
			{
				struct wbargs *wba;
				if ( (wba = wba_create(obj)) )
				{
					do_action(app, TA_AppMsg_Send,/* XXX: I *think* 'app' is ok here.. check */
						TT_AppMsg_Send_Type,      AMTYPE_APPWINDOW,
						TT_AppMsg_Send_Window,    data->appwin.window,
						TT_AppMsg_Send_Path,      wba->basepath,
						TT_AppMsg_Send_ID,        data->appwin.id,
						TT_AppMsg_Send_Userdata,  data->appwin.userdata,
						TT_AppMsg_Send_NumArgs,   wba->count,
						TT_AppMsg_Send_WBArgList, wba->wba,
						TT_AppMsg_Send_Class,     AM_CLASS_MOUSEENTER,
						TT_AppMsg_Send_MouseX,    data->appwin.window->WScreen->MouseX,
						TT_AppMsg_Send_MouseY,    data->appwin.window->WScreen->MouseY,
					TAG_DONE);
					wba_delete(wba);
				}
			}

			AppWindowRelease();
		}
		else if ( (data->appwin.window ))
		{
			if (data->appwin.message_types & AM_CLASS_MOUSEMOVE)
			{
				struct wbargs *wba;
				if ( (wba = wba_create(obj)) )
				{
					do_action(app, TA_AppMsg_Send,/* XXX: I *think* 'app' is ok here.. check */
						TT_AppMsg_Send_Type,      AMTYPE_APPWINDOW,
						TT_AppMsg_Send_Window,    data->appwin.window,
						TT_AppMsg_Send_Path,      wba->basepath,
						TT_AppMsg_Send_ID,        data->appwin.id,
						TT_AppMsg_Send_Userdata,  data->appwin.userdata,
						TT_AppMsg_Send_NumArgs,   wba->count,
						TT_AppMsg_Send_WBArgList, wba->wba,
						TT_AppMsg_Send_Class,     AM_CLASS_MOUSEMOVE,
						TT_AppMsg_Send_MouseX,    data->appwin.window->WScreen->MouseX,
						TT_AppMsg_Send_MouseY,    data->appwin.window->WScreen->MouseY,
					TAG_DONE);
					wba_delete(wba);
				}
			}

			// keep telling MUI to show the 'drop ok' pointer
			msg->mouseptrtype = POINTERTYPE_NORMAL;
			msg->flags |= MUIF_DRAGEVENT_FOREIGNDROP | MUIF_DRAGEVENT_MOUSECHANGED;
		}
	}
	else if (data->appwin.window )
	{
		if(data->appwin.message_types  & AM_CLASS_MOUSEEXIT)
		{
			struct wbargs *wba;
			if ( (wba = wba_create(obj)) )
			{
				do_action(app, TA_AppMsg_Send,/* XXX: I *think* 'app' is ok here.. check */
					TT_AppMsg_Send_Type,      AMTYPE_APPWINDOW,
					TT_AppMsg_Send_Window,    data->appwin.window,
					TT_AppMsg_Send_Path,      wba->basepath,
					TT_AppMsg_Send_ID,        data->appwin.id,
					TT_AppMsg_Send_Userdata,  data->appwin.userdata,
					TT_AppMsg_Send_NumArgs,   wba->count,
					TT_AppMsg_Send_WBArgList, wba->wba,
					TT_AppMsg_Send_Class,     AM_CLASS_MOUSEEXIT,
					TT_AppMsg_Send_MouseX,    data->appwin.window->WScreen->MouseX,
					TT_AppMsg_Send_MouseY,    data->appwin.window->WScreen->MouseY,
					TAG_DONE);
				wba_delete(wba);
			}
		}
		data->appwin.window = NULL;
	}

	return (0);
}

/* Allocate lister pens */
DEFTMETHOD(Listview_AllocPens)
{
	GETDATA;
	data->pen_file_fg          = gfx_get_penspec_value(muiRenderInfo(obj), &_conf(fl_color_file_fg));
	data->pen_file_sel_fg      = gfx_get_penspec_value(muiRenderInfo(obj), &_conf(fl_color_file_sel_fg));

	data->pen_directory_fg     = gfx_get_penspec_value(muiRenderInfo(obj), &_conf(fl_color_directory_fg));
	data->pen_directory_sel_fg = gfx_get_penspec_value(muiRenderInfo(obj), &_conf(fl_color_directory_sel_fg));

	data->pen_softlink_fg      = gfx_get_penspec_value(muiRenderInfo(obj), &_conf(fl_color_softlink_fg));
	data->pen_hardlink_fg      = gfx_get_penspec_value(muiRenderInfo(obj), &_conf(fl_color_hardlink_fg));

	data->pen_assign_fg        = gfx_get_penspec_value(muiRenderInfo(obj), &_conf(fl_color_assign_fg));
	data->pen_volume_fg        = gfx_get_penspec_value(muiRenderInfo(obj), &_conf(fl_color_volume_fg));

	data->pen_column_fg        = gfx_get_penspec_value(muiRenderInfo(obj), &_conf(fl_color_column_fg));

	data->pen_drop_fg          = gfx_get_penspec_value(muiRenderInfo(obj), &_conf(dragdrop_tintval));

	return (0);
}

#if 0 
DEFTMETHOD(Rexx_Rename)
{
	GETDATA;
	struct MUI_List_TestPos_Result res;
	struct Window * win = (struct Window *) getv(_win(obj), MUIA_Window_Window);
	LONG x = win->MouseX, y = win->MouseY;

	DoMethod(obj, MUIM_List_TestPos, x, y, &res);

	if ( _isinobject(x, y))
	{
		if(res.entry != -1 )
		{
			APTR parent = _parent(obj);
			ssize_t col = DoMethod(parent, MM_Listview_GetColumnType, res.column);

			set( obj, MUIA_List_Active, res.entry );
			data->edit_cursor_pos = 0; /* we reset cursor pos when using middle button */

			switch (col)
			{
				case LISTVIEW_FILE_COL_NAME:
				case LISTVIEW_FILE_COL_DATE:
				case LISTVIEW_FILE_COL_ATTRS:
				case LISTVIEW_FILE_COL_FULLDATE:
				case LISTVIEW_DEVICE_COL_VOLUME:
					data->edit_column_type = col;
					DoMethod(app, MUIM_Application_PushMethod, obj, 3, MUIM_List_Edit, MUIV_List_EditEntry_Active, res.column);
					break;
			}
		}
	}
	return (0);
}
#else // bitRocky
DEFTMETHOD(Rexx_Rename)
{
	GETDATA;
	APTR entry;

	DoMethod( obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &entry );
	if ( entry )
	{
		data->edit_cursor_pos = 0; /* we reset cursor pos when using middle button */
		data->edit_column_type = LISTVIEW_FILE_COL_NAME;
		DoMethod(app, MUIM_Application_PushMethod, obj, 3, MUIM_List_Edit, MUIV_List_EditEntry_Active, LISTVIEW_FILE_COL_NAME);
	}
	return (0);
}
#endif


DEFMMETHOD(List_Edit)
{
	GETDATA;
	ssize_t col = msg->column;

	if (data->type == FLT_DEVICES)
		col += LVFORMAT_DEVICES_BASE;

	switch (col)
	{
		case LISTVIEW_FILE_COL_NAME:
		case LISTVIEW_FILE_COL_DATE:
		case LISTVIEW_FILE_COL_ATTRS:
		case LISTVIEW_FILE_COL_COMMENT:
		case LISTVIEW_FILE_COL_FULLDATE:
		case LISTVIEW_DEVICE_COL_VOLUME:
			data->edit_column_type = col;
			if (DOSUPER)
			{
				set(_parent(obj), MA_Listview_InEditMode, TRUE); // bitRocky
				return (TRUE);
			}
			else {
				return (FALSE);
			}
			//return DOSUPER;
	}

	return 0;
}


#define SHORT_DATEFORMAT "%d-%m-%Y"
#define FULL_DATEFORMAT  "%d-%m-%Y %H:%M:%S"

DEFMMETHOD(List_CreateEditObject)
{
	GETDATA;
	struct aline *selected;
	STRPTR buf = NULL;
	TEXT tmpbuf[64];
	ssize_t col = msg->column;
	APTR o;

	if (data->type == FLT_DEVICES)
		col += LVFORMAT_DEVICES_BASE;

	selected = (struct aline *)msg->entry;

	switch (col)
	{
		case LISTVIEW_FILE_COL_DATE:
			CreateDateString(SHORT_DATEFORMAT, &selected->datestamp, tmpbuf);
			buf = name_build(tmpbuf);
			break;

		case LISTVIEW_FILE_COL_FULLDATE:
			CreateDateString(FULL_DATEFORMAT, &selected->datestamp, tmpbuf);
			buf = name_build(tmpbuf);
			break;

		case LISTVIEW_FILE_COL_ATTRS:
			return (size_t)FSProtectionBitsObject,
				MUIA_FSProtectionBits_Flags, selected->un.file.protection,
				TAG_DONE);

		case LISTVIEW_DEVICE_COL_VOLUME:
		case LISTVIEW_FILE_COL_NAME:
			if (data->type != FLT_DEVICES || selected->type != DLT_DIRECTORY)
				buf = name_build(selected->name);
			break;

		case LISTVIEW_FILE_COL_COMMENT:
			if (selected->un.file.comment)
				buf = name_build(selected->un.file.comment);
			else
				buf = name_build("");
			break;

		default:
			break;
	}

	o = NULL;

	if (buf)
	{
		switch (col)
		{
			case LISTVIEW_DEVICE_COL_VOLUME:
			case LISTVIEW_FILE_COL_NAME:
			{
				STRPTR str;
				ULONG n;

				/* we search for the last '.' in file name and calculate number of chars to mark */

				if (col == LISTVIEW_FILE_COL_NAME && (str = strrchr(buf, '.')))
				{
					n = ((STRPTR) str) - ((STRPTR) buf) - 1;
				} else {
					n = strlen(buf);
				}

				o = StringObject,
					MUIA_String_MaxLen, NAME_SIZE,
					MUIA_String_Reject, ":/",
					MUIA_String_Contents, buf,
					StringFrame,

					/* Small Textinput bug workaround. This must be after MUIA_String_Contents. */
					MUIA_Textinput_MarkStart, 0,
					MUIA_Textinput_MarkEnd, n,
					MUIA_Textinput_CursorPos, 0,
					MUIA_Textinput_ResetMarkOnCursor, TRUE,
					TAG_DONE);
			}
			break;

			case LISTVIEW_FILE_COL_DATE:
			case LISTVIEW_FILE_COL_FULLDATE:
			{
				o = StringObject,
					MUIA_String_Accept, "01234567890-: ",
					MUIA_String_Contents, buf,
					MUIA_Textinput_CursorPos, 0,
					StringFrame,
					TAG_DONE);
			}
			break;

			case LISTVIEW_FILE_COL_COMMENT:
			{
				o = StringObject,
					MUIA_String_Contents, buf,
					MUIA_Textinput_CursorPos, 0,
					StringFrame,
					TAG_DONE);
			}
			break;
		}

		if (o)
		{
			/* XXX: doesn't work very well, refresh issue with cursor */
			// DoMethod(app, MUIM_Application_PushMethod, o, 3, MUIM_Set, MUIA_String_BufferPos, data->edit_cursor_pos);
		}
	}

	return (size_t)o;
}


static void SetDate(CONST_STRPTR path, CONST_STRPTR ptr, const struct aline *selected)
{
	struct DateStamp ds;

	if (ParseDateString(SHORT_DATEFORMAT, &ds, ptr))
	{
		if (ds.ds_Days != selected->datestamp.ds_Days)
		{
			ds.ds_Minute = selected->datestamp.ds_Minute;
			ds.ds_Tick = selected->datestamp.ds_Tick;

			if (SetFileDate(path, &ds))
			{
				notify_action(path, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Date, &ds);
			}
		}
	}
}

static void SetFullDate(CONST_STRPTR path, CONST_STRPTR ptr, const struct aline *selected)
{
	struct DateStamp ds;

	if (ParseDateString(FULL_DATEFORMAT, &ds, ptr))
	{
		if (CompareDates(&ds, &selected->datestamp) != 0)
		{
			if (SetFileDate(path, &ds))
			{
				notify_action(path, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Date, &ds);
			}
		}
	}
}
 
DEFMMETHOD(List_EditDone)
{
	const struct aline *selected = msg->entry;
	GETDATA;

	if (selected)
	{
		STRPTR ptr = (STRPTR)getv(msg->editobj, MUIA_String_Contents);

		switch (data->edit_column_type)
		{
			case LISTVIEW_FILE_COL_NAME:
			case LISTVIEW_DEVICE_COL_VOLUME:
				if (*ptr && strcmp(ptr, selected->name))
				{
					UBYTE newpath[256];
					UBYTE oldpath[256];

					if(data->type == FLT_FILES)
					{
						stccpy(newpath, data->path, sizeof(newpath)-1);
						AddPart(newpath, ptr, sizeof(newpath)-1);

						stccpy(oldpath, data->path, sizeof(oldpath)-1);
						AddPart(oldpath, selected->name, sizeof(oldpath)-1);
					}
					else if(data->type == FLT_DEVICES)
					{
						stccpy(newpath, ptr, sizeof(newpath)-1);
						snprintf(oldpath, sizeof(oldpath), "%s:", selected->name);
					}

					do_action(_parent(obj), TA_File_Rename,
						TT_File_Rename_Path, oldpath,
						TT_File_Rename_Name, newpath,
						TT_File_Rename_NoIcon, getv(obj, MA_View_HandleIcons) ? FALSE : TRUE,
					TAG_DONE);
				}
				break;

			case LISTVIEW_FILE_COL_COMMENT:
				if ((selected->un.file.comment == NULL && *ptr == '\0') || (selected->un.file.comment && strcmp(ptr, selected->un.file.comment) == 0))
					break;
			case LISTVIEW_FILE_COL_DATE:
			case LISTVIEW_FILE_COL_FULLDATE:
			case LISTVIEW_FILE_COL_ATTRS:
				{
					if (selected->obj)
					{
						STRPTR path = (STRPTR) getv(selected->obj, MA_Icon_Path);

						if(path)
						{
							u_int32_t flags;

							/* XXX : do that in a thread : create an iconobj for the file and call TA_Icon_Write, TT_Icon_Write_Comment, see infowin */
							methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, path , FALSE );

							switch (data->edit_column_type)
							{
								case LISTVIEW_FILE_COL_COMMENT:
									if (SetComment(path, ptr))
									{
										notify_action(path, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Comment, ptr);
									}
									break;

								case LISTVIEW_FILE_COL_DATE:
									SetDate(path, ptr, selected);
									break;

								case LISTVIEW_FILE_COL_FULLDATE:
									SetFullDate(path, ptr, selected);
									break;

								case LISTVIEW_FILE_COL_ATTRS:
									flags = getv(msg->editobj, MUIA_FSProtectionBits_Flags);

									if ((selected->un.file.protection != flags) && SetProtection(path, flags))
									{
										notify_action(path, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Flags, flags);
									}
									break;
							}

							methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, path , TRUE );
						}
					}
				}
				break;
		}

		data->edit_cursor_pos = getv(msg->editobj, MUIA_String_BufferPos);
	}

	set(_parent(obj), MA_Listview_InEditMode, FALSE); // bitRocky

	return (TRUE);
}

DEFMMETHOD(List_TitleChange)
{
	DoMethod(_parent(obj), MM_Listview_UpdateColumns);
	return (DOSUPER);
}

/* forward to parent for preview */
DEFSMETHOD(Listview_ShowPreview)
{
	DoMethod(obj, MM_Listviewlist_SelectChange);
	DoMethod(_parent(obj), MM_Listview_ShowPreview, msg->enable);

	return (0);
}

DEFSMETHOD(View_Refresh)
{
	GETDATA;

	if (msg->flags &  MF_View_Refresh_Fonts)
	{
		data->alternated_rows = getprefslong(DSI_FASTLIST_ALTERNATED_ROWS);
		data->hilighted_sorting_column = getprefslong(DSI_FASTLIST_HILIGHTED_SORTING_COLUMN);
		data->bold_directories = getprefslong(DSI_FASTLIST_BOLD_DIRECTORIES);

		/* redraw all entries */
		DoMethod(obj, MUIM_List_Redraw, MUIV_List_Redraw_All);
	}
	return (0);
}

BEGINMTABLE
DECNEW
DECSET
DECGET
DECDISP
DECMMETHOD(List_Display)
DECMMETHOD(List_Compare)
DECMMETHOD(List_CreateEditObject)
DECMMETHOD(List_Edit)
DECMMETHOD(List_EditDone)
DECMMETHOD(DragQuery)
DECMMETHOD(DragDrop)
DECMMETHOD(DragReport)
DECMMETHOD(DragBegin)
DECMMETHOD(DragFinish)
DECSMETHOD(Listviewlist_DoubleClick)
DECSMETHOD(Listviewlist_EntryClick)
DECTMETHOD(Listviewlist_SelectChange)
DECSMETHOD(View_GetSelectionList)
DECSMETHOD(View_Select)
DECTMETHOD(Listview_AllocPens)
DECMMETHOD(Setup)
DECMMETHOD(DragEvent)
DECMMETHOD(List_TitleChange)
DECSMETHOD(Listview_ShowPreview)
DECSMETHOD(View_Refresh)
DECTMETHOD(Rexx_Rename)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, listviewlistclass)
