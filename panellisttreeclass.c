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
 *
 */

/************************************************************************/

#include "ambient.h"

#if USE_INTERNAL_PANELS

/* public */

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "prefswin.h"
#include "paneltags.h"
#include "panelitem.h"
#include <mui/Listtree_mcc.h>

#define DROP_VOID  	0
#define DROP_ONTO 	1
#define DROP_ABOVE 	2
#define DROP_BELOW  3

struct Data {
	APTR images[ PREFSWIN_NUMPAGES ];
	APTR iobjs[ PREFSWIN_NUMPAGES ];
	APTR ibm[ PREFSWIN_NUMPAGES ];
	APTR prefswin;
	ULONG droppos;
};

/************************************************************************/

static void ListPanel( APTR panel_list, APTR mother, APTR panelwin )
{
	APTR tbar, button, object_state;
	struct List *button_list;

	GetAttr( MA_Panelwin_Group, panelwin, (ULONG*) &tbar );
	if( tbar )
	{
		GetAttr( MUIA_Group_ChildList,tbar, (ULONG*) &button_list );
		object_state = button_list->lh_Head;
		while( ( button = (Object*) NextObject( &object_state ) ) )
		{
			DoMethod( panel_list, MM_PanelListTree_CreateItem, mother, button );
		}
	}
}

/************************************************************************/

DEFNEW
{
	struct TagItem *otag;
	if( ( obj = DoSuperNew( cl, obj,
		InputListFrame,
		MUIA_List_ShowDropMarks, TRUE,
		MUIA_Listview_DragType , MUIV_Listview_DragType_Immediate,
		MUIA_List_AutoVisible  , TRUE,
		TAG_DONE ) ) )
	{
		struct Data *data = INST_DATA( cl, obj );
		if( ( otag = FindTagItem( MA_Prefswin_Object, INITTAGS ) ) )
		{
			data->prefswin = (APTR) otag->ti_Data;
		}
	}
	return( (ULONG) obj );
}

/************************************************************************/

DEFMMETHOD(Setup)
{
	ULONG c;
	GETDATA;

	if( !DOSUPER )
	{
		return( FALSE );
	}

	for( c = 0 ; c < PREFSWIN_NUMPAGES; c++ )
	{
		if( ( data->ibm[c] = gfx_bitmap_create( 26, 20, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE ) ) )
		{
			gfx_blit( prefsgrp[c].logo, data->ibm[c],
					BLITTAG_SrcType  , BLITVAL_SrcType_Array,
					BLITTAG_SrcFormat, BLITVAL_SrcFormat_ARGB,
					TAG_DONE);

			data->iobjs[c] = BitmapObject,
				MUIA_Bitmap_Height, 20,
				MUIA_Bitmap_Width , 26,
				MUIA_FixHeight    , 20,
				MUIA_FixWidth     , 26,
				MUIA_Bitmap_Bitmap, gfx_bitmap_bm( data->ibm[c] ),
				MUIA_Bitmap_Alpha , 0xffffffff,
			End;

			data->images[c] = (APTR) DoMethod( obj, MUIM_List_CreateImage, data->iobjs[c], 0 );
		} else {
			/* XXX: we should fail.. and is Cleanup executed ? */
		}
	}
	return( TRUE );
}

/************************************************************************/
static ULONG DragDropMove( APTR obj,struct Data *data, APTR parent_node, APTR target_node )
{
	APTR active_node, active_parent_node;
	APTR dummy = 0;
	APTR panel_entry, panel_win, panel_group;
	APTR active_panel_entry, active_panel_win, active_panel_group;
	APTR object_state;
	struct List *button_list;
	APTR button, target_object, active_object;
	LONG button_pos = 1;

	active_node = (APTR) DoMethod( obj, MUIM_Listtree_GetEntry, obj, MUIV_Listtree_GetEntry_Position_Active );

	active_parent_node = (APTR) DoMethod( obj, MUIM_Listtree_GetEntry, active_node, MUIV_Listtree_GetEntry_Position_Parent );

	PDB(("move inside panel\n"));
	if( ( active_node ) && ( active_parent_node ) )
	{
		set(obj,MUIA_Listtree_Quiet,TRUE);
		if( ( ((struct MUIS_Listtree_TreeNode *) parent_node)->tn_Flags & TNF_LIST ) && ( data->droppos == DROP_ONTO ) )
		{
			APTR head  = (APTR) DoMethod( obj, MUIM_Listtree_GetEntry, parent_node, MUIV_Listtree_GetEntry_Position_Head );
			PDB(("dropped onto treenode %x\n",head));
			if(head == 0)dummy = (APTR) DoMethod( obj, MUIM_Listtree_Insert, "dummy", 0, parent_node, target_node, 0 );
		
		}
		DoMethod( obj, MUIM_Listtree_Move, active_parent_node, active_node, parent_node, target_node, 0 );
		if( dummy )
		{
			DoMethod( obj, MUIM_Listtree_Remove, active_parent_node, dummy );
		}
		active_object = ((struct MUIS_Listtree_TreeNode *) active_node )->tn_User;
		if( ( panel_entry = ((struct MUIS_Listtree_TreeNode *) parent_node )->tn_User ) )
		{
			if( ( MV_Panel_Type_SubPanel ==  getv( panel_entry, MA_Panel_Type ) ) )
			{
				panel_win = (APTR) getv( panel_entry, MA_Panelbutton_AttachedObject );
			} else {
				panel_win = panel_entry;
			}
			if( ( panel_win ) && ( GetAttr( MA_Panelwin_Group, panel_win, (ULONG*) &panel_group ) ) && ( panel_group ) )
			{
				DoMethod( panel_group, MUIM_Group_InitChange );
				if( parent_node != active_parent_node )
				{
					if( ( active_panel_entry = ((struct MUIS_Listtree_TreeNode *) active_parent_node )->tn_User ) )
					{
						if( ( MV_Panel_Type_SubPanel ==  getv( active_panel_entry, MA_Panel_Type ) ) )
						{
							active_panel_win = (APTR) getv( active_panel_entry, MA_Panelbutton_AttachedObject );
						} else {
							active_panel_win = active_panel_entry;
						}
						if( ( active_panel_win ) && ( GetAttr( MA_Panelwin_Group, active_panel_win, (ULONG*) &active_panel_group ) ) && ( active_panel_group ) )
						{
							DoMethod( active_panel_group, MUIM_Group_InitChange );
							DoMethod( active_panel_group, OM_REMMEMBER, active_object );
							DoMethod( active_panel_group, MUIM_Group_ExitChange );
							DoMethod( panel_group, MUIM_Group_AddTail, active_object );
						}
						PDB(("move from one panel to another\n"));
						if( getv( active_panel_win, MUIA_Window_Open ) )
						{
							PDB(("setattrs\n"));
							SetAttrs( active_panel_win,
								MUIA_Window_Width, MUIV_Window_Width_Default,
								MUIA_Window_Height, MUIV_Window_Height_Default,
							TAG_DONE );
							/*
							SetAttrs( active_panel_win, MUIA_Window_Width , MUIV_Window_Width_Default , TAG_DONE );
							SetAttrs( active_panel_win, MUIA_Window_Height, MUIV_Window_Height_Default, TAG_DONE );
							*/
						}
					}
				}
				if( target_node == (APTR) MUIV_Listtree_Insert_PrevNode_Head )
				{
					button_pos = 0;
				}
				else if( target_node == (APTR) MUIV_Listtree_Insert_PrevNode_Tail )
				{
					button_pos = -1;
				}
				else
				{
					target_object = ((struct MUIS_Listtree_TreeNode *) target_node )->tn_User;
					if( ( target_object ) && ( GetAttr( MUIA_Group_ChildList, panel_group, (ULONG*) &button_list ) ) )
					{
						object_state = button_list->lh_Head;
						while( ( button =  NextObject( &object_state ) ) && ( button != target_object ) )
						{
							if( button != active_object ) {
								button_pos++;
							}
						}
					}
				}
				DoMethod( panel_group, MUIM_Group_MoveMember, active_object, button_pos );
				DoMethod( panel_group, MUIM_Group_ExitChange );
			}
			if( getv( panel_win, MUIA_Window_Open ) )
			{
				PDB(("setattrs\n"));
				SetAttrs( panel_win,
					MUIA_Window_Width,  MUIV_Window_Width_Default,
					MUIA_Window_Height, MUIV_Window_Height_Default,
				TAG_DONE );
				/*
				SetAttrs( panel_win, MUIA_Window_Width , MUIV_Window_Width_Default , TAG_DONE );
				SetAttrs( panel_win, MUIA_Window_Height, MUIV_Window_Height_Default, TAG_DONE );
				*/
			}
		}
		set(obj,MUIA_Listtree_Quiet,FALSE);
	}
	return( 0 );
}

DEFMMETHOD(DragDrop)
{
	GETDATA;
	STRPTR path;
	ULONG rx, ry, x, y, dummy;
	APTR ins_object = NULL;
	struct MUIS_Listtree_TestPos_Result pos = { NULL, 0, 0, 0 };
	APTR target_node, parent_node, treeins;
	APTR panel_entry, panel_win, panel_group;

	GetAttr( MUIA_Window_LeftEdge, _win( obj ), &x );
	GetAttr( MUIA_Window_TopEdge , _win( obj ), &y );
	rx = msg->x - x;
	ry = msg->y - y;

	if( ( ( DoMethod( obj, MUIM_Listtree_TestPos, rx, ry, &pos ) ) && ( pos.tpr_TreeNode ) ) )
	{
		set(obj, MUIA_Listtree_Quiet, TRUE);
		target_node = pos.tpr_TreeNode;
		parent_node = (APTR)DoMethod( obj, MUIM_Listtree_GetEntry, target_node, MUIV_Listtree_GetEntry_Position_Parent );

		if( ( ( (struct MUIS_Listtree_TreeNode *) target_node )->tn_Flags & TNF_LIST ) && ( data->droppos == DROP_ONTO ) )
		{
			PDB(("dropped onto treenode\n"));
			parent_node = target_node;
			target_node = (APTR)MUIV_Listtree_Insert_PrevNode_Tail;
		}
		else if( data->droppos == DROP_ABOVE )
		{
			APTR prev_node;
			PDB(("dropped above node\n"));
			prev_node = (APTR) DoMethod( obj, MUIM_Listtree_GetEntry, target_node, MUIV_Listtree_GetEntry_Position_Previous, 0 );
			if( prev_node )
			{
				target_node = prev_node;
			} else {
				target_node = (APTR) MUIV_Listtree_Insert_PrevNode_Head;
			}
		}
		if( msg->obj == obj ) /* user moves existing object around */
		{
			return DragDropMove( obj, data, parent_node, target_node );
		}
		if( get( msg->obj, MA_Panelitem_list_IsList, &dummy ) )    /* user tries to add a new object */
		{
			//LONG id;
			struct PanelItem *pi;

			PDB(("user tries to add a new object\n"));
			/*
			get( msg->obj, MUIA_List_Active, &id );
			DoMethod( msg->obj, MUIM_List_GetEntry, id, &pi );
			*/
			DoMethod( msg->obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &pi );
			PDB(("panelitem pi = %x\n", pi));
			if( pi )
			{
				PDB(("type is 0x%08lx\n", pi->pi_Type ));
				if( pi->pi_Type == PT_EXTERNAL )
				{
					APTR sobj;
					if( ( sobj = NewObject( getpanelexternalsupportclass(), NULL, TAG_DONE ) ) )
					{
						if( !( ins_object = MUI_NewObject( pi->pi_ClassName, MA_Panelextern_SupportObject, sobj, TAG_DONE ) ) )
						{
							DisposeObject( sobj );
						}
					}
				} else {
					ins_object = NewObject( pi->pi_Class, NULL, TAG_DONE );
				}
			}
		}
		else if( get( msg->obj, MA_Icon_PathInfo, &path ) )  /* user drops icon into treelist */
		{
			PDB(("user drops icon into treelist\n"));
			ins_object = NewObject( getpanelcommandbuttonclass(), NULL, MA_Panel_URI, getv( msg->obj, MA_Icon_Path ), MA_Panel_Imagepath, path, TAG_DONE );
		}
		PDB(("insobject %x\n",ins_object));
		if( ins_object )
		{
			if( getv( ins_object, MA_Panel_Type ) == MV_Panel_Type_SubPanel )
			{
				APTR tbar = 0;
				APTR subpanel = 0;
				treeins = (struct MUIS_Listtree_TreeNode *) DoMethod( obj, MUIM_Listtree_Insert, 0, ins_object, parent_node, target_node,TNF_LIST);
				GetAttr( MA_Panelbutton_AttachedObject, ins_object, (ULONG*) &subpanel );
				if(subpanel) GetAttr( MA_Panelwin_Group, subpanel, (ULONG*) &tbar );
				PDB(("%x\n",tbar));
				if(tbar)
				{
					DoMethod( tbar, MUIM_Notify, MA_Panelgroup_HasChanged, TRUE, obj, 2, MM_PanelListTree_RefreshNode, treeins );
				}
			} else {
				treeins = (struct MUIS_Listtree_TreeNode *) DoMethod( obj, MUIM_Listtree_Insert, 0, ins_object, parent_node, target_node, 0 );
			}
			if( ( panel_entry = ( (struct MUIS_Listtree_TreeNode *) parent_node )->tn_User ) )
			{
				if( ( getv( panel_entry, MA_Panel_Type ) == MV_Panel_Type_SubPanel ) )
				{
					panel_win = (APTR) getv( panel_entry, MA_Panelbutton_AttachedObject );
					// XXX: strange: "panel_win" is set to "MV_Panel_type_SubPanel" ??? not a ptr to the window...
					// this only happens for the release version !??!?!?!
					// in the beta/debug build, "getv(panel_entry, MA_Panel_Type)" seems to be a ptr!? 
					if (panel_win == (APTR)MV_Panel_Type_SubPanel) panel_win = panel_entry;
				} else {
					panel_win = panel_entry;
				}
				if( panel_win && GetAttr( MA_Panelwin_Group, panel_win, (ULONG*) &panel_group ) && panel_group )
				{
					DoMethod( panel_group, MUIM_Group_InitChange );
					if( (ULONG) target_node == MUIV_Listtree_Insert_PrevNode_Tail )
					{
						DoMethod( panel_group, MUIM_Group_AddTail, ins_object );
					}
					else if( (ULONG) target_node == MUIV_Listtree_Insert_PrevNode_Head )
					{
						DoMethod( panel_group, MUIM_Group_AddHead, ins_object );
					}
					else
					{
						ULONG pos = 1;
						APTR object_state;
						struct List *button_list;
						APTR button, target_object;
						target_object = ( (struct MUIS_Listtree_TreeNode *) target_node)->tn_User;
						if( ( target_object ) && ( GetAttr( MUIA_Group_ChildList, panel_group, (ULONG*) &button_list ) ) && ( button_list ) )
						{
							object_state = button_list->lh_Head;
							while( ( button = NextObject( &object_state ) ) && ( button != target_object ) )
							{
								pos++;
							}
							DoMethod( panel_group, OM_ADDMEMBER, ins_object );
							DoMethod( panel_group, MUIM_Group_MoveMember, ins_object, pos );
						}
					}
					DoMethod( panel_group, MUIM_Group_ExitChange );
				}
				if( panel_win && getv( panel_win, MUIA_Window_Open ) )
				{
					PDB(("setattrs\n"));
					SetAttrs( panel_win,
						MUIA_Window_Width, MUIV_Window_Width_Default,
						MUIA_Window_Height, MUIV_Window_Height_Default,
					TAG_DONE );
					/*
					SetAttrs( panel_win, MUIA_Window_Width , MUIV_Window_Width_Default , TAG_DONE );
					SetAttrs( panel_win, MUIA_Window_Height, MUIV_Window_Height_Default, TAG_DONE );
					*/
				}
			}
		}
		set(obj, MUIA_Listtree_Quiet, FALSE);	
	}
	return (0);
}
/************************************************************************/
   
DEFMMETHOD(DragReport)
{
	struct MUIS_Listtree_TestPos_Result r  = { NULL, 0, 0, 0 };
	struct MUIS_Listtree_TestPos_Result r2 = { NULL, 0, 0, 0 };
	struct MUIS_Listtree_TreeNode *tn;
//	  struct treedata *td;

	GETDATA;
PDB(("%d %d \n",msg->x,msg->y));
//	  if(msg->obj == obj)
	{

		/* Autoscroll list, when mouse is in upper or lower area */
		if( _isinobject( msg->x, msg->y ) )
		{
			if( ( msg->y + 10 ) > _top( obj ) + _height( obj ) )
			{
				if( !msg->update )
				{
					return( MUIV_DragReport_Refresh );
				}

				SetAttrs( obj, MUIA_List_TopPixel, getv(obj, MUIA_List_TopPixel ) + 10, TAG_DONE );
				return ( MUIV_DragReport_Continue );
			}
			else if( ( _top( obj ) + 20 ) > msg->y )
			{
				if( getv( obj, MUIA_List_TopPixel ) >= 10 )
				{
					if( !msg->update )
					{
						return( MUIV_DragReport_Refresh );
					}

					SetAttrs( obj, MUIA_List_TopPixel, getv( obj, MUIA_List_TopPixel ) - 10, TAG_DONE );
					return(MUIV_DragReport_Continue );
				}
			}
		}
		else
		{
			return( MUIV_DragReport_Continue );
		}

		// Sort mode
		if( DoMethod( obj, MUIM_Listtree_TestPos, msg->x, msg->y, &r ) && ( r.tpr_TreeNode ) )
		{
			ULONG lineheight;
			lineheight = getv( obj, MUIA_List_LineHeight );
			tn = (struct MUIS_Listtree_TreeNode *) r.tpr_TreeNode;

			if( tn->tn_Flags & TNF_LIST )
			{
				struct MUIS_Listtree_TreeNode *parent_node;
				parent_node = (APTR)DoMethod( obj, MUIM_Listtree_GetEntry,tn, MUIV_Listtree_GetEntry_Position_Parent );
				PDB(("%x %x\n",tn,parent_node));
				data->droppos = DROP_VOID;
				if(parent_node == 0) 
				{
					data->droppos = DROP_ONTO;
					DoMethod( obj, MUIM_Listtree_SetDropMark, r.tpr_ListEntry, MUIV_Listtree_SetDropMark_Values_Onto );
				} else {
					if(msg->y > lineheight/3)
					{
						DoMethod( obj, MUIM_Listtree_TestPos, msg->x, msg->y - lineheight/4, &r2 );
						if( r.tpr_TreeNode != r2.tpr_TreeNode )
						{
							DoMethod( obj, MUIM_Listtree_SetDropMark, r.tpr_ListEntry, MUIV_Listtree_SetDropMark_Values_Above );
							data->droppos = DROP_ABOVE;
						}
					}
					DoMethod( obj, MUIM_Listtree_TestPos, msg->x, msg->y + lineheight/4, &r2 );
					if( r.tpr_TreeNode != r2.tpr_TreeNode )
					{
						DoMethod( obj, MUIM_Listtree_SetDropMark, r.tpr_ListEntry, MUIV_Listtree_SetDropMark_Values_Below );
						data->droppos = DROP_BELOW;
					}
					if(data->droppos == DROP_VOID)
					{
						DoMethod( obj, MUIM_Listtree_SetDropMark, r.tpr_ListEntry, MUIV_Listtree_SetDropMark_Values_Onto );
						data->droppos = DROP_ONTO;
					}
				}
				
			} else {
				
			//	  PDB(("lien %d\n",lineheight));
				DoMethod( obj, MUIM_Listtree_TestPos, msg->x, msg->y + lineheight/2, &r2 );
			//	  PDB((" %x %x\n", r.tpr_TreeNode,r2.tpr_TreeNode));
				if( r.tpr_TreeNode == r2.tpr_TreeNode )
				{
					DoMethod( obj, MUIM_Listtree_SetDropMark, r.tpr_ListEntry, MUIV_Listtree_SetDropMark_Values_Above );
					data->droppos = DROP_ABOVE;
				} else {
					DoMethod( obj, MUIM_Listtree_SetDropMark, r.tpr_ListEntry, MUIV_Listtree_SetDropMark_Values_Below );
					data->droppos = DROP_BELOW;
				}

			}
#if 0
			if ( ( r.tpr_TreeNode != data->drop_node ) || ( r.tpr_Flags != data->drop_flags ) )
			{
				// Entry under mouse change need refresh
				if( !msg->update ) {
					return( MUIV_DragReport_Refresh );
				}

				data->drop_node = (struct MUIS_Listtree_TreeNode *) r.tpr_TreeNode;
				data->drop_flags = r.tpr_Flags;

				tn = (struct MUIS_Listtree_TreeNode *) r.tpr_TreeNode;
				if( tn )
				{
				//	  td = (struct treedata *) tn->tn_User;
					if( td )
					{
						if( ( data->drop_flags == MUIV_Listtree_TestPos_Result_Flags_Onto ) && !( data->drop_node->tn_Flags & TNF_LIST ) )
						{
							// Don't allow
							DoMethod( obj, MUIM_Listtree_SetDropMark, r.tpr_ListEntry, MUIV_Listtree_SetDropMark_Values_None );
							data->drop = NULL;
							//PDB(("###### Lock\n" ));
							return( MUIV_DragReport_Lock );
						} else {
							//PDB(("###### Draw\n"));
						//	  data->drop = tn;
							DoMethod( obj, MUIM_Listtree_SetDropMark, r.tpr_ListEntry, r.tpr_Flags );
						}
					}
					else
					{//PDB(( "###### DragReport: no td\n" ) );
					}
				}
				else
				{//PDB(( "###### DragReport: not en entry\n" ) );
				}
			}
#endif
		} else {
			//PDB(("###### DragReport: not en entry\n" ) );
			//put under
		}
	}
	return( MUIV_DragReport_Continue );
}

/************************************************************************/

DEFTMETHOD(PanelListTree_Refresh)
{
	GETDATA;
	APTR win;
	ULONG type,open;
	APTR treenode = 0;
	APTR tbar;
	APTR win_state;
	struct List *win_list;

	DoMethod( obj, MUIM_List_Clear );
	set( obj, MUIA_Listtree_Quiet, TRUE );

	GetAttr( MUIA_Application_WindowList, app, (ULONG*) &win_list );
	win_state = win_list->lh_Head;
	while( ( win = NextObject( &win_state ) ) )
	{
		if( ( GetAttr( MA_Panelwin_Type, win, &type ) ) && ( type == MV_Panelwin_Type_Root ) )
		{
			get( win, MUIA_Window_Open, &open );
			if( open )
			{
				GetAttr( MA_Panelwin_Group, win, (ULONG*) &tbar );
				treenode = (APTR) DoMethod( obj, MUIM_Listtree_Insert, getv( win, MA_Panelwin_Name ), win, MUIV_Listtree_Insert_ListNode_Root, MUIV_Listtree_Insert_PrevNode_Tail, TNF_LIST );
				ListPanel( obj, treenode, win );
				if( tbar )
				{
					DoMethod( tbar, MUIM_Notify, MA_Panelgroup_HasChanged, TRUE, obj, 2, MM_PanelListTree_RefreshNode, treenode );
				}
				DoMethod( win, MUIM_Notify, MA_Panelwin_Closed, TRUE          , data->prefswin, 2, MM_Prefswin_Panels_HasClosed,win);
				DoMethod( win, MUIM_Notify, MA_Panelwin_Name  , MUIV_EveryTime, obj           , 3, MUIM_Listtree_Rename, treenode, MUIV_TriggerValue, 0 );
			}
		}
	}
	set( obj, MUIA_Listtree_Quiet, FALSE );
	if( treenode ) {
		set( obj, MUIA_Listtree_Active, treenode ); /* set last panel as active */
	}
	return( 0 );
}

/************************************************************************/

DEFMMETHOD(DragQuery)
{
	ULONG x,y;
	APTR *active_node;
	struct MUIS_Listtree_TestPos_Result pos = { NULL, 0, 0, 0 };
	x = _window(obj)->MouseX;
	y = _window(obj)->MouseY;
	active_node =  (APTR) DoMethod( obj, MUIM_Listtree_GetEntry, obj        , MUIV_Listtree_GetEntry_Position_Active );
	if(msg->obj == obj)
	{
		if(( DoMethod( obj, MUIM_Listtree_TestPos, x, y, &pos ) && (pos.tpr_TreeNode != active_node)) ) return( MUIV_DragQuery_Accept );
		else return( MUIV_DragQuery_Refuse );
	}
	return( MUIV_DragQuery_Accept);
}

/************************************************************************/

DEFMMETHOD(Cleanup)
{
	GETDATA;
	int	c;

	for( c = PREFSWIN_NUMPAGES - 1 ; ( c > 0 ) ; c-- )
	{
		DoMethod( obj, MUIM_List_DeleteImage, data->images[c] );
		MUI_DisposeObject( data->iobjs[ c ] );
		gfx_bitmap_delete( data->ibm[ c ] );
	}
	return( DOSUPER );
}

/************************************************************************/

DEFSMETHOD(PanelListTree_CreateItem)
{
	GETDATA;
	ULONG type;
	APTR treenode = 0,subpanel;
	STRPTR oname;
	ASSERT( msg->panelobject );
	if( msg->parent )
	{
		get( msg->panelobject, MA_Panel_Extern_DisplayName, (ULONG*) &oname );
		if( get( msg->panelobject, MA_Panel_Type, &type ) )
		{
			switch( type )
			{
				case MV_Panel_Type_Spacer:
				case MV_Panel_Type_Separator:
				case MV_Panel_Type_ViewWatcher:
				case MV_Panel_Type_Bookmarks:
				case MV_Panel_Type_DirPanel:
				case MV_Panel_Type_External:
					treenode = (APTR) DoMethod( obj, MUIM_Listtree_Insert, oname, msg->panelobject, msg->parent, MUIV_Listtree_Insert_PrevNode_Tail, 0 );
					break;
				case MV_Panel_Type_Button:
					get( msg->panelobject, MA_Panel_URI, (ULONG*) &oname );
					treenode = (APTR) DoMethod( obj, MUIM_Listtree_Insert, oname, msg->panelobject, msg->parent, MUIV_Listtree_Insert_PrevNode_Tail, 0 );
					break;

				case MV_Panel_Type_SubPanel:
					treenode = (APTR) DoMethod( obj, MUIM_Listtree_Insert, oname, msg->panelobject, msg->parent, MUIV_Listtree_Insert_PrevNode_Tail, TNF_LIST/*|TNF_OPEN*/ );
					get( msg->panelobject, MA_Panelbutton_AttachedObject, (ULONG*) &subpanel );
					if(subpanel)
					{
						APTR group;
						GetAttr( MA_Panelwin_Group, subpanel, (ULONG*) &group );
						if(group)
						{
							DoMethod( group, MUIM_Notify, MA_Panelgroup_HasChanged, TRUE, obj, 2, MM_PanelListTree_RefreshNode, treenode );
						}
						ListPanel( obj, treenode, subpanel );
					}
					break;

			}
		}
	}
	else if( ( get( msg->panelobject, MA_Panelwin_Type, &type ) ) && ( type == MV_Panelwin_Type_Root ) )
	{
		ULONG open;
		APTR tbar;
		STRPTR pname;

		get( msg->panelobject, MUIA_Window_Open, &open );
		if( open )
		{
			GetAttr( MA_Panelwin_Name , msg->panelobject, (ULONG*) &pname );
			GetAttr( MA_Panelwin_Group, msg->panelobject, (ULONG*) &tbar );
			treenode = (APTR) DoMethod( obj, MUIM_Listtree_Insert, pname, msg->panelobject, MUIV_Listtree_Insert_ListNode_Root, MUIV_Listtree_Insert_PrevNode_Tail, TNF_LIST );
			ListPanel( obj, treenode, msg->panelobject );
			if(tbar)
			{
				DoMethod( tbar, MUIM_Notify, MA_Panelgroup_HasChanged, TRUE,obj, 2, MM_PanelListTree_RefreshNode, treenode );
			}
			set( obj, MUIA_Listtree_Active, treenode );
			DoMethod( msg->panelobject, MUIM_Notify, MA_Panelwin_Closed, TRUE          , data->prefswin, 2, MM_Prefswin_Panels_HasClosed, msg->panelobject );
			DoMethod( msg->panelobject, MUIM_Notify, MA_Panelwin_Name  , MUIV_EveryTime, obj           , 3, MUIM_Listtree_Rename, treenode, MUIV_TriggerValue, 0 );
		}
	}
	return (ULONG)treenode;
}

/************************************************************************/

DEFMMETHOD(Listtree_Insert)
{
	ULONG type;

	if( ( get( msg->User, MA_Panel_Type, &type ) ) && ( msg->Name == 0 ) )
	{
		switch( type )
		{
			case MV_Panel_Type_Spacer:
			case MV_Panel_Type_Separator:
			case MV_Panel_Type_ViewWatcher:
			case MV_Panel_Type_Bookmarks:
			case MV_Panel_Type_DirPanel:
			case MV_Panel_Type_SubPanel:
			case MV_Panel_Type_External:
				get( msg->User, MA_Panel_Extern_DisplayName, (ULONG*) &msg->Name );
				break;

			case MV_Panel_Type_Button:
				get( msg->User, MA_Panel_URI, (ULONG*) &msg->Name );
				break;
		}
	}
	return( DOSUPER );
}

/************************************************************************/

DEFSMETHOD(PanelListTree_RefreshNode)
{
	APTR panel = NULL;
	struct MUIS_Listtree_TreeNode *node = (struct MUIS_Listtree_TreeNode*) msg->treenode;

	if (node)
	{
		SetAttrs( obj, MUIA_Listtree_Quiet, TRUE, TAG_DONE );
		if( !( get( node->tn_User, MA_Panelbutton_AttachedObject, (ULONG*) &panel ) ) )
		{
			panel = node->tn_User;
		}
		DoMethod( obj, MUIM_Listtree_Remove, node, MUIV_Listtree_Remove_TreeNode_All, 0 );
		ListPanel( obj, node, panel );
		SetAttrs( obj, MUIA_Listtree_Quiet, FALSE, TAG_DONE );
	}

	return( 0 );
}

/************************************************************************/

DEFSMETHOD(PanelListTree_FindUData)
{
	ULONG n = 0;
	ULONG c;
	struct MUIS_Listtree_TreeNode *node;

	get( obj, MUIA_List_Entries, &c );
	while( ( node = (struct MUIS_Listtree_TreeNode*) DoMethod( obj, MUIM_Listtree_GetEntry, 0, n, 0 ) ) )
	{
		if( node->tn_User == msg->UData ) {
			return( (ULONG) node );
		}
		n++;
	}

	return( 0 );
}

/************************************************************************/
  
BEGINMTABLE
DECNEW
DECMMETHOD(DragReport)
DECMMETHOD(DragQuery)
DECMMETHOD(DragDrop)
DECMMETHOD(Setup)
DECTMETHOD(PanelListTree_Refresh)
DECSMETHOD(PanelListTree_CreateItem)
DECSMETHOD(PanelListTree_RefreshNode)
DECSMETHOD(PanelListTree_FindUData)
DECMMETHOD(Listtree_Insert)
DECMMETHOD(Cleanup)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Listtree, panellisttreeclass)
#endif
