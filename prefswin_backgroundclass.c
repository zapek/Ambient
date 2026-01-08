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
 * $Id: prefswin_backgroundclass.c,v 1.18 2019/12/18 19:13:44 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <libraries/asl.h>
#include <intuition/monitorclass.h>

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "prefswin.h"
#include "prefsclone.h"
#include "file_func.h"
#include "name.h"

struct Data {
	APTR fpanel_root;
	APTR str_window;
	APTR num_root_delay;
	APTR cc_root_mode;
	APTR cc_window_mode;
	APTR cb_transition;
};

DEFNEW
{
	struct Data *data;
	APTR fpanel_root, str_window;
	APTR cc_root_mode, cc_window_mode, ps_win, num_root_delay;
	APTR cb_transition;

	obj = DoSuperNew(cl, obj,

		Child, ScrollgroupObject,
			MUIA_Scrollgroup_FreeVert, TRUE,
			MUIA_Scrollgroup_AutoBars, TRUE,
			MUIA_Scrollgroup_Contents,
			VirtgroupObject,

				Child, VGroup, GroupFrameT(GSI(MSG_PREFSWIN_BACKGROUND_DESKTOP)),
					Child, fpanel_root = MUI_NewObject(MUIC_Filepanel,
						MUIA_ShortHelp, GSI(MSG_PREFSWIN_BACKGROUND_FILE_HELP),
						TAG_DONE),

					Child, VSpace(-1),

					Child, ColGroup(2),
						Child, NSLabel2(MSG_PREFSWIN_BACKGROUND_MODE),
						Child, cc_root_mode = NewObject(getbgrenderclass(), NULL,
							MA_BGRender_Label, GSI(MSG_PREFSWIN_BACKGROUND_MODE),
							MA_BGRender_Mode, getprefslong(DSI_BACKGROUND_ROOT_BGRENDER),
							MA_BGRender_Color, getprefs(DSI_BACKGROUND_ROOT_BGCOLOR),
							MA_BGRender_Root, TRUE,
							MUIA_ShortHelp, GSI(MSG_PREFSWIN_BACKGROUND_MODE_HELP),
						End,

						Child, NSLabel2(MSG_PREFSWIN_BACKGROUND_ROOT_REFRESH_DELAY),
						Child, num_root_delay = NewObject(getbgrefreshdelaysliderclass(), NULL,
													MUIA_Slider_Level, getprefslong(DSI_BACKGROUND_ROOT_REFRESH_DELAY),
													MUIA_ShortHelp, GSI(MSG_PREFSWIN_BACKGROUND_ROOT_REFRESH_DELAY_HELP),
													MUIA_ControlChar , MUIGetUnderScore( MSG_PREFSWIN_BACKGROUND_ROOT_REFRESH_DELAY ),
						End,

						Child, HSpace(-1),
						Child, HGroup,
							Child, cb_transition = MUICreateCheckbox(MSG_PREFSWIN_BACKGROUND_TRANSITION, getprefslong(DSI_BACKGROUND_TRANSITION), "PREF_BACKGROUND_TRANSITION"),
							Child, MUICreateLabel(MSG_PREFSWIN_BACKGROUND_TRANSITION, MUIO_Label_SingleFrame),
							Child, HSpace(0),
						End,
					End,
				End,

				Child, ColGroup(2), GroupFrameT(GSI(MSG_PREFSWIN_BACKGROUND_WINDOWS)),
					Child, NSLabel2(MSG_PREFSWIN_BACKGROUND_WINDOWS_MODE),
					Child, cc_window_mode = NewObject(getbgrenderclass(), NULL,
						MA_BGRender_Label, GSI(MSG_PREFSWIN_BACKGROUND_WINDOWS_MODE),
						MA_BGRender_Mode, getprefslong(DSI_BACKGROUND_WINDOW_BGRENDER),
						MA_BGRender_Color, getprefs(DSI_BACKGROUND_WINDOW_BGCOLOR),
						MUIA_ShortHelp, GSI(MSG_PREFSWIN_BACKGROUND_MODE_HELP),
					End,
					Child, NSLabel2(MSG_PREFSWIN_BACKGROUND_WINDOWS_FILE),
					Child, str_window =  PopaslObject,
						MUIA_Popasl_Type, ASL_FileRequest,
						MUIA_Popstring_String, ps_win = pstring(DSI_BACKGROUND_WINDOW, PATH_SIZE, GSI(MSG_PREFSWIN_BACKGROUND_WINDOWS_FILE)),
						MUIA_Popstring_Button, MUICreatePopButton( MSG_PREFSWIN_BACKGROUND_FILE, MUII_PopFile, NULL ),
						ASLFR_TitleText, GSI(MSG_PREFSWIN_BACKGROUND_REQUESTER),
						MUIA_ShortHelp, GSI(MSG_PREFSWIN_BACKGROUND_FILE_HELP),
					End,
				End,
			End,
		End,
	End;

	if (!obj)
	{
		return (IPTR)(NULL);
	}

	data = INST_DATA(cl, obj);
	data->fpanel_root = fpanel_root;
	data->cc_root_mode = cc_root_mode;
	data->num_root_delay = num_root_delay;
	data->str_window = str_window;
	data->cc_window_mode = cc_window_mode;
	data->cb_transition = cb_transition;

	setupprefs_noset(fpanel_root, MUIA_Panel_Terminate, MUIV_EveryTime);
	setupprefs_noset(ps_win, MUIA_String_Acknowledge, MUIV_EveryTime);

	set(data->fpanel_root, MUIA_Filepanel_Path, getprefsstr(DSI_BACKGROUND_ROOT));

	set(data->fpanel_root, MUIA_Disabled, !getv(data->cc_root_mode, MA_BGRender_NeedsBackground));

	DoMethod(data->cc_root_mode, MUIM_Notify, MA_BGRender_NeedsBackground, MUIV_EveryTime,
		data->fpanel_root, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue
	);

	DoMethod(data->cc_root_mode, MUIM_Notify, MA_BGRender_NeedsBackground, MUIV_EveryTime,
		data->num_root_delay, 1, MM_Prefswin_Background_Invalidate_Delay
	);
	DoMethod(data->fpanel_root, MUIM_Notify, MUIA_Panel_Terminate, MUIV_EveryTime,
		obj, 1, MM_Prefswin_Background_Invalidate_Delay
	);
	DoMethod(data->cc_root_mode, MUIM_Notify, MA_BGRender_NeedsBackground, MUIV_EveryTime,
		obj, 1, MM_Prefswin_Background_Invalidate_Delay
	);
	DoMethod( obj, MM_Prefswin_Background_Invalidate_Delay );

	set(data->str_window, MUIA_Disabled, !getv(data->cc_window_mode, MA_BGRender_NeedsBackground));
	DoMethod(data->cc_window_mode, MUIM_Notify, MA_BGRender_NeedsBackground, MUIV_EveryTime,
		data->str_window, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue
	);

	/* Are transitions even legal? */
	ULONG memory = 0; // of by default
	Object **monitors = GetMonitorList(NULL);
	if (monitors)
	{
		ULONG monitor = 0;
		memory = 0xffffffff;
		while (monitors[monitor])
		{
			ULONG ms = 0;
			get(monitors[monitor], MA_MemorySize, &ms);
			// limit by the lowest capable monitor since we don't know where ambient is going to be displayed here
			if (ms < memory)
				memory = ms;
			monitor++;
		}
		FreeMonitorList(monitors);
	}

	if (memory <= MINIMUM_TRANSITIONS_VMEM)
		set(data->cb_transition, MUIA_Disabled, TRUE);

	return ((ULONG)obj);
}


DEFTMETHOD(Prefswin_Store)
{
	GETDATA;
	STRPTR t;

	setprefsstr_ctx( cloneprefspool, DSI_BACKGROUND_ROOT, "" );
	if( ( t =  (STRPTR) getv( data->fpanel_root, MUIA_Filepanel_Path ) ) )
	{
		if( ( t = name_build_sysify( t ) ) )
		{
			// done like this because we can't give a limit to the filepanel
			if( strlen( t ) < PATH_SIZE )
			{
				setprefsstr_ctx( cloneprefspool, DSI_BACKGROUND_ROOT, t );
			}
			free( t );
		}
	}

	storepath( data->str_window, DSI_BACKGROUND_WINDOW );
	setprefslong(DSI_BACKGROUND_ROOT_BGRENDER, getv(data->cc_root_mode, MA_BGRender_Mode));
	setprefslong(DSI_BACKGROUND_WINDOW_BGRENDER, getv(data->cc_window_mode, MA_BGRender_Mode));
	setprefs(DSI_BACKGROUND_ROOT_BGCOLOR, sizeof(struct MUI_PenSpec), (APTR)getv(data->cc_root_mode, MA_BGRender_Color));
	setprefs(DSI_BACKGROUND_WINDOW_BGCOLOR, sizeof(struct MUI_PenSpec), (APTR)getv(data->cc_window_mode, MA_BGRender_Color));
	setprefslong(DSI_BACKGROUND_ROOT_REFRESH_DELAY, getv(data->num_root_delay, MUIA_Slider_Level) );
	setprefslong(DSI_BACKGROUND_TRANSITION, getv(data->cb_transition, MUIA_Selected));

	return (0);
}


DEFDISPOSE
{
	DoMethod(obj, MM_Prefswin_Store);

	return (DOSUPER);
}

DEFTMETHOD(Prefswin_Background_Invalidate_Delay)
{
	GETDATA;
	int state = !isdir((STRPTR) getv(data->fpanel_root, MUIA_Filepanel_Path)) || !getv(data->cc_root_mode, MA_BGRender_NeedsBackground);

	set(data->num_root_delay, MUIA_Disabled, state);
	set(data->cb_transition, MUIA_Disabled, state);

	return (0);
}

BEGINMTABLE
DECNEW
DECTMETHOD(Prefswin_Store)
DECTMETHOD(Prefswin_Background_Invalidate_Delay)
DECDISPOSE
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, prefswin_backgroundclass)
