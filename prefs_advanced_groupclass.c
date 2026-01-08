/*
 * Ambient - the ultimate desktop
 * ------------------------------
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
 * $Id: prefs_advanced_groupclass.c,v 1.4 2017/07/25 20:18:40 piru Exp $
 */

#include "ambient.h"

/* public */
#include <mui/Listtree_mcc.h>
#include <proto/dos.h>

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "prefs_advanced.h"


struct Data {
	APTR ll_list;
	APTR editgroup;
	APTR editobj;
	APTR bt_reset;

	struct pa_node *cn;
};


DEFNEW
{
	APTR ll_list, editgroup, bt_reset;

	obj = DoSuperNew(cl, obj,
		Child, ll_list = NewObject(getadvancedprefslistclass(), NULL, 
			MUIA_CycleChain, TRUE,
			TAG_DONE),
		Child, editgroup = HGroup,
			MUIA_ShowMe, FALSE,
			Child, TextObject, MUIA_Weight, 1, MUIA_Text_Contents, GSI( MSG_PREFSADVANCEDGROUPCLASS_VALUE ), End,
			Child, RectangleObject, MUIA_Rectangle_VBar, TRUE, MUIA_Weight, 0, End,
			Child, bt_reset = MUICreateButton( MSG_PREFSADVANCEDGROUPCLASS_RESETTODEFAULT, ""),
		End,
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		GETDATA;

		data->ll_list   = ll_list;
		data->bt_reset  = bt_reset;
		data->editgroup = editgroup;
		data->editobj   = NULL;
		data->cn        = NULL;

		set(bt_reset, MUIA_CycleChain, TRUE);
		set(bt_reset, MUIA_Disabled,   TRUE);

		DoMethod(ll_list,  MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, obj, 1, MM_Advancedprefsgroup_UpdateGadgets);
		DoMethod(bt_reset, MUIM_Notify, MUIA_Pressed,     FALSE,          obj, 1, MM_Advancedprefsgroup_ResetValue);
	}

	return (ULONG)obj; /* returns NULL if DoSuperNew() failed */
}


DEFDISP
{
	return (DOSUPER);
}


DEFTMETHOD(Advancedprefsgroup_UpdateGadgets)
{
	GETDATA;
	struct pa_node *n;


	DoMethod(data->ll_list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &n);

	if (n)
	{
		APTR  editobj    = NULL;
		APTR  editgroup  = data->editgroup;
		ULONG notifymeon = 0;

		switch(n->n_type)
		{
			case PA_STRING:
				editobj = StringObject, 
					StringFrame, 
					MUIA_Text_Contents, n->n_value, 
				End;
				notifymeon = MUIA_String_Contents; /* XXX: or better String_Acknowledge? might be more save. */
				break;

			case PA_UINT: 
			case PA_INT:  
				editobj = StringObject, 
					StringFrame, 
					MUIA_String_Accept,  (n->n_type == PA_INT) ? "-0123456789" : "0123456879",
	   				MUIA_String_Integer, n->n_value,
				End;
				notifymeon = MUIA_String_Contents; /* XXX: or better String_Acknowledge? might be more save. */
				break;

			case PA_BOOL:
				{
					const STRPTR onandoff[] = { GSI( MSG_PREFSADVANCEDGROUPCLASS_ON ), GSI( MSG_PREFSADVANCEDGROUPCLASS_OFF ), NULL};
					editobj = RadioObject,
						MUIA_Group_Horiz,   TRUE,
						MUIA_Radio_Entries, onandoff,
						MUIA_Radio_Active,  !n->n_value,
						Child, RectangleObject, End,  /* 'dirty' trick, because MUI layouting suxx sometimes */
					End;
					notifymeon = MUIA_Radio_Active;
				}
				break;

			case PA_COLOUR:
				/* XXX: */
				//break;
			default:
				DB(("unsupported"))
				break;
		}

		if (editobj)
		{
			set(editobj, MUIA_Weight,     300);
			set(editobj, MUIA_CycleChain, TRUE);

			DoMethod(editobj, MUIM_Notify, notifymeon, MUIV_EveryTime, obj, 1, MM_Advancedprefsgroup_ApplyValue);

			DoMethod(editgroup, MUIM_Group_InitChange);

			if (data->editobj)
			{
				DoMethod(editgroup, OM_REMMEMBER, data->editobj);
				MUI_DisposeObject(data->editobj);
			}

			DoMethod(editgroup, OM_ADDMEMBER, editobj);
			DoMethod(editgroup, MUIM_Group_MoveMember, editobj, 1);
			DoMethod(editgroup, MUIM_Group_ExitChange);	

			data->editobj = editobj;	
			data->cn      = n;

			set(editgroup, MUIA_ShowMe, TRUE);

			set(data->bt_reset, MUIA_Disabled, !(n->n_flags & PAF_USERDEFINED));
		}
		else
		{
			data->cn = NULL;
			set (editgroup, MUIA_ShowMe, FALSE);
		}
	}

	return 0;
}


DEFTMETHOD(Advancedprefsgroup_ApplyValue)
{
	GETDATA;
	struct pa_node *n = data->cn;
	APTR editobj      = data->editobj;
	
	if (n && editobj)
	{
		ULONG v;

		switch(n->n_type)
		{
			case PA_STRING:
				get(editobj, MUIA_String_Contents, &v);
				break;

			case PA_BOOL:
				get(editobj, MUIA_Radio_Active, &v);
				v = !v;
				break;

			case PA_INT:
			case PA_UINT:
				get(editobj, MUIA_String_Integer, &v); /* might be a bit limited for UINT, but what the dell! :) */
				break;

			case PA_COLOUR:
			default:
				DB(("unsupported"));
		}

		prefs_advanced_setvalue(n, v);
		DoMethod(data->ll_list, MUIM_List_Redraw, MUIV_List_Redraw_Active);
		
		/* refresh disabled state if required */
		set(data->bt_reset, MUIA_Disabled, !(n->n_flags & PAF_USERDEFINED));
	}

	return 0;
}


DEFTMETHOD(Advancedprefsgroup_ResetValue)
{
	GETDATA;
	struct pa_node *n = data->cn;
	
	if (n)
	{
		prefs_advanced_resetvalue(n);

		/*  update gadget status
		 */
		DoMethod(data->ll_list, MUIM_List_Redraw, MUIV_List_Redraw_Active);
		DoMethod(obj, MM_Advancedprefsgroup_UpdateGadgets);
	}

	return 0;
}


BEGINMTABLE
DECNEW
DECDISP
DECTMETHOD(Advancedprefsgroup_UpdateGadgets)
DECTMETHOD(Advancedprefsgroup_ApplyValue)
DECTMETHOD(Advancedprefsgroup_ResetValue)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, advancedprefsgroupclass)
