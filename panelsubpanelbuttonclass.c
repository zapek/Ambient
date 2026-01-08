
/* ANSI C */
#include <stdlib.h>
#include <string.h>

/* System */
#include <dos/dos.h>
#include <graphics/gfxmacros.h>
#include <workbench/workbench.h>


#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/utility.h>
#include <proto/asl.h>
#include <proto/muimaster.h>
#include <proto/cybergraphics.h>
#include <datatypes/pictureclass.h>
#include <cybergraphx/cybergraphics.h>
#include <clib/datatypes_protos.h>

#include "ambient.h"
#include "classes.h"


#include "ambient_cat.h"
#include "rexx.h"
#include "command.h"
#include "contextmenu.h"
#include "iconio.h"
#include "name.h"
#include "gfx_scale.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "gfx_pen.h"
#include "prefs.h"
#include "datatypes_picture.h"
#include "wbstart.h"
#include "threads.h"
#include "file_func.h"
#include "dragdrop.h"
#include "legacy.h"
#include "paneltags.h"

/************************************************************************/

static TEXT defaultImagepath[]  = "sys:tools.info";

struct Data
{
	ULONG ID;
	APTR subpanel;
};

/************************************************************************/

static void doset( APTR obj UNUSED, struct Data *data, struct TagItem *tags )
{
	FORTAG( tags )
	{
		case MA_SubPanel_ID:
			data->ID = tag->ti_Data;
			break;
		case MA_Panelbutton_AttachedObject:
			data->subpanel = (APTR) tag->ti_Data;
			break;
		case MA_PanelZipLock:
		//	PDB(("%d\n",tag->ti_Data));
			break;
	}
	NEXTTAG
}

/************************************************************************/

DEFNEW
{
	if( ( obj = (APTR) DoSuperNew(cl, obj, TAG_MORE, msg->ops_AttrList ) ) )
	{
		struct Data *data = INST_DATA( cl, obj );

		if( ( data->subpanel = NewObject( getpanelsubwinclass(), NULL,
							MA_Panelwin_Type        , MV_Panelwin_Type_SubPanel,
							MA_Panelwin_ParentObject, obj,
							MA_Panelgroup_Size      , 48,
							TAG_DONE ) ) )
		{
			DoMethod( app, OM_ADDMEMBER, data->subpanel );
			data->ID = 0;

			doset( obj, data, msg->ops_AttrList );
			set( obj, MUIA_ShortHelp, GSI(MSG_PANELITEM_SUBPANEL_SHORTHELP) );
		} else {
			MUI_DisposeObject( obj );
			obj = NULL;
		}
	}
	return( (ULONG) obj );
}

/************************************************************************/

DEFMMETHOD(Setup)
{
	GETDATA;
	APTR group;

	group = (APTR) getv( data->subpanel, MA_Panelwin_Group );

	DoMethod( _parent(obj), MUIM_Notify, MA_Panel_HasMoved , MUIV_EveryTime, data->subpanel, 2, MM_Panelsubwin_Open, FALSE );

	DoMethod( _parent(obj), MUIM_Notify, MA_Panelgroup_Size, MUIV_EveryTime, data->subpanel, 2, MM_Panelsubwin_Open, FALSE );

	DoMethod( _parent(obj), MUIM_Notify, MA_Panelgroup_Size, MUIV_EveryTime, group, 3, MUIM_Set, MA_Panelgroup_Size, MUIV_TriggerValue );

	return( DOSUPER );
}

/************************************************************************/

DEFMMETHOD(DragDrop)
{
	GETDATA;
    STRPTR path;
	Object *button, *tbar;

	if( msg->obj && data->subpanel )
	{
		if( ( path = (STRPTR) getv( msg->obj, MA_Icon_PathInfo ) ) )
		{
			if( ( tbar = (Object *) getv( data->subpanel, MA_Panelwin_Group ) ) )
			{
				if( ( button = NewObject( getpanelcommandbuttonclass(), NULL, TAG_DONE ) ) )
				{
					DoMethod( button,  MM_Panelbutton_AddIcon, msg->obj );
					DoMethod( tbar, MUIM_Group_InitChange );
					DoMethod( tbar, OM_ADDMEMBER, button );
					DoMethod( tbar, MUIM_Group_ExitChange );
					SetAttrs( data->subpanel, MUIA_Window_Width , MUIV_Window_Width_MinMax(0) , TAG_DONE );
					SetAttrs( data->subpanel, MUIA_Window_Height, MUIV_Window_Height_MinMax(0), TAG_DONE );
					set( tbar, MA_Panelgroup_HasChanged, TRUE );
			 	}
			}
	    }
	}
	return( 0 );
}

/************************************************************************/
#if 0
DEFMMETHOD(DragQuery)
{
	return( MUIV_DragQuery_Accept );
}
#endif
/************************************************************************/

DEFGET
{
	GETDATA;
	ULONG result = TRUE;

	switch( msg->opg_AttrID )
	{
		case MA_Panel_Type:
			*msg->opg_Storage = MV_Panel_Type_SubPanel;
			break;
		case MA_Panelbutton_AttachedObject:
			*msg->opg_Storage = (ULONG) data->subpanel;
			break;
		case MA_SubPanel_ID:
			*msg->opg_Storage = data->ID;
			break;
		case MA_Panelbutton_DefaultImagePath:
			*msg->opg_Storage = (ULONG) defaultImagepath;
			break;
		case MA_Panel_Extern_DisplayName:
			*msg->opg_Storage = (ULONG) GSI(MSG_PANELITEM_SUBPANEL);
			break;
		case MA_Panel_Extern_Version:
			*msg->opg_Storage = 1;
			break;
		case MA_Panel_Extern_Revision:
			*msg->opg_Storage = 0;
			break;
		case MA_Panel_Extern_Author:
			*msg->opg_Storage = (ULONG) "Stefan Kleinheinrich,\nAmbient Open Source Team";
			break;
		case MA_Panel_Extern_Description:
			*msg->opg_Storage = (ULONG) GSI(MSG_PANELITEM_SUBPANELDESC);
			break;
		case MA_PanelZipLock:
			{
				ULONG panelwinopen;
				get( data->subpanel,MUIA_Window_Open,&panelwinopen);
				*msg->opg_Storage = panelwinopen;
			} 
			break;
		default:
			result = DOSUPER;
			break;
	}
	return( result );
}

/************************************************************************/

DEFSET
{
	GETDATA;

	doset( obj, data, INITTAGS );

	return( DOSUPER );
}

/************************************************************************/

DEFTMETHOD(Panelbutton_Launch)
{
    GETDATA;

	if( data->subpanel ) {
		DoMethod( data->subpanel, MM_Panelsubwin_Open, TRUE, TAG_DONE );
    }
	return( 0 );
}

/************************************************************************/

DEFTMETHOD(Panelbutton_Close)
{
	GETDATA;
	PDB(("buttonclose \n"));
	set(data->subpanel,MA_Panelsubwin_StayOpen,FALSE);
	DoMethod( data->subpanel, MM_Panelsubwin_Open, FALSE, TAG_DONE );
	return( 0 );
}

/************************************************************************/



DEFSMETHOD(Panel_SaveConfig)
{
	APTR pl, pi; /* list, item */
	APTR tbar;
	ULONG index = 1;
	ULONG type;
	ULONG backgroundmode;
	GETDATA;
	ASSERT(msg->pctx);
	GetAttr(MA_Panelwin_Group,data->subpanel,(ULONG*)&tbar);
	get(tbar ,MA_Panelgroup_BackMode,&backgroundmode);
	if (!(pl = prefspool_item_get(msg->pctx, NULL, DSI_LISTPOOL_PANEL, NULL, NULL)))
	{
		pl = prefspool_item_add(msg->pctx, NULL, DSI_LISTPOOL_PANEL, NULL, NULL);
	}
	PDB(("%x\n",data->ID));
	if (pl)
	{
		if (!(pi = prefspool_item_get(msg->pctx, pl, msg->index | DSF_LISTPOOL, NULL, NULL)))
		{
			pi = prefspool_item_add(msg->pctx, pl, msg->index | DSF_LISTPOOL, NULL, NULL);
		}
		if (pi)
		{
			STRPTR imagepath;
			if( get( obj, MA_Panel_Imagepath,&imagepath ) )
			{
				/* XXX: we should check the retvals or so */
				/* XXX: perhaps we should just delete the prefs item instead of setting it to nothing */
				setprefsstr_lp( msg->pctx, pi, DSI_LISTPOOL_PANEL_IMAGEPATH, imagepath ? imagepath : (STRPTR) "" );
				data->ID = msg->index+1;
				setprefslong_lp( msg->pctx, pi, DSI_LISTPOOL_PANEL_SUBPANEL_ROOT ,data->ID );
				PDB(("sub id %d\n",data->ID));
				setprefslong_lp( msg->pctx, pi, DSI_PANELGROUP_BACKMODE          , backgroundmode );
				setprefslong_lp( msg->pctx, pi, DSI_PANELGROUP_BACKCOLOR         , getv( tbar, MA_Panelgroup_BackColor ) );
				if( backgroundmode == MV_Panelgroup_BackMode_Picture )
				{
					setprefsstr_lp( msg->pctx, pi, DSI_PANELGROUP_BACKDROP, (STRPTR) getv( tbar, MA_Panelgroup_Backdrop ) );
				}
				FORCHILD( tbar, MUIA_Group_ChildList )
				{
					PDB(("child %d\n",data->ID));
					if( !( pi = prefspool_item_get( msg->pctx, pl, ( msg->index + index ) | DSF_LISTPOOL, NULL, NULL ) ) )
					{
						pi = prefspool_item_add( msg->pctx, pl, ( msg->index + index ) | DSF_LISTPOOL, NULL, NULL );
					}
					if(pi)
					{
						type = getv( child, MA_Panel_Type );
						PDB(("%x %d\n",type,data->ID));
						setprefslong_lp( msg->pctx, pi, DSI_LISTPOOL_PANEL_SUBPANEL, data->ID );
						if( type && ( type != MV_Panel_Type_External ) )
						{
							setprefslong_lp( msg->pctx, pi, DSI_LISTPOOL_PANEL_TYPE, type );
							index += DoMethod( child, MM_Panel_SaveConfig, msg->pctx, msg->index + index );
						}
						else /* o.k. must be an external class, needs better detecting */
						{
							APTR supobj;
							STRPTR classname;
							
							classname = (APTR) getv( child, MA_Panel_Extern_ClassName );

							setprefslong_lp( msg->pctx, pi, DSI_LISTPOOL_PANEL_TYPE    , MV_Panel_Type_External );
							setprefsstr_lp ( msg->pctx, pi, DSI_LISTPOOL_PANEL_EXT_NAME, classname ? classname : (STRPTR) "" );
							if( ( get( child, MA_Panelextern_SupportObject, (ULONG*) &supobj ) ) && ( supobj ) )
							{
								DoMethod( supobj, MM_Panelsupport_Saveconfig, msg->pctx, pi );
							}
						}
					}
					/* XXX */
					index++;
				}
				NEXTCHILD
			}
		}
		/* XXX */
	}
	/* XXX */
	return( index - 1 );
}

/************************************************************************/

DEFTMETHOD(Panel_Settings_Group)
{
	APTR group;
	GETDATA;
	group = VGroup,
		Child,VSpace(0),
		Child, HGroup, GroupFrameT( GSI(MSG_PANELSUBWIN_BUTTON_FRAME) ),
			Child,DOSUPER,
		End,
		Child,VSpace(0),
		Child, HGroup, GroupFrameT( GSI(MSG_PANELSUBWIN_PANEL_FRAME) ),
			Child, DoMethod( data->subpanel, MM_Panel_Settings_Group ),
		End,
		Child,VSpace(0),
	End;
	return( (ULONG) group );
}

/************************************************************************/

BEGINMTABLE
DECNEW
DECGET
DECSET
DECMMETHOD(Setup)
DECMMETHOD(DragDrop)
//DECMMETHOD(DragQuery)
DECTMETHOD(Panelbutton_Launch)
DECTMETHOD(Panelbutton_Close)
DECSMETHOD(Panel_SaveConfig)
DECTMETHOD(Panel_Settings_Group)
ENDMTABLE



DECSUBCLASSPTR_NC(panelbasebuttonclass,  panelsubpanelbuttonclass)
