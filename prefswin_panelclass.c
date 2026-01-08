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
 * $Id: prefswin_panelclass.c,v 1.19 2021/04/18 12:08:07 kronos Exp $
 */

#include "ambient.h"
#include <mui/Listtree_mcc.h>
#include <libraries/asl.h>
/* public */

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "prefswin.h"
#include "prefsclone.h"
#include "panelprefs.h"
#include "methodstack.h"
#include "screen.h"
#include "legacy.h"
#include "paneltags.h"
#include "panelitem.h"

/************************************************************************/

struct Data {
	APTR parentobj;
	APTR panel_list,listtreegroup;
	APTR panelwin,panelobject;
	APTR lv_item,rescan;
	APTR new_panel,del_object;
	APTR object_group;
	APTR highlight,highlight_grp;
	APTR highlight_brighten,highlight_darken;
	APTR highlight_tint,highlight_tintfade;
	APTR dragdrop,dragdrop_grp;
	APTR dragdrop_brighten,dragdrop_darken;
	APTR dragdrop_tint,dragdrop_tintfade;
	APTR selected,selected_grp;
	APTR selected_brighten,selected_darken;
	APTR selected_tint,selected_tintfade;
	APTR sl_speed;
	APTR as_drop,as_delete,as_move,as_windowpos;
	APTR grid;
	APTR txt_version,txt_author,txt_author2,ft_descr;
};

APTR prefswin = NULL; // bitRocky: removed "static", caused its "extern"ed this way in prefswin.h (GCC5)

/************************************************************************/

/* XXX: should use a table.. or better, some general routine taking a table. panel has other setting using that conversion stuff */
static ULONG speed_to_slider(ULONG speed)
{
	switch (speed)
	{
		case MV_Panel_ZipSpeed_Slow:
			return (0);

		case MV_Panel_ZipSpeed_Medium:
			return (1);

		case MV_Panel_ZipSpeed_Fast:
			return (2);

		default:
			return (3);
	}
}


static ULONG slider_to_speed(ULONG active)
{
	switch (active)
	{
		case 0:
			return (MV_Panel_ZipSpeed_Slow);

		case 1:
			return (MV_Panel_ZipSpeed_Medium);

		case 2:
			return (MV_Panel_ZipSpeed_Fast);

		default:
			return (MV_Panel_ZipSpeed_Instant);
	}
}

/************************************************************************/
#if 0
static void ListPanel( APTR panel_list, APTR mother, APTR panelwin )
{
		APTR tbar, button, object_state, treenode;
		struct List *button_list;
		STRPTR oname;

		GetAttr( MA_Panelwin_Group,panelwin, (ULONG*) &tbar );
		if( tbar )
		{
			GetAttr( MUIA_Group_ChildList, tbar, (ULONG*) &button_list );
			object_state = button_list->lh_Head;
			while ( ( button = (Object*) NextObject( &object_state ) ) )
			{
				ULONG type;
				get( button, MA_Panel_Extern_DisplayName, (ULONG*) &oname );
				if (get( button, MA_Panel_Type, &type ) )
				{
					switch( type )
					{
						case MV_Panel_Type_Spacer:
						case MV_Panel_Type_Separator:
						case MV_Panel_Type_ViewWatcher:
						case MV_Panel_Type_Bookmarks:
						case MV_Panel_Type_DirPanel:
						case MV_Panel_Type_External:
							treenode = (APTR) DoMethod( panel_list, MUIM_Listtree_Insert, oname, button, mother, MUIV_Listtree_Insert_PrevNode_Tail, 0 );
							break;
						case MV_Panel_Type_Button:
							get( button, MA_Panel_URI, (ULONG*) &oname );
							treenode = (APTR) DoMethod( panel_list, MUIM_Listtree_Insert, oname, button, mother, MUIV_Listtree_Insert_PrevNode_Tail, 0 );
							break;
						case MV_Panel_Type_SubPanel:
							{
								APTR subpanel;
								treenode = (APTR) DoMethod( panel_list, MUIM_Listtree_Insert, oname, button, mother, MUIV_Listtree_Insert_PrevNode_Tail, TNF_LIST/*|TNF_OPEN*/ );
								get( button, MA_Panelbutton_AttachedObject, (ULONG*) &subpanel );
								if(subpanel) ListPanel( panel_list,treenode,subpanel );
							}
							break;
					}
				}
			}
		}
 
}
#endif
/************************************************************************/

extern APTR prefwin,app;

DEFNEW
{
	struct Data *data, t;

	static STRPTR pages[ MSG_PREFSWIN_PANEL_GLOBAL - MSG_PREFSWIN_PANEL_PANELS + 2 ];
	static STRPTR cyc_effects[ MSG_PREFSWIN_PANEL_EFFECT_TINT_FADE - MSG_PREFSWIN_PANEL_EFFECT_NONE + 2 ];

	memset( &t, 0, sizeof( struct Data ) );
	MUIInitStringArray( (APTR) pages, MSG_PREFSWIN_PANEL_PANELS, MSG_PREFSWIN_PANEL_GLOBAL );

	prefswin = obj = DoSuperNew( cl, obj,
		Child, RegisterGroup( pages ),
			Child, ColGroup(2),
			Child, VGroup,  MUIA_Weight,  80,
				Child, t.listtreegroup = VGroup, MUIA_Weight,  80,
				
					Child,HGroup,
						Child, t.new_panel = MUICreateButton( MSG_PREFSWIN_PANEL_NEW , NULL ),
					//	  Child, HSpace(0),
						Child, t.del_object = MUICreateButton( MSG_PREFSWIN_PANEL_DELETE , NULL ),
						End,
					End,
				//	  End,
			
				End,
					
				
					
					//Child,HGroup,
						Child, VGroup,MUIA_HorizWeight,  30,
						Child, t.lv_item = NewObject( getpanelitem_listclass(), NULL,
																MUIA_CycleChain,           TRUE,
																MUIA_Listview_DragType,    MUIV_Listview_DragType_Immediate,
																MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
																MUIA_ShortHelp, GSI(MSG_PREFSWIN_PANEL_LIST_HELP) ,
																End,
						Child, t.rescan = MUICreateButton(MSG_PREFSWIN_PANEL_RESCAN , NULL),
					//End,
					End,


					Child,VGroup,
					Child, t.object_group = VGroup, MUIA_VertWeight,  30,MUIA_HorizWeight,  80,     End,
					End,

			//	  Child, MUI_MakeObject( MUIO_VBar, 1 ),
			
					Child,VGroup, MUIA_VertWeight,  30,MUIA_HorizWeight,  30,
					Child, ColGroup(2), MUIA_Group_HorizCenter,2,
					//	  Child, MUICreateLabel( MSG_PREFSWIN_PANEL_CLASS_NAME, /*MUIO_Label_LeftAligned|*/MUIO_Label_SingleFrame),
					//	  Child, t.txt_name = MUICreateTextNoFrame( MSG_PREFSWIN_PANEL_CLASS_NAME, NULL ),

						Child, MUICreateLabel( MSG_PREFSWIN_PANEL_CLASS_VERSION, /*MUIO_Label_LeftAligned|*/MUIO_Label_SingleFrame),
						Child, t.txt_version = MUICreateTextNoFrame( MSG_PREFSWIN_PANEL_CLASS_VERSION, "" ),

						Child, MUICreateLabel( MSG_PREFSWIN_PANEL_CLASS_AUTHOR, /*MUIO_Label_LeftAligned|*/MUIO_Label_SingleFrame),
						Child, t.txt_author = MUICreateTextNoFrame( MSG_PREFSWIN_PANEL_CLASS_AUTHOR,  NULL  ),
						Child, HSpace(0),
						Child, t.txt_author2 = MUICreateTextNoFrame( MSG_PREFSWIN_PANEL_CLASS_AUTHOR,  NULL  ),
					End,
					Child, MUI_MakeObject(MUIO_BarTitle,GSI(MSG_PANELITEMWINCLASS_DESCGROUPTITLE)),
				//	  Child, HGroup,
					//	  GroupFrameT(GSI(MSG_PANELITEMWINCLASS_DESCGROUPTITLE)),

						Child, t.ft_descr = FloattextObject, MUIA_List_HScrollerVisibility, MUIV_List_HScrollerVisibility_Never,
												TextFrame, MUIA_ShortHelp, GSI(MSG_PREFSWIN_PANEL_CLASS_DESCGROUPTITLE_HELP ),
												End,
						//End,
				//	  End,
				End,
			End,
			Child,VGroup,
				Child,VSpace(0),
					Child,HGroup, GroupFrameT(GSI(MSG_PREFSWIN_PANEL_LAYOUT)),
						Child, t.grid = MUICreateCheckbox( MSG_PREFSWIN_PANEL_FORCE_GRID, getprefslong( DSI_PANEL_LAYOUT_GRID ),NULL ),
						Child, MUICreateLabel( MSG_PREFSWIN_PANEL_FORCE_GRID, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned ),
						Child,HSpace(0),
					End,
                Child,VSpace(0),
				Child, HGroup, GroupFrameT( GSI(MSG_PREFSWIN_PANEL_AUTOSAVE) ),
					Child, ColGroup(2),
						Child, t.as_drop = MUICreateCheckbox( MSG_PREFSWIN_PANEL_AUTOSAVE_DROP, getprefslong( DSI_PANEL_AUTOSAVE_DROP ),NULL ),
						Child, MUICreateLabel( MSG_PREFSWIN_PANEL_AUTOSAVE_DROP, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned ),
						Child, t.as_delete = MUICreateCheckbox( MSG_PREFSWIN_PANEL_AUTOSAVE_DELETE, getprefslong( DSI_PANEL_AUTOSAVE_DELETE ), NULL ),
						Child, MUICreateLabel(MSG_PREFSWIN_PANEL_AUTOSAVE_DELETE, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned ),
						Child, t.as_move = MUICreateCheckbox( MSG_PREFSWIN_PANEL_AUTOSAVE_MOVE, getprefslong( DSI_PANEL_AUTOSAVE_MOVE ), NULL ),
						Child, MUICreateLabel( MSG_PREFSWIN_PANEL_AUTOSAVE_MOVE, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned ),
						Child, t.as_windowpos = MUICreateCheckbox( MSG_PREFSWIN_PANEL_AUTOSAVE_WINDOWPOS , getprefslong( DSI_PANEL_AUTOSAVE_WINDOWPOS), NULL ),
						Child, MUICreateLabel( MSG_PREFSWIN_PANEL_AUTOSAVE_WINDOWPOS , MUIO_Label_SingleFrame | MUIO_Label_LeftAligned ),
					End,
					Child, HSpace(-1),
				End,
			    Child,VSpace(0),
				Child, VGroup,GroupFrameT(GSI(MSG_PREFSWIN_PANEL_EFFECTS)),
					Child, ColGroup(3),
						Child, NSLabel2(MSG_PREFSWIN_PANEL_HIGHLIGHT),
						Child, t.highlight = MUICreateCycle( MSG_PREFSWIN_PANEL_HIGHLIGHT, cyc_effects, MSG_PREFSWIN_PANEL_EFFECT_NONE, MSG_PREFSWIN_PANEL_EFFECT_TINT_FADE, 0 ),
						Child, t.highlight_grp = PageGroup,

							/* NONE */
							Child, HVSpace,
							
							/*  CLONE */
							Child, HVSpace,

							/* LASSO */
							Child, HVSpace,

							/* BRIGHTEN */
							Child, HGroup,
								Child, NSLabel2(MSG_PREFSWIN_PANEL_EFFECT_DELTA),
								Child, t.highlight_brighten = NumericbuttonObject,
									MUIA_CycleChain, 1,
									MUIA_Numeric_Value, getprefslong(DSI_PANEL_HIGHLIGHT_BRIGHTEN),
									MUIA_Numeric_Max, 255,
								End,
							End,
							/* DARKEN */
							Child, HGroup,
								Child, NSLabel2(MSG_PREFSWIN_PANEL_EFFECT_DELTA),
								Child, t.highlight_darken = NumericbuttonObject,
									MUIA_Numeric_Value, getprefslong(DSI_PANEL_HIGHLIGHT_DARKEN),
									MUIA_CycleChain, 1,
									MUIA_Numeric_Max, 255,
								End,
							End,
							/* TINT */
							Child, HGroup,
								Child, NSLabel2(MSG_PREFSWIN_ICONDISPLAY_DESKTEXTCOLOR),
								Child, t.highlight_tint = PoppenObject,
									MUIA_CycleChain, 1,
								//	  MUIA_Window_Title, GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_TINT_REQ),
									MUIA_Pendisplay_Spec, getprefs(DSI_PANEL_HIGHLIGHT_TINT),//GetTagData(MA_DropEffect_TintVal, (ULONG)&deftint, INITTAGS),
								End,
							End,   	

							/* BLUR */
							Child, HVSpace,

							/*  GREY */
							Child, HVSpace,

							/* NEGATIVE */
							Child, HVSpace,
							
							/* NEGATIVEFADE */
							Child, HVSpace,
                          
							/* TINTFADE */
							Child, HGroup,
								Child, NSLabel2(MSG_PREFSWIN_ICONDISPLAY_DESKTEXTCOLOR),
								Child, t.highlight_tintfade = PoppenObject,
									MUIA_CycleChain, 1,
								//	  MUIA_Window_Title, GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_TINT_REQ),
									MUIA_Pendisplay_Spec, getprefs(DSI_PANEL_HIGHLIGHT_TINTFADE),//GetTagData(MA_DropEffect_TintVal, (ULONG)&deftint, INITTAGS),
								End,
							End,
						End,
						Child, NSLabel2(MSG_PREFSWIN_PANEL_DRAGDROP_EFFECT),
						Child, t.dragdrop = MUICreateCycle( MSG_PREFSWIN_PANEL_DRAGDROP_EFFECT , cyc_effects, MSG_PREFSWIN_PANEL_EFFECT_NONE, MSG_PREFSWIN_PANEL_EFFECT_TINT_FADE, 0 ),

						Child, t.dragdrop_grp = PageGroup,

							/* NONE */
							Child, HVSpace,

							/*  CLONE */
							Child, HVSpace,


							/* LASSO */
							Child, HVSpace,

							/* BRIGHTEN */
							Child, HGroup,
								Child, NSLabel2(MSG_PREFSWIN_PANEL_EFFECT_DELTA),
								Child, t.dragdrop_brighten = NumericbuttonObject,
																MUIA_CycleChain   , 1,
																MUIA_Numeric_Value, getprefslong( DSI_PANEL_DRAGDROP_BRIGHTEN ),
																MUIA_Numeric_Max  , 255,
																End,
							End,
							/* DARKEN */
							Child, HGroup,
								Child, NSLabel2(MSG_PREFSWIN_PANEL_EFFECT_DELTA),
								Child, t.dragdrop_darken = NumericbuttonObject,
																MUIA_CycleChain   , 1,
																MUIA_Numeric_Value, getprefslong( DSI_PANEL_DRAGDROP_DARKEN ),
																MUIA_Numeric_Max  , 255,
																End,
							End,
							/* TINT */
							Child, HGroup,
								Child, NSLabel2(MSG_PREFSWIN_ICONDISPLAY_DESKTEXTCOLOR),
								Child, t.dragdrop_tint = PoppenObject,
																MUIA_CycleChain     , 1,
															//	  MUIA_Window_Title, GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_TINT_REQ),
																MUIA_Pendisplay_Spec, getprefs( DSI_PANEL_DRAGDROP_TINT ),// GetTagData(MA_DropEffect_TintVal, (ULONG)&deftint, INITTAGS),
																End,
							End,

							/* BLUR */
							Child, HVSpace,

							/*  GREY */
							Child, HVSpace,

							/* NEGATIVE */
							Child, HVSpace,

							/* NEGATIVEFADE */
							Child, HVSpace,

							/* TINTFADE */
							Child, HGroup,
								Child, NSLabel2(MSG_PREFSWIN_ICONDISPLAY_DESKTEXTCOLOR),
								Child, t.dragdrop_tintfade = PoppenObject,
																MUIA_CycleChain     , 1,
															//	  MUIA_Window_Title, GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_TINT_REQ),
																MUIA_Pendisplay_Spec, getprefs( DSI_PANEL_DRAGDROP_TINTFADE ),// GetTagData(MA_DropEffect_TintVal, (ULONG)&deftint, INITTAGS),
																End,
							End,
						End,

						Child, NSLabel2( MSG_PREFSWIN_PANEL_SELECTED_EFFECT ),
						Child, t.selected = MUICreateCycle( MSG_PREFSWIN_PANEL_SELECTED_EFFECT , cyc_effects, MSG_PREFSWIN_PANEL_EFFECT_NONE, MSG_PREFSWIN_PANEL_EFFECT_TINT_FADE, 0 ),

						Child, t.selected_grp = PageGroup,
						
							/* NONE */
							Child, HVSpace,
							
							/*  CLONE */
							Child, HVSpace,


							/* LASSO */
							Child, HVSpace,

							/* BRIGHTEN */
							Child, HGroup,
								Child, NSLabel2(MSG_PREFSWIN_PANEL_EFFECT_DELTA),
								Child, t.selected_brighten = NumericbuttonObject,
																MUIA_CycleChain   , 1,
																MUIA_Numeric_Value, getprefslong( DSI_PANEL_SELECTED_BRIGHTEN ),
																MUIA_Numeric_Max  , 255,
																End,
							End,
							/* DARKEN */
							Child, HGroup,
								Child, NSLabel2(MSG_PREFSWIN_PANEL_EFFECT_DELTA),
								Child, t.selected_darken = NumericbuttonObject,
																MUIA_CycleChain   , 1,
																MUIA_Numeric_Value, getprefslong( DSI_PANEL_SELECTED_DARKEN ),
																MUIA_Numeric_Max  , 255,
																End,
							End,
							/* TINT */
							Child, HGroup,
								Child, NSLabel2( MSG_PREFSWIN_ICONDISPLAY_DESKTEXTCOLOR ),
								Child, t.selected_tint = PoppenObject,
																MUIA_CycleChain     , 1,
															//	  MUIA_Window_Title, GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_TINT_REQ),
																MUIA_Pendisplay_Spec,getprefs( DSI_PANEL_SELECTED_TINT ),// GetTagData(MA_DropEffect_TintVal, (ULONG)&deftint, INITTAGS),
																End,
							End,

							/* BLUR */
							Child, HVSpace,

							/*  GREY */
							Child, HVSpace,

							/* NEGATIVE */
							Child, HVSpace,

							/* NEGATIVEFADE */
							Child, HVSpace,

							/* TINTFADE */
							Child, HGroup,
								Child, NSLabel2(MSG_PREFSWIN_ICONDISPLAY_DESKTEXTCOLOR),
								Child, t.selected_tintfade = PoppenObject,
									MUIA_CycleChain, 1,
								//	  MUIA_Window_Title, GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_TINT_REQ),
									MUIA_Pendisplay_Spec, getprefs( DSI_PANEL_SELECTED_TINTFADE ),// GetTagData(MA_DropEffect_TintVal, (ULONG)&deftint, INITTAGS),
								End,
							End,
						End,
					End,
				End,
				Child, VSpace(0),
				Child, HGroup,
					Child, NSLabel2(MSG_PREFSWIN_PANEL_ZIPSPEED),
					Child, t.sl_speed = NewObject( getpanelsliderspeedclass(), NULL, TAG_DONE ), /* XXX: ditto */
				End,
			/*	  Child,HGroup,
					Child, NSLabel1(MSG_PREFSWIN_DRAGDROP_DROPPING_MARK),
					Child,  NewObject(getdropeffectclass(), NULL,
						MA_DropEffect_Label, GSI(MSG_PREFSWIN_DRAGDROP_DROPPING_MARK),
						MA_DropEffect_Mode, getprefslong(DSI_DRAGDROP_DROPEFFECT),
						MA_DropEffect_TintVal, getprefs(DSI_DRAGDROP_TINTVAL),
						MA_DropEffect_BrightenVal, getprefslong(DSI_DRAGDROP_BRIGHTENVAL),
						MA_DropEffect_DarkenVal, getprefslong(DSI_DRAGDROP_DARKENVAL),
						MA_DropEffect_TintfadeVal, getprefs(DSI_DRAGDROP_TINTFADEVAL),
						MUIA_ShortHelp, GSI(MSG_PREFSWIN_DRAGDROP_DISPLAY_HELP),
					End,
				End,  */
				Child, VSpace(0),
			End,
		End,
	End;

	if( !obj )
	{
		return( 0 );
	}
	SetAttrs(t.sl_speed, MUIA_Slider_Level, speed_to_slider(getprefslong(DSI_PANEL_ZIPSPEED)),
							MUIA_ShortHelp, GSI(MSG_PREFSWIN_PANEL_ZIPSPEED_HELP),
							TAG_DONE );
	if( ( t.panel_list = NewObject( getpanellisttreeclass(), NULL, MA_Prefswin_Object, obj, TAG_DONE ) ) )
	{
		DoMethod( t.listtreegroup, MUIM_Group_InitChange );
		DoMethod( t.listtreegroup, MUIM_Group_AddHead , t.panel_list );
		DoMethod( t.listtreegroup, MUIM_Group_ExitChange );
	}
	SetAttrs( t.highlight         , MUIA_NoNotify,TRUE, MUIA_Cycle_Active, getprefslong( DSI_PANEL_HIGHLIGHT_EFFECT ), TAG_DONE );
	SetAttrs( t.dragdrop          , MUIA_NoNotify,TRUE, MUIA_Cycle_Active, getprefslong( DSI_PANEL_DRAGDROP_EFFECT  ), TAG_DONE );
	SetAttrs( t.selected          , MUIA_NoNotify,TRUE, MUIA_Cycle_Active, getprefslong( DSI_PANEL_SELECTED_EFFECT  ), TAG_DONE );
	DoMethod( t.panel_list        , MUIM_Notify  , MUIA_Listtree_Active, MUIV_EveryTime, obj, 2, MM_Prefswin_Panels_Selected, MUIV_TriggerValue );
	DoMethod( t.new_panel         , MUIM_Notify  , MUIA_Pressed        , FALSE         , obj, 1, MM_Prefswin_Panels_New );
	DoMethod( t.del_object        , MUIM_Notify  , MUIA_Pressed        , FALSE         , obj, 1, MM_Prefswin_Panels_Delete );
	DoMethod( t.rescan            , MUIM_Notify  , MUIA_Pressed        , FALSE         , obj, 1, MM_Prefswin_Panels_RescanClasses );
	DoMethod( t.lv_item           , MUIM_Notify  , MUIA_List_Active    , MUIV_EveryTime, obj, 2, MM_Prefswin_Panels_ItemListChange, MUIV_TriggerValue );
	DoMethod( t.grid              , MUIM_Notify  , MUIA_Selected       , MUIV_EveryTime, obj, 2 ,MM_Prefswin_Panels_UpdatePanels, MV_Prefswin_Panels_UpdatePanels_Resize );
	DoMethod( t.highlight         , MUIM_Notify  , MUIA_Cycle_Active   , MUIV_EveryTime, obj, 2 ,MM_Prefswin_Panels_UpdatePanels, MV_Prefswin_Panels_UpdatePanels_Simple );
	DoMethod( t.highlight_brighten, MUIM_Notify  , MUIA_Numeric_Value  , MUIV_EveryTime, obj, 2 ,MM_Prefswin_Panels_UpdatePanels, MV_Prefswin_Panels_UpdatePanels_Simple );
	DoMethod( t.highlight_darken  , MUIM_Notify  , MUIA_Numeric_Value  , MUIV_EveryTime, obj, 2 ,MM_Prefswin_Panels_UpdatePanels, MV_Prefswin_Panels_UpdatePanels_Simple );
	DoMethod( t.highlight_tint    , MUIM_Notify  , MUIA_Pendisplay_Spec, MUIV_EveryTime, obj, 2 ,MM_Prefswin_Panels_UpdatePanels, MV_Prefswin_Panels_UpdatePanels_Simple );
	DoMethod( t.highlight_tintfade, MUIM_Notify  , MUIA_Pendisplay_Spec, MUIV_EveryTime, obj, 2 ,MM_Prefswin_Panels_UpdatePanels, MV_Prefswin_Panels_UpdatePanels_Simple );
	DoMethod( t.dragdrop_brighten , MUIM_Notify  , MUIA_Numeric_Value  , MUIV_EveryTime, obj, 2 ,MM_Prefswin_Panels_UpdatePanels, MV_Prefswin_Panels_UpdatePanels_Simple );
	DoMethod( t.dragdrop_darken   , MUIM_Notify  , MUIA_Numeric_Value  , MUIV_EveryTime, obj, 2 ,MM_Prefswin_Panels_UpdatePanels, MV_Prefswin_Panels_UpdatePanels_Simple );
	DoMethod( t.dragdrop_tint     , MUIM_Notify  , MUIA_Pendisplay_Spec, MUIV_EveryTime, obj, 2 ,MM_Prefswin_Panels_UpdatePanels, MV_Prefswin_Panels_UpdatePanels_Simple );
	DoMethod( t.dragdrop_tintfade , MUIM_Notify  , MUIA_Pendisplay_Spec, MUIV_EveryTime, obj, 2 ,MM_Prefswin_Panels_UpdatePanels, MV_Prefswin_Panels_UpdatePanels_Simple );
	DoMethod( t.dragdrop          , MUIM_Notify  , MUIA_Cycle_Active   , MUIV_EveryTime, obj, 2, MM_Prefswin_Panels_UpdatePanels, MV_Prefswin_Panels_UpdatePanels_Simple );
	
	DoMethod( t.selected_brighten , MUIM_Notify  , MUIA_Numeric_Value  , MUIV_EveryTime, obj, 2 ,MM_Prefswin_Panels_UpdatePanels,MV_Prefswin_Panels_UpdatePanels_Simple );
	DoMethod( t.selected_darken   , MUIM_Notify  , MUIA_Numeric_Value  , MUIV_EveryTime, obj, 2 ,MM_Prefswin_Panels_UpdatePanels,MV_Prefswin_Panels_UpdatePanels_Simple );
	DoMethod( t.selected_tint     , MUIM_Notify  , MUIA_Pendisplay_Spec, MUIV_EveryTime, obj, 2 ,MM_Prefswin_Panels_UpdatePanels,MV_Prefswin_Panels_UpdatePanels_Simple );
	DoMethod( t.selected_tintfade , MUIM_Notify  , MUIA_Pendisplay_Spec, MUIV_EveryTime, obj, 2 ,MM_Prefswin_Panels_UpdatePanels,MV_Prefswin_Panels_UpdatePanels_Simple );
	DoMethod( t.selected          , MUIM_Notify  , MUIA_Cycle_Active   , MUIV_EveryTime, obj, 2 ,MM_Prefswin_Panels_UpdatePanels,MV_Prefswin_Panels_UpdatePanels_Simple );

	
	set( t.highlight_grp , MUIA_Group_ActivePage, getv( t.highlight, MUIA_Cycle_Active ) );
	DoMethod( t.highlight, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, t.highlight_grp, 3, MUIM_Set, MUIA_Group_ActivePage, MUIV_TriggerValue );
	set( t.dragdrop_grp  , MUIA_Group_ActivePage, getv( t.dragdrop, MUIA_Cycle_Active ) );
	DoMethod( t.dragdrop , MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, t.dragdrop_grp, 3, MUIM_Set, MUIA_Group_ActivePage, MUIV_TriggerValue );
	set( t.selected_grp  , MUIA_Group_ActivePage, getv( t.selected, MUIA_Cycle_Active ) );
	DoMethod( t.selected , MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, t.selected_grp, 3, MUIM_Set, MUIA_Group_ActivePage, MUIV_TriggerValue );
	set( t.del_object, MUIA_Disabled, TRUE );

	data = INST_DATA( cl, obj );

	CopyMem( &t, data, sizeof( struct Data ) );

//	  *data = t;
//	  data->panelwin = 0;

	DoMethod( data->panel_list, MM_PanelListTree_Refresh );

	return( (ULONG) obj );
}

/************************************************************************/

DEFGET
{
	ULONG result = TRUE;

	switch( msg->opg_AttrID )
	{
		case MA_Window_ID:
			*msg->opg_Storage = 0;
			break;
		case MA_Window_Path:
			*msg->opg_Storage = MV_Window_Path_None;
			break;
		case MA_Window_Type:
			*msg->opg_Storage = MV_Window_Type_Panelconfig;
			break;
		case MA_Window_UserData:
			{
				GETDATA;
				*msg->opg_Storage = (ULONG) data->parentobj;
			}
			break;
		default:
			result = DOSUPER;
	}
	return( result );
}

/************************************************************************/

DEFSMETHOD(Prefswin_Main_Close)
{
	APTR win;
	ULONG type;
	APTR win_state;
	GETDATA;
	struct List *win_list;
	SetAttrs( data->panelobject, MA_AmbientPanel_SettingsPanel ,0,TAG_DONE);
	
	switch( msg->mode )
	{
		case MV_Prefswin_Main_Close_Test:
			DoMethod( obj, MM_Prefswin_Store );
			break;
		case MV_Prefswin_Main_Close_Save:
			DoMethod( obj, MM_Prefswin_Store );
			GetAttr( MUIA_Application_WindowList, app, (ULONG*) &win_list );
			win_state = win_list->lh_Head;
			while ( ( win = NextObject( &win_state ) ) )
			{
				if( ( GetAttr( MA_Panelwin_Type, win, &type ) ) && ( type == MV_Panelwin_Type_Root ) )
				{
					if( getv( win, MUIA_Window_Open ) )
					{
						DoMethod( win, MM_Panelwin_SaveConfig );
					}
					else /* this panel is up for deletion */
					{
						DoMethod( win, MM_Panelwin_Close );
					}
				}
			}
			break;
		case  MV_Prefswin_Main_Close_Cancel:
			GetAttr( MUIA_Application_WindowList, app, (ULONG*) &win_list );
			win_state = win_list->lh_Head;
			while ( ( win = NextObject( &win_state ) ) )
			{
				if( ( GetAttr( MA_Panelwin_Type, win, &type ) ) )
				{
					set( win, MUIA_Window_Open, FALSE );    /* close and remove all panel windows*/
				}
			}          	
			win_state = win_list->lh_Head;
			while ( ( win = NextObject( &win_state ) ) )     /*disposing root-panels while subpanels are still open causes trouble */
			{                                                /* so lets do it in an extra turn */
				if(GetAttr( MA_Panelwin_Type, win, &type ) )
				{
					DoMethod( app, OM_REMMEMBER, win );
					MUI_DisposeObject( win );
				}
			}
			panelprefs_loadall();    /* reload them from prefs-files */
			break;
	}
	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Prefswin_Panels_Selected)
{
	GETDATA;
	struct MUIS_Listtree_TreeNode *parent;
	APTR tbar;
	APTR settings;
	APTR child;
	APTR object_state;
	struct List *child_list;

	if( data->panelwin )
	{
		GetAttr( MA_Panelwin_Group, data->panelwin, (ULONG*) &tbar );
	}
	if( msg->node )
	{
		data->panelobject = msg->node->tn_User;
		parent = (struct MUIS_Listtree_TreeNode*) DoMethod( data->panel_list, MUIM_Listtree_GetEntry, msg->node, MUIV_Listtree_GetEntry_Position_Parent );
		if( !( parent ) )
		{
			data->panelwin = msg->node->tn_User;
			DoMethod( data->panel_list, MUIM_Listtree_Open, MUIV_Listtree_Open_ListNode_Root, MUIV_Listtree_Open_ListNode_Active );
		} else {
			while( parent )
			{
				data->panelwin = parent->tn_User;
				parent = (struct MUIS_Listtree_TreeNode*) DoMethod( data->panel_list, MUIM_Listtree_GetEntry, parent, MUIV_Listtree_GetEntry_Position_Parent );
			}
		}
		DoMethod( data->object_group, MUIM_Group_InitChange );
		get( data->object_group, MUIA_Group_ChildList, (ULONG*) &child_list );

		object_state = child_list->lh_Head;
		while( ( child = NextObject( &object_state ) ) )
		{
			DoMethod( data->object_group, OM_REMMEMBER, child );
			MUI_DisposeObject( child );
		}

		if( ( settings = (APTR) DoMethod(	data->panelobject,	MM_Panel_Settings_Group ) ) )
		{
			APTR defgroup = VGroup,
				Child,VSpace( -1 ),
				Child,HGroup,
					Child,HSpace( -1 ),
					Child,settings,
					Child,HSpace( -1 ),
				End,
				Child,VSpace( -1 ),
			End;
			DoMethod( data->object_group, OM_ADDMEMBER, defgroup );
		} else {
			APTR defgroup = VGroup,
				Child,VSpace( -1 ),
					Child,HGroup,
					Child,HSpace( -1 ),
					Child,Label1( GSI(MSG_PREFSWIN_PANEL_CLASS_NOPREFSAVAILABLE) ),
					Child,HSpace( -1 ),
				End,
				Child, VSpace( -1 ),
			End;
			DoMethod( data->object_group, OM_ADDMEMBER, defgroup );
		}
		DoMethod( data->object_group, MUIM_Group_ExitChange );
		set( data->del_object, MUIA_Disabled, FALSE );
	}
	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Prefswin_Panels_HasClosed)
{
	GETDATA;
	struct MUIS_Listtree_TreeNode *panelnode;
	if( ( panelnode = (struct MUIS_Listtree_TreeNode *)  DoMethod( data->panel_list, MM_PanelListTree_FindUData, msg->panel ) ) )
	{
		DoMethod( data->panel_list, MUIM_Listtree_Remove, 0, panelnode, 0 );
	}
	return( 0 );
}

/************************************************************************/

DEFTMETHOD(Prefswin_Panels_Delete)
{
	GETDATA;
	struct MUIS_Listtree_TreeNode *node,*parent_node;
	APTR o, parent;
	ULONG active_pos;

	node = (struct MUIS_Listtree_TreeNode*) DoMethod( data->panel_list, MUIM_Listtree_GetEntry, 0, MUIV_Listtree_GetEntry_Position_Active );
	active_pos  = getv( data->panel_list, MUIA_List_Active );
	if( node )
	{
		if( (o = node->tn_User ) )
		{
			if( ( parent = _parent( o ) ) )
			{
				APTR win;
				parent_node = (struct MUIS_Listtree_TreeNode*) DoMethod( data->panel_list, MUIM_Listtree_GetEntry, node, MUIV_Listtree_GetEntry_Position_Parent );
				DoMethod( parent, MUIM_Group_InitChange );
				DoMethod( parent, OM_REMMEMBER, o );
				DoMethod( parent, MUIM_Group_ExitChange );
				get( parent, MUIA_Window, (ULONG*) &win );
				if( win )
				{
					 SetAttrs( _win(parent), MUIA_Window_Width , MUIV_Window_Width_Default , TAG_DONE );
					 SetAttrs( _win(parent), MUIA_Window_Height, MUIV_Window_Height_Default, TAG_DONE );
				}
				MUI_DisposeObject( o );
				set( parent, MA_Panelgroup_HasChanged, TRUE );
				node = (struct MUIS_Listtree_TreeNode*) DoMethod( data->panel_list, MUIM_Listtree_GetEntry, parent_node, active_pos - 2 );
				if( ( node ) && ( active_pos > 1 ) )
				{
					set( data->panel_list,MUIA_Listtree_Active,node);
				}
			} else { /* no parent == panelwindow*/
				if( active_pos > 0 )
				{                /*move active item BEFORE deleting, treelist might get corrupted otherwise*/
					set( data->panel_list, MUIA_List_Active, MUIV_List_Active_Up );
				} else {                /* treelist also doesn't like active to set below 0 .... */
					set( data->panel_list, MUIA_List_Active, MUIV_List_Active_Off );
				}
				set(data->panel_list, MUIA_List_Quiet, TRUE );
				DoMethod(data->panel_list,MUIM_Listtree_Remove, 0 ,node,0); /* now it's safe to remove item*/
				set( data->panel_list, MUIA_List_Quiet, FALSE );
				set( o, MUIA_Window_Open, FALSE ); /* only hide the panel in case user hits "cancel" */
			}
	
		}
	}
	return( 0 );
}

/************************************************************************/

DEFTMETHOD(Prefswin_Panels_New)
{
	GETDATA;
    APTR o;

	if( ( o = NewObject( getpanelwinclass(), NULL, TAG_DONE ) ) )
	{
		DoMethod( app, OM_ADDMEMBER, o );
		set( o, MUIA_Window_Open, TRUE );
		data->panelwin = 0;
		DoMethod( data->panel_list, MM_PanelListTree_CreateItem, 0, o );
	}
	return( 0 );
}

/************************************************************************/

DEFTMETHOD(Prefswin_Store)
{
	GETDATA;
	setprefslong( DSI_PANEL_ZIPSPEED          , slider_to_speed( getv( data->sl_speed, MUIA_Slider_Level ) ) );
	setprefslong( DSI_PANEL_AUTOSAVE_DROP     , getv( data->as_drop           , MUIA_Selected ) );
	setprefslong( DSI_PANEL_AUTOSAVE_DELETE   , getv( data->as_delete         , MUIA_Selected ) );
	setprefslong( DSI_PANEL_AUTOSAVE_MOVE     , getv( data->as_move           , MUIA_Selected ) );
	setprefslong( DSI_PANEL_AUTOSAVE_WINDOWPOS, getv( data->as_windowpos      , MUIA_Selected ) );
	setprefslong( DSI_PANEL_HIGHLIGHT_EFFECT  , getv( data->highlight         , MUIA_Cycle_Active ) );
	setprefslong( DSI_PANEL_HIGHLIGHT_BRIGHTEN, getv( data->highlight_brighten, MUIA_Numeric_Value ) );
	setprefslong( DSI_PANEL_HIGHLIGHT_DARKEN  , getv( data->highlight_darken  , MUIA_Numeric_Value ) );
	setprefslong( DSI_PANEL_DRAGDROP_EFFECT   , getv( data->dragdrop          , MUIA_Cycle_Active ) );
	setprefslong( DSI_PANEL_DRAGDROP_BRIGHTEN , getv( data->dragdrop_brighten , MUIA_Numeric_Value ) );
	setprefslong( DSI_PANEL_DRAGDROP_DARKEN   , getv( data->dragdrop_darken   , MUIA_Numeric_Value ) );
	setprefs(     DSI_PANEL_HIGHLIGHT_TINT    , sizeof( struct MUI_PenSpec )  , (APTR) getv(data->highlight_tint      , MUIA_Pendisplay_Spec ) );
	setprefs(     DSI_PANEL_DRAGDROP_TINT     , sizeof( struct MUI_PenSpec )  , (APTR) getv(data->dragdrop_tint       , MUIA_Pendisplay_Spec ) );
	setprefs(     DSI_PANEL_HIGHLIGHT_TINTFADE, sizeof( struct MUI_PenSpec )  , (APTR) getv(data->highlight_tintfade  , MUIA_Pendisplay_Spec ) );
	setprefs(     DSI_PANEL_DRAGDROP_TINTFADE , sizeof( struct MUI_PenSpec )  , (APTR) getv(data->dragdrop_tintfade   , MUIA_Pendisplay_Spec ) );


	setprefslong( DSI_PANEL_SELECTED_EFFECT   , getv( data->selected          , MUIA_Cycle_Active ) );
	setprefslong( DSI_PANEL_SELECTED_BRIGHTEN , getv( data->selected_brighten , MUIA_Numeric_Value ) );
	setprefslong( DSI_PANEL_SELECTED_DARKEN   , getv( data->selected_darken   , MUIA_Numeric_Value ) );
	setprefs(     DSI_PANEL_SELECTED_TINT     , sizeof( struct MUI_PenSpec )  , (APTR) getv( data->selected_tint    , MUIA_Pendisplay_Spec ) );
	setprefs(     DSI_PANEL_SELECTED_TINTFADE , sizeof( struct MUI_PenSpec )  , (APTR) getv( data->selected_tintfade, MUIA_Pendisplay_Spec ) );

	setprefslong( DSI_PANEL_LAYOUT_GRID, getv( data->grid                     , MUIA_Selected ) );

	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Prefswin_Panels_UpdatePanels)
{
	APTR win_state,win;
	struct List *win_list;
	ULONG type;

	DoMethod( _win(obj), MM_Prefswin_Main_Close, MV_Prefswin_Main_Close_Test );
#if 0
 /*  seems the reason for it to exist doesn't get triggered on a current MUI */ 	
	if( msg->mode )
	{
		GetAttr( MUIA_Application_WindowList, app, (ULONG*) &win_list );
		win_state = win_list->lh_Head;
		while( (win = NextObject( &win_state ) ) )
		{
			if( ( GetAttr( MA_Panelwin_Type,win,&type ) ) )
			{
				SetAttrs( win, MUIA_Window_Width , MUIV_Window_Width_Default , TAG_DONE );    /*force MUI to rethink sizes*/
				SetAttrs( win, MUIA_Window_Height, MUIV_Window_Height_Default, TAG_DONE );
			}
		}
	}
#endif
	return( 0 );
}

/************************************************************************/

DEFTMETHOD(Prefswin_Panels_RescanClasses)
{
	GETDATA;
#if 0
	ULONG type;
	APTR win_state,win;
	struct List *win_list;

	GetAttr( MUIA_Application_WindowList, app, (ULONG*) &win_list );
	win_state = win_list->lh_Head;
	while( (win = NextObject( &win_state ) ) )
	{
		if( ( GetAttr( MA_Panelwin_Type, win, &type ) ) )
		{
			set( win, MUIA_Window_Open, FALSE );    /* close and remove all panel windows*/
		}
	}
	win_state = win_list->lh_Head;
	while( (win = NextObject( &win_state ) ) )       /*disposing root-panels while subpanels are still open causes trouble */
	{                                                /* so lets do it in an extra turn */
		if(GetAttr( MA_Panelwin_Type, win, &type ) )
		{
			DoMethod( app, OM_REMMEMBER, win );
			MUI_DisposeObject( win );
		}
	}

	panelprefs_loadall();    /* reload them from prefs-files */
#endif
	DoMethod( data->lv_item, MM_PanelItemList_RefreshList );
	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Prefswin_Panels_ItemListChange)
{
	GETDATA;
	TEXT aut1[ AMBIENTPANEL_AUTHORNAME_SIZEOF ];
	STRPTR aut2 = NULL;
	struct PanelItem *pi = NULL;
	aut1[0] = '\0';

	DoMethod( data->lv_item, MUIM_List_GetEntry, msg->val, &pi );

	if( pi && pi->pi_Author )
	{
		strncpy( aut1, pi->pi_Author, AMBIENTPANEL_AUTHORNAME_SIZEOF );
		aut1[ AMBIENTPANEL_AUTHORNAME_SIZEOF - 1 ] = 0x00;

		aut2 = aut1;
		while( *aut2 && ( *aut2 != '\n' ) )
		{
			aut2++;
		}
		if( *aut2 == '\n' )
		{
			*aut2 = '\0';
			aut2++;
		}
	}

	//set( data->txt_name,    MUIA_Text_Contents,  ( pi && pi->pi_DisplayName ) ? pi->pi_DisplayName : (STRPTR) GSI(MSG_PANELITEMWINCLASS_UNKNOWN) );
	set( data->txt_author,  MUIA_Text_Contents,  aut1[0]                      ? (STRPTR)&aut1[0]           : (STRPTR) GSI(MSG_PANELITEMWINCLASS_UNKNOWN) );
	set( data->txt_author2, MUIA_Text_Contents,  aut2 );
	set( data->ft_descr,    MUIA_Floattext_Text, ( pi && pi->pi_Description ) ? pi->pi_Description : (STRPTR) GSI(MSG_PANELITEMWINCLASS_NONE) );
	if( pi ) {
		DoMethod( data->txt_version, MUIM_SetAsString, MUIA_Text_Contents, "%lu.%lu\n", pi->pi_Version, pi->pi_Revision );
	} else {
		set( data->txt_version, MUIA_Text_Contents, "" );
	}
	return( 0 );
}

/************************************************************************/

DEFDISPOSE
{
	return( DOSUPER );
}

/************************************************************************/

BEGINMTABLE
DECNEW
DECGET
DECSMETHOD(Prefswin_Panels_Selected)
DECSMETHOD(Prefswin_Panels_HasClosed)
DECTMETHOD(Prefswin_Panels_Delete)
DECTMETHOD(Prefswin_Panels_New)
DECSMETHOD(Prefswin_Main_Close)
DECTMETHOD(Prefswin_Store)
DECTMETHOD(Prefswin_Panels_RescanClasses)
DECSMETHOD(Prefswin_Panels_UpdatePanels)
DECSMETHOD(Prefswin_Panels_ItemListChange)
DECDISPOSE
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, prefswin_panelclass)
 
