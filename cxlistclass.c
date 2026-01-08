/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2013 Ambient Open Source Team
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
 * $Id: cxlistclass.c,v 1.11 2017/07/23 20:58:35 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <dos/dosextens.h>
#include <libraries/commodities.h>
#include <proto/exec.h>
#include <proto/dos.h>

/* private */
#include "cx.h"
#include "mui_func.h"
#include "prefs.h"
#include "prefs_advanced.h"


struct cxadd {
	struct IClass       *cl;
	Object              *obj;
	struct PrivateCxObj *mco;
};

struct cxent {
	APTR           addr;   /* stupid GoldED adds two brokers so.. */
	STRPTR         name;
	STRPTR         title;
	STRPTR         descr;
	struct Task    *task;
	struct MsgPort *mp;
	LONG           pri;    /* XXX: would be useful to show that I think */
	ULONG          cnt;
	UBYTE          flags;
};


struct Data {
	ULONG       cnt;
	STRPTR      tokenized;
};


DEFMMETHOD(List_Construct)
{
	struct cxadd *ca = msg->entry;
	struct cxent *cxe;

	if ((cxe = malloc(sizeof(*cxe))))
	{
		if ((cxe->name = malloc(strlen(ca->mco->mco_Name) + 1)))
		{
			strcpy(cxe->name, ca->mco->mco_Name);

			if ((cxe->title = malloc(strlen(ca->mco->mco_Title) + 1)))
			{
				strcpy(cxe->title, ca->mco->mco_Title);

				if ((cxe->descr = malloc(strlen(ca->mco->mco_Descr) + 1)))
				{
					struct Data *data = INST_DATA(ca->cl, ca->obj);

					strcpy(cxe->descr, ca->mco->mco_Descr);

					cxe->addr  = ca->mco;
					cxe->task  = ca->mco->mco_Task;
					cxe->mp    = ca->mco->mco_Port;
					cxe->pri   = ca->mco->mco_Node.ln_Pri;
					cxe->flags = ca->mco->mco_Flags;
					cxe->cnt   = data->cnt;

					return ((ULONG)cxe);
				}
				free(cxe->title);
			}
			free(cxe->name);
		}
		free(cxe);
	}
	return (0);
}


DEFMMETHOD(List_Destruct)
{
	struct cxent *cxe = msg->entry;

	ASSERT(cxe);
	ASSERT(cxe->name);
	ASSERT(cxe->title);
	ASSERT(cxe->descr);

	free(cxe->descr);
	free(cxe->title);
	free(cxe->name);
	free(cxe);

	return (0);
}


DEFMMETHOD(List_Display)
{
	struct cxent *cxe = msg->entry;

	msg->array[0] = cxe->name;

	return (0);
}


DEFMMETHOD(List_Compare)
{
	struct cxent *cxe1 = msg->entry1;
	struct cxent *cxe2 = msg->entry2;

	return (stricmp(cxe1->name, cxe2->name));
}


DEFNEW
{
	obj = DoSuperNew(cl, obj,
		InputListFrame,
		//MUIA_List_AdjustWidth, TRUE,
		MUIA_List_AutoVisible, TRUE,
	 TAG_MORE, INITTAGS);


	if (obj)
	{
		GETDATA;

		/*  set up commodity filter
	 	 */
		{
			STRPTR pattern = _ap_string(commodityfilter);
			ULONG l = strlen(pattern);

			if ((data->tokenized = malloc(l*2+16)))
			{
				(void)ParsePattern(pattern, data->tokenized, l*2+16);
			}
			else
			{
				PDB(("Not enough memory for creating tokenized string out of: %s\n", pattern));
			}
		}

		DoMethod(obj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
			obj, 2, MM_Cxlist_SetStatus, MUIV_TriggerValue
		);

		return ((ULONG)obj);
	}

	return ((ULONG)NULL);
}


DEFDISP
{
	GETDATA;

	if (data->tokenized)
	{
		free(data->tokenized);
	}

	return (DOSUPER);
}


DEFTMETHOD(Cxlist_InitChange)
{
	GETDATA;

	data->cnt++;

	set(obj, MUIA_List_Quiet, TRUE);

	return (0);
}


DEFSMETHOD(Cxlist_TryAdd)
{
	GETDATA;

	ULONG  i;
	struct cxent *cxe;
	ULONG  found = FALSE;

	/*   block 2 brokers from same adress (e.g. GoldEd)
	 */
	for (i = 0; ; i++)
	{
		DoMethod(obj, MUIM_List_GetEntry, i, &cxe);

		if (cxe)
		{
			if (cxe->addr == msg->mco)
			{
				cxe->cnt = data->cnt;
				found = TRUE;
				break;
			}
		}
		else
		{
			break;
		}
	}

	/*   if not in filter, then block
	 */
	if ( !MatchPattern(data->tokenized, msg->mco->mco_Name))
	{
		found = TRUE;
	}

	if (!found)
	{
		struct cxadd ca;

		ca.cl  = cl;
		ca.obj = obj;
		ca.mco = msg->mco;

		DoMethod(obj, MUIM_List_InsertSingle, &ca, MUIV_List_Insert_Sorted);
	}
	return (0);
}


DEFTMETHOD(Cxlist_ExitChange)
{
	GETDATA;

	ULONG i = 0;
	struct cxent *cxe;

	while (1)
	{
		DoMethod(obj, MUIM_List_GetEntry, i, &cxe);

		if (cxe)
		{
			if (cxe->cnt != data->cnt)
			{
				DoMethod(obj, MUIM_List_Remove, i);
				continue;
			}
		}
		else
		{
			break;
		}
		i++;
	}

	set(obj, MUIA_List_Quiet, FALSE);

	return (0);
}


DEFSMETHOD(Cxlist_CxNotify)
{
	struct cxent *cxe;

	DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &cxe);

	if (cxe)
	{
		if (!cxe->mp && cxe->task && msg->cmd == CXCMD_KILL)
		{
			Signal(cxe->task, SIGBREAKF_CTRL_E);
		}
		else if (msg->cmd != CXCMD_LIST_CHG) /* we shouldn't get that signal but who knows.. */
		{
			if (cxe->mp)
			{
				struct PrivateCxMsg *cxmsg;

				if ((cxmsg = malloc(sizeof(*cxmsg))))
				{
					memset(cxmsg, 0, sizeof(*cxmsg));

					cxmsg->cxm_Type = CXM_COMMAND;
					cxmsg->cxm_ID = msg->cmd;
					cxmsg->cxm_Message.mn_ReplyPort = cxport;

					PutMsg(cxe->mp, (struct Message *)cxmsg);

					if (msg->cmd == CXCMD_ENABLE)
					{
						cxe->flags |= 2;
					}
					else if (msg->cmd == CXCMD_DISABLE)
					{
						cxe->flags &= ~2;
					}
				}
			}
			/* XXX */
		}
	}
	return (0);
}


DEFSMETHOD(Cxlist_SetStatus)
{
	if (msg->val == MUIV_List_Active_Off)
	{
		if (muiRenderInfo(obj) && _win(obj))
		{
			DoMethod(_win(obj), MM_Cxwin_SetStatus,
				0,
				0,
				FALSE,
				NULL,
				NULL
			);
		}
	}
	else
	{
		struct cxent *cxe;

		DoMethod(obj, MUIM_List_GetEntry, msg->val, &cxe);

		ASSERT(cxe);

		if (muiRenderInfo(obj) && _win(obj)) /* XXX: well, we could lose the commodity when switching screenmode.. unlikely but well :) */
		{
			DoMethod(_win(obj), MM_Cxwin_SetStatus,
				cxe->flags & 4, /* COF_SHOW_HIDE */
				cxe->flags & 2, /* COF_ACTIVE */
				TRUE,
				cxe->title,
				cxe->descr
			);
		}
	}
	return (0);
}


BEGINMTABLE
DECNEW
DECDISP
DECMMETHOD(List_Construct)
DECMMETHOD(List_Destruct)
DECMMETHOD(List_Display)
DECMMETHOD(List_Compare)
DECTMETHOD(Cxlist_InitChange)
DECSMETHOD(Cxlist_TryAdd)
DECTMETHOD(Cxlist_ExitChange)
DECSMETHOD(Cxlist_CxNotify)
DECSMETHOD(Cxlist_SetStatus)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, cxlistclass)
