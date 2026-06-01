#include "ambient.h"

#if USE_INTERNAL_PANELS

#include <proto/gadtools.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <proto/graphics.h>
#include <proto/muimaster.h>
/* public */

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "screen.h"
#include "prefs.h"
#include "threads.h"
#include "panelprefs.h"
#include "dosnotify.h"
#include "notify.h"
#include "legacy.h"
#include "paneltags.h"
#include "name.h"

#include "classes.h"

/************************************************************************/

struct Data
{
	ULONG                       ID;
	ULONG                       type;
	APTR                        subpanel;
	STRPTR                      directory;
	ULONG                       stayopen;
	ULONG                       placement;
	APTR                        group;
	APTR                        parent;
	APTR                        nctx;
	struct TimeVal              open_time;
	BOOL                        timeout;
	struct MUI_InputHandlerNode ihnode;
};

/************************************************************************/


static void doset( APTR obj, struct Data *data, struct TagItem *tags )
{
	FORTAG( tags )
	{
		case MA_Panelwin_Type:
			data->type = tag->ti_Data;
			break;
		case MA_Panelwin_ParentObject:
			data->parent = (APTR) tag->ti_Data;
			break;
		case MA_Panelsubwin_StayOpen:
			data->stayopen = tag->ti_Data;
			break;
		case MA_Panelsubwin_Dir:
			if( ( data->directory = name_build_sysify( (STRPTR) tag->ti_Data ) ) )
			{
				DoMethod( obj, MM_Panelsubwin_ParseDir, data->directory );
				if( data->nctx )
				{
					notify_delete( data->nctx );
				}
				data->nctx = notify_create();
				if( data->nctx )
				{
					#define MONITORFILEPATERN_SIZEOF 300
					TEXT pattern[ MONITORFILEPATERN_SIZEOF ];
					sprintf( pattern,"%s*",data->directory );
					notify_register( data->nctx,
											NOTIFYTAG_Monitor_File       , pattern,
											NOTIFYTAG_Monitor_File_Name  , TRUE,
											NOTIFYTAG_Monitor_File_Create, TRUE,
											NOTIFYTAG_Monitor_File_Delete, TRUE,
											NOTIFYTAG_Inform_Object      , obj,
											TAG_DONE );
				}
			}
		break;
		case MUIA_Window_MouseObject:
			if( tag->ti_Data )
			{
				if( data->timeout ) {
					DoMethod( _app(obj), MUIM_Application_RemInputHandler, &data->ihnode );
				}
				data->timeout = FALSE;
			} else {
				if( !( data->timeout ) )
				{
					DoMethod( _app(obj), MUIM_Application_AddInputHandler, &data->ihnode );
					data->timeout = TRUE;
				}
			}
			break;
	}
	NEXTTAG
}

/************************************************************************/

DEFNEW
{
	APTR grp;

	if( ( obj = DoSuperNew( cl, obj,
									MUIA_Window_Screen      , get_screen(),
									MUIA_Window_ScreenTitle , screentitle,
									MUIA_Window_CloseGadget , FALSE,
									MUIA_Window_Borderless  , TRUE,
									MUIA_Window_CloseGadget , FALSE,
									MUIA_Window_DepthGadget , FALSE,
									MUIA_Window_SizeGadget  , FALSE,
									MUIA_Window_DragBar     , FALSE,
									MUIA_Window_BackfillHook, LAYERS_NOBACKFILL,
									MUIA_Window_HasAlpha    ,TRUE,
									( EnableExtensions ? MUIA_Window_PanelWindow : TAG_IGNORE ), TRUE,
									WindowContents,  grp = NewObject( getpanelgroupclass(), NULL,
																		MUIA_Group_Horiz, TRUE,
																		MUIA_FillArea, FALSE,
																		MA_Panelgroup_DragMode, MV_Panelgroup_DragMode_None,
																		TAG_MORE, msg->ops_AttrList,
																		TAG_DONE ),
									TAG_DONE ) ) )
	{
		GETDATA;
		data->ihnode.ihn_Object  = obj;
		data->ihnode.ihn_Flags   = MUIIHNF_TIMER;
		data->ihnode.ihn_Method  = MM_Panelsubwin_Timer;
		data->ihnode.ihn_Millis  = 3000;
		data->group = grp;
		data->nctx  = 0;
		data->directory = 0;
		SetAttrs( obj, TAG_MORE, msg->ops_AttrList, TAG_DONE );
	}
	return( (ULONG) obj );
}

/************************************************************************/

DEFGET
{
	GETDATA;
	ULONG result = TRUE;
	switch( msg->opg_AttrID )
	{
		case MA_Panelwin_Group:
			*msg->opg_Storage = (ULONG) data->group;
			break;
		case MA_Panelwin_Type:
			*msg->opg_Storage = data->type;
			break;
		case MA_Window_Type:
			*msg->opg_Storage =  MV_Window_Type_Panel;
			break;
		case MA_Panelsubwin_Dir:
			*msg->opg_Storage =  (ULONG) data->directory;
			break;
		case MA_Panelsubwin_StayOpen:
			*msg->opg_Storage = data->stayopen;
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

#define CORNER_ATTRACTION 6 /* percent of the screen size */

DEFTMETHOD(Panelsubwin_ParseDir)
{
	GETDATA;
	BPTR conflock;
	struct ExAllControl *eac;

	DoMethod( data->group, MM_Panelgroup_Clear );

	if( ( conflock = Lock( data->directory, ACCESS_READ ) ) )
	{
		if( ( eac = (struct ExAllControl*) AllocDosObject( DOS_EXALLCONTROL, NULL ) ) )
		{
			ULONG e = TRUE;
			struct ExAllData *ead;
			#define PARSEDIR_BUFFER_SIZEOF 1000
			#define PARSEDIR_PATH_SIZEOF 200
			#define PARSEDIR_INFO_LENGTH 5
			char confListBuffer[ PARSEDIR_BUFFER_SIZEOF ];
			char iconpath[ PARSEDIR_PATH_SIZEOF ];
			char path[ PARSEDIR_PATH_SIZEOF ];

			eac->eac_LastKey = 0;
			while( e )
			{
				e = ExAll( conflock, (struct ExAllData*) confListBuffer, PARSEDIR_BUFFER_SIZEOF, ED_NAME, eac );
				if( eac->eac_Entries != 0 )
				{
					ead = (struct ExAllData*) confListBuffer;
					while( (ead != NULL ) && ( ead->ed_Name ) )
					{
						if( ( name_isinfo( ead->ed_Name ) ) )
						{
							APTR button;
							strcpy( iconpath, data->directory );
							AddPart( iconpath, ead->ed_Name, PARSEDIR_PATH_SIZEOF );
							strncpy( path, iconpath, strlen( iconpath ) - PARSEDIR_INFO_LENGTH );
							path[ strlen( iconpath ) - PARSEDIR_INFO_LENGTH ] = '\0';
							if( ( button =  NewObject( getpanelcommandbuttonclass(), NULL,
												MA_Panel_URI      , path,
												MA_Panel_Imagepath, iconpath,
												TAG_DONE ) ) )
							{
								DoMethod( data->group, OM_ADDMEMBER, button );
							}
						}
						ead = ead->ed_Next;
					}
				}
			}
			FreeDosObject( DOS_EXALLCONTROL, eac );
		}
		UnLock( conflock );
	}
 return( 0 );
}

/************************************************************************/

DEFTMETHOD(Panelwin_SaveConfig)
{
	APTR rootwin;
	if( ( rootwin = (APTR) DoMethod( obj, MM_Panelwin_FindRoot ) ) )   /*we should have a valid rootwin at this point*/
	{
		DoMethod( rootwin, MM_Panelwin_SaveConfig );
	}
	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Panelsubwin_Open)
{
	GETDATA;
    struct TimeVal currenttime;
	BOOL winclose = FALSE;

	if( msg->open )
	{
		if( getv( obj, MUIA_Window_Open ) )
		{
			if (TimerBase->lib_Version >= 52)
				GetUTCSysTime( &currenttime );
			else
				GetSysTime( &currenttime );
			SubTime( &currenttime, &data->open_time );
#warning "Since open_time is always 0 this is always true -- BUG?"
			if( currenttime.tv_secs > 0 )
			{
				winclose = TRUE;
			}
			data->stayopen = FALSE;
		} else {
			set( obj, MUIA_Window_Open, TRUE );
			set( obj, MUIA_Window_Activate, TRUE );
			DoMethod( _app(obj), MUIM_Application_AddInputHandler, &data->ihnode );
			data->timeout = TRUE;
		}
	} else {
		if (TimerBase->lib_Version >= 52)
			GetUTCSysTime( &currenttime );
		else
			GetSysTime( &currenttime );
		SubTime( &currenttime, &data->open_time );
		if( !( data->stayopen ) )
		{
			winclose = TRUE;
		}
	}
	if(winclose)
	{
		if( data->timeout )
		{
			DoMethod( _app(obj), MUIM_Application_RemInputHandler, &data->ihnode );
		}
    	data->timeout = FALSE;
		FORCHILD( data->group, MUIA_Group_ChildList )
		{
			DoMethod(child,MM_Panelbutton_Close);
		}
		NEXTCHILD
		set( obj, MUIA_Window_Open, FALSE );
		set(data->parent,MA_PanelZipLock,FALSE);
	}
	return( 0 );
}

/************************************************************************/

DEFTMETHOD(Panelwin_FindRoot)
{
	GETDATA;

	if( data->parent ) {
		return( DoMethod( _win( data->parent ), MM_Panelwin_FindRoot ) );
	}
	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Panelwin_Placement)
{
	GETDATA;
	ULONG winleftedge, wintopedge;
	ULONG x, y, w, h, horiz;
	ULONG open;


	get( obj, MUIA_Window_Window, &open );

	if( open ) /* this one is evil! It make no sense, but removing kills Ambient on click (geit, KRONOS_TAG) */
	{
		set( obj, MUIA_Window_Open, FALSE );
	}
	GetAttr( MA_Panelgroup_Horiz , data->group,&horiz );
	GetAttr( MUIA_Window_TopEdge , _win(data->parent), &wintopedge );
	GetAttr( MUIA_Window_LeftEdge, _win(data->parent), &winleftedge );
	GetAttr( MUIA_LeftEdge       , data->parent, &x );
	GetAttr( MUIA_TopEdge        , data->parent, &y );
	GetAttr( MUIA_Width          , data->parent, &w );
	GetAttr( MUIA_Height         , data->parent, &h );

	if( msg->placement == MV_Panelsubwin_Default )
	{
		msg->placement = data->placement;
	}
	if( msg->placement == 0 )
	{
		if( horiz )
		{
			msg->placement = MV_Panelsubwin_Left;
		} else {
			msg->placement = MV_Panelsubwin_Top;
		}
	}
	
	switch( msg->placement )
	{
		case MV_Panelsubwin_Left:
			if( msg->width > winleftedge )
			{
				if( msg->attempt < 2 )
				{
					DoMethod( obj, MM_Panelwin_Placement, MV_Panelsubwin_Right, msg->attempt + 1 );
				}
			} else {
				SetAttrs( obj, MUIA_Window_TopEdge , wintopedge + y, TAG_DONE );
				SetAttrs( obj, MUIA_Window_LeftEdge, winleftedge - msg->width,TAG_DONE );
			}
			break;
		case MV_Panelsubwin_Right:
			SetAttrs( obj, MUIA_Window_TopEdge , wintopedge  + y, TAG_DONE );
			SetAttrs( obj, MUIA_Window_LeftEdge, winleftedge + w, TAG_DONE );
			break;
		case MV_Panelsubwin_Top:
			if( msg->height > wintopedge )
			{
				if( msg->attempt < 2 )
				{
					DoMethod( obj, MM_Panelwin_Placement, MV_Panelsubwin_Bottom, msg->attempt + 1 );
				}
			} else {
				SetAttrs( obj, MUIA_Window_TopEdge , wintopedge - msg->height, TAG_DONE );
				SetAttrs( obj, MUIA_Window_LeftEdge, winleftedge + x, TAG_DONE );
			}
			break;
		case MV_Panelsubwin_Bottom:
			SetAttrs( obj, MUIA_Window_TopEdge , wintopedge  + h, TAG_DONE );
			SetAttrs( obj, MUIA_Window_LeftEdge, winleftedge + x, TAG_DONE );
			break;
	}
		if( open )
	{
		set( obj, MUIA_Window_Open, TRUE );
	}
	return( 0 );
}

/************************************************************************/

DEFTMETHOD(Panelwin_Close)
{
	PDB(("\n"));
	set( obj, MUIA_Window_Open, FALSE );

	return( 0 );
}

/************************************************************************/

DEFTMETHOD(Panelsubwin_Timer)
{
	GETDATA;
	if(!DoMethod(data->group,MM_Panelgroup_CheckZipLock))
		DoMethod(obj,MM_Panelsubwin_Open,FALSE);
	return( 0 );
}
/************************************************************************/

DEFMMETHOD(Window_Setup)
{
	GETDATA;
	/*
	 * Set proper window mode here.
	 */
	APTR rootwin;
	APTR parentgroup;
	ULONG depth;

	PDB(("1\n"))

	if( ( rootwin = (APTR) DoMethod( obj, MM_Panelwin_FindRoot ) ) )
	{
		depth = getv( rootwin, MA_Panelwin_Depth );
		if( ( parentgroup = _parent( data->parent ) ) )
		{
			ULONG horiz;
			GetAttr( MA_Panelgroup_Horiz, parentgroup, &horiz );
			set( data->group, MA_Panelgroup_Size, getv( parentgroup, MA_Panelgroup_Size ) );
			horiz = ~horiz & 0x00000001;
			set( data->group, MA_Panelgroup_Horiz, horiz );
		}
		switch (depth)
		{
			case MV_Panelgroup_Depth_Normal:
				SetAttrs( obj, MUIA_Window_Backdrop, FALSE, MUIA_Window_Frontdrop, FALSE, TAG_DONE );
				break;

			case MV_Panelgroup_Depth_Back:
				SetAttrs( obj, MUIA_Window_Backdrop, TRUE, MUIA_Window_Frontdrop, FALSE, TAG_DONE );
				break;

			case MV_Panelgroup_Depth_Front:
				SetAttrs( obj, MUIA_Window_Backdrop, FALSE, MUIA_Window_Frontdrop, EnableExtensions ? TRUE : FALSE, TAG_DONE );
				break;

			#ifdef DEBUG
			default:
				PDB(("wrong val\n"));
				break;
			#endif
		}
	}
	return( DOSUPER );
}

/************************************************************************/

DEFSMETHOD(Panelwin_BackPen)
{
	GETDATA;
	ULONG pen, pen2;
	ULONG RGB[4];
	ULONG alpha;

	PDB(("1\n"))

	pen = MUI_ObtainPen( muiRenderInfo( msg->poppen ), msg->penspec, 0 );
	pen2 = MUIPEN( pen );
	
	GetRGB32( muiRenderInfo( msg->poppen )->mri_Screen->ViewPort.ColorMap, pen2, 1, RGB );
	MUI_ReleasePen( muiRenderInfo( msg->poppen ), pen );
	get( data->group, MA_Panelgroup_BackColor, &alpha );
	
	alpha  = alpha >> 24;
	RGB[ 0 ] = RGB[ 0 ] >> 24;
	RGB[ 1 ] = RGB[ 1 ] >> 24;
	RGB[ 2 ] = RGB[ 2 ] >> 24;
	
	set( data->group, MA_Panelgroup_BackColor,( alpha << 24 ) + ( RGB[ 0 ] << 16 ) + ( RGB[ 1 ] << 8 ) + RGB[ 2 ] );

	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Panelwin_BackAlpha)
{
	GETDATA;
	ULONG color, alpha;

	get( data->group, MA_Panelgroup_BackColor, &color );
	alpha = msg->alpha << 24;
	color = ( color & 0x00FFFFFF ) + alpha;

	set( data->group, MA_Panelgroup_BackColor, color );
	return( 0 );
}

/************************************************************************/

DEFTMETHOD(Panel_Settings_Group)
{
    GETDATA;
	APTR group;
	APTR backmode;
	APTR backpen, alpha, alpha_label;
	APTR backasl, backasl_str;
	ULONG RGB[3],color;
	ULONG alpha_val;
	ULONG mode;
//	ULONG aslshow   = FALSE;
//	ULONG colorshow = FALSE;
//	ULONG spaceshow = FALSE;

	static STRPTR cyc_backmodes[ MSG_PANELSUBWIN_BACKMODE_CLONE - MSG_PANELSUBWIN_BACKMODE_COLOR + 2 ];

	get( data->group, MA_Panelgroup_BackColor, &color );
	get( data->group, MA_Panelgroup_BackMode , &mode );

//	if( mode == MV_Panelgroup_BackMode_Picture ) aslshow =TRUE;
//	else if( mode == MV_Panelgroup_BackMode_Color ) colorshow =TRUE;
//	else spaceshow = TRUE;

	RGB[0] = ( color >> 16 ) << 24;
	RGB[1] = ( color >> 8  ) << 24;
	RGB[2] = ( color << 24 );
	alpha_val =  color >> 24;
	
	if( ( group = ColGroup(2),
		Child, NSLabel1(MSG_PANELSUBWIN_BACKGROUND),
		Child, HGroup,
			Child, backmode = MUICreateCycle( MSG_PANELSUBWIN_BACKGROUND, cyc_backmodes, MSG_PANELSUBWIN_BACKMODE_COLOR, MSG_PANELSUBWIN_BACKMODE_CLONE, 0 ),
			Child, backpen = PoppenObject,End,
			Child, backasl = PopaslObject, MUIA_ShowMe,FALSE,
				MUIA_Popstring_String, backasl_str = KeyString( getv( data->group, MA_Panelgroup_Backdrop ), 60, "n" ),
				MUIA_Popstring_Button, PopButton( MUII_PopUp ),
			End,
		End,
			Child, alpha_label = NSLabel1(MSG_PANELWIN_ALPHA),
			Child, alpha = MUICreateSlider( MSG_PANELWIN_ALPHA, 0, 255, alpha_val, 0 ),
		End ) )
	{

		if( mode == MV_Panelgroup_BackMode_Picture) set( backasl, MUIA_ShowMe, TRUE );
		else
		if( mode == MV_Panelgroup_BackMode_Color  ) set( backpen, MUIA_ShowMe, TRUE );
		else
		if( mode == MV_Panelgroup_BackMode_Clone  ) set( alpha  , MUIA_ShowMe, TRUE );

		DoMethod( backpen    , MUIM_Pendisplay_SetRGB, RGB[ 0 ], RGB[ 1 ], RGB[ 2 ] );
		DoMethod( backmode   , MUIM_Notify           , MUIA_Cycle_Active   , 0             , backpen    , 3, MUIM_Set, MUIA_ShowMe  , TRUE  );
		DoMethod( backmode   , MUIM_Notify           , MUIA_Cycle_Active   , 1             , backpen    , 3, MUIM_Set, MUIA_ShowMe  , FALSE );
		DoMethod( backmode   , MUIM_Notify           , MUIA_Cycle_Active   , 1             , backasl    , 3, MUIM_Set, MUIA_ShowMe  , TRUE  );
		DoMethod( backmode   , MUIM_Notify           , MUIA_Cycle_Active   , 0             , backasl    , 3, MUIM_Set, MUIA_ShowMe  , FALSE );
		DoMethod( backmode   , MUIM_Notify           , MUIA_Cycle_Active   , 2             , backpen    , 3, MUIM_Set, MUIA_ShowMe  , FALSE );
		DoMethod( backmode   , MUIM_Notify           , MUIA_Cycle_Active   , 2             , backasl    , 3, MUIM_Set, MUIA_ShowMe  , FALSE );
		DoMethod( backmode   , MUIM_Notify           , MUIA_Cycle_Active   , 0             , alpha      , 3, MUIM_Set, MUIA_Disabled, FALSE );
		DoMethod( backmode   , MUIM_Notify           , MUIA_Cycle_Active   , 1             , alpha      , 3, MUIM_Set, MUIA_Disabled, FALSE );
		DoMethod( backmode   , MUIM_Notify           , MUIA_Cycle_Active   , 2             , alpha      , 3, MUIM_Set, MUIA_Disabled, TRUE  );
		DoMethod( backmode   , MUIM_Notify           , MUIA_Cycle_Active   , MUIV_EveryTime, data->group, 3, MUIM_Set, MA_Panelgroup_BackMode, MUIV_TriggerValue );
		DoMethod( backpen    , MUIM_Notify           , MUIA_Pendisplay_Spec, MUIV_EveryTime, obj        , 3, MM_Panelwin_BackPen, MUIV_TriggerValue, backpen );
		DoMethod( alpha      , MUIM_Notify           , MUIA_Slider_Level   , MUIV_EveryTime, obj        , 2, MM_Panelwin_BackAlpha, MUIV_TriggerValue );
		DoMethod( backasl_str, MUIM_Notify           , MUIA_String_Contents, MUIV_EveryTime, data->group, 3, MUIM_Set, MA_Panelgroup_Backdrop, MUIV_TriggerValue );

		set( backmode, MUIA_Cycle_Active, mode );
	}
	return( (ULONG) group );
}

/************************************************************************/

DEFTMETHOD(Panelsubwin_ToggleStayOpen)
{
	GETDATA;

	data->stayopen ^= 1;

	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Notify_Change)
{
	GETDATA;
	DoMethod( obj, MM_Panelsubwin_ParseDir, data->directory );
	return( 0 );
}

/************************************************************************/

BEGINMTABLE
DECNEW
DECGET
DECSET
DECTMETHOD(Panelwin_SaveConfig)
DECSMETHOD(Panelsubwin_Open)
DECTMETHOD(Panelwin_FindRoot)
DECTMETHOD(Panelsubwin_ParseDir)
DECSMETHOD(Panelwin_Placement)
DECTMETHOD(Panelwin_Close)
DECTMETHOD(Panelsubwin_Timer)
DECMMETHOD(Window_Setup)
DECSMETHOD(Panelwin_BackPen)
DECSMETHOD(Panelwin_BackAlpha)
DECTMETHOD(Panel_Settings_Group)
DECTMETHOD(Panelsubwin_ToggleStayOpen)
DECSMETHOD(Notify_Change)
ENDMTABLE


DECSUBCLASS_NC(MUIC_Window,  panelsubwinclass)
#endif
