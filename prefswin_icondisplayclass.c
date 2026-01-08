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
 * $Id: prefswin_icondisplayclass.c,v 1.14 2016/12/25 22:00:57 geit Exp $
 */

#include "ambient.h"

/* public */
#include <libraries/asl.h>
#include <proto/intuition.h>

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "prefswin.h"
#include "prefsclone.h"

static LONG lastpage;

struct Data {
	APTR grp_reg;
	APTR pop_lasso_color;
	APTR pop_root_font;
	APTR pop_root_font_small;
	APTR pop_root_color;
	APTR pop_root_color_back;
	APTR cyc_minsize;
	APTR cyc_maxsize;
	APTR cyc_root_effect;
	APTR cyc_window_effect;
	#if USE_SILLY_ICONPREFS
	APTR sl_root_linespace;
	APTR sl_window_linespace;
	#endif
	APTR pop_window_font;
	APTR pop_window_color;
	APTR pop_window_color_back;
	#if USE_DROP_EFFECT_PREFS
	APTR cc_select;
	#else
	APTR pop_tint_color;
	#endif
	APTR pop_dropmark_color;
	APTR bt_hover;
	APTR bt_defghosted;
	APTR bt_shortcutsidentifier;
	APTR bt_appiconsidentifier;
	APTR bt_dualpng;
	APTR bt_fade;
	APTR bt_autosnapshot;
	APTR bt_separatefiles;
	APTR cyc_driveinfo;
};


DEFNEW
{
	STATIC CONST_STRPTR drive_entries[2 + MSG_PREFSWIN_ICONDISPLAY_DRIVE_TEXTGAUGE - MSG_PREFSWIN_ICONDISPLAY_DRIVE_NONE];
	struct Data *data;
	APTR grp_reg;
	APTR pop_lasso_color, pop_root_font, pop_root_font_small, pop_window_font, pop_root_color, pop_window_color, pop_root_color_back, pop_window_color_back;
	APTR pop_dropmark_color, fontstring_root, fontstring_root_small, fontstring_window;
	APTR bt_hover, bt_separatefiles, cyc_driveinfo;
	APTR bt_defghosted;
	APTR bt_shortcutsidentifier;
	APTR bt_appiconsidentifier;
	APTR bt_dualpng;
	APTR bt_fade;
	APTR bt_autosnapshot;
	APTR cyc_minsize, cyc_maxsize;
	
	#if USE_DROP_EFFECT_PREFS
	APTR cc_select;
	#else
	APTR pop_tint_color;
	#endif

	APTR cyc_root_effect, cyc_window_effect;

	#if USE_SILLY_ICONPREFS
	APTR sl_root_linespace, sl_window_linespace;
	static STRPTR cycleopts[ 2 + MSG_PREFSWIN_ICONDISPLAY_TEXT_MODE_GRADIENT - MSG_PREFSWIN_ICONDISPLAY_TEXT_MODE_NORMAL ];
    #else
	static STRPTR cycleopts[ 2 + MSG_PREFSWIN_ICONDISPLAY_TEXT_MODE_OUTLINE - MSG_PREFSWIN_ICONDISPLAY_TEXT_MODE_NORMAL ];
	#endif
	static STRPTR regtitle[2 + MSG_PREFSWIN_ICONDISPLAY_REGTITLE_WINDOW - MSG_PREFSWIN_ICONDISPLAY_REGTITLE_DESKTOP ];
	static STRPTR cyclesizeopts[ 2 + MSG_PREFSWIN_ICONDISPLAY_RESIZE_HUGE - MSG_PREFSWIN_ICONDISPLAY_RESIZE_MICRO ];

    MUIInitStringArray( (APTR) regtitle, MSG_PREFSWIN_ICONDISPLAY_REGTITLE_DESKTOP, MSG_PREFSWIN_ICONDISPLAY_REGTITLE_WINDOW);

	obj = DoSuperNew(cl, obj,
		Child,  ScrollgroupObject,
			MUIA_Scrollgroup_FreeVert, TRUE,
			MUIA_Scrollgroup_AutoBars, TRUE,
			MUIA_Scrollgroup_Contents,
			VirtgroupObject,
		
			Child, VSpace(0),
			
			Child, VGroup, GroupFrameT(GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION)),
				#if USE_DROP_EFFECT_PREFS
				Child, NSLabel1(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT),
				Child, cc_select = NewObject(getdropeffectclass(), NULL,
					MA_DropEffect_Label, GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT),
					MA_DropEffect_Mode, getprefslong(DSI_ICON_SELECTEFFECT),
					MA_DropEffect_TintVal, getprefs(DSI_ICON_TINTVAL),
					MA_DropEffect_BrightenVal, getprefslong(DSI_ICON_BRIGHTENVAL),
					MA_DropEffect_DarkenVal, getprefslong(DSI_ICON_DARKENVAL),
					MA_DropEffect_TintfadeVal, getprefs(DSI_ICON_TINTFADEVAL),
					MUIA_ShortHelp, GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTEFFECT_HELP),
				End,
				#endif
				Child, HGroup,
					#if !USE_DROP_EFFECT_PREFS
					Child, NSLabel2(MSG_PREFSWIN_ICONDISPLAY_SELECT),
					Child, pop_tint_color = MUICreatePoppen( MSG_PREFSWIN_ICONDISPLAY_SELECT, "PREF_ICON_SELECT"),
					Child, HSpace(0),
					#endif

					Child, NSLabel2(MSG_PREFSWIN_ICONDISPLAY_LASSO),
					Child, pop_lasso_color = MUICreatePoppen( MSG_PREFSWIN_ICONDISPLAY_LASSO, "PREF_ICON_LASSO"),
					Child, HSpace(0),

					Child, NSLabel2(MSG_PREFSWIN_ICONDISPLAY_DROPMARK),
					Child, pop_dropmark_color = MUICreatePoppen( MSG_PREFSWIN_ICONDISPLAY_DROPMARK, "PREF_ICON_DROPMARK"),
				End,
			End,

			Child, VSpace(0),

			Child, VGroup, GroupFrameT(GSI(MSG_PREFSWIN_ICONDISPLAY_RESIZE)),
				Child, HGroup,
					Child, NSLabel1(MSG_PREFSWIN_ICONDISPLAY_RESIZE_MINSIZE),
					Child, cyc_minsize = MUICreateCycle( MSG_PREFSWIN_ICONDISPLAY_RESIZE_MINSIZE, cyclesizeopts, MSG_PREFSWIN_ICONDISPLAY_RESIZE_MICRO, MSG_PREFSWIN_ICONDISPLAY_RESIZE_HUGE, "PREF_ICON_RESIZEMIN"),
					Child, NSLabel1(MSG_PREFSWIN_ICONDISPLAY_RESIZE_MAXSIZE),
					Child, cyc_maxsize = MUICreateCycle( MSG_PREFSWIN_ICONDISPLAY_RESIZE_MAXSIZE, cyclesizeopts, MSG_PREFSWIN_ICONDISPLAY_RESIZE_MICRO, MSG_PREFSWIN_ICONDISPLAY_RESIZE_HUGE, "PREF_ICON_RESIZEMAX"),
				End,
			End,
			
			Child, VSpace(0),

			Child, VGroup, GroupFrameT(GSI(MSG_PREFSWIN_ICONDISPLAY_EXTRA_FEATURES)),
				Child, HGroup,
					Child, ColGroup(3),
						Child, bt_hover = MUICreateCheckbox( MSG_PREFSWIN_ICONDISPLAY_MOUSEHOVER, FALSE, "PREF_ICONDISPLAY_MOUSEHOVER"),
						Child, MUICreateLabel(MSG_PREFSWIN_ICONDISPLAY_MOUSEHOVER, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
						Child, HSpace(0),
						Child, bt_separatefiles = MUICreateCheckbox( MSG_PREFSWIN_ICONDISPLAY_SEPARATEFILES, TRUE, "PREF_ICONDISPLAY_SEPARATEFILES"),
						Child, MUICreateLabel(MSG_PREFSWIN_ICONDISPLAY_SEPARATEFILES, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
						Child, HSpace(0),
						Child, bt_fade = MUICreateCheckbox( MSG_PREFSWIN_ICONDISPLAY_FADEICONSONSTARTUP, TRUE, "PREF_ICONDISPLAY_FADEICONSONSTARTUP"),
						Child, MUICreateLabel(MSG_PREFSWIN_ICONDISPLAY_FADEICONSONSTARTUP, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
						Child, HSpace(0),
						Child, bt_defghosted = MUICreateCheckbox( MSG_PREFSWIN_ICONDISPLAY_DEFAULTICONSTRANSPARENT, TRUE, "PREF_ICONDISPLAY_DEFAULTICONTRANSPARENT"),
						Child, MUICreateLabel(MSG_PREFSWIN_ICONDISPLAY_DEFAULTICONSTRANSPARENT, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
						Child, HSpace(0),
					End,

					Child, HSpace(0),

					Child, ColGroup(3),
						Child, bt_appiconsidentifier = MUICreateCheckbox( MSG_PREFSWIN_ICONDISPLAY_APPICONIDENTIFIER, TRUE, "PREF_ICONDISPLAY_APPICONIDENTIFIER"),
						Child, MUICreateLabel(MSG_PREFSWIN_ICONDISPLAY_APPICONIDENTIFIER, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
						Child, HSpace(0),
						Child, bt_shortcutsidentifier = MUICreateCheckbox( MSG_PREFSWIN_ICONDISPLAY_SHORTCUTIDENTIFIER, TRUE, "PREF_ICONDISPLAY_SHORTCUTIDENTIFIER"),
						Child, MUICreateLabel(MSG_PREFSWIN_ICONDISPLAY_SHORTCUTIDENTIFIER, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
						Child, HSpace(0),
						Child, bt_autosnapshot = MUICreateCheckbox( MSG_PREFSWIN_ICONDISPLAY_AUTOMATICSNAPSHOT, TRUE, "PREF_ICONDISPLAY_AUTOMATICSNAPSHOT"),
						Child, MUICreateLabel(MSG_PREFSWIN_ICONDISPLAY_AUTOMATICSNAPSHOT, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
						Child, HSpace(0),
						Child, bt_dualpng = MUICreateCheckbox( MSG_PREFSWIN_ICONDISPLAY_TWOSTATEPNGICONS, TRUE, "PREF_ICONDISPLAY_TWOSTATEPNGICONS"),
						Child, MUICreateLabel(MSG_PREFSWIN_ICONDISPLAY_TWOSTATEPNGICONS, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
						Child, HSpace(0),
					End,
				End,

				Child, HGroup,
					Child, NSLabel2(MSG_PREFSWIN_ICONDISPLAY_DRIVEINFORMATION),
					Child, cyc_driveinfo = MUICreateCycle( MSG_PREFSWIN_ICONDISPLAY_DRIVEINFORMATION, &drive_entries, MSG_PREFSWIN_ICONDISPLAY_DRIVE_NONE, MSG_PREFSWIN_ICONDISPLAY_DRIVE_TEXTGAUGE, "PREF_ICONDISPLAY_DRIVEINFO"),
					Child, HSpace(0),
				End,

			End,

			Child, VGroup, GroupFrameT(GSI(MSG_PREFSWIN_ICONDISPLAY_TEXT)),
				Child, grp_reg = RegisterGroup(regtitle),
					MUIA_CycleChain, TRUE,
					MUIA_Group_ActivePage, lastpage,
					Child, VGroup,
						Child, ColGroup(2),
							Child, NSLabel2(MSG_PREFSWIN_ICONDISPLAY_TEXT_MODE),
	                        #if USE_SILLY_ICONPREFS
							Child, cyc_root_effect = MUICreateCycle( MSG_PREFSWIN_ICONDISPLAY_TEXT_MODE, cycleopts, MSG_PREFSWIN_ICONDISPLAY_TEXT_MODE_NORMAL, MSG_PREFSWIN_ICONDISPLAY_TEXT_MODE_GRADIENT, "PREF_ICON_DESKTEXTMODE"),
	        				#else
							Child, cyc_root_effect = MUICreateCycle( MSG_PREFSWIN_ICONDISPLAY_TEXT_MODE, cycleopts, MSG_PREFSWIN_ICONDISPLAY_TEXT_MODE_NORMAL, MSG_PREFSWIN_ICONDISPLAY_TEXT_MODE_OUTLINE , "PREF_ICON_DESKTEXTMODE"),
	                        #endif
							Child, NSLabel2(MSG_PREFSWIN_ICONDISPLAY_TEXT_FONT),
							Child, pop_root_font = PopaslObject,
								MUIA_ShortHelp, GSI( MSG_PREFSWIN_ICONDISPLAY_TEXT_FONT_HELP ),
								MUIA_Popasl_Type, ASL_FontRequest,
								MUIA_Popstring_String, fontstring_root = pstring(DSI_FONT_ROOT, PATH_SIZE, GSI(MSG_PREFSWIN_ICONDISPLAY_TEXT_FONT)),
								MUIA_Popstring_Button, MUICreatePopButton( MSG_PREFSWIN_ICONDISPLAY_TEXT_FONT, MUII_PopFont, NULL),
								ASLFO_TitleText, GSI(MSG_PREFSWIN_ICONDISPLAY_TEXT_FONT_REQ),
								ASLFO_DoStyle, TRUE,
							End,

							Child, NSLabel2(MSG_PREFSWIN_ICONDISPLAY_TEXT_SMALLFONT),
							Child, pop_root_font_small = PopaslObject,
								MUIA_ShortHelp, GSI( MSG_PREFSWIN_ICONDISPLAY_TEXT_SMALLFONT_HELP ),
								MUIA_Popasl_Type, ASL_FontRequest,
								MUIA_Popstring_String, fontstring_root_small = pstring(DSI_FONT_ROOT_SMALL, PATH_SIZE, GSI(MSG_PREFSWIN_ICONDISPLAY_TEXT_SMALLFONT)),
								MUIA_Popstring_Button, MUICreatePopButton( MSG_PREFSWIN_ICONDISPLAY_TEXT_SMALLFONT, MUII_PopFont, NULL),
								ASLFO_TitleText, GSI(MSG_PREFSWIN_ICONDISPLAY_TEXT_SMALLFONT_REQ),
								ASLFO_DoStyle, TRUE,
							End,

							Child, NSLabel2(MSG_PREFSWIN_ICONDISPLAY_DESKTEXTCOLOR),
							Child, HGroup,
								Child, pop_root_color = MUICreatePoppen( MSG_PREFSWIN_ICONDISPLAY_DESKTEXTCOLOR, "PREF_ICON_DESKTEXTCOLOR"),
								Child, pop_root_color_back = MUICreatePoppen( MSG_PREFSWIN_ICONDISPLAY_DESKTEXTBACK, "PREF_ICON_DESKTEXTBACK"),
							End,
							#if USE_SILLY_ICONPREFS
							Child, NSLabel1( MSG_PREFSWIN_ICONDISPLAY_TEXT_LABELSPACE ),
							Child, sl_root_linespace = MUI_MakeObject(MUIO_Slider, GSI(MSG_PREFSWIN_ICONDISPLAY_TEXT_LABELSPACE), 0, 7),
							#endif
						End,
					End,

					Child, VGroup,
						Child, ColGroup(2),
							Child, NSLabel2(MSG_PREFSWIN_ICONDISPLAY_TEXT_MODE),
							#if USE_SILLY_ICONPREFS
							Child, cyc_window_effect = MUICreateCycle( MSG_PREFSWIN_ICONDISPLAY_TEXT_MODE, cycleopts, MSG_PREFSWIN_ICONDISPLAY_TEXT_MODE_NORMAL, MSG_PREFSWIN_ICONDISPLAY_TEXT_MODE_GRADIENT, "PREF_ICON_WINTEXTMODE"),
							#else
							Child, cyc_window_effect = MUICreateCycle( MSG_PREFSWIN_ICONDISPLAY_TEXT_MODE, cycleopts, MSG_PREFSWIN_ICONDISPLAY_TEXT_MODE_NORMAL, MSG_PREFSWIN_ICONDISPLAY_TEXT_MODE_OUTLINE , "PREF_ICON_WINTEXTMODE"),
							#endif
							


							Child, NSLabel2(MSG_PREFSWIN_ICONDISPLAY_WINTEXTFONT),
							Child, pop_window_font = PopaslObject,
								MUIA_ShortHelp, GSI( MSG_PREFSWIN_ICONDISPLAY_WINTEXTFONT_HELP ),
								MUIA_Popasl_Type, ASL_FontRequest,
								MUIA_Popstring_String, fontstring_window = pstring(DSI_FONT_WINDOW, PATH_SIZE, GSI(MSG_PREFSWIN_ICONDISPLAY_TEXT_FONT)),
								MUIA_Popstring_Button, MUICreatePopButton( MSG_PREFSWIN_ICONDISPLAY_WINTEXTFONT, MUII_PopFont, NULL),
								ASLFO_TitleText, GSI(MSG_PREFSWIN_ICONDISPLAY_WINTEXTFONT_REQ),
								ASLFO_DoStyle, TRUE,
							End,
						
							Child, NSLabel2(MSG_PREFSWIN_ICONDISPLAY_WINTEXTCOLOR),
							Child, HGroup,
								Child, pop_window_color = MUICreatePoppen( MSG_PREFSWIN_ICONDISPLAY_WINTEXTCOLOR, "PREF_ICON_WINTEXTCOLOR"),
								Child, pop_window_color_back = MUICreatePoppen( MSG_PREFSWIN_ICONDISPLAY_WINTEXTBACK, "PREF_ICON_WINTEXTBACK"),
							End,
							#if USE_SILLY_ICONPREFS
							Child, NSLabel1(MSG_PREFSWIN_ICONDISPLAY_TEXT_LABELSPACE),
							Child, sl_window_linespace = MUI_MakeObject(MUIO_Slider, GSI(MSG_PREFSWIN_ICONDISPLAY_TEXT_LABELSPACE), 0, 7),
							#endif
						End,
					End,
				End,
			End,

			Child, VSpace(0),
			End,
		End,
	End;

	if (!obj)
	{
		return (0);
	}

	data = INST_DATA(cl, obj);
	data->grp_reg = grp_reg;
	data->pop_lasso_color = pop_lasso_color;
	data->pop_root_font = pop_root_font;
	data->pop_root_font_small = pop_root_font_small;
	data->pop_window_font = pop_window_font;
	data->cyc_minsize = cyc_minsize;
	data->cyc_maxsize = cyc_maxsize;
	data->cyc_root_effect = cyc_root_effect;
	data->cyc_window_effect = cyc_window_effect;
	#if USE_SILLY_ICONPREFS
	data->sl_root_linespace = sl_root_linespace;
	data->sl_window_linespace = sl_window_linespace;
	#endif
	data->pop_root_color = pop_root_color;
	data->pop_window_color = pop_window_color;
	data->pop_root_color_back = pop_root_color_back;
	data->pop_window_color_back = pop_window_color_back;
	#if USE_DROP_EFFECT_PREFS
	data->cc_select = cc_select;
	#else
	data->pop_tint_color = pop_tint_color;
	setupprefs(pop_tint_color, MUIA_Pendisplay_Spec, (ULONG) getprefs(DSI_ICON_TINTVAL));
	#endif
	
	data->pop_dropmark_color = pop_dropmark_color;
	data->bt_hover = bt_hover;
	data->bt_fade = bt_fade;
	data->bt_defghosted = bt_defghosted;
	data->bt_dualpng = bt_dualpng;
	data->bt_autosnapshot = bt_autosnapshot;
	data->bt_appiconsidentifier = bt_appiconsidentifier;
	data->bt_shortcutsidentifier = bt_shortcutsidentifier;

	data->bt_separatefiles = bt_separatefiles;
	data->cyc_driveinfo = cyc_driveinfo;

	#if USE_SILLY_ICONPREFS
	DoMethod(obj, MUIM_MultiSet, MUIA_CycleChain, 1,
		cyc_root_effect, cyc_window_effect, sl_root_linespace, sl_window_linespace, NULL
	);
	#else
	DoMethod(obj, MUIM_MultiSet, MUIA_CycleChain, 1,
		cyc_root_effect, cyc_window_effect, NULL
	);
	#endif

	setupprefs(pop_lasso_color, MUIA_Pendisplay_Spec, (ULONG) getprefs(DSI_COLOR_LASSO));
	setupprefs(pop_dropmark_color, MUIA_Pendisplay_Spec, (ULONG) getprefs(DSI_DRAGDROP_TINTVAL));
	setupprefs_noset(fontstring_window, MUIA_String_Acknowledge, MUIV_EveryTime);
	setupprefs_noset(fontstring_root, MUIA_String_Acknowledge, MUIV_EveryTime);
	setupprefs_noset(fontstring_root_small, MUIA_String_Acknowledge, MUIV_EveryTime);
	setupprefs(pop_root_color, MUIA_Pendisplay_Spec, (ULONG) getprefs(DSI_COLOR_ROOT));
	setupprefs(pop_root_color_back, MUIA_Pendisplay_Spec, (ULONG) getprefs(DSI_COLOR_ROOT2));
	setupprefs(pop_window_color, MUIA_Pendisplay_Spec, (ULONG) getprefs(DSI_COLOR_WINDOW));
	setupprefs(pop_window_color_back, MUIA_Pendisplay_Spec, (ULONG) getprefs(DSI_COLOR_WINDOW2));

	setupprefs(cyc_window_effect, MUIA_Cycle_Active, getprefslong(DSI_FONT_WINDOW_EFFECT));
    setupprefs(cyc_root_effect, MUIA_Cycle_Active, getprefslong(DSI_FONT_ROOT_EFFECT));

	setupprefs(cyc_minsize, MUIA_Cycle_Active, getprefslong(DSI_ICON_MINSIZE));
	setupprefs(cyc_maxsize, MUIA_Cycle_Active, getprefslong(DSI_ICON_MAXSIZE));

	#if USE_SILLY_ICONPREFS
	setupprefs(sl_root_linespace, MUIA_Numeric_Value, getprefslong(DSI_FONT_ROOTSPACE));
	set(sl_root_linespace, MUIA_ShortHelp, GSI(MSG_PREFSWIN_ICONDISPLAY_TEXT_LABELSPACE_HELP));
	setupprefs(sl_window_linespace, MUIA_Numeric_Value, getprefslong(DSI_FONT_WINDOWSPACE));
	set(sl_window_linespace, MUIA_ShortHelp, GSI(MSG_PREFSWIN_ICONDISPLAY_TEXT_LABELSPACE_HELP));
	#endif

	setupprefs(bt_hover, MUIA_Selected, getprefslong(DSI_ICON_HOVER));
	setupprefs(bt_defghosted, MUIA_Selected, getprefslong(DSI_ICON_DEFGHOSTED));
	setupprefs(bt_dualpng, MUIA_Selected, getprefslong(DSI_ICON_DUALPNG));
	setupprefs(bt_fade, MUIA_Selected, getprefslong(DSI_ICON_FADE));
	setupprefs(bt_autosnapshot, MUIA_Selected, getprefslong(DSI_ICON_AUTOSNAPSHOT));
	setupprefs(bt_appiconsidentifier, MUIA_Selected, getprefslong(DSI_ICON_APPICONSIDENTIFIER));
	setupprefs(bt_shortcutsidentifier, MUIA_Selected, getprefslong(DSI_ICON_SHORTCUTSIDENTIFIER));
	setupprefs(bt_separatefiles, MUIA_Selected, getprefslong(DSI_ICON_SEPARATEFILES));
	setupprefs(cyc_driveinfo, MUIA_Cycle_Active, getprefslong(DSI_MISC_DRIVE_INFO));

	DoMethod(cyc_minsize, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
		obj, 2, MM_Prefswin_Icondisplay_AdjustSize, MV_Prefswin_Icondisplay_AdjustSize_Min
	);
	DoMethod(cyc_maxsize, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
		obj, 2, MM_Prefswin_Icondisplay_AdjustSize, MV_Prefswin_Icondisplay_AdjustSize_Max
	);

	return ((ULONG)obj);
}


DEFTMETHOD(Prefswin_Store)
{
	GETDATA;

	#if USE_DROP_EFFECT_PREFS
	storeattr(data->cc_select, MA_DropEffect_Mode, DSI_ICON_SELECTEFFECT);
	setprefs(DSI_ICON_TINTVAL, sizeof(struct MUI_PenSpec), (APTR)getv(data->cc_select, MA_DropEffect_TintVal));
	setprefs(DSI_ICON_TINTFADEVAL, sizeof(struct MUI_PenSpec), (APTR)getv(data->cc_select, MA_DropEffect_TintfadeVal));
	storeattr(data->cc_select, MA_DropEffect_BrightenVal, DSI_ICON_BRIGHTENVAL);
	storeattr(data->cc_select, MA_DropEffect_DarkenVal, DSI_ICON_DARKENVAL);
	#else
	setprefs(DSI_ICON_TINTVAL, sizeof(struct MUI_PenSpec), (APTR)getv(data->pop_tint_color, MUIA_Pendisplay_Spec));
	#endif

	setprefs(DSI_DRAGDROP_TINTVAL, sizeof(struct MUI_PenSpec), (APTR)getv(data->pop_dropmark_color, MUIA_Pendisplay_Spec));

	setprefs(DSI_COLOR_LASSO, sizeof(struct MUI_PenSpec), (APTR)getv(data->pop_lasso_color, MUIA_Pendisplay_Spec));

	setprefs(DSI_COLOR_ROOT, sizeof(struct MUI_PenSpec), (APTR)getv(data->pop_root_color, MUIA_Pendisplay_Spec));
	setprefs(DSI_COLOR_WINDOW, sizeof(struct MUI_PenSpec), (APTR)getv(data->pop_window_color, MUIA_Pendisplay_Spec));
	
	setprefs(DSI_COLOR_ROOT2, sizeof(struct MUI_PenSpec), (APTR)getv(data->pop_root_color_back, MUIA_Pendisplay_Spec));
	setprefs(DSI_COLOR_WINDOW2, sizeof(struct MUI_PenSpec), (APTR)getv(data->pop_window_color_back, MUIA_Pendisplay_Spec));
	
	setprefslong(DSI_ICON_MINSIZE, getv(data->cyc_minsize, MUIA_Cycle_Active));
	setprefslong(DSI_ICON_MAXSIZE, getv(data->cyc_maxsize, MUIA_Cycle_Active));

	setprefslong(DSI_FONT_ROOT_EFFECT, getv(data->cyc_root_effect, MUIA_Cycle_Active));
	setprefslong(DSI_FONT_WINDOW_EFFECT, getv(data->cyc_window_effect, MUIA_Cycle_Active));

	#if USE_SILLY_ICONPREFS
	storeattr(data->sl_root_linespace, MUIA_Numeric_Value, DSI_FONT_ROOTSPACE);
	storeattr(data->sl_window_linespace, MUIA_Numeric_Value, DSI_FONT_WINDOWSPACE);	       
	#endif

	storestring(data->pop_root_font, DSI_FONT_ROOT);
	storestring(data->pop_root_font_small, DSI_FONT_ROOT_SMALL);
	storestring(data->pop_window_font, DSI_FONT_WINDOW);
	setprefslong(DSI_ICON_HOVER, getv(data->bt_hover, MUIA_Selected));
	setprefslong(DSI_ICON_DEFGHOSTED, getv(data->bt_defghosted, MUIA_Selected));
	setprefslong(DSI_ICON_DUALPNG, getv(data->bt_dualpng, MUIA_Selected));
	setprefslong(DSI_ICON_FADE, getv(data->bt_fade, MUIA_Selected));
	setprefslong(DSI_ICON_SHORTCUTSIDENTIFIER, getv(data->bt_shortcutsidentifier, MUIA_Selected));
	setprefslong(DSI_ICON_APPICONSIDENTIFIER, getv(data->bt_appiconsidentifier, MUIA_Selected));
	setprefslong(DSI_ICON_AUTOSNAPSHOT, getv(data->bt_autosnapshot, MUIA_Selected));
	setprefslong(DSI_ICON_SEPARATEFILES, getv(data->bt_separatefiles, MUIA_Selected));
	setprefslong(DSI_MISC_DRIVE_INFO, getv(data->cyc_driveinfo, MUIA_Cycle_Active));

	#if 0
	{
		TEXT buf[34];

		snprintf(buf, 32, "%s", getv(data->pop_root_color, MUIA_Pendisplay_Spec));
		dprintf("%s\n", buf);
	}
	#endif

	lastpage = getv(data->grp_reg, MUIA_Group_ActivePage);

	return (0);
}


DEFSMETHOD(Prefswin_Icondisplay_AdjustSize)
{
	GETDATA;
	ULONG minval;
	ULONG maxval;

	minval = getv(data->cyc_minsize, MUIA_Cycle_Active);
	maxval = getv(data->cyc_maxsize, MUIA_Cycle_Active);

	if (msg->sizemode == MV_Prefswin_Icondisplay_AdjustSize_Min)
	{
		if (minval > maxval )
		{
			nnset(data->cyc_maxsize, MUIA_Cycle_Active, minval);
		}
	}
	else
	{
		if (maxval < minval )
		{
			nnset(data->cyc_minsize, MUIA_Cycle_Active, maxval);
		}
	}
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
DECSMETHOD(Prefswin_Icondisplay_AdjustSize)
DECDISPOSE
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, prefswin_icondisplayclass)
