/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006 Ambient Open Source Team
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
 * $Id: listviewentryclass.c,v 1.7 2013/10/29 22:33:15 geit Exp $
 */

#include "ambient.h"

/* public */
#include <proto/exec.h>

/* private */
#include "mui_func.h"
#include "gfx_bitmap.h"
#include "file_func.h"

struct Data {
	STRPTR path;
	STRPTR pathinfo;
	APTR   pool;
	ULONG  icontype; /* a bit useless, but needed for viewclass selection state */
	ULONG  filetype;
	APTR   mimetype;
	UQUAD  filesize;
	ULONG  filedate;
	STRPTR version;
	STRPTR md5;
	APTR   bitmap;
	APTR   bitmapobject;
	APTR   image;
	ULONG  hash;

	LONG   pos;
	APTR   list;
};


DEFNEW
{
	struct Data * data;

	obj = DoSuperNew(cl, obj,
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return (NULL);
	}

	data = INST_DATA(cl, obj);
	data->path = NULL;
	data->pathinfo = NULL;
	data->pool = NULL;
	data->icontype = MV_Icon_Type_Tool;
	data->filetype = MV_Icon_FileType_File;
	data->mimetype = NULL;
	data->version  = NULL;
	data->md5      = NULL;
	data->filesize = NO_FILESIZE;
	data->filedate = 0;

	data->list     = NULL;
	data->pos      = -1;

	/* these are only references.*/
	data->bitmap = NULL;
	data->bitmapobject = NULL;
	data->image = NULL;
	data->hash  = 0;

	return ((ULONG)obj);
}

DEFDISP
{
	GETDATA;

	if (data->path)
	{
		FreeVecPooled(data->pool, data->path);
		data->path = NULL;
	}

	if (data->pathinfo)
	{
		FreeVecPooled(data->pool, data->pathinfo);
		data->pathinfo = NULL;
	}

	if (data->version)
	{
		FreeVecPooled(data->pool, data->version);
		data->version = NULL;
	}

	if (data->md5)
	{
		FreeVecPooled(data->pool, data->md5);
		data->md5 = NULL;
	}

	return (DOSUPER);
}


DEFSET
{
	GETDATA;

	FORTAG(INITTAGS)
	{
		case MA_ListviewEntry_Pool:
			{
				data->pool = (APTR) tag->ti_Data;
				break;
			}


		case MA_Icon_Path:
			{
				if(data->pool)
				{
					if(data->path)
					{
						FreeVecPooled(data->pool, data->path);
					}

					data->path = AllocVecPooled(data->pool, strlen((STRPTR)tag->ti_Data)+1);

					if(data->path)
					{
						strcpy(data->path, (STRPTR)tag->ti_Data);
					}
				}
			}
			break;

		case MA_Icon_PathInfo:
			{
				if(data->pool)
				{
					if(data->pathinfo)
					{
						FreeVecPooled(data->pool, data->pathinfo);
					}

					data->pathinfo = AllocVecPooled(data->pool, strlen((STRPTR)tag->ti_Data)+1);

					if(data->pathinfo)
					{
						strcpy(data->pathinfo, (STRPTR)tag->ti_Data);
					}
				}
			}
			break;

		case MA_Icon_Type:
			data->icontype = tag->ti_Data;
			break;

		case MA_Icon_FileType:
			data->filetype = tag->ti_Data;
			break;

		case MA_Icon_MimeType:
			data->mimetype = (APTR) tag->ti_Data;
			break;

		case MA_Icon_FileSize:
			data->filesize = *((UQUAD *)tag->ti_Data);
			break;

		case MA_Icon_FileDate:
			data->filedate = (ULONG) tag->ti_Data;
			break;

		case MA_Icon_Version:
			{
				if(data->pool)
				{
					if(data->version)
					{
						FreeVecPooled(data->pool, data->version);
					}

					if(tag->ti_Data == NULL)
					{
						data->version = NULL;
					}
					else
					{
						data->version = AllocVecPooled(data->pool, strlen((STRPTR)tag->ti_Data)+1);

						if(data->version)
						{
							strcpy(data->version, (STRPTR)tag->ti_Data);
						}
					}
				}
			}
			break;

		case MA_Icon_MD5:
			{
				if(data->pool)
				{
					if(data->md5)
					{
						FreeVecPooled(data->pool, data->md5);
					}

					if(tag->ti_Data == NULL)
					{
						data->md5 = NULL;
					}
					else
					{
						data->md5 = AllocVecPooled(data->pool, strlen((STRPTR)tag->ti_Data)+1);

						if(data->md5)
						{
							strcpy(data->md5, (STRPTR)tag->ti_Data);
						}
					}
				}
			}
			break;

		case MA_Icon_Image:
			data->image = (APTR) tag->ti_Data;
			break;

		case MA_Icon_Bitmap:
			data->bitmap = (APTR) tag->ti_Data;
			break;

		case MA_Icon_BitmapObject:
			data->bitmapobject = (APTR) tag->ti_Data;
			break;

		case MA_Icon_Hash:
			data->hash = (ULONG) tag->ti_Data;
			break;

		case MA_ListviewEntry_Position:
			data->pos = tag->ti_Data;
			break;

		case MA_ListviewEntry_ListObject:
			data->list = (APTR) tag->ti_Data;
			break;

	}
	NEXTTAG

	return (DOSUPER);
}

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_Icon_Path:
			*msg->opg_Storage = (ULONG) data->path;
			return (TRUE);

		case MA_Icon_PathInfo:
			*msg->opg_Storage = (ULONG) data->pathinfo;
			return (TRUE);

		case MA_Icon_Type:
			*msg->opg_Storage = data->icontype;
			return (TRUE);

		case MA_Icon_FileType:
			*msg->opg_Storage = data->filetype;
			return (TRUE);

		case MA_Icon_FileSize:
			if(data->filesize != NO_FILESIZE)
			{
				*msg->opg_Storage = (ULONG) &data->filesize;
			}
			else
			{
				*msg->opg_Storage =  NULL;
			}
			return (TRUE);

		case MA_Icon_FileDate:
			*msg->opg_Storage = (ULONG) data->filedate;
			return (TRUE);

		case MA_Icon_MimeType:
			*msg->opg_Storage = (ULONG) data->mimetype;
			return (TRUE);

		case MA_Icon_Version:
			*msg->opg_Storage = (ULONG) data->version;
			return (TRUE);

		case MA_Icon_MD5:
			*msg->opg_Storage = (ULONG) data->md5;
			return (TRUE);

		case MA_Icon_Image:
			*msg->opg_Storage = (ULONG) data->image;
			return (TRUE);

		case MA_Icon_Bitmap:
			*msg->opg_Storage = (ULONG) data->bitmap;
			return (TRUE);

		case MA_Icon_BitmapObject:
			*msg->opg_Storage = (ULONG) data->bitmapobject;
			return (TRUE);

		case MA_Icon_Hash:
			*msg->opg_Storage = (ULONG) data->hash;
			return (TRUE);

		case MA_ListviewEntry_Position:
		{
			*msg->opg_Storage = (ULONG) data->pos;
			return (TRUE);
		}

		case MA_ListviewEntry_IsVisible:
		{
			//ULONG top    = getv(data->list, MUIA_List_TopPixel);
			ULONG height = getv(data->list, MUIA_List_VisiblePixel);
			LONG x, y;

			DoMethod(data->list, MUIM_List_QueryPosition, data->pos, 0, &x, &y);

			y -= _top(data->list);

			if( y >= 0 && y <= height)
			{
				*msg->opg_Storage = (ULONG) TRUE;
			}
			else
			{
				*msg->opg_Storage = (ULONG) FALSE;
			}
			return (TRUE);
		}
	}
	return (DOSUPER);
}

DEFTMETHOD(ListviewEntry_Redraw)
{
	GETDATA;

	if(data->list && data->pos >= 0)
	{
		DoMethod(data->list, MUIM_List_Redraw, data->pos);
	}
	return (0);
}

BEGINMTABLE
DECNEW
DECDISP
DECSET
DECGET
DECTMETHOD(ListviewEntry_Redraw)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Notify, listviewentryclass)


