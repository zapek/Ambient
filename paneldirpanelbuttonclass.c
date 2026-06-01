/* ANSI C */
#include <stdlib.h>
#include <string.h>

/* System */
#include <dos/dos.h>
#include <graphics/gfxmacros.h>
#include <workbench/workbench.h>
#include <libraries/mui.h>


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

#include "ambient_cat.h"
#include "ambient.h"
#include "classes.h"
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

#if USE_INTERNAL_PANELS

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
		case MA_Panel_URI:
			if( data->subpanel )
			{
				set( data->subpanel, MA_Panelsubwin_Dir, tag->ti_Data );
			}
			break;

	}
	NEXTTAG
}

/************************************************************************/

DEFNEW
{
	if( ( obj = (APTR) DoSuperNew( cl, obj, TAG_MORE  ,msg->ops_AttrList ) ) )
	{
		struct Data *data = INST_DATA( cl, obj );

		if( ( data->subpanel = NewObject( getpanelsubwinclass(), NULL,
													MA_Panelwin_Type        , MV_Panelwin_Type_DirPanel,
													MA_Panelwin_ParentObject, obj,
													TAG_DONE ) ) )
		{
			DoMethod( app, OM_ADDMEMBER, data->subpanel );
			set( obj, MUIA_ShortHelp, GSI(MSG_PANELDIRPANELBUTTON_SHORTHELP) );
		}
		SetAttrs( obj, TAG_MORE, msg->ops_AttrList, TAG_DONE );
	}
	return( (ULONG) obj );
}

/************************************************************************/

DEFMMETHOD(DragDrop)
{
	STRPTR path, iconpath;
	ULONG type;

	ASSERT( msg->obj );

	type = getv( msg->obj, MA_Icon_Type );

	if( type == MV_Icon_Type_Drawer )
	{
		if(    ( get(msg->obj, MA_Icon_PathInfo, &iconpath ) )
			&& ( get(msg->obj, MA_Icon_Path    , &path ) ) )
		{
			SetAttrs( obj, MA_Panel_URI, path, MA_Panel_Imagepath, iconpath, TAG_DONE );
			MUI_Redraw( _parent( obj ), MADF_DRAWUPDATE );
		}
	}
	return( 0 );
}

/************************************************************************/

DEFMMETHOD(DragQuery)
{
	return( MUIV_DragQuery_Accept );
}

/************************************************************************/

DEFGET
{
	GETDATA;
	ULONG result = TRUE;

	switch( msg->opg_AttrID )
	{
		case MA_Panel_Type:
			*msg->opg_Storage = MV_Panel_Type_DirPanel;
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
			*msg->opg_Storage = (ULONG) GSI(MSG_PANELITEM_DIRPANEL);
			break;
		case MA_Panel_Extern_Version:
			*msg->opg_Storage = 1;
			break;
		case MA_Panel_Extern_Revision:
			*msg->opg_Storage = 1;
			break;
		case MA_Panel_Extern_Author:
			*msg->opg_Storage = (ULONG) "Stefan Kleinheinrich,\nAmbient Open Source Team";
			break;
		case MA_Panel_Extern_Description:
			*msg->opg_Storage = (ULONG) GSI(MSG_PANELITEM_DIRPANELDESC);
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

	PDB(("Panelbutton_Launch\n"));
	if( data->subpanel )
	{
		APTR parent, tbar;
		ULONG horiz, size;

		GetAttr( MUIA_Parent, obj, (ULONG*) &parent );

		if( parent )
		{
			GetAttr( MA_Panelgroup_Horiz, parent, &horiz);
			GetAttr( MA_Panelgroup_Size, parent, &size);
			GetAttr( MA_Panelwin_Group, data->subpanel, (ULONG*) &tbar );
			horiz = ~horiz & 0x00000001;
		}
	PDB(("Panelbutton_Launch - panelsubwin\n"));

		DoMethod( data->subpanel, MM_Panelsubwin_Open, TRUE, TAG_DONE );

	PDB(("Panelbutton_Launch - subpanel\n"));

		SetAttrs( tbar, MA_Panelgroup_Size , size , TAG_DONE );
		SetAttrs( tbar, MA_Panelgroup_Horiz, horiz, TAG_DONE );

		DoMethod( data->subpanel, MM_Panelwin_Placement, MV_Panelsubwin_Default, 0 );

	PDB(("Panelbutton_Launch - subpanel - done\n"));
    }
	return( 0 );
}

/************************************************************************/

DEFTMETHOD(Panelbutton_Close)
{
	GETDATA;
	set(data->subpanel,MA_Panelsubwin_StayOpen,FALSE);
	DoMethod( data->subpanel, MM_Panelsubwin_Open, FALSE, TAG_DONE );
	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Panel_SaveConfig)
{
	GETDATA;
	APTR pl, pi; /* list, item */
	ULONG index =1;
	ASSERT(msg->pctx);

	PDB(("Panelbutton_SaveConfig\n"));

	if( !( pl = prefspool_item_get( msg->pctx, NULL, DSI_LISTPOOL_PANEL, NULL, NULL ) ) )
	{
		pl = prefspool_item_add( msg->pctx, NULL, DSI_LISTPOOL_PANEL, NULL, 0 );
	}

	if( pl )
	{
		if( !( pi = prefspool_item_get( msg->pctx, pl, msg->index | DSF_LISTPOOL, NULL, NULL ) ) )
		{
			pi = prefspool_item_add( msg->pctx, pl, msg->index | DSF_LISTPOOL, NULL, 0 );
		}
		if( pi )
		{
			STRPTR imagepath,uri;
			APTR tbar;
			ULONG backgroundmode;

			GetAttr( MA_Panelwin_Group, data->subpanel, (ULONG*) &tbar );
			get( tbar, MA_Panelgroup_BackMode, &backgroundmode );

			setprefslong_lp( msg->pctx, pi, DSI_PANELGROUP_BACKMODE , backgroundmode );
			setprefslong_lp( msg->pctx, pi, DSI_PANELGROUP_BACKCOLOR, getv( tbar, MA_Panelgroup_BackColor ) );
			setprefsstr_lp ( msg->pctx, pi, DSI_PANELGROUP_BACKDROP , (STRPTR) getv( tbar, MA_Panelgroup_Backdrop ) );

			if( ( get(obj, MA_Panel_Imagepath, &imagepath ) ) && ( get(obj, MA_Panel_URI, &uri ) ) )
			{
			/* XXX: we should check the retvals or so */
			/* XXX: perhaps we should just delete the prefs item instead of setting it to nothing */
				setprefsstr_lp( msg->pctx, pi, DSI_LISTPOOL_PANEL_IMAGEPATH, imagepath ? imagepath : (STRPTR) "" );
				setprefsstr_lp( msg->pctx, pi, DSI_LISTPOOL_PANEL_URI      , uri ? uri : (STRPTR) "" );
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
	GETDATA;
	APTR group;
	APTR dirasl, dirasl_str;
	STRPTR dir_name;

	GetAttr( MA_Panelsubwin_Dir, data->subpanel, (ULONG*) &dir_name );

	if( ( group = VGroup,
		Child,VSpace(0),
		Child, HGroup, GroupFrameT("Button"),
			Child,DOSUPER,
		End,
		Child,VSpace(0),
		Child, HGroup, GroupFrameT("Panel"),
			Child, DoMethod( data->subpanel, MM_Panel_Settings_Group ),
		End,
		Child,VSpace(0),
		Child, HGroup,
			Child, NSLabel1(MSG_PANELDIRPANELBUTTON_DIRECTORY),
			Child, dirasl = PopaslObject, ASLFR_Flags1, FRF_DRAWERSONLY, ASLFR_DrawersOnly,TRUE,
										MUIA_Popstring_String, dirasl_str = KeyString( dir_name, 60, "n" ),
										MUIA_Popstring_Button, PopButton( MUII_PopUp ),
			End,
		End,
	End ) )
	{
		DoMethod( dirasl_str, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 3, MUIM_Set, MA_Panel_URI, MUIV_TriggerValue );
	}
	return( (ULONG) group );
}

/************************************************************************/

BEGINMTABLE
DECNEW
DECGET
DECSET
DECMMETHOD(DragDrop)
DECMMETHOD(DragQuery)
DECTMETHOD(Panelbutton_Launch)
DECTMETHOD(Panelbutton_Close)
DECSMETHOD(Panel_SaveConfig)
DECTMETHOD(Panel_Settings_Group)
ENDMTABLE

DECSUBCLASSPTR_NC(panelbasebuttonclass,  paneldirpanelbuttonclass)
#endif
