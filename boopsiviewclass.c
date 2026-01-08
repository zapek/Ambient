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
 * $Id: boopsiviewclass.c,v 1.8 2017/08/21 06:17:44 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <devices/rawkeycodes.h>
#include <intuition/icclass.h>
#include <graphics/text.h>
#include <graphics/gfxbase.h>
#include <proto/diskfont.h>
#include <proto/datatypes.h>
#include <proto/graphics.h>

/* private */
#include "boopsiview.h"
#include "mui_func.h"
#include "threads.h"
#include "methodstack.h"
#include "keymap.h"
#include "dos_internal.h"


struct dtobject {
	APTR dt;
	struct TextFont *font;
};


struct Data {
	struct dtobject *dtobj;
	ULONG aftershow;
	ULONG barsdone;
	APTR scrollerh;
	APTR scrollerv;
};

static void dto_delete(struct dtobject *dt)
{
	DisposeDTObject(dt->dt);
	CloseFont(dt->font);
	free(dt);
}


DEFNEW
{
	obj = DoSuperNew(cl, obj,
		MUIA_FillArea, FALSE,
		MUIA_CustomBackfill, TRUE,
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return ((ULONG)NULL);
	}
	return ((ULONG)obj);
}


DEFDISP
{
	GETDATA;

	if (data->dtobj)
	{
		dto_delete(data->dtobj);
	}
	return (DOSUPER);
}


DEFMMETHOD(AskMinMax)
{
	DOSUPER;

	msg->MinMaxInfo->MinWidth += 4; /* XXX: hm.. */
	msg->MinMaxInfo->MinHeight += 4;

	msg->MinMaxInfo->DefWidth = 640; /* XXX */
	msg->MinMaxInfo->DefHeight = 480;

	msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
	msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

	return (0);
}


DEFMMETHOD(Setup)
{
	ULONG rc;

	if ((rc = DOSUPER))
	{
		GETDATA;

		if (!data->barsdone)
		{
			data->scrollerv	= (APTR)getv(obj, MUIA_Scrollgroup_VertBar);
			data->scrollerh	= (APTR)getv(obj, MUIA_Scrollgroup_HorizBar);

			ASSERT(data->scrollerv);
			ASSERT(data->scrollerh);

			DoMethod(data->scrollerv, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime,
				obj, 3, MUIM_Set, DTA_TopVert, MUIV_TriggerValue
			);
			
			DoMethod(data->scrollerh, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime,
				obj, 3, MUIM_Set, DTA_TopHoriz, MUIV_TriggerValue
			);
			
			/*
			 * Setting notifies there doesn't work, so
			 * we update the scrollbars on datatype refresh.
			 */
			data->barsdone = TRUE;
		}
		MUI_RequestIDCMP(obj, IDCMP_IDCMPUPDATE | IDCMP_RAWKEY);
	}
	return (rc);
}


DEFMMETHOD(Cleanup)
{
	MUI_RejectIDCMP(obj, IDCMP_IDCMPUPDATE | IDCMP_RAWKEY);

	return (DOSUPER);
}


DEFTMETHOD(View_LoadURI)
{
	do_action(obj, TA_Boopsiview_Load,
		TT_Boopsiview_Load_Path, getv(obj, MA_View_Path),
	TAG_DONE);

	return (0);
}


DEFTMETHOD(Boopsiview_AddGadget)
{
	GETDATA;

	if (data->aftershow && data->dtobj)
	{
	    ASSERT(data->dtobj);
		ASSERT(_window(obj));

		SetAttrs(data->dtobj->dt,
			GA_Left, _mleft(obj),
			GA_Top, _mtop(obj),
			GA_RelWidth, - _mleft(obj) - (_window(obj)->Width - _mright(obj) - 1), /* datatypes only resize with REL gadgets (GM_LAYOUT) */
			GA_RelHeight, - _mtop(obj) - (_window(obj)->Height - _mbottom(obj) - 1),
		TAG_DONE);

		AddDTObject(_window(obj), NULL, data->dtobj->dt, -1);
	}
	return (0);
}


DEFTMETHOD(Boopsiview_RemoveGadget)
{
	GETDATA;

	ASSERT(_window(obj));

	if (data->dtobj)
	{
		RemoveDTObject(_window(obj), data->dtobj->dt);
	}
	return (0);
}


DEFSMETHOD(Boopsiview_AddDatatype)
{
	GETDATA;
	STRPTR dtname;

	if (data->dtobj)
	{
		dto_delete(data->dtobj);
	}
	data->dtobj = msg->dtobj;

	DoMethod(obj, MM_Boopsiview_RemoveGadget);
	DoMethod(obj, MM_Boopsiview_AddGadget);
	
	GetDTAttrs(data->dtobj->dt, DTA_ObjName, &dtname, TAG_DONE);
	
	DoSuperMethod(cl, obj, MM_View_SetStatus, MF_View_SetStatus_Window, "%s", dtname ? dtname : (STRPTR)getv(obj, MA_View_Path));

	MUI_Redraw(obj, MADF_DRAWOBJECT);

	return (0);
}


DEFMMETHOD(Show)
{
	ULONG rc;

	if ((rc = DOSUPER))
	{
		GETDATA;
		data->aftershow = TRUE;
	
		DoMethod(obj, MM_Boopsiview_AddGadget);
	}
	return (rc);
}


DEFMMETHOD(Hide)
{
	GETDATA;

	DoMethod(obj, MM_Boopsiview_RemoveGadget);

	data->aftershow = FALSE;

	return (DOSUPER);
}


DEFSET
{
	GETDATA;

	if (data->dtobj)
	{
		if (_window(obj)) /* XXX: clumsy.. */
		{
			SetGadgetAttrsA((struct Gadget *)data->dtobj->dt, _window(obj), NULL, INITTAGS);
		}
	}
	return (DOSUPER);
}


DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_View_NeedsBackfill:
			*msg->opg_Storage = TRUE;
			return (TRUE);

		case MA_View_HasBackground:
			*msg->opg_Storage = FALSE;
			return (TRUE);

		case MA_View_NewWin:
			*msg->opg_Storage = TRUE;
			return (TRUE);

		case MA_View_Type:
			*msg->opg_Storage = MV_View_Type_Boopsi;
			return (TRUE);
	}

	if (data->dtobj)
	{
		ULONG *store = ((struct opGet *)msg)->opg_Storage;
		ULONG rc     = GetAttr(((struct opGet *)msg)->opg_AttrID, data->dtobj->dt, store);
		
		if (rc)
		{
			return (rc);
		}
	}
	return (DOSUPER);
}

DEFMMETHOD(Draw)
{
	GETDATA;

	DOSUPER;

	if (data->dtobj)
	{
		RefreshDTObject(data->dtobj->dt, _window(obj), NULL, TAG_DONE);
	}
	return (0);
}


DEFMMETHOD(HandleInput)
{
	if (msg->imsg)
	{
		switch (msg->imsg->Class)
		{
			case IDCMP_IDCMPUPDATE:
				{
					FORTAG(msg->imsg->IAddress)
					{
						case DTA_Busy:
							/* XXX: used to turn busy pointer on or off but that shall be something else */
							break;

						case DTA_ErrorLevel:
							/* XXX: or should that be a requester ? */
							if (tag->ti_Data)
							{
								ULONG errnum = GetTagData(DTA_ErrorNumber, (ULONG)NULL, (struct TagItem *)msg->imsg->IAddress);
								STRPTR errstr = (STRPTR) GetTagData(DTA_ErrorString, (ULONG)"", (struct TagItem *)msg->imsg->IAddress);
								STRPTR s;

								if (errnum > 0 && errnum < DTERROR_UNKNOWN_DATATYPE)
								{
									s = DosGetString(errnum);

									DoSuperMethod(cl, obj, MM_View_SetStatus, MF_View_SetStatus_Window, "Datatype error: \"%s\" %s", errstr, (s && *s) ? s : (STRPTR)"unknown");
								}
							    else
								{
									if (errnum >= DTERROR_UNKNOWN_DATATYPE && errnum < DTMSG_TYPE_OFFSET)
									{
										s = GetDTString(errnum);

										DoSuperMethod(cl, obj, MM_View_SetStatus, MF_View_SetStatus_Window, "Datatype error: \"%s\" %s", errstr, (s && *s) ? s : (STRPTR)"unknown");
									}
								}
							}
							break;

						case DTA_Title:
							DoSuperMethod(cl, obj, MM_View_SetStatus, MF_View_SetStatus_Window, "%s", tag->ti_Data);
							break;

						case DTA_Sync:
							{
								GETDATA;
								if (data->dtobj)
								{
									SetAttrs(data->scrollerv, MUIA_Prop_First, getv(data->dtobj->dt, DTA_TopVert),
										MUIA_Prop_Entries, getv(data->dtobj->dt, DTA_TotalVert),
										MUIA_Prop_Visible, getv(data->dtobj->dt, DTA_VisibleVert),
										TAG_DONE
									);

									SetAttrs(data->scrollerh, MUIA_Prop_First, getv(data->dtobj->dt, DTA_TopHoriz),
										MUIA_Prop_Entries, getv(data->dtobj->dt, DTA_TotalHoriz),
										MUIA_Prop_Visible, getv(data->dtobj->dt, DTA_VisibleHoriz),
										TAG_DONE
									);
								}
							}
							MUI_Redraw(obj, MADF_DRAWOBJECT);
							break;
					}
					NEXTTAG
				}
				break;
			
			case IDCMP_RAWKEY:
				{
					switch (msg->imsg->Code)
					{
						case RAWKEY_UP:
							DoMethod(obj, MM_Boopsiview_SetPos, MV_Boopsiview_SetPos_TopDecrease, msg->imsg->Qualifier);
							break;

						case RAWKEY_DOWN:
							DoMethod(obj, MM_Boopsiview_SetPos, MV_Boopsiview_SetPos_TopIncrease, msg->imsg->Qualifier);
							break;

						case RAWKEY_PAGEUP:
							DoMethod(obj, MM_Boopsiview_SetPos, MV_Boopsiview_SetPos_TopDecrease, IEQUALIFIER_SHIFTS);
							break;

						case RAWKEY_PAGEDOWN:
							DoMethod(obj, MM_Boopsiview_SetPos, MV_Boopsiview_SetPos_TopIncrease, IEQUALIFIER_SHIFTS);
							break;

						case RAWKEY_HOME:
							DoMethod(obj, MM_Boopsiview_SetPos, MV_Boopsiview_SetPos_TopDecrease, IEQUALIFIER_CONTROLS);
							break;

						case RAWKEY_END:
							DoMethod(obj, MM_Boopsiview_SetPos, MV_Boopsiview_SetPos_TopIncrease, IEQUALIFIER_CONTROLS);
							break;

						case RAWKEY_LEFT:
							DoMethod(obj, MM_Boopsiview_SetPos, MV_Boopsiview_SetPos_LeftDecrease, msg->imsg->Qualifier);
							break;

						case RAWKEY_RIGHT:
							DoMethod(obj, MM_Boopsiview_SetPos, MV_Boopsiview_SetPos_LeftIncrease, msg->imsg->Qualifier);
							break;

						case NM_WHEEL_UP:
							if (_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
							{
								DoMethod(obj, MM_Boopsiview_SetPos, MV_Boopsiview_SetPos_TopDecrease, IEQUALIFIER_ALTS | msg->imsg->Qualifier);
							}
							break;

						case NM_WHEEL_DOWN:
							if (_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
							{
								DoMethod(obj, MM_Boopsiview_SetPos, MV_Boopsiview_SetPos_TopIncrease, IEQUALIFIER_ALTS | msg->imsg->Qualifier);
							}
							break;

						case RAWKEY_TAB:
							if (msg->imsg->Qualifier & IEQUALIFIER_SHIFTS) /* shift tab */
							{
								DoMethod(obj, MM_Boopsiview_Trigger, STM_PREV_FIELD);
							}
							else /* tab */
							{
								DoMethod(obj, MM_Boopsiview_Trigger, STM_NEXT_FIELD);
							}
							break;

						default: /* VANILLAKEY */
							switch (keymap_vanilla(msg->imsg))
							{
								case '/':
									DoMethod(obj, MM_Boopsiview_Trigger, STM_RETRACE);
									break;

								case 32: /* space */
									DoMethod(obj, MM_Boopsiview_SetPos, MV_Boopsiview_SetPos_TopIncrease, IEQUALIFIER_SHIFTS);
									break;

								case 13: /* return */
									DoMethod(obj, MM_Boopsiview_Trigger, STM_ACTIVATE_FIELD);
									break;

								case 8: /* backspace */
									DoMethod(obj, MM_Boopsiview_SetPos, MV_Boopsiview_SetPos_TopDecrease, IEQUALIFIER_SHIFTS);
									break;
							}
							break;
					}
				}
				break;
		}
	}
	return (DOSUPER);
}


DEFMMETHOD(Backfill)
{
	SetAPen(_rp(obj), _pens(obj)[MPEN_BACKGROUND]);

	RectFill(_rp(obj), msg->left, msg->top, msg->right, msg->bottom);
	
	return (0);
}


static LONG setqual(LONG val, ULONG quals, LONG dir, ULONG stride)
{
	if (quals & IEQUALIFIER_CONTROLS)
	{
		val += dir * 10000; /* begin/end */
	}
	else if (quals & IEQUALIFIER_SHIFTS)
	{
		val += dir * stride;
	}
	else if (quals & IEQUALIFIER_ALTS)
	{
		val += dir * 6;
	}
	else
	{
		val += dir * 3;
	}

	if (quals & IEQUALIFIER_REPEAT)
	{
		val += dir * 2;
	}
	return (val);
}


DEFSMETHOD(Boopsiview_SetPos)
{
	GETDATA;
	
	if (data->dtobj)
	{
		LONG val = val; /* shut up gcc */
		LONG visx, visy;
		LONG topx, topy;
		LONG totalx, totaly;
		
		GetDTAttrs(data->dtobj->dt,
			DTA_VisibleHoriz, &visx,
			DTA_VisibleVert, &visy,
			DTA_TopHoriz, &topx,
			DTA_TopVert, &topy,
			DTA_TotalHoriz, &totalx,
			DTA_TotalVert, &totaly,
		TAG_DONE);
		
		switch (msg->dir)
		{
			case MV_Boopsiview_SetPos_TopIncrease:
				val = setqual(topy, msg->quals, 1, visy);
				break;

			case MV_Boopsiview_SetPos_TopDecrease:
				val = setqual(topy, msg->quals, -1, visy);
				break;

			case MV_Boopsiview_SetPos_LeftIncrease:
				val = setqual(topx, msg->quals, 1, visx);
				break;

			case MV_Boopsiview_SetPos_LeftDecrease:
				val = setqual(topx, msg->quals, -1, visx);
				break;
		}

		if ((msg->dir == MV_Boopsiview_SetPos_TopIncrease) || (msg->dir == MV_Boopsiview_SetPos_TopDecrease))
		{
			if (data->scrollerv)
			{
				SetAttrs(data->scrollerv,
					MUIA_Prop_First, minmax(0, val, totaly - visy),
				TAG_DONE);
			}
		}
		else
		{
			if (data->scrollerh)
			{
				SetAttrs(data->scrollerh,
					MUIA_Prop_First, minmax(0, val, totalx - visx),
				TAG_DONE);
			}
		}
	}
	return (0);
}


DEFSMETHOD(Boopsiview_Trigger)
{
	GETDATA;
	struct dtTrigger dtt;

	dtt.MethodID = DTM_TRIGGER;
	dtt.dtt_GInfo = NULL;
	dtt.dtt_Function = msg->type;
	dtt.dtt_Data = NULL;

	DoDTMethodA(data->dtobj->dt, _window(obj), NULL, (Msg)&dtt);

	return (0);
}


BEGINMTABLE
DECNEW
DECDISP
DECSET
DECGET
DECMMETHOD(AskMinMax)
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(Show)
DECMMETHOD(Hide)
DECMMETHOD(Draw)
DECMMETHOD(HandleInput)
DECMMETHOD(Backfill)
DECSMETHOD(Boopsiview_SetPos)
DECSMETHOD(Boopsiview_AddDatatype)
DECTMETHOD(Boopsiview_AddGadget)
DECTMETHOD(Boopsiview_RemoveGadget)
DECSMETHOD(Boopsiview_Trigger)
DECTMETHOD(View_LoadURI)
ENDMTABLE

DECSUBCLASSPTR_NC(viewclass, boopsiviewclass)


static struct dtobject * dto_create(STRPTR path)
{
	struct dtobject *dt;

	if ((dt = malloc(sizeof(*dt))))
	{
		struct TextAttr ta;

		Forbid(); /* yeah, this sucks.. but the font could change anytime */
		ta.ta_Name  = GfxBase->DefaultFont->tf_Message.mn_Node.ln_Name;
		ta.ta_YSize = GfxBase->DefaultFont->tf_YSize;
		ta.ta_Style = GfxBase->DefaultFont->tf_Style;
		ta.ta_Flags = GfxBase->DefaultFont->tf_Flags;

		if ((dt->font = OpenDiskFont(&ta)))
		{
			Permit();
		}
		else
		{
			Permit();
			ta.ta_Name = "topaz.font";
			ta.ta_YSize = 8;
			ta.ta_Style = 0;
			ta.ta_Flags = 0;

			dt->font = OpenFont(&ta);
		}

		if (dt->font)
		{
			dt->dt = NewDTObject(path,
				GA_ID, 1000, /* multiview does that */
				DTA_SourceType, DTST_FILE,
				DTA_TextAttr, &ta,
				ICA_TARGET, ICTARGET_IDCMP,
			TAG_DONE);

			if (dt->dt)
			{
				return (dt);
			}
			CloseFont(dt->font);
		}
		free(dt);
	}
	return (NULL);
}




ULONG tr_loaddatatype(APTR obj, STRPTR path)
{
	struct dtobject	*dt;

	THREAD;

	if ((dt = dto_create(path)))
	{
		methodstack_push_sync(obj, 2, MM_Boopsiview_AddDatatype, dt);
	}
	return (FALSE);
}
