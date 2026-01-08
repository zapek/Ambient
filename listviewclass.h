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
 * $Id: listviewclass.h,v 1.16 2017/04/06 21:29:57 itix Exp $
 */

/*
 * Line structure. Can be a file
 * or a device, depending on the
 * lister context..
 */

#ifndef AMBIENT_LISTVIEWCLASS_H
#define AMBIENT_LISTVIEWCLASS_H

/* list types */
enum {
	FLT_UNKNOWN,
	FLT_DEVICES,
	FLT_FILES,
};

/* files list */
#define LVFORMAT_FILES   "C=5,C=0 MIW=-1 MAW=-2,C=1,C=2,C=8 H,C=3,C=6 MIW=-1 H,C=4,C=7 H,C=9 H,C=10 H, C=11 H"
#define LISTVIEW_FILE_COL_NAME     0
#define LISTVIEW_FILE_COL_SIZE     1
#define LISTVIEW_FILE_COL_DATE     2
#define LISTVIEW_FILE_COL_ATTRS    3
#define LISTVIEW_FILE_COL_COMMENT  4
#define LISTVIEW_FILE_COL_ICON     5
#define LISTVIEW_FILE_COL_FILETYPE 6
#define LISTVIEW_FILE_COL_VERSION  7
#define LISTVIEW_FILE_COL_FULLDATE 8
#define LISTVIEW_FILE_COL_UID      9
#define LISTVIEW_FILE_COL_GID     10
#define LISTVIEW_FILE_COL_MD5     11

#define LISTVIEW_FILE_COL_COUNT   12

/* devices list */
#define LVFORMAT_DEVICES_BASE          1024
#define LVFORMAT_DEVICES "C=5,C=0 MIW=-1 MAW=-2,C=1,C=2,C=3,C=4"
#define LISTVIEW_DEVICE_COL_VOLUME    (LVFORMAT_DEVICES_BASE + 0)
#define LISTVIEW_DEVICE_COL_DEVICE    (LVFORMAT_DEVICES_BASE + 1)
#define LISTVIEW_DEVICE_COL_FREE      (LVFORMAT_DEVICES_BASE + 2)
#define LISTVIEW_DEVICE_COL_TOTAL     (LVFORMAT_DEVICES_BASE + 3)
#define LISTVIEW_DEVICE_COL_DOSTYPE   (LVFORMAT_DEVICES_BASE + 4)
#define LISTVIEW_DEVICE_COL_ICON      (LVFORMAT_DEVICES_BASE + 5)

#define LISTVIEW_DEVICE_COL_COUNT     6

/* view options */
#define LVMODE_DEFAULT_FILES   "ICONSIZE=0 SORTCOLUMN=0 SORTMODE=1 VIEWMODE=1"
#define LVMODE_DEFAULT_DEVICES "ICONSIZE=0 SORTCOLUMN=0 SORTMODE=1 VIEWMODE=0"

struct aline {
	struct MinNode n;
	LONG nodetype;
	LONG type;
	STRPTR name;
	STRPTR date;   /* in seconds */
	STRPTR time;
	ULONG namelen; /* speed up */
	BYTE  invalidate_cache;
	BYTE  invalidate_mimetype;
	BYTE  active;
	ULONG newentry;
	ULONG removed;

	union {
		struct aline_file {
			UQUAD size;
			LONG protection;
			ULONG date;
			STRPTR comment;
			STRPTR uid;
			STRPTR gid;
			STRPTR target; /* resolved link */
		} file;
		struct aline_device {
			QUAD full;
			QUAD total;
			STRPTR devname; /* DH0, etc.. */
			ULONG devnamelen;
			ULONG dostype;  /* SFS0, etc.. */
		} dev;
	} un;

	APTR obj;      /* listviewentryclass object */

	struct DateStamp datestamp;
};

/* listview format description */
struct listview_format
{
	STRPTR string_format; /* string representation */
	ULONG icon_size;      /* iconsize index */
	ULONG sort_column;    /* sorted column index */
	LONG sort_mode;       /* 1 for asc, -1 for desc */
	ULONG view_mode;      /* listview submode index */

	union
	{
		struct listview_format_file
		{
			LONG icon; /* -1 = hidden, else, column order */
			LONG name;
			LONG size;
			LONG date;
			LONG attrs;
			LONG comment;
			LONG filetype;
			LONG version;
			LONG md5;
			LONG fulldate;
			LONG uid;
			LONG gid;
		} file;

		struct listview_format_device
		{
			LONG icon;
			LONG volume;
			LONG device;
			LONG free;
			LONG total;
			LONG dostype;
		} dev;
	} un;
};


ULONG tr_listview_createicons(APTR obj, APTR iconlist, APTR d);
ULONG tr_listview_showdevices(APTR obj, APTR d);
ULONG tr_listview_showpreview(APTR obj, APTR entry, APTR d);

struct column_entry;

ULONG listview_get_columns(CONST_STRPTR format, ULONG type, struct column_entry *** columns);
ULONG listview_generate_format(struct column_entry ** columns, STRPTR * format);
void  listview_parse_mode(CONST_STRPTR mode, ULONG type, struct listview_format * format);

void listview_free_columns(struct column_entry ** columns);
void listview_free_format(STRPTR format);

ULONG listview_check_format_string(CONST_STRPTR format, int mode);

#endif
