/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2006 Ambient Open Source Team
 * notificationclass.c, Copyright 2005-2006 by Adam Waldenberg
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
 * $Id: notificationclass.c,v 1.11 2014/04/05 20:13:00 tcheko Exp $
 */

#include "ambient.h"
#include "classes.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "mimetype.h"
#include "info_error_logo.h"
#include "info_help_logo.h"
#include "info_warning_logo.h"

#include <macros/vapor.h>
#include "mui_func.h"

/* public */
#include <exec/types.h>

#include <proto/graphics.h>
#include <exec/rawfmt.h>

struct Data {
	APTR bitmap, bmp_o, txt_o;
	LONG type;

	/* text split */
	struct RastPort rp;
	STRPTR text;
	STRPTR formatedtext;
	LONG screenwidth;
};

static BOOL ischarinarray(STRPTR chars, BYTE c)
{
	while(*chars)
	{
		if(c == *chars) return TRUE;
		chars++;
	}
	return FALSE;
}

static LONG findlastfavoritechar(CONST STRPTR str, CONST STRPTR splitchar, LONG from)
{
	while(--from)
	{
		if(ischarinarray(splitchar, str[from])) return from;
	}
	return from;
}

static STRPTR splitmesmart(CONST STRPTR source, struct RastPort *rp, LONG width, CONST STRPTR splitchar)
{
	/* This function adds \n to source string when the string 
	   would overflow width length.
	   XXX: splitchar will be used to split string on more suitable characters. For
	   example, on a filepath, suitable split chars could be '/' and ' '.
	   
	   It returns a string allocated with AllocVecTaskPooled and should be freed. 
	*/

	struct TextExtent tex;
	STRPTR s = source;
	STRPTR d, p;
	LONG c, k;
	LONG len = strlen(source);
	struct TextFont *font = rp->Font;

	/* allocating ram for the worst case scenario, add a \n for every char */
	p = d = AllocVecTaskPooled(strlen(source) * 2 + 1);	/* +1 for \0 terminating char */

	if(d)
	{
		len = strlen(source);
		s = source;

		while(len > 0)
		{
			/* how much char fits in width? */
			c = TextFit(rp, s, strlen(s), &tex, NULL, 1, width, font->tf_YSize);

			if(c != strlen(s))
			{
				if((k = findlastfavoritechar(s, splitchar, c)) > 0) c = k;
			}

			len -= c;

			while(c--)
			{
				if(*s != '\n')
				{
					*p++ = *s++;
				}
				else
				{
					/* replace source \n for a space */
					*p++ = ' ';
					s++;
				}
			}
			if(*s != 0 && *s != '\n') *p++ = '\n'; /* add \n only if not the end of string or \n is already in the source string */
		}
		*p = 0;
	}
	
	return d;
}

static void set_notificationimg(struct Data *data)
{
	APTR img;
	int w, h;

	switch (data->type)
	{
		case MV_Notification_Error:
			w = INFO_ERROR_WIDTH;
			h = INFO_ERROR_HEIGHT;
			img = &info_error;
			break;

		case MV_Notification_Warning:
			w = INFO_WARNING_WIDTH;
			h = INFO_WARNING_HEIGHT;
			img = &info_warning;
			break;

		default: /* MV_Notification_Help */
			w = INFO_HELP_WIDTH;
			h = INFO_HELP_HEIGHT;
			img = &info_help;
			break;
	}

	set(data->bmp_o, MUIA_Bitmap_Bitmap, NULL);

	if (data->bitmap)
		gfx_bitmap_delete(data->bitmap);

	if ((data->bitmap = gfx_bitmap_create(w, h, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)))
	{
		gfx_blit(img, data->bitmap, BLITTAG_SrcType, BLITVAL_SrcType_Array,
			BLITTAG_SrcFormat, BLITVAL_SrcFormat_ARGB,
		TAG_DONE);

		SetAttrs(data->bmp_o,
			MUIA_Bitmap_Bitmap, gfx_bitmap_bm(data->bitmap),
			MUIA_Bitmap_Height, h,
			MUIA_Bitmap_Width, w,
			MUIA_FixHeight, h,
			MUIA_FixWidth, w,
			TAG_DONE
		);
	}
}

DEFNEW
{
	APTR txt_o = NULL, bmp_o = NULL;
	STRPTR text = NULL;
	LONG type = MV_Notification_Help;
	struct Data *data;

	FORTAG(INITTAGS)
	{
		case MA_Notification_Type:
			type = (LONG) tag->ti_Data;
			break;
		case MA_Notification_Text:
			text = AllocVecTaskPooled((LONG)strlen((char*)tag->ti_Data) + 1);
			if(text) strcpy(text, (const char*)tag->ti_Data);
	}
	NEXTTAG

	obj = DoSuperNew(cl, obj, MUIA_Group_Horiz, TRUE,
		Child, VGroup,
			Child, bmp_o = BitmapObject,
				MUIA_Bitmap_Alpha, 0xffffffff,
			End,
			Child, RectangleObject, MUIA_Weight, 0, End,
		End,
		Child, txt_o = TextObject,
		End,
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		data = INST_DATA(cl, obj);
		data->type = type;
		data->bitmap = NULL;
		data->text = text;
		data->txt_o = txt_o;
		data->bmp_o = bmp_o;
		set_notificationimg(data);
	
		InitRastPort(&data->rp);
	}

	return (ULONG) obj;
}

DEFSET
{
	GETDATA;

	FORTAG(INITTAGS)
	{
		case MA_Notification_Text:
			if(data->text) FreeVecTaskPooled(data->text);
			data->text = AllocVecTaskPooled((LONG)strlen((char*)tag->ti_Data) + 1);
			if(data->text)
			{
				strcpy(data->text, (const char*)tag->ti_Data);
			}
		break;

		case MA_Notification_Type:
			data->type = tag->ti_Data;
			set_notificationimg(data);
		break;
	}
	NEXTTAG

	return (DOSUPER);
}

DEFDISPOSE
{
	GETDATA;

	if (data->text)
		FreeVecTaskPooled(data->text);

	if (data->formatedtext)
		FreeVecTaskPooled(data->formatedtext);

	if (data->bitmap)
		gfx_bitmap_delete(data->bitmap);

	return DOSUPER;
}

DEFMMETHOD(Setup)
{
	GETDATA;
	LONG rc = DOSUPER;
	struct Screen *sc = _screen(obj);
	GetAttr(SA_Width, sc, &data->screenwidth);
	SetFont(&data->rp, _font(data->txt_o));

	if(data->text && data->screenwidth && TextLength(&data->rp, data->text, strlen(data->text)) > data->screenwidth * 0.5)
	{
		if(data->formatedtext) FreeVecTaskPooled(data->formatedtext);
		data->formatedtext = splitmesmart(data->text, &data->rp, data->screenwidth * 0.5, "/ .");

		set(data->txt_o, MUIA_Text_Contents, data->formatedtext);
	}
	else
	{
		set(data->txt_o, MUIA_Text_Contents, data->text);
	}


	return rc;
}

BEGINMTABLE
DECNEW
DECSET
DECDISPOSE
DECMMETHOD(Setup)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, notificationclass)
