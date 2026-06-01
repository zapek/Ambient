#include <proto/gadtools.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <proto/graphics.h>
#include <proto/muimaster.h>
#include <clib/wb_protos.h>
/* public */

/* private */

#include "muifuncs.h"
#include "MUIClasses.h"
#include "debug.h"

#include "../mui_func.h"

#include "paneltags.h"

extern struct MsgPort *panel_port;
struct MUI_CustomClass *SubPanelWindowClass_Class(void);

/************************************************************************/

struct Data
{
	ULONG                       ID;
	ULONG                       type;
	APTR                        subpanel;
	STRPTR                      directory;
	ULONG                       stayopen;
	ULONG                       placement;
	APTR                        tbar;
	APTR                        parent;
	APTR                        nctx;
//	struct TimeVal              open_time;
	BOOL                        timeout;
	ULONG 						time_ct;
	struct MUI_InputHandlerNode ihnode;
	struct AppWindow *aw;
};




static void doset( APTR obj, struct Data *data, struct TagItem *tags, BOOL init)
{
	struct TagItem *tstate = tags, *tag;
	while ((tag = (struct TagItem *) NextTagItem(&tstate)))
	{
		switch (tag->ti_Tag)
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
#if 0
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
#endif
#if 1
			case MUIA_Window_MouseObject:
				if( tag->ti_Data )
				{
					data->time_ct = 0;
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
#endif
			case MUIA_Window_Open:
				if(tag->ti_Data == 0)
				{
					APTR window;
					GetAttr(MUIA_Window_Window,obj,(ULONG*)&window);
					if((window)&&(data->aw))
					{
						RemoveAppWindow(data->aw);
						data->aw = 0;
					}
				}
				break;
		}
	}
}

/************************************************************************/

static ULONG mNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
	APTR tbar;
	struct TagItem *id_ti;

//	else return 0;
	if( ( obj = DoSuperNew( cl, obj,
		//							MUIA_Window_Screen      , get_screen(),
		//							MUIA_Window_ScreenTitle , screentitle,
									MUIA_Window_CloseGadget , FALSE,
									MUIA_Window_Borderless  , TRUE,
									MUIA_Window_CloseGadget , FALSE,
									MUIA_Window_DepthGadget , FALSE,
									MUIA_Window_SizeGadget  , FALSE,
									MUIA_Window_DragBar     , FALSE,
									MUIA_Window_BackfillHook, LAYERS_NOBACKFILL,
									MUIA_Window_HasAlpha    ,TRUE,
									MUIA_Window_PanelWindow , TRUE,
									WindowContents,  tbar = (Object*)NewObject(GetClass("DefaultGroup"),NULL,//NewObject( getpanelgroupclass(), NULL,
														MUIA_Group_Horiz, TRUE,
	#warning ziptest
														MA_Panelgroup_Zipping,TRUE,
														MUIA_FillArea, FALSE,
														MA_Panelgroup_DragMode, MV_Panelgroup_DragMode_None,	
														TAG_MORE, msg->ops_AttrList,
														TAG_DONE ),
									TAG_DONE ) ) )
	{
		struct Data *data = (struct Data*) INST_DATA( cl, obj );
		data->type = MV_Panel_Type_SubPanel;
		data->ihnode.ihn_Object  = obj;
		data->ihnode.ihn_Flags   = MUIIHNF_TIMER;
		data->ihnode.ihn_Method  = MM_Panelsubwin_Timer;
		data->ihnode.ihn_Millis  = 1000;
		data->tbar = tbar;
		data->nctx  = 0;
		data->directory = 0;
		doset( obj, data, msg->ops_AttrList, TRUE );

	}
	return (ULONG) obj ;
}

/************************************************************************/



static ULONG mGet(struct IClass *cl,Object *obj,struct opGet *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	ULONG result = TRUE;
	switch( msg->opg_AttrID )
	{
		case MA_Panelwin_Group:
			*msg->opg_Storage = (ULONG) data->tbar;
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
			result = DoSuperMethodA(cl, obj, (Msg)msg);
			break;
	}
	return( result );
}

/************************************************************************/

static ULONG mSet(struct IClass *cl,Object *obj,struct opSet *msg)
{
	ULONG retval;
	struct TagItem *tstate, *tag;
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	tstate = msg->ops_AttrList;
	doset( obj, data,msg->ops_AttrList,FALSE); 
	retval = DoSuperMethodA(cl,obj,msg);
	if((tag = FindTagItem(MUIA_Window_Open,msg->ops_AttrList))&&(tag->ti_Data))
	{
		APTR window;
		GetAttr(MUIA_Window_Window,obj,(ULONG*)&window);
		if(window)
		{
			#warning appwin
				data->aw = AddAppWindow(120, data->tbar, window, panel_port, 0);
		//		struct AppWindowDropZone *awdz = AddAppWindowDropZone(aw, 100, 110, TAG_DONE )
		}
	}
	return retval;
}

/************************************************************************/


static ULONG mWindowSetup(struct IClass *cl,Object *obj,struct MUIP_Window_Setup *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	/*
	 * Set proper window mode here.
	 */
	APTR rootwin;
	APTR parentgroup;
	ULONG depth;
	
	if( ( rootwin = (APTR) DoMethod( obj, MM_Panelwin_FindRoot ) ) )
	{
		depth = getv( rootwin, MA_Panelwin_Depth );
		if( ( parentgroup = _parent( data->parent ) ) )
		{
			ULONG horiz;
			GetAttr( MA_Panelgroup_Horiz, parentgroup, &horiz );
			set( data->tbar, MA_Panelgroup_Size, getv( parentgroup, MA_Panelgroup_Size ) );
			horiz = ~horiz & 0x00000001;
			set( data->tbar, MA_Panelgroup_Horiz, horiz );
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
				SetAttrs( obj, MUIA_Window_Backdrop, FALSE, MUIA_Window_Frontdrop, TRUE , TAG_DONE );
				break;

			#ifdef DEBUG
			default:
			//	PDB(("wrong val\n"));
				break;
			#endif
		}
	}
	return DoSuperMethodA(cl,obj,msg);
}



static ULONG mOpen(struct IClass *cl,Object *obj,struct MP_Panelsubwin_Open *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
 //   struct TimeVal currenttime;
	BOOL winclose = FALSE;

	//set( obj, MUIA_Window_Open, msg->open);
#if 1
	
	if( msg->open )
	{
		if( getv( obj, MUIA_Window_Open ) )
		{
			winclose = TRUE;
			data->stayopen = FALSE;
		} else {
			set( obj, MUIA_Window_Open, TRUE );
			set( obj, MUIA_Window_Activate, TRUE );
		//	DoMethod( data->tbar, MM_Panelgroup_ToggleZip, MV_Paneldrag_Type_LeftUp );
			data->time_ct = 0;
			DoMethod( _app(obj), MUIM_Application_AddInputHandler, &data->ihnode );
			data->timeout = TRUE;
		}
	} else {
		
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
		FORCHILD( data->tbar, MUIA_Group_ChildList )
		{
			DoMethod(child,MM_Panelbutton_Close);
		}
		NEXTCHILD
	//	DoMethod( data->tbar, MM_Panelgroup_ToggleZip, MV_Paneldrag_Type_RightBottom );
		set( obj, MUIA_Window_Open, FALSE );
		set(data->parent,MA_PanelZipLock,FALSE);
	}
#endif
	return( 0 );
}

/************************************************************************/
static ULONG mFindRoot(struct IClass *cl,Object *obj,Msg msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	if( data->parent ) {
		return( DoMethod( _win( data->parent ), MM_Panelwin_FindRoot ) );
	}
	return( 0 );
}


/************************************************************************/

static ULONG mPlacement(struct IClass *cl,Object *obj,struct MP_Panelwin_Placement *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	ULONG winleftedge, wintopedge;
	ULONG x, y, w, h, horiz;
	ULONG open;

	get( obj, MUIA_Window_Window, &open );

	if( open ) /* this one is evil! It make no sense, but removing kills Ambient on click (geit, KRONOS_TAG) */
	{
		set( obj, MUIA_Window_Open, FALSE );
	}
	GetAttr( MA_Panelgroup_Horiz , data->tbar,&horiz );
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
#define TIME_OUT 3
static ULONG mTimer(struct IClass *cl,Object *obj,Msg msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);

	data->time_ct++;
	if(data->time_ct > TIME_OUT)
	{
		if(!DoMethod(data->tbar,MM_Panelgroup_CheckZipLock))
			set( obj, MUIA_Window_Open, FALSE );
	}
	return( 0 );
}
/*
static ULONG mSaveConfig(struct IClass *cl,Object *obj,struct MP_Panelwin_SaveConfig *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	if( data->parent )
	{
		Object *root_win;
		if((root_win = DoMethod( _win( data->parent ), MM_Panelwin_FindRoot ) ))
		{
			return DoMethodA(root_win,msg);
		}
	}
	return( 0 );
}*/

DISPATCHER(SubPanelWindowClass)
{
	switch (msg->MethodID)
	{
		case OM_NEW             			: return(mNew				(cl,obj,(struct opSet*)msg));
		case OM_SET        	    			: return(mSet				(cl,obj,(struct opSet*)msg));
		case OM_GET			    			: return(mGet				(cl,obj,(struct opGet *)msg));
		case MUIM_Window_Setup     			: return(mWindowSetup  		(cl,obj,(struct MUIP_Window_Setup *)msg));
		case MM_Panelsubwin_Open			: return(mOpen				(cl,obj,(struct MP_Panelsubwin_Open *)msg));
		case MM_Panelwin_FindRoot 			: return(mFindRoot			(cl,obj,(Msg)msg));	
		case MM_Panelwin_Placement			: return(mPlacement			(cl,obj,(struct MP_Panelwin_Placement *)msg));
		case MM_Panelsubwin_Timer 			: return(mTimer				(cl,obj,(Msg)msg));
//		case MM_Panelwin_SaveConfig 		: return(mSaveConfig		(cl,obj,(struct MP_Panelwin_SaveConfig *)msg));
	}
	return(DoSuperMethodA(cl,obj,msg));
}
DISPATCHER_END



struct MUI_CustomClass *SubPanelWindowClass_Class(void)
{
		return MUI_CreateCustomClass(NULL,MUIC_Window,NULL,sizeof(struct Data),DISPATCHER_REF(SubPanelWindowClass));
}
