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
 * $Id: soundwinclass.c,v 1.10 2017/12/03 14:48:55 bitrocky Exp $
 */

#include "ambient.h"

/* public */

#include <proto/intuition.h>
#include <proto/graphics.h>
#include <intuition/screenbar.h>
#include <intuition/screens.h>

/* private */
#include "methodstack.h"
#include "mui_func.h"
#include "ambient_cat.h"
#include "command.h"
#include "rexx.h"
#include "screen.h"
#include "soundwin.h"
#include "playsound.h"


static LONG added = FALSE;
static TEXT title[ 127 ];
static APTR pluginobj; /* XXX: make it protected with semaphore? */
static LONG update;

struct Data {
	APTR bt_pause;
	APTR bt_stop;
	APTR ar_scroller;
	APTR hb_bubble;
	struct MUI_InputHandlerNode ihnode;
	LONG scrollpos;
	LONG doscroll;
	LONG textwidth;
	LONG textheight;
	TEXT text[127];
	TEXT help[127+32];
	LONG bubbletimeout;
	LONG update;
};


static Object * makedeckbutton(void)
{
	Object *o;

	o = RectangleObject,
			MUIA_Frame, MUIV_Frame_None,
			MUIA_InnerLeft, 0,
			MUIA_InnerRight, 0,
			MUIA_InnerTop, 0,
			MUIA_InnerBottom, 0,
			MUIA_InputMode, MUIV_InputMode_RelVerify,
			MUIA_FixWidth, 10,
			MUIA_FixHeight, 10,
			MUIA_FillArea, FALSE,
		End;
	return (o);
}


DEFNEW
{
	struct Data *data;
	APTR bt_pause, bt_stop, ar_scroller;
	
	pluginobj = NULL;

	obj = DoSuperNew(cl, obj,
			MUIA_Group_Horiz, FALSE,
			MUIA_Group_VertSpacing, 0,
			MUIA_Group_HorizSpacing, 0,
			MUIA_Frame, MUIV_Frame_None,
			InnerSpacing(0,0),
			Child, VSpace(0),
			Child, HGroup,
				InnerSpacing(0,0),
				MUIA_Group_VertSpacing, 0,
				MUIA_Group_HorizSpacing, 2,
				MUIA_Frame, MUIV_Frame_None,
				Child, bt_pause = makedeckbutton(),
				Child, bt_stop = makedeckbutton(),
				Child, ar_scroller = RectangleObject,
					MUIA_FillArea, FALSE,
					InnerSpacing(0,0),
					End,
				End,
			Child, VSpace(0),
		End;

	if (obj != NULL)
	{
		data = INST_DATA(cl, obj);
		data->bt_pause = bt_pause;
		data->bt_stop = bt_stop;
		data->ar_scroller = ar_scroller;

		DoMethod(data->bt_pause, MUIM_Notify, MUIA_Pressed, FALSE,
			obj, 1, MM_Soundwin_Play
		);

		DoMethod(data->bt_stop, MUIM_Notify, MUIA_Pressed, FALSE,
			obj, 1, MM_Soundwin_Stop
		);
	}

	pluginobj = obj;
	return ((ULONG)obj);
}

DEFDISP
{
	pluginobj = NULL;
	return DOSUPER;
}


DEFGET
{
	switch (msg->opg_AttrID)
	{
		case MA_Window_ID:
			*msg->opg_Storage = 0;
			return (TRUE);

		case MA_Window_Path:
			*msg->opg_Storage = MV_Window_Path_None;
			return (TRUE);

		case MA_Window_Type:
			*msg->opg_Storage = MV_Window_Type_Sound;
			return (TRUE);
	}
	return (DOSUPER);
}


DEFTMETHOD(Soundwin_Play)
{
	methodstack_push(app, 2, MM_Application_SoundControl, MV_Application_SoundControl_Pause);
	return (0);
}


DEFTMETHOD(Soundwin_Stop)
{
	methodstack_push(app, 2, MM_Application_SoundControl, MV_Application_SoundControl_Stop);
	return (0);
}


DEFTMETHOD(Soundwin_Close)
{
	soundwin_stop();
	return (0);
}

DEFTMETHOD(Soundwin_Scroll)
{
	GETDATA;

	if (update || data->update)
	{
		/*
		 * New title is set, so we recalculate and resetup object.
		 */

        struct TextExtent te;

		stccpy(data->text, title, sizeof(data->text));
		TextExtent(_rp(obj), data->text, strlen(data->text), &te);
		data->textheight = _font(obj)->tf_YSize;//te.te_Extent.MaxY - te.te_Extent.MinY + 1;
		data->textwidth = te.te_Extent.MaxX - te.te_Extent.MinX + 1 + 10; /* 10 is for spacing */
		data->scrollpos = 0;

		update = FALSE; data->update = FALSE;
		DoMethod(_parent(obj), MUIM_Group_InitChange);
		DoMethod(_parent(obj), MUIM_Group_ExitChange2, TRUE);

		/*
		 * Help bubble to focus on title change,
		 */

		if (data->hb_bubble != NULL)
			DoMethod(obj, MUIM_DeleteBubble, data->hb_bubble);

		snprintf(data->help, sizeof(data->help), "Now playing:\n%s", data->text);
		data->hb_bubble = (APTR)DoMethod(obj, MUIM_CreateBubble, _left(obj), 0, data->help, MUIV_CreateBubble_DontHidePointer);
		data->bubbletimeout = 50;

	}
	else if (data->doscroll)
	{
		data->scrollpos += 1;
		MUI_Redraw(obj, MADF_DRAWOBJECT);
	}

	data->bubbletimeout--;
	if (data->bubbletimeout <= 0)
	{
		if (data->hb_bubble != NULL)
			DoMethod(obj, MUIM_DeleteBubble, data->hb_bubble);

		data->hb_bubble = NULL;
	}

	return 0;
}

DEFMMETHOD(AskMinMax)
{
	GETDATA;
	ULONG rc = DOSUPER;
	struct MUI_MinMax *mm = msg->MinMaxInfo;

	mm->MinWidth = mm->DefWidth = mm->MaxWidth = min(200, data->textwidth + 40);
	mm->MinHeight = mm->DefHeight = mm->MaxHeight = _screen(obj)->BarHeight + 1;

	/* here we can decide if we want scrolling or not */

	if (mm->MinWidth == 200)
		data->doscroll = TRUE;
	else
		data->doscroll = FALSE;

	return rc;
}

DEFMMETHOD(Draw)
{
	GETDATA;
	ULONG rc = DOSUPER;

	if (msg->flags & (MADF_DRAWOBJECT|MADF_DRAWUPDATE))
	{
		/* XXX: This really should be split into 3 classes, but for now lets pretend it's OK */

		struct TextExtent te;
		struct RastPort rp = *_rp(obj);
		LONG height = _screen(obj)->BarHeight + 1;
		LONG width = _width(data->ar_scroller);
		LONG textlen = strlen(data->text);
		LONG numchars;
		LONG off;
		APTR handle;
		struct TextFont *sbar_font;
		ULONG sbar_pen;
		ULONG sbar_font_baseline;

		/*
		 * Get screenbar font and pen to be skin compliant.
		 */

		sbar_font = (struct TextFont*)getv(_screen(obj), SA_ScreenbarTextFont);
		sbar_pen = getv(_screen(obj), SA_ScreenbarTextPen);
		sbar_font_baseline = getv(_screen(obj), SA_ScreenbarTextYPos);
		
		SetFont(&rp, sbar_font);
		SetAPen(&rp, sbar_pen);

		/*
		 * Scroller.
		 */

		off = data->scrollpos % data->textwidth;

		if (data->textwidth < width)
			off = 0;

		Move(&rp, _left(data->ar_scroller) - off, sbar_font_baseline);

		numchars = TextFit(&rp, data->text, textlen, &te, NULL, 1, width + 10 + off, height);

		DoMethod(obj, MUIM_DrawBackground, _left(data->ar_scroller), _top(data->ar_scroller), _width(data->ar_scroller), _height(data->ar_scroller), 0, 0, 0);

		handle = MUI_AddClipping(muiRenderInfo(obj), _mleft(data->ar_scroller), _mtop(obj), _mwidth(data->ar_scroller), _mheight(obj));
		Text(&rp, data->text, numchars);

		if (data->textwidth > width && data->textwidth - off < width )
		{
			LONG newoff = -off + data->textwidth;
			Move(&rp, _left(data->ar_scroller) + newoff, sbar_font_baseline);
			numchars = TextFit(&rp, data->text, textlen, &te, NULL, 1, width + 10 - newoff, height);
			if (numchars > 0)
				Text(&rp, data->text, numchars);
		}

		MUI_RemoveClipping(muiRenderInfo(obj), handle);

		/*
		 * Controls.
		 */

		RectFill(&rp, _left(data->bt_stop) + 2, _top(data->bt_stop) + 2, _right(data->bt_stop) - 2, _bottom(data->bt_stop) - 2);

		RectFill(&rp, _left(data->bt_pause) + 2, _top(data->bt_pause) + 2, _left(data->bt_pause) + 4, _bottom(data->bt_pause) - 2);
		RectFill(&rp, _left(data->bt_pause) + 7, _top(data->bt_pause) + 2, _left(data->bt_pause) + 9, _bottom(data->bt_pause) - 2);
	}

	return rc;
}

DEFMMETHOD(Setup)
{
	GETDATA;
	LONG rc = DOSUPER;

    data->ihnode.ihn_Object = obj;
	data->ihnode.ihn_Flags = MUIIHNF_TIMER;
	data->ihnode.ihn_Millis = 1000 / 10;
	data->ihnode.ihn_Method = MM_Soundwin_Scroll;

	DoMethod(_app(obj), MUIM_Application_AddInputHandler, &data->ihnode);

	update = TRUE; data->update = TRUE;

	return rc;
}

DEFMMETHOD(Cleanup)
{
	GETDATA;

	DoMethod(_app(obj), MUIM_Application_RemInputHandler, &data->ihnode);

	if (data->hb_bubble != NULL)
		DoMethod(obj, MUIM_DeleteBubble, data->hb_bubble);

	data->hb_bubble = NULL;

	return DOSUPER;
}

/* XXX: Hack to not get tons of warnings in a log.  */
#undef MAINTASK
#define MAINTASK

BEGINMTABLE
DECNEW
DECDISP
DECGET
DECMMETHOD(Draw)
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(AskMinMax)
DECTMETHOD(Soundwin_Play)
DECTMETHOD(Soundwin_Stop)
DECTMETHOD(Soundwin_Close)
DECTMETHOD(Soundwin_Scroll)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, soundwinclass)

ULONG soundwin_init(void)
{
	return TRUE;
}

void soundwin_cleanup(void)
{
	if (added)
	{
		ScreenbarControl(SBCT_UninstallPlugin,(ULONG)mccsoundwinclass,TAG_DONE);
		added = FALSE;
	}
}

void soundwin_play(void)
{
	if (!added)
	{
		mccsoundwinclass->mcc_Class->cl_ID = "Ambient_SoundPlayer.sbar";
		ScreenbarControl(SBCT_InstallPlugin,(ULONG)mccsoundwinclass,TAG_DONE);
		added = TRUE;
	}
}

void soundwin_stop(void)
{
	if (added)
	{
		ScreenbarControl(SBCT_UninstallPlugin,(ULONG)mccsoundwinclass,TAG_DONE);
		added = FALSE;
	}
}

void soundwin_pause(void)
{
}

void soundwin_setattr(ULONG attr, ULONG value)
{
	LONG rfr = FALSE;

	switch(attr)
	{
		case SOUNDWINTAG_TITLE:
			if (value != 0)
				stccpy(title, (STRPTR)value, sizeof(title));
			else
				title[ 0 ] = 0;
			rfr = TRUE;
			break;
	}

	if (rfr)
	{
		update = TRUE;
	}
}
