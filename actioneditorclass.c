/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2006 Ambient Open Source Team
 * actioneditclass.c, Copyright 2005-2006 by Adam Waldenberg
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
 * $Id: actioneditorclass.c,v 1.3 2007/05/08 19:27:07 fab Exp $
 */

/*
 * Internal actions for icon actions:
 * IconInfo, LoadURI, Makedir, MakeLink, Rename, Select,
 * Move, Copy, LoadBackground
 */

/* public */
#include <libraries/asl.h>
#include <proto/dos.h>

/* private */
#include "ambient_cat.h"
#include "ambient.h"
#include "action.h"
#include "appclass.h"
#include "command.h"
#include "descsaver.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "imagecache.h"
#include "mimetype.h"
#include "mui_func.h"
#include "rexx.h"
#include "screen.h"

#define GETTAIL(_l)  \
({ struct List *l = (struct List *)(_l);  \
	l->lh_TailPred->ln_Pred ? l->lh_TailPred : (struct Node *)0;  \
})

typedef struct {
	const struct ambient_command *c;
	CONST_STRPTR name, description;
	ULONG set;
	APTR bitmap, grp;
} Command;

struct Data {
	struct action_node *action_node, *action_node_temp;
	ULONG delete_action_node;
	ULONG mimeaction;
	ULONG accept;

	struct {
		APTR command;
	} list;

	struct {
		APTR cdd, cds, quote, multiple;
	} checkmark;

	struct {
		APTR grp, str_name, str_menu, popasl, popstr, cyc_type, cyc_qualifier, cyc_event, lab_qualifier, c_menu, lab_menu1, lab_menu2, mimeactiongrp;
	} commandwidgets;

	struct {
		APTR up, down, addcmd, remcmd, bt_ok, bt_cancel;
	} action;
};

static STRPTR cyc_events[ 2 + MSG_ACTIONEDITORCLASS_EVENTDRAGNDROP - MSG_ACTIONEDITORCLASS_EVENTDOUBLE ];

static ULONG get_event_type(ULONG index)
{
	switch(index)
	{
		case 0:
			return ACTION_EVENT_DOUBLECLICK;
		case 1:
			return ACTION_EVENT_MENU;
		case 2:
			return ACTION_EVENT_DRAGNDROP;
	}

	return ACTION_EVENT_DOUBLECLICK;
}

static ULONG get_event_index(ULONG type)
{
	switch(type)
	{
		case ACTION_EVENT_DOUBLECLICK:
			return 0;
		case ACTION_EVENT_MENU:
			return 1;
		case ACTION_EVENT_DRAGNDROP:
			return 2;
	}

	return 0;
}

static STRPTR cyc_qualifiers[ 2 + MSG_ACTIONEDITORCLASS_QUALCONTROL - MSG_ACTIONEDITORCLASS_QUALNONE ];

static ULONG get_qualifier_type(ULONG index)
{
	switch(index)
	{
		case 0:
			return ACTION_QUALIFIER_NONE;
		case 1:
			return ACTION_QUALIFIER_SHIFT;
		case 2:
			return ACTION_QUALIFIER_ALT;
		case 3:
			return ACTION_QUALIFIER_CONTROL;
	}

	return ACTION_QUALIFIER_NONE;
}

static ULONG get_qualifier_index(ULONG type)
{
	switch(type)
	{
		case ACTION_QUALIFIER_NONE:
			return 0;
		case ACTION_QUALIFIER_SHIFT:
			return 1;
		case ACTION_QUALIFIER_ALT:
			return 2;
		case ACTION_QUALIFIER_CONTROL:
			return 3;
	}

	return 0;
}

static STRPTR cyc_commandtypes[ 2 + MSG_ACTIONEDITORCLASS_AREXX - MSG_ACTIONEDITORCLASS_INTERNAL ];

static ULONG get_command_type(ULONG index)
{
	switch(index)
	{
		case 0:
			return AC_INTERNAL;
		case 1:
			return AC_AMIGADOS;
		case 2:
			return AC_WORKBENCH;
		case 3:
			return AC_SCRIPT;
		case 4:
			return AC_AREXX;
	}

	return AC_INTERNAL;
}

static ULONG get_command_index(ULONG type)
{
	switch(type)
	{
		case AC_INTERNAL:
			return 0;
		case AC_AMIGADOS:
			return 1;
		case AC_WORKBENCH:
			return 2;
		case AC_SCRIPT:
			return 3;
		case AC_AREXX:
			return 4;
	}

	return AC_INTERNAL;
}

static CONST_STRPTR command_type(ULONG type)
{
	switch (type)
	{
		case AC_INTERNAL:
			return GSI(MSG_ACTIONEDITORCLASS_INTERNAL);
		case AC_AMIGADOS:
			return GSI(MSG_ACTIONEDITORCLASS_AMIGADOS);
		case AC_WORKBENCH:
			return GSI(MSG_ACTIONEDITORCLASS_WORKBENCH);
		case AC_SCRIPT:
			return GSI(MSG_ACTIONEDITORCLASS_SCRIPT);
		case AC_AREXX:
			return GSI(MSG_ACTIONEDITORCLASS_AREXX);
	}

	return GSI(MSG_ACTIONEDITORCLASS_UNKNOWN);
}

MUI_HOOK(actioneditor_displayfunc, STRPTR *str, APTR cn)
{
	if (!cn)
	{
		*str++ = GSI(MSG_ACTIONEDITORCLASS_TYPE);
		*str   = GSI(MSG_ACTIONEDITORCLASS_COMMANDSTRING);
	}
	else
	{
		ULONG type = (ULONG) commandnode_getattr(cn, COMMANDNODETAG_TYPE);
		STRPTR s = commandnode_getattr(cn, COMMANDNODETAG_COMMAND);
	
		*str++ = (STRPTR) command_type(type);
		*str   = s;
	}

	return 0;
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

static void set_mainmethods(struct Data *data, APTR obj)
{
	DoMethod(data->action.up, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Actioneditor_MoveCommand, -1L
	);

	DoMethod(data->action.down, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Actioneditor_MoveCommand, 1L
	);

	DoMethod(data->action.addcmd, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1, MM_Actioneditor_AddCommand
	);

	DoMethod(data->action.remcmd, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1, MM_Actioneditor_RemCommand
	);

	DoMethod(data->list.command, MUIM_Notify, MUIA_List_Active,
		MUIV_EveryTime, obj, 3, MM_Actioneditor_ChangeCommand, NULL, -1
	);

	DoMethod(data->list.command, MUIM_Notify, MUIA_List_Active,
		MUIV_EveryTime, obj, 1, MM_Actioneditor_CheckCommand
	);

	DoMethod(data->commandwidgets.popstr, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
		obj, 3, MM_Actioneditor_ChangeCommand, MUIV_TriggerValue, -1
	);

	DoMethod(data->commandwidgets.cyc_type, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
		obj, 3, MM_Actioneditor_ChangeCommand, NULL, MUIV_TriggerValue
	);

	DoMethod(data->action.bt_ok, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Actioneditor_Accept, TRUE
	);

	DoMethod(data->action.bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Actioneditor_Accept, FALSE
	);

	DoMethod(data->commandwidgets.cyc_event, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
		obj, 1, MM_Actioneditor_UpdateEvent
	);

	DoMethod(data->commandwidgets.c_menu, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
		data->commandwidgets.str_menu, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue
	);

	DoMethod(obj, MUIM_MultiSet, MUIA_CycleChain, 1,
		data->commandwidgets.str_name, data->action.addcmd, data->action.remcmd,
		data->commandwidgets.cyc_type, data->commandwidgets.popstr,
		data->commandwidgets.cyc_event, data->commandwidgets.cyc_qualifier,
		data->checkmark.cds, data->checkmark.cdd, data->checkmark.quote,
		data->action.bt_ok, data->action.bt_cancel,
		NULL
	);
}

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_Actioneditor_ActionNode:
			*msg->opg_Storage = (ULONG) (data->accept ? data->action_node : NULL);
			return(TRUE);
	}
	return (DOSUPER);
}

DEFNEW
{
	APTR listobj, c_cdd, c_cds, c_quote, c_multiple, commandwidgetsgrp,
		 mimeactiongrp, cyc_type, cyc_event, cyc_qualifier, lab_qualifier, c_menu, lab_menu1, lab_menu2;
	APTR ac_up, ac_down, ac_addcmd, ac_remcmd;
	APTR bt_ok, bt_cancel;
	APTR popstr, popasl, str_name, str_menu;
	APTR action_node = NULL, action_node_temp = NULL;
	ULONG delete_action_node = FALSE;
	ULONG mimeaction = FALSE;
	struct Data *data;

	FORTAG(INITTAGS)
	{
		case MA_Actioneditor_ActionNode:
			action_node = (APTR) tag->ti_Data;
			break;

		case MA_Actioneditor_MimeAction:
			mimeaction = tag->ti_Data;
			break;
	}
	NEXTTAG

	if(!action_node)
	{
		action_node = actionnode_create();

		if(!action_node)
		{
			return NULL;
		}

		delete_action_node = TRUE;
	}

	action_node_temp = actionnode_duplicate(action_node);

	if(!action_node_temp)
	{
		if(delete_action_node)
		{
			actionnode_delete(action_node);
		}
		return NULL;
	}

	obj = DoSuperNew(cl, obj,
		MUIA_Group_Horiz, TRUE,
		Child, VGroup,
			Child, VGroup,
				GroupFrameT(GSI(MSG_ACTIONEDITORCLASS_COMMANDGROUP)),
				Child, HGroup,
					Child, NSLabel2(MSG_ACTIONEDITORCLASS_NAME),
					Child, str_name = MUICreateString( MSG_ACTIONEDITORCLASS_NAME, 512, NULL),
					End,
				Child, HGroup,
					Child, listobj = ListObject,
						InputListFrame,
						MUIA_List_DisplayHook, (ULONG) &actioneditor_displayfunc_hook,
						MUIA_List_Format, "C=0 MIW=-1 BAR,C=1 MIW=-1",
						MUIA_List_Title, TRUE,
						MUIA_List_MinLineHeight, 20,
						MUIA_List_AutoVisible, TRUE,
						MUIA_ShortHelp, GSI(MSG_ACTIONEDITORCLASS_ACTIONLIST_HELP),
						MUIA_CycleChain, 1,
					End,
					Child, VGroup,
						MUIA_Weight, 1,
						Child, ac_addcmd = MUICreateButton(MSG_ACTIONEDITORCLASS_ADD, NULL),
						Child, ac_remcmd = MUICreateButton(MSG_ACTIONEDITORCLASS_REMOVE, NULL),
						Child, HGroup,
							Child, ac_up   = MUICreateImageButton( MSG_ACTIONEDITORCLASS_UP_HELP-1  , MUII_TapeUp  , NULL),
							Child, ac_down = MUICreateImageButton( MSG_ACTIONEDITORCLASS_DOWN_HELP-1, MUII_TapeDown, NULL),
						End,
						Child, VSpace(0),
					End,
				End,
				Child, RectangleObject,
					MUIA_Rectangle_HBar, TRUE,
					MUIA_Weight, 0L,
				End,
				Child, commandwidgetsgrp = HGroup,
					InnerSpacing(0,0),
					MUIA_ShowMe, TRUE,
					MUIA_Weight, 0L,
					Child, HGroup,
						MUIA_Weight, 0L,
						Child, cyc_type = MUICreateCycle( MSG_ACTIONEDITORCLASS_TYPE, &cyc_commandtypes, MSG_ACTIONEDITORCLASS_INTERNAL, MSG_ACTIONEDITORCLASS_AREXX, NULL ),
					End,
					Child, popasl = PopaslObject,
						MUIA_Popasl_Type, ASL_FileRequest,
						MUIA_Popstring_Button, MUICreatePopButton(MSG_ACTIONEDITORCLASS_COMMAND_HELP-1, MUII_PopFile, NULL),
						MUIA_Popstring_String, popstr = StringObject,
							StringFrame,
							MUIA_String_MaxLen, 512,
							MUIA_ShortHelp  , GSI(MSG_ACTIONEDITORCLASS_COMMAND_HELP),
						End,
					End,
				End,
			End,
			Child, mimeactiongrp = HGroup, MUIA_ShowMe, mimeaction,
					Child, NSLabel2(MSG_ACTIONEDITORCLASS_EVENTTYPE),
					Child, cyc_event = MUICreateCycle( MSG_ACTIONEDITORCLASS_EVENTTYPE, &cyc_events, MSG_ACTIONEDITORCLASS_EVENTDOUBLE, MSG_ACTIONEDITORCLASS_EVENTDRAGNDROP, NULL ),
					Child, lab_qualifier = NSLabel2(MSG_ACTIONEDITORCLASS_QUALIFIER),
					Child, cyc_qualifier = MUICreateCycle( MSG_ACTIONEDITORCLASS_QUALIFIER, &cyc_qualifiers, MSG_ACTIONEDITORCLASS_QUALNONE, MSG_ACTIONEDITORCLASS_QUALCONTROL, NULL ),

					Child, lab_menu1 = NSLabel2(MSG_ACTIONEDITORCLASS_SORTBYPARENT),
					Child, c_menu    = MUICreateCheckbox(MSG_ACTIONEDITORCLASS_SORTBYPARENT, FALSE, NULL),
					Child, lab_menu2 = NSLabel2(MSG_ACTIONEDITORCLASS_PARENTNAME),
					Child, str_menu  = MUICreateString( MSG_ACTIONEDITORCLASS_PARENTNAME, 512, NULL),
					Child, HSpace(0),
				End,
			Child, HGroup,
				Child, ColGroup(3),
					Child, c_cds   = MUICreateCheckbox(MSG_ACTIONEDITORCLASS_CDTOSOURCE         , FALSE, NULL),
					Child, MUICreateLabel(MSG_ACTIONEDITORCLASS_CDTOSOURCE     , MUIO_Label_LeftAligned|MUIO_Label_SingleFrame),
					Child, HSpace(0),
					Child, c_cdd   = MUICreateCheckbox(MSG_ACTIONEDITORCLASS_CDTODESTINATION    , FALSE, NULL),
					Child, MUICreateLabel(MSG_ACTIONEDITORCLASS_CDTODESTINATION, MUIO_Label_LeftAligned|MUIO_Label_SingleFrame),
					Child, HSpace(0),
					Child, c_quote = MUICreateCheckbox(MSG_ACTIONEDITORCLASS_DONTQUOTEARGS      , FALSE, NULL),
					Child, MUICreateLabel(MSG_ACTIONEDITORCLASS_DONTQUOTEARGS  , MUIO_Label_LeftAligned|MUIO_Label_SingleFrame),
					Child, HSpace(0),
					Child, c_multiple = MUICreateCheckbox(MSG_ACTIONEDITORCLASS_ACTIONMULTIPLEFILES , FALSE, NULL),
					Child, MUICreateLabel(MSG_ACTIONEDITORCLASS_ACTIONMULTIPLEFILES, MUIO_Label_LeftAligned|MUIO_Label_SingleFrame),
					Child, HSpace(0),

				End,
			End,
			Child, HGroup,
				Child, bt_ok     = MUICreateButton( MSG_ACTIONEDITORCLASS_OK    , NULL ),
				Child, RectangleObject, End,
				Child, bt_cancel = MUICreateButton( MSG_ACTIONEDITORCLASS_CANCEL, NULL ),
			End,
		End,
		TAG_MORE, INITTAGS,
	End;

	if (obj)
	{
		data = INST_DATA(cl, obj);
		data->action_node = action_node;
		data->action_node_temp = action_node_temp;
		data->delete_action_node = delete_action_node;
		data->accept = FALSE;
		data->mimeaction = mimeaction;

		data->list.command = listobj;
		data->checkmark.cds = c_cds;
		data->checkmark.cdd = c_cdd;
		data->checkmark.quote = c_quote;
		data->checkmark.multiple = c_multiple;
		data->action.up = ac_up;
		data->action.down = ac_down;
		data->action.addcmd = ac_addcmd;
		data->action.remcmd = ac_remcmd;
		data->action.bt_ok = bt_ok;
		data->action.bt_cancel = bt_cancel;
		data->commandwidgets.str_name  = str_name;
		data->commandwidgets.grp = commandwidgetsgrp;
		data->commandwidgets.popstr = popstr;
		data->commandwidgets.popasl = popasl;
		data->commandwidgets.cyc_type = cyc_type;
		data->commandwidgets.cyc_event = cyc_event;
		data->commandwidgets.cyc_qualifier = cyc_qualifier;
		data->commandwidgets.lab_qualifier = lab_qualifier;
		data->commandwidgets.str_menu = str_menu;
		data->commandwidgets.c_menu = c_menu;
		data->commandwidgets.lab_menu1 = lab_menu1;
		data->commandwidgets.lab_menu2 = lab_menu2;

		DoMethod(obj, MM_Actioneditor_ShowAction);

		set_mainmethods(data, obj);
	}

	return (ULONG) obj;
}

DEFDISPOSE
{
	GETDATA;

	if(data->action_node_temp)
	{
		actionnode_delete(data->action_node_temp);
	}

	if(data->action_node && data->delete_action_node && !data->accept)
	{
		actionnode_delete(data->action_node);
	}

	return DOSUPER;
}

DEFSMETHOD(Actioneditor_ChangeCommand)
{
	GETDATA;
	APTR cn;

	DoMethod(data->list.command, MUIM_List_GetEntry,
		MUIV_List_GetEntry_Active, &cn
	);

	if(cn)
	{
		if(msg->type != -1)
		{
			commandnode_setattrs(cn, COMMANDNODETAG_TYPE, get_command_type(msg->type), TAG_DONE);
            DoMethod(data->list.command, MUIM_List_Redraw, MUIV_List_Redraw_Active);
		}

		if(msg->cmd)
		{
			commandnode_setattrs(cn, COMMANDNODETAG_COMMAND, msg->cmd, TAG_DONE);
			DoMethod(data->list.command, MUIM_List_Redraw, MUIV_List_Redraw_Active);
		}
		
		if(msg->type == -1 && msg->cmd == NULL)
		{
			STRPTR s;
			ULONG  type;

			if ((s = commandnode_getattr(cn, COMMANDNODETAG_COMMAND)))
			{
				set(data->commandwidgets.popstr, MUIA_String_Contents, s);
			}

			if ((type = (ULONG) commandnode_getattr(cn, COMMANDNODETAG_TYPE)))
			{
				set(data->commandwidgets.cyc_type, MUIA_Cycle_Active, get_command_index(type));
			}
		}
	}

	DoMethod(obj, MM_Actioneditor_CheckCommand);

	return 0;
}

DEFTMETHOD(Actioneditor_ShowAction)
{
	GETDATA;

	APTR commandlist = (APTR)   actionnode_getattr(data->action_node_temp, ACTIONNODETAG_COMMAND_LIST);
	STRPTR name      = (STRPTR) actionnode_getattr(data->action_node_temp, ACTIONNODETAG_NAME);
	STRPTR menuname  = (STRPTR) actionnode_getattr(data->action_node_temp, ACTIONNODETAG_MENU_NAME);
	ULONG flags      = (ULONG)  actionnode_getattr(data->action_node_temp, ACTIONNODETAG_FLAGS);
	ULONG qualifier  = (ULONG)  actionnode_getattr(data->action_node_temp, ACTIONNODETAG_QUALIFIER);
	ULONG event      = (ULONG)  actionnode_getattr(data->action_node_temp, ACTIONNODETAG_EVENT);
	APTR cn;

	/* fill action */

	/* event type, qualifier, menu name */
	if(data->mimeaction)
	{
		set(data->commandwidgets.cyc_event, MUIA_Cycle_Active, get_event_index(event));
		set(data->commandwidgets.cyc_qualifier, MUIA_Cycle_Active, get_qualifier_index(qualifier));

		/* menu name */

		set(data->commandwidgets.c_menu, MUIA_Selected, menuname != NULL);
		set(data->commandwidgets.str_menu, MUIA_Disabled, menuname == NULL);

		if(menuname)
		{
			set(data->commandwidgets.str_menu, MUIA_String_Contents, menuname);
		}

		DoMethod(obj, MM_Actioneditor_UpdateEvent);
	}

	/* name */
	set(data->commandwidgets.str_name, MUIA_String_Contents, name);

	/* command list */
	DoMethod(data->list.command, MUIM_List_Clear);

	if (!ISLISTEMPTY(commandlist))
	{
		ITERATELIST(cn, commandlist)
		{
			DoMethod(data->list.command, MUIM_List_InsertSingle,
				cn, MUIV_List_Insert_Bottom
			);
		}
	}

	/* flags */
	set(data->checkmark.quote,    MUIA_Selected, (flags & ACTION_FLAG_UNQUOTED) ? TRUE : FALSE);
	set(data->checkmark.cds,      MUIA_Selected, (flags & ACTION_FLAG_CD_SOURCE) ? TRUE : FALSE);
	set(data->checkmark.cdd,      MUIA_Selected, (flags & ACTION_FLAG_CD_DESTINATION) ? TRUE : FALSE);
	set(data->checkmark.multiple, MUIA_Selected, (flags & ACTION_FLAG_MULTIPLE) ? TRUE : FALSE);

	if(getv(data->list.command, MUIA_List_Entries) > 0)
	{
		set(data->list.command, MUIA_List_Active, 0);
		DoMethod(obj, MM_Actioneditor_ChangeCommand, NULL, -1);
	}

	DoMethod(obj, MM_Actioneditor_CheckCommand);

	return 0;
}

DEFTMETHOD(Actioneditor_AddCommand)
{
	GETDATA;

	if (actionnode_addcommand(data->action_node_temp, AC_AMIGADOS, GSI(MSG_ACTIONEDITORCLASS_SPECIFYCOMMANDHERE)))
	{
		APTR commandlist = (APTR) actionnode_getattr(data->action_node_temp, ACTIONNODETAG_COMMAND_LIST);
		struct MinNode *cn = (struct MinNode *) GETTAIL(commandlist);

		DoMethod(data->list.command, MUIM_List_InsertSingle,
			cn, MUIV_List_Insert_Bottom
		);

		set(data->list.command, MUIA_List_Active, MUIV_List_Active_Bottom);
        set(_win(obj), MUIA_Window_ActiveObject, data->commandwidgets.popstr);
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}

DEFTMETHOD(Actioneditor_RemCommand)
{
	struct MinNode *cn;
	GETDATA;

	DoMethod(data->list.command, MUIM_List_GetEntry,
		MUIV_List_GetEntry_Active, &cn
	);

	if(cn)
	{
		DoMethod(data->list.command, MUIM_List_Remove, MUIV_List_Remove_Active);
		actionnode_remcommand(cn);
	}
	
	return 0;
}

DEFSMETHOD(Actioneditor_MoveCommand)
{
	struct MinNode *cn;
	GETDATA;

	DoMethod(data->list.command, MUIM_List_GetEntry,
		MUIV_List_GetEntry_Active, &cn
	);

	if (msg->offset == -1)
	{	 
		if(getv(data->list.command, MUIA_List_Active) > 0)
		{
			swap_nodes(cn->mln_Pred);
			DoMethod(data->list.command, MUIM_List_Exchange,
				MUIV_List_Exchange_Active, MUIV_List_Exchange_Previous
			);
			set(data->list.command, MUIA_List_Active, MUIV_List_Active_Up);
		}
	}
	else if(getv(data->list.command, MUIA_List_Active) < (getv(data->list.command, MUIA_List_Entries) - 1))
	{
		swap_nodes(cn);
		DoMethod(data->list.command, MUIM_List_Exchange,
			MUIV_List_Exchange_Active, MUIV_List_Exchange_Next
		);
		set(data->list.command, MUIA_List_Active, MUIV_List_Active_Down);
	}

	return 0;
}

DEFSMETHOD(Actioneditor_Accept)
{
	GETDATA;

	if(msg->accept)
	{
		APTR commandlist;
		APTR commandlist_temp;
		ULONG flags;
		APTR cn;

		data->accept = TRUE;

		DoMethod(obj, MM_Actioneditor_ChangeCommand, getv(data->commandwidgets.popstr, MUIA_String_Contents), -1);

		commandlist      = (APTR)  actionnode_getattr(data->action_node, ACTIONNODETAG_COMMAND_LIST);
		commandlist_temp = (APTR)  actionnode_getattr(data->action_node_temp, ACTIONNODETAG_COMMAND_LIST);
		flags            = (ULONG) actionnode_getattr(data->action_node_temp, ACTIONNODETAG_FLAGS);

		/* name */
		actionnode_setattrs(data->action_node,
							ACTIONNODETAG_NAME, getv(data->commandwidgets.str_name, MUIA_String_Contents),
							TAG_DONE
		);

		/* command list XXX:not sure it should clean or delete(kiero) */
		commandlist_clear(commandlist);

		if (!ISLISTEMPTY(commandlist_temp))
		{
			ITERATELIST(cn, commandlist_temp)
			{
				STRPTR cmd = (STRPTR) commandnode_getattr(cn, COMMANDNODETAG_COMMAND);
				ULONG type = (ULONG) commandnode_getattr(cn, COMMANDNODETAG_TYPE);
				
				actionnode_addcommand(data->action_node, type, cmd);
			}
		}

		/* copy event, qualifier, flags */

		if(data->mimeaction)
		{
			STRPTR menuname = NULL;

			actionnode_setattrs(data->action_node,
								ACTIONNODETAG_EVENT, get_event_type(getv(data->commandwidgets.cyc_event, MUIA_Cycle_Active)),
								TAG_DONE
			);

			actionnode_setattrs(data->action_node,
								ACTIONNODETAG_QUALIFIER, get_qualifier_type(getv(data->commandwidgets.cyc_qualifier, MUIA_Cycle_Active)),
								TAG_DONE
			);

			if( (get_event_type(getv(data->commandwidgets.cyc_event, MUIA_Cycle_Active) == ACTION_EVENT_MENU) &&
				getv(data->commandwidgets.c_menu, MUIA_Selected)) )
			{
				menuname = (STRPTR) getv(data->commandwidgets.str_menu, MUIA_String_Contents);
			}

			actionnode_setattrs(data->action_node,
								ACTIONNODETAG_MENU_NAME, menuname,
								TAG_DONE
			);
		}

		if(getv(data->checkmark.quote, MUIA_Selected))
		{
			flags |= ACTION_FLAG_UNQUOTED;
		}
		else
		{
			flags &= ~ACTION_FLAG_UNQUOTED;
		}

		if(getv(data->checkmark.cds, MUIA_Selected))
		{
			flags |= ACTION_FLAG_CD_SOURCE;
		}
		else
		{
			flags &= ~ACTION_FLAG_CD_SOURCE;
		}

		if(getv(data->checkmark.cdd, MUIA_Selected))
		{
			flags |= ACTION_FLAG_CD_DESTINATION;
		}
		else
		{
			flags &= ~ACTION_FLAG_CD_DESTINATION;
		}

		if(getv(data->checkmark.multiple, MUIA_Selected))
		{
			flags |= ACTION_FLAG_MULTIPLE;
		}
		else
		{
			flags &= ~ACTION_FLAG_MULTIPLE;
		}

		actionnode_setattrs(data->action_node,
							ACTIONNODETAG_FLAGS, flags,
							TAG_DONE
		);
	}
	else
	{
		data->accept = FALSE;
	}

	set(_win(obj), MUIA_Window_Open, FALSE);

	return (0);
}

DEFTMETHOD(Actioneditor_UpdateEvent)
{
	GETDATA;

	if(get_event_type(getv(data->commandwidgets.cyc_event, MUIA_Cycle_Active)) != ACTION_EVENT_DRAGNDROP)
	{
		set(data->commandwidgets.cyc_qualifier, MUIA_Cycle_Active, get_qualifier_index(ACTION_QUALIFIER_NONE));
		set(data->commandwidgets.lab_qualifier, MUIA_ShowMe, FALSE);
		set(data->commandwidgets.cyc_qualifier, MUIA_ShowMe, FALSE);
	}
	else
	{
		set(data->commandwidgets.lab_qualifier, MUIA_ShowMe, TRUE);
		set(data->commandwidgets.cyc_qualifier, MUIA_ShowMe, TRUE);
	}

	if(get_event_type(getv(data->commandwidgets.cyc_event, MUIA_Cycle_Active)) != ACTION_EVENT_MENU)
	{
		set(data->commandwidgets.lab_menu1, MUIA_ShowMe, FALSE);
		set(data->commandwidgets.c_menu   , MUIA_ShowMe, FALSE);
		set(data->commandwidgets.lab_menu2, MUIA_ShowMe, FALSE);
		set(data->commandwidgets.str_menu , MUIA_ShowMe, FALSE);
	}
	else
	{
		set(data->commandwidgets.lab_menu1, MUIA_ShowMe, TRUE);
		set(data->commandwidgets.c_menu   , MUIA_ShowMe, TRUE);
		set(data->commandwidgets.lab_menu2, MUIA_ShowMe, TRUE);
		set(data->commandwidgets.str_menu , MUIA_ShowMe, TRUE);
	}

	return (0);
}

DEFTMETHOD(Actioneditor_CheckCommand)
{
	GETDATA;

	if(getv(data->list.command, MUIA_List_Active) == MUIV_List_Active_Off)
	{
		set(data->commandwidgets.grp, MUIA_ShowMe, FALSE);
	}
	else
	{
		set(data->commandwidgets.grp, MUIA_ShowMe, TRUE);
	}

	return (0);
}

BEGINMTABLE
DECNEW
DECGET
DECDISPOSE
DECSMETHOD(Actioneditor_ChangeCommand)
DECTMETHOD(Actioneditor_ShowAction)
DECTMETHOD(Actioneditor_AddCommand)
DECTMETHOD(Actioneditor_RemCommand)
DECSMETHOD(Actioneditor_MoveCommand)
DECTMETHOD(Actioneditor_CheckCommand)
DECSMETHOD(Actioneditor_Accept)
DECTMETHOD(Actioneditor_UpdateEvent)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, actioneditorclass)
