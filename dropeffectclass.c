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
 * $Id: dropeffectclass.c,v 1.8 2006/12/20 13:34:14 fab Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "ambient_cat.h"
#include "mui_func.h"

struct Data {
	APTR cyc_effect;
	APTR grp_effect;
	APTR pp_tint;
	APTR nb_brighten;
	APTR nb_darken;
	APTR pp_tintfade;
};


const struct MUI_PenSpec deftint = {
	"r00000000,00000000,00000000"
};

DEFNEW
{
	struct Data *data;
	APTR cyc_effect, grp_effect, nb_brighten, nb_darken, pp_tint, pp_tintfade;
	static STRPTR effects[9];

	effects[0] = GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_TINT);
	effects[1] = GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_BLUR);
	effects[2] = GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_BRIGHTEN);
	effects[3] = GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_DARKEN);
	effects[4] = GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_GREY);
	effects[5] = GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_NEGATIVE);
	effects[6] = GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_NEGATIVE_FADE);
	effects[7] = GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_TINT_FADE);

	obj = DoSuperNew(cl, obj,
		Child, HGroup,
			
			Child, cyc_effect = MUI_MakeObject(MUIO_Cycle, GetTagData(MA_DropEffect_Label, NULL, INITTAGS), effects),

			Child, grp_effect = PageGroup,
				
				/* POP_TINT */
				Child, HGroup,
					Child, NSLabel2(MSG_PREFSWIN_ICONDISPLAY_DESKTEXTCOLOR),
					Child, pp_tint = PoppenObject,
						MUIA_CycleChain, 1,
						MUIA_Window_Title, GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_TINT_REQ),
						MUIA_Pendisplay_Spec, GetTagData(MA_DropEffect_TintVal, (ULONG)&deftint, INITTAGS),
					End,
				End,

				/* POP_BLUR */
				Child, HVSpace,

				/* POP_BRIGHTEN */
				Child, HGroup,
					Child, NSLabel2(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_DELTA),
					Child, nb_brighten = NumericbuttonObject,
						MUIA_CycleChain, 1,
						MUIA_Numeric_Value, GetTagData(MA_DropEffect_BrightenVal, 64, INITTAGS),
						MUIA_Numeric_Max, 255,
					End,
				End,

				/* POP_DARKEN */
				Child, HGroup,
					Child, NSLabel2(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_DELTA),
					Child, nb_darken = NumericbuttonObject,
						MUIA_CycleChain, 1,
						MUIA_Numeric_Value, GetTagData(MA_DropEffect_DarkenVal, 64, INITTAGS),
						MUIA_Numeric_Max, 255,
					End,
				End,
			
				/* POP_GREY */
				Child, HVSpace,

				/* POP_NEGATIVE */
				Child, HVSpace,

				/* POP_NEGFADE */
				Child, HVSpace,

				/* POP_TINTFADE */
				Child, HGroup,
					Child, NSLabel2(MSG_PREFSWIN_ICONDISPLAY_DESKTEXTCOLOR),
					Child, pp_tintfade = PoppenObject,
						MUIA_CycleChain, 1,
						MUIA_Window_Title, GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_TINT_FADE_REQ),
						MUIA_Pendisplay_Spec, GetTagData(MA_DropEffect_TintfadeVal, (ULONG)&deftint, INITTAGS),
					End,
				End,
			
			End,
		
		End,
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return (NULL);
	}

	data = INST_DATA(cl, obj);
	data->cyc_effect = cyc_effect;
	data->grp_effect = grp_effect;
	data->pp_tint = pp_tint;
	data->nb_brighten = nb_brighten;
	data->nb_darken = nb_darken;
	data->pp_tintfade = pp_tintfade;

	SetAttrs(cyc_effect, MUIA_Cycle_Active, GetTagData(MA_DropEffect_Mode, 0, INITTAGS), MUIA_CycleChain, 1, TAG_DONE);

	set(data->grp_effect, MUIA_Group_ActivePage, getv(data->cyc_effect, MUIA_Cycle_Active));

	DoMethod(data->cyc_effect, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
		data->grp_effect, 3, MUIM_Set, MUIA_Group_ActivePage, MUIV_TriggerValue
	);

	return ((ULONG)obj);
}


DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_DropEffect_Mode:
			*msg->opg_Storage = getv(data->cyc_effect, MUIA_Cycle_Active);
			return (TRUE);

		case MA_DropEffect_TintVal:
			*msg->opg_Storage = getv(data->pp_tint, MUIA_Pendisplay_Spec);
			return (TRUE);

		case MA_DropEffect_BrightenVal:
			*msg->opg_Storage = getv(data->nb_brighten, MUIA_Numeric_Value);
			return (TRUE);

		case MA_DropEffect_DarkenVal:
			*msg->opg_Storage = getv(data->nb_darken, MUIA_Numeric_Value);
			return (TRUE);

		case MA_DropEffect_TintfadeVal:
			*msg->opg_Storage = getv(data->pp_tintfade, MUIA_Pendisplay_Spec);
			return (TRUE);
	}
	return (DOSUPER);
}


BEGINMTABLE
DECNEW
DECGET
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, dropeffectclass)
