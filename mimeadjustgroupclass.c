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
 * $Id: mimeadjustgroupclass.c,v 1.12 2016/01/02 19:07:23 itix Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>

/* private */
#include "mimeadjustgroupclass.h"
#include "ambient_cat.h"
#include "mui_func.h"
#include "mimetype.h"
#include "recog.h"
#include "action.h"
#include "iconio.h"
#include "methodstack.h"
#include "name.h"
#include "deficon_getpath.h"
#include "file_func.h"
#include "def_tool_logo.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "prefs.h"
#include "descsaver.h"
#include "threads.h"
#include "deficonpool.h"
#include "cache.h"
#include "smartreq.h"

#define TJ_ICONREAD  (1 << 0UL)
#define TJ_ICONWRITE (1 << 1UL)

struct window_entry
{
	struct MinNode n;
	APTR obj;
};

struct Data {
	struct internal_mimetype_node *mime_node_temp; /* mimetype node */
	struct MinList generic_action_list;
	ULONG  edit;                        /* edit mode ? */
	ULONG  generic;                     /* generic mimetype */
	ULONG  accept;                      /* set to true if mimetype is validated */

	APTR cyc_mime;
	APTR str_mime;
	APTR str_descr;
	APTR str_pri;
	APTR grp_action;
	APTR lst_actions;

	APTR bt_add;
	APTR bt_remove;
	APTR bt_edit;
	APTR bt_up;
	APTR bt_down;

	APTR deficon;                       /* icon object */
	STRPTR iconpath;                    /* icon path */
	APTR iconarea;                      /* where to insert icon */
	APTR icongroup;                     /* icon group object */
	ULONG iconchanged;

	ULONG jobs;                         /* pending threads flag */
	ULONG closing;                      /* set when window has to be closed */
	struct MinList windows_list;
};

static STRPTR cycleopts[] =
{
	"application",
	"audio",
	"image",
	"message",
	"model",
	"multipart",
	"text",
	"video",
	"internal",
	NULL,
};

/* build mimetype from cycle + string */
static STRPTR get_mimetype(APTR obj UNUSED, struct Data * data, STRPTR mimetype, ULONG size, ULONG generic)
{
	STRPTR ptr = NULL;

	stccpy(mimetype, cycleopts[getv(data->cyc_mime, MUIA_Cycle_Active)], size);

	if(generic)
	{
		ptr = "*";
	}
	else
	{
		ptr = (STRPTR) getv(data->str_mime, MUIA_String_Contents);
	}

	if(ptr)
	{
		strncat(mimetype, "/", size);
		strncat(mimetype, ptr, size);
		ptr = mimetype;
	}
	else
	{
		ptr = NULL;
	}

	return (ptr);
}

static void free_generic_actions(struct Data * data)
{
	if ( !ISLISTEMPTY( &data->generic_action_list ) )
	{
		APTR an;

		while((an = REMHEAD( &data->generic_action_list )))
		{
			actionnode_delete(an);
		}
	}
}

/* get actions from family (e.g video / *) and copy them locally */
static void get_generic_actions(APTR obj, struct Data * data)
{
	TEXT generic_mimetype[256];
	STRPTR ptr;

	free_generic_actions(data);

	ptr = get_mimetype(obj, data, generic_mimetype, sizeof(generic_mimetype), TRUE);

	if(ptr)
	{
		struct internal_mimetype_node * generic_mime_node = mimetype_find_generic_by_mimetype(generic_mimetype);

		if(generic_mime_node)
		{
			NEWLIST( &data->generic_action_list );

			if ( !ISLISTEMPTY( generic_mime_node->action_list ) )
			{
				APTR san = NULL;
				APTR dan = NULL;

				ITERATELIST( san, generic_mime_node->action_list )
				{
					dan = actionnode_duplicate( san );

					if(dan)
					{
						ADDTAIL(&data->generic_action_list, dan);
					}
				}
			}
		}
	}
}

static void mimeadjustgroup_refresh_actions(APTR obj, struct Data * data)
{
	/* assign them if type has generic actions */
	if(data->mime_node_temp->flags & MIMETYPEFLAG_FOREIGNACTIONS)
	{
		get_generic_actions(obj, data);
		data->mime_node_temp->action_list = &data->generic_action_list;
	}

	/* fill with actionlist */
	if(data->mime_node_temp)
	{
		struct MinList * action_list = data->mime_node_temp->action_list;

		DoMethod(data->lst_actions, MUIM_List_Clear);

		if ( action_list && !ISLISTEMPTY( action_list ) )
		{
			APTR an;

			ITERATELIST( an, action_list )
			{
				APTR aearray[2];

				aearray[0] = an;
				aearray[1] = (APTR) (data->generic ? FALSE : (data->mime_node_temp->flags & MIMETYPEFLAG_FOREIGNACTIONS));

				DoMethod(data->lst_actions, MUIM_List_InsertSingle, aearray, MUIV_List_Insert_Bottom);
			}
		}
	}
}

static STRPTR get_iconpath_from_mimetype(STRPTR mimetype, STRPTR buff, ULONG buff_size)
{
	STRPTR p = NULL;
	STRPTR separator = strchr( mimetype, '/' );	/* assume it's present */
	TEXT family[ 32 ];
	STRPTR type;
	TEXT deficon_path[ PATH_SIZE ];
	TEXT deficon_name[ PATH_SIZE ];

	stccpy( family, mimetype, separator - mimetype + 1 );
	type = separator + 1;

	if ( type[ 0 ] == '*' )
	{
		type = "default";
	}

	if (!deficon_getpath( deficon_path, sizeof( deficon_path ), family ))
	{
		return p;
	}

	snprintf( deficon_name, sizeof( deficon_name ), "%s/%s.info", deficon_path, type );

	stccpy( buff, deficon_name, buff_size );

	p = buff;

	return p;
}

static ULONG mimeadjustgroup_refresh_icon(APTR obj UNUSED, struct Data * data, STRPTR mimetype)
{
	ULONG rc = FALSE;
	ULONG icon_found = FALSE;

	/* this block should be done in a thread ! */
	if ((data->deficon = (APTR)methodstack_push_sync(app, 3, MM_Application_CreateIcon, TRUE, MV_ViewID_Info)))
	{
		if(mimetype && strchr(mimetype, '/')) /* if it's a full or generic mimetype, we can find icon */
		{
			TEXT iconpath[PATH_SIZE];

			if(get_iconpath_from_mimetype(mimetype, iconpath, sizeof(iconpath)))
			{
				if (exists(iconpath) && icon_read(iconpath, data->deficon,
					ICONTAG_Ancillary, TRUE,
					ICONTAG_Deficon, TRUE,
					ICONTAG_Infowin, TRUE,
				TAG_DONE))
				{
					icon_found = TRUE;
				}
				else
				{
					/* fallback image */
					ULONG *srcarray = NULL;
					APTR tbm;
					ULONG xs = 0;
					ULONG ys = 0;

					srcarray = def_tool;
					xs = DEF_TOOL_WIDTH;
					ys = DEF_TOOL_HEIGHT;

					if ( (tbm = gfx_bitmap_create(xs, ys, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)) )
					{
						gfx_blit(srcarray, tbm,
							BLITTAG_SrcType, BLITVAL_SrcType_Array,
							BLITTAG_SrcFormat, BLITVAL_SrcFormat_ARGB,
						TAG_DONE);

						set(data->deficon, MA_Icon_PathInfo, iconpath);
						set(data->deficon, MA_Icon_Infowin, TRUE);
						set(data->deficon, MA_Icon_IsDefault, TRUE);
						set(data->deficon, MA_Icon_FileType, MV_Icon_FileType_File);

						DoMethod(data->deficon, MM_Icon_AddBitMap, tbm, MV_Icon_BitMap_PNGicon, MV_Icon_BitMap_Normal);
						set(data->deficon, MA_Icon_ImageType, MV_Icon_ImageType_PNGicon);
						set(data->deficon, MA_Icon_Refine, FALSE);
						DoMethod(data->deficon, MM_Icon_End);

						icon_found = TRUE;
					}
				}
			}
		}
	}

	if(icon_found)
	{
		data->icongroup = NewObject(getinfoicongroupclass(), NULL,
								MA_Infoicongroup_Child, data->deficon,
								MA_Infoicongroup_Editable, TRUE,
								TAG_DONE);

		if(data->icongroup)
		{
			DoMethod(data->iconarea, OM_ADDMEMBER, data->icongroup);
			rc = TRUE;
		}
	}

	if(!rc)
	{
		if(data->deficon)
		{
			methodstack_push_sync(app, 2, MM_Application_DisposeObject, data->deficon);
			data->deficon = NULL;
		}
	}

	return (rc);
}

static ULONG mimeadjustgroup_refresh_mimetype(APTR obj, struct Data * data, STRPTR mimetype)
{
	ULONG rc = FALSE;

	/* set the mimetype cycle+string */
	if(mimetype)
	{
		char * tmp;
		ULONG i = 0;

		while (cycleopts[i])
		{
			if (!strnicmp(cycleopts[i], mimetype, strlen(cycleopts[i])))
			{
				set(data->cyc_mime, MUIA_Cycle_Active, i);
				break;
			}
			i++;
		}

		tmp = strchr(mimetype, '/');

		if(tmp && *(tmp+1))
		{
			set(data->str_mime, MUIA_String_Contents, tmp+1);
		}

		if(data->generic ||
		  (data->mime_node_temp->flags & MIMETYPEFLAG_FOREIGNRECOG) ||
		  (data->edit && !data->mime_node_temp->descriptor))
		{
			/* XXX: show it in a better way, ghost makes it unreadable */
			set(data->cyc_mime, MUIA_Disabled, TRUE);
			set(data->str_mime, MUIA_Disabled, TRUE);
		}
	}

	/* description */
	if(data->mime_node_temp->description)
	{
		set(data->str_descr, MUIA_String_Contents, data->mime_node_temp->description);
	}

	/* priority */
	set(data->str_pri, MUIA_String_Integer, data->mime_node_temp->priority);

	/* icon */
	rc = mimeadjustgroup_refresh_icon(obj, data, mimetype);

	/* actions list */
	mimeadjustgroup_refresh_actions(obj, data);

	return (rc);
}

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_Mimeadjustwin_MimeNode:
			*msg->opg_Storage = (ULONG) (data->accept ? data->mime_node_temp : NULL);
			return(TRUE);

		case MA_Mimeadjustwin_Generic:
			*msg->opg_Storage = data->generic;
			return(TRUE);
	}
	return (DOSUPER);
}

DEFNEW
{
	struct Data *data;
	APTR bt_cancel, bt_ok, bt_add, bt_remove, bt_edit, bt_up, bt_down;
	APTR cyc_mime;
	APTR str_mime, str_descr, str_pri;
	APTR grp_action;
	APTR iconarea;
	APTR lst_actions;
	STRPTR mimetype = NULL;
	APTR mime_node = NULL;

	obj = DoSuperNew(cl, obj,

		Child, HGroup,
			Child, HSpace(0),
			Child, iconarea = HGroup,
								MUIA_ShortHelp, GSI(MSG_MIMEADJUSTGROUPCLASS_ICON_HELP),
							  End,
			Child, HSpace(0),
		End,

		Child, ColGroup(2),
			Child, NSLabel2(MSG_MIMEADJUSTGROUPCLASS_MIMETYPE),
			Child, HGroup,
				Child, cyc_mime = CycleObject,
					MUIA_Cycle_Entries, cycleopts,
					MUIA_Font, MUIV_Font_Button,
					MUIA_Weight, 0,
					MUIA_ShortHelp  , GSI(MSG_MIMEADJUSTGROUPCLASS_MIMETYPE_HELP),
					MUIA_ControlChar, MUIGetUnderScore( MSG_MIMEADJUSTGROUPCLASS_MIMETYPE ),
					MUIA_CycleChain, 1,
				End,
				Child, TextObject,
					MUIA_Text_Contents, "/",
					MUIA_ShortHelp    , GSI(MSG_MIMEADJUSTGROUPCLASS_MIMESUBTYPE_HELP),
					MUIA_Weight, 0,
				End,
				Child, str_mime = MUICreateString( MSG_MIMEADJUSTGROUPCLASS_MIMETYPE, 64, NULL), /* XXX: should be enough */
				Child, NSLabel2(MSG_MIMEADJUSTGROUPCLASS_PRIORITY),
				Child, str_pri = MUICreateInteger( MSG_MIMEADJUSTGROUPCLASS_PRIORITY, 5, NULL, -1000, 1000, 1 ),
			End,
			
			Child, NSLabel2(MSG_MIMEADJUSTGROUPCLASS_DESCRIPTION),
			Child, str_descr = MUICreateString( MSG_MIMEADJUSTGROUPCLASS_DESCRIPTION, 256, NULL), /* XXX: should be enough */
		End,
	
		Child, grp_action = VGroup,
			GroupFrameT(GSI(MSG_MIMEADJUSTGROUPCLASS_ACTIONS_GROUP)),

			Child, HGroup,
				Child, lst_actions = NewObject(getmimeadjustlistclass(), NULL, MUIA_CycleChain, 1, MUIA_ShortHelp, GSI(MSG_MIMEADJUSTGROUPCLASS_ACTIONS_HELP), TAG_DONE),
				Child, VGroup,
					MUIA_Weight, 0,
					Child, bt_add    = MUICreateButton( MSG_MIMEADJUSTGROUPCLASS_ADD   , NULL ),
					Child, bt_edit   = MUICreateButton( MSG_MIMEADJUSTGROUPCLASS_EDIT  , NULL ),
					Child, bt_remove = MUICreateButton( MSG_MIMEADJUSTGROUPCLASS_REMOVE, NULL ),
					Child, HGroup,
							Child, bt_up   = MUICreateImageButton(MSG_MIMEADJUSTGROUPCLASS_UP_HELP-1, MUII_TapeUp, NULL),
							Child, bt_down = MUICreateImageButton(MSG_MIMEADJUSTGROUPCLASS_DOWN_HELP-1, MUII_TapeDown, NULL),
					End,
					Child, VSpace(0),
				End,

			End,
			
		End,

		Child, HGroup,
			Child, bt_ok     = MUICreateButton( MSG_MIMEADJUSTGROUPCLASS_OK    , NULL ), /* XXX: Save, not OK */
			Child, RectangleObject, End,
			Child, bt_cancel = MUICreateButton( MSG_MIMEADJUSTGROUPCLASS_CANCEL, NULL ),
		End,

	End;

	if (!obj)
	{
		return (NULL);
	}

	data = INST_DATA(cl, obj);

	data->mime_node_temp = NULL;
	data->edit      = FALSE;
	data->generic   = FALSE;
	data->accept    = FALSE;
	data->jobs      = 0;
	data->closing   = FALSE;
	NEWLIST(&data->generic_action_list);
	NEWLIST(&data->windows_list);

	data->cyc_mime    = cyc_mime;
	data->str_mime    = str_mime;
	data->str_pri     = str_pri;
	data->str_descr   = str_descr;
	data->grp_action  = grp_action;
	data->lst_actions = lst_actions;

	data->bt_add    = bt_add;
	data->bt_edit   = bt_edit;
	data->bt_remove = bt_remove;
	data->bt_up     = bt_up;
	data->bt_down   = bt_down;

	data->iconarea = iconarea;
	data->iconchanged  = FALSE;

	FORTAG(INITTAGS)
	{
		case MA_Mimeadjustwin_MimeNode:
			{
				mime_node = (struct internal_mimetype_node *) tag->ti_Data;
			}
			break;

		case MA_Mimeadjustwin_MimeType:
			{
				mimetype = (STRPTR) tag->ti_Data;
			}
			break;

		case MA_Mimeadjustwin_Edit:
			data->edit = tag->ti_Data;
			break;

		case MA_Mimeadjustwin_Generic:
			data->generic = tag->ti_Data;
			break;
	}
	NEXTTAG

	if(mime_node)
	{
		/* set it in case it was not passed	*/
		mimetype = ((struct internal_mimetype_node *) mime_node)->mimetype;
	}
	else if(mimetype) /* last chance to find mime node */
	{
		if(data->generic)
		{
			mime_node = mimetype_find_generic_by_mimetype(mimetype);
		}
		else
		{
			mime_node = mimetype_find_by_mimetype(mimetype);
		}
	}

	/* copy mime_node for local modifications or create it if needed */
	if(!mime_node)
	{
		data->mime_node_temp = imn_create(NULL, NULL);
	}
	else
	{
		data->mime_node_temp = imn_duplicate(mime_node);
	}

	if(!data->mime_node_temp)
	{
		CoerceMethod(cl, obj, OM_DISPOSE);
		return (NULL);
	}

	/* fill with mimetype information */
	if(!mimeadjustgroup_refresh_mimetype(obj, data, mimetype))
	{
		CoerceMethod(cl, obj, OM_DISPOSE);
		return (NULL);
	}

	DoMethod(obj, MUIM_MultiSet, MUIA_CycleChain, 1,
		cyc_mime, str_mime, str_descr, bt_ok, bt_cancel, NULL
	);

	DoMethod(obj, MM_Mimeadjustgroup_CheckMime);

	DoMethod(obj, MM_Mimeadjustgroup_Selection_Change);

	/* notifications */
	DoMethod(data->str_mime, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
		obj, 1, MM_Mimeadjustgroup_CheckMime
	);

	DoMethod(data->lst_actions, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
			 obj, 1, MM_Mimeadjustgroup_Selection_Change);

	DoMethod(data->bt_add, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Mimeadjustgroup_Action_Add, FALSE
	);

	DoMethod(data->bt_edit, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Mimeadjustgroup_Action_Add, TRUE
	);

	DoMethod(data->lst_actions, MUIM_Notify, MUIA_List_DoubleClick, TRUE,
			 obj, 2, MM_Mimeadjustgroup_Action_Add, TRUE);

	DoMethod(data->bt_remove, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1, MM_Mimeadjustgroup_Action_Remove
	);

	DoMethod(data->bt_up, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Mimeadjustgroup_Action_Move, -1L
	);

	DoMethod(data->bt_down, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Mimeadjustgroup_Action_Move, 1L
	);

	DoMethod(bt_ok, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Mimeadjustgroup_Accept, TRUE
	);

	DoMethod(bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Mimeadjustgroup_Accept, FALSE
	);

	DoMethod(data->icongroup, MUIM_Notify, MA_Infoicongroup_Changed, TRUE,
		obj, 1, MM_Mimeadjustgroup_IconChanged
	);

	return ((ULONG)obj);
}

DEFDISP
{
	GETDATA;
	struct window_entry *n, *nextn;

	if(data->mime_node_temp)
	{
		imn_delete(data->mime_node_temp);
		data->mime_node_temp = NULL;
	}

	free_generic_actions(data);

	ITERATELISTSAFE(n, nextn, &data->windows_list)
	{
		if(n->obj)
		{
			DoMethod(n->obj, MUIM_KillNotify, MUIA_Window_Open);
			DoMethod(n->obj, MM_ActioneditorWin_Close);
		}
		FreeVecTaskPooled(n);
	}
	return (DOSUPER);
}


DEFTMETHOD(Mimeadjustgroup_CheckMime)
{
	GETDATA;
	STRPTR p;

	p = (STRPTR)getv(data->str_mime, MUIA_String_Contents);

	if (p && *p)
	{
		while (isalnum(*p) || *p == '-' || *p == '+' || *p == '.' || *p == '_' || *p == '*')
		{
			p++;
		}
		if (*p == '\0')
		{
			/* valid mimetype */
			if (getv(data->grp_action, MUIA_Disabled))
			{
				SetAttrs(data->grp_action,
					MUIA_Disabled, FALSE,
					MUIA_ShortHelp, GSI(MSG_MIMEADJUSTGROUPCLASS_ACTIONSTOPERFORM_HELP),
					TAG_DONE
				);
			}
			return (0);
		}
	}
	if (!getv(data->grp_action, MUIA_Disabled))
	{
		SetAttrs(data->grp_action,
			MUIA_Disabled, TRUE,
			MUIA_ShortHelp, GSI(MSG_MIMEADJUSTGROUPCLASS_DISABLED_HELP),
			TAG_DONE
		);
	}
	return (0);
}

DEFTMETHOD(Mimeadjustgroup_Selection_Change)
{
	GETDATA;

	struct ActionEntry * ae;
	DoMethod(data->lst_actions, MUIM_List_GetEntry,  MUIV_List_GetEntry_Active, (ULONG *)&ae);

	if(ae)
	{
		if(ae->inherited)
		{
			set(data->bt_edit, MUIA_Disabled, TRUE);
			set(data->bt_remove, MUIA_Disabled, TRUE);
			set(data->bt_up, MUIA_Disabled, TRUE);
			set(data->bt_down, MUIA_Disabled, TRUE);
		}
		else
		{
			set(data->bt_edit, MUIA_Disabled, FALSE);
			set(data->bt_remove, MUIA_Disabled, FALSE);
			set(data->bt_up, MUIA_Disabled, FALSE);
			set(data->bt_down, MUIA_Disabled, FALSE);
		}
	}
	else
	{
		set(data->bt_edit, MUIA_Disabled, TRUE);
		set(data->bt_remove, MUIA_Disabled, TRUE);
		set(data->bt_up, MUIA_Disabled, TRUE);
		set(data->bt_down, MUIA_Disabled, TRUE);
	}

	return (0);
}

DEFSMETHOD(Mimeadjustgroup_Action_Add)
{
	GETDATA;
	APTR an = NULL;
	APTR o;
	struct window_entry * n;

	if(msg->edit)
	{
		struct ActionEntry * ae;
		DoMethod(data->lst_actions, MUIM_List_GetEntry,  MUIV_List_GetEntry_Active, (ULONG *)&ae);

		if(ae->inherited)
		{
			return (0);
		}

		if(ae)
		{
			an = ae->action_node;
		}
	}

	n = AllocVecTaskPooled(sizeof(*n));

	if(n)
	{
		n->obj = NULL;

		o = NewObject(getactioneditorwinclass(), NULL,
					MA_ActioneditorWin_ActionNode, an,
					MA_Actioneditor_MimeAction, TRUE,
					TAG_DONE
			);

		if(o)
		{
			DoMethod(o, MUIM_Notify, MUIA_Window_Open, FALSE, obj, 3, MM_Mimeadjustgroup_Action_Ack, o, msg->edit);
			DoMethod(o, MM_ActioneditorWin_Open);

			n->obj = o;

			//set(_win(obj), MUIA_Window_Sleep, TRUE); /* block this window */
		}

		ADDTAIL(&data->windows_list, n);
	}

	return (0);
}

DEFSMETHOD(Mimeadjustgroup_Action_Ack)
{
	GETDATA;
	APTR an;
	struct window_entry *n;

	//set(_win(obj), MUIA_Window_Sleep, FALSE);

	an = (APTR) getv(msg->actionobj, MA_ActioneditorWin_ActionNode);

	if(an)
	{
		if(msg->edit)
		{
			DoMethod(data->lst_actions, MUIM_List_Redraw, MUIV_List_Redraw_Active);
		}
		else
		{
			/* we created an action, now add it to the action list */
			struct MinList * action_list;
			APTR aearray[2];

			aearray[0] = an;
			aearray[1] = FALSE;

			if(!data->generic)
			{
				struct ActionEntry * ae;

				DoMethod(data->lst_actions, MUIM_List_GetEntry, 0, (ULONG *)&ae);

				/* if an inherited action is found (will be necessary the first), clear action list */
				if(ae && ae->inherited)
				{
	                DoMethod(data->lst_actions, MUIM_List_Clear);

					NEWLIST(&data->mime_node_temp->internal_action_list);
					data->mime_node_temp->action_list = &data->mime_node_temp->internal_action_list;
					data->mime_node_temp->flags &= ~MIMETYPEFLAG_FOREIGNACTIONS;
				}				 
			}

			action_list = data->mime_node_temp->action_list;
			ADDTAIL(action_list, an);

			DoMethod(data->lst_actions, MUIM_List_InsertSingle, aearray, MUIV_List_Insert_Bottom);
		}
	}

	/* remove opened window */
	ITERATELIST(n, &data->windows_list)
	{
		if(n->obj == msg->actionobj)
		{
			REMOVE(n);
			FreeVecTaskPooled(n);
			break;
		}
	}

	DoMethod(msg->actionobj, MM_ActioneditorWin_Close);

	return (0);
}

DEFTMETHOD(Mimeadjustgroup_Action_Remove)
{
	GETDATA;
	struct ActionEntry * ae;
	DoMethod(data->lst_actions, MUIM_List_GetEntry,  MUIV_List_GetEntry_Active, (ULONG *)&ae);

	if(ae)
	{
		DoMethod(data->lst_actions, MUIM_List_Remove, MUIV_List_Remove_Active);
		REMOVE(ae->action_node);
		actionnode_delete(ae->action_node);

		/* if there are no own actions, add foreign actions list again, if it exists */
		if(!data->generic && ISLISTEMPTY(data->mime_node_temp->action_list))
		{
			TEXT mimetype[256];
			STRPTR ptr;

			ptr = get_mimetype(obj, data, mimetype, sizeof(mimetype), TRUE);
		
			if(ptr)
			{
				data->mime_node_temp->flags |= MIMETYPEFLAG_FOREIGNACTIONS;
				mimeadjustgroup_refresh_actions(obj, data);
			}
		}
	}

	return (0);
}

DEFSMETHOD(Mimeadjustgroup_Accept)
{
	GETDATA;

	if(msg->accept)
	{
		TEXT mimetype[256];
		STRPTR ptr;
		APTR iconobj;

		data->accept = TRUE;


		/* XXX: recog to be completed once recog editor is done */

		if(data->mime_node_temp->flags & MIMETYPEFLAG_FOREIGNRECOG)
		{
			data->mime_node_temp->rctx = NULL;
		}

		if(data->mime_node_temp->rctx && data->mime_node_temp->descriptor == NULL)
		{
			/* HACK: we'll assume a descriptor-less mimetype comes from recog.db for now,
			 *	     so we delete its recog context	(which was duplicated before) */

			recog_delete(data->mime_node_temp->rctx);
			data->mime_node_temp->rctx = NULL;
			data->mime_node_temp->flags |= MIMETYPEFLAG_FOREIGNRECOG;
		}

		/* mimetype */
		ptr = get_mimetype(obj, data, mimetype, sizeof(mimetype), FALSE);

		if(ptr)
		{
			if(data->mime_node_temp->mimetype)
			{
				name_delete(data->mime_node_temp->mimetype);
			}
			data->mime_node_temp->mimetype = name_build(ptr);
		}

		/* description */
		if((ptr = (STRPTR) getv(data->str_descr, MUIA_String_Contents)))
		{
			if(data->mime_node_temp->description)
			{
				name_delete(data->mime_node_temp->description);
			}
			data->mime_node_temp->description = name_build(ptr);
		}


		if(data->mime_node_temp->descriptor == NULL)
		{
			TEXT descriptor[PATH_SIZE];

			if(data->generic)
			{
				stccpy(descriptor, PREFS_PATH "filetypes/", sizeof(descriptor));
				strncat(descriptor, cycleopts[getv(data->cyc_mime, MUIA_Cycle_Active)], sizeof(descriptor));
				strncat(descriptor, "/", sizeof(descriptor));
				strncat(descriptor, "default", sizeof(descriptor));
			}
			else
			{
				stccpy(descriptor, PREFS_PATH "filetypes/", sizeof(descriptor));
				strncat(descriptor, data->mime_node_temp->mimetype, sizeof(descriptor));
			}

			data->mime_node_temp->descriptor = name_build(descriptor);
		}

		/* priority */
		data->mime_node_temp->priority = getv(data->str_pri, MUIA_String_Integer);

		/* Save descriptor XXX: should be done in a thread */

		/* if there are only external actions, we can delete descriptor */
		if((data->mime_node_temp->flags & MIMETYPEFLAG_FOREIGNACTIONS) && ISLISTEMPTY(&data->mime_node_temp->internal_action_list))
		{
			DeleteFile(data->mime_node_temp->descriptor);
		}
		else
		{
			TEXT dir[PATH_SIZE];
			STRPTR ptr = data->mime_node_temp->descriptor;

			if(ptr)
			{
				stccpy(dir, ptr, FilePart(ptr) - ptr);

				if(!exists(dir))
				{
					makedir(dir);
				}
			}

			if(!descriptor_save(data->mime_node_temp))
			{
				smartreq_request(NULL, NULL, GSI( MSG_MIMEADJUSTGROUPCLASS_TITLE_REQ ), 0, 0, GSI( MSG_MIMEADJUSTGROUPCLASS_OK_REQ ), MV_Notification_Warning,
								 GSI( MSG_MIMEADJUSTGROUPCLASS_COULDNTSAVEFILETYPEDESC ), data->mime_node_temp->descriptor
				);
			}
		}

		/* invalidate mimetypes and reload database (ugly) -> in a thread */
		mimetype_invalidate( NULL, NULL );
		mimetype_load_database( NULL );

		/* Save icon */
		iconobj = (APTR)getv(data->icongroup, MA_Infoicongroup_Child);

		if(data->iconchanged && iconobj && getv(iconobj, MA_Icon_PathInfo))
		{
			DoMethod(obj, MM_Mimeadjustgroup_SaveIcon);

			/* Hack */
			cache_invalidate( 0, CACHETAG_THUMBNAIL ); /* for cached images */
			deficonpool_flush(); /* for deficon cached images */
		}
	}
	else
	{
		data->accept = FALSE;
	}

	data->closing = TRUE;
	threads_abort(obj, NULL);

	return (0);
}

DEFTMETHOD(Mimeadjustgroup_SaveIcon)
{
	GETDATA;
	APTR iconobj;
	ULONG type = MV_Icon_Type_Project;
	TEXT dir[PATH_SIZE];
	STRPTR ptr;

	iconobj = (APTR)getv(data->icongroup, MA_Infoicongroup_Child);

	if(iconobj)
	{
		set(iconobj, MA_Icon_Type, type); 
		set(iconobj, MA_Icon_DefaultTool, (STRPTR) "");

		ptr = (STRPTR) getv(iconobj, MA_Icon_PathInfo);
		if(ptr)
		{
			stccpy(dir, ptr, FilePart(ptr) - ptr);
			if(!exists(dir))
			{
				makedir(dir);
			}
		}

		if (do_action(iconobj, TA_Icon_Write,
			TT_Object, obj,
		TAG_DONE))
		{
			data->jobs |= TJ_ICONWRITE;
		}
	}

	return (0);
}

DEFTMETHOD(Mimeadjustgroup_IconChanged)
{
	GETDATA;

	data->iconchanged = TRUE;

	return (0);
}

DEFSMETHOD(Thread_Finished)
{
	GETDATA;

	switch (msg->action)
	{
		case TA_Icon_Write:
			data->jobs &= ~TJ_ICONWRITE;

			if(!msg->status)
			{
				APTR iconobj = (APTR) getv(data->icongroup, MA_Infoicongroup_Child);

				smartreq_request(NULL, NULL, GSI( MSG_MIMEADJUSTGROUPCLASS_TITLE_REQ ),
									0, 0   , GSI( MSG_MIMEADJUSTGROUPCLASS_OK_REQ ), MV_Notification_Warning,
										GSI( MSG_MIMEADJUSTGROUPCLASS_COULDNTSAVEDEFAULTICON ), getv(iconobj, MA_Icon_PathInfo)
				);
			}

			break;

		case TA_Icon_Read:
			data->jobs &= ~TJ_ICONREAD;
			break;
	}

	if (data->closing && !data->jobs)
	{
		set(_win(obj), MUIA_Window_Open, FALSE);
	}
	return (0);
}

static void swap_nodes(struct MinNode *cn)
{
	struct MinNode *pn = cn->mln_Pred;
	struct MinNode *nn = cn->mln_Succ;

	if (pn) pn->mln_Succ = nn;
	nn->mln_Pred = pn;
	cn->mln_Succ = nn->mln_Succ;
	nn->mln_Succ = cn;
	cn->mln_Pred = nn;
}

DEFSMETHOD(Mimeadjustgroup_Action_Move)
{
	struct ActionEntry * ae;
	struct MinNode *cn;
	GETDATA;

	DoMethod(data->lst_actions, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ULONG*) &ae);

	if(ae)
	{
		cn = ae->action_node;

		if (msg->offset == -1)
		{
			if(getv(data->lst_actions, MUIA_List_Active) > 0)
			{
				swap_nodes(cn->mln_Pred);
				DoMethod(data->lst_actions, MUIM_List_Exchange,
					MUIV_List_Exchange_Active, MUIV_List_Exchange_Previous
				);
				set(data->lst_actions, MUIA_List_Active, MUIV_List_Active_Up);
			}
		}
		else if(getv(data->lst_actions, MUIA_List_Active) < (getv(data->lst_actions, MUIA_List_Entries) - 1))
		{
			swap_nodes(cn);
			DoMethod(data->lst_actions, MUIM_List_Exchange,
				MUIV_List_Exchange_Active, MUIV_List_Exchange_Next
			);
			set(data->lst_actions, MUIA_List_Active, MUIV_List_Active_Down);
		}
	}

	return 0;
}

DEFMMETHOD(DragQuery)
{
	GETDATA;

	ULONG dummy = 0;

	if (get(msg->obj, MA_Actioneditor_ActionNode, &dummy) && msg->obj != data->lst_actions)
	{
		return (MUIV_DragQuery_Accept);
	}
	return (MUIV_DragQuery_Refuse);
}

DEFMMETHOD(DragDrop)
{
	ULONG dummy = 0;

	if (get(msg->obj, MA_Actioneditor_ActionNode , &dummy))
	{
		LONG id = MUIV_List_NextSelected_Start;
		struct ActionEntry *ae;
		ULONG i;

		for (i = 0; ; i++)
		{
			DoMethod(msg->obj, MUIM_List_NextSelected, &id);

			if (id == MUIV_List_NextSelected_End) break;

			DoMethod(msg->obj, MUIM_List_GetEntry, id, &ae);

			if (ae)
			{
				GETDATA;

				struct MinList * action_list;
				APTR aearray[2];
				APTR an = NULL;

				if(!data->generic)
				{
					struct ActionEntry * ae2;

					DoMethod(data->lst_actions, MUIM_List_GetEntry, 0, (ULONG *)&ae2);

					/* if an inherited action is found (will be necessary the first), clear action list */
					if(ae2 && ae2->inherited)
					{
		                DoMethod(data->lst_actions, MUIM_List_Clear);

						NEWLIST(&data->mime_node_temp->internal_action_list);
						data->mime_node_temp->action_list = &data->mime_node_temp->internal_action_list;
						data->mime_node_temp->flags &= ~MIMETYPEFLAG_FOREIGNACTIONS;
					}
				}

				an = actionnode_duplicate(ae->action_node);

				if(an)
				{
					aearray[0] = an;
					aearray[1] = FALSE;

					action_list = data->mime_node_temp->action_list;
					ADDTAIL(action_list, an);

					DoMethod(data->lst_actions, MUIM_List_InsertSingle, aearray, MUIV_List_Insert_Bottom);
				}
			}
		}
	}

	return (0);
}

BEGINMTABLE
DECNEW
DECDISP
DECGET
DECTMETHOD(Mimeadjustgroup_CheckMime)
DECTMETHOD(Mimeadjustgroup_Selection_Change)
DECSMETHOD(Mimeadjustgroup_Action_Add)
DECTMETHOD(Mimeadjustgroup_Action_Remove)
DECSMETHOD(Mimeadjustgroup_Action_Move)
DECSMETHOD(Mimeadjustgroup_Action_Ack)
DECSMETHOD(Mimeadjustgroup_Accept)
DECTMETHOD(Mimeadjustgroup_SaveIcon)
DECTMETHOD(Mimeadjustgroup_IconChanged)
DECSMETHOD(Thread_Finished)
DECMMETHOD(DragQuery)
DECMMETHOD(DragDrop)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, mimeadjustgroupclass)
