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
 * $Id: prefswin_dragdropclass.c,v 1.7 2006/12/20 13:34:15 fab Exp $
 */

#include "ambient.h"

#if USE_DROP_EFFECT_PREFS

/* public */

/* private */
#include "mui_func.h"
#include "prefswin.h"
#include "prefsclone.h"
#include "ambient_cat.h"


struct Data {
	#if USE_SOLIDDRAG
	APTR cyc_mode;
	#endif
	APTR cc_drop;
};


DEFNEW
{
	struct Data *data;
	APTR cc_drop;
	
	#if USE_SOLIDDRAG
	APTR cyc_mode;
	static STRPTR displaymodes[4];

	displaymodes[0] = GSI(MSG_PREFSWIN_DRAGDROP_DISPLAYMODE_SOLID);
	displaymodes[1] = GSI(MSG_PREFSWIN_DRAGDROP_DISPLAYMODE_CHECKERED);
	displaymodes[2] = GSI(MSG_PREFSWIN_DRAGDROP_DISPLAYMODE_TRANSPARENT);
	#endif

	obj = DoSuperNew(cl, obj,
		Child, HGroup,
			Child, HSpace(0),

			Child, VGroup,

				Child, ColGroup(2), GroupFrameT(GSI(MSG_PREFSWIN_DRAGDROP_DISPLAY)),
					#if USE_SOLIDDRAG
					Child, NSLabel1(MSG_PREFSWIN_DRAGDROP_DRAGGING_EFFECT),
					Child, cyc_mode = MUI_MakeObject(MUIO_Cycle, GSI(MSG_PREFSWIN_DRAGDROP_DRAGGING_EFECT), displaymodes),
					#endif

					Child, NSLabel1(MSG_PREFSWIN_DRAGDROP_DROPPING_MARK),
					Child, cc_drop = NewObject(getdropeffectclass(), NULL,
						MA_DropEffect_Label, GSI(MSG_PREFSWIN_DRAGDROP_DROPPING_MARK),
						MA_DropEffect_Mode, getprefslong(DSI_DRAGDROP_DROPEFFECT),
						MA_DropEffect_TintVal, getprefs(DSI_DRAGDROP_TINTVAL),
						MA_DropEffect_BrightenVal, getprefslong(DSI_DRAGDROP_BRIGHTENVAL),
						MA_DropEffect_DarkenVal, getprefslong(DSI_DRAGDROP_DARKENVAL),
						MA_DropEffect_TintfadeVal, getprefs(DSI_DRAGDROP_TINTFADEVAL),
						MUIA_ShortHelp, GSI(MSG_PREFSWIN_DRAGDROP_DISPLAY_HELP),
					End,
				End,

			End,

			Child, HSpace(0),
		End,
	End;

	if (!obj)
	{
		return (0);
	}

	data = INST_DATA(cl, obj);

	data->cc_drop = cc_drop;

	#if USE_SOLIDDRAG
	data->cyc_mode = cyc_mode;
	set(cyc_mode, MUIA_Cycle_Active, getprefslong(DSI_DRAGDROP_DISPLAY));

	DoMethod(obj, MUIM_MultiSet, MUIA_CycleChain, 1,
		cyc_mode, NULL
	);
	#endif

	return ((ULONG)obj);
}


DEFTMETHOD(Prefswin_Store)
{
	GETDATA;

	#if USE_SOLIDDRAG
	storeattr(data->cyc_mode, MUIA_Cycle_Active, DSI_DRAGDROP_DISPLAY);
	#endif

	storeattr(data->cc_drop, MA_DropEffect_Mode, DSI_DRAGDROP_DROPEFFECT);
	setprefs(DSI_DRAGDROP_TINTVAL, sizeof(struct MUI_PenSpec), (APTR)getv(data->cc_drop, MA_DropEffect_TintVal));
	setprefs(DSI_DRAGDROP_TINTFADEVAL, sizeof(struct MUI_PenSpec), (APTR)getv(data->cc_drop, MA_DropEffect_TintfadeVal));
	storeattr(data->cc_drop, MA_DropEffect_BrightenVal, DSI_DRAGDROP_BRIGHTENVAL);
	storeattr(data->cc_drop, MA_DropEffect_DarkenVal, DSI_DRAGDROP_DARKENVAL);

	return (0);
}


DEFDISPOSE
{
	DoMethod(obj, MM_Prefswin_Store);

	return (DOSUPER);
}


BEGINMTABLE
DECNEW
DECTMETHOD(Prefswin_Store)
DECDISPOSE
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, prefswin_dragdropclass)

#endif
