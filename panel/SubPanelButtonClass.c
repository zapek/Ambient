
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

#include <proto/panel.h>

#include "debug.h"
#include "muifuncs.h"
#include "MUIClasses.h"
#include "paneltags.h"


#include "../prefs.h"



#include "ambient_cat.h"


#include <proto/ambient.h>
struct MUI_CustomClass *SubPanelButtonClass_Class(void);

#warning locale pfusch
/* catmaker */
#define CATCOMP_NUMBERS
extern const char * const __stringtable[];
#ifdef __SASC
#define GSI(x) __stringtable[x]
#else
#define GSI(x) ( STRPTR )__stringtable[x]
#endif

/************************************************************************/

static TEXT defaultImagepath[]  = "sys:tools.info";
extern APTR app;
struct Data
{
	ULONG ID;
	APTR subpanel;
};

/************************************************************************/
static void doset( APTR obj, struct Data *data, struct TagItem *tags,BOOL _init )
{
	struct TagItem *tstate = tags, *tag;
	while ((tag = (struct TagItem *) NextTagItem(&tstate)))
	{
		switch (tag->ti_Tag)
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
	}
}

static ULONG mNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
	if( ( obj = (APTR) DoSuperNew(cl, obj, TAG_MORE, msg->ops_AttrList ) ) )
	{
		struct Data *data = INST_DATA( cl, obj );

		if( ( data->subpanel = NewObject(GetClass("SubPanelWindow"), NULL,
							MA_Panelwin_Type        , MV_Panelwin_Type_SubPanel,
							MA_Panelwin_ParentObject, obj,
							MA_Panelgroup_Size      , 48,
						//	MA_Panelwin_Prefspool	, pctx,
						//	MA_Panelwin_PrefspoolStartIndex, start_index,
							TAG_DONE ) ) )
		{	
			data->ID = 0;
			doset( obj, data, msg->ops_AttrList ,TRUE);
			set( obj, MUIA_ShortHelp, " ");//GSI(MSG_PANELITEM_SUBPANEL_SHORTHELP) );
			DoMethod( app, OM_ADDMEMBER, data->subpanel );
		} else {
			MUI_DisposeObject( obj );
			obj = NULL;
		}
	}
	return( (ULONG) obj );
}

static ULONG mSet(struct IClass *cl,Object *obj,struct opSet *msg)
{
	struct TagItem *tstate, *tag;
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	tstate = msg->ops_AttrList;
	doset( obj, data,msg->ops_AttrList,FALSE); 
	return DoSuperMethodA(cl,obj,msg);
}

/************************************************************************/

static ULONG mGet(struct IClass *cl,Object *obj,struct opGet *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	ULONG result = TRUE;

	switch( msg->opg_AttrID )
	{
	/*	case MA_Panel_Imagepath:
			PDB(("%x\n",obj));
			*msg->opg_Storage = (ULONG) 0;
			break;*/
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
			*msg->opg_Storage = (ULONG) "SubPanel";//GSI(MSG_PANELITEM_SUBPANEL);
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
			*msg->opg_Storage = (ULONG) "SubPanel";//GSI(MSG_PANELITEM_SUBPANELDESC);
			break;
		case MA_PanelZipLock:
			{
				ULONG panelwinopen;
				get( data->subpanel,MUIA_Window_Open,&panelwinopen);
				*msg->opg_Storage = panelwinopen;
			} 
			break;
		default:
			result = DoSuperMethodA(cl,obj,msg);
			break;
	}
	return( result );
}


/************************************************************************/
static ULONG mSetup(struct IClass *cl,Object *obj,struct MUIP_Setup *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
/*
	APTR group;

	group = (APTR) getv( data->subpanel, MA_Panelwin_Group );

	DoMethod( _parent(obj), MUIM_Notify, MA_Panel_HasMoved , MUIV_EveryTime, data->subpanel, 2, MM_Panelsubwin_Open, FALSE );

	DoMethod( _parent(obj), MUIM_Notify, MA_Panelgroup_Size, MUIV_EveryTime, data->subpanel, 2, MM_Panelsubwin_Open, FALSE );

	DoMethod( _parent(obj), MUIM_Notify, MA_Panelgroup_Size, MUIV_EveryTime, group, 3, MUIM_Set, MA_Panelgroup_Size, MUIV_TriggerValue );
*/
	return DoSuperMethodA(cl,obj,msg);
}


static ULONG mLaunch(struct IClass *cl,Object *obj,struct MP_Panelbutton_Launch *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	if( data->subpanel )
	{
		if(msg->NumArgs)
		{
			struct WBArg *ap;
			int i;
			static char buf[256];
			char *b=buf;

			for (ap=msg->args,i=0;i< msg->NumArgs;i++,ap++)
			{
				NameFromLock(ap->wa_Lock,buf,sizeof(buf));
				AddPart(buf,ap->wa_Name,sizeof(buf));
			//	DoMethod(obj,MM_Panel_Drop,appmsg->am_MouseX,appmsg->am_MouseY,b);
			}
		}
		else
	 	{
			DoMethod( data->subpanel, MM_Panelsubwin_Open, TRUE, TAG_DONE );
    	}
	}
	return( 0 );
}


/************************************************************************/

static ULONG mClose(struct IClass *cl,Object *obj,Msg msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	set(data->subpanel,MA_Panelsubwin_StayOpen,FALSE);
	DoMethod( data->subpanel, MM_Panelsubwin_Open, FALSE, TAG_DONE );
	return( 0 );
}

/************************************************************************/



static ULONG mSaveConfig(struct IClass *cl,Object *obj,struct MP_Panel_SaveConfig *msg)
{
	APTR pl, pi; /* list, item */
	struct Data *data = (struct Data*)INST_DATA(cl,obj);

	APTR tbar;
	ULONG index = 1;
	ULONG type;
	ULONG backgroundmode;
	GetAttr(MA_Panelwin_Group,data->subpanel,(ULONG*)&tbar);
	get(tbar ,MA_Panelgroup_BackMode,&backgroundmode);
	
	if(!(pl = (APTR) GetPPoolItem(msg->panel_lock, NULL,  DSI_LISTPOOL_PANEL, NULL, NULL)))
	{
		pl = (APTR) AddPPoolItem(msg->panel_lock, NULL, DSI_LISTPOOL_PANEL, NULL, 0 );
	}
	if (pl)
	{
		if(!(pi = (APTR)GetPPoolItem(msg->panel_lock, pl, msg->index | DSF_LISTPOOL, NULL, NULL)))
		{
			pi = (APTR) AddPPoolItem(msg->panel_lock, pl, msg->index | DSF_LISTPOOL, NULL, (ULONG)NULL );
		}
		if( pi )
		{
			STRPTR imagepath;
			if(( get( obj, MA_Panel_Imagepath, &imagepath ) && (imagepath) && (strlen(imagepath))))
			{
				AddPPoolItem(msg->panel_lock, pi,DSI_LISTPOOL_PANEL_IMAGEPATH, imagepath , strlen(imagepath)+1);
			}
			else
			{
				RemovePPoolItem(msg->panel_lock,pi,DSI_LISTPOOL_PANEL_IMAGEPATH);
			}
			data->ID = msg->index+1;
			AddPPoolItem(msg->panel_lock,pi, DSI_LISTPOOL_PANEL_SUBPANEL_ROOT  	, (APTR) &data->ID, 4 );
		
#warning these should be saved in panelgroup anyways
				//	setprefslong_lp( pctx, pi, DSI_PANELGROUP_BACKMODE          , backgroundmode );
			//	setprefslong_lp( pctx, pi, DSI_PANELGROUP_BACKCOLOR         , getv( tbar, MA_Panelgroup_BackColor ) );
			/*	if( backgroundmode == MV_Panelgroup_BackMode_Picture )
				{
					setprefsstr_lp( pctx, pi, DSI_PANELGROUP_BACKDROP, (STRPTR) getv( tbar, MA_Panelgroup_Backdrop ) );
				}*/
			
				FORCHILD( tbar, MUIA_Group_ChildList )
				{
					if(!(pi = (APTR) GetPPoolItem(msg->panel_lock, pl, ( msg->index + index )  | DSF_LISTPOOL, NULL, NULL)))
					{
						pi = (APTR) AddPPoolItem(msg->panel_lock, pl, ( msg->index + index )  | DSF_LISTPOOL, NULL, (ULONG)NULL );
					}
					if(pi)
					{
						AddPPoolItem(msg->panel_lock,pi, DSI_LISTPOOL_PANEL_SUBPANEL,&data->ID, 4 );
						type = getv( child, MA_Panel_Type );
						if( ( type != MV_Panel_Type_External ) )
						{
							AddPPoolItem(msg->panel_lock,pi, DSI_LISTPOOL_PANEL_TYPE, &type, 4 );
							index += DoMethod( child, MM_Panel_SaveConfig, msg->panel_lock, msg->index + index );  /* subpanels will add items here*/
						} 
						#warning no save for external
						#if 0
						else /* o.k. must be an external class, needs better detecting */
						{
							APTR supobj;
							STRPTR classname;
							
							classname = (APTR) getv( child, MA_Panel_Extern_ClassName );

							setprefslong_lp( pctx, pi, DSI_LISTPOOL_PANEL_TYPE    , MV_Panel_Type_External );
							setprefsstr_lp ( pctx, pi, DSI_LISTPOOL_PANEL_EXT_NAME, classname ? classname : (STRPTR) "" );
							if( ( get( child, MA_Panelextern_SupportObject, (ULONG*) &supobj ) ) && ( supobj ) )
							{
								DoMethod( supobj, MM_Panelsupport_Saveconfig, pctx, pi );
							}
						}
						#endif
					}
					/* XXX */
					index++;
				}
				NEXTCHILD
			
		}
		/* XXX */
	}
	/* XXX */
	return( index - 1 );
}

DISPATCHER(SubPanelButtonClass)
{
	switch (msg->MethodID)
	{
		case OM_NEW             			: return(mNew				(cl,obj,(struct opSet*)msg));
		case OM_SET        	    			: return(mSet				(cl,obj,(struct opSet*)msg));
		case OM_GET			    			: return(mGet				(cl,obj,(struct opGet *)msg));
		case MUIM_Setup     				: return(mSetup    			(cl,obj,(struct MUIP_Setup *)msg));
		case MM_Panelbutton_Launch 			: return(mLaunch			(cl,obj,(struct MP_Panelbutton_Launch *)msg));
		case MM_Panelbutton_Close 			: return(mClose				(cl,obj,(Msg)msg));
		case MM_Panel_SaveConfig			: return(mSaveConfig		(cl,obj,(struct MP_Panel_SaveConfig *)msg));
	}
	return(DoSuperMethodA(cl,obj,msg));
}
DISPATCHER_END



struct MUI_CustomClass *SubPanelButtonClass_Class(void)
{
	return MUI_CreateCustomClass(NULL,NULL,GetMUIClass("BaseButton"),sizeof(struct Data),DISPATCHER_REF(SubPanelButtonClass));
}

