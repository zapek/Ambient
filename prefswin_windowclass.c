/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber
 * Copyright 2005-2016 Ambient Open Source Team
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
 * $Id: prefswin_windowclass.c,v 1.20 2025/11/17 22:18:14 tcheko Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "prefswin.h"
#include "prefsclone.h"
#include "toolbarclass.h"
#include "screen.h"

#ifndef MUIC_Popstring
#define MUIC_Popstring "Popstring.mui"
#endif


struct Data {
	APTR browsermode;
	APTR cyc_modes;
	APTR font;
	APTR lst_sbformat;
	APTR str_sbformat;
	APTR pop_sbformat;
	APTR fr_button;

	APTR toolbar;
	APTR grp_toolbar;

	APTR s_coords;

	LONG l_left;
	LONG l_top;
	LONG l_width;
	LONG l_height;

	APTR bt_getdim;
	APTR cb_mouse;
	APTR win_selectview;

	APTR cyc_views;
	APTR cyc_viewmodes;
	APTR ch_inherit_viewmode;
	
	APTR pop_sbcolor;
};

/* XXX: kiero: put them into statusbar. together with descriptions ids. */

static const char * const placeholders[] = {"%tf%", "%td%", "%i%", "%l%", "%du%", "%ns%", "%sdu%", NULL};

DEFNEW
{
	struct Data *data;
	static CONST_STRPTR displaymodes[4];
	static CONST_STRPTR placeholdersdesc[8];
	static STRPTR views[ 2 + MSG_PREFSWIN_TOOLBAR_VIEW_LIST - MSG_PREFSWIN_TOOLBAR_VIEW_ICON ];
	static STRPTR viewmodes[ 2 + MSG_VIEW_THUMB - MSG_VIEW_ICONS ];

	APTR cyc_modes;
	APTR toolbar;
	APTR ch_enabled, grp_toolbar;
	APTR s_coords;
	APTR cb_mouse;
	APTR bt_getdim;
	APTR fr_button;
	APTR cyc_views;
	APTR cyc_viewmodes;
	APTR ch_inherit_viewmode;
	APTR lst_sbformat;
	APTR str_sbformat;
	APTR pop_sbformat;
	APTR pop_sbcolor;

	placeholdersdesc[0] = GSI( MSG_PREFSWIN_TOOLBAR_STATUSBAR_FORMAT_0 );
	placeholdersdesc[1] = GSI( MSG_PREFSWIN_TOOLBAR_STATUSBAR_FORMAT_1 );
	placeholdersdesc[2] = GSI( MSG_PREFSWIN_TOOLBAR_STATUSBAR_FORMAT_2 );
	placeholdersdesc[3] = GSI( MSG_PREFSWIN_TOOLBAR_STATUSBAR_FORMAT_3 );
	placeholdersdesc[4] = GSI( MSG_PREFSWIN_TOOLBAR_STATUSBAR_FORMAT_4 );
	placeholdersdesc[5] = GSI( MSG_PREFSWIN_TOOLBAR_STATUSBAR_FORMAT_5 );
	placeholdersdesc[6] = GSI( MSG_PREFSWIN_TOOLBAR_STATUSBAR_FORMAT_6 );
	placeholdersdesc[7] = NULL;

	obj = DoSuperNew(cl, obj,

		Child, ScrollgroupObject,
			MUIA_Scrollgroup_FreeVert, TRUE,
			MUIA_Scrollgroup_AutoBars, TRUE,
			MUIA_Scrollgroup_Contents,
			VirtgroupObject,

				Child, HGroup,
					GroupFrame, MUIA_Background, MUII_GroupBack,
					Child, HSpace(0),
					Child, ch_enabled = MUICreateCheckbox( MSG_PREFSWIN_TOOLBAR_ENABLE, FALSE, NULL),
					Child, MUICreateLabel( MSG_PREFSWIN_TOOLBAR_ENABLE, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
					Child, HSpace(0),
				End,

				Child, grp_toolbar = VGroup,
					/* fixme: disabled for now, because it looks ugly. someone else should decide what happens with it */
					GroupFrameT(GSI(MSG_PREFSWIN_TOOLBAR_TOOLBUTTONS_GROUP)),
					Child, HGroup,
						Child, NSLabel1(MSG_PREFSWIN_TOOLBAR_DISPLAYMODE),
						Child, cyc_modes = MUICreateCycle( MSG_PREFSWIN_TOOLBAR_DISPLAYMODE, displaymodes, MSG_PREFSWIN_TOOLBAR_DISPLAYMODE_IMAGETEXT, MSG_PREFSWIN_TOOLBAR_DISPLAYMODE_IMAGE, NULL),
						Child, HSpace(0),
						Child, MUICreateLabel( MSG_PREFSWIN_TOOLBAR_BUTTONFRAME, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
						Child, fr_button = MUI_NewObject(MUIC_Popfrimage, 0x80426a55, TRUE,
							MUIA_Window_Title, GSI( MSG_PREFSWIN_TOOLBAR_BUTTONFRAME_WINTITLE ),
							MUIA_Framedisplay_Spec, NULL,
							MUIA_ShortHelp, GSI(MSG_PREFSWIN_TOOLBAR_BUTTONFRAME+1),
							MUIA_CycleChain, 1,
							MUIA_ControlChar, MUIGetUnderScore( MSG_PREFSWIN_TOOLBAR_BUTTONFRAME ),
							//MUIA_Imagedisplay_Spec, NULL,
							End,
					End,

					/* fixme: we should not use virtgroup but scrollgroup because we may run out of space */

					Child, ScrollgroupObject,
						GroupFrameT(GSI(MSG_PREFSWIN_TOOLBAR_CURRENT_GROUP)),
						MUIA_Scrollgroup_FreeVert, FALSE,
						MUIA_Scrollgroup_AutoBars, TRUE,
						MUIA_Scrollgroup_Contents, VirtgroupObject,
							MUIA_Dropable, TRUE,
							Child, toolbar = NewObject(gettoolbargroupclass(), NULL,
								MA_Toolbargroup_Definition, getprefsstr(DSI_TOOLBAR_DEFINITION),
								MA_Toolbargroup_Draggable, TRUE,
								MUIA_ShortHelp, GSI(MSG_PREFSWIN_TOOLBAR_CURRENT_HELP),
							End,
						End,
					End,

					Child, ScrollgroupObject,
						GroupFrameT(GSI(MSG_PREFSWIN_TOOLBAR_AVAILABLE_GROUP)),
						MUIA_Scrollgroup_AutoBars, TRUE,
						MUIA_Scrollgroup_Contents, VirtgroupObject,
							Child, NewObject(gettoolbargroupclass(), NULL,
								MA_Toolbargroup_Definition, MV_Toolbargroup_Definition_All,
								MA_Toolbargroup_Draggable, TRUE,
								MA_Toolbargroup_Source, TRUE,
								MUIA_ShortHelp, GSI( MSG_PREFSWIN_TOOLBAR_AVAILABLE_HELP),
							End,
							Child, VSpace(0),
						End,
					End,
				End,

				Child, VGroup, GroupFrameT(GSI(MSG_PREFSWIN_TOOLBAR_DEFAULTDIMENSIONS_GROUP)),
					GroupFrame, MUIA_Background, MUII_GroupBack,
					Child, HGroup,
						Child, cb_mouse = MUICreateCheckbox(MSG_PREFSWIN_TOOLBAR_PLACEUNDERMOUSE, 0, NULL),
						Child, MUICreateLabel( MSG_PREFSWIN_TOOLBAR_PLACEUNDERMOUSE, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
						Child, HSpace(0),
		            End,
					Child, HGroup,
						Child, bt_getdim = MUICreateButton(MSG_PREFSWIN_TOOLBAR_GETFROMWINDOW, NULL),
						Child, HSpace(10),
						Child, s_coords = MUICreateTextNoFrame(MSG_PREFSWIN_TOOLBAR_COORDS, NULL),
						Child, HSpace(0),
		            End,
				End,

				Child, VGroup, GroupFrameT(GSI(MSG_PREFSWIN_TOOLBAR_VIEW_GROUP)),
					GroupFrame, MUIA_Background, MUII_GroupBack,
					Child, HGroup,
						Child, NSLabel1(MSG_PREFSWIN_TOOLBAR_DEFAULT_VIEW),
						Child, cyc_views = MUICreateCycle( MSG_PREFSWIN_TOOLBAR_DEFAULT_VIEW, views, MSG_PREFSWIN_TOOLBAR_VIEW_ICON, MSG_PREFSWIN_TOOLBAR_VIEW_LIST, NULL),
						Child, NSLabel1(MSG_PREFSWIN_TOOLBAR_DEFAULT_VIEWMODE),
						Child, cyc_viewmodes = MUICreateCycle( MSG_PREFSWIN_TOOLBAR_DEFAULT_VIEWMODE, viewmodes, MSG_VIEW_ICONS, MSG_VIEW_THUMB, NULL),
						Child, ch_inherit_viewmode = MUICreateCheckbox( MSG_PREFSWIN_TOOLBAR_INHERIT_VIEWMODE, TRUE, NULL),
						Child, MUICreateLabel( MSG_PREFSWIN_TOOLBAR_INHERIT_VIEWMODE, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
					End,
				End,

				Child, HGroup, GroupFrameT( GSI( MSG_PREFSWIN_TOOLBAR_STATUSBAR ) ),
					Child, NSLabel2( MSG_PREFSWIN_TOOLBAR_STATUSBAR_FORMAT ), //MUICreateLabel( MSG_PREFSWIN_TOOLBAR_STATUSBAR_FORMAT, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
					Child, pop_sbformat = PopobjectObject,
						MUIA_Weight, 1000,
						MUIA_Popstring_String, str_sbformat = StringObject,
							StringFrame,
							MUIA_CycleChain, 1,
							MUIA_String_MaxLen, STATUSBARTEXTSIZE,
							MUIA_ControlChar, MUIGetUnderScore( MSG_PREFSWIN_TOOLBAR_STATUSBAR_FORMAT ),
							MUIA_ShortHelp, GSI( MSG_PREFSWIN_TOOLBAR_STATUSBAR_FORMAT_HELP ),
						End,
						MUIA_Popstring_Button, MUICreatePopButton( MSG_PREFSWIN_TOOLBAR_STATUSBAR_FORMAT, MUII_PopUp, NULL),
						MUIA_Popobject_Object, lst_sbformat = ListObject,
							InputListFrame,
							MUIA_List_SourceArray, placeholdersdesc,
						End,
					End,
					Child, NSLabel2( MSG_PREFSWIN_TOOLBAR_STATUSBAR_COLOR ), // bitRocky
					Child, pop_sbcolor = MUICreatePoppen( MSG_PREFSWIN_TOOLBAR_STATUSBAR_COLOR, "PREF_WIN_SBCOLOR"), // bitRocky
				End,
			End,
		End,
	End;

	if (!obj)
	{
		return (0);
	}

	data = INST_DATA(cl, obj);

	data->browsermode  = ch_enabled;
	data->cyc_modes    = cyc_modes;
	data->font         = NULL;
	data->toolbar      = toolbar;
	data->grp_toolbar  = grp_toolbar;
	data->fr_button    = fr_button;

	data->s_coords     = s_coords;
	data->cb_mouse     = cb_mouse;
	data->win_selectview = NULL;
	data->bt_getdim    = bt_getdim;
	data->cyc_viewmodes  = cyc_viewmodes;
	data->cyc_views      = cyc_views;
	data->ch_inherit_viewmode = ch_inherit_viewmode;
	data->lst_sbformat = lst_sbformat;
	data->str_sbformat = str_sbformat;
	data->pop_sbformat = pop_sbformat;
	data->pop_sbcolor  = pop_sbcolor;

	data->l_left   = getprefslong(DSI_WINDOW_DEFAULT_LEFT);
	data->l_top    = getprefslong(DSI_WINDOW_DEFAULT_TOP);
	data->l_width  = getprefslong(DSI_WINDOW_DEFAULT_WIDTH);
	data->l_height = getprefslong(DSI_WINDOW_DEFAULT_HEIGHT);

	DoMethod(obj, MUIM_MultiSet, MUIA_CycleChain, 1,
		ch_enabled, cyc_modes, NULL
	);

	setupprefs(cyc_modes, MUIA_Cycle_Active, getprefslong(DSI_TOOLBAR_DISPLAYMODE));
	setupprefs(ch_enabled, MUIA_Selected, getprefslong(DSI_TOOLBAR_BROWSERMODE));
	setupprefs_noset(toolbar, MA_Toolbargroup_Definition, MV_Toolbargroup_Definition_Updated);

	setupprefs_noset(fr_button, MUIA_Framedisplay_Spec, MUIV_EveryTime);
	setupprefs_noset(fr_button, MUIA_Imagedisplay_Spec, MUIV_EveryTime);

	setupprefs(str_sbformat, MUIA_String_Contents, (ULONG)getprefsstr(DSI_WINDOW_STATUSBARFORMAT));
	setupprefs(pop_sbcolor, MUIA_Pendisplay_Spec, (ULONG)getprefs(DSI_WINDOW_STATUSBARCOLOR)); // bitRocky

	setupprefs(cyc_views,     MUIA_Cycle_Active, getprefslong(DSI_WINDOW_DEFAULT_VIEW));
	setupprefs(cyc_viewmodes, MUIA_Cycle_Active, getprefslong(DSI_WINDOW_DEFAULT_VIEWMODE));
	setupprefs(ch_inherit_viewmode, MUIA_Selected, getprefslong(DSI_WINDOW_INHERIT_VIEWMODE));

    DoMethod(obj, MUIM_MultiSet, MUIA_Disabled, !getprefslong(DSI_TOOLBAR_BROWSERMODE),
		grp_toolbar, NULL
	);

	DoMethod(ch_enabled, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, obj, 6, MUIM_MultiSet, MUIA_Disabled, MUIV_NotTriggerValue,
		 grp_toolbar, NULL
	);

	DoMethod(bt_getdim, MUIM_Notify, MUIA_Pressed, FALSE,          obj,      1, MM_Prefswin_Window_GetSizeFromView);

	DoMethod(s_coords, MUIM_SetAsString, MUIA_Text_Contents, GSI(MSG_PREFSWIN_TOOLBAR_COORDS), max(data->l_left, 0), max(data->l_top, 0), data->l_width, data->l_height );

	set(cb_mouse, MUIA_Selected, (getprefslong(DSI_WINDOW_DEFAULT_LEFT) == -1));

	DoMethod(lst_sbformat, MUIM_Notify, MUIA_List_DoubleClick, TRUE,
		pop_sbformat, 2, MUIM_Popstring_Close, TRUE);

	DoMethod(lst_sbformat, MUIM_Notify, MUIA_List_DoubleClick, TRUE,
		obj, 1, MM_Prefswin_Window_AddStatusbarPlaceholder);


	return ((ULONG)obj);
}


DEFTMETHOD(Prefswin_Store)
{
	GETDATA;
	STRPTR toolbar_definition;
	STRPTR statusbar_format;

	if (getv(data->cb_mouse, MUIA_Selected))
	{
		setprefslong(DSI_WINDOW_DEFAULT_LEFT, -1);
		setprefslong(DSI_WINDOW_DEFAULT_TOP,  -1);
	}
	else
	{
		setprefslong(DSI_WINDOW_DEFAULT_LEFT, data->l_left);
		setprefslong(DSI_WINDOW_DEFAULT_TOP,  data->l_top);
	}

	setprefslong(DSI_WINDOW_DEFAULT_WIDTH,  data->l_width);
	setprefslong(DSI_WINDOW_DEFAULT_HEIGHT, data->l_height);

	toolbar_definition = (STRPTR)getv(data->toolbar, MA_Toolbargroup_Definition);
	statusbar_format = (STRPTR)getv(data->str_sbformat, MUIA_String_Contents);

	setprefslong(DSI_TOOLBAR_BROWSERMODE, getv(data->browsermode, MUIA_Selected));
	setprefslong(DSI_TOOLBAR_DISPLAYMODE, getv(data->cyc_modes, MUIA_Cycle_Active));
	setprefsstr(DSI_TOOLBAR_DEFINITION, toolbar_definition);
	setprefsstr(DSI_WINDOW_STATUSBARFORMAT, statusbar_format);
	setprefs(DSI_WINDOW_STATUSBARCOLOR, sizeof(struct MUI_PenSpec), (APTR)getv(data->pop_sbcolor, MUIA_Pendisplay_Spec)); // bitRocky

	setprefsstr(DSI_TOOLBAR_BUTTONFRAMESPEC, (STRPTR)getv(data->fr_button, MUIA_Framedisplay_Spec));
	setprefsstr(DSI_TOOLBAR_BUTTONIMAGESPEC, (STRPTR)getv(data->fr_button, MUIA_Imagedisplay_Spec));

	setprefslong(DSI_WINDOW_DEFAULT_VIEW,     getv(data->cyc_views,     MUIA_Cycle_Active));
	setprefslong(DSI_WINDOW_DEFAULT_VIEWMODE, getv(data->cyc_viewmodes, MUIA_Cycle_Active));
	setprefslong(DSI_WINDOW_INHERIT_VIEWMODE, getv(data->ch_inherit_viewmode, MUIA_Selected));

	return (0);
}

DEFTMETHOD(Prefswin_Window_GetSizeFromView)
{
	GETDATA;

	if (data->win_selectview == NULL)
	{
		data->win_selectview = NewObject(getviewselectwinclass(), NULL,
				MA_ViewSelector_Left,   getprefslong(DSI_WINDOW_DEFAULT_LEFT),
				MA_ViewSelector_Top,    getprefslong(DSI_WINDOW_DEFAULT_TOP),
				MA_ViewSelector_Width,  max(getprefslong(DSI_WINDOW_DEFAULT_WIDTH), 250),
				MA_ViewSelector_Height, max(getprefslong(DSI_WINDOW_DEFAULT_HEIGHT), 30),
				TAG_DONE
		);

		if (data->win_selectview != NULL)
		{
			DoMethod(app, OM_ADDMEMBER, data->win_selectview);
			DoMethod(data->win_selectview, MUIM_Notify, MUIA_Window_Open, FALSE, obj, 1, MM_Prefswin_Window_GetSizeFromViewAck);
        }

	}

	if (data->win_selectview != NULL)
	{
		set(_win(obj), MUIA_Window_Sleep, TRUE);
		set(data->win_selectview, MUIA_Window_Open, TRUE);
    }

	return (0);
}

DEFTMETHOD(Prefswin_Window_GetSizeFromViewAck)
{
	GETDATA;

	set(_win(obj), MUIA_Window_Sleep, FALSE);

	data->l_left   = getv(data->win_selectview, MA_ViewSelector_Left  );
	data->l_top    = getv(data->win_selectview, MA_ViewSelector_Top   );
	data->l_width  = getv(data->win_selectview, MA_ViewSelector_Width );
	data->l_height = getv(data->win_selectview, MA_ViewSelector_Height);

	DoMethod(data->s_coords, MUIM_SetAsString, MUIA_Text_Contents, GSI(MSG_PREFSWIN_TOOLBAR_COORDS), data->l_left, data->l_top, data->l_width, data->l_height);

	return (0);
}


DEFDISPOSE
{
	GETDATA;
	DoMethod(obj, MM_Prefswin_Store);

	if (data->win_selectview != NULL)
	{
		DoMethod(app, OM_REMMEMBER, data->win_selectview);
		MUI_DisposeObject(data->win_selectview);
    }

	return (DOSUPER);
}

DEFMMETHOD(Setup)
{
	ULONG rc = DOSUPER;
	GETDATA;

	LONG deffspec = _conf(toolbar_framespec)[0] != '!' ? FALSE : TRUE; /* just checking for strlen() doesn't work for this... */
	LONG defispec = _conf(toolbar_imagespec)[0] != '!' ? FALSE : TRUE; /* ... an empty string is a valid imagespec            */

	/*
	 *  XXX: Abuse other button for defaults. Better idea?
	 */
	SetAttrs(data->fr_button,
			MUIA_Group_Forward, FALSE,
			MUIA_Framedisplay_Spec, deffspec ? getv(data->bt_getdim, MUIA_Frame)                : (ULONG)_conf(toolbar_framespec),
			MUIA_Imagedisplay_Spec, defispec ? (ULONG)"6:2" /* took a while to figure out ;) */ : (ULONG)_conf(toolbar_imagespec), 
			TAG_DONE);

	return rc;
}

DEFTMETHOD(Prefswin_Window_AddStatusbarPlaceholder)
{
    GETDATA;

    STRPTR oldpat;
    char newpat[STATUSBARTEXTSIZE + 16];
    ULONG clicked, pos;

	DoMethod(data->pop_sbformat, MUIM_Popstring_Close, TRUE);

	if (get(data->str_sbformat, MUIA_String_Contents, &oldpat))
    {
		get(data->str_sbformat, MUIA_String_BufferPos, &pos);
		get(data->lst_sbformat, MUIA_List_Active, &clicked);

		if (clicked < sizeof(placeholders))
		{
			newpat[pos] = '\0';
			strncpy(newpat, oldpat, pos);
			strcat(newpat, placeholders[clicked]);
			strcat(newpat, oldpat + pos);
			set(data->str_sbformat, MUIA_String_Contents, newpat);
		}
	}

	return (0);
}

BEGINMTABLE
DECNEW
DECTMETHOD(Prefswin_Store)
DECTMETHOD(Prefswin_Window_GetSizeFromView)
DECTMETHOD(Prefswin_Window_GetSizeFromViewAck)
DECTMETHOD(Prefswin_Window_AddStatusbarPlaceholder)
DECDISPOSE
DECMMETHOD(Setup)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, prefswin_windowclass)
