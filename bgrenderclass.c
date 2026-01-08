/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2006 Ambient Open Source Team
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
 * $Id: bgrenderclass.c,v 1.13 2017/08/21 06:17:44 cyfm Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "background.h"
#include "prefswin.h"

struct Data {
	APTR cyc_bgrender;
	APTR grp_bgrender;
	APTR pp_color;
	ULONG needsbg;
};


struct MUI_PenSpec defcol = {
	"m2"
};


DEFNEW
{
	struct Data *data;
	APTR cyc_bgrender;
	APTR grp_bgrender;
	APTR pp_color;
	ULONG isroot;
	static STRPTR cycleopts_root[7];
	static STRPTR cycleopts_window[3];

	cycleopts_window[0] = cycleopts_root[0] = GSI(MSG_PREFSWIN_BACKGROUND_MODE_COLOR);
	cycleopts_window[1] = cycleopts_root[1] = GSI(MSG_PREFSWIN_BACKGROUND_MODE_TILED);
	cycleopts_root[2] = GSI(MSG_PREFSWIN_BACKGROUND_MODE_CENTERED);
	cycleopts_root[3] = GSI(MSG_PREFSWIN_BACKGROUND_MODE_SCALED);
	cycleopts_root[4] = GSI(MSG_PREFSWIN_BACKGROUND_MODE_STRETCHED);
	cycleopts_root[5] = GSI(MSG_PREFSWIN_BACKGROUND_MODE_ZOOMED);

	isroot = GetTagData(MA_BGRender_Root, FALSE, INITTAGS);

	obj = DoSuperNew(cl, obj,
		Child, HGroup,

			Child, cyc_bgrender = MUI_MakeObject(MUIO_Cycle, GetTagData(MA_BGRender_Label, (ULONG)NULL, INITTAGS), isroot ? cycleopts_root : cycleopts_window),

			Child, grp_bgrender = PageGroup,

				MUIA_HorizWeight, 25,

				Child, HGroup,
					Child, NSLabel2(MSG_PREFSWIN_BACKGROUND_MODE_COLOR),
					Child, pp_color = PoppenObject,
						MUIA_CycleChain, 1,
						MUIA_Window_Title, GSI(MSG_PREFSWIN_BACKGROUND_MODE_COLOR_SELECT),
					End,
				End,

				Child, HVSpace,

			End,
		End,
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return ((ULONG)NULL);
	}

	data = INST_DATA(cl, obj);
	data->cyc_bgrender = cyc_bgrender;
	data->grp_bgrender = grp_bgrender;
	data->pp_color = pp_color;

	set(cyc_bgrender, MUIA_CycleChain, 1);
	setupprefs(cyc_bgrender, MUIA_Cycle_Active,
		GetTagData(MA_BGRender_Mode, BGRENDER_Color, INITTAGS)
	);

	setupprefs(pp_color, MUIA_Pendisplay_Spec,
		GetTagData(MA_BGRender_Color, (ULONG)&defcol, INITTAGS)
	);

	DoMethod(obj, MM_BGRender_Change, getv(data->cyc_bgrender, MUIA_Cycle_Active));

	DoMethod(data->cyc_bgrender, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
		obj, 2, MM_BGRender_Change, MUIV_TriggerValue
	);

	return ((ULONG)obj);
}


DEFSMETHOD(BGRender_Change)
{
	GETDATA;

	switch (msg->mode)
	{
		case BGRENDER_Color:
			set(obj, MA_BGRender_NeedsBackground, FALSE);
			set(data->grp_bgrender, MUIA_Group_ActivePage, 0);
			break;

		case BGRENDER_Centered:
		case BGRENDER_Scaled:
			set(obj, MA_BGRender_NeedsBackground, TRUE);
			set(data->grp_bgrender, MUIA_Group_ActivePage, 0);
			break;

		case BGRENDER_Stretched:
		case BGRENDER_Tiled:
		case BGRENDER_Zoomed:
			set(obj, MA_BGRender_NeedsBackground, TRUE);
			set(data->grp_bgrender, MUIA_Group_ActivePage, 1);
			break;
	
		#ifdef DEBUG
		default:
			PDB(("erk, value exceeded\n"));
			break;
		#endif
	}
	return (0);
}


DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_BGRender_Mode:
			*msg->opg_Storage = getv(data->cyc_bgrender, MUIA_Cycle_Active);
			return (TRUE);

		case MA_BGRender_Color:
			*msg->opg_Storage = getv(data->pp_color, MUIA_Pendisplay_Spec);
			return (TRUE);
		
		case MA_BGRender_NeedsBackground:
			*msg->opg_Storage = data->needsbg;
			return (TRUE);
	}
	return (DOSUPER);
}


DEFSET
{
	struct TagItem *ti;

	if ((ti = FindTagItem(MA_BGRender_NeedsBackground, INITTAGS)))
	{
		GETDATA;

		data->needsbg = ti->ti_Data;
	}
	return (DOSUPER);
}


BEGINMTABLE
DECNEW
DECGET
DECSET
DECSMETHOD(BGRender_Change)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, bgrenderclass)
