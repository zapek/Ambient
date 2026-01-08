/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
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
 * $Id: mimegroupclass.c,v 1.10 2017/08/21 06:17:44 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <mui/Listtree_mcc.h>
#include <proto/dos.h>

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "mimetype.h"
#include "mimegroupclass.h"

struct window_entry
{
	struct MinNode n;
	APTR obj;
};

struct Data {
	APTR lv_mime;
	APTR lt_mime;
	APTR bt_add;
	APTR bt_copy;
	APTR bt_edit;
	APTR bt_remove;
	APTR cb_search;
	APTR gp_search;


	struct MinList windows_list;
};

DEFNEW
{
	struct Data *data;
	APTR lv_mime, lt_mime;
	APTR bt_add, bt_copy, bt_edit, bt_remove, gp_search, cb_search;

	obj = DoSuperNew(cl, obj,
		Child, lv_mime = ListviewObject,
			MUIA_CycleChain, TRUE,
			MUIA_Listview_List, lt_mime = NewObject(getmimelisttreeclass(), NULL, TAG_DONE),
		End,
		Child, HGroup,
			Child, bt_add    = MUICreateButton( MSG_MIMEGROUPCLASS_ADD   , NULL ),
			Child, bt_copy   = MUICreateButton( MSG_MIMEGROUPCLASS_COPY  , NULL ),
			Child, bt_edit   = MUICreateButton( MSG_MIMEGROUPCLASS_EDIT  , NULL ),
			Child, bt_remove = MUICreateButton( MSG_MIMEGROUPCLASS_REMOVE, NULL ),
			Child, cb_search = MUICreateCheckbox( MSG_MIMEGROUPCLASS_SEARCH, FALSE, NULL),
			Child, MUICreateLabel( MSG_MIMEGROUPCLASS_SEARCH, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
		End,
		Child, gp_search = NewObject(getsearchbarclass(), NULL,
										MA_Searchbar_Flags, MV_Searchbar_Flags_ShowNext,
										MUIA_ShowMe, FALSE,
										TAG_DONE),
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return ((ULONG)NULL);
	}

	data = INST_DATA(cl, obj);

	data->lv_mime   = lv_mime;
	data->lt_mime   = lt_mime;
	data->bt_add    = bt_add;
	data->bt_copy   = bt_copy;
	data->bt_edit   = bt_edit;
	data->bt_remove = bt_remove;
	data->cb_search = cb_search;
	data->gp_search = gp_search;
	
	NEWLIST(&data->windows_list);

	DoMethod(obj, MM_Mimegroup_ChangeButtons, MV_Mimelisttree_ActiveType_None);
	
	DoMethod(data->lv_mime, MUIM_Notify, MA_Mimelisttree_ActiveType, MUIV_EveryTime,
		obj, 2, MM_Mimegroup_ChangeButtons, MUIV_TriggerValue
	);

	DoMethod(data->bt_add, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Mimegroup_Add, FALSE
	);

	DoMethod(data->bt_edit, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Mimegroup_Add, TRUE
	);

	DoMethod(data->bt_remove, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1, MM_Mimegroup_Remove
	);

	DoMethod(data->lv_mime, MUIM_Notify, MUIA_Listtree_DoubleClick, MUIV_EveryTime,
		obj, 2, MM_Mimegroup_Add, TRUE
	);

	DoMethod(data->cb_search, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
		data->gp_search, 3, MUIM_Set, MUIA_ShowMe, MUIV_TriggerValue
	);

	DoMethod(data->gp_search, MUIM_Notify, MUIA_ShowMe, FALSE,
		data->cb_search, 3, MUIM_Set, MUIA_Selected, FALSE
	);

	set(data->gp_search, MA_Searchbar_Target, data->lt_mime);

	/* let's hide not yet implemented functions for now */
	set(data->bt_add, MUIA_ShowMe, FALSE);
	/*set(data->bt_remove, MUIA_ShowMe, FALSE);*/
	set(data->bt_copy, MUIA_ShowMe, FALSE);

	return ((ULONG)obj);
}

DEFDISP
{
	GETDATA;
	struct window_entry *n, *nextn;

	ITERATELISTSAFE(n, nextn, &data->windows_list)
	{
		if(n->obj)
		{
			DoMethod(n->obj, MUIM_KillNotify, MUIA_Window_Open);
			DoMethod(n->obj, MM_Mimeadjustwin_Close);
		}
		FreeVecTaskPooled(n);
	}
	return (DOSUPER);
}

DEFSMETHOD(Mimegroup_ChangeButtons)
{
	GETDATA;

	switch (msg->val)
	{
		case MV_Mimelisttree_ActiveType_None:
			set(data->bt_add, MUIA_Disabled, TRUE);
			set(data->bt_copy, MUIA_Disabled, TRUE);
			set(data->bt_edit, MUIA_Disabled, TRUE);
			set(data->bt_remove, MUIA_Disabled, TRUE);
			break;

		case MV_Mimelisttree_ActiveType_Media:
			set(data->bt_add, MUIA_Disabled, FALSE);
			set(data->bt_copy, MUIA_Disabled, TRUE);
			set(data->bt_edit, MUIA_Disabled, FALSE);
			set(data->bt_remove, MUIA_Disabled, TRUE);
			break;

		case MV_Mimelisttree_ActiveType_MediaOverloaded:
			set(data->bt_add, MUIA_Disabled, FALSE);
			set(data->bt_copy, MUIA_Disabled, TRUE);
			set(data->bt_edit, MUIA_Disabled, FALSE);
			set(data->bt_remove, MUIA_Disabled, FALSE);
			break;

		case MV_Mimelisttree_ActiveType_Mime:
			set(data->bt_add, MUIA_Disabled, FALSE);
			set(data->bt_copy, MUIA_Disabled, FALSE);
			set(data->bt_edit, MUIA_Disabled, FALSE);
			set(data->bt_remove, MUIA_Disabled, TRUE);
			break;

		case MV_Mimelisttree_ActiveType_MimeOverloaded:
			set(data->bt_add, MUIA_Disabled, FALSE);
			set(data->bt_copy, MUIA_Disabled, FALSE);
			set(data->bt_edit, MUIA_Disabled, FALSE);
			set(data->bt_remove, MUIA_Disabled, FALSE);
			break;

		#ifdef DEBUG
		default:
			PDB(("argl\n"));
			break;
		#endif
	}
	return (0);
}


DEFSMETHOD(Mimegroup_Edit)
{
	GETDATA;
	APTR maobj;
	struct window_entry * n;

	n = AllocVecTaskPooled(sizeof(*n));

	if(n)
	{
		n->obj = NULL;

		if ((maobj = NewObject(getmimeadjustwinclass(), NULL,
						msg->mimetype ? MA_Mimeadjustwin_MimeType : TAG_IGNORE, msg->mimetype,
						msg->mimenode ? MA_Mimeadjustwin_MimeNode : TAG_IGNORE, msg->mimenode,
						MA_Mimeadjustwin_Edit, msg->edit,
						MA_Mimeadjustwin_Generic, msg->generic,
						TAG_DONE)))
		{
			DoMethod(app, OM_ADDMEMBER, maobj);
			set(maobj, MUIA_Window_Open, TRUE);

			n->obj = maobj;

			DoMethod(maobj, MUIM_Notify, MUIA_Window_Open, FALSE, obj, 3, MM_Mimegroup_Mime_Ack, maobj, msg->edit);
			//set(_win(obj), MUIA_Window_Sleep, TRUE); /* block this window */
		}

		ADDTAIL(&data->windows_list, n);
	}

	return (0);
}

DEFSMETHOD(Mimegroup_Add)
{
	GETDATA;
	TEXT mimetype[256];
	STRPTR ptr = NULL;
	struct MUIS_Listtree_TreeNode *active = NULL, *parent = NULL;
	ULONG generic = FALSE;

	active = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_mime, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);

	if(active)
	{
		if(((struct treedata *)active->tn_User)->flags & NODEFLAG_MEDIATYPE) /* it's a mediatype family */
		{
			snprintf(mimetype, sizeof(mimetype), "%s/*", active->tn_Name);
			ptr = mimetype;
			generic = TRUE;
		}
		else /* it's the mimetype */
		{
			parent = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_mime, MUIM_Listtree_GetEntry, active, MUIV_Listtree_GetEntry_Position_Parent, 0);

			if(parent)
			{
				snprintf(mimetype, sizeof(mimetype), "%s/%s", parent->tn_Name, active->tn_Name);
				ptr = mimetype;
			}
		}
	}

	if (active == MUIV_Listtree_Active_Off)
	{
		DoMethod(obj, MM_Mimegroup_Edit, NULL, NULL, FALSE, generic);
	}
	else
	{
		if(!msg->edit)
		{
			ptr = NULL;

			if(active)
			{
				if(((struct treedata *)active->tn_User)->flags & NODEFLAG_MEDIATYPE)
				{
					ptr = active->tn_Name;
				}
				else if(parent)
				{
					ptr = parent->tn_Name;
				}
			}

			DoMethod(obj, MM_Mimegroup_Edit, ptr, NULL, FALSE, generic);
		}
		else
		{
			struct internal_mimetype_node * imn = NULL;

			if(ptr)
			{
				if(generic)
				{
					imn	= mimetype_find_generic_by_mimetype(ptr);		 
				}
				else
				{
					imn	= mimetype_find_by_mimetype(ptr);
				}
			}

			SDB(("passing mimetype = <%s> imn = %p generic = %ld descriptor = <%s>\n", ptr, imn, generic, (imn && imn->descriptor) ? imn->descriptor : (STRPTR) "none"));

			if(imn)
			{
				SDB(("Foreign actions : %d\nForeign recognition : %d\n", imn->flags & MIMETYPEFLAG_FOREIGNACTIONS, imn->flags & MIMETYPEFLAG_FOREIGNRECOG));
			}


			DoMethod(obj, MM_Mimegroup_Edit, ptr, imn, TRUE, generic);
		}
	}
	return (0);
}

DEFSMETHOD(Mimegroup_Mime_Ack)
{
	GETDATA;
	APTR imn;
	struct window_entry * n;

	//set(_win(obj), MUIA_Window_Sleep, FALSE);

	imn = (APTR) getv(msg->mimeobj, MA_Mimeadjustwin_MimeNode);

	if(imn)
	{
		/*
		if(msg->edit)
		{
		}
		else
		{
			
		}
		*/
        DoMethod(data->lt_mime, MM_Mimelisttree_Refresh);
	}

	/* remove from opened windows */
	ITERATELIST(n, &data->windows_list)
	{
		if(n->obj == msg->mimeobj)
		{
			REMOVE(n);
			FreeVecTaskPooled(n);
			break;
		}
	}

	DoMethod(msg->mimeobj, MM_Mimeadjustwin_Close);

	return (0);
}

DEFTMETHOD(Mimegroup_Remove)
{
	GETDATA;
	TEXT mimetype[256];
	STRPTR ptr = NULL;
	struct MUIS_Listtree_TreeNode *active = NULL, *parent = NULL;
	ULONG generic = FALSE;

	active = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_mime, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);

	if(active)
	{
		if(((struct treedata *)active->tn_User)->flags & NODEFLAG_MEDIATYPE) /* it's a mediatype family */
		{
			snprintf(mimetype, sizeof(mimetype), "%s/*", active->tn_Name);
			ptr = mimetype;
			generic = TRUE;
		}
		else /* it's a mimetype */
		{
			parent = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_mime, MUIM_Listtree_GetEntry, active, MUIV_Listtree_GetEntry_Position_Parent, 0);

			if(parent)
			{
				snprintf(mimetype, sizeof(mimetype), "%s/%s", parent->tn_Name, active->tn_Name);
				ptr = mimetype;
			}
		}
	}

	if (active != MUIV_Listtree_Active_Off)
	{
		struct internal_mimetype_node * imn = NULL;

		if(ptr)
		{
			if(generic)
			{
				imn	= mimetype_find_generic_by_mimetype(ptr);
			}
			else
			{
				imn	= mimetype_find_by_mimetype(ptr);
			}
		}

		SDB(("passing mimetype = <%s> imn = %p generic = %ld descriptor = <%s>\n", ptr, imn, generic, (imn && imn->descriptor) ? imn->descriptor : (STRPTR) "none"));

		if(imn && imn->descriptor)
		{
			DeleteFile(imn->descriptor);
			/* invalidate mimetypes and reload database (ugly) -> in a thread */
			mimetype_invalidate( NULL, NULL );
			mimetype_load_database( NULL );

			DoMethod(data->lt_mime, MM_Mimelisttree_Refresh);
		}
	}

	return (0);
}

BEGINMTABLE
DECNEW
DECDISP
DECSMETHOD(Mimegroup_ChangeButtons)
DECSMETHOD(Mimegroup_Edit)
DECTMETHOD(Mimegroup_Remove)
DECSMETHOD(Mimegroup_Add)
DECSMETHOD(Mimegroup_Mime_Ack)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, mimegroupclass)
