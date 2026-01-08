/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
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
 * $Id: prefswin_listerclass.c,v 1.17 2016/12/25 22:00:57 geit Exp $
 */

#include "ambient.h"

/* public */
#include <libraries/asl.h>

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "prefswin.h"
#include "prefsclone.h"
#include "columnslistclass.h"
#include "listviewclass.h"

/*static LONG lastpage;*/

struct Data {
	APTR flp_file;
	APTR flp_directory;
	APTR flp_file_sel;
	APTR flp_directory_sel;
	APTR flp_softlink;
	APTR flp_hardlink;
	APTR flp_volume;
	APTR flp_assign;
	APTR flp_source;
	APTR flp_destination;
	APTR flp_column;
	APTR flc_background;
	APTR flc_alternated_rows;
	APTR flc_hilighted_sorting_column;
	APTR flc_bold_directories;
	APTR flpop_font;

	APTR fll_used_columns_files;
	APTR flc_iconsize_files;
	APTR flc_modes;
	APTR flc_compactsize;
	APTR flc_selection_modes;
	
	APTR fll_used_columns_devices;
	APTR flc_iconsize_devices;
	APTR flc_displayassigns;
};

static void store_listview_format(APTR obj, struct Data * data, ULONG type);

DEFNEW
{
	struct Data *data;

	APTR flp_file, flp_directory, flp_file_sel, flp_directory_sel, flp_softlink,
		 flp_hardlink, flp_volume, flp_assign, flp_column, flc_background,
		 fll_used_columns_files, flc_modes, flc_iconsize_files, flc_compactsize, flc_selection_modes,
		 fll_used_columns_devices, flc_iconsize_devices, flc_displayassigns,
		 flc_alternated_rows, flc_hilighted_sorting_column, flpop_font, flc_bold_directories, fls_font;

	int i=0;
	struct listview_format lv_format;
	struct column_entry ** columns;
	static STRPTR cyclemodes[ 2 + MSG_VIEW_THUMB - MSG_VIEW_ICONS ];
	static STRPTR cyclesizes[ 2 + MSG_PREFSWIN_LISTER_ICON_HUGE - MSG_PREFSWIN_LISTER_ICON_SMALL ];
	static STRPTR cyclesizes2[ 2 + MSG_PREFSWIN_LISTER_ICON_HUGE - MSG_PREFSWIN_LISTER_ICON_SMALL ];
	static STRPTR cycleselectionmodes[ 2 + MSG_PREFSWIN_LISTER_SELECTION_ALWAYS - MSG_PREFSWIN_LISTER_SELECTION_DEFAULT ];

	obj = DoSuperNew(cl, obj,
		
		Child,  ScrollgroupObject,
			MUIA_Scrollgroup_FreeVert, TRUE,
			MUIA_Scrollgroup_AutoBars, TRUE,
			MUIA_Scrollgroup_Contents,
			VirtgroupObject,

				Child, HGroup,
					Child, VGroup, GroupFrameT( GSI(MSG_PREFSWIN_LISTER_FILES)),
						Child, fll_used_columns_files = NewObject(getcolumnslistclass(), NULL, MUIA_CycleChain, TRUE, TAG_DONE),
						Child, ColGroup(2),
							Child, NSLabel2(MSG_PREFSWIN_LISTER_ICONSIZEFILES),
							Child, flc_iconsize_files = MUICreateCycle( MSG_PREFSWIN_LISTER_ICONSIZEFILES, cyclesizes, MSG_PREFSWIN_LISTER_ICON_SMALL, MSG_PREFSWIN_LISTER_ICON_HUGE, "PREF_LISTER_ICONSIZES_FILES"),
							Child, NSLabel2(MSG_PREFSWIN_LISTER_MODES),
							Child, flc_modes = MUICreateCycle( MSG_PREFSWIN_LISTER_MODES, cyclemodes, MSG_VIEW_ICONS, MSG_VIEW_THUMB, "PREF_LISTER_CYCLE_MODES"),
							Child, NSLabel2(MSG_PREFSWIN_LISTER_SELECTION_MODES),
							Child, flc_selection_modes = MUICreateCycle( MSG_PREFSWIN_LISTER_SELECTION_MODES, cycleselectionmodes, MSG_PREFSWIN_LISTER_SELECTION_DEFAULT, MSG_PREFSWIN_LISTER_SELECTION_ALWAYS, "PREF_LISTER_CYCLE_SELECTION_MODES"),
							Child, HSpace(0),
							Child, HGroup,
								Child, flc_compactsize = MUICreateCheckbox( MSG_PREFSWIN_LISTER_COMPACT_SIZE, FALSE, "PREF_LISTER_COMPACTSIZE"),
								Child, MUICreateLabel( MSG_PREFSWIN_LISTER_COMPACT_SIZE, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
								Child, HSpace(0),
							End,
						End,
					End,
					Child, VGroup, GroupFrameT(GSI(MSG_PREFSWIN_LISTER_DEVICES)),
						Child, fll_used_columns_devices = NewObject(getcolumnslistclass(), NULL, MUIA_CycleChain, TRUE, TAG_DONE),
						Child, ColGroup(2),
							Child, NSLabel2(MSG_PREFSWIN_LISTER_ICONSIZEDEVICES),
							Child, flc_iconsize_devices = MUICreateCycle( MSG_PREFSWIN_LISTER_ICONSIZEDEVICES, cyclesizes2, MSG_PREFSWIN_LISTER_ICON_SMALL, MSG_PREFSWIN_LISTER_ICON_HUGE, "PREF_LISTER_ICONSIZES_DEVICES"),
							Child, HSpace(0),
							Child, HGroup,
								Child, flc_displayassigns = MUICreateCheckbox( MSG_PREFSWIN_LISTER_DISPLAY_ASSIGNS, FALSE, "PREF_LISTER_ASSIGNS"),
								Child, MUICreateLabel( MSG_PREFSWIN_LISTER_DISPLAY_ASSIGNS, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
								Child, HSpace(0),
							End,
							Child, Label2(" "),
							Child, HSpace(0),
							End,
						End,
					End,

				Child, VGroup, GroupFrameT(GSI(MSG_PREFSWIN_LISTER_COLORS)),
					Child, HVSpace,
					Child, ColGroup(4),

						Child, NSLabel2(MSG_PREFSWIN_LISTER_FILE),
						Child, flp_file = MUICreatePoppen( MSG_PREFSWIN_LISTER_FILE, "PREF_LISTER_FILE"),

						Child, NSLabel2(MSG_PREFSWIN_LISTER_DIRECTORY),
						Child, flp_directory = MUICreatePoppen( MSG_PREFSWIN_LISTER_DIRECTORY, "PREF_LISTER_DIRECTORY"),

						Child, NSLabel2(MSG_PREFSWIN_LISTER_SELECTED_FILE),
						Child, flp_file_sel = MUICreatePoppen( MSG_PREFSWIN_LISTER_SELECTED_FILE, "PREF_LISTER_SELECTEDFILE"),
						
						Child, NSLabel2(MSG_PREFSWIN_LISTER_SELECTED_DIRECTORY),
						Child, flp_directory_sel = MUICreatePoppen( MSG_PREFSWIN_LISTER_SELECTED_DIRECTORY, "PREF_LISTER_SELECTED_DIRECTORY"),
						
						Child, NSLabel2(MSG_PREFSWIN_LISTER_SOFTLINK),
						Child, flp_softlink = MUICreatePoppen( MSG_PREFSWIN_LISTER_SOFTLINK, "PREF_LISTER_SOFTLINK"),

						Child, NSLabel2(MSG_PREFSWIN_LISTER_HARDLINK),
						Child, flp_hardlink = MUICreatePoppen( MSG_PREFSWIN_LISTER_HARDLINK, "PREF_LISTER_HARDLINK"),

						Child, NSLabel2(MSG_PREFSWIN_LISTER_VOLUME),
						Child, flp_volume = MUICreatePoppen( MSG_PREFSWIN_LISTER_VOLUME, "PREF_LISTER_VOLUME"),
						
						Child, NSLabel2(MSG_PREFSWIN_LISTER_ASSIGN),
						Child, flp_assign = MUICreatePoppen( MSG_PREFSWIN_LISTER_ASSIGN, "PREF_LISTER_ASSIGNCOLOR"),

						Child, NSLabel2(MSG_PREFSWIN_LISTER_COLUMN),
						Child, flp_column = MUICreatePoppen( MSG_PREFSWIN_LISTER_COLUMN, "PREF_LISTER_COLUMN"),
		/*
						Child, SLabel1("Source:"),
						Child, flp_source = NewObject(PoppenObject,
							MUIA_Window_Title, "Ambient · Source color",
							MUIA_CycleChain, 1,
							MUIA_Pendisplay_Spec, getprefs(DSI_FASTLIST_COLOR_SOURCE_FG),
						End,
						
						Child, SLabel1("Destination:"),
						Child, flp_destination = NewObject(PoppenObject,
							MUIA_Window_Title, "Ambient · Destination color",
							MUIA_CycleChain, 1,
							MUIA_Pendisplay_Spec, getprefs(DSI_FASTLIST_COLOR_DESTINATION_FG),
						End,
		*/
					End,
					
					Child, VGroup,
						Child, HVSpace,
						Child,  HGroup,
								Child, NSLabel2(MSG_PREFSWIN_LISTER_FONT),
								Child, flpop_font = PopaslObject,
									MUIA_ShortHelp, GSI( MSG_PREFSWIN_LISTER_FONT_HELP ),
									MUIA_Popasl_Type, ASL_FontRequest,
									MUIA_Popstring_String, fls_font = pstring(DSI_FASTLIST_FONT, PATH_SIZE, GSI(MSG_PREFSWIN_LISTER_FONT)),
									MUIA_Popstring_Button, MUICreatePopButton( MSG_PREFSWIN_LISTER_FONT, MUII_PopFont, NULL),
									ASLFO_TitleText, GSI(MSG_PREFSWIN_LISTER_FONT_REQ),
									ASLFO_DoStyle, FALSE,
									End,
								End,
						Child, HVSpace,
						Child, HGroup,
							Child, ColGroup(3),
									Child, flc_background = MUICreateCheckbox( MSG_PREFSWIN_LISTER_USEWINDOWBACKGROUND, FALSE, "PREF_LISTER_USEWINDOWBACKGROUND"),
									Child, MUICreateLabel( MSG_PREFSWIN_LISTER_USEWINDOWBACKGROUND, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
									Child, HSpace(0),
									Child, flc_hilighted_sorting_column = MUICreateCheckbox( MSG_PREFSWIN_LISTER_HILIGHTED_SORTING_COLUMN, FALSE, "PREF_LISTER_HILIGHTED_SORTING_COLUMN"),
									Child, MUICreateLabel( MSG_PREFSWIN_LISTER_HILIGHTED_SORTING_COLUMN, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
									Child, HSpace(0),
							End,
							Child, ColGroup(3),
									Child, flc_alternated_rows = MUICreateCheckbox( MSG_PREFSWIN_LISTER_ALTERNATED_ROWS, FALSE, "PREF_LISTER_ALTERNATED_ROWS"),
									Child, MUICreateLabel( MSG_PREFSWIN_LISTER_ALTERNATED_ROWS, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
									Child, HSpace(0),
									Child, flc_bold_directories = MUICreateCheckbox( MSG_PREFSWIN_LISTER_BOLD_DIRECTORIES, FALSE, "PREF_LISTER_BOLD_DIRECTORIES"),
									Child, MUICreateLabel( MSG_PREFSWIN_LISTER_BOLD_DIRECTORIES, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
									Child, HSpace(0),
							End,
						Child, HVSpace,
						End,
					End,
					Child, HVSpace,
				End,
			End, /* virtgroup object */
		End, /* scrollgroup object */
	End;

	if (!obj)
	{
		return (0);
	}

	data = INST_DATA(cl, obj);

	data->fll_used_columns_files = fll_used_columns_files;
	data->flc_modes = flc_modes;
	data->flc_selection_modes = flc_selection_modes;
	data->flc_iconsize_files = flc_iconsize_files;
	data->flc_compactsize = flc_compactsize;

	data->fll_used_columns_devices = fll_used_columns_devices;
	data->flc_iconsize_devices = flc_iconsize_devices;
	data->flc_displayassigns = flc_displayassigns;

	data->flp_file = flp_file;
	data->flp_directory = flp_directory;
	data->flp_file_sel = flp_file_sel;
	data->flp_directory_sel = flp_directory_sel;
	data->flp_softlink = flp_softlink;
	data->flp_hardlink = flp_hardlink;
	data->flp_volume = flp_volume;
	data->flp_assign = flp_assign;
	data->flp_column = flp_column;

/*
	data->flp_source = flp_source;
	data->flp_destination = flp_destination;
*/

	data->flc_background = flc_background;
	data->flc_alternated_rows = flc_alternated_rows;
	data->flc_hilighted_sorting_column = flc_hilighted_sorting_column;
	data->flc_bold_directories = flc_bold_directories;
	data->flpop_font = flpop_font;

	listview_parse_mode(getprefsstr(DSI_FASTLIST_DEFAULT_MODE_FILES), FLT_FILES, &lv_format);

	if(listview_get_columns(getprefsstr(DSI_FASTLIST_DEFAULT_FORMAT_FILES), FLT_FILES, &columns))
	{
		for(i=0; columns[i] ; i++)
		{
			if(columns[i]->col == lv_format.sort_column)
			{
				columns[i]->sort_column = TRUE;
				columns[i]->sort_direction = lv_format.sort_mode;
			}
			else
			{
				columns[i]->sort_column = FALSE;
				columns[i]->sort_direction = 1;
			}

			DoMethod(data->fll_used_columns_files, MUIM_List_InsertSingle, columns[i], MUIV_List_Insert_Bottom);
		}

		listview_free_columns(columns);
	}

	setupprefs_noset(fll_used_columns_files, MUIA_List_DoubleClick, MUIV_EveryTime);
	setupprefs_noset(fll_used_columns_files, MUIA_List_DropMark, MUIV_EveryTime);

	setupprefs(flc_modes, MUIA_Cycle_Active, lv_format.view_mode);
	setupprefs(flc_iconsize_files, MUIA_Cycle_Active, lv_format.icon_size);
	setupprefs(flc_selection_modes, MUIA_Cycle_Active, (ULONG) getprefslong(DSI_FASTLIST_SELECTION_MODE));
	setupprefs(flc_compactsize, MUIA_Selected, (ULONG) getprefslong(DSI_FASTLIST_COMPACT_SIZE_DISPLAY));

	listview_parse_mode(getprefsstr(DSI_FASTLIST_DEFAULT_MODE_DEVICES), FLT_DEVICES, &lv_format);

	if(listview_get_columns(getprefsstr(DSI_FASTLIST_DEFAULT_FORMAT_DEVICES), FLT_DEVICES, &columns))
	{
		for(i=0; columns[i] ; i++)
		{
			if(columns[i]->col == lv_format.sort_column)
			{
				columns[i]->sort_column = TRUE;
				columns[i]->sort_direction = lv_format.sort_mode;
			}
			else
			{
				columns[i]->sort_column = FALSE;
				columns[i]->sort_direction = lv_format.sort_mode;
			}

			DoMethod(data->fll_used_columns_devices, MUIM_List_InsertSingle, columns[i], MUIV_List_Insert_Bottom);
		}

		listview_free_columns(columns);
	}

	setupprefs_noset(fll_used_columns_devices, MUIA_List_DoubleClick, MUIV_EveryTime);
	setupprefs_noset(fll_used_columns_devices, MUIA_List_InsertPosition, MUIV_EveryTime);

	setupprefs(flc_iconsize_devices, MUIA_Cycle_Active, lv_format.icon_size);
	setupprefs(flc_displayassigns, MUIA_Selected, (ULONG) getprefslong(DSI_FASTLIST_ASSIGN_DISPLAY));


	setupprefs(flp_file, MUIA_Pendisplay_Spec, (ULONG) getprefs(DSI_FASTLIST_COLOR_FILE_FG));
	setupprefs(flp_directory, MUIA_Pendisplay_Spec, (ULONG) getprefs(DSI_FASTLIST_COLOR_DIRECTORY_FG));
	setupprefs(flp_file_sel, MUIA_Pendisplay_Spec, (ULONG) getprefs(DSI_FASTLIST_COLOR_FILE_SEL_FG));
	setupprefs(flp_directory_sel, MUIA_Pendisplay_Spec, (ULONG) getprefs(DSI_FASTLIST_COLOR_DIRECTORY_SEL_FG));
	setupprefs(flp_softlink, MUIA_Pendisplay_Spec, (ULONG) getprefs(DSI_FASTLIST_COLOR_SOFTLINK_FG));
	setupprefs(flp_hardlink, MUIA_Pendisplay_Spec, (ULONG) getprefs(DSI_FASTLIST_COLOR_HARDLINK_FG));
	setupprefs(flp_volume, MUIA_Pendisplay_Spec, (ULONG) getprefs(DSI_FASTLIST_COLOR_VOLUME_FG));
	setupprefs(flp_assign, MUIA_Pendisplay_Spec, (ULONG) getprefs(DSI_FASTLIST_COLOR_ASSIGN_FG));
	setupprefs(flp_column, MUIA_Pendisplay_Spec, (ULONG) getprefs(DSI_FASTLIST_COLOR_COLUMN_FG));

	setupprefs(data->flc_background, MUIA_Selected, getprefslong(DSI_FASTLIST_WINDOW_BG));
	setupprefs(data->flc_alternated_rows, MUIA_Selected, getprefslong(DSI_FASTLIST_ALTERNATED_ROWS));
	setupprefs(data->flc_hilighted_sorting_column, MUIA_Selected, getprefslong(DSI_FASTLIST_HILIGHTED_SORTING_COLUMN));
	setupprefs(data->flc_bold_directories, MUIA_Selected, getprefslong(DSI_FASTLIST_BOLD_DIRECTORIES));

	setupprefs_noset(fls_font, MUIA_String_Acknowledge, MUIV_EveryTime);

	return ((ULONG)obj);
}


static void store_listview_format(APTR obj UNUSED, struct Data * data, ULONG type)
{
	int i;
	ULONG sort_column = 0;
	LONG  sort_direction = 1;
	struct column_entry * e;
	struct column_entry ** columns;
	ULONG count = 0;
	STRPTR format = NULL;
	TEXT mode[64];
	APTR listobject = (type == FLT_FILES) ? data->fll_used_columns_files : data->fll_used_columns_devices;
	ULONG formatid = (type == FLT_FILES) ? DSI_FASTLIST_DEFAULT_FORMAT_FILES : DSI_FASTLIST_DEFAULT_FORMAT_DEVICES;

	for(i=0;; i++)
	{
		DoMethod(listobject, MUIM_List_GetEntry, i, &e);

		if(e)
		{
			count++;
		}
		else
		{
			break;
		}
	}

	columns = (struct column_entry **) malloc(sizeof(struct column_entry *)*(count+1));

	if(columns)
	{
		for(i=0;; i++)
		{
			DoMethod(listobject, MUIM_List_GetEntry, i, &e);

			if(e)
			{
				columns[i] = e;

				if(e->sort_column)
				{
					sort_column	= e->col;
					sort_direction = e->sort_direction;
				}
			}
			else
			{
				break;
			}
		}

		columns[i] = NULL;

		if(listview_generate_format(columns, &format))
		{
			setprefsstr(formatid, format);
			listview_free_format(format);
		}

		free(columns);
	}

	if(type == FLT_FILES)
	{
		snprintf(mode, sizeof(mode), "ICONSIZE=%ld SORTCOLUMN=%ld SORTMODE=%ld VIEWMODE=%ld",
				 getv(data->flc_iconsize_files, MUIA_Cycle_Active),
				 sort_column,
				 sort_direction,
				 getv(data->flc_modes, MUIA_Cycle_Active)
				);

		setprefsstr(DSI_FASTLIST_DEFAULT_MODE_FILES, mode);
	}
	else
	{
		snprintf(mode, sizeof(mode), "ICONSIZE=%ld SORTCOLUMN=%ld SORTMODE=%ld VIEWMODE=0",
				 getv(data->flc_iconsize_devices, MUIA_Cycle_Active),
				 sort_column,
				 sort_direction
				);

		setprefsstr(DSI_FASTLIST_DEFAULT_MODE_DEVICES, mode);
	}
}

DEFTMETHOD(Prefswin_Store)
{
	GETDATA;


	store_listview_format(obj, data, FLT_FILES);
	setprefslong(DSI_FASTLIST_COMPACT_SIZE_DISPLAY, getv(data->flc_compactsize, MUIA_Selected));

	store_listview_format(obj, data, FLT_DEVICES);
	setprefslong(DSI_FASTLIST_ASSIGN_DISPLAY, getv(data->flc_displayassigns, MUIA_Selected));

	setprefslong(DSI_FASTLIST_SELECTION_MODE, getv(data->flc_selection_modes, MUIA_Cycle_Active));
	
	setprefs(DSI_FASTLIST_COLOR_FILE_FG, sizeof(struct MUI_PenSpec), (APTR)getv(data->flp_file, MUIA_Pendisplay_Spec));
	setprefs(DSI_FASTLIST_COLOR_DIRECTORY_FG, sizeof(struct MUI_PenSpec), (APTR)getv(data->flp_directory, MUIA_Pendisplay_Spec));
	setprefs(DSI_FASTLIST_COLOR_FILE_SEL_FG, sizeof(struct MUI_PenSpec), (APTR)getv(data->flp_file_sel, MUIA_Pendisplay_Spec));
	setprefs(DSI_FASTLIST_COLOR_DIRECTORY_SEL_FG, sizeof(struct MUI_PenSpec), (APTR)getv(data->flp_directory_sel, MUIA_Pendisplay_Spec));
	setprefs(DSI_FASTLIST_COLOR_SOFTLINK_FG, sizeof(struct MUI_PenSpec), (APTR)getv(data->flp_softlink, MUIA_Pendisplay_Spec));
	setprefs(DSI_FASTLIST_COLOR_HARDLINK_FG, sizeof(struct MUI_PenSpec), (APTR)getv(data->flp_hardlink, MUIA_Pendisplay_Spec));
	setprefs(DSI_FASTLIST_COLOR_VOLUME_FG, sizeof(struct MUI_PenSpec), (APTR)getv(data->flp_volume, MUIA_Pendisplay_Spec));
	setprefs(DSI_FASTLIST_COLOR_ASSIGN_FG, sizeof(struct MUI_PenSpec), (APTR)getv(data->flp_assign, MUIA_Pendisplay_Spec));
	setprefs(DSI_FASTLIST_COLOR_SOURCE_FG, sizeof(struct MUI_PenSpec), (APTR)getv(data->flp_source, MUIA_Pendisplay_Spec));
	setprefs(DSI_FASTLIST_COLOR_DESTINATION_FG, sizeof(struct MUI_PenSpec), (APTR)getv(data->flp_destination, MUIA_Pendisplay_Spec));
	setprefs(DSI_FASTLIST_COLOR_COLUMN_FG, sizeof(struct MUI_PenSpec), (APTR)getv(data->flp_column, MUIA_Pendisplay_Spec));

	setprefslong(DSI_FASTLIST_WINDOW_BG, getv(data->flc_background, MUIA_Selected));
	setprefslong(DSI_FASTLIST_ALTERNATED_ROWS, getv(data->flc_alternated_rows, MUIA_Selected));
	setprefslong(DSI_FASTLIST_HILIGHTED_SORTING_COLUMN, getv(data->flc_hilighted_sorting_column, MUIA_Selected));
	setprefslong(DSI_FASTLIST_BOLD_DIRECTORIES, getv(data->flc_bold_directories, MUIA_Selected));

	storestring(data->flpop_font, DSI_FASTLIST_FONT);

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

DECSUBCLASS_NC(MUIC_Group, prefswin_listerclass)
