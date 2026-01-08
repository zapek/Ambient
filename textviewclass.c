/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2007 Ambient Open Source Team
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
 * $Id: textviewclass.c,v 1.17 2019/02/04 23:05:29 bitrocky Exp $
 */

#include "ambient.h"


#if USE_VIEW_TEXT

/* public */
#include <devices/rawkeycodes.h>
#include <mui/textinput_mcc.h>
#include <proto/dos.h>

/* private */
#include "keymap.h"
#include "print.h"
#include "textview.h"
#include "mui_func.h"
#include "threads.h"
#include "file_io.h"
#include "methodstack.h"
#include "storage.h"
#include "rexx.h"


#define IOBUFFERSIZE 2048 /* XXX: finetune */

struct Data {
	APTR list;
	APTR text;
	APTR searchGroup;
	ULONG barsdone;
	struct MUI_EventHandlerNode ehnode;

	APTR hscroller;
	APTR vscroller;
	ULONG inform;
	ULONG printing;
};

struct windowpos {
	LONG *left, *top;
	LONG *width, *height;
};

static void doset(APTR obj UNUSED, struct Data *data, struct TagItem *tags)
{
	FORTAG(tags)
	{
		case MA_Viewgroup_HScroller:
			data->hscroller = (APTR)tag->ti_Data;
			break;

		case MA_Viewgroup_VScroller:
			data->vscroller = (APTR)tag->ti_Data;
			break;

		case MUIA_Textinput_TopLine:
			break;

	}
	NEXTTAG
}

DEFNEW
{
	struct Data *data;
	APTR list = NULL, text, sGroup;

	obj = DoSuperNew(cl, obj,
		Child, text = TextinputscrollObject,
			InnerSpacing(2,2),                     /* force some space between window boarder and text */
			MUIA_Frame, MUIV_Frame_None,           /* no frame required */ 
			MUIA_Background, MUII_TextBack,
			MUIA_Textinput_Multiline, TRUE,
			MUIA_Textinput_NoInput, TRUE,
			MUIA_Textinput_Editable, FALSE,
			MUIA_Textinputscroll_UseWinBorder, TRUE,
			MUIA_Textinput_Font, MUIV_Textinput_Font_Fixed,
			//MUIA_Textinputscroll_HorizBar, GetTagData( MA_Viewgroup_HScroller, NULL, INITTAGS ),
			//MUIA_Textinputscroll_VertBar, GetTagData( MA_Viewgroup_VScroller, NULL, INITTAGS ),
		End,
		Child, sGroup = NewObject(getsearchbarclass(), NULL, MUIA_ShowMe, FALSE, TAG_DONE),
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return ((ULONG)NULL);
	}

	data = INST_DATA(cl, obj);
	data->list = list;
	data->text = text;
	data->searchGroup = sGroup;
	data->hscroller = NULL;
	data->vscroller = NULL;
	data->inform = FALSE;

	doset(obj, data, INITTAGS);

	data->ehnode.ehn_Object = obj;
	data->ehnode.ehn_Class = cl;
	data->ehnode.ehn_Events = IDCMP_RAWKEY;
	data->ehnode.ehn_Priority = 0;
	data->ehnode.ehn_Flags = MUI_EHF_GUIMODE;

	if ( !data->hscroller )
		data->hscroller = (APTR)getv(obj, MUIA_Scrollgroup_HorizBar);
	if ( !data->vscroller )
		data->vscroller = (APTR)getv(obj, MUIA_Scrollgroup_VertBar);

	set(data->text, MUIA_Textinput_TSCO, obj); /* spoof as the TIScroll class */

	/* disable styles and forbit URL highlight. still too slow */

	set(data->text, MUIA_Textinput_Styles, MUIV_Textinput_Styles_None);
	set(data->text, MUIA_Textinput_ProhibitParse, TRUE);

	set(data->searchGroup, MA_Searchbar_Target, obj);

	DoMethod(data->vscroller, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, data->text,3,MUIM_Set,MUIA_Textinput_TopOffset,MUIV_TriggerValue);
	DoMethod(data->hscroller, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, data->text,3,MUIM_Set,MUIA_Textinput_LeftOffset,MUIV_TriggerValue);
	return ((ULONG)obj);
}

DEFMMETHOD(Textinputscroll_Inform)
{
	GETDATA;

	if (msg->xv && msg->yv && !data->inform)
	{
		data->inform = TRUE; /* TextInput sucks balls deep */

		SetAttrs(data->hscroller,MUIA_NoNotify,TRUE,MUIA_Prop_Entries,msg->xs,MUIA_Prop_First,msg->xo,MUIA_Prop_Visible,msg->xv,TAG_DONE);
		SetAttrs(data->vscroller,MUIA_NoNotify,TRUE,MUIA_Prop_Entries,msg->ys,MUIA_Prop_First,msg->yo,MUIA_Prop_Visible,msg->yv,TAG_DONE);

		data->inform = FALSE;
	}

	return 0;
}

DEFMMETHOD(AskMinMax)
{
    STRPTR buff = NULL;

	DOSUPER;

	storage_get(STORAGE_TEXTVIEW_SNAPSHOT, STORAGE_STRING, (APTR*)&buff);
	if (buff != NULL)
	{
		struct windowpos wp;
		struct RDArgs *rda;

		memset(&wp, 0, sizeof(wp));
		rda	= readargsstring(buff, "LEFT/N,TOP/N,WIDTH/N,HEIGHT/N", (ULONG*)&wp);

		if (rda != NULL)
		{
			msg->MinMaxInfo->DefWidth = *wp.width != 0 ? *wp.width : 640;
			msg->MinMaxInfo->DefHeight = *wp.height != 0 ? *wp.height : 480;

			freeargsstring(rda);
		}

	}
	else
	{
		msg->MinMaxInfo->DefWidth = 640;
		msg->MinMaxInfo->DefHeight = 480;
	}

	return (0);
}


DEFGET
{
	switch (msg->opg_AttrID)
	{
		case MA_View_NeedsBackfill:
			*msg->opg_Storage = TRUE;	/* XXX: Just for now until i figure how to change it on the fly */
			return (TRUE);

		case MA_View_HasBackground:
			*msg->opg_Storage = FALSE;
			return (TRUE);

		case MA_View_NewWin:
			*msg->opg_Storage = FALSE;//TRUE;
			return (TRUE);

		case MA_View_Type:
			*msg->opg_Storage = MV_View_Type_Text;
			return (TRUE);

		case MA_View_ShowDevices:
			*msg->opg_Storage = FALSE;
			return (TRUE);
	}
	return (DOSUPER);
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

	if ((rc = DOSUPER))
	{
		GETDATA;

		DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehnode);

        /* initial window position */

        {
			STRPTR buff = NULL;

			storage_get(STORAGE_TEXTVIEW_SNAPSHOT, STORAGE_STRING, (APTR*)&buff);
			if (buff != NULL)
			{
				struct windowpos wp;
				struct RDArgs *rda;

				memset(&wp, 0, sizeof(wp));
				rda	= readargsstring(buff, "LEFT/N,TOP/N,WIDTH/N,HEIGHT/N", (ULONG*)&wp);

				if (rda != NULL)
				{
					if (wp.left != 0 && wp.top != NULL)
					{
						set(_win(obj), MUIA_Window_LeftEdge, *wp.left);
						set(_win(obj), MUIA_Window_TopEdge, *wp.top);
					}

					freeargsstring(rda);
				}
			}
		}

	}
	return (rc);
}


DEFMMETHOD(Cleanup)
{
	GETDATA;

	DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode);

	return (DOSUPER);
}


DEFMMETHOD(HandleEvent)
{
	struct IntuiMessage *imsg;

	if ((imsg = msg->imsg))
	{
		GETDATA;

		switch (imsg->Class)
		{
			case IDCMP_RAWKEY:
			{
				switch (imsg->Code)
				{
					long am, v;
					case RAWKEY_KP_8: /* this stupid class lacks settable MUIA_List_First ! */
					case RAWKEY_UP:
						DoMethod(data->text, MUIM_Textinput_DoUp);
						break;

					case RAWKEY_KP_2:
					case RAWKEY_DOWN:
						DoMethod(data->text, MUIM_Textinput_DoDown);
						break;

					case RAWKEY_KP_4:
					case RAWKEY_LEFT:
						v = getv(data->hscroller, MUIA_Prop_Visible);
						am = (msg->imsg->Qualifier & IEQUALIFIER_CONTROL) ? v*40/100 :
								(msg->imsg->Qualifier & IEQUALIFIER_SHIFTS) ? v*20/100 : _font(obj)->tf_XSize;
						DoMethod(data->hscroller, MUIM_Prop_Decrease, am);
						break;

					case RAWKEY_KP_6:
					case RAWKEY_RIGHT:
						v = getv(data->hscroller, MUIA_Prop_Visible);
						am = (msg->imsg->Qualifier & IEQUALIFIER_CONTROL) ? v*40/100 :
								(msg->imsg->Qualifier & IEQUALIFIER_SHIFTS) ? v*20/100 : _font(obj)->tf_XSize;
						DoMethod(data->hscroller, MUIM_Prop_Increase, am);
						break;

					case RAWKEY_KP_7:
					case RAWKEY_HOME:
						DoMethod(data->text, MUIM_Textinput_DoLineStart);
						break;

					case RAWKEY_KP_1:
					case RAWKEY_END:
						DoMethod(data->text, MUIM_Textinput_DoLineEnd);
						break;

					case RAWKEY_KP_9:
					case RAWKEY_PAGEUP:
						DoMethod(data->text, MUIM_Textinput_DoPageUp);
						break;

					case RAWKEY_KP_3:
					case RAWKEY_PAGEDOWN:
						DoMethod(data->text, MUIM_Textinput_DoPageDown);
						break;

					case NM_WHEEL_LEFT:
						DoMethod(data->hscroller, MUIM_Prop_Decrease, max(1, getv(data->hscroller, MUIA_Prop_Visible)*5/100));
						break;
					
					case NM_WHEEL_RIGHT:
						DoMethod(data->hscroller, MUIM_Prop_Increase, max(1, getv(data->hscroller, MUIA_Prop_Visible)*5/100));
						break;

					case NM_WHEEL_DOWN:
						{
							LONG tl = getv( data->text, MUIA_Textinput_TopLine );
							set( data->text, MUIA_Textinput_TopLine, tl + 3 );
						}
						break;
					case NM_WHEEL_UP:
						{
							LONG tl = getv( data->text, MUIA_Textinput_TopLine );
							set( data->text, MUIA_Textinput_TopLine, max( 0, tl - 3 ) );
						}
						break;

					case RAWKEY_PRTSCREEN:
						if (!data->printing)
						{
							STRPTR text, buf;
							ULONG textlen;

							text = (STRPTR)getv(data->text, MUIA_Textinput_Contents);
							textlen = strlen(text);

							buf = AllocVec(textlen + 1, MEMF_ANY);

							if (buf)
							{
								strcpy(buf, text);

								if (do_action(obj, TA_Print, TT_Print_Source, buf, TT_Print_Type, PRT_TEXT, TAG_DONE))
								{
									data->printing = 1;
								}
								else
								{
									FreeVec(buf);
								}
							}
						}
						break;

					default: /* VANILLAKEY */
						switch (keymap_vanilla(msg->imsg))
						{
							case	'/':
							{
								set(_win(obj), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_None);
								set(data->searchGroup, MUIA_ShowMe, TRUE);
							}
							break;
						}
						break;
				}

				return (MUI_EventHandlerRC_Eat);
			}
			break;
		}
	}

	return (0);
}


DEFTMETHOD(View_LoadURI)
{
	DoSuperMethod(cl, obj, MM_View_SetStatus, MF_View_SetStatus_Window | MF_View_SetStatus_Bar | MF_View_SetStatus_Busy, "Loading text..");

	do_action(obj, TA_Textview_Load,
		TT_Textview_Load_Path, getv(obj, MA_View_Path), TAG_DONE
	);

	return (0);
}


DEFSMETHOD(Thread_Finished)
{
	GETDATA;

	if (msg->action == TA_Print)
	{
		data->printing = 0;
	}

	if (msg->status == MV_Thread_Finished_Abort)
	{
		return (DOSUPER);
	}
	else
	{
		switch (msg->action)
		{
			case TA_Textview_Load:
				DoSuperMethod(cl, obj, MM_View_SetStatus, MF_View_SetStatus_Window, "%s", FilePart((STRPTR)getv(obj, MA_View_Path)));
				break;
		}
		return (0);
	}
}

DEFSMETHOD(Search)
{
	STRPTR c;
	STRPTR str;
	LONG pos;
	LONG len;
	ULONG rc = FALSE;

	GETDATA;

	c = (STRPTR)getv(data->text, MUIA_Textinput_Contents);
	
	pos = getv(data->text, MUIA_Textinput_CursorPos);
	if (msg->direction == MV_Search_SearchNext)
	{
		pos++;
	}
	else if (msg->direction == MV_Search_SearchPrev)
	{
		if (pos > 0)
			pos--;
	}
	else
	{
		/* XXX: Kiero: this makes it search always from top and not cursor position. disabled for now */
		//pos = 0;
	}

	c += pos;

	str = msg->string;

	if (str == NULL)
	{
		DB(("NULL passed to search\n"));
		return rc;
	}

	if (*str == 0)
	{
		set(data->text, MUIA_Textinput_MarkStart, MUIV_Textinput_NoMark);
		return rc;
	}

	len = strlen(str);

	if (msg->direction != MV_Search_SearchPrev)
	{
		while (TRUE)
		{
			if (msg->casesensitive == 0)
			{
				if (strncasecmp(c, str, len) == 0)
				{
					set(data->text, MUIA_Textinput_CursorPos, pos);
					set(data->text, MUIA_Textinput_MarkStart, pos);
					set(data->text, MUIA_Textinput_MarkEnd, pos + len - 1);
					rc = TRUE;
					break;
				}
			}
			else
			{
				if (strncmp(c, str, len) == 0)
				{
					set(data->text, MUIA_Textinput_CursorPos, pos);
					set(data->text, MUIA_Textinput_MarkStart, pos);
					set(data->text, MUIA_Textinput_MarkEnd, pos + len - 1);
					rc = TRUE;
					break;
				}
			}
			pos++;
			c++;

			if (*c == 0)
			{
				set(data->text, MUIA_Textinput_MarkStart, MUIV_Textinput_NoMark);
				break;
			}
		}
	}
	else
	{
		while (TRUE)
		{
			if (msg->casesensitive == 0)
			{
				if (strncasecmp(c, str, len) == 0)
				{
					set(data->text, MUIA_Textinput_CursorPos, pos);
					set(data->text, MUIA_Textinput_MarkStart, pos);
					set(data->text, MUIA_Textinput_MarkEnd, pos + len - 1);
					rc = TRUE;
					break;
				}
			}
			else
			{
				if (strncmp(c, str, len) == 0)
				{
					set(data->text, MUIA_Textinput_CursorPos, pos);
					set(data->text, MUIA_Textinput_MarkStart, pos);
					set(data->text, MUIA_Textinput_MarkEnd, pos + len - 1);
					rc = TRUE;
					break;
				}
			}
			pos--;
			c--;

			if (pos == 0)
			{
				set(data->text, MUIA_Textinput_MarkStart, MUIV_Textinput_NoMark);
				break;
			}
		}
	}
	return rc;
}

DEFSMETHOD(Rexx_Snapshot)
{
	LONG left, top;
	LONG width, height;
	TEXT buff[128];

	left = getv(_win(obj), MUIA_Window_LeftEdge);
	top	= getv(_win(obj), MUIA_Window_TopEdge);
	width = _width(obj);
	height = _height(obj);

	snprintf(buff, sizeof(buff), "%ld %ld %ld %ld", left, top, width, height);
	storage_set(STORAGE_TEXTVIEW_SNAPSHOT, STORAGE_STRING, buff);

	return (0);
}

DEFSMETHOD(Rexx_Unsnapshot)
{
	/* XXX: Note to self: add storage_remove() */
	storage_set(STORAGE_TEXTVIEW_SNAPSHOT, STORAGE_STRING, "");
	return (DOSUPER);
}

/* XXX: No idea why ContextMenuBuild is not called. This is a workaround */

DEFMMETHOD(ContextMenuAdd)
{
	APTR cmenu = (APTR)DoSuperMethod(cl, obj, MUIM_ContextMenuBuild, msg->mx, msg->my);

	if (cmenu != NULL)
	{
		APTR menu = NULL;

		FORCHILD(cmenu, MUIA_Family_List)
		{
			menu = child;
			DoMethod(cmenu, OM_REMMEMBER, menu);
			break;
		}
		NEXTCHILD;

		if (menu != NULL)
			DoMethod(msg->menustrip,OM_ADDMEMBER,menu);

		MUI_DisposeObject(cmenu);
	}

	return( DOSUPER );
}

BEGINMTABLE
DECNEW
DECGET
DECSET
DECMMETHOD(HandleEvent)
DECMMETHOD(Cleanup)
DECMMETHOD(Setup)
DECMMETHOD(AskMinMax)
DECTMETHOD(View_LoadURI)
DECSMETHOD(Thread_Finished)
DECMMETHOD(Textinputscroll_Inform)
DECSMETHOD(Search)
DECSMETHOD(Rexx_Snapshot)
DECSMETHOD(Rexx_Unsnapshot)
DECMMETHOD(ContextMenuAdd)
ENDMTABLE

DECSUBCLASSPTR_NC(viewclass, textviewclass)


ULONG tr_loadtext(APTR obj, CONST_STRPTR path)
{
	APTR fh;
	ULONG rc = TRUE;

	THREAD;

	/*  reset view, because we append text later only.
	 */
	methodstack_push(obj, 3, MUIM_Set, MUIA_Textinput_Contents, NULL);

	if ((fh = file_open(path, MODE_OLDFILE)))
	{
		STRPTR buf = malloc( 256 * 1024 );
		LONG len;

		if ( buf )
		{
			while ((len = file_readmost(fh, buf, 256 * 1024)) > 0)
			{
				ULONG i;

				if (threads_check_abort())
				{
					rc = ABORTED;
					break;
				}

				/* process non-native EOLs */

				for (i=1; i<len; i++)
				{
					if ( buf[ i ] == 10 && buf[ i - 1 ] == 13   )
					{
						buf[ i - 1 ] = ' ';
					}
				}

				methodstack_push_sync(obj, 3, MUIM_Textinput_AppendText, buf, len); /* XXX: no retval check, and the broadcast sucks */
			}
		}

		methodstack_push(obj, 3, MUIM_Set, MUIA_Textinput_CursorPos, 0);
		methodstack_push_sync(obj, 3, MUIM_Set, MUIA_Textinput_MarkStart, MUIV_Textinput_NoMark);

		file_close(fh);
	}
	return rc;
}

#endif
