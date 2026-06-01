/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2017 Ambient Open Source Team
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
 * long with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * $Id: listviewclass.c,v 1.86 2026/04/20 16:37:58 piru Exp $
 */

#include "ambient.h"

/* public */
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <graphics/gfx.h>
#include <graphics/gfxmacros.h>
#include <graphics/rpattr.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/locale.h>
#include <proto/dos.h>
#include <devices/rawkeycodes.h>
#include <ctype.h>
#include <workbench/workbench.h>
#include <graphics/rpattr.h>
#include <libraries/usergroup.h>
#include <proto/usergroup.h>
#include <proto/thumbnails.h>
#include <libraries/thumbnails.h>

/* private */
#include "ambient_cat.h"
#include "listviewclass.h"
#include "locale.h"
#include "mui_func.h"
#include "doslistcache.h"
#include "capacity.h"
#include "prefs.h"
#include "gfx_pen.h"
#include "threads.h"
#include "file_func.h"
#include "command.h"
#include "rexx.h"
#include "notify.h"
#include "contextmenu.h"
#include "methodstack.h"
#include "config.h"
#include "keymap.h"
#include "time_func.h"
#include "vfs.h"
#include "mimetype.h"
#include "typescanner.h"
#include "iconio.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "gfx_scale.h"
#include "gfx_alpha.h"
#include "name.h"
#include "hash.h"
#include "cache.h"
#include "dragdrop.h"
#include "deficonpool.h"
#include "deficon_getpath.h"
#include "imagecache.h"
#include "typescanner.h"
#include "exdir.h"
#include "storage.h"
#include "datatypes.h"
#include "viewapi.h"
#include "listsort.h"
#include "columnslistclass.h"
#include "viewapi.h"
#include "shortcuts.h"
#include "mimeuri.h"
#include "prefs_desktop.h"
#include "prefs_advanced.h"
#include "examine64.h"
#include "info64.h"
#include "thumbs.h"
#include "avcodec.h"
#include "str.h"
#include "time_func.h"
#include "viewclass.h"
#include "deficon.h"
#include "fonts.h"
#include "trashcan.h"
#include "icondata.h"
#include "networksfs.h"

#define WIPEOUT_FRIENDLY 1

#define ARRAY_SIZE_ADD 32 /* number of entries to add in advance */

void SetWindowTitle(struct IClass *cl, Object *obj, ULONG flags);

/* icon sizes to cycle thru */
static const ULONG iconsizes[]= {20, 32, 48, 64, 128};
// static const ULONG iconsizes[]= {16, 24, 32, 48, 64, 128};

#define LISTVIEW_MODE_TEMPLATE   "ICONSIZE=IS/N,SORTCOLUMN=SC/N,SORTMODE=SM/N,VIEWMODE=VM/N"
struct lv_mode_args {
	LONG *icon_size;
	LONG *sort_column;
	LONG *sort_mode;
	LONG *view_mode;
};

/* readarg struct for listview format */
#define LISTVIEW_FORMAT_TEMPLATE "COL=C/N,DELTA=D/N,PREPARSE=P/K,WEIGHT=W/N,MINWIDTH=MIW/N,MAXWIDTH=MAW/N,BAR/S,ISOBJECT/S,H=HIDDEN/S"
struct lv_format_args {
	LONG *col;
	LONG *delta;
	STRPTR preparse;
	LONG *weight;
	LONG *minwidth;
	LONG *maxwidth;
	LONG bar;
	LONG isobject;
	LONG hidden;
};

struct listview_remove_node
{
	struct MinNode n;
	struct aline * entrynode;
};

struct listview_deficon_node
{
	struct MinNode n;
	ULONG  hash;
	STRPTR iconpath;
	APTR   bitmap;
	APTR   bitmapobject;
	APTR   image;
};

struct Data {
	APTR npool; /* names pool */
	APTR apool; /* pool for alines.. must be flushable when needed */

	/* array */
	struct MinList alist; /* file list */
	struct MinList rlist; /* remove list */
	struct aline **a;     /* insert array */
	ULONG cnt;            /* number of entries */
	ULONG asize;          /* array size */

	/* list type : FLT_DEVICES or FLT_FILES */
	ULONG type;

	/* list subclass */
	APTR list;

	/* notify context */
	APTR nctx;

	/* file */
	TEXT path[PATH_SIZE];

	STRPTR focuspath;
	LONG focuspos;
	STRPTR *initialselection;

	/* status bar */
	struct dirinfo di;

	ULONG newdone;    /* true when new is done */
	ULONG dosetup;    /* true when setup is done */

	/* event handler */
	struct MUI_EventHandlerNode ehnode;

	ULONG qualifier; /* current pressed qualifier */

	ULONG update;    /* flag to allow listview update or not during notifications */

	LONG  win_top;   /* win top  coord */
	LONG  win_left;  /* win left coord */
	ULONG win_width; /* win width */
	ULONG win_height;/* win height */

	APTR  cmenu;     /* entry menu */
	ULONG viewcm;    /* view menu */
	ULONG cmgrouped; /* TRUE if the context menu is in grouped mode */

	/* icon stuff */
	ULONG icondisplay;                  /* flag for icon display */
	ULONG thumbdisplay;                 /* flag to display images thumbnails */
	ULONG iconsize;                     /* size for icons */
	ULONG iconsizeindex;                /* current index in the iconsize array */
	ULONG icons_setup_done;             /* set to true when default images are created and icon thread is ready to start */
	struct MinList deficonlist;         /* list of created icons objects */
	struct SignalSemaphore deficonlistsem; /* deficonlist semaphore*/
	ULONG cleanupicons;

	/* default images */
	APTR defaultfile_bitmap;
	APTR defaultdir_bitmap;
	APTR defaultdisk_bitmap;

	APTR defaultfile_bitmapobject;
	APTR defaultdir_bitmapobject;
	APTR defaultdisk_bitmapobject;

	APTR defaultfile_image;
	APTR defaultdir_image;
	APTR defaultdisk_image;

	/* flags for version and filetypes display */
	ULONG versiondisplay;
	ULONG filetypedisplay;
	ULONG sizesdisplay;
	ULONG md5display;

	/* custom dnd */
	ULONG click_x;
	ULONG click_y;

	/* notify info */
	ULONG notify_filerename;
	ULONG notify_filedelete;
	ULONG notify_filecreate;
	ULONG in_notify;

	/* XXX: too many variables for these threads. will use a mask for pending/restarting threads */

	/* flag to decide whether a thread will be started or just aborted after threads_abort() */
	ULONG startnewthread_createicons;
	ULONG startnewthread_scandir;
	ULONG startnewthread_showdevices;
	ULONG startnewthread_scantype;
	ULONG startnewthread_version;
	ULONG startnewthread_sizes;
	ULONG startnewthread_preview;
	ULONG startnewthread_md5;

	/* pending threads flags */
	ULONG thread_createicons_busy;
	ULONG thread_scandir_busy;
	ULONG thread_showdevices_busy;
	ULONG thread_scantype_busy;
	ULONG thread_version_busy;
	ULONG thread_sizes_busy;
	ULONG thread_preview_busy;
	ULONG thread_md5_busy;

	/* flag to decide what has to be displayed after loaduri */
	ULONG showfiles;
	ULONG showdevices;

	/* listview current format */
	struct listview_format lv_format;
	ULONG  mode_set_by_uri;
	ULONG  load_default_format;
	ULONG  sort_set_by_uri; // bitRocky

	/* scheduled viewmode after a viewmode change */
	ULONG  viewmode_postpone;

	struct Library *LocalUserGroupBase;

	ULONG is_active;

	ULONG was_iconified;
	
	ULONG multiSelect; // bitRocky
	ULONG editMode; // bitRocky
};

#define UserGroupBase data->LocalUserGroupBase

struct createicon_args
{
	APTR  obj;
	APTR  d;
	APTR  entry;
	APTR  bm;
	ULONG cache;
	APTR  av_ctx;
};

static ULONG listview_createicon(APTR viewobj, struct createicon_args * args);
static APTR  listview_findicon(APTR entry, ULONG type, APTR d, ULONG defaultimage);
static void  listview_deleteicon(APTR obj, APTR entry, ULONG invalidate);
static APTR  listview_finddeficon(CONST_STRPTR name, APTR d);
static void  listview_deletedeficon(APTR obj, struct listview_deficon_node * n, APTR d);

static void listview_createlistimages(struct Data * data);
static void listview_deletelistimages(struct Data * data);

static void listview_enable_dosnotify(CONST_STRPTR spath, ULONG enable);
static void listview_reset_to_default(APTR obj, struct Data * data);

static void listview_abort_threads(APTR obj, struct Data * data);
static void listview_abort_thread(int action, int restart, APTR obj, struct Data * data);

static ULONG explode(CONST_STRPTR str, TEXT sep, STRPTR ** array, ULONG * count);

static ULONG listview_is_shown(ULONG col, struct listview_format * format, ULONG type);
static void  listview_parse_format(CONST_STRPTR sformat, CONST_STRPTR mode, struct listview_format * format, ULONG type);
static ULONG listview_retrieve_format(CONST_STRPTR path, struct listview_format * format, ULONG type);
static void  listview_store_format(CONST_STRPTR path, struct listview_format * format, ULONG remove);

static ULONG listview_contextmenu_callback(APTR obj, int entry, struct menuitem_state * state);

void listview_parse_mode(CONST_STRPTR mode, ULONG type, struct listview_format * format)
{
	memset(format, 0, sizeof(*format));
	listview_parse_format(NULL, mode, format, type);
}

extern const ULONG  LVTITLES_FILES[];
extern const ULONG  LVTITLES_DEVICES[];

/* quick check for prefs purpose */
ULONG listview_check_format_string(CONST_STRPTR format, int mode)
{
	STRPTR * array;
	ULONG count = 0;
	ULONG ret = FALSE;

	if(explode(format, ',', &array, &count))
	{
		int i=0;

		switch(mode)
		{
			case FLT_FILES:
				if(count == LISTVIEW_FILE_COL_COUNT)
				{
					ret = TRUE;
				}
				break;

			case FLT_DEVICES:
				if(count == LISTVIEW_DEVICE_COL_COUNT)
				{
					ret = TRUE;
				}
				break;
		}

		while(array[i])
		{
			free(array[i]);
			i++;
		}

		free(array);
	}

	return ret;
}

/* returns the array of column entries associated to list format. */
ULONG listview_get_columns(CONST_STRPTR format, ULONG type, struct column_entry *** columns)
{
	STRPTR * array;
	ULONG count = 0;
	ULONG ret = TRUE;

	*columns = NULL;

	if(explode(format, ',', &array, &count))
	{
		int i=0;

		*columns = (struct column_entry **) malloc(sizeof(struct column_entry *)*(count+1));

		while(array[i])
		{
			if(*columns)
			{
				struct RDArgs *result = NULL;
				struct lv_format_args args;

				memset(&args, 0, sizeof(args));
				result = readargsstring(array[i], LISTVIEW_FORMAT_TEMPLATE, (LONG *) &args);

				if(result)
				{
					struct column_entry * ce;

					ce = (struct column_entry *) malloc(sizeof(*ce));

					if(ce)
					{
						if(args.col)
						{
							ce->col   = *args.col;

							if(type == FLT_FILES)
							{
								ce->msgid = LVTITLES_FILES[ce->col];
							}

							if(type == FLT_DEVICES)
							{
								ce->msgid = LVTITLES_DEVICES[ce->col];
							}
						}

						if(args.minwidth)
						{
							ce->minwidth = *args.minwidth;
							ce->hasminwidth = TRUE;
						}
						else
						{
							ce->hasminwidth = FALSE;
						}

						if(args.maxwidth)
						{
							ce->maxwidth = *args.maxwidth;
							ce->hasmaxwidth = TRUE;
						}
						else
						{
							ce->hasmaxwidth = FALSE;
						}

						if(args.hidden)
						{
							ce->hidden = TRUE;
						}
						else
						{
							ce->hidden = FALSE;
						}

						(*columns)[i] = ce;
					}

					freeargsstring(result);
				}
				else
				{
					ret = FALSE;
				}
			}
			else
			{
				ret = FALSE;
			}

			free(array[i]);
			i++;
		}
		free(array);

		if(*columns)
		{
			(*columns)[i] = NULL;
		}
	}
	else
	{
		ret = FALSE;
	}

	return ret;
}

/* frees columns list */
void listview_free_columns(struct column_entry ** columns)
{
	int i=0;

	while(columns[i])
	{
		free(columns[i]);
		i++;
	}
	free(columns);
}

/* allocates and computes format string from columns list */
ULONG listview_generate_format(struct column_entry ** columns, STRPTR * format)
{
	int i = 0;
	TEXT fmt[128]="";
	TEXT buf[16];
	ULONG ret = FALSE;

	while(columns[i])
	{
		snprintf(buf, sizeof(buf), "C=%ld", columns[i]->col);
		strncat(fmt, buf, sizeof(fmt));

		if(columns[i]->hasminwidth)
		{
			snprintf(buf, sizeof(buf), " MIW=%ld", columns[i]->minwidth);
			strncat(fmt, buf, sizeof(fmt));
		}

		if(columns[i]->hasmaxwidth)
		{
			snprintf(buf, sizeof(buf), " MAW=%ld", columns[i]->maxwidth);
			strncat(fmt, buf, sizeof(fmt));
		}

		if(columns[i]->hidden)
		{
			strncat(fmt, " H", sizeof(fmt));
		}

		if(columns[i+1])
		{
			strncat(fmt, ",", sizeof(fmt));
		}

		i++;
	}

	*format = (STRPTR) malloc(strlen(fmt)+1);

	if(*format)
	{
		strcpy(*format, fmt);
		ret = TRUE;
	}

	return ret;
}

/* free format */
void listview_free_format(STRPTR format)
{
	free(format);
}

/* splits a string in an array of strings, given a separator */
static ULONG explode(CONST_STRPTR str, TEXT sep, STRPTR ** array, ULONG * count)
{
	ULONG cnt;
	CONST_STRPTR ptr = str;

	if(!str)
	{
		*count = 0;
		return FALSE;
	}

	*count = 1;

	while(*ptr)
	{
		if(*ptr == sep)
		{
			(*count)++;
		}
		ptr++;
	}

	cnt = *count;

	*array = (STRPTR *) malloc((*count+1)*sizeof(STRPTR *));

	if(array)
	{
		CONST_STRPTR prevptr;
		ULONG i=0;

		ptr = str;
		prevptr = str;

		while(cnt-- )
		{
			while(*ptr && *ptr != sep)
			{
				ptr++;
			}

			(*array)[i] = (STRPTR) malloc(ptr-prevptr+1);

			if((*array)[i])
			{
				memcpy((*array)[i], prevptr, ptr-prevptr);
				(*array)[i][ptr-prevptr]=0;

				i++;
			}
			ptr++;
			prevptr = ptr;
		}
		(*array)[i] = NULL;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

#if 0
DEFSMETHOD(Listview_GetColumnType)
{
	GETDATA;
	ssize_t col = msg->column;

	if (col >= 0)
	{
		struct listview_format_device *d;
		struct listview_format_file *f;

		switch (data->type)
		{
			case FLT_FILES:
				f = &data->lv_format.un.file;

				if (f->icon == col)
					return LISTVIEW_FILE_COL_ICON;
				else if (f->name == col)
					return LISTVIEW_FILE_COL_NAME;
				else if (f->size == col)
					return LISTVIEW_FILE_COL_SIZE;
				else if (f->date == col)
					return LISTVIEW_FILE_COL_DATE;
				else if (f->attrs == col)
					return LISTVIEW_FILE_COL_ATTRS;
				else if (f->comment == col)
					return LISTVIEW_FILE_COL_COMMENT;
				else if (f->filetype == col)
					return LISTVIEW_FILE_COL_FILETYPE;
				else if (f->version == col)
					return LISTVIEW_FILE_COL_VERSION;
				else if (f->md5 == col)
					return LISTVIEW_FILE_COL_MD5;
				else if (f->fulldate == col)
					return LISTVIEW_FILE_COL_FULLDATE;
				else if (f->uid == col)
					return LISTVIEW_FILE_COL_UID;
				else if (f->gid == col)
					return LISTVIEW_FILE_COL_GID;
				break;

			case FLT_DEVICES:
				d = &data->lv_format.un.dev;

				if (d->icon == col)
					return LISTVIEW_DEVICE_COL_ICON;
				else if (d->volume == col)
					return LISTVIEW_DEVICE_COL_VOLUME;
				else if (d->device == col)
					return LISTVIEW_DEVICE_COL_DEVICE;
				else if (d->free == col)
					return LISTVIEW_DEVICE_COL_FREE;
				else if (d->total == col)
					return LISTVIEW_DEVICE_COL_TOTAL;
				else if (d->dostype == col)
					return LISTVIEW_DEVICE_COL_DOSTYPE;
				break;
		}
	}

	return -1;
}
#endif

DEFSMETHOD(Listview_GetColumnOrder)
{
	GETDATA;
	switch(data->type)
	{
		case FLT_FILES:
			switch(msg->columnid)
			{
				case LISTVIEW_FILE_COL_NAME:
					return (data->lv_format.un.file.name);
				case LISTVIEW_FILE_COL_SIZE:
					return (data->lv_format.un.file.size);
				case LISTVIEW_FILE_COL_DATE:
					return (data->lv_format.un.file.date);
				case LISTVIEW_FILE_COL_ATTRS:
					return (data->lv_format.un.file.attrs);
				case LISTVIEW_FILE_COL_COMMENT:
					return (data->lv_format.un.file.comment);
				case LISTVIEW_FILE_COL_ICON:
					return (data->lv_format.un.file.icon);
				case LISTVIEW_FILE_COL_FILETYPE:
					return (data->lv_format.un.file.filetype);
				case LISTVIEW_FILE_COL_VERSION:
					return (data->lv_format.un.file.version);
				case LISTVIEW_FILE_COL_MD5:
					return (data->lv_format.un.file.md5);
				case LISTVIEW_FILE_COL_FULLDATE:
					return (data->lv_format.un.file.fulldate);
				case LISTVIEW_FILE_COL_UID:
					return (data->lv_format.un.file.uid);
				case LISTVIEW_FILE_COL_GID:
					return (data->lv_format.un.file.gid);
			}
			break;

		case FLT_DEVICES:
			switch(msg->columnid)
			{
				case LISTVIEW_DEVICE_COL_VOLUME:
					return (data->lv_format.un.dev.volume);
				case LISTVIEW_DEVICE_COL_DEVICE:
					return (data->lv_format.un.dev.device);
				case LISTVIEW_DEVICE_COL_FREE:
					return (data->lv_format.un.dev.free);
				case LISTVIEW_DEVICE_COL_TOTAL:
					return (data->lv_format.un.dev.total);
				case LISTVIEW_DEVICE_COL_DOSTYPE:
					return (data->lv_format.un.dev.dostype);
				case LISTVIEW_DEVICE_COL_ICON:
					return (data->lv_format.un.dev.icon);
			}
			break;
	}

	return -1;
}

/* tells if a column is shown or not */
static ULONG listview_is_shown(ULONG col, struct listview_format * format, ULONG type)
{
	switch(type)
	{
		case FLT_FILES:
			switch(col)
			{
				case LISTVIEW_FILE_COL_NAME:
					return (format->un.file.name != -1);
				case LISTVIEW_FILE_COL_SIZE:
					return (format->un.file.size != -1);
				case LISTVIEW_FILE_COL_DATE:
					return (format->un.file.date != -1);
				case LISTVIEW_FILE_COL_ATTRS:
					return (format->un.file.attrs != -1);
				case LISTVIEW_FILE_COL_COMMENT:
					return (format->un.file.comment != -1);
				case LISTVIEW_FILE_COL_ICON:
					return (format->un.file.icon != -1);
				case LISTVIEW_FILE_COL_FILETYPE:
					return (format->un.file.filetype != -1);
				case LISTVIEW_FILE_COL_VERSION:
					return (format->un.file.version != -1);
				case LISTVIEW_FILE_COL_MD5:
					return (format->un.file.md5 != -1);
				case LISTVIEW_FILE_COL_FULLDATE:
					return (format->un.file.fulldate != -1);
				case LISTVIEW_FILE_COL_UID:
					return (format->un.file.uid != -1);
				case LISTVIEW_FILE_COL_GID:
					return (format->un.file.gid != -1);
			}
			break;

		case FLT_DEVICES:
			switch(col)
			{
				case LISTVIEW_DEVICE_COL_VOLUME:
					return (format->un.dev.volume != -1);
				case LISTVIEW_DEVICE_COL_DEVICE:
					return (format->un.dev.device != -1);
				case LISTVIEW_DEVICE_COL_FREE:
					return (format->un.dev.free != -1);
				case LISTVIEW_DEVICE_COL_TOTAL:
					return (format->un.dev.total != -1);
				case LISTVIEW_DEVICE_COL_DOSTYPE:
					return (format->un.dev.dostype != -1);
				case LISTVIEW_DEVICE_COL_ICON:
					return (format->un.dev.icon != -1);
			}
			break;
	}

	return FALSE;
}

#define LISTVIEW_SET_STATE(var) if(args.hidden) (var) = -1; else (var) = i;

/* parse current list format/mode and fills information in listview_format struct */
static void listview_parse_format(CONST_STRPTR sformat, CONST_STRPTR mode, struct listview_format * format, ULONG type)
{
	STRPTR * array = NULL;
	ULONG count = 0;
	ULONG i = 0;

	if(format->string_format)
	{
		free(format->string_format);
	}

	memset(format, 0, sizeof(*format));

	if(type == FLT_FILES)
	{
		format->un.file.name = -1;
		format->un.file.size = -1;
		format->un.file.date = -1;
		format->un.file.attrs = -1;
		format->un.file.comment = -1;
		format->un.file.icon = -1;
		format->un.file.filetype = -1;
		format->un.file.version = -1;
		format->un.file.md5 = -1;
		format->un.file.fulldate = -1;
		format->un.file.uid = -1;
		format->un.file.gid = -1;
	}
	else
	{
		format->un.dev.volume  = -1;
		format->un.dev.device  = -1;
		format->un.dev.free    = -1;
		format->un.dev.total   = -1;
		format->un.dev.dostype = -1;
		format->un.dev.icon    = -1;
	}

	/* get icon size, sort options and view submode */
	if(mode)
	{
		struct RDArgs *result = NULL;
		struct lv_mode_args args;

		memset(&args, 0, sizeof(args));

		result = readargsstring(mode, LISTVIEW_MODE_TEMPLATE, (LONG *) &args);

		if(result)
		{
			if(args.icon_size)
			{
				format->icon_size = *args.icon_size;
			}

			if(args.sort_column)
			{
				format->sort_column = *args.sort_column;
			}

			if(args.sort_mode)
			{
				if(abs(*args.sort_mode) != 1)
				{
					format->sort_mode = 1;
				}
				else
				{
					format->sort_mode = *args.sort_mode;
				}
			}

			if(args.view_mode)
			{
				if(*args.view_mode >2)
				{
					format->view_mode = LVM_SHOWALL;
				}
				else
				{
					format->view_mode = *args.view_mode;
				}
			}

			freeargsstring(result);
		}
	}

	/* get format */
	if(sformat)
	{
		format->string_format = (STRPTR) malloc(strlen(sformat)+1);

		if(format->string_format)
		{
			strcpy(format->string_format, sformat);
		}

		/* get shown/hidden columns state from list format string */
		if(explode(sformat, ',', &array, &count))
		{
			while(array[i])
			{
				struct RDArgs *result = NULL;
				struct lv_format_args args;

				memset(&args, 0, sizeof(args));

				result = readargsstring(array[i], LISTVIEW_FORMAT_TEMPLATE, (LONG *) &args);

				if(result)
				{
					switch(type)
					{
						case FLT_FILES:
							if(args.col)
							{
								switch(*args.col)
								{
									case LISTVIEW_FILE_COL_NAME:
										LISTVIEW_SET_STATE(format->un.file.name);
										break;
									case LISTVIEW_FILE_COL_SIZE:
										LISTVIEW_SET_STATE(format->un.file.size);
										break;
									case LISTVIEW_FILE_COL_DATE:
										LISTVIEW_SET_STATE(format->un.file.date);
										break;
									case LISTVIEW_FILE_COL_ATTRS:
										LISTVIEW_SET_STATE(format->un.file.attrs);
										break;
									case LISTVIEW_FILE_COL_COMMENT:
										LISTVIEW_SET_STATE(format->un.file.comment);
										break;
									case LISTVIEW_FILE_COL_ICON:
										LISTVIEW_SET_STATE(format->un.file.icon);
										break;
									case LISTVIEW_FILE_COL_FILETYPE:
										LISTVIEW_SET_STATE(format->un.file.filetype);
										break;
									case LISTVIEW_FILE_COL_VERSION:
										LISTVIEW_SET_STATE(format->un.file.version);
										break;
									case LISTVIEW_FILE_COL_MD5:
										LISTVIEW_SET_STATE(format->un.file.md5);
										break;
									case LISTVIEW_FILE_COL_FULLDATE:
										LISTVIEW_SET_STATE(format->un.file.fulldate);
										break;
									case LISTVIEW_FILE_COL_UID:
										LISTVIEW_SET_STATE(format->un.file.uid);
										break;
									case LISTVIEW_FILE_COL_GID:
										LISTVIEW_SET_STATE(format->un.file.gid);
										break;
								}
							}
							break;


						case FLT_DEVICES:
							if(args.col)
							{
								switch(*args.col + LVFORMAT_DEVICES_BASE)
								{
									case LISTVIEW_DEVICE_COL_VOLUME:
										LISTVIEW_SET_STATE(format->un.dev.volume);
										break;
									case LISTVIEW_DEVICE_COL_DEVICE:
										LISTVIEW_SET_STATE(format->un.dev.device);
										break;
									case LISTVIEW_DEVICE_COL_FREE:
										LISTVIEW_SET_STATE(format->un.dev.free);
										break;
									case LISTVIEW_DEVICE_COL_TOTAL:
										LISTVIEW_SET_STATE(format->un.dev.total);
										break;
									case LISTVIEW_DEVICE_COL_DOSTYPE:
										LISTVIEW_SET_STATE(format->un.dev.dostype);
										break;
									case LISTVIEW_DEVICE_COL_ICON:
										LISTVIEW_SET_STATE(format->un.dev.icon);
										break;
								}
							}
							break;
					}
					freeargsstring(result);
				}
				free(array[i]);
				i++;
			}
			free(array);
		}
	}
}

/* fills listview format structure from database for a given path */
static ULONG listview_retrieve_format(CONST_STRPTR path, struct listview_format * format, ULONG type)
{
	ULONG i=0;
	ULONG found = FALSE;
	TEXT ** formats;

	storage_get(STORAGE_LISTVIEW_FORMAT, STORAGE_STRARRAY, (APTR *) &formats);

	if(formats)
	{
		while(formats[i] && !found)
		{
			ULONG j = 0;
			STRPTR * entry;
			ULONG count = 0;

			if(explode(formats[i], '\1', &entry, &count))
			{
				if(count == 3)
				{
					if(!stricmp(path, entry[0]))
					{
						listview_parse_format(entry[2], entry[1], format, type);
						found = TRUE;
					}
				}

				j=0;
				while(entry[j])
				{
					free(entry[j]);
					j++;
				}
				free(entry);
			}
			i++;
		}
		free(formats);
	}
	return found;
}

/* stores listview structure in database, for a given path */
static void listview_store_format(CONST_STRPTR path, struct listview_format * format, ULONG remove)
{
	LONG i = 0, found = -1;
	TEXT ** formats;
	TEXT ** newformats;
	ULONG formatcount = 0;
	STRPTR newentry = NULL;
	ULONG len;
	TEXT  mode[64];

	snprintf(mode, sizeof(mode), "ICONSIZE=%ld SORTCOLUMN=%ld SORTMODE=%ld VIEWMODE=%ld", format->icon_size, format->sort_column, format->sort_mode, format->view_mode);

	len = strlen(path)+strlen(mode)+strlen(format->string_format)+2;

	newentry = (STRPTR) malloc(len+1);

	if(newentry)
	{
		/* generating new entry string */
		snprintf(newentry, len+1, "%s\1%s\1%s", path, mode, format->string_format);

		/* retrieve entries from storage */
		storage_get(STORAGE_LISTVIEW_FORMAT, STORAGE_STRARRAY, (APTR *) &formats);

		if(formats)
		{
			/* search if passed path is already in database */
			while(formats[i] && (found == -1))
			{
				ULONG j = 0;
				STRPTR * entry;
				ULONG count = 0;

				if(explode(formats[i], '\1', &entry, &count))
				{
					if(count == 3)
					{
						if(!stricmp(path, entry[0]))
						{
							found = i;
						}
					}
					j=0;
					while(entry[j])
					{
						free(entry[j]);
						j++;
					}
					free(entry);
				}
				i++;
			}

			i = 0;

			/* entries count */
			while(formats[i])
			{
				formatcount++;
				i++;
			}

			if(found == -1)
			{
				formatcount++;
			}
			else
			{
				if(remove)
				{
					formatcount--;
				}
			}
		}
		else
		{
			formatcount = 1;
		}

		/* copy entries and update with newentry */
		newformats = (TEXT **) malloc(sizeof(TEXT *)*(formatcount+1));

		if(newformats)
		{
			ULONG j = 0;

			i = 0;

			while(formats && formats[i])
			{
				if(i != found)
				{
					newformats[j] = name_build(formats[i]);
					j++;
				}
				else
				{
					if(!remove)
					{
						newformats[j] = name_build(newentry);
						j++;
					}
				}
				i++;
			}

			if(found == -1)
			{
				newformats[j] = name_build(newentry);
				j++;
			}

			newformats[j] = NULL;

			/* store them to database, at last */
			storage_set(STORAGE_LISTVIEW_FORMAT, STORAGE_STRARRAY, (APTR) newformats);

			i = 0;
			while(newformats[i])
			{
				free(newformats[i]);
				i++;
			}
			free(newformats);
		}

		if(formats)
		{
			free(formats);
		}
		free(newentry);
	}
}


/* enable or disable dosnotify for a given path */
static void listview_enable_dosnotify(CONST_STRPTR spath, ULONG enable)
{
	TEXT to[PATH_SIZE];

	/* Enable dos notify */
	/* EnableDOSNotify makes parent from it, so we have to trick it. */

	if (spath && *spath)
	{
		CONST_STRPTR path;
		ULONG len;

		len = strlen(spath);
		path = spath;

		if ( spath[len - 1] != ':' && spath[len - 1] != '/' )
		{
			path = to;
			stccpy(to, spath, sizeof(to));
			if (len + 1 < sizeof(to))
			{
				to[len++] = '/';
				to[len] = '\0';
			}

		}

		methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, path, enable );
	}
}

/* update data needed by statusbar */
static void listview_update_statusbar_info(APTR obj, struct Data * data)
{
	struct aline * al;
	ULONG totalfiles = 0;
	ULONG totaldirs  = 0;
	UQUAD diskusage  = 0;
	UQUAD selecteddiskusage = 0;

	ITERATELIST(al, &data->alist)
	{
		if (al->type == ST_USERDIR && al->obj)
		{
			UQUAD *sizep = (UQUAD *) getv(al->obj, MA_Icon_FileSize);
			if(sizep)
			{
				diskusage += *sizep;

				if(al->active)
				{
					selecteddiskusage += *sizep;
				}
			}

			totaldirs++;
		}
		else if(al->type == ST_FILE)
		{
			diskusage += al->un.file.size;

			if(al->active)
			{
				selecteddiskusage += al->un.file.size;
			}

			totalfiles++;
		}
	}

	SetAttrs(obj,
		MA_View_TotalFiles,totalfiles,
		MA_View_TotalDirs, totaldirs,
		MA_View_DiskUsage, &diskusage,
		MA_View_SelectedDiskUsage, &selecteddiskusage,
		TAG_DONE
	);
}

/* set format and mode to default settings */
static void listview_reset_to_default(APTR obj, struct Data * data)
{
	if(data->type == FLT_FILES)
	{
DB(("before listview_parse_format(): data->lv_format.sort_column = %ld, data->lv_format.sort_mode = %ld\n", data->lv_format.sort_column, data->lv_format.sort_mode));
		listview_parse_format(_conf(fl_default_format_files), _conf(fl_default_mode_files), &data->lv_format, data->type);
DB(("after  listview_parse_format(): data->lv_format.sort_column = %ld, data->lv_format.sort_mode = %ld\n", data->lv_format.sort_column, data->lv_format.sort_mode));

		data->filetypedisplay = listview_is_shown(LISTVIEW_FILE_COL_FILETYPE, &data->lv_format, data->type);
		data->icondisplay     = listview_is_shown(LISTVIEW_FILE_COL_ICON, &data->lv_format, data->type);
		data->versiondisplay  = listview_is_shown(LISTVIEW_FILE_COL_VERSION, &data->lv_format, data->type);
		data->md5display      = listview_is_shown(LISTVIEW_FILE_COL_MD5, &data->lv_format, data->type);

		/* spawn threads if needed */
		listview_abort_thread(TA_Listview_CreateIcons, TRUE, obj, data);
		listview_abort_thread(TA_Version_Find, TRUE, obj, data);
		listview_abort_thread(TA_File_MD5sum, TRUE, obj, data);
		listview_abort_thread(TA_MimeType_Scan, TRUE, obj, data);
	}

	if(data->type == FLT_DEVICES)
	{
		listview_parse_format(_conf(fl_default_format_devices), _conf(fl_default_mode_devices), &data->lv_format, data->type);

		data->icondisplay = listview_is_shown(LISTVIEW_DEVICE_COL_ICON, &data->lv_format, data->type);

		/* spawn threads if needed */
		listview_abort_thread(TA_Listview_CreateIcons, TRUE, obj, data);
	}

DB(("before SetAttrs(): data->lv_format.sort_column = %ld, data->lv_format.sort_mode = %ld\n", data->lv_format.sort_column, data->lv_format.sort_mode));
	SetAttrs(data->list,
		MUIA_List_Format, data->lv_format.string_format, /* set format */
		MA_Listview_SortColumn, data->lv_format.sort_column, /* set sort attributes */
		MA_Listview_SortDirection, data->lv_format.sort_mode,
		TAG_DONE
	);
	DoMethod(data->list, MUIM_List_Sort);
}

/* aborting all threads */
static void listview_abort_threads(APTR obj, struct Data * data)
{
	data->thread_scandir_busy     = TRUE;
	data->thread_showdevices_busy = TRUE;
	data->thread_createicons_busy = TRUE;
	data->thread_scantype_busy    = TRUE;
	data->thread_version_busy     = TRUE;
	data->thread_md5_busy         = TRUE;
	data->thread_sizes_busy       = TRUE;
	data->thread_preview_busy     = TRUE;

	threads_abort(obj, TA_File_ScanDir,         NULL);
	threads_abort(obj, TA_Listview_ShowDevices, NULL);
	threads_abort(obj, TA_Listview_CreateIcons, NULL);
	threads_abort(obj, TA_MimeType_Scan,        NULL);
	threads_abort(obj, TA_Version_Find,         NULL);
	threads_abort(obj, TA_File_MD5sum,          NULL);
	threads_abort(obj, TA_File_GetSizes,        NULL);
	threads_abort(obj, TA_Listview_ShowPreview, NULL);

}

/* abort a given thread action, and restart if wanted */
static void listview_abort_thread(int action, int restart, APTR obj, struct Data * data)
{
	switch(action)
	{
		case TA_File_ScanDir:
			data->startnewthread_scandir = restart;
			break;
		case TA_Listview_ShowDevices:
			data->startnewthread_showdevices = restart;
			break;
		case TA_Listview_CreateIcons:
			data->startnewthread_createicons = restart;
			break;
		case TA_MimeType_Scan:
			data->startnewthread_scantype = restart;
			break;
		case TA_Version_Find:
			data->startnewthread_version = restart;
			break;
		case TA_File_MD5sum:
			data->startnewthread_md5 = restart;
			break;
		case TA_File_GetSizes:
			data->startnewthread_sizes = restart;
			break;
		case TA_Listview_ShowPreview:
			data->startnewthread_preview = restart;
			break;
	}
	threads_abort(obj, action, NULL);
}

static void doset(APTR obj, struct Data *data, struct TagItem *tags)
{
	FORTAG(tags)
	{
		case MA_View_DiskUsage:
			data->di.diskusage = *(QUAD *)(tag->ti_Data);
			break;

		case MA_View_SelectedDiskUsage:
			data->di.selecteddiskusage = *(QUAD *)(tag->ti_Data);
			break;

		case MA_Listviewlist_IconDisplay:
			data->icondisplay = tag->ti_Data;
			break;

		case MA_Listview_IconSize:
			if(tag->ti_Data <= 0)
			{
				data->lv_format.icon_size = (data->iconsizeindex+1)%(sizeof(iconsizes)/sizeof(ULONG));
			}
			else
			{
				data->iconsizeindex = 0;
				data->iconsize      = tag->ti_Data;
			}
			break;

		case MA_View_ModeIndex:
			if(tag->ti_Data == LVM_SHOWALL || tag->ti_Data == LVM_ICONS || tag->ti_Data == LVM_THUMBS)
			{
				if(data->lv_format.view_mode != tag->ti_Data)
				{
					/* scheduled viewmode */
					data->viewmode_postpone = tag->ti_Data;

					/* scandir thread can't be done in OM_NEW */
					if(data->newdone && data->type == FLT_FILES)
					{
						listview_abort_threads(obj, data);
					}
				}
			}
			break;

		case MA_Listview_SortDirection:
			data->lv_format.sort_mode = tag->ti_Data;
			DB(("data->lv_format.sort_mode = %ld\n", data->lv_format.sort_mode)); // bitRocky
			break;

		case MA_Listview_SortColumn:
			data->lv_format.sort_column = tag->ti_Data;
			DB(("data->lv_format.sort_column = %ld\n", data->lv_format.sort_column)); // bitRocky
			break;
		
		case MA_Listview_InEditMode: // bitRocky: 
			data->editMode = tag->ti_Data;
			break;
	}
	NEXTTAG
}

DEFNEW
{
	struct Data *data;
	APTR list;

	obj = DoSuperNew(cl, obj,
		Child, list = NewObject(getlistviewlistclass(), NULL, TAG_DONE),
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return (IPTR)(NULL);
	}

	data = INST_DATA(cl, obj);

	if (!(data->apool = CreatePool(MEMF_SEM_PROTECTED | MEMF_ANY, 4096, 2048)))
	{
		CoerceMethod(cl, obj, OM_DISPOSE);
		return (IPTR)(NULL);
	}

	if( !(data->npool = CreatePool(MEMF_SEM_PROTECTED | MEMF_ANY, 2048, 1024)))
	{
		CoerceMethod(cl, obj, OM_DISPOSE);
		return (IPTR)(NULL);
	}

	NEWLIST(&data->alist);
	NEWLIST(&data->rlist);
	NEWLIST(&data->deficonlist); /* This too */

	InitSemaphore(&(data->deficonlistsem));

	data->list = list;
	data->newdone = FALSE;
	data->update = TRUE;
	data->type = FLT_UNKNOWN;
	data->focuspath = NULL;

	data->cmenu     = NULL;
	data->cmgrouped = FALSE;
	data->viewcm    = FALSE;

	data->iconsize = iconsizes[0];
	data->iconsizeindex = 0;
	data->icons_setup_done = FALSE;
	data->dosetup = TRUE;
	data->cleanupicons = FALSE;

	data->icondisplay     = FALSE;
	data->thumbdisplay    = FALSE;
	data->filetypedisplay = FALSE;
	data->versiondisplay  = FALSE;
	data->sizesdisplay    = FALSE;
	data->md5display      = FALSE;

	data->click_x = -1;
	data->click_y = -1;

	data->notify_filerename = FALSE;
	data->notify_filedelete = FALSE;
	data->notify_filecreate = FALSE;

	data->thread_createicons_busy = FALSE;
	data->thread_scandir_busy     = FALSE;
	data->thread_showdevices_busy = FALSE;
	data->thread_scantype_busy    = FALSE;
	data->thread_version_busy     = FALSE;
	data->thread_sizes_busy       = FALSE;
	data->thread_preview_busy     = FALSE;
	data->thread_md5_busy         = FALSE;

	data->startnewthread_scandir     = FALSE;
	data->startnewthread_showdevices = FALSE;
	data->startnewthread_createicons = FALSE;
	data->startnewthread_scantype    = FALSE;
	data->startnewthread_version     = FALSE;
	data->startnewthread_sizes       = FALSE;
	data->startnewthread_preview     = FALSE;
	data->startnewthread_md5         = FALSE;

	data->showdevices = FALSE;
	data->showfiles   = FALSE;

	memset(&data->lv_format, 0, sizeof(data->lv_format));
	data->load_default_format = FALSE;

	doset(obj, data, INITTAGS);

	data->newdone = TRUE;

	data->LocalUserGroupBase = OpenLibrary("usergroup.library", 4);

	data->is_active = 1;

	return ((ULONG)obj);
}


DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_View_NeedsBackfill:
			*msg->opg_Storage = TRUE;
			return (TRUE);
			break;

		case MA_View_NewWin:
			*msg->opg_Storage = FALSE;
			break;

		case MA_View_HasBackground:
			*msg->opg_Storage = TRUE;
			return (TRUE);
			break;

		case MA_View_Type:
			*msg->opg_Storage = MV_View_Type_List;
			return (TRUE);

		case MA_View_ViewMode:
			*msg->opg_Storage = IVM_LISTER;
			return (TRUE);

		case MA_Iconview_Qualifier:
		{
			*msg->opg_Storage = data->qualifier;
			return (TRUE);
		}

		case MA_View_DiskUsage:
		{
			*msg->opg_Storage = (ULONG)&(data->di.diskusage);
			return (TRUE);
		}

		case MA_View_SelectedDiskUsage:
		{
			*msg->opg_Storage = (ULONG)&(data->di.selecteddiskusage);
			return (TRUE);
		}

		case MA_View_ModeIndex:
		{
			*msg->opg_Storage = data->lv_format.view_mode;
			return (TRUE);
		}

		case MA_View_ShowDevices:
		{
			*msg->opg_Storage = data->type == FLT_DEVICES;
			return (TRUE);
		}

		case MA_View_SubObject:
		{
			*msg->opg_Storage = (ULONG)data->list;
			return (FALSE);
		}

		case MA_Listview_GetSizes:
		{
			*msg->opg_Storage = data->sizesdisplay;
			return (TRUE);
		}

		case MA_Listview_Version:
		{
			*msg->opg_Storage = data->versiondisplay;
			return (TRUE);
		}

		case MA_Listview_MD5:
		{
			*msg->opg_Storage = data->md5display;
			return (TRUE);
		}

		case MUIA_List_TopPixel:
		{
			*msg->opg_Storage = getv(data->list, MUIA_List_TopPixel);
			return (TRUE);
		}

		case MA_Listview_SortDirection:
		{
			*msg->opg_Storage = data->lv_format.sort_mode;
			return (TRUE);
		}

		case MA_Listview_SortColumn:
		{
			*msg->opg_Storage = data->lv_format.sort_column;
			return (TRUE);
		}

		case MA_Listview_Preview:
		{
			*msg->opg_Storage = data->thread_preview_busy;
			return (TRUE);
		}

		case MA_View_IsViewObject:
			*msg->opg_Storage = TRUE;
			return (TRUE);

		case MA_View_HandleIcons:
			*msg->opg_Storage = (data->lv_format.view_mode == LVM_ICONS) ? TRUE : FALSE;
			return (TRUE);
	}

	return (DOSUPER);
}

static void listview_deleteentry(APTR pool, struct aline * al)
{
	if(al)
	{
		switch(al->nodetype)
		{
			case FLT_FILES:
			{
				if(al->name)
				{
					FreeVecPooled(pool, al->name);
					al->name = NULL;
				}

				if(al->obj)
				{
					MUI_DisposeObject(al->obj);
					al->obj = NULL;
				}

				if(al->un.file.uid)
				{
					FreeVecPooled(pool, al->un.file.uid);
					al->un.file.uid = NULL;
				}

				if(al->un.file.gid)
				{
					FreeVecPooled(pool, al->un.file.gid);
					al->un.file.gid = NULL;
				}

				if(al->un.file.comment)
				{
					FreeVecPooled(pool, al->un.file.comment);
					al->un.file.comment = NULL;
				}

				if(al->date)
				{
					FreeVecPooled(pool, al->date);
					al->date = NULL;
				}

				if(al->time)
				{
					FreeVecPooled(pool, al->time);
					al->time = NULL;
				}

				if(al->un.file.target)
				{
					FreeVecPooled(pool, al->un.file.target);
					al->un.file.target = NULL;
				}

				break;
			}

			case FLT_DEVICES:
			{
				if(al->name)
				{
					FreeVecPooled(pool, al->name);
					al->name = NULL;
				}

				if(al->obj)
				{
					MUI_DisposeObject(al->obj);
					al->obj = NULL;
				}

				if(al->un.dev.devname)
				{
					FreeVecPooled(pool, al->un.dev.devname);
					al->un.dev.devname = NULL;
				}

				break;
			}
		}

		FreeVecPooled(pool, al);
	}
}

static ULONG dostype2filetype(LONG dostype, ULONG mode, ULONG dir)
{
	if(mode == FLT_FILES)
	{
		switch(dostype)
		{
			case ST_FILE:
				return MV_Icon_FileType_File;

			case ST_USERDIR:
				return MV_Icon_FileType_Directory;

			case ST_LINKDIR:
				return MV_Icon_FileType_Directory/*MV_Icon_FileType_Hardlink_Directory*/;

			case ST_LINKFILE:
				return MV_Icon_FileType_File/*MV_Icon_FileType_Hardlink_File*/;

			case ST_SOFTLINK:
				if(dir)
					return MV_Icon_FileType_Directory;
				else
					return MV_Icon_FileType_File/*MV_Icon_FileType_Softlink*/;

			default:
				return MV_Icon_FileType_None;
		}
	}
	else if(mode == FLT_DEVICES)
	{
		switch(dostype)
		{
			case DLT_VOLUME:
				return MV_Icon_FileType_Device;

			case DLT_DIRECTORY:
				return MV_Icon_FileType_Directory;

			default:
				return MV_Icon_FileType_None;
		}	 
	}
	else
	{
		return MV_Icon_FileType_None;
	}
}

static ULONG dostype2icontype(LONG dostype, ULONG mode, ULONG dir)
{
	if(mode == FLT_FILES)
	{
		switch(dostype)
		{
			case ST_FILE:
			case ST_LINKFILE:
				return MV_Icon_Type_Tool;

			case ST_SOFTLINK:
				if (dir)
					return MV_Icon_Type_Drawer;
				else
					return MV_Icon_Type_Tool;

			case ST_USERDIR:
			case ST_LINKDIR:
				return MV_Icon_Type_Drawer;

			default:
				return MV_Icon_Type_None;
		}
	}
	else if(mode == FLT_DEVICES)
	{
		switch(dostype)
		{
			case DLT_VOLUME:
				return MV_Icon_Type_Device;

			case DLT_DIRECTORY:
				return MV_Icon_Type_Drawer;

			default:
				return MV_Icon_Type_None;
		}
	}
	else
	{
		return MV_Icon_Type_None;
	}
}

static void SetDateText(struct Data *data, struct aline *al)
{
	struct DateTime dt;
	TEXT tmpdate[ 50 ];
	TEXT tmptime[ 50 ];
	STRPTR p;

	al->un.file.date = al->datestamp.ds_Tick / TICKS_PER_SECOND + al->datestamp.ds_Minute * 60 + al->datestamp.ds_Days * 60 * 24 * 60;

	dt.dat_Stamp            = al->datestamp;
	dt.dat_Format           = FORMAT_DEF; /* FORMAT_DOS */
	dt.dat_Flags            = DTF_SUBST;
	dt.dat_StrDay           = NULL;
	dt.dat_StrDate          = tmpdate;
	dt.dat_StrTime          = tmptime;

	if (!DateToStr(&dt))
	{
		strcpy(tmpdate, "Unknown");
		strcpy(tmptime, "Unknown");
	}

	p = AllocVecPooled(data->apool, strlen(tmpdate) + 1);

	if (p)
	{
		if (al->date)
			FreeVecPooled(data->apool, al->date);

		al->date = p;
		strcpy(p, tmpdate);
	}

	p = AllocVecPooled(data->apool, strlen(tmptime) + 1);

	if (p)
	{
		if (al->time)
			FreeVecPooled(data->apool, al->time);

		al->time = p;
		strcpy(p, tmptime);
	}
}

DEFSMETHOD(Listview_AddFile)
{
	GETDATA;
	struct aline *al;
	struct aline *repl;
	ULONG rc = FALSE;

	if ((al = AllocVecPooled(data->apool, sizeof(*al))))
	{
		al->nodetype = FLT_FILES;
		al->newentry = FALSE;
		al->invalidate_cache = 0;
		al->invalidate_mimetype = 0;
		al->active = 0;
		al->removed = FALSE;
		al->namelen = strlen(msg->ead->ed_Name);
		if ((al->name = AllocVecPooled(data->apool, al->namelen + 1)))
		{
			al->type = msg->ead->ed_Type;

			strcpy(al->name, msg->ead->ed_Name);

			#if USE_LEGACY
			al->un.file.size = msg->ead->ed_Size;
			#else
			al->un.file.size = msg->ead->ed_Size64;
			#endif
			al->un.file.protection = msg->ead->ed_Prot;

			al->datestamp.ds_Days   = msg->ead->ed_Days;
			al->datestamp.ds_Minute = msg->ead->ed_Mins;
			al->datestamp.ds_Tick   = msg->ead->ed_Ticks;
			al->time = NULL;
			al->date = NULL;

			/* Convert date to string format */
			SetDateText(data, al);

			al->un.file.comment = NULL;

			if (msg->ead->ed_Comment)
			{
				ULONG len = strlen( msg->ead->ed_Comment );
				if ( len )
				{
					if ((al->un.file.comment = AllocVecPooled(data->apool, len + 1)))
					{
						strcpy(al->un.file.comment, msg->ead->ed_Comment);
					}
				}
			}

			al->un.file.uid = NULL;
			al->un.file.gid = NULL;

			if (data->LocalUserGroupBase)
			{
				struct passwd * pwd = getpwuid(msg->ead->ed_OwnerUID);
				struct group  * gr  = getgrgid(msg->ead->ed_OwnerGID);

				if(pwd)
				{
					if ((al->un.file.uid = AllocVecPooled(data->apool, strlen(pwd->pw_name) + 1)))
					{
						strcpy(al->un.file.uid, pwd->pw_name);
					}
				}

				if(gr)
				{
					if ((al->un.file.gid = AllocVecPooled(data->apool, strlen(gr->gr_name) + 1)))
					{
						strcpy(al->un.file.gid, gr->gr_name);
					}
				}
			}

			if(msg->target)
			{
				if ((al->un.file.target = AllocVecPooled(data->apool, strlen(msg->target) + 1)))
				{
					strcpy(al->un.file.target, msg->target);
				}
			}
			else
			{
				al->un.file.target = NULL;
			}

			if(msg->replace)
			{
				ITERATELIST(repl, &data->alist)
				{
					if ((stricmp(repl->name, al->name) == 0) && !repl->removed)
					{
						struct listview_remove_node * n;

						n = AllocVecPooled(data->apool, sizeof(*n));

						if(n)
						{
							repl->removed = TRUE;
							n->entrynode = repl;

							ADDTAIL(&data->rlist, n);
						}

						break;
					}
				}
			}

			al->obj = NewObject(getlistviewentryclass(), NULL, TAG_DONE);

			if(al->obj)
			{
				ULONG len;
				STRPTR temp;
				STRPTR p;

				p    = (STRPTR) getv(obj, MA_View_Path);
				len  = strlen(p) + strlen(msg->ead->ed_Name) + 3;
				temp = AllocVecPooled(data->apool, len);

				if (temp)
				{
					ULONG dir = FALSE;
					strcpy(temp, p);
					AddPart(temp, msg->ead->ed_Name, len);

					if(al->type == ST_SOFTLINK)
					{
						dir = isdir(temp);
					}

					SetAttrs(al->obj,
						MA_ListviewEntry_Pool, data->apool,
						MA_ListviewEntry_ListObject, data->list,
						MA_Icon_Path, temp,
						MA_Icon_Type, dostype2icontype(al->type, al->nodetype, dir),
						MA_Icon_FileType, dostype2filetype(al->type, al->nodetype, dir),
						MA_Icon_FileDate, al->un.file.date,
						TAG_DONE
					);

					/*
					if(al->type != ST_USERDIR)
					{
						set(al->obj, MA_Icon_FileSize, (LONG) &al->un.file.size);
					}
					*/

					FreeVecPooled(data->apool, temp);

					if(msg->mode == MV_Listview_AddFile_New)
					{
						al->newentry = TRUE;
					}

					ADDTAIL(&data->alist, al);
					data->cnt++;

					rc = TRUE;
				}
			}
		}
	}


	if(rc == FALSE)
	{
		listview_deleteentry(data->apool, al);
	}


	return (rc);
}

DEFSMETHOD(Listview_RemoveByName)
{
	GETDATA;
	struct aline *rem;

	ITERATELIST(rem, &data->alist)
	{
		if ((stricmp(rem->name, msg->name) == 0) && !rem->removed)
		{
			struct listview_remove_node * n;

			n = AllocVecPooled(data->apool, sizeof(*n));

			if(n)
			{
				rem->removed = TRUE;
				n->entrynode = rem;
				ADDTAIL(&data->rlist, n);
			}

			break;
		}
	}

	return (0);
}

DEFSMETHOD(Listview_RemoveFiles)
{
	GETDATA;
	struct listview_remove_node * rem, * nextrem;
	struct aline * al;
	LONG i;
	ULONG remove_selected = TRUE;
	ULONG count = 0;
	LONG state;
	int firstselected = -1;

	DoMethod(data->list, MUIM_List_Select, MUIV_List_Select_Active, MUIV_List_Select_On, NULL);
	set(data->list, MUIA_List_Active, MUIV_List_Active_Off);

	/*
	 * if at least one of the selected entries hasn't been marked as removed or one of the unselected
	 * entries has been marked as removed, we must remove each entry individually (slow), else we can remove selected entries (fast).
	 */

	for(i=0;;i++)
	{
		DoMethod(data->list, MUIM_List_GetEntry, i, &al);

		if(al)
		{
			DoMethod(data->list, MUIM_List_Select, i, MUIV_List_Select_Ask, &state);

			if(state == MUIV_List_Select_On && firstselected == -1)
			{
				firstselected = i;
			}

			if((state == MUIV_List_Select_Off && al->removed) || (state == MUIV_List_Select_On && !al->removed))
			{
				//SDB(("%s is NOT marked as removed or is NOT selected. Individual remove\n", al->name));
				remove_selected = FALSE;
				break;
			}
			else if(state == MUIV_List_Select_On && al->removed)
			{
				//SDB(("%s is marked as removed\n", al->name));
				count++;
			}
		}
		else
		{
			break;
		}
	}

	if(remove_selected && count)
	{
		DoMethod(data->list, MUIM_List_Remove, MUIV_List_Remove_Selected);
	}
	else
	{
		ITERATELIST(rem, &data->rlist)
		{
			for(i=0;;i++)
			{
				DoMethod(data->list, MUIM_List_GetEntry, i, &al);

				if(al == NULL)
				{
					/* can happen if a deletion op was started before entering the new path, then don't try to delete this entry */
					rem->entrynode->removed = FALSE;
					break;
				}

				if(al == rem->entrynode)
				{
					DoMethod(data->list, MUIM_List_Remove, i);
					break;
				}
			}
		}
	}

	if(!data->thread_createicons_busy)
	{
		ITERATELISTSAFE(rem, nextrem, &data->rlist)
		{
			if(rem->entrynode->removed)
			{
				REMOVE((struct Node *) rem->entrynode);
				data->cnt--;

#if !WIPEOUT_FRIENDLY
				if(rem->entrynode->obj)
				{
					MUI_DisposeObject(rem->entrynode->obj);
					rem->entrynode->obj = NULL;
				}
#else
				listview_deleteentry(data->apool, rem->entrynode);
#endif
			}

			FreeVecPooled(data->apool, rem);
		}
		NEWLIST(&data->rlist);
	}
	else
	{
		DB(("threads busy, deferring entry objects deletion...\n"));
	}

	if(firstselected != -1)
	{
		set(data->list, MUIA_List_Active, min(firstselected, getv(data->list, MUIA_List_Entries)-1));
	}

	SetWindowTitle(cl, obj, MF_View_SetStatus_Window);

	return (0);
}

DEFSMETHOD(Listview_AddFiles)
{
	GETDATA;

	if ( data->a )
		FreeVecPooled(data->apool, data->a);

	data->asize = data->cnt + ARRAY_SIZE_ADD;
	if ((data->a = AllocVecPooled(data->apool, data->asize * sizeof(APTR))))
	{
		struct aline *al;
		ULONG i = 0;

		ITERATELIST(al, &data->alist)
		{
			if(msg->mode == MV_Listview_AddFiles_New)
			{
				if(al->newentry)
				{
					al->newentry = FALSE;
					data->a[ i ] = al;
					i++;
				}
			}
			else
			{
				data->a[ i ] = al;
				i++;
			}
		}

		data->a[ i ] = NULL;
	}

	return (0);
}

DEFSET
{
	GETDATA;

	doset(obj, data, INITTAGS);

	return (DOSUPER);
}

DEFMMETHOD(Setup)
{
	ULONG rc;
	GETDATA;

	if(!data->dosetup)
	{
		return (0);
	}

	if ((rc = (DOSUPER && _win(obj))))
	{
		set( _win(obj), MUIA_Window_DefaultObject, data->list );

		data->ehnode.ehn_Object = obj;
		data->ehnode.ehn_Class = cl;
		data->ehnode.ehn_Events =  IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY /*|  IDCMP_MOUSEOBJECT | IDCMP_MOUSEMOVE*/;
		data->ehnode.ehn_Priority = 1; /* priority over MUI's areaclass */
		data->ehnode.ehn_Flags = MUI_EHF_GUIMODE | MUI_EHF_PRIORITY; // bitRocky: added MUI_EHF_PRIORITY to also get KEYDOWN events!
		DoMethod(_win(obj), MUIM_Window_AddEventHandler, (ULONG)&data->ehnode);

		if(data->was_iconified)
		{
			listview_createlistimages(data);
			data->was_iconified = FALSE;
		}
	}

	return (rc);
}

DEFMMETHOD(Cleanup)
{
	GETDATA;

	if(!data->dosetup)
	{
		return (0);
	}

	stccpy(data->path, (STRPTR) getv(obj, MA_View_Path), sizeof(data->path));

	if (muiRenderInfo(obj) && _win(obj))
	{
		DoMethod(_win(obj), MUIM_Window_RemEventHandler, (ULONG)&data->ehnode);
	}

	set( _win(obj), MUIA_Window_DefaultObject, NULL );

	if(getv(_win(obj), MA_Window_IsIconified))
	{
		data->was_iconified = TRUE;
		listview_deletelistimages(data);
	}
	else
	{
		if(data->icondisplay && data->icons_setup_done)
		{
			DoMethod(obj, MM_Listview_CleanupIcons);
			DoMethod(obj, MM_Listview_DeleteDefaultImages);
		}
	}

	return (DOSUPER);
}

DEFTMETHOD(View_Setup)
{
	return (DOSUPER);
}

DEFTMETHOD(View_Cleanup)
{
	return (DOSUPER);
}

DEFMMETHOD(AskMinMax)
{
	GETDATA;

	DOSUPER;

	msg->MinMaxInfo->MinWidth  += 160;
	msg->MinMaxInfo->MinHeight +=  20;

	/* XXX: we should only do that stuff on the first opening ! */
	if (data->win_width && data->win_height)
	{
		msg->MinMaxInfo->DefWidth  = max(msg->MinMaxInfo->MinWidth, data->win_width - (15 + 2));
		msg->MinMaxInfo->DefHeight = max(msg->MinMaxInfo->MinHeight, data->win_height - (15 + 15));
	}

	msg->MinMaxInfo->MaxWidth  = MUI_MAXMAX;
	msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;


	return (0);
}

/* experimental custom drag'n'drop. not up-to-date/working */
#if 0
#define MUIM_List_CreateDragMap             0x80424534 /* private */ /* V11 */
#define MUIM_List_FreeDragMap               0x80425a68
struct  MUIP_List_CreateDragMap             { ULONG MethodID; LONG touchx; LONG touchy; LONG pos; }; /* private */
struct  MUIP_List_FreeDragMap               { ULONG MethodID; struct DragMap *dragmap; }; /* private */
#endif

DEFMMETHOD(HandleEvent)
{
	GETDATA;

	if (msg->imsg)
	{
		data->qualifier = msg->imsg->Qualifier;

		switch (msg->imsg->Class)
		{	
			case IDCMP_RAWKEY:
			{
				ULONG code = msg->imsg->Code & 0x7F;
				ULONG keyDown = (msg->imsg->Code < IECODE_UP_PREFIX);
				if(code)
				{
					switch(code)
					{
						case RAWKEY_SPACE:
						{
							DB(("bitRocky-Test: SPACE pressed, ->multiSelect = %ld, ->editMode = %ld\n", data->multiSelect, data->editMode));
							if (!data->editMode && keyDown) // only DOWN events
							{
								LONG act;
								get(obj, MUIA_List_Active, &act);
								DB(("bitRocky-Test: act = %ld\n", act));
								if (act != MUIV_List_Active_Off)
								{
									ULONG numS;
									LONG state;
									get(_parent(obj), MA_View_NumSelected, &numS);
									DB(("bitRocky-Test: numS = %ld\n", numS));
									if (numS == 1 && !data->multiSelect)
									{
										DoMethod(obj, MUIM_List_Select, act, MUIV_List_Select_Ask, &state);
										DB(("bitRocky-Test: state = %ld\n", state));
										//if (state == MUIV_List_Select_Off)
										{
											DoMethod(obj, MUIM_List_Select, act, MUIV_List_Select_On, NULL);
											set(_parent(obj), MA_View_NumSelected, 2);
											data->multiSelect = TRUE;
											set(obj, MUIA_List_Active, MUIV_List_Active_Down);
											return (MUI_EventHandlerRC_Eat);
										}
									}
									data->multiSelect = TRUE;
								}
							}
						}
						break;
						case RAWKEY_UP:
						case RAWKEY_DOWN:
						{
							if (keyDown)
							{
								LONG act = (code == RAWKEY_DOWN) ? MUIV_List_Active_Down : MUIV_List_Active_Up;

								if (msg->imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) act -= 2;
								else if (msg->imsg->Qualifier & IEQUALIFIER_CONTROL) act += 2;

								if (!data->multiSelect)
									data->multiSelect = (getv(obj, MUIA_List_Active) == MUIV_List_Active_Off) && (getv(_parent(obj), MA_View_NumSelected) > 0);

								if (!data->multiSelect) DoMethod(obj, MUIM_List_Select, MUIV_List_Select_Active, MUIV_List_Select_Off, NULL);

								set(obj, MUIA_List_Active, act);

								return (MUI_EventHandlerRC_Eat);
							}
						}
						break;
						case RAWKEY_RETURN:
						{
							if ((msg->imsg->Qualifier & IEQUALIFIER_COMMANDS) && !data->multiSelect)
								DoMethod(obj, MUIM_List_Select, MUIV_List_Select_Active, MUIV_List_Select_Off, NULL);
						}
						break;
					}
				}
			}
			break;
			case IDCMP_MOUSEBUTTONS:
			{
				DB(("bitRocky-Test: IDCMP_MOUSEBUTTONS, code = %ld (0x%08lx)\n", msg->imsg->Code, msg->imsg->Code));
				switch(msg->imsg->Code)
				{
					case MENUDOWN:
					case MIDDLEDOWN:
					{
						/*
						 * Trap RMB on the window's titlebar..
						 */
						if ((msg->imsg->Code == MENUDOWN) && _isinwinborder(msg->imsg->MouseX, msg->imsg->MouseY))
						{
							set(_win(obj), MUIA_Window_MouseObject, obj);
							DoMethod(_win(obj), MUIM_Window_HandleRMB, msg->imsg);
							return (MUI_EventHandlerRC_Eat);
						}
						else
						{
							DB(("bitRocky-Test: %s down, data->multiSelect = %ld\n", (msg->imsg->Code == MENUDOWN) ? "rmb" : "mmb", data->multiSelect));
							if (!data->multiSelect)
							{
								ULONG numS;
								get(_parent(obj), MA_View_NumSelected, &numS);
								DB(("bitRocky-Test: numS = %ld\n", numS));
								if (numS == 1)
								{
									ULONG act;

									get(obj, MUIA_List_Active, &act);
									if (act != MUIV_List_Active_Off) // shouldn't occur, but hey
									{
										DoMethod(obj, MUIM_List_Select, MUIV_List_Select_All, MUIV_List_Select_Off, NULL);
										//set(_parent(obj), MA_View_NumSelected, 1);
									}
								}
							}
						}
					}
					break;

					case SELECTUP:
					{
						DB(("bitRocky-Test: select up\n"));
						if (msg->imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
						{
							data->multiSelect = TRUE;
						}
						else
						{
							data->multiSelect = (getv(_parent(obj), MA_View_NumSelected) > 1);
							DB(("bitRocky-Test: w/o shift, data->multiSelect = %ld\n", data->multiSelect));
						}
					}
					break;
#if 0
					case SELECTDOWN:
					{
						data->click_x = msg->imsg->MouseX;
						data->click_y = msg->imsg->MouseY;
					}
					break;

					case SELECTUP:
					{
						data->click_x = -1;
						data->click_y = -1;
					}
					break;
#endif

					default: ;
				}
			}
			break;

#if 0
			case IDCMP_MOUSEMOVE:
			{
/* custom dnd stuff, just for experiment */
				if ( data->click_x != -1 && data->click_y != -1 )
				{
					if(msg->imsg->MouseX <= 5 || msg->imsg->MouseX >= _window(obj)->Width-15 )
					{
						/*
						handle_drag(data->list, NULL, NULL, data->click_x, data->click_y);
						data->click_x = data->click_y = -1;
						return (MUI_EventHandlerRC_Eat);
						*/

						struct MUI_DragImage *di = DoMethod(data->list, MUIM_List_CreateDragMap, data->click_x, data->click_y, 0);

						if(di)
						{
							handle_drag(data->list, di, NULL, 0, 0);

							/*
							struct MUI_DragImage *di;

							if (di = malloc(sizeof(*di)))
							{
								APTR tbm = NULL;

								memclr(di, sizeof(*di));

								di->dragmode = DD_TRANSPARENT;
								if (tbm = gfx_bitmap_create(gfx_bitmap_width(data->defaultfile_bitmap), gfx_bitmap_height(data->defaultfile_bitmap), 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE))
								{
									LONG tx, ty;

									gfx_blit(data->defaultfile_bitmap, tbm, TAG_DONE);
									di->bm = tbm;

									di->width = gfx_bitmap_width(data->defaultfile_bitmap);
									di->height = gfx_bitmap_height(data->defaultfile_bitmap);

									gfx_alpha_compose(tbm, 0, 0, gfx_bitmap_width(data->defaultfile_bitmap), gfx_bitmap_height(data->defaultfile_bitmap), TRANSPARENT_VAL);

									di->touchx = 0;
									di->touchy = 0;

									handle_drag(data->list, di, NULL, 0, 0);

									gfx_bitmap_delete(di->bm);

									data->click_x = data->click_y = -1;
								}
								free(di);
							}
							*/

							return (MUI_EventHandlerRC_Eat);
						}
				}
			}
			break;
#endif
		}
	}

	return 0;
}

/*
 * Maps list indexes to alines entries
 */
DEFTMETHOD(Listview_UpdateEntriesPositions)
{
	GETDATA;
	int i;
	struct aline * al;

	for(i=0;;i++)
	{
		DoMethod(data->list, MUIM_List_GetEntry, i, &al);

		if(al)
		{
			if(al->obj)
			{
				set(al->obj, MA_ListviewEntry_Position, i);
			}
		}
		else
		{
			break;
		}
	}

	return (0);
}

/*
 * Updates the view.
 */

static int cmp_name( STRPTR *n1, STRPTR *n2)
{
	return StrnCmp(locale, *n1, *n2, -1, SC_ASCII);
}

DEFSMETHOD(Listview_UpdateView)
{
	GETDATA;

	set(data->list, MUIA_List_Quiet, TRUE);

	if(msg->mode == MV_Listview_UpdateView_Normal)
	{
		DoMethod(data->list, MUIM_List_Clear);
	}

	DoMethod(obj, MM_Listview_RemoveFiles, 0);

	if (data->a != NULL)
	{
		ULONG num = 0;

		DoMethod(data->list, MUIM_List_Insert, data->a, -1, MUIV_List_Insert_Sorted);
		FreeVecPooled(data->apool, data->a);
		data->a = NULL;
		data->asize = 0;

		/* lookup in optional selectionlist */

		if ( data->initialselection != NULL )
		{
			ULONG i, n;

			/* sort array by name */

			for(n=0; data->initialselection[ n ] != NULL ;n++) {}

			qsort(data->initialselection, n, sizeof( APTR ), (const void *)cmp_name);

			for(i=0;;i++)
			{
				struct aline *al;
				DoMethod(data->list, MUIM_List_GetEntry, i, &al);

				if(al != NULL)
				{
					APTR select = bsearch( &al->name, data->initialselection, n, sizeof( APTR ), (const void *)cmp_name);

					if ( select )
					{
						al->active = TRUE;
						num++;

						DoMethod(data->list, MUIM_List_Select, i, MUIV_List_Select_On, NULL);
					}
				}
				else
				{
					break;
				}
			}

			for(i=0; data->initialselection[ i ] != NULL ;i++)
				name_delete( data->initialselection[ i ] );

			FreeVecTaskPooled( data->initialselection );
			data->initialselection = NULL;
		}
	}

	DoMethod(obj, MM_Listview_UpdateEntriesPositions);

	DoMethod(data->list, MM_Listviewlist_SelectChange);

	set(data->list, MUIA_List_Quiet, FALSE);

	SetWindowTitle(cl, obj, MF_View_SetStatus_Window);

	return (0);
}


DEFTMETHOD(Listview_Clear)
{
	GETDATA;
	struct aline * al, * nextal;
	struct listview_remove_node * n, * nextn;

	set(data->list, MUIA_List_Quiet, TRUE);
	DoMethod(data->list, MUIM_List_Clear);
	set(data->list, MUIA_List_Quiet, FALSE);

#if WIPEOUT_FRIENDLY
	ITERATELISTSAFE(al, nextal, &data->alist)
	{
		listview_deleteentry(data->apool, al);
	}

	ITERATELISTSAFE(n, nextn, &data->rlist)
	{
		FreeVecPooled(data->apool, n);
	}
#else
	ITERATELIST(al, &data->alist)
	{
		if(al->obj)
		{
			MUI_DisposeObject(al->obj);
			al->obj = NULL;
		}
	}

	ITERATELIST(n, &data->rlist)
	{
		if(n->entrynode->obj)
		{
			MUI_DisposeObject(n->entrynode->obj);
			n->entrynode->obj = NULL;
		}
	}
#endif

	NEWLIST(&data->alist);
	NEWLIST(&data->rlist);

	/* XXX: Seems it throws wipeout hits although all the referenced memory in apool is freed (i hope), weird */
#if !WIPEOUT_FRIENDLY
	FlushPool(data->apool);
#endif
	data->cnt = 0;
	data->asize = 0;
	data->a = NULL;

	DoMethod(obj, MM_Listview_UpdateView, MV_Listview_UpdateView_Normal);

	return (0);
}


DEFDISPOSE
{
	GETDATA;

	/* initialselection might be still here */

	if ( data->initialselection != NULL )
	{
		LONG i;
		for(i=0; data->initialselection[ i ] != NULL ;i++)
			name_delete( data->initialselection[ i ] );

		FreeVecTaskPooled( data->initialselection );
		data->initialselection = NULL;
	}

	CloseLibrary(data->LocalUserGroupBase);

	listview_enable_dosnotify(data->path, FALSE);

	if(data->lv_format.string_format)
	{
		free(data->lv_format.string_format);
		data->lv_format.string_format = NULL;
	}

	if (data->cmenu)
	{
		MUI_DisposeObject(data->cmenu);
		data->cmenu = NULL;
	}

	if (data->nctx)
	{
		notify_delete(data->nctx);
	}

	if (data->apool)
	{
		struct aline * al, * nextal;
		struct listview_remove_node * n, * nextn;

#if WIPEOUT_FRIENDLY
		ITERATELISTSAFE(al, nextal, &data->alist)
		{
			listview_deleteentry(data->apool, al);
		}

		ITERATELISTSAFE(n, nextn, &data->rlist)
		{
			FreeVecPooled(data->apool, n);
		}
#else
		ITERATELIST(al, &data->alist)
		{
			if(al->obj)
			{
				MUI_DisposeObject(al->obj);
				al->obj = NULL;
			}
		}

		ITERATELIST(n, &data->rlist)
		{
			if(n->entrynode->obj)
			{
				MUI_DisposeObject(n->entrynode->obj);
				n->entrynode->obj = NULL;
			}
		}
#endif
		DeletePool(data->apool);
		data->apool = NULL;
	}

	if (data->npool)
	{
		DeletePool(data->npool);
		data->npool = NULL;
	}

	return (DOSUPER);
}

DEFTMETHOD(View_LoadURI)
{
	GETDATA;
	STRPTR scheme;
	CONST_STRPTR type = "";
	STRPTR path = (STRPTR) getv(_view(obj), MA_View_Path);

	LONG mode;

	if ( data->focuspath )
	{
		name_delete( data->focuspath );
		data->focuspath = NULL;
	}

	/* check if we are going one level up */

	if ( is_path_parent( path, data->path ) )
	{
		data->focuspath = name_build( data->path );
	}

	/* check if we are just reloading. (XXX: This will do IO ops, but it should be safe here) */

	if ( is_path_equal( path, data->path ) && data->initialselection == NULL )
	{
		LONG i;
		struct Rect32 area;
		DoMethod( obj, MM_View_QueryDisplayArea, &area );
		data->focuspos = area.MinY;

		data->initialselection = (STRPTR*)DoMethod( obj, MM_View_PickSelected );

		/* process array to contain names, not objects (objects will be disposed later). */
	
		for(i=0; data->initialselection != NULL && data->initialselection[ i ] != NULL; i++)
			data->initialselection[ i ] = name_build( FilePart( (STRPTR)getv( data->initialselection[ i ], MA_Icon_Path ) ) );
	}
	else
	{
		data->focuspos = 0;

		/* initialselection might be still here */

		if ( data->initialselection != NULL )
		{
			LONG i;
			for(i=0; data->initialselection[ i ] != NULL ;i++)
				name_delete( data->initialselection[ i ] );

			FreeVecTaskPooled( data->initialselection );
			data->initialselection = NULL;
		}
	}

	stccpy( data->path, path, sizeof( data->path ) );

	/* set window size and position */

	if (DoSuperMethod(cl, obj, MM_View_ParseWindowArgs, &type, &mode, &data->mode_set_by_uri, &data->win_width, &data->win_height, &data->sort_set_by_uri, &data->lv_format.sort_column, &data->lv_format.sort_mode))
	{
		if(data->mode_set_by_uri)
		{
			data->lv_format.view_mode = mode;
		}
/*
		if(data->sort_set_by_uri) // bitRocky
		{
			SetAttrs(data->list, MUIA_List_Format, data->lv_format.string_format,
				MA_Listview_SortColumn, data->lv_format.sort_column,
				MA_Listview_SortDirection, data->lv_format.sort_mode,
				TAG_DONE
			);
		}
*/
	}
	// bitRocky:
	DB(("after MM_View_ParseWindowArgs(): data->lv_format.sort_column = %ld, data->lv_format.sort_mode = %ld\n", data->lv_format.sort_column, data->lv_format.sort_mode));

	scheme = (STRPTR)getv(_view(obj), MA_View_Scheme);

	if(scheme)
	{
		/* choose mode from the scheme */
		if(!stricmp("devices", scheme))
		{
			data->showfiles   = FALSE;
			data->showdevices = TRUE;
		}
		else if(!stricmp("file", scheme))
		{
			data->showfiles   = TRUE;
			data->showdevices = FALSE;
		}
		else if(!stricmp("vfs", scheme))
		{
			APTR fscontext;

			if((fscontext = vfs_open(path , vfs_lookup(type))))
			{
				STRPTR buffer = (STRPTR) malloc(strlen(vfs_device(fscontext))+2);

				if(buffer)
				{
					sprintf(buffer, "%s:", vfs_device(fscontext));
					SetAttrs(_view(obj), MA_View_Path, buffer,
						MA_View_PreviousPath, buffer,
						MA_View_Scheme, "file",
						TAG_DONE
					);
					free(buffer);
				}

				data->showfiles   = TRUE;
				data->showdevices = FALSE;
			}
		}

		data->viewmode_postpone = -1;

		listview_abort_threads(obj, data);
	}

	return (0);
}

/*
 * Displays the assigns, volumes, etc..
 */

ULONG tr_listview_showdevices(APTR obj UNUSED, APTR d)
{
	struct Data * data = (struct Data *) d;

	data->thread_showdevices_busy = TRUE;

	return TRUE;
}

static void listview_showdevices(struct IClass * cl, APTR obj, struct Data * data)
{
	struct dlcnode *n;
	struct aline *al;
	ULONG i = 0;
	ULONG type = DLT_VOLUME;
	UQUAD diskusage = 0;
	UQUAD selecteddiskusage = 0;

	for (i = 0; i < ((_conf(fl_assign_display))?2:1); i++)
	{
		ITERATEDLC(n)
		{
			if(n->state == DLC_NEW || n->state == DLC_ADDED)
			{
				if (n->type == type)
				{
					if ((al = AllocVecPooled(data->apool, sizeof(*al))))
					{
						al->nodetype = FLT_DEVICES;
						al->invalidate_cache = 0; // bitRocky
						al->invalidate_mimetype = 0; // bitRocky
						al->active = 0;
						al->obj    = NULL;
						al->date   = NULL;
						al->time   = NULL;

						al->namelen = strlen(n->name);

						if ((al->name = AllocVecPooled(data->apool, al->namelen + 1)))
						{
							strcpy(al->name, n->name);
							al->type = n->type;

							if (type == DLT_DIRECTORY)
							{
								al->un.dev.devname = NULL;

								al->un.dev.total   = 0;
								al->un.dev.full    = 0;
								al->un.dev.dostype = 0;
							}
							else
							{
								struct dlcnode *dln;

								if ((dln = doslistcache_find_dlcdevice(n->mp)))
								{
									TEXT t[32]; /* should be enough (tm) */

									al->un.dev.devnamelen = strlen(dln->name) + 2; /* () around the names */
									if ((al->un.dev.devname = AllocVecPooled(data->apool, al->un.dev.devnamelen + 1)))
									{
										struct devinfo64 di;

										al->un.dev.devname[0] = '(';
										strcpy(al->un.dev.devname + 1, dln->name);
										al->un.dev.devname[al->un.dev.devnamelen - 1] = ')';
										al->un.dev.devname[al->un.dev.devnamelen - 0] = 0;

										stccpy(t, al->un.dev.devname, al->un.dev.devnamelen + 1);

										snprintf(t, sizeof(t), "%s:", dln->name);

										if (info64(t, &di))
										{
											/* XXX: we should test di_DiskState too */
											al->un.dev.total = di.di_NumBlocks * di.di_BytesPerBlock;
											al->un.dev.full  = di.di_NumBlocksUsed * di.di_BytesPerBlock;
										}
										else
										{
											al->un.dev.total = 0;
											al->un.dev.full  = 0;
										}
									}
									al->un.dev.dostype = dln->disktype;
								}
								else
								{
									al->un.dev.devname = NULL;
									al->un.dev.dostype = 0;
								}
								//al->un.dev.dostype = n->disktype; /* XXX: remove once fixed */

							}

							al->obj = NewObject(getlistviewentryclass(), NULL, TAG_DONE);
							if(al->obj)
							{
								TEXT path[al->namelen+2];

								/* add ':' to path */
								stccpy(path, al->name, sizeof(path));

								path[al->namelen] = ':';
								path[al->namelen+1] = 0;


								SetAttrs(al->obj, MA_ListviewEntry_Pool, data->apool,
									MA_ListviewEntry_ListObject, data->list,
									MA_Icon_Path, path,
									MA_Icon_Type, dostype2icontype(al->type, al->nodetype, FALSE),
									MA_Icon_FileType, dostype2filetype(al->type, al->nodetype, FALSE),
								TAG_DONE
								);
							}

							ADDTAIL(&data->alist, al);
							data->cnt++;
							continue;
						}
						/* XXX: ouch */
					}
					/* XXX: ouch */
				}
			}
		}
		type = DLT_DIRECTORY;
	}

	set(data->list, MA_Listviewlist_Path, data->path);

	DoMethod(obj, MM_Listview_AddFiles, MV_Listview_AddFiles_All);
	DoMethod(obj, MM_Listview_UpdateView, MV_Listview_UpdateView_Normal);
	DoMethod(data->list, MUIM_List_Sort);

	DoSuperMethod(cl, obj, MM_View_SetStatus, MF_View_SetStatus_Window, "%s", dprefs_mymorphos_name_get());

	SetAttrs(obj, MA_View_TotalFiles, 0, MA_View_TotalDirs, 0, MA_View_Links, 0, MA_View_IconFiles, 0,
	              MA_View_DiskUsage, &diskusage, MA_View_SelectedDiskUsage, &selecteddiskusage,
	              TAG_DONE);
}

DEFTMETHOD(Listview_ShowDevices)
{
	GETDATA;
	LONG sort_column = data->lv_format.sort_column; // bitRocky: backup sort_col and sort_mode
	LONG sort_mode = data->lv_format.sort_mode;

	if(data->type != FLT_DEVICES)
	{
		data->load_default_format = TRUE;
	}

	data->type = FLT_DEVICES;

	set(data->list, MA_Listviewlist_Type, data->type);

	/* XXX: PROGDIR: becomes current path in device mode. it has annoying sideeffects */

	DoMethod(obj, MM_Listview_Clear);

	if(!data->in_notify)
	{
		set(obj, MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None); 

		listview_enable_dosnotify((STRPTR) getv(obj, MA_View_Path), FALSE);
		if(listview_retrieve_format("devices://", &data->lv_format, data->type))
		{
			data->load_default_format = FALSE;
			/* bitRocky: if uri sets the sort column/order, then enforce it */
			if (data->sort_set_by_uri)
			{
				data->lv_format.sort_column = sort_column;
				data->lv_format.sort_mode = sort_mode;
			}

			SetAttrs(data->list, MUIA_List_Format, data->lv_format.string_format,
				MA_Listview_SortColumn, data->lv_format.sort_column,
				MA_Listview_SortDirection, data->lv_format.sort_mode,
				TAG_DONE
			);
		}
		else
		{
			if(data->load_default_format)
			{
				listview_parse_format(_conf(fl_default_format_devices), _conf(fl_default_mode_devices), &data->lv_format, data->type);

				/* bitRocky: if uri sets the sort column/order, then enforce it */
				if (data->sort_set_by_uri)
				{
					data->lv_format.sort_column = sort_column;
					data->lv_format.sort_mode = sort_mode;
				}

				SetAttrs(data->list, MUIA_List_Format, data->lv_format.string_format,
					MA_Listview_SortColumn, data->lv_format.sort_column,
					MA_Listview_SortDirection, data->lv_format.sort_mode,
					TAG_DONE
				);
				data->load_default_format = FALSE;
			}
		}

		data->icondisplay = listview_is_shown(LISTVIEW_DEVICE_COL_ICON, &data->lv_format, data->type);

		if (data->nctx)
		{
			notify_unregister(data->nctx,
				NOTIFYTAG_Monitor_Device, NOTIFYVAL_Monitor_Device_ClearAll,
			TAG_DONE);
		}
		else
		{
			data->nctx = notify_create();
		}

		if (data->nctx)
		{
			notify_register(data->nctx,
				NOTIFYTAG_Monitor_Device, "*",
				NOTIFYTAG_Monitor_Device_Mount, TRUE,
				NOTIFYTAG_Monitor_Device_UnMount, TRUE,
				NOTIFYTAG_Monitor_Device_Name, TRUE,
				NOTIFYTAG_Inform_Object, obj,
			TAG_DONE);
		}
	}

	do_action(obj, TA_Listview_ShowDevices, TT_Listview_Data, (APTR) data, TAG_DONE);

	DoSuperMethod(cl, obj, MM_View_SetStatus, MF_View_SetStatus_Window | MF_View_SetStatus_Bar | MF_View_SetStatus_Busy, "Scanning location...");

	return (0);
}

DEFTMETHOD(Listview_ShowFiles)
{
	GETDATA;

	ULONG scanmode;
	LONG defmode = data->lv_format.view_mode; /* back up default mode */
	LONG sort_column = data->lv_format.sort_column; // bitRocky: backup sort_col and sort_mode
	LONG sort_mode = data->lv_format.sort_mode;

DB(("data->type = %ld (FLT_FILES = %ld), ->load_default_format = %ld\n", data->type, FLT_FILES, data->load_default_format)); // bitRocky
	if(data->type != FLT_FILES)
	{
		data->load_default_format = TRUE;
	}
DB(("data->lv_format.sort_column = %ld, ->lv_format.sort_mode = %ld, ->load_default_format = %ld\n", data->lv_format.sort_column, data->lv_format.sort_mode, data->load_default_format)); // bitRocky

	data->type = FLT_FILES;
	set(data->list, MA_Listviewlist_Type, data->type);

	set(obj, MUIA_Listview_MultiSelect, _conf(fl_selection_mode) == 0 ? MUIV_Listview_MultiSelect_Default : MUIV_Listview_MultiSelect_Always);
	DoMethod(obj, MM_Listview_Clear);

	listview_enable_dosnotify((STRPTR) getv(obj, MA_View_Path), TRUE);

DB(("before listview_retrieve_format(): data->lv_format.sort_column/mode = %ld/%ld\n", data->lv_format.sort_column, data->lv_format.sort_mode)); // bitRocky
	if(listview_retrieve_format((STRPTR)getv(obj, MA_View_Path), &data->lv_format, data->type))
	{
		data->load_default_format = FALSE; /* XXX: should we inherit a snapshot mode or not ? */
DB(("after listview_retrieve_format(): data->lv_format.sort_column/mode = %ld/%ld\n", data->lv_format.sort_column, data->lv_format.sort_mode)); // bitRocky
DB(("mode_set_by_uri = %ld\n", data->mode_set_by_uri));
DB(("sort_set_by_uri = %ld\n", data->sort_set_by_uri)); // bitRocky

		/* if uri set the mode, then enforce it */
		if(data->mode_set_by_uri) data->lv_format.view_mode = defmode;

		/* bitRocky: if uri sets the sort column/order, then enforce it */
		if (data->sort_set_by_uri)
		{
			data->lv_format.sort_column = sort_column;
			data->lv_format.sort_mode = sort_mode;
		}
DB(("before SetAttrs(): data->lv_format.sort_column/mode = %ld/%ld\n", data->lv_format.sort_column, data->lv_format.sort_mode)); // bitRocky
		SetAttrs(data->list, MUIA_List_Format, data->lv_format.string_format,
			MA_Listview_SortColumn, data->lv_format.sort_column,
			MA_Listview_SortDirection, data->lv_format.sort_mode,
			TAG_DONE
		);
	}
	else
	{
		if(data->load_default_format)
		{
			/* we set format from preferences first */
DB(("before listview_parse_format(): data->lv_format.sort_column/mode = %ld/%ld\n", data->lv_format.sort_column, data->lv_format.sort_mode)); // bitRocky
			listview_parse_format(_conf(fl_default_format_files), _conf(fl_default_mode_files), &data->lv_format, data->type);

DB(("mode_set_by_uri = %ld\n", data->mode_set_by_uri));
DB(("sort_set_by_uri = %ld\n", data->sort_set_by_uri)); // bitRocky
			
			/* if uri set the mode, then enforce it */
			if(data->mode_set_by_uri) data->lv_format.view_mode = defmode;

			/* bitRocky: if uri sets the sort column/order, then enforce it */
			if (data->sort_set_by_uri)
			{
				data->lv_format.sort_column = sort_column;
				data->lv_format.sort_mode = sort_mode;
			}

DB(("before SetAttrs(): data->lv_format.sort_column/mode = %ld/%ld\n", data->lv_format.sort_column, data->lv_format.sort_mode)); // bitRocky
			SetAttrs(data->list, MUIA_List_Format, data->lv_format.string_format,
				MA_Listview_SortColumn, data->lv_format.sort_column,
				MA_Listview_SortDirection, data->lv_format.sort_mode,
				TAG_DONE
			);
			data->load_default_format = FALSE;
		}
	}

	if(data->viewmode_postpone != -1)
		data->lv_format.view_mode = data->viewmode_postpone;

	data->icondisplay     = listview_is_shown(LISTVIEW_FILE_COL_ICON, &data->lv_format, data->type);
	data->versiondisplay  = listview_is_shown(LISTVIEW_FILE_COL_VERSION, &data->lv_format, data->type);
	data->filetypedisplay = listview_is_shown(LISTVIEW_FILE_COL_FILETYPE, &data->lv_format, data->type);
	data->md5display      = listview_is_shown(LISTVIEW_FILE_COL_MD5, &data->lv_format, data->type);

	/*
	 * Stop notification monitor (or create new one if doesn't exist) before loading new URI.
	 */
	if (data->nctx)
	{
		notify_unregister(data->nctx,
			NOTIFYTAG_Monitor_File, NOTIFYVAL_Monitor_File_ClearAll,
		TAG_DONE);
	}
	else
	{
		data->nctx = notify_create();
	}

	if (data->nctx)
	{
		TEXT pat[PATH_SIZE];

		name_quotepattern((STRPTR)getv(obj, MA_View_Path), pat, sizeof(pat)); /* XXX: that sucks.. and what about "" paths ? */
		AddPart(pat, "*", PATH_SIZE);

		notify_register(data->nctx,
			NOTIFYTAG_Monitor_File, pat,
			NOTIFYTAG_Monitor_File_Name, TRUE,
			NOTIFYTAG_Monitor_File_Create, TRUE,
			NOTIFYTAG_Monitor_File_Delete, TRUE,
			NOTIFYTAG_Monitor_File_Comment, TRUE,
			//NOTIFYTAG_Monitor_File_Size, TRUE,
			NOTIFYTAG_Monitor_File_Flags, TRUE,
			//NOTIFYTAG_Monitor_File_UID, TRUE,
			//NOTIFYTAG_Monitor_File_GID, TRUE,
			NOTIFYTAG_Monitor_File_Date, TRUE,
			NOTIFYTAG_Monitor_Enable, TRUE,
			NOTIFYTAG_Inform_Object, obj,
		TAG_DONE);
	}

	set(data->list, MA_Listviewlist_Path, data->path/*getv(obj, MA_View_Path)*/);

	switch(data->lv_format.view_mode)
	{
		case LVM_ICONS:
			scanmode = TV_File_ScanDir_Mode_FilesIcons;
			data->thumbdisplay = FALSE;
			break;
		case LVM_THUMBS:
			scanmode = TV_File_ScanDir_Mode_Files;
			data->thumbdisplay = TRUE;
			break;
		default:
		case LVM_SHOWALL:
			scanmode = TV_File_ScanDir_Mode_Files;
			data->thumbdisplay = FALSE;
			break;
	}

	do_action(obj, TA_File_ScanDir,
		TT_File_ScanDir_Path, getv(obj, MA_View_Path),
		TT_File_ScanDir_Mode, scanmode,
	TAG_DONE);

	DoSuperMethod(cl, obj, MM_View_SetStatus, MF_View_SetStatus_Window | MF_View_SetStatus_Bar | MF_View_SetStatus_Busy, "Scanning location...");

	return (0);
}

void SetWindowTitle(struct IClass *cl, Object *obj, ULONG flags)
{
	TEXT wintitle[PATH_SIZE];
	TEXT resolved_path[PATH_SIZE];

	struct devinfo64 di;
	STRPTR path;
	TEXT size[ 16 ], free[ 16 ], used[ 16 ];
	long full;

	path = (STRPTR)getv(obj, MA_View_Path);
	if (path == NULL || *path == 0)
		return;

	vfs_resolve_path(path, resolved_path, sizeof(resolved_path), FALSE);

	if (name_build_wintitle(wintitle, sizeof(wintitle), resolved_path) && info64(path, &di))
	{
		capacity_format_size(size, sizeof(size), di.di_NumBlocks * di.di_BytesPerBlock); // from devices.c
		capacity_format_size(free, sizeof(free), (di.di_NumBlocks - di.di_NumBlocksUsed) * di.di_BytesPerBlock); // from devices.c
		capacity_format_size(used, sizeof(used), di.di_NumBlocksUsed * di.di_BytesPerBlock);
		full = (100 * di.di_NumBlocksUsed) / di.di_NumBlocks;

		DoSuperMethod(cl, obj, MM_View_SetStatus, flags, GSI( MSG_LISTVIEWCLASS_WINTITLE), wintitle, free, used, size, full);
	}else{
		DoSuperMethod(cl, obj, MM_View_SetStatus, flags, wintitle);
	}
}

DEFSMETHOD(Thread_Finished)
{
	GETDATA;

	if (msg->status == MV_Thread_Finished_Abort)
	{
		ULONG callsuper = TRUE;

		//DB(("action %d aborted\n", msg->action));

		switch ( msg->action )
		{
			case TA_Version_Find:
				{
					data->thread_version_busy = FALSE;

					callsuper = FALSE;

					//DB(("TA_Version_Find aborted\n"));

					if(data->startnewthread_version)
					{
						//DB(("TA_Version_Find restarting\n"));

						data->startnewthread_version = FALSE;
						DoMethod(obj, MM_Listview_RunVersionThread);
					}
				}
				break;

			case TA_File_MD5sum:
				{
					data->thread_md5_busy = FALSE;

					callsuper = FALSE;

					//DB(("TA_File_MD5sum aborted\n"));

					if(data->startnewthread_md5)
					{
						//DB(("TA_File_MD5sum restarting\n"));

						data->startnewthread_md5 = FALSE;
						DoMethod(obj, MM_Listview_RunMD5Thread);
					}
				}
				break;

			case TA_File_GetSizes:
				{
					data->thread_sizes_busy = FALSE;

					callsuper = FALSE;

					//DB(("TA_File_GetSizes aborted\n"));

					if(data->startnewthread_sizes)
					{
						//DB(("TA_File_GetSizes restarting\n"));

						data->startnewthread_sizes = FALSE;
						DoMethod(obj, MM_Listview_RunDirSizeThread);
					}
				}
				break;

			case TA_MimeType_Scan:
				{
					data->thread_scantype_busy = FALSE;

					callsuper = FALSE;

					//DB(("TA_MimeType_Scan aborted\n"));

					if(data->startnewthread_scantype)
					{
						//DB(("TA_MimeType_Scan restarting\n"));

						data->startnewthread_scantype = FALSE;
						DoMethod(obj, MM_Listview_RunFileTypeThread);
					}
				}
				break;

			case TA_Listview_CreateIcons:
				{
					data->thread_createicons_busy = FALSE;

					callsuper = FALSE;

					//DB(("TA_Listview_CreateIcons aborted\n"));

					if(data->startnewthread_createicons)
					{
						//DB(("TA_Listview_CreateIcons restarting\n"));

						data->startnewthread_createicons = FALSE;
						DoMethod(obj, MM_Listview_RunIconThread, data->icondisplay, TRUE);
					}
				}
				break;

			case TA_Listview_ShowPreview:
				{
					data->thread_preview_busy = FALSE;

					callsuper = FALSE;

					//DB(("TA_Listview_ShowPreview aborted\n"));

					if(data->startnewthread_preview)
					{
						//DB(("TA_Listview_ShowPreview restarting\n"));

						data->startnewthread_preview = FALSE;
						DoMethod(obj, MM_Listview_RunPreviewThread);
					}
				}
				break;

			case TA_File_ScanDir:
				{
					data->thread_scandir_busy = FALSE;

					callsuper = FALSE;

					//DB(("TA_File_ScanDir aborted\n"));

					if (data->viewmode_postpone != -1 && data->viewmode_postpone != data->lv_format.view_mode)
					{
						data->lv_format.view_mode = data->viewmode_postpone;
						data->showfiles   = TRUE;
						data->showdevices = FALSE;
					}
				}
				break;

			case TA_Listview_ShowDevices:
				{
					data->thread_showdevices_busy = FALSE;

					callsuper = FALSE;

					//DB(("TA_Listview_ShowDevices aborted\n"));
				}
				break;
		}

		DB(("aborted/scandir=%d showdevices=%d createicons=%d scantype=%d version=%d sizes=%d preview=%d md5=%d\n",
				data->thread_scandir_busy, data->thread_showdevices_busy, data->thread_createicons_busy,
				data->thread_scantype_busy, data->thread_version_busy,
				data->thread_sizes_busy, data->thread_preview_busy, data->thread_md5_busy
		  ));

		DB(("showfiles : %d showdevices : %d\n", data->showfiles, data->showdevices));

		if(!data->thread_scandir_busy && !data->thread_showdevices_busy && !data->thread_scantype_busy &&
		   !data->thread_createicons_busy && !data->thread_version_busy &&
		   !data->thread_sizes_busy && !data->thread_preview_busy && !data->thread_md5_busy)
		{
			if(data->showfiles && msg->action > 0)
			{
				//DB(("ShowFiles\n"));
				data->showfiles   = FALSE;
				data->showdevices = FALSE;
				DoMethod(obj, MM_Listview_ShowFiles);
				return (0);
			}
			else if(data->showdevices && msg->action > 0)
			{
				//DB(("ShowDevices\n"));
				data->showfiles   = FALSE;
				data->showdevices = FALSE;
				DoMethod(obj, MM_Listview_ShowDevices);
				return (0);
			}
			else
			{
				if(data->cleanupicons)
				{
					data->cleanupicons = FALSE;

					DoMethod(obj, MM_Listview_CleanupIcons);
					DoMethod(obj, MM_Listview_DeleteDefaultImages);
				}

				callsuper = TRUE;
			}
		}

		if ( callsuper )
			return (DOSUPER);
		else
			return 0;
	}
	else
	{
		//DB(("action %d finished\n", msg->action));

		switch ( msg->action )
		{
			case TA_Version_Find:
				{
					DoMethod(data->list, MUIM_List_Redraw, MUIV_List_Redraw_All);

					data->thread_version_busy = FALSE;
				}
				break;

			case TA_File_MD5sum:
				{
					DoMethod(data->list, MUIM_List_Redraw, MUIV_List_Redraw_All);

					data->thread_md5_busy = FALSE;
				}
				break;

			case TA_File_GetSizes:
				{
					DoMethod(data->list, MUIM_List_Redraw, MUIV_List_Redraw_All);

					listview_update_statusbar_info(obj, data);

					data->thread_sizes_busy = FALSE;
				}
				break;

			case TA_MimeType_Scan:
				{
					DoMethod(data->list, MUIM_List_Redraw, MUIV_List_Redraw_All);

					data->thread_scantype_busy = FALSE;
				}
				break;

			case TA_Listview_CreateIcons:
				{
					DoMethod(data->list, MUIM_List_Redraw, MUIV_List_Redraw_All);

					data->thread_createicons_busy = FALSE;
				}
				break;

			case TA_Listview_ShowPreview:
				{
					DoMethod(data->list, MUIM_List_Redraw, MUIV_List_Redraw_All);

					data->thread_preview_busy = FALSE;
				}
				break;

			case TA_File_ScanDir:
				{
					SetWindowTitle(cl, obj, MF_View_SetStatus_Window);

					DoMethod(obj, MM_Listview_AddFiles, MV_Listview_AddFile_Normal);
					DoMethod(obj, MM_Listview_UpdateView, MV_Listview_UpdateView_Normal);
					DoMethod(data->list, MUIM_List_Sort);

					listview_update_statusbar_info(obj, data);

					DoMethod(obj, MM_Listview_RunIconThread, data->icondisplay, TRUE);
					DoMethod(obj, MM_Listview_RunFileTypeThread);
					DoMethod(obj, MM_Listview_RunVersionThread);
					DoMethod(obj, MM_Listview_RunMD5Thread);
					DoMethod(obj, MM_Listview_RunDirSizeThread);

					if ( data->focuspath )
					{
						ULONG l = strlen( data->focuspath );
						if ( l && data->focuspath[ l - 1 ] == '/' )
							data->focuspath[ l - 1 ] = 0;

						DoMethod( obj, MM_View_Focus, FALSE, FilePart(data->focuspath), FALSE );
						name_delete( data->focuspath );
						data->focuspath = NULL;
					}

					if ( data->focuspos != 0 )
					{
						set( data->list, MUIA_List_TopPixel, data->focuspos );
						data->focuspos = 0;
					}

					data->thread_scandir_busy = FALSE;
				}
				break;

			case TA_Listview_ShowDevices:
				{
					listview_showdevices(cl, obj, data);

					DoMethod(obj, MM_Listview_RunIconThread, data->icondisplay, TRUE);

					data->thread_showdevices_busy = FALSE;
				}
				break;
		}

		DB(("finished/ scandir : %d showdevices : %d createicons : %d scantype : %d version : %d sizes : %d preview : %d md5 : %d\n",
				data->thread_scandir_busy, data->thread_showdevices_busy, data->thread_createicons_busy,
				data->thread_scantype_busy, data->thread_version_busy,
				data->thread_sizes_busy, data->thread_preview_busy,
				data->thread_md5_busy
		  ));

		DB(("showfiles : %d showdevices : %d\n", data->showfiles, data->showdevices));

		if(!data->thread_scandir_busy && !data->thread_showdevices_busy && !data->thread_scantype_busy &&
		   !data->thread_createicons_busy && !data->thread_version_busy &&
		   !data->thread_sizes_busy && !data->thread_preview_busy && !data->thread_md5_busy)
		{
			if(data->showfiles && msg->action > 0)
			{
				//DB(("ShowFiles\n"));
				data->showfiles   = FALSE;
				data->showdevices = FALSE;
				DoMethod(obj, MM_Listview_ShowFiles);
				return (0);
			}
			else if(data->showdevices && msg->action > 0)
			{
				//DB(("ShowDevices\n"));
				data->showfiles   = FALSE;
				data->showdevices = FALSE;
				DoMethod(obj, MM_Listview_ShowDevices);
				return (0);
			}
		}
	}

	return (0);
}

DEFSMETHOD(Notify_Change)
{
	GETDATA;

	struct notifyact *na;
	BOOL restart = TRUE;

	data->in_notify = TRUE;

	if(data->update)
	{
		set(data->list, MUIA_List_Quiet, TRUE);
	}

	while ((na = notify_action_get(msg->ctx)))
	{
		switch (na->action)
		{
			case NOTIFYTAG_Monitor_Enable:
				{
					struct notifyact_file_enable *naf = (struct notifyact_file_enable *)na;
					data->update = naf->enable;
				}
				break;

				case NOTIFYTAG_Monitor_Device_Name:
					{
						struct aline * al;
						ULONG len = 0;
						TEXT uri[PATH_SIZE];
						TEXT nuri[PATH_SIZE];

						struct notifyact_device_name *naf = (struct notifyact_device_name *)na;

						stccpy(uri, naf->uri, sizeof(uri));
						len = strlen(uri);
						if (len && uri[len - 1] == ':')
							uri[len - 1] = '\0';

						data->notify_filerename = TRUE;

						ITERATELIST(al, &data->alist)
						{
							if(!stricmp(al->name, uri))
							{
								STRPTR newname;
								ULONG  newlen = 0;

								stccpy(nuri, naf->newname, sizeof(nuri));
								newlen  = strlen(nuri);
								if ( newlen && nuri[ newlen - 1 ] == ':' )
									nuri[newlen - 1] = '\0';

								newname = AllocVecPooled(data->apool, newlen + 1);

								if (newname)
								{
									stccpy(newname, nuri, newlen + 1);
									newname[newlen] = 0;
									al->namelen = newlen;
									if(al->name)
									{
										FreeVecPooled(data->apool, al->name);
									}
									al->name = newname;

									if(al->obj)
									{
										SetAttrs(al->obj, MA_Icon_Path, naf->newname, MA_Icon_MimeType, NULL, TAG_DONE);
										listview_deleteicon(data->list, al->obj, TRUE);
									}
								}
								break;
							}
						}
					}
					break;

			case NOTIFYTAG_Monitor_Device_Mount:
				{
					//struct notifyact_device_mount *naf = (struct notifyact_device_mount *)na;

					/* simple refresh for now */
					data->showfiles   = FALSE;
					data->showdevices = TRUE;
					listview_abort_threads(obj, data);
				}
				break;

			case NOTIFYTAG_Monitor_Device_UnMount:
				{
					//struct notifyact_device_unmount *naf = (struct notifyact_device_unmount *)na;

					/* simple refresh for now */
					data->showfiles   = FALSE;
					data->showdevices = TRUE;
					listview_abort_threads(obj, data);
				}
				break;

			case NOTIFYTAG_Monitor_File_Name:
				{
					STRPTR p;
					struct aline *al;
					struct notifyact_file_name *naf = (struct notifyact_file_name *)na;

					p = FilePart(naf->uri);

					data->notify_filerename = TRUE;

					ITERATELIST(al, &data->alist)
					{
						if(!stricmp(al->name, p))
						{
							STRPTR newname;
							ULONG  newlen;
							p = FilePart(naf->newname);

							newlen  = strlen(p);
							newname = AllocVecPooled(data->apool, newlen + 1);

							if (newname)
							{
								stccpy(newname, p, newlen + 1);
								al->namelen = newlen;
								if(al->name)
								{
									FreeVecPooled(data->apool, al->name);  /* XXX : a semaphore could be needed here */
								}
								al->name = newname;

								if(al->obj)
								{
									SetAttrs(al->obj, MA_Icon_Path, naf->newname, MA_Icon_MimeType, NULL, TAG_DONE);
									listview_deleteicon(data->list, al->obj, TRUE);
								}
							}
							break;
						}
					}
				}
				break;

			case NOTIFYTAG_Monitor_File_Comment:
				{
					STRPTR p;
					struct aline *al;
					struct notifyact_file_comment *naf = (struct notifyact_file_comment *)na;

					p = FilePart(naf->uri);

					ITERATELIST(al, &data->alist)
					{
						if(!stricmp(al->name, p))
						{
							STRPTR newcomment;
							ULONG  len = strlen(naf->comment);

							newcomment = AllocVecPooled(data->apool, len + 1);
							restart = FALSE;

							if (newcomment)
							{
								stccpy(newcomment, naf->comment, len + 1);
								if(al->un.file.comment)
								{
									FreeVecPooled(data->apool, al->un.file.comment);
								}
								al->un.file.comment = newcomment;
							}
							break;
						}
					}
				}
				break;

			case NOTIFYTAG_Monitor_File_Flags:
				{
					STRPTR p;
					struct aline *al;
					struct notifyact_file_flags *naf = (struct notifyact_file_flags *)na;
					//restart = FALSE;

					p = FilePart(naf->uri);

					ITERATELIST(al, &data->alist)
					{
						if(!stricmp(al->name, p))
						{
							ULONG old = al->un.file.protection & (FIBF_SCRIPT | FIBF_READ | FIBF_EXECUTE);
							ULONG new = naf->flags             & (FIBF_SCRIPT | FIBF_READ | FIBF_EXECUTE);

							al->un.file.protection = naf->flags;

							if (new != old)
							{
								al->invalidate_mimetype = 1;

								if (al->obj != NULL)
								{
									SetAttrs(al->obj, MA_Icon_MimeType, NULL, TAG_DONE);
									listview_deleteicon(data->list, al->obj, TRUE);
								}
							}
							break;
						}
					}
				}
				break;

			case NOTIFYTAG_Monitor_File_Date:
				{
					struct aline *al;
					struct notifyact_file_date *naf = (struct notifyact_file_date *)na;
					STRPTR p = FilePart(naf->uri);
					restart = FALSE;

					ITERATELIST(al, &data->alist)
					{
						if(!stricmp(al->name, p))
						{
							al->datestamp = naf->datestamp;
							SetDateText(data, al);
							break;
						}
					}
				}
				break;

			case NOTIFYTAG_Monitor_File_Delete:
				{
					struct notifyact_file_delete *naf = (struct notifyact_file_delete *)na;
					data->notify_filedelete = TRUE;
					DoMethod(obj, MM_Listview_RemoveByName, FilePart(naf->uri), FALSE);
				}
				break;

			case NOTIFYTAG_Monitor_File_Create:
				{
					struct notifyact_file_create *naf = (struct notifyact_file_create *)na;
					struct fileinfo64 fi;

					/* Don't bother with new .info files in icon mode */
					if (data->icondisplay && /*getv(obj, MA_View_HandleIcons) &&*/ name_isinfo(naf->uri))
					{
						STRPTR filepart = FilePart(naf->uri);

						if (stricmp(filepart, "disk.info"))
						{
							STRPTR s = name_build_noinfo(filepart);

							if (s)
							{
								struct aline *al;
								int max_count = 2;

								ITERATELIST(al, &data->alist)
								{
									if (!stricmp(al->name, filepart) || !stricmp(al->name, s))
									{
										listview_deleteicon(obj, al->obj, TRUE);
										al->invalidate_cache = 1;

										if (--max_count == 0)
											break;
									}
								}

								if (max_count < 2)
								{
								}

								name_delete(s);
							}
						}
					}

					data->notify_filecreate = TRUE;

					if (examine64(naf->uri, &fi))
					{
						STRPTR target = NULL;
						struct ExAllData ead;

						ead.ed_Name     = fi.fi_FileName; /* FilePart(naf->uri); */
						ead.ed_Type     = fi.fi_Type;
						ead.ed_Size     = (fi.fi_Size < 0x100000000ULL) ? (ULONG) fi.fi_Size : 0;
						ead.ed_Prot     = fi.fi_Protection;
						ead.ed_Days     = fi.fi_Date.ds_Days;
						ead.ed_Mins     = fi.fi_Date.ds_Minute;
						ead.ed_Ticks    = fi.fi_Date.ds_Tick;
						ead.ed_Comment  = fi.fi_Comment[0] ? fi.fi_Comment : NULL;
						ead.ed_OwnerUID = fi.fi_OwnerUID;
						ead.ed_OwnerGID = fi.fi_OwnerGID;
						#if !USE_LEGACY
						ead.ed_Size64   = fi.fi_Size;
						#endif

						if ( ead.ed_Type == ST_SOFTLINK || ead.ed_Type == ST_LINKFILE || ead.ed_Type == ST_LINKDIR )
						{
							TEXT linktarget[PATH_SIZE];
							BPTR l;

							if( (l = Lock(naf->uri, ACCESS_READ)) )
							{
								if(NameFromLock(l, linktarget, sizeof(linktarget)))
								{
									target = linktarget;
								}
								UnLock(l);
							}
						}

						DoMethod(obj, MM_Listview_AddFile, &ead, TRUE, MV_Listview_AddFile_New, target);
					}
				}
				break;
		}
	}

	if(data->update )
	{
		if(data->notify_filecreate)
		{
			DoMethod(obj, MM_Listview_AddFiles, MV_Listview_AddFiles_New);
			DoMethod(obj, MM_Listview_UpdateView, MV_Listview_UpdateView_New);

			data->notify_filecreate = FALSE;
		}

		if(data->notify_filedelete)
		{
			/* XXX: what if a thread references the disposed listviewentryclass objects ?
			 * should object disposal be deferred until all threads are finished ? i guess it's
			 * a solution...
			 */
			DoMethod(obj, MM_Listview_RemoveFiles, 0);

			DoMethod(obj, MM_Listview_UpdateEntriesPositions);

			//set(data->list, MUIA_List_Active, MUIV_List_Active_Off);

			data->notify_filedelete = FALSE;
		}

		if(data->notify_filerename)
		{
			// needs to be a bit smarter. disabled for now
			//DoMethod(data->list, MUIM_List_Sort);
		}

		if (restart)
		{
			DB(("Restart threads...\n"));

			listview_update_statusbar_info(obj, data);
			listview_abort_thread(TA_Listview_CreateIcons, TRUE, obj, data);

			if(data->type == FLT_FILES)
			{
				listview_abort_thread(TA_MimeType_Scan,        TRUE, obj, data);
				listview_abort_thread(TA_Version_Find,         TRUE, obj, data);
				listview_abort_thread(TA_File_MD5sum,          TRUE, obj, data);
				listview_abort_thread(TA_File_GetSizes,        TRUE, obj, data);
			}
		}
	}

	if(data->update)
	{
		set(data->list, MUIA_List_Quiet, FALSE);
	}

	data->in_notify = FALSE;

	return (0);
}

DEFMMETHOD(ContextMenuBuild)
{
	GETDATA;
	ULONG flags = AS_LISTVIEW;
	LONG pos;
	//ULONG inselection = FALSE;
	struct aline *entry;

	struct MUI_List_TestPos_Result res;

	DoMethod(data->list, MUIM_List_TestPos, msg->mx, msg->my, &res);

	data->viewcm = FALSE;

	if (data->cmenu)
	{
		MUI_DisposeObject(data->cmenu);
		data->cmenu = NULL;
	}

	/* if mouse is over an empty area or not over name column, display view menu */
	if(res.entry == -1 || res.column != DoMethod(obj, MM_Listview_GetColumnOrder, (data->type == FLT_FILES) ? LISTVIEW_FILE_COL_NAME : LISTVIEW_DEVICE_COL_VOLUME))
	{
		data->viewcm    = TRUE;
		data->cmgrouped = FALSE;

		return (DOSUPER);
	}

	/* check if mouse is inside selected area or not */

	/* not sure it's really a good idea to enable this */
	/*
	DoMethod(data->list, MUIM_List_GetEntry, res.entry, (ULONG *)&entry);

	pos = MUIV_List_NextSelected_Start;
	DoMethod(data->list, MUIM_List_NextSelected, &pos);
	while (pos != MUIV_List_NextSelected_End)
	{
		struct aline *selentry;

		DoMethod(data->list, MUIM_List_GetEntry, pos, (ULONG *)&selentry);

		if (selentry == entry)
		{
			inselection = TRUE;
			break;
		}
		DoMethod(data->list, MUIM_List_NextSelected, &pos);
	}
	*/

	if (getv(obj, MA_View_NumSelected) > 1/* && inselection*/)
	{
		/*
		 * Grouped mode.
		 */

		set(data->list, MUIA_List_Active, res.entry);

		for(pos=0;;pos++)
		{
			DoMethod(data->list, MUIM_List_GetEntry, pos, (ULONG *)&entry);

			if(entry && entry->active)
			{
				if(data->type == FLT_DEVICES)
				{
					switch (entry->type)
					{
						case DLT_VOLUME:
						{
							struct dlcnode *dlc;    /* depending on the device we hide "Eject" and "Format" */
							flags |= AS_DEVICE;
							doslistcache_lock();
							if( (dlc = doslistcache_find_dlcdevice_by_volumename( (STRPTR) getv(entry->obj, MA_Icon_Path) ) ) )
							{
								flags |= AS_FORMATABLE;
								if( (dlc->flags & DLF_REMOVABLE) )
								{
									flags |= AS_REMOVABLE;
								}
								else if( (dlc->flags & DLF_UNMOUNTABLE) )
								{
									flags |= AS_UNMOUNTABLE;
								}
							}
							doslistcache_unlock();
							if (is_trashcan((STRPTR) getv(entry->obj, MA_Icon_Path)))
								flags |= AS_ISTRASHCAN;
						}
						break;
						case DLT_DIRECTORY:
						{
							flags |= AS_ASSIGN;
							break;
						}
					}
				}

				if(data->type == FLT_FILES)
				{
					switch(entry->type)
					{
						case ST_USERDIR:
							flags |= AS_DRAWER;
							flags |= icon_flags_for_path((STRPTR) getv(entry->obj, MA_Icon_Path), entry->type);
							break;

						case ST_FILE:
							flags |= AS_TOOL;
							flags |= AS_PROJECT;
							flags |= icon_flags_for_path((STRPTR) getv(entry->obj, MA_Icon_Path), entry->type);
							break;

						default:
							PDB(("huh.. no type..\n"));
							flags |= AS_TOOL; /* XXX: workaround.. */
							break;
					}
				}
			}
			else if(!entry)
			{
				break;
			}
		}

		if ((data->cmenu = contextmenu_build(CM_LISTGROUP, flags)))
		{
			struct MinList *l;
			struct typescannernode *tn;
			LONG cnt = 0;
			ULONG globaltype;

			data->viewcm = FALSE;
			data->cmgrouped = TRUE;

			l = malloc( sizeof( struct MinList ) );

			if(l)
			{
				if(1) //????  (tn = malloc(sizeof(*tn) + strlen(data->path) + 1)) )
				{
					Object *mimeTypeObject = (Object*) DoMethod(NewObject(getmimetypeclass(), NULL, TAG_DONE), OM_RETAIN);
					NEWLIST(l);

					for(pos=0;;pos++)
					{
						DoMethod(data->list, MUIM_List_GetEntry, pos, (ULONG *)&entry);

						if(entry && entry->obj && entry->active)
						{
							STRPTR p = (STRPTR) getv(entry->obj, MA_Icon_Path);

							if( (tn = malloc(sizeof(*tn) + strlen(p) + 1)) )
							{
								tn->obj = entry->obj;
								strcpy(tn->path, p);
								ADDTAIL(l, tn);
							}
						}
						else if(!entry)
						{
							break;
						}
					}

					if (do_action(obj, TA_MimeType_Scan,
									TT_MimeType_Scan_List, l,
									TT_MimeType_Scan_MatchFirst, TRUE,
									TT_MimeType_Scan_MimetypeObject, mimeTypeObject,
									TAG_DONE))
					{
						l = NULL; /* thread will free nodes and the list */

						/*
						 * We give it max 1s to check the conditions.
						 */

						while( cnt < 25 && !xget(mimeTypeObject, MA_Mimetype_TypeResolved) )
						{
							Delay( 2 );
							cnt++;
							methodstack_check( FALSE );
						}
					}
					else
					{
						struct typescannernode *nexttn;

						ITERATELISTSAFE(tn, nexttn, l)
						{
							free(tn);
						}
					}

					/* */

					/* Add global menu */
					if((flags & AS_DRAWER) && ((flags & (AS_TOOL | AS_PROJECT)) == 0))
					{
						globaltype = MENU_GLOBALACTION_DIRECTORY;
					}
					else
					{
						globaltype = MENU_GLOBALACTION_FILE;
					}

					contextmenu_add_global( data->cmenu, NULL, globaltype );

					/* Add mimetype menu */
					if ( xget(mimeTypeObject, MA_Mimetype_Type) != 0 )
					{
						struct internal_mimetype_node *imn = (APTR) xget(mimeTypeObject, MA_Mimetype_Type);
						D(MIMETYPE,bug("Common mimetype:%s\n", imn->mimetype ));

						contextmenu_add_mime( data->cmenu, NULL, imn );
					}
					
					DoMethod(mimeTypeObject, OM_RELEASE);
				}
				free(l);
			}

			return ((ULONG)data->cmenu);
		}
	}
	else
	{
		DoMethod(data->list, MUIM_List_GetEntry, res.entry, (ULONG *)&entry);

		set(data->list, MUIA_List_Active, res.entry);

		if(entry && entry->obj)
		{
			if(data->type == FLT_DEVICES)
			{
				switch (entry->type)
				{
					case DLT_VOLUME:
					{
						struct dlcnode *dlc;    /* depending on the device we hide "Eject" and "Format" */
						flags |= AS_DEVICE;
						doslistcache_lock();
						if( (dlc = doslistcache_find_dlcdevice_by_volumename( (STRPTR) getv(entry->obj, MA_Icon_Path) ) ) )
						{
							flags |= AS_FORMATABLE;
							if( (dlc->flags & DLF_REMOVABLE) )
							{
								flags |= AS_REMOVABLE;
							}
							else if( (dlc->flags & DLF_UNMOUNTABLE) )
							{
								flags |= AS_UNMOUNTABLE;
							}
						}
						doslistcache_unlock();
						if (is_trashcan((STRPTR) getv(entry->obj, MA_Icon_Path)))
							flags |= AS_ISTRASHCAN;
					}
					break;
					case DLT_DIRECTORY:
					{
						flags |= AS_ASSIGN;
						break;
					}
				}
			}

			if(data->type == FLT_FILES)
			{
				switch(entry->type)
				{
					case ST_USERDIR:
						flags |= AS_DRAWER;
						flags |= icon_flags_for_path((STRPTR) getv(entry->obj, MA_Icon_Path), entry->type);
						break;

					case ST_FILE:
						flags |= AS_TOOL;
						flags |= AS_PROJECT;
						flags |= icon_flags_for_path((STRPTR) getv(entry->obj, MA_Icon_Path), entry->type);
						break;

					default:
						PDB(("huh.. no type..\n"));
						flags |= AS_TOOL; /* XXX: workaround.. */
						break;
				}
			}

			if ((data->cmenu = contextmenu_build(CM_LIST, flags)))
			{
				//TEXT path[PATH_SIZE];
				STRPTR path;
				APTR mimetype = (APTR) getv(entry->obj, MA_Icon_MimeType);
				//char * p = (STRPTR) getv( data->list, MA_Listviewlist_Path );

				/*
				 * Add type dependant actions to context menu.
				 */

				path = (STRPTR) getv(entry->obj, MA_Icon_Path);

				if(path)
				{
					if ( !mimetype )
					{
						/*
						 * Needs to be found. This is a bit problematic as it should be done on
						 * a thread but for now we do this workaround.
						 */

						LONG cnt = 0;
						Object *mimeTypeObject = (Object*) DoMethod(NewObject(getmimetypeclass(), NULL, TAG_DONE), OM_RETAIN);

						if (do_action(data->list, TA_MimeType_Scan,
						          TT_MimeType_Scan_Path, path,
						          TT_MimeType_Scan_MimetypeObject, mimeTypeObject,
						          TAG_DONE))
						{
							/*
							 * We give it max 1s to find the type.
							 */

							while( cnt < 25 && !xget(mimeTypeObject, MA_Mimetype_TypeResolved) )
							{
								Delay( 2 );
								cnt++;
								methodstack_check( FALSE );
							}

						}

						mimetype = (Object *)xget(mimeTypeObject, MA_Mimetype_Type);
						set(entry->obj, MA_Icon_MimeType, (IPTR)mimetype);
						DoMethod(mimeTypeObject, OM_RELEASE);
					}

					if ( mimetype )
					{
						TEXT buf[ 1024 ];
						CONST_STRPTR mode = NULL; /* default if no 'vn' */
						CONST_STRPTR view = "List";
						ULONG globaltype;

						/* we inherit list view and its current submode */
						struct viewnode * vn = viewapi_findbyname(view);

						if(vn && data->type == FLT_FILES)
						{
							mode = viewapi_getmodename(vn, data->lv_format.view_mode);
						}

						{
							UBYTE encpath[mimeuri_encodepath(NULL, 0, path)];

							mimeuri_encodepath(encpath, sizeof(encpath), path);

							/* XXX: if it's a directory, read icon window coords and pass them, or use some listview preferences coords ? */
							snprintf(buf, sizeof(buf), "file:///%s?view=%s",
								encpath,
								view );
							if(mode)
							{
								strncat(buf, "&mode=", sizeof(buf));
								strncat(buf, mode, sizeof(buf));
							}
						}

						if(flags & AS_DRAWER)
						{
							globaltype = MENU_GLOBALACTION_DIRECTORY;
						}
						else if(flags & AS_DEVICE)
						{
							globaltype = MENU_GLOBALACTION_DEVICE;
						}
						else
						{
							globaltype = MENU_GLOBALACTION_FILE;
						}

						contextmenu_add_global( data->cmenu, buf, globaltype );

						contextmenu_add_mime( data->cmenu, buf, mimetype );
					}
				}

				data->viewcm    = FALSE;
				data->cmgrouped = FALSE;
				return ((ULONG)data->cmenu);
			}
		}
	}

	return (0);
}

DEFTMETHOD(View_PickSelected)
{
	/* Overload MM_View_PickSelected method from viewclass.c */
	GETDATA;

	LONG pos = 0;
	ULONG objcount;
	APTR array, list;

	list = data->list;
	objcount = 0;
	array = NULL;
	objcount = getv(obj, MA_View_NumSelected);

	if(objcount)
	{
		array = AllocVecTaskPooled((objcount+1) * sizeof(APTR));

		if (array)
		{
			ULONG *ptr = array;

			for(pos=0;;pos++)
			{
				struct aline *entry;

				DoMethod(list, MUIM_List_GetEntry, pos, (ULONG *)&entry);

				if (entry && entry->obj && entry->active)
				{
					*ptr++ = (ULONG) entry->obj;
				}
				else if(!entry)
				{
					break;
				}
			}

			*ptr = 0;
		}
	}

	return ((ULONG)array);
}

static ULONG listview_contextmenu_callback(APTR obj, int entry, struct menuitem_state * state)
{
	switch(entry)
	{
		case 0:
			state->enabled = TRUE;
			state->checkit = TRUE;
			state->checked = getv(obj, MA_Listview_GetSizes);
			return (TRUE);
		case 1:
			state->enabled = TRUE;
			state->checkit = FALSE;
			state->checked = FALSE;
			return (TRUE);
		default:
			return (FALSE);
	}
}

DEFMMETHOD(ContextMenuAdd)
{
	contextmenu_add_listview_options(obj, msg->menustrip, listview_contextmenu_callback);
	return (0);
}

DEFMMETHOD(ContextMenuChoice)
{
	GETDATA;

	if (data->viewcm)
	{
		return (DOSUPER);
	}
	else
	{
		struct command_menu *cm;

		if ((cm = (struct command_menu *)getv(msg->item, MA_Menuitem_Command)))
		{
			#if 1
			if (cm->args)
			{
				/* XXX: will be moved as true commands in appclass */
				if(!stricmp(cm->args, "Listview GetSizes"))
				{
					data->sizesdisplay = data->sizesdisplay ? FALSE : TRUE;
					listview_abort_thread(TA_File_GetSizes, TRUE, obj, data);

					return (0);
				}

				if(!stricmp(cm->args, "Listview DefaultFormat"))
				{
					listview_reset_to_default(obj, data);

					return (0);
				}
				/* Ugly Hack: Abort the preview to unlock the file. This makes it possible
				   to actually delete it. This doesn't solve the the issue that the selected
				   file is still locked from deletion, for example from shell.
				   See MT#2841 for discussion. */
				if(!stricmp(cm->args, "Delete"))
				{
					listview_abort_thread(TA_Listview_ShowPreview, FALSE, obj, data);
				}
			}
			#endif

			contextmenu_execute(data->list, obj, msg->item, data->cmgrouped);
		}
	}

	return (0);
}

DEFSMETHOD(Rexx_Snapshot)
{
	if (msg->window)
	{
		CONST_STRPTR p;
		GETDATA;

		if (data->type == FLT_DEVICES)
		{
			p = "devices://";
		}
		else
		{
			p = (CONST_STRPTR)getv(obj, MA_View_Path);
		}
		listview_store_format(p, &data->lv_format, FALSE);
	}

	return (DOSUPER);
}


DEFSMETHOD(Rexx_Unsnapshot)
{
	GETDATA;
	CONST_STRPTR path;

	if(data->type == FLT_DEVICES)
	{
		path = "devices://";
	}
	else
	{
		path = (CONST_STRPTR)getv(obj, MA_View_Path);
	}

	listview_store_format(path, &data->lv_format, TRUE);

	return (DOSUPER);
}

DEFTMETHOD(Listview_UpdateIconsSize)
{
	GETDATA;
	data->icondisplay = TRUE;
	set(obj, MA_Listview_IconSize, 0); /* cycles thru icon sizes */

	listview_abort_thread(TA_Listview_CreateIcons, TRUE, obj, data);
	return (0);
}

DEFSMETHOD(Listview_RunIconThread)
{
	GETDATA;
	ULONG rc = FALSE;

	if(data->iconsize == 0)
	{
		msg->enable = FALSE;
	}

	/* Don't mess with icons while preview is running.
	 * It sucks but it's easier
	 */
	if(data->thread_preview_busy)
	{
		DoMethod(obj, MM_Listview_ShowPreview, FALSE);
		return (0);
	}

	data->thread_createicons_busy = TRUE;

	if(msg->enable)
	{
		ULONG sizechanged = FALSE;
		if(data->lv_format.icon_size != data->iconsizeindex)
		{
			sizechanged = TRUE;
			data->iconsizeindex = data->lv_format.icon_size%(sizeof(iconsizes)/sizeof(ULONG));
			data->iconsize = iconsizes[data->iconsizeindex];
		}

		if(sizechanged)
		{
			if(data->icons_setup_done)
			{
				DoMethod(obj, MM_Listview_DeleteDefaultImages);
				DoMethod(obj, MM_Listview_CleanupIcons);
			}
		}

		data->icondisplay = TRUE;

		if(!data->icons_setup_done)
		{
			DoMethod(obj, MM_Listview_SetupIcons);
			DoMethod(obj, MM_Listview_CreateDefaultImages);
		}
	}
	else
	{
		if(data->icons_setup_done)
		{
			DoMethod(obj, MM_Listview_DeleteDefaultImages);
			DoMethod(obj, MM_Listview_CleanupIcons);
		}

		data->icondisplay = FALSE;
	}

	data->dosetup = FALSE;
	/* Warning:
	 * OM_REMMEMBER causes inline edit gadget to lose focus.
	 *
	 * Should we call InitChange/ExitChange? It seems to work
	 * without.
	 */
	//DoMethod(obj, MUIM_Group_InitChange);
	//DoMethod(obj, OM_REMMEMBER, data->list);

	SetAttrs(data->list,
		MA_Listviewlist_IconDisplay, data->icondisplay,
		MUIA_List_MinLineHeight, data->icondisplay ? data->iconsize : 0,
		TAG_DONE
	);

	//DoMethod(obj, OM_ADDMEMBER, data->list);
	//DoMethod(obj, MUIM_Group_ExitChange);
	data->dosetup = TRUE;

	if(msg->enable && msg->immediate) /* let's run the thread */
	{
		struct MinList *ml;

		if ((ml = AllocVecPooled(data->npool, sizeof(*ml))))
		{
			int i;
			struct aline *al;
			struct typescannernode *tn, *nexttn;

			NEWLIST(ml);

			for(i=0;;i++)
			{
				DoMethod(data->list, MUIM_List_GetEntry, i, &al);

				if(al)
				{
					if ((tn = AllocVecPooled(data->npool, sizeof(*tn))))
					{
						tn->obj = al; //al->obj;
						ADDTAIL(ml, tn);
					}
				}
				else
				{
					break;
				}
			}

			if(do_action(obj, TA_Listview_CreateIcons,
					TT_Listview_Iconlist, (APTR) ml,
					TT_Listview_Data, (APTR) data,
					TAG_DONE))
			{
				rc = TRUE;
			}
			else
			{
				ITERATELISTSAFE(tn, nexttn, ml)
				{
					FreeVecPooled(data->npool, tn);
				}
				FreeVecPooled(data->npool, ml);
			}
		}
	}

	if(rc == FALSE)
	{
		data->thread_createicons_busy = FALSE;
	}

	return (0);
}

DEFSMETHOD(View_Refresh)
{
	GETDATA;

	if (msg->flags & (MF_View_Refresh_Background | MF_View_Refresh_Fonts))
	{
		if (msg->flags & MF_View_Refresh_Fonts)
		{
			STRPTR fontname = getprefsstr(DSI_FASTLIST_FONT);
			struct listview_format lv_format;
			memset(&lv_format, 0, sizeof(lv_format));

			/* set entries colors */
			DoMethod(data->list, MM_Listview_AllocPens);

			/* set font */
			if(fontname && *fontname)
			{
				DoMethod(obj, MUIM_Group_InitChange);
				DoMethod(obj, OM_REMMEMBER, data->list);
				set(data->list, MUIA_Font, _conf(fl_lister_font)->tf);
				DoMethod(obj, OM_ADDMEMBER, data->list);
				DoMethod(obj, MUIM_Group_ExitChange);
			}

			DoMethod(data->list, MM_View_Refresh, MF_View_Refresh_Fonts, NULL);

			if(!listview_retrieve_format((STRPTR) getv(obj, MA_View_Path), &lv_format, data->type))
			{
				listview_reset_to_default(obj, data);
			}
		}

		if (msg->flags & MF_View_Refresh_Background)
		{
			static TEXT background[PATH_SIZE];
			ULONG bgmode = getprefslong(DSI_BACKGROUND_WINDOW_BGRENDER);

			if(bgmode)
			{
				snprintf(background, sizeof(background)-1, "5:%s", getprefsstr(DSI_BACKGROUND_WINDOW));
			}

			if(getprefslong(DSI_FASTLIST_WINDOW_BG))
			{
				set(data->list, MUIA_Background, bgmode?background:getprefs(DSI_BACKGROUND_WINDOW_BGCOLOR));
			}
			else
			{
				set(data->list, MUIA_Background, MUII_ListBack);
			}
		}
	}
	return (0);
}

/* helpers to retrieve deficons paths, without using any dynamic memory */

static ULONG name_build_info_static(STRPTR name, ULONG type, STRPTR info, ULONG *isdefault); // bitRocky: added "isdefault" parameter
static void deficonpool_get_icon_name_static(ULONG viewid, STRPTR path, ULONG type, APTR mimetype, STRPTR info, ULONG * isdefault);

static ULONG name_build_info_static(STRPTR name, ULONG type, STRPTR info, ULONG *isdefault) // bitRocky: added "isdefault" parameter
{
	ULONG len;
	ULONG found = FALSE;

	ASSERT(name);

	len = strlen(name);

	D(CREATEICON, bug("name = '%s'\n", name)); // bitRocky
	/* volume/assign name?... */
	if (len && name[len - 1] == ':')
	{
		if(type == MV_Icon_FileType_Device)
		{
			stccpy(info, name, PATH_SIZE);
			strncat(info, "disk.info", PATH_SIZE);

			/* No disk.info, let's check if a def_thing exists */
			if (!exists(info))
			{
				STRPTR filename = deficon_build_devicename(name);
				D(CREATEICON, bug("No disk.info, let's check if a def_thing exists\n")); // bitRocky
				if(filename)
				{
					D(CREATEICON, bug("yes, filename = '%s'\n", filename)); // bitRocky
					found = TRUE;
					*isdefault = TRUE; // bitRocky
					stccpy(info, filename, PATH_SIZE);
					name_delete(filename);
				}
			}
			else
			{
				found = TRUE;
			}
		}
		else
		{
			/* It's an assign, resolve and get icon name */
			TEXT expandedname[PATH_SIZE];

			if(path_expand(name, expandedname, PATH_SIZE))
			{
				stccpy(info, expandedname, PATH_SIZE);

				strncat(info, ".info", PATH_SIZE);

				if(exists(info))
				{
					found = TRUE;
				}
			}
		}
	}
	else /* ... or normal path? */
	{
		stccpy(info, name, PATH_SIZE);

		if (!(len > 5
			&& name[len - 5] == '.'
			&& (name[len - 4] == 'i' || name[len - 4] == 'I')
			&& (name[len - 3] == 'n' || name[len - 3] == 'N')
			&& (name[len - 2] == 'f' || name[len - 2] == 'F')
			&& (name[len - 1] == 'o' || name[len - 1] == 'O')
		))
		{
			strncat(info, ".info", PATH_SIZE);

			if(exists(info))
			{
				found = TRUE;
			}
		}
		else
		{
			found = TRUE;
		}
	}

	D(CREATEICON, bug("found = %ld\n", found)); // bitRocky
	return(found);
}

static void deficonpool_get_icon_name_static(ULONG viewid, STRPTR path, ULONG type, APTR mimetype, STRPTR info, ULONG * isdefault)
{
	ULONG refine;

	D(CREATEICON, bug("before name_build_info_static(): isdefault = %ld\n", *isdefault));
	/* does icon exist ? */
	/* XXX: handle default disk icon flag */
	if(name_build_info_static(path, type, info, isdefault))
	{
		return;
	}

	*isdefault = TRUE;
	D(CREATEICON, bug("after name_build_info_static(): isdefault = %ld\n", *isdefault));
	
	/* is it a directory ? */
	if(isdir(path))
	{
		/* XXX: should check for overflow */
		deficon_getpath(info, PATH_SIZE, "def_drawer.info");
		return;
	}

	/* use iconmime hack to get icon name */
	if(new_deficonpool_build_name(viewid, path, mimetype, &refine, info, PATH_SIZE))
	{
		return;
	}

	/* fallback to def_tool.info then */
	/* XXX: should check for overflow */
	deficon_getpath(info, PATH_SIZE, "default.info");
}

/*****************************************************************/


static APTR listview_finddeficon(CONST_STRPTR name, APTR d)
{
	struct listview_deficon_node * n;
	struct Data * data = (struct Data *) d;

	//ObtainSemaphore(&(data->deficonlistsem));

	ITERATELIST(n, &(data->deficonlist))
	{
		if(!stricmp(name, n->iconpath))
		{
			return n;
		}
	}

	//ReleaseSemaphore(&(data->deficonlistsem));

	return NULL;
}

static APTR get_default_image(ULONG type, APTR d)
{
	struct Data * data = (struct Data *) d;

	if(data->type == FLT_DEVICES)
	{
		if(type == DLT_VOLUME)
		{
			return data->defaultdisk_image;
		}
		else
		{
			return data->defaultdir_image;
		}
	}
	else
	{
		if(type == ST_USERDIR)
		{
			return data->defaultdir_image;
		}
		else
		{
			return data->defaultfile_image;
		}
	}
}

static APTR listview_findicon(APTR entry, ULONG type, APTR d, ULONG defaultimage)
{
	APTR  image = NULL;

	image = (APTR) getv(entry, MA_Icon_Image);

	if (!image)
	{
		if(defaultimage)
		{
			image = get_default_image(type, d);
		}
	}

	return image;
}

ULONG tr_listview_createicons(APTR obj, APTR iconlist, APTR d)
{
	ULONG rc = TRUE;
	struct typescannernode *tn;
	ULONG abort = FALSE;
	struct MinList * l = (struct MinList *) iconlist;
	struct Data * data = (struct Data *) d;
	ULONG toppos = 0xffffff;	/* NAN */
	APTR  ctx = NULL;

	THREAD;
	CHECKOBJECT(obj);
	ASSERT(l);

	if(!data->defaultfile_image || !data->defaultdir_image || !data->defaultdisk_image)
	{
		methodstack_push_sync(obj, 1, MM_Listview_CreateDefaultImages);
	}

	tn = FIRSTNODE( l );

	while ( !ISLISTEMPTY( l ) )
	{
		if (threads_check_abort())
		{
			abort = TRUE;
			rc = ABORTED;
		}

		if (!abort)
		{
			ULONG new_toppos;
			struct createicon_args args;

			/*
			 * Check if view scrolled. If it did we will recalculate
			 * first node to be scanned. Some better strategy?
			 */

			methodstack_push_sync( obj, 3, OM_GET, MUIA_List_TopPixel, &new_toppos );

			if ( new_toppos != toppos )
			{
				struct typescannernode *ttn;

				toppos = new_toppos;
				tn = NULL;

				ITERATELIST( ttn, l )
				{
					ULONG isvisible;
					struct aline *al = ttn->obj;

					methodstack_push_sync( al->obj, 3, OM_GET, MA_ListviewEntry_IsVisible, &isvisible );

					if ( tn == NULL && isvisible )
					{
						tn = ttn;
						break;
					}
				}

				/*
				 * There is a case when all visible objects are already processed.
				 */

				if ( !tn )
				{
					tn = FIRSTNODE( l );
				}
			}

			args.obj    = data->list;
			args.d      = d;
			args.entry  = tn->obj;
			args.bm     = NULL;
			args.cache  = TRUE;
			args.av_ctx = ctx;

			listview_createicon(obj, &args);
			methodstack_push_sync(((struct aline *)tn->obj)->obj, 1, MM_ListviewEntry_Redraw);
		}

		/*
		 * If we reached end of list we need to check if we skipped some nodes.
		 */

		{
			struct typescannernode *ttn;

			if ( tn != LASTNODE( l ) )
			{
				ttn = NEXTNODE( tn );
			}
			else
			{
				ttn = FIRSTNODE( l );
			}

			REMOVE(tn);
			FreeVecPooled(data->npool, tn);
			tn = ttn;
		}
	}
	FreeVecPooled(data->npool, l);

	return (rc);
}

static APTR get_cached_thumbnail( struct Data *data UNUSED, ULONG hash )
{
	APTR cached_image;
	ULONG *header;
	APTR bm = NULL;

	if ( !cache_get( hash, CACHETAG_THUMBNAIL, &cached_image ) )
		return NULL;

	header = cached_image;

	/*
	 * Since we don't do anything with this bitmap, we can optimize the format.
	 */

	bm = gfx_bitmap_create( header[ 0 ], header[ 1 ], header[ 2 ] == 4 ? 32 : 32,
		BITMAPTAG_Format, header[ 2 ] == 4 ? BITMAPVAL_Format_ARGB32 : BITMAPVAL_Format_ARGB32,
		TAG_DONE );

	if ( bm )
	{
		gfx_blit( (UBYTE*)cached_image + 12, bm,
					BLITTAG_SrcType, BLITVAL_SrcType_Array,
					BLITTAG_DstWidth, header[ 0 ],
					BLITTAG_DstHeight, header[ 1 ],
					BLITTAG_SrcFormat, header[ 2 ] == 4 ? BLITVAL_SrcFormat_ARGB : BLITVAL_SrcFormat_RGB,
					TAG_DONE );
	}

	// we must unlock manually for CACHETAG_THUMBNAIL as soon as we're done - Piru
	cache_unlock();

	return bm;
}

static struct listview_deficon_node * listview_createdeficon(APTR obj, struct Data * data, STRPTR name, APTR bm, ULONG isdefault)
{
	struct listview_deficon_node * deficon = NULL;
	APTR bitmap      = NULL;

	if((deficon = (struct listview_deficon_node *) AllocVecPooled(data->npool, sizeof(*deficon))))
	{
		deficon->iconpath = (STRPTR) AllocVecPooled(data->npool, strlen(name)+1);
		if(deficon->iconpath)
		{
			bitmap = BitmapObject,
						MUIA_Bitmap_Height, gfx_bitmap_height(bm), MUIA_Bitmap_Width, gfx_bitmap_width(bm),
						MUIA_FixHeight, gfx_bitmap_height(bm), MUIA_FixWidth, gfx_bitmap_width(bm),
						MUIA_Bitmap_Bitmap, gfx_bitmap_bm(bm),
						MUIA_Bitmap_Alpha, ( isdefault && _conf(icon_defghosted) ) ? 0xffffffff - 0x88888888 : 0xffffffff,
						End;

			if(bitmap)
			{
				deficon->bitmapobject = bitmap;
				deficon->bitmap       = bm;

				if(muiRenderInfo(obj) && _win(obj))
				{
					deficon->image = (APTR) methodstack_push_sync(obj, 3, MUIM_List_CreateImage, bitmap, 0);
				}
				else
				{
					deficon->image = NULL;
				}

				strcpy(deficon->iconpath, name);

				return deficon;
			}
		}
	}

	if(bitmap)
	{
		MUI_DisposeObject(bitmap);
		bitmap = NULL;
	}

	if(deficon)
	{
		if(deficon->iconpath)
		{
			FreeVecPooled(data->npool, deficon->iconpath);
			deficon->iconpath = NULL;
		}

		FreeVecPooled(data->npool, deficon);
		deficon = NULL;
	}

	return NULL;
}

static APTR listview_rescaleicon(APTR bm, ULONG width, ULONG height)
{
	APTR bms = NULL;
	ULONG xs = width, ys = height;

	gfx_scale_calc_aspect_constraints(gfx_bitmap_width(bm), gfx_bitmap_height(bm), &xs, &ys);

	if ((bms = gfx_bitmap_create(xs, ys, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)))
	{
		if (!gfx_scale(bm, bms, xs, ys, SCALETAG_Bicubic, TRUE, SCALETAG_Average, TRUE, TAG_DONE))
		{
			gfx_bitmap_delete(bms);
			bms = NULL;
		}
	}

	return bms;
}

static APTR listview_createthumbbitmap(STRPTR name)
{
	return tr_thumb_createbitmap(name, NULL, TCF_CREATE);
}

static APTR listview_createiconbitmap(APTR obj, STRPTR name, ULONG width, ULONG height)
{
	APTR bm = NULL;
	APTR ibm = NULL;
	APTR iconobj;
	ULONG viewid = 0;

	if(muiRenderInfo(obj) && _win(obj))
	{
		methodstack_push_sync(_win(obj), 3, OM_GET, MA_Window_ID, &viewid);

		if (viewid && (iconobj = (APTR)methodstack_push_sync(app, 3, MM_Application_CreateIcon, FALSE, viewid)))
		{
			if(icon_read(name, iconobj, ICONTAG_Deficon, TRUE, TAG_DONE))
			{
				methodstack_push_sync(iconobj, 3, OM_GET, MA_Icon_ImageNormal, &ibm);

				if(ibm)
				{
					bm = listview_rescaleicon(ibm, width, height);
				}
			}

			methodstack_push(app, 2, MM_Application_DisposeObject, iconobj);
		}
	}

	return bm;
}

enum { LISTVIEW_ICONTYPE_ICON, LISTVIEW_ICONTYPE_PICTURE, LISTVIEW_ICONTYPE_VIDEO };

static ULONG listview_geticon(struct Data * data, APTR entry, ULONG seconds, STRPTR iconname, ULONG * icontype, ULONG * isdefault, LONG * percent, struct aline *al)
{
	ULONG rc = TRUE;
	ULONG type;
	APTR mimetype = NULL;
	STRPTR name = NULL;
	ULONG viewid = MV_ViewID_Unknown;

	*isdefault = FALSE;
	*percent   = -1;

	methodstack_push_sync(entry, 3, OM_GET, MA_Icon_Path, &name);

	if(name)
	{
		/* if it's not a .info, let's get the target path (deficon, image file, video file...) */
		if(!name_isinfo(name))
		{
			methodstack_push_sync(entry, 3, OM_GET, MA_Icon_FileType, &type);

			/* Get mimetype */
			if(type != ST_USERDIR && type != DLT_VOLUME && type != DLT_DIRECTORY)
			{
				methodstack_push_sync(entry, 3, OM_GET, MA_Icon_MimeType, &mimetype);

				if(!mimetype)
				{
					mimetype = typescanner_get(name, seconds, NULL, al->invalidate_mimetype);

					if(mimetype)
					{
						al->invalidate_mimetype = 0;
						methodstack_push(entry, 3, MUIM_Set, MA_Icon_MimeType, mimetype );
					}
				}
			}

			/* handle assigns, get target icon */
			if(data->type == FLT_DEVICES && type == DLT_DIRECTORY)
			{
				TEXT  target[PATH_SIZE];
				BPTR l;

				*icontype = LISTVIEW_ICONTYPE_ICON;

				if( (l = Lock(name, ACCESS_READ)) )
				{
					if(NameFromLock(l, target, PATH_SIZE))
					{
						DB(("call deficonpool_get_icon_name_static() with target = %s\n", target)); //?bitRocky
						deficonpool_get_icon_name_static(viewid, target, type, NULL, iconname, isdefault);
					}
					else
					{
						DB(("call deficonpool_get_icon_name_static() with name = %s\n", name)); //?bitRocky
						deficonpool_get_icon_name_static(viewid, name, type, NULL, iconname, isdefault);
					}
					UnLock(l);
				}
				else
				{
					DB(("call deficonpool_get_icon_name_static() with target = %s\n", target)); //?bitRocky
					deficonpool_get_icon_name_static(viewid, name, type, mimetype, iconname, isdefault);
				}
				DB(("iconname = %s, *isdefault = %ld\n", iconname ? iconname : (STRPTR)"NIL", *isdefault)); //?bitRocky
			}
			/* thumb ? */
			else if(data->thumbdisplay && picture_validate(name, mimetype))
			{
				*icontype = LISTVIEW_ICONTYPE_PICTURE;
				stccpy(iconname, name, PATH_SIZE);
			}
#if USE_AVCODEC
			/* video ? */
			else if(data->thumbdisplay && video_validate(name, mimetype, percent))
			{
				*icontype = LISTVIEW_ICONTYPE_VIDEO;
				stccpy(iconname, name, PATH_SIZE);
			}
#endif
			/* it's a deficon */
			else
			{
				*icontype = LISTVIEW_ICONTYPE_ICON;
				deficonpool_get_icon_name_static(viewid, name, type, mimetype, iconname, isdefault);
			}
		}
		else
		{
			*icontype = LISTVIEW_ICONTYPE_ICON;
			stccpy(iconname, name, PATH_SIZE);
		}
	}
	else
	{
		rc = FALSE;
	}

	return (rc);
}

DEFSMETHOD(Listview_ShowPreview)
{
#if USE_AVCODEC
	GETDATA;
	if(_aprefs(videopreview) && data->thumbdisplay)
	{
		listview_abort_thread(TA_Listview_ShowPreview, msg->enable, obj, data);
	}
#endif
	return (0);
}

DEFTMETHOD(Listview_RunPreviewThread)
{
	GETDATA;
	struct aline * al;
	ULONG rc = FALSE;

	if(data->type != FLT_FILES  || !data->icondisplay || !data->thumbdisplay)
	{
		return (0);
	}

	data->thread_preview_busy = TRUE;

	DoMethod(data->list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ULONG *)&al);

	if(al && al->obj)
	{
#if USE_AVCODEC
		LONG percent = -1;
		if(video_validate((STRPTR) getv(al->obj, MA_Icon_Path) , (APTR) getv(al->obj, MA_Icon_MimeType), &percent))
		{
      /*
       * Ensure that the iconobj cannot be disposed while
       * the videopreview thread is working on it. - Piru
       */
      //kprintf("%s: OM_RETAIN al->obj %p\n", __func__, al->obj);
      DoMethod(al->obj, OM_RETAIN);

			if(do_action(obj, TA_Listview_ShowPreview,
				TT_Listview_Entry, al->obj,
				TT_Listview_Data, data,
			TAG_DONE))
			{
				rc = TRUE;
			}
      else
      {
        DoMethod(al->obj, OM_RELEASE);
      }
		}
#endif
	}

	if(rc == FALSE)
	{
		data->thread_preview_busy = FALSE;
	}

	return (0);
}

DEFSMETHOD(Listview_InvalidateIcon)
{
	GETDATA;

#if 1
	listview_deletedeficon(data->list, msg->entry, data); 
#else
	struct listview_deficon_node * deficon;
	STRPTR name = (STRPTR) getv(msg->entry, MA_Icon_Path);

	if(name)
	{
		listview_deleteicon(obj, msg->entry, TRUE);

		deficon = listview_finddeficon(name, data);

		if(deficon)
		{
			DoMethod(data->list, MUIM_List_DeleteImage, deficon->image);
			deficon->image = NULL;
		}
	}
#endif

	return (0);
}

#if USE_AVCODEC
static ULONG listview_isinicon(APTR obj, APTR list)
{
	struct MUI_List_TestPos_Result res;
	LONG  active = MUIV_List_Active_Off;

	methodstack_push_sync(list, 3, OM_GET, MUIA_List_Active, &active);

	if(active != MUIV_List_Active_Off)
	{
		methodstack_push_sync(list, 4, MUIM_List_TestPos, _window(list)->MouseX, _window(list)->MouseY, &res);

		if(res.entry == active && res.column == methodstack_push_sync(obj, 2, MM_Listview_GetColumnOrder, LISTVIEW_FILE_COL_ICON))
		{
			return (TRUE);
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
}
#endif

ULONG tr_listview_showpreview(APTR obj, APTR entry, APTR d)
{
	ULONG rc = TRUE;
	ULONG abort = FALSE;
	struct Data * data = (struct Data *) d;
	STRPTR name = NULL;
	APTR bm  = NULL;
	APTR image = NULL;
	APTR oldimage = NULL;
	APTR bitmapobject = NULL;
	struct timerequest * timer;
	struct TimeVal tv;
	struct TimeVal tv_start;
	struct TimeVal tv_end;
	ULONG  elapsed = 0;
	double framerate = 24.0;
	APTR winsave = _win(obj);

	THREAD;
	CHECKOBJECT(obj);

#if USE_AVCODEC

	timer = timer_create(UNIT_MICROHZ, 0);

	if(timer)
	{
		methodstack_push_sync(entry, 3, OM_GET, MA_Icon_Image,  &oldimage);
		methodstack_push_sync(entry, 3, OM_GET, MA_Icon_Path, &name);

		if(name)
		{
			ULONG width = data->iconsize, height = data->iconsize;

			if(ThumbnailsBase &&
			   ((ThumbnailsBase->lib_Version == 50 && ThumbnailsBase->lib_Revision >= 5) || ThumbnailsBase->lib_Version > 50) )
			{
				struct TagItem tags[] = {{THB_Width, width},
										 {THB_Height, height},
										 {THB_Format, THB_FORMAT_RGB888},
										 //{THB_ResizeFilter, THB_FILTER_QUALITY},
										 {THB_Animated, TRUE},
										 {(ULONG)NULL, (ULONG)NULL}}; // bitRocky: to get rid of the warning

				APTR thumb = ThbGenerateThumbnailA(name, tags);

				if(thumb)
				{
					ULONG animated;
					ULONG width, height;
					UBYTE *bdata;
					struct TagItem init_attrs[] = {{THB_Width, (ULONG)&width},
											  {THB_Height, (ULONG)&height},
											  {THB_IsAnimated, (ULONG)&animated},
											  {THB_FramesPerSecond, (ULONG)&framerate},
											  {(ULONG)NULL, (ULONG)NULL}}; // bitRocky: to get rid of the warning

					ThbGetAttrsA(thumb, init_attrs);

					if(animated)
					{
						SDB(("Playing %s, width = %ld height = %ld framerate = %f\n", name, width, height, framerate));

						bm = gfx_bitmap_create(width, height, 32,
							BITMAPTAG_Format, BITMAPVAL_Format_ARGB32,
							TAG_DONE);

						while(!abort)
						{
							if(threads_check_abort())
							{
								abort = TRUE;
								rc = ABORTED;
							}

							if(!abort)
							{
								struct TagItem frame_attrs[] = { {THB_Data, (ULONG)&bdata}, {(ULONG)NULL, (ULONG)NULL} }; // bitRocky: to get rid of the warning
								ULONG win_opened;

								methodstack_push_sync(winsave, 3, OM_GET, MUIA_Window_Open, &win_opened);

								getlocaltime(&tv_start);

								if(win_opened && listview_isinicon(obj, data->list))
								{
									methodstack_push_sync(entry, 3, MUIM_Set, MA_Icon_Image, NULL);

									if(image)
									{
										methodstack_push_sync(data->list, 2, MUIM_List_DeleteImage, image);
										image = NULL;
									}

									if(bitmapobject)
									{
										MUI_DisposeObject(bitmapobject);
										bitmapobject = NULL;
									}

									ThbGetAttrsA(thumb, frame_attrs);

									if(bm && bdata)
									{
										gfx_blit(bdata, bm,
								            BLITTAG_SrcType, BLITVAL_SrcType_Array,
											BLITTAG_SrcFormat, BLITVAL_SrcFormat_RGB,
										    BLITTAG_DstWidth, width,
										    BLITTAG_DstHeight, height,
											BLITTAG_Modulo, width * 3,
								            TAG_DONE );

										bitmapobject = BitmapObject,
													MUIA_Bitmap_Height, gfx_bitmap_height(bm),
													MUIA_Bitmap_Width,  gfx_bitmap_width(bm),
													MUIA_FixHeight,     gfx_bitmap_height(bm),
													MUIA_FixWidth,      gfx_bitmap_width(bm),
													MUIA_Bitmap_Bitmap, gfx_bitmap_bm(bm),
													MUIA_Bitmap_Alpha,  0xffffffff,
													End;

										if(bitmapobject)
										{
											if(muiRenderInfo(data->list) && _win(data->list))
											{
												image = (APTR) methodstack_push_sync(data->list, 3, MUIM_List_CreateImage, bitmapobject, 0);

												if(image)
												{
													methodstack_push(entry, 3, MUIM_Set, MA_Icon_Image, image);

													methodstack_push_sync(entry, 1, MM_ListviewEntry_Redraw);

													getlocaltime(&tv_end);

													elapsed = (tv_end.tv_secs - tv_start.tv_secs)*1000000 + (tv_end.tv_micro - tv_start.tv_micro);

													if(elapsed < 1000000/framerate)
													{
														tv.tv_secs  = 0;
														tv.tv_micro = 1000000/framerate - elapsed;
														timer_addreq_sync(timer, &tv);
													}
												}
											}
										}
									}

									if (!ThbNextFrame(thumb))
									{
										abort = TRUE;
									}
								}
								else
								{
									tv.tv_secs  = 0;
									tv.tv_micro = 100000;
									timer_addreq_sync(timer, &tv);
								}
							}
						}

						if(image)
						{
							methodstack_push_sync(data->list, 2, MUIM_List_DeleteImage, image);
							image = NULL;
						}

						if(bitmapobject)
						{
							MUI_DisposeObject(bitmapobject);
							bitmapobject = NULL;
						}

						if(bm)
						{
							gfx_bitmap_delete(bm);
							bm = NULL;
						}
					}

					ThbDeleteThumbnail(thumb);
				}
			}
		}

		/* restore old static bitmap */
		methodstack_push(entry, 3, MUIM_Set, MA_Icon_Image, oldimage);
		methodstack_push_sync(entry, 1, MM_ListviewEntry_Redraw);

		timer_delete(timer);
	}

  /*
   * We no longer access the iconobj in question, so let it
   * to be disposed. - Piru
   */
  //kprintf("%s: OM_RELEASE entry %p\n", __func__, entry);
  methodstack_push(entry, 1, OM_RELEASE);

#endif

	return (rc);
}

static ULONG listview_createicon(APTR viewobj, struct createicon_args * args)
{
	struct Data * data = (struct Data *) args->d;
	struct aline *al   = args->entry;
	//APTR entry         = args->entry;
	APTR obj           = args->obj;
	APTR sbm           = args->bm;
	ULONG cache        = args->cache;
	//APTR  av_ctx       = args->av_ctx;

	APTR bm          = NULL;
	APTR bms         = NULL;
	ULONG rc         = FALSE;
	APTR  image      = NULL;
	struct listview_deficon_node * deficon = NULL;
	APTR entry = al->obj;

	THREAD;
	CHECKOBJECT(obj);
	CHECKOBJECT(entry);

	D(CREATEICON, bug("al->invalidate_cache = %ld, al->invalidate_mimetype = %ld, entry = 0x%08lx\n", al->invalidate_cache, al->invalidate_mimetype, entry)); //?bitRocky
	if (!al->invalidate_cache && !al->invalidate_mimetype)
		methodstack_push_sync(entry, 3, OM_GET, MA_Icon_Image, &image);
		D(CREATEICON, bug("image = 0x%08lx\n", image));

	if (!image)
	{
		TEXT   iconname[PATH_SIZE];
		ULONG  seconds = 0;
		ULONG  hash;
		ULONG  icontype  = LISTVIEW_ICONTYPE_ICON;
		ULONG  isdefault = FALSE;
		LONG   percent   = -1;

		methodstack_push_sync(entry, 3, OM_GET, MA_Icon_FileDate, &seconds);

		if(listview_geticon(data, entry, seconds, iconname, &icontype, &isdefault, &percent, al))
		{
			hash = hash_nocase_seconds(iconname, seconds);

			deficon = listview_finddeficon(iconname, data);
			D(CREATEICON, bug("deficon = 0x%08lx, iconname = '%s', isdefault = %ld\n", deficon, iconname, isdefault));
			if (al->invalidate_cache)
			{
				if (deficon && !isdefault)
				{
					REMOVE(deficon);
					D(CREATEICON, bug("Call MM_Listview_InvalidateIcon with deficon = 0x%08lx\n", deficon));
					methodstack_push(viewobj, 2, MM_Listview_InvalidateIcon, deficon);
					deficon = NULL;
				}
				al->invalidate_cache = 0;
			}

			if (!deficon)
			{
				D(CREATEICON, bug("Object %s had no deficon...\n", iconname));
				/* if we have a source bitmap, we use it, otherwise, we try to fetch it from cache  */
				if(sbm)
				{
					bm = sbm;
				}
				else if(cache)
				{
					bm = get_cached_thumbnail(data, hash);
				}
				/* then we have to create the bitmap (picture thumbnail is not cached here. it's done in thumbs.c)*/
				if(!bm)
				{
					switch(icontype)
					{
						case LISTVIEW_ICONTYPE_VIDEO:
						case LISTVIEW_ICONTYPE_PICTURE:
							bm = listview_createthumbbitmap(iconname);
							cache = FALSE;
							break;
						
						case LISTVIEW_ICONTYPE_ICON:
							D(CREATEICON, bug("Object %s create icon...\n", iconname));
							bm = listview_createiconbitmap(obj, iconname, 64, 64);
							cache = FALSE;
							break;
					}

					if(bm && cache && hash)
						cache_set( hash, CACHETAG_THUMBNAIL, bm );
				}

				/* now we have a bitmap, we rescale it to target size, and add it as deficon to the list */
				if(bm)
				{
					bms = listview_rescaleicon(bm, data->iconsize, data->iconsize);
					if(bms)
					{
						deficon = listview_createdeficon(obj, data, iconname, bms, isdefault);
						if(deficon)
						{
							//ObtainSemaphore(&(data->deficonlistsem));
							ADDTAIL(&(data->deficonlist), deficon);
							//ReleaseSemaphore(&(data->deficonlistsem));
						}
					}

					gfx_bitmap_delete(bm);
					bm = NULL;
				}
			}

			if(deficon)
			{
				D(CREATEICON, bug("Setting hash 0x%08lx to %s\n", hash, iconname));
				methodstack_push(entry, 3,      MUIM_Set, MA_Icon_Image, deficon->image);
				methodstack_push_sync(entry, 3, MUIM_Set, MA_Icon_Hash, hash);
				rc = TRUE;
			}
		}

		if(rc == FALSE)
		{
			if(bm)
			{
				gfx_bitmap_delete(bm);
			}

			if(bms)
			{
				gfx_bitmap_delete(bms);
			}
		}
	}

	return rc;
}

static void listview_deleteicon(APTR obj UNUSED, APTR entry, ULONG invalidate)
{
	//APTR image;

	MAINTASK;

	if(entry == NULL)
		return;

	//image = (APTR) getv(entry, MA_Icon_Image);

	//if(image)
	{
		set(entry, MA_Icon_Image, NULL);
	}

	/* remove image from cache */
	if(invalidate)
	{
		ULONG hash = (ULONG)  getv(entry, MA_Icon_Hash);

		if(hash)
		{
			cache_invalidate( hash, CACHETAG_THUMBNAIL );
		}
	}
}

static void listview_deletedeficon(APTR obj, struct listview_deficon_node * n, APTR d)
{
	struct Data * data = (struct Data *) d;

	MAINTASK;

	if (muiRenderInfo(obj) && _win(obj))
	{
		if(n->image)
		{
			DoMethod(obj, MUIM_List_DeleteImage, n->image);
		}
	}

	if(n->bitmap)
	{
		gfx_bitmap_delete(n->bitmap);
	}

	if(n->bitmapobject)
	{
		MUI_DisposeObject(n->bitmapobject);
	}

	if(n->iconpath)
	{
		FreeVecPooled(data->npool, n->iconpath);
	}

	FreeVecPooled(data->npool, n);
}

DEFSMETHOD(Listview_FindIcon)
{
	GETDATA;
	if(data->icondisplay)
	{
		return ((ULONG) listview_findicon(msg->entry, msg->type, data, TRUE));
	}
	return(0);
}

DEFTMETHOD(Listview_SetupIcons)
{
	GETDATA;

	//ObtainSemaphore(&(data->deficonlistsem));
	NEWLIST(&(data->deficonlist));
	//ReleaseSemaphore(&(data->deficonlistsem));

	SetAttrs(data->list, MUIA_List_MinLineHeight, data->iconsize, MA_Listviewlist_IconDisplay, data->icondisplay, TAG_DONE);

	data->icons_setup_done = TRUE;

	return (0);
}

DEFTMETHOD(Listview_CleanupIcons)
{
	GETDATA;

	struct aline * al;
	struct listview_deficon_node * d, * nextd;

	ObtainSemaphore(&(data->deficonlistsem));

	ITERATELIST(al, &data->alist)
	{
		listview_deleteicon(data->list, al->obj, FALSE);
	}

	if(!ISLISTEMPTY(&data->deficonlist))
	{
		ITERATELISTSAFE(d, nextd, &data->deficonlist)
		{
			listview_deletedeficon(data->list, d, data);
		}
		NEWLIST(&data->deficonlist);
	}

	ReleaseSemaphore(&(data->deficonlistsem));

	data->icons_setup_done = FALSE;

	return (0);
}

/* should only be called in setup (createimage) */

static void listview_createlistimages(struct Data * data)
{
	struct listview_deficon_node * d, * nextd;

	if(data->defaultfile_bitmapobject)
		data->defaultfile_image  = (APTR) DoMethod(data->list, MUIM_List_CreateImage, data->defaultfile_bitmapobject, 0);

	if(data->defaultdir_bitmapobject)
		data->defaultdir_image   = (APTR) DoMethod(data->list, MUIM_List_CreateImage, data->defaultdir_bitmapobject, 0);

	if(data->defaultdisk_bitmapobject)
		data->defaultdisk_image  = (APTR) DoMethod(data->list, MUIM_List_CreateImage, data->defaultdisk_bitmapobject, 0);

	if(!ISLISTEMPTY(&data->deficonlist))
	{
		ITERATELISTSAFE(d, nextd, &data->deficonlist)
		{
			d->image = (APTR) DoMethod(data->list, MUIM_List_CreateImage, d->bitmapobject, 0);
		}
	}
}

static void listview_deletelistimages(struct Data * data)
{
	struct listview_deficon_node * d, * nextd;

	if(data->defaultfile_image)
		DoMethod(data->list, MUIM_List_DeleteImage, data->defaultfile_image);
	if(data->defaultdir_image)
		DoMethod(data->list, MUIM_List_DeleteImage, data->defaultdir_image);
	if(data->defaultdisk_image)
		DoMethod(data->list, MUIM_List_DeleteImage, data->defaultdisk_image);

	data->defaultfile_image  = NULL;
	data->defaultdir_image   = NULL;
	data->defaultdisk_image  = NULL;

	if(!ISLISTEMPTY(&data->deficonlist))
	{
		ITERATELISTSAFE(d, nextd, &data->deficonlist)
		{
			DoMethod(data->list, MUIM_List_DeleteImage, d->image);
			d->image = NULL;
		}
	}
}

DEFTMETHOD(Listview_CreateDefaultImages)
{
	GETDATA;
	ULONG xs, ys;
	TEXT icon[PATH_SIZE];

	if (muiRenderInfo(obj) && _win(obj))
	{
		/* XXX: use builtin default images (def_tool def_drawer def_disk arrays), instead of using these icons ? */
		/* XXX: should check for overflow */

		data->defaultfile_image  = NULL;
		data->defaultdir_image   = NULL;
		data->defaultdisk_image  = NULL;

		data->defaultfile_bitmapobject = NULL;
		data->defaultdir_bitmapobject  = NULL;
		data->defaultdisk_bitmapobject = NULL;

		deficon_getpath(icon, PATH_SIZE, "default.info");
		if(exists(icon))
			data->defaultfile_bitmap = imagecache_getbitmap(icon, data->iconsize);
		else
			data->defaultfile_bitmap = imagecache_getbitmap("tool", data->iconsize);

		deficon_getpath(icon, PATH_SIZE, "def_drawer.info");
		if(exists(icon))
			data->defaultdir_bitmap  = imagecache_getbitmap(icon, data->iconsize);
		else
			data->defaultdir_bitmap = imagecache_getbitmap("drawer", data->iconsize);

		deficon_getpath(icon, PATH_SIZE, "def_device.info");
		if(exists(icon))
			data->defaultdisk_bitmap = imagecache_getbitmap(icon, data->iconsize);
		else
			data->defaultdisk_bitmap = imagecache_getbitmap("disk", data->iconsize);

		if(data->defaultfile_bitmap)
		{
			xs = gfx_bitmap_width(data->defaultfile_bitmap);
			ys = gfx_bitmap_height(data->defaultfile_bitmap);
			data->defaultfile_bitmapobject = BitmapObject,
			                                        MUIA_Bitmap_Height, ys, MUIA_Bitmap_Width, xs,
			                                        MUIA_FixHeight, ys, MUIA_FixWidth, xs,
			                                        MUIA_Bitmap_Bitmap, gfx_bitmap_bm(data->defaultfile_bitmap),
													MUIA_Bitmap_Alpha, _conf(icon_defghosted) ? 0xffffffff - 0x88888888 : 0xffffffff,
			                                        End;
		}

		if(data->defaultdir_bitmap)
		{
			xs = gfx_bitmap_width(data->defaultdir_bitmap);
			ys = gfx_bitmap_height(data->defaultdir_bitmap);
			data->defaultdir_bitmapobject = BitmapObject,
			                                        MUIA_Bitmap_Height, ys, MUIA_Bitmap_Width, xs,
			                                        MUIA_FixHeight, ys, MUIA_FixWidth, xs,
			                                        MUIA_Bitmap_Bitmap, gfx_bitmap_bm(data->defaultdir_bitmap),
													MUIA_Bitmap_Alpha, _conf(icon_defghosted) ? 0xffffffff - 0x88888888 : 0xffffffff,
			                                        End;
		}

		if(data->defaultdisk_bitmap)
		{
			xs = gfx_bitmap_width(data->defaultdisk_bitmap);
			ys = gfx_bitmap_height(data->defaultdisk_bitmap);
			data->defaultdisk_bitmapobject = BitmapObject,
			                                        MUIA_Bitmap_Height, ys, MUIA_Bitmap_Width, xs,
			                                        MUIA_FixHeight, ys, MUIA_FixWidth, xs,
			                                        MUIA_Bitmap_Bitmap, gfx_bitmap_bm(data->defaultdisk_bitmap),
													MUIA_Bitmap_Alpha,  _conf(icon_defghosted) ? 0xffffffff - 0x88888888 : 0xffffffff,
			                                        End;
		}

		if(data->defaultfile_bitmapobject)
			data->defaultfile_image  = (APTR) DoMethod(data->list, MUIM_List_CreateImage, data->defaultfile_bitmapobject, 0);

		if(data->defaultdir_bitmapobject)
			data->defaultdir_image   = (APTR) DoMethod(data->list, MUIM_List_CreateImage, data->defaultdir_bitmapobject, 0);

		if(data->defaultdisk_bitmapobject)
			data->defaultdisk_image  = (APTR) DoMethod(data->list, MUIM_List_CreateImage, data->defaultdisk_bitmapobject, 0);

		data->icons_setup_done = TRUE;
	}

	return (0);
}

DEFTMETHOD(Listview_DeleteDefaultImages)
{
	GETDATA;

	if (muiRenderInfo(obj) && _win(obj))
	{
		if(data->defaultfile_image)
			DoMethod(data->list, MUIM_List_DeleteImage, data->defaultfile_image);
		if(data->defaultdir_image)
			DoMethod(data->list, MUIM_List_DeleteImage, data->defaultdir_image);
		if(data->defaultdisk_image)
			DoMethod(data->list, MUIM_List_DeleteImage, data->defaultdisk_image);

		if(data->defaultfile_bitmapobject)
			MUI_DisposeObject(data->defaultfile_bitmapobject);
		if(data->defaultdir_bitmapobject)
			MUI_DisposeObject(data->defaultdir_bitmapobject);
		if(data->defaultdisk_bitmapobject)
			MUI_DisposeObject(data->defaultdisk_bitmapobject);

		if(data->defaultfile_bitmap)
			imagecache_releasebitmap(data->defaultfile_bitmap);
		if(data->defaultdir_bitmap)
			imagecache_releasebitmap(data->defaultdir_bitmap);
		if(data->defaultdisk_bitmap)
			imagecache_releasebitmap(data->defaultdisk_bitmap);

		data->defaultfile_image  = NULL;
		data->defaultdir_image   = NULL;
		data->defaultdisk_image  = NULL;

		data->defaultfile_bitmapobject = NULL;
		data->defaultdir_bitmapobject  = NULL;
		data->defaultdisk_bitmapobject = NULL;

		data->defaultfile_bitmap = NULL;
		data->defaultdir_bitmap  = NULL;
		data->defaultdisk_bitmap = NULL;

		data->icons_setup_done = FALSE;
	}

	return (0);
}

DEFSMETHOD(View_GetEntry)
{
	struct aline *entry;

	DoMethod(obj, MUIM_List_GetEntry, msg->pos, (ULONG *)&entry);

	return (ULONG)(entry ? entry->obj : NULL);
}

DEFSMETHOD(View_Focus)
{
	GETDATA;
	struct aline *entry;
	LONG i = 0;

	for(;;)
	{
		DoMethod(obj, MUIM_List_GetEntry, i, (ULONG *)&entry);

		if ( !entry )
			return 0;

		if ( strcmp(entry->name, msg->name ) == 0)
		{
			ULONG vis = getv( data->list, MUIA_List_Visible );

			DoMethod(data->list, MUIM_List_Jump, i + vis - 2 );
			set(data->list, MUIA_List_Active, i);

			return 1;
		}
		i++;
	}

	return 0;
}

DEFTMETHOD(Rexx_Rename)
{
	GETDATA;
	return (DoMethod(data->list, MM_Rexx_Rename));
}

DEFTMETHOD(Listview_RunFileTypeThread)
{
	GETDATA;
	ULONG rc = FALSE;

	if(!data->filetypedisplay)
	{
		return(0);
	}

	data->thread_scantype_busy = TRUE;

	if(data->type == FLT_FILES)
	{
		struct MinList *ml;

		if ((ml = malloc(sizeof(*ml))))
		{
			//STRPTR p;
			int i;
			struct aline *al;
			struct typescannernode *tn, *nexttn;

			//p = (STRPTR)getv(obj, MA_View_Path);

			NEWLIST(ml);

			for(i=0;;i++)
			{
				DoMethod(data->list, MUIM_List_GetEntry, i, &al);

				if(al && al->obj)
				{
					if (!getv(al->obj, MA_Icon_MimeType))
					{
						STRPTR path = (STRPTR) getv(al->obj, MA_Icon_Path);

						if(path)
						{
							ULONG len = strlen(path)+1;

							if ((tn = (struct typescannernode *) malloc(sizeof(*tn) + len)))
							{
								strcpy(tn->path, path);
								tn->obj = al->obj;
								ADDTAIL(ml, tn);
							}
						}
					}
				}
				else
				{
					break;
				}
			}

			if(do_action(obj, TA_MimeType_Scan,
			                   TT_MimeType_Scan_List, (APTR) ml,
			                   TT_Priority, -2,
			                   TAG_DONE))
			{
				ml = NULL; /* thread will free nodes and the list */
				rc = TRUE;
			}
			else
			{
				ITERATELISTSAFE(tn, nexttn, ml)
				{
					free(tn);
				}
			}
			free(ml);
		}
	}

	if(rc == FALSE)
	{
		data->thread_scantype_busy = FALSE;
	}

	return(0);
}

DEFTMETHOD(Listview_RunVersionThread)
{
	GETDATA;
	ULONG rc = FALSE;

	if(!data->versiondisplay)
	{
		return(0);
	}

	data->thread_version_busy = TRUE;

	if(data->type == FLT_FILES)
	{
		struct MinList *ml;

		if ((ml = malloc(sizeof(*ml))))
		{
			//STRPTR p;
			int i;
			struct aline *al;
			struct typescannernode *tn, *nexttn;

			//p = (STRPTR)getv(obj, MA_View_Path);

			NEWLIST(ml);

			for(i=0;;i++)
			{
				DoMethod(data->list, MUIM_List_GetEntry, i, &al);

				if(al && al->obj)
				{
					if (!getv(al->obj, MA_Icon_Version))
					{
						ULONG len;
						STRPTR path;

						path = (STRPTR) getv(al->obj, MA_Icon_Path);

						if(path)
						{
							len = strlen(path)+1;

							if ((tn = (struct typescannernode *) malloc(sizeof(*tn) + len)))
							{
								strcpy(tn->path, path);
								tn->obj = al->obj;

								ADDTAIL(ml, tn);
							}
						}
					}
				}
				else
				{
					break;
				}
			}

			if(do_action(obj, TA_Version_Find,
			                   TT_Version_Find_List, (APTR) ml,
			                   TT_Priority, -2,
			                   TAG_DONE))
			{
				rc = TRUE;
			}
			else
			{
				ITERATELISTSAFE(tn, nexttn, ml)
				{
					free(tn);
				}
				free(ml);
			}
		}
	}

	if(rc == FALSE)
	{
		data->thread_version_busy = FALSE;
	}

	return(0);
}

DEFTMETHOD(Listview_RunMD5Thread)
{
	GETDATA;
	ULONG rc = FALSE;

	if(!data->md5display)
	{
		return(0);
	}

	data->thread_md5_busy = TRUE;

	if(data->type == FLT_FILES)
	{
		struct MinList *ml;

		if ((ml = malloc(sizeof(*ml))))
		{
			//STRPTR p;
			int i;
			struct aline *al;
			struct typescannernode *tn, *nexttn;

			//p = (STRPTR)getv(obj, MA_View_Path);

			NEWLIST(ml);

			for(i=0;;i++)
			{
				DoMethod(data->list, MUIM_List_GetEntry, i, &al);

				if(al && al->obj)
				{
					if(!getv(al->obj, MA_Icon_MD5))
					{
						ULONG len;
						STRPTR path;

						path = (STRPTR) getv(al->obj, MA_Icon_Path);

						if(path)
						{
							len = strlen(path)+1;

							if ((tn = (struct typescannernode *) malloc(sizeof(*tn) + len)))
							{
								strcpy(tn->path, path);
								tn->obj = al->obj;

								ADDTAIL(ml, tn);
							}
						}
					}
				}
				else
				{
					break;
				}
			}

			if(do_action(obj, TA_File_MD5sum,
			                   TT_File_MD5sum_List, (APTR) ml,
			                   TT_Priority, -2,
			                   TAG_DONE))
			{
				rc = TRUE;
			}
			else
			{
				ITERATELISTSAFE(tn, nexttn, ml)
				{
					free(tn);
				}
				free(ml);
			}
		}
	}

	if(rc == FALSE)
	{
		data->thread_md5_busy = FALSE;
	}

	return(0);
}

DEFSMETHOD(Listview_IsColumnVisible)
{
	GETDATA;

	return listview_is_shown(msg->col, &data->lv_format, data->type);
}

DEFTMETHOD(Listview_UpdateColumns)
{
	GETDATA;
	STRPTR format = (STRPTR) getv(data->list, MUIA_List_Format);

	/* if format has changed, then we update */
	if(!data->lv_format.string_format || (data->lv_format.string_format && strcmp(data->lv_format.string_format, format)))
	{
		TEXT  mode[64];
		ULONG shown;
		ULONG prev_icon     = listview_is_shown( (data->type == FLT_FILES) ? LISTVIEW_FILE_COL_ICON : LISTVIEW_DEVICE_COL_ICON, &data->lv_format, data->type);
		ULONG prev_version  = listview_is_shown(LISTVIEW_FILE_COL_VERSION, &data->lv_format, data->type);
		ULONG prev_filetype = listview_is_shown(LISTVIEW_FILE_COL_FILETYPE, &data->lv_format, data->type);
		ULONG prev_md5      = listview_is_shown(LISTVIEW_FILE_COL_MD5, &data->lv_format, data->type);

		snprintf(mode, sizeof(mode), "ICONSIZE=%ld SORTCOLUMN=%ld SORTMODE=%ld VIEWMODE=%ld",
		         data->iconsizeindex,
		         data->lv_format.sort_column,
		         data->lv_format.sort_mode,
		         data->lv_format.view_mode);

		listview_parse_format(format, mode, &data->lv_format, data->type);
DB(("after listview_parse_format(): data->lv_format.sort_column = %ld, data->lv_format.sort_mode = %ld\n", data->lv_format.sort_column, data->lv_format.sort_mode)); // bitRocky

		shown = listview_is_shown(LISTVIEW_FILE_COL_FILETYPE, &data->lv_format, data->type);

		if(data->type == FLT_FILES)
		{
			if(shown != prev_filetype)
			{
				if(shown)
				{
					data->filetypedisplay = TRUE;
					listview_abort_thread(TA_MimeType_Scan, TRUE, obj, data);
				}
				else
				{
					data->filetypedisplay = FALSE;
					listview_abort_thread(TA_MimeType_Scan, FALSE, obj, data);
				}
			}

			shown = listview_is_shown(LISTVIEW_FILE_COL_VERSION, &data->lv_format, data->type);

			if(shown != prev_version)
			{
				if(shown)
				{
					data->versiondisplay = TRUE;
					listview_abort_thread(TA_Version_Find, TRUE, obj, data);

				}
				else
				{
					data->versiondisplay = FALSE;
					listview_abort_thread(TA_Version_Find, FALSE, obj, data);
				}
			}

			shown = listview_is_shown(LISTVIEW_FILE_COL_MD5, &data->lv_format, data->type);

			if(shown != prev_md5)
			{
				if(shown)
				{
					data->md5display = TRUE;
					listview_abort_thread(TA_File_MD5sum, TRUE, obj, data);

				}
				else
				{
					data->md5display = FALSE;
					listview_abort_thread(TA_File_MD5sum, FALSE, obj, data);
				}
			}

		}

		shown = listview_is_shown( (data->type == FLT_FILES) ? LISTVIEW_FILE_COL_ICON : LISTVIEW_DEVICE_COL_ICON, &data->lv_format, data->type);

		if(shown != prev_icon)
		{
			data->icondisplay = shown;
			listview_abort_thread(TA_Listview_CreateIcons, TRUE, obj, data);
		}
	}

	return (0);
}

DEFTMETHOD(Listview_RunDirSizeThread)
{
	GETDATA;
	ULONG rc = FALSE;

	if(!data->sizesdisplay)
	{
		return(0);
	}

	data->thread_sizes_busy = TRUE;

	if(data->type == FLT_FILES)
	{
		struct MinList *ml;

		if ((ml = malloc(sizeof(*ml))))
		{
			//STRPTR p;
			int i;
			struct aline *al;
			struct typescannernode *tn, *nexttn;

			//p = (STRPTR)getv(obj, MA_View_Path);

			NEWLIST(ml);

			for(i=0;;i++)
			{
				DoMethod(data->list, MUIM_List_GetEntry, i, &al);

				if(al && al->obj)
				{
					if(al->type == ST_USERDIR)
					{
						QUAD * size = (QUAD *) getv(al->obj, MA_Icon_FileSize);

						if(size == NULL)
						{
							ULONG len;
							STRPTR path;

							path = (STRPTR) getv(al->obj, MA_Icon_Path);

							if(path)
							{
								len = strlen(path)+1;

								if ((tn = (struct typescannernode *) malloc(sizeof(*tn) + len)))
								{
									strcpy(tn->path, path);
									tn->obj = al->obj;

									ADDTAIL(ml, tn);
								}
							}
						}
					}
				}
				else
				{
					break;
				}
			}

			if(do_action(obj, TA_File_GetSizes,
			                   TT_File_GetSizes_List, (APTR) ml,
			                   TT_Priority, -2,
			                   TAG_DONE))
			{
				rc = TRUE;
			}
			else
			{
				ITERATELISTSAFE(tn, nexttn, ml)
				{
					free(tn);
				}
				free(ml);
			}
		}
	}

	if(rc == FALSE)
	{
		data->thread_sizes_busy = FALSE;
	}

	return (0);
}

DEFSMETHOD(View_CheckFocus)
{
	return  getv( msg->entry, MA_ListviewEntry_IsVisible );
}

DEFSMETHOD(View_QueryDisplayArea)
{
	GETDATA;
	struct Rect32 *r = msg->rect;

	r->MinX = 0;
	r->MaxX = _width( data->list );
	r->MinY = getv( data->list, MUIA_List_TopPixel );
	r->MaxY = r->MinY + getv( obj, MUIA_Height );

	return (ULONG)r;

}

DEFSMETHOD(View_RedrawEntry)
{
	DoMethod( msg->entry, MM_ListviewEntry_Redraw);
	return (0);
}

DEFTMETHOD(Listview_UpdateStatusBar)
{
	GETDATA;
	listview_update_statusbar_info(obj, data);
	return (0);
}

#if 0
/* TODO: verify tags and move to mui_internal.h */
#define MUIM_GoActive           0x8042491a
#define MUIM_GoInactive         0x80422c0c
#define MUIA_Window_DisableKeys 0x80424c36 /* V15 isg ULONG */
struct MUIP_GoActive { ULONG MethodID; };
struct MUIP_GoInactive { ULONG MethodID; };

DEFMMETHOD(GoActive)
{
	GETDATA;

	data->is_active = 1;
	set(_win(obj), MUIA_Window_DisableKeys, (1 << MUIKEY_GADGET_NEXT) | (1 << MUIKEY_GADGET_PREV));
	return (0);
}


DEFMMETHOD(GoInactive)
{
	GETDATA;
	DOSUPER;

	set(_win(obj), MUIA_Window_DisableKeys, 0);
	data->is_active = 0;
	return (0);
}
#endif

BEGINMTABLE
DECSMETHOD(Listview_AddFile)
DECSMETHOD(Listview_AddFiles)
DECSMETHOD(Listview_RemoveByName)
DECSMETHOD(Listview_RemoveFiles)
DECSMETHOD(Listview_FindIcon)
DECTMETHOD(Listview_SetupIcons)
DECTMETHOD(Listview_CleanupIcons)
DECTMETHOD(Listview_CreateDefaultImages)
DECTMETHOD(Listview_DeleteDefaultImages)
DECSET
DECGET
DECNEW
DECDISPOSE
DECTMETHOD(View_LoadURI)
DECTMETHOD(Listview_UpdateView)
DECTMETHOD(Listview_ShowDevices)
DECTMETHOD(Listview_ShowFiles)
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(AskMinMax)
DECTMETHOD(Listview_Clear)
DECSMETHOD(Thread_Finished)
DECMMETHOD(HandleEvent)
DECSMETHOD(Notify_Change)
DECMMETHOD(ContextMenuBuild)
DECMMETHOD(ContextMenuChoice)
DECMMETHOD(ContextMenuAdd)
DECSMETHOD(Rexx_Snapshot)
DECSMETHOD(Rexx_Unsnapshot)
DECSMETHOD(View_Refresh)
DECSMETHOD(View_GetEntry)
DECSMETHOD(View_Focus)
DECTMETHOD(Rexx_Rename)
DECTMETHOD(Listview_UpdateIconsSize)
DECSMETHOD(Listview_RunIconThread)
DECTMETHOD(Listview_RunFileTypeThread)
DECTMETHOD(Listview_RunVersionThread)
DECTMETHOD(Listview_RunMD5Thread)
DECTMETHOD(Listview_RunDirSizeThread)
DECTMETHOD(Listview_RunPreviewThread)
DECSMETHOD(Listview_ShowPreview)
DECTMETHOD(Listview_UpdateColumns)
DECTMETHOD(Listview_UpdateStatusBar)
DECTMETHOD(Listview_UpdateEntriesPositions)
DECSMETHOD(Listview_IsColumnVisible)
DECSMETHOD(Listview_GetColumnOrder)
//DECSMETHOD(Listview_GetColumnType)
DECSMETHOD(View_QueryDisplayArea)
DECSMETHOD(View_CheckFocus)
DECSMETHOD(View_RedrawEntry)
DECTMETHOD(View_Setup)
DECTMETHOD(View_Cleanup)
DECTMETHOD(View_PickSelected)
DECSMETHOD(Listview_InvalidateIcon)

/*
DECMMETHOD(GoActive)
DECMMETHOD(GoInactive)*/
ENDMTABLE

DECSUBCLASSPTR_NC(gviewclass, listviewclass)
