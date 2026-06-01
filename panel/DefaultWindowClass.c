
#include <libraries/mui.h>
#include <clib/wb_protos.h>
#include <proto/panel.h>
#include "muifuncs.h"
#include "MUIClasses.h"
#include "debug.h"

#include "paneltags.h"



#include <proto/ambient.h>
#include <libraries/ambient.h>

#include <AppWindow.h>
#include "../prefs.h"




struct MUI_CustomClass *DefaultWindowClass_Class(void);
/************************************************************************/

#define PANELNAME_SIZEOF 300
extern struct MsgPort *panel_port;
struct Data {
	ULONG panelwin_delete;
	ULONG attaching;
	ULONG position;
	ULONG fixed,depth;
	APTR tbar;
    APTR stayopen_mi;
    BOOL stayopen;
	BOOL settings_active;
	STRPTR file_name;
	STRPTR panel_name;
	struct AppWindow *aw;
};


static void doset( APTR obj, struct Data *data, struct TagItem *tags,BOOL _init )
{
	struct TagItem *tstate = tags, *tag;
	while ((tag = (struct TagItem *) NextTagItem(&tstate)))
	{
		if (_init)
		{
			switch (tag->ti_Tag)
			{
			
			}
		}
		switch (tag->ti_Tag)
		{		
			case MA_Panelwin_Group:
				data->tbar = (APTR) tag->ti_Data;
				break;
			case MA_Panelwin_Depth:
				data->depth = tag->ti_Data;
				if( getv( obj, MUIA_Window_Open ) )
				{
					SetAttrs( obj, MUIA_NoNotify, TRUE, MUIA_Window_Open,  FALSE, TAG_DONE );
					SetAttrs( obj, MUIA_Window_Open, TRUE, TAG_DONE ); /* XXX: what happens if the window doesn't open again ? */
				}
					break;
			case MA_Panelwin_Position:
				data->position = tag->ti_Data;
				if( data->position == MV_Panelwin_Position_Attached )
				{
					DoMethod( obj, MM_Panelwin_AttachToBorder, FALSE );
				}
				break;
			case  MA_Panelwin_Name:
				if((tag->ti_Data)&&(strlen((STRPTR)tag->ti_Data)))
				{
					if(!data->file_name)
					{
						data->file_name =(STRPTR) AllocVecTaskPooled(strlen((STRPTR)tag->ti_Data)+1);
						strcpy(data->file_name, (STRPTR) tag->ti_Data );
					}
					if(data->panel_name)
					{
						FreeVecTaskPooled(data->panel_name);
					}
					data->panel_name =(STRPTR) AllocVecTaskPooled(strlen((STRPTR)tag->ti_Data)+1);
					strcpy(data->panel_name, (STRPTR) tag->ti_Data );
				//	PanelHasChanged(data->panel_name,PanelHasChanged_Rename,0,0);
				}
				break;
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


/*

static void SetPanelAttrStr_Default(APTR panel_lock, ULONG id, CONST_STRPTR data)
{
	if (!GetPanelAttr(panel_lock,0, id,NULL,NULL))
	{
		SetPanelAttr(panel_lock,NULL, id,(APTR*)data, strlen(data)+1);
	}
}


static void SetPanelAttrLong_Default(APTR panel_lock, ULONG id, ULONG v)
{
	if(!GetPanelAttr(panel_lock,0, id,NULL,NULL ))
	{
		SetPanelAttr(panel_lock,NULL, id,&v, sizeof(v));
	}
}
*/
static void SetPanelAttrStr_Default(APTR ppool, ULONG id, CONST_STRPTR data)
{
	if (!GetPPoolItem(ppool,0, id,NULL,NULL))
	{
		AddPPoolItem(ppool,NULL, id,(APTR*)data, strlen(data)+1);
	}
}


static void SetPanelAttrLong_Default(APTR ppool, ULONG id, ULONG v)
{
	if(!GetPPoolItem(ppool,0, id,NULL,NULL ))
	{
		AddPPoolItem(ppool,NULL, id,&v, sizeof(v));
	}
}


static ULONG mNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
	APTR tbar;
	APTR ppool;
//	APTR panel_lock = 0;
	struct TagItem *ti;
	BOOL newpanel = FALSE;
	ULONG *left,*top,*depth;
	ULONG *pos,*size,*zip,*horiz,*locked,*dockmode;
	ULONG *auto_zip,*drag,*backmode,*backcolor;
	ULONG *hidedrag;
	STRPTR backdrop = 0;
	STRPTR panel_name = 0;
	BOOL new_panel = FALSE;
//	char path[PATH_SIZE];

	if( ( ti = FindTagItem( MA_Panelwin_Name, INITTAGS ) ) )
	{
		panel_name = (STRPTR)ti->ti_Data;
	}
	else
	{
		//newpanel = TRUE;
		//panel_name = AddPanel(0);
	}
	if( ( ti = FindTagItem( MA_Panel_PrefsPool, INITTAGS ) ) )
	{
	//	ppool = (APTR)ti->ti_Data;
	}
	#warning what on new panel
	//if(!ppool) return 0;
	if(panel_name)
	{
		APTR xx;
		ppool = LockPPool(panel_name,PPOOL_TYPE_PANEL);
	//	PDB(("%x\n",ppool));
		if(!ppool)
		{
			if(AddPrefsPool(panel_name,PPOOL_TYPE_PANEL))
			{
				if((ppool = LockPPool(panel_name,PPOOL_TYPE_PANEL)))
				{
					AddPPoolItem(ppool, NULL, DSI_LISTPOOL_PANEL, NULL, (ULONG)NULL );
					new_panel = TRUE;
				}
			}
		//	PDB(("\n"));
		}
	//	panel_lock = LockPanel(panel_name);
	}
	if( !ppool )
	{
	//	PDB(("sigh, no prefspool\n"));
		return(0);
	}

#warning no defaults
#if 1
	SetPanelAttrLong_Default( ppool, DSI_PANELGROUP_SIZE                , MV_Panelgroup_Size_Large );
	SetPanelAttrLong_Default( ppool, DSI_PANELGROUP_DEPTH               , MV_Panelgroup_Depth_Normal );
	SetPanelAttrLong_Default( ppool, DSI_PANELGROUP_ISHORIZ             , TRUE );
	SetPanelAttrLong_Default( ppool, DSI_PANELGROUP_DRAGGADGET_PLACEMENT, MV_Panelgroup_DragMode_LeftUp );
	SetPanelAttrLong_Default( ppool, DSI_PANELGROUP_ZIPPING_ENABLED     , TRUE );

	SetPanelAttrLong_Default( ppool, DSI_PANELGROUP_POSMODE             , 0  );
	SetPanelAttrLong_Default( ppool, DSI_PANELGROUP_DOCKMODE             , 0  );
	SetPanelAttrLong_Default( ppool, DSI_PANELGROUP_X                   , 400 );
	SetPanelAttrLong_Default( ppool, DSI_PANELGROUP_Y                   , 300 );
	SetPanelAttrLong_Default( ppool, DSI_PANELGROUP_LOCKED              , FALSE );
	SetPanelAttrLong_Default( ppool, DSI_PANELGROUP_BACKMODE            , MV_Panelgroup_BackMode_Color );
	SetPanelAttrLong_Default( ppool, DSI_PANELGROUP_BACKCOLOR           , 0xa0a0a0a0 );
	SetPanelAttrLong_Default( ppool, DSI_PANELGROUP_HIDEDRAGBAR         , 0 );
#endif
#warning
	//	SetPanelAttrStr_Default( ppool, DSI_PANELGROUP_BACKDROP            , "XX" );

	//UnLockPanel(panel_lock);
	//	PDB(("%x %d\n",ppool,*left));
		#warning
	if((ppool))
	{
		APTR pll;
		GetPPoolItem(ppool,0, DSI_PANELGROUP_X,(APTR*)&left,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_Y,(APTR*)&top,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_DEPTH,(APTR*)&depth,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_POSMODE,(APTR*)&pos,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_DOCKMODE,(APTR*)&dockmode,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_LOCKED,(APTR*)&locked,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_SIZE,(APTR*)&size,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_ZIPPING_ENABLED,(APTR*)&zip,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_ISHORIZ,(APTR*)&horiz,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_AUTOZIP,(APTR*)&auto_zip,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_DRAGGADGET_PLACEMENT,(APTR*)&drag,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_BACKMODE,(APTR*)&backmode,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_BACKCOLOR,(APTR*)&backcolor,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_BACKDROP,(APTR*)&backdrop,NULL);
		GetPPoolItem(ppool,0, DSI_PANELGROUP_HIDEDRAGBAR,(APTR*)&hidedrag,NULL);
	}
	UnLockPPool(ppool);

	if( ( obj = DoSuperNew( cl, obj,
				MUIA_Window_Borderless  , TRUE,
				MUIA_Window_CloseGadget , FALSE,
				MUIA_Window_DepthGadget , FALSE,
				MUIA_Window_SizeGadget  , FALSE,
				MUIA_Window_DragBar     , FALSE,
				MUIA_Window_PanelWindow , TRUE,
				MUIA_Window_BackfillHook, LAYERS_NOBACKFILL,
				MUIA_Window_HasAlpha    , TRUE,
				MUIA_Window_LeftEdge    , *left,
				MUIA_Window_TopEdge     , *top,
	
				WindowContents, 
					tbar =  (Object*)NewObject(GetClass("DefaultGroup"),NULL, 
						MA_Panelgroup_DockMode	 , *dockmode,
						MA_Panelgroup_Locked     , *locked,
						MA_Panelgroup_Size       , *size,
						MA_Panelgroup_Zipping    , *zip,
						MA_Panelgroup_Horiz      , *horiz,
						MA_Panelgroup_AutoZip    , *auto_zip,
						MA_Panelgroup_DragMode   , *drag,
						MA_Panelgroup_BackMode   , *backmode,
						MA_Panelgroup_BackColor  , *backcolor,
						MA_Panelgroup_Backdrop   , backdrop,//getprefsstr_ctx ( pctx, DSI_PANELGROUP_BACKDROP ),
						MA_Panelgroup_HideDragBar, *hidedrag,
						MA_Panelgroup_New,TRUE	, new_panel,
				
						#warning Gridmode allways on
						MA_Panelgroup_GridMode   , 1,
				End,
	TAG_MORE			,	msg->ops_AttrList,
	TAG_DONE ) ) )
	{
		struct Data *data = (struct Data*) INST_DATA( cl, obj );
		data->tbar = tbar;
		data->depth    = *depth;
		data->position = *pos;
		data->file_name = 0;
		data->panel_name = 0;
		doset( obj, data, msg->ops_AttrList, TRUE );
		if(newpanel)
		{ 
			SetAttrs(obj,MA_Panelwin_Name,panel_name,TAG_DONE);
			SetAttrs(data->tbar,MA_Panelgroup_New,TRUE,TAG_DONE);
		}
	}
//		PDB(("\n\n"));
	return( (ULONG) obj );
}




static ULONG mSet(struct IClass *cl,Object *obj,struct opSet *msg)
{
	struct TagItem *tstate, *tag;
	ULONG retval;
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
			data->aw = AddAppWindow(11120, data->tbar, window, panel_port,WB_AW_MouseReport,AM_CLASS_MOUSEENTER |AM_CLASS_MOUSEEXIT |AM_CLASS_MOUSEMOVE ,TAG_DONE);
		}
	}
	return retval;
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
			*msg->opg_Storage = MV_Panelwin_Type_Root;
			break;
		case MA_Panelwin_Name:
			*msg->opg_Storage =	(ULONG) data->panel_name;
			break;
		case MA_Panelwin_FileName:
			*msg->opg_Storage =	(ULONG) data->file_name;
			break;
		case MA_Panelwin_Depth:
			*msg->opg_Storage = data->depth;
			break;
		case MA_Panelwin_Position:
			*msg->opg_Storage = data->position;
			break;
	/*	case MA_Window_Type:
			*msg->opg_Storage = MV_Window_Type_Panel;
			break;*/
		case MA_Panelwin_Closed:
			/* Always TRUE just to trigger the notify. */
			*msg->opg_Storage = TRUE;
			break;
		default:
			result = DoSuperMethodA(cl,obj,msg);
			break;
	}
	return( result );
}

/************************************************************************/




static ULONG mSetup(struct IClass *cl,Object *obj,struct MUIP_Window_Setup *msg)
{
	ULONG retval;
	ULONG dockmode;
	APTR window;
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	/*
	 * Set proper window mode here.
	 */
	switch( data->depth )
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

	/*
	 * If the user changed the screenmode this lets
	 * attached panels join their borders.
	 */
	GetAttr(MA_Panelgroup_DockMode,data->tbar,&dockmode);
	if( (data->position == MV_Panelwin_Position_Attached )||(dockmode) )
	{
		DoMethod( app, MUIM_Application_PushMethod, obj, 1, MM_Panelwin_AttachToBorder, FALSE );
	}

	retval = DoSuperMethodA(cl,obj,msg);
	//SetAttrs(data->tbar,MA_Panelgroup_DragMode,MV_Panelgroup_DragMode_None,TAG_DONE);
	//	GetAttr(MUIA_Window_Window,obj,(ULONG*)&window);	
	return retval;
}


static ULONG mSaveConfig(struct IClass *cl,Object *obj,struct MP_Panelwin_SaveConfig *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	
//	char path[PATH_SIZE];
	APTR ppool,main_ppool;
	
	APTR bctxt;
//	APTR panel_lock;
	
	
	ULONG size = getv( data->tbar, MA_Panelgroup_Size );
	ULONG zip = getv( data->tbar, MA_Panelgroup_Zipping );
 	ULONG autozip = getv( data->tbar, MA_Panelgroup_AutoZip  );
	ULONG locked = getv( data->tbar, MA_Panelgroup_Locked  );
	ULONG dragmode = getv( data->tbar, MA_Panelgroup_DragMode  );
 	ULONG hidedrag =getv( data->tbar, MA_Panelgroup_HideDragBar  );
	ULONG horiz =  getv( data->tbar, MA_Panelgroup_Horiz  );
	ULONG x = 0;
	ULONG y = 0;
	Object *root_win = (Object*)DoMethod( obj, MM_Panelwin_FindRoot);
	if(root_win != obj) return DoMethod(root_win,MM_Panelwin_SaveConfig,msg->flags);
	
	
	
	#warning use addpart
//	sprintf(path,"%s%s.prefs",PANEL_PATH,data->file_name);
//	PDB(("%s\n",path));	
	#warning
	if((ppool = LockPPool(data->panel_name,PPOOL_TYPE_PANEL)))
	{

		BOOL save_prefs = FALSE;
		AddPPoolItem(ppool,NULL, DSI_PANELGROUP_SIZE 					, (APTR) &size, 4);
		AddPPoolItem(ppool,NULL, DSI_PANELGROUP_ZIPPING_ENABLED    	, (APTR) &zip, 4 );
		AddPPoolItem(ppool,NULL, DSI_PANELGROUP_AUTOZIP            	, (APTR) &autozip, 4 );
		AddPPoolItem(ppool,NULL, DSI_PANELGROUP_DEPTH              	, &data->depth, 4);
		AddPPoolItem(ppool,NULL, DSI_PANELGROUP_LOCKED             	, (APTR) &locked, 4);
		AddPPoolItem(ppool,NULL, DSI_PANELGROUP_DRAGGADGET_PLACEMENT	, (APTR) &dragmode, 4);
		AddPPoolItem(ppool,NULL, DSI_PANELGROUP_HIDEDRAGBAR         	, (APTR) &hidedrag, 4);
		AddPPoolItem(ppool,NULL, DSI_PANELGROUP_ISHORIZ             	, (APTR) &horiz, 4);
		AddPPoolItem(ppool,NULL, DSI_PANELGROUP_POSMODE             	, (APTR) &data->position, 4);
	//	AddPPoolItem(ppool,NULL, DSI_PANELGROUP_DOCKMODE             	, (APTR) &data->dockmode, 4);
		if( getv( obj, MUIA_Window_Open ) )
		{
			x = getv( obj, MUIA_Window_LeftEdge);
			y = getv( obj, MUIA_Window_TopEdge);
		} 
		AddPPoolItem(ppool,NULL,  DSI_PANELGROUP_X, (APTR) &x, 4);
		AddPPoolItem(ppool,NULL,  DSI_PANELGROUP_Y, (APTR) &y, 4);
		DoMethod( data->tbar, MM_Panelgroup_SaveConfig, ppool, 0 );
		if((main_ppool = LockPPool("MAIN_PPOOL",PPOOL_TYPE_MAIN)))
		{
			
		//	PDB(("%x\n",main_ppool));
			if(msg->flags & MV_Panelwin_SaveConfig_Add)
			{
				ULONG *sc_add;
				if(GetPPoolItem(main_ppool,0, DSI_PANEL_AUTOSAVE_DROP,(APTR*)&sc_add,NULL )&&(*sc_add))save_prefs = TRUE;
			
			}
	/*		if(msg->flags & MV_Panelwin_SaveConfig_Delete)
			{
				if(GetGlobalPrefs( DSI_PANEL_AUTOSAVE_DELETE )) save_prefs = TRUE;
			
			}
			if(msg->flags & MV_Panelwin_SaveConfig_MoveObject)
			{
				if(GetGlobalPrefs( DSI_PANEL_AUTOSAVE_MOVE )) save_prefs = TRUE;
			
			}	
			if(msg->flags & MV_Panelwin_SaveConfig_WindowPos)
			{
				if(GetGlobalPrefs( DSI_PANEL_AUTOSAVE_WINDOWPOS)) save_prefs = TRUE;
			
			}
		*/
			if((msg->flags & MV_Panelwin_SaveConfig_Write)||(save_prefs))
			{
		//	PDB(("\n"));
	//		if(strcmp(data->panel_name,data->file_name))
	//		{}
			
#warning duplicate?
				SavePPool(ppool,0);//data->panel_name);
			}
				UnLockPPool(main_ppool);	
		}
		UnLockPPool(ppool);	
		PanelHasChanged(data->panel_name,0,0,0);
	}
#if 0	
	if(0)//(panel_lock = LockPanel(data->file_name)))//panel_name)))
	{
		BOOL save_prefs = FALSE;
		SetPanelAttr(panel_lock,NULL, DSI_PANELGROUP_SIZE 					, (APTR) &size, 4);
		SetPanelAttr(panel_lock,NULL, DSI_PANELGROUP_ZIPPING_ENABLED    	, (APTR) &zip, 4 );
		SetPanelAttr(panel_lock,NULL, DSI_PANELGROUP_AUTOZIP            	, (APTR) &autozip, 4 );
		SetPanelAttr(panel_lock,NULL, DSI_PANELGROUP_DEPTH              	, &data->depth, 4);
		SetPanelAttr(panel_lock,NULL, DSI_PANELGROUP_LOCKED             	, (APTR) &locked, 4);
		SetPanelAttr(panel_lock,NULL, DSI_PANELGROUP_DRAGGADGET_PLACEMENT	, (APTR) &dragmode, 4);
		SetPanelAttr(panel_lock,NULL, DSI_PANELGROUP_HIDEDRAGBAR         	, (APTR) &hidedrag, 4);
		SetPanelAttr(panel_lock,NULL, DSI_PANELGROUP_ISHORIZ             	, (APTR) &horiz, 4);
		SetPanelAttr(panel_lock,NULL, DSI_PANELGROUP_POSMODE             	, (APTR) &data->position, 4);
		if( getv( obj, MUIA_Window_Open ) )
		{
			x = getv( obj, MUIA_Window_LeftEdge);
			y = getv( obj, MUIA_Window_TopEdge);
		} 
		SetPanelAttr(panel_lock,NULL,  DSI_PANELGROUP_X, (APTR) &x, 4);
		SetPanelAttr(panel_lock,NULL,  DSI_PANELGROUP_Y, (APTR) &y, 4);
	//	DoMethod( data->tbar, MM_Panelgroup_SaveConfig, panel_lock, 0 );
		if(msg->flags & MV_Panelwin_SaveConfig_Add)
		{
			if(GetGlobalPrefs( DSI_PANEL_AUTOSAVE_DROP )) save_prefs = TRUE;
		}
		if(msg->flags & MV_Panelwin_SaveConfig_Delete)
		{
			if(GetGlobalPrefs( DSI_PANEL_AUTOSAVE_DELETE )) save_prefs = TRUE;
		}
		if(msg->flags & MV_Panelwin_SaveConfig_MoveObject)
		{
			if(GetGlobalPrefs( DSI_PANEL_AUTOSAVE_MOVE )) save_prefs = TRUE;
		}	
		if(msg->flags & MV_Panelwin_SaveConfig_WindowPos)
		{
			if(GetGlobalPrefs( DSI_PANEL_AUTOSAVE_WINDOWPOS)) save_prefs = TRUE;
		}
		
		if((msg->flags & MV_Panelwin_SaveConfig_Write)||(save_prefs))
		{
		//	PDB(("\n"));
	//		if(strcmp(data->panel_name,data->file_name))
	//		{}
#warning duplicate?
	//		SavePanel(panel_lock,data->panel_name);
		}
		UnLockPanel(panel_lock);
		PanelHasChanged(data->panel_name,0,0,0);
	}
#endif
	return( 0 );
}


static ULONG mFindRoot(struct IClass *cl,Object *obj,Msg msg)
{
	return( (ULONG) obj );
}

#define CORNER_ATTRACTION 6 /* percent of the screen size */

static ULONG mAttachToBorder(struct IClass *cl,Object *obj,struct MP_Panelwin_AttachToBorder *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	struct Window *win = (struct Window *) getv( obj, MUIA_Window_Window);
	ULONG dockmode = 0;
	LONG width,height;
	if(!win) return 0;
	width = _width(data->tbar);
	height = _height(data->tbar);
	GetAttr(MA_Panelgroup_DockMode,data->tbar,&dockmode);
	if(dockmode )
	{
			ULONG dir;
			LONG dist, distcomp;
			LONG tx;
			LONG ty;
			struct Screen *scr = win->WScreen;
			ULONG was_offscreen = FALSE;
			
			ty = scr->Height - win->Height;
			tx = (scr->Width - win->Width)/2;
			
			ChangeWindowBox(win, tx, ty, win->Width, win->Height);
	}
	else if(data->position == MV_Panelwin_Position_Attached )
	{
		if( !data->attaching  )
		{
			ULONG dir;
			LONG dist, distcomp;
			LONG tx = tx; /* shut up, gcc */
			LONG ty = ty; /* really */
			struct Screen *scr = win->WScreen;
			ULONG was_offscreen = FALSE;

			/*
			 * If we are in offscreen mode, we need to
			 * put back the panels inside and ignore the rest.
			 */
			ty = win->TopEdge;
			tx = win->LeftEdge;

			if( win->TopEdge < 0 )
			{
				ty = 0;
				was_offscreen = TRUE;
			}

			if( win->LeftEdge < 0 )
			{
				tx = 0;
				was_offscreen = TRUE;
			}

			if( win->TopEdge + height > scr->Height )
			{
				ty = scr->Height - win->Height;
				was_offscreen = TRUE;
			}

			if( win->LeftEdge + width > scr->Width )
			{
				tx = scr->Width - width;
				was_offscreen = TRUE;
			}

			/* XXX: we should check if a border is already set in the prefs! for example screenmode change.. the method might need a new parameter though */

			/*
			 * Find out to which border we
			 * have to attach.
			 */
	
			if( !was_offscreen )
			{
	            dir = BA_TOP;
	
				dist = win->TopEdge;
				distcomp = scr->Width - ( win->LeftEdge + win->Width );

				if( distcomp < dist )
				{
					dir = BA_RIGHT;
					dist = distcomp;
				}
				distcomp = scr->Height - ( win->TopEdge + win->Height );

				if( distcomp < dist )
				{
					dir = BA_BOTTOM;
					dist = distcomp;
				}
				distcomp = win->LeftEdge;

				if( distcomp < dist )
				{
					dir = BA_LEFT;
				}

				switch( dir )
				{
					case BA_LEFT:
						tx = 0;
						ty = win->TopEdge;
						break;

					case BA_RIGHT:
						tx = scr->Width - win->Width;
						ty = win->TopEdge;
						break;

					case BA_TOP:
						tx = win->LeftEdge;
						ty = 0;
						break;

					case BA_BOTTOM:
						tx = win->LeftEdge;
						ty = scr->Height - win->Height;
						break;
				}

				/*
				 * Find if we are near a corner, if so,
				 * attach to it.
				 */
				if( win->LeftEdge < scr->Width * CORNER_ATTRACTION / 100 )
				{
					tx = 0;
				}
				if( win->TopEdge < scr->Height * CORNER_ATTRACTION / 100 )
				{
					ty = 0;
				}
				if( scr->Width - ( win->LeftEdge + win->Width ) < scr->Width * CORNER_ATTRACTION / 100 )
				{
					tx = scr->Width - win->Width;
				}
				if( scr->Height - ( win->TopEdge + win->Height ) < scr->Height * CORNER_ATTRACTION / 100 )
				{
					ty = scr->Height - win->Height;
				}
#warning pctx
	//			setprefslong_ctx( data->pctx, DSI_PANELGROUP_BORDERATTACH, dir );
			}
			ChangeWindowBox(win, tx, ty, width, height);
		}
	}
#warning check autosave
#if 0
	if( !data->position || msg->immediate_save )
	{
		/* save the new positions immediately */
		if( getprefslong( DSI_PANEL_AUTOSAVE_WINDOWPOS ) )
		{
			DoMethod( obj, MM_Panelwin_SaveConfig );
		}
	}
#endif
	return( 0 );
}
APTR CreatePanelitem(APTR panel_lock,ULONG index,ULONG type);


#if 0
static ULONG m_LoadPanel(struct IClass *cl,Object *obj,Msg msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	ULONG node,i = 0;
	ULONG root_node;
	APTR panel_lock;

	if((panel_lock = LockPanel(data->panel_name)))
	{
		APTR pl, pi,issub;
		ULONG *type;
		ULONG sub_level = 0;
		ULONG *subpanelroot[10];
		if((pl = (APTR) GetPanelAttr(panel_lock,0,  DSI_LISTPOOL_PANEL ,NULL,NULL)))
		{
			while ((pi = (APTR) GetPanelAttr(panel_lock, pl, i | DSF_LISTPOOL, NULL, NULL)) && GetPanelAttr(panel_lock, pi, DSI_LISTPOOL_PANEL_TYPE, (APTR)&type, NULL))
			{
				STRPTR object_str = 0;	
				APTR o = 0;
		
				switch (*type)
				{
					
					//case MV_Panel_Type_ViewWatcher:
					//case MV_Panel_Type_Bookmarks:
					//case MV_Panel_Type_DirPanel:
					case MV_Panel_Type_SubPanel:
						o = CreatePanelitem(panel_lock,i,*type);
						//object_str = GSI(MSG_PANELITEM_SUBPANEL);
					//	GetPanelAttr(panel_lock, pi, DSI_LISTPOOL_PANEL_SUBPANEL_ROOT , (APTR) &sub_panel[sub_level].ID, NULL );
				//		sub_panel[sub_level].node = DoMethod(obj,MUIM_Listtree_Insert,GSI(MSG_PANELITEM_SUBPANEL),i,node,MUIV_Listtree_Insert_PrevNode_Tail,TNF_LIST);
					//	node = sub_panel[sub_level].node;
					
						
						break;
					case MV_Panel_Type_External:
					//	get( msg->User, MA_Panel_Extern_DisplayName, (ULONG*) &msg->Name );
					
						o = CreatePanelitem(panel_lock,i,*type);
						break;
					case MV_Panel_Type_Separator:
					case MV_Panel_Type_Button:
					case MV_Panel_Type_Spacer:
						//o = (APTR)DoMethod(_app(panelwin), MM_Application_CreatePanelitem, *type, panelwin, prefspool, i);
						o = CreatePanelitem(panel_lock,i,*type);
 						break;
				}
				if(o) 
				{
					ULONG *sub;
					APTR subtbar;
					/* old files don't have DSI_LISTPOOL_PANEL_SUBPANEL so it must be a root panel*/
					if((GetPanelAttr(panel_lock, pi, DSI_LISTPOOL_PANEL_SUBPANEL, (APTR)&sub, NULL ) ) && ( *sub != 0 ) )
					{
						if( ( subtbar = (APTR) DoMethod( data->tbar, MM_Panelgroup_FindSubPanel, *sub ) ) )
						{
							DoMethod( subtbar, MUIM_Group_InitChange );
							DoMethod( subtbar, OM_ADDMEMBER, o );
							DoMethod( subtbar, MUIM_Group_ExitChange );
						}
					}
					else
					{							
						DoMethod( data->tbar, MUIM_Group_InitChange ); /* needed because of our paneldrag repositioning */
						DoMethod( data->tbar, OM_ADDMEMBER, o );
						DoMethod( data->tbar, MUIM_Group_ExitChange );
					}
				}					
				i++;
			}
		}
		UnLockPanel(panel_lock);
	}		
	return 0;
}

#endif

static ULONG mLoadPanel(struct IClass *cl,Object *obj,Msg msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	ULONG node,i = 0;
	ULONG root_node;
	APTR ppool,pll;
	
	if((ppool = LockPPool(data->panel_name,PPOOL_TYPE_PANEL)))
	{
		APTR pl, pi,issub;
		ULONG *type;
		ULONG sub_level = 0;
		ULONG *subpanelroot[10];
		
	
	//	pll =  GetPPoolItem(ppool, 0,  DSI_LISTPOOL_PANEL ,NULL,NULL);
	
		if((pl = (APTR) GetPPoolItem(ppool,0,  DSI_LISTPOOL_PANEL ,NULL,NULL)))
		{
			while ((pi = (APTR) GetPPoolItem(ppool, pl, i | DSF_LISTPOOL, NULL, NULL)) && GetPPoolItem(ppool, pi, DSI_LISTPOOL_PANEL_TYPE, (APTR)&type, NULL))
			{
				//STRPTR object_str = 0;
				APTR pii;			
				APTR o = 0;
			
				
				
			//	pii = GetPPoolItem(ppool, pll, i | DSF_LISTPOOL,NULL,NULL );
			//	GetPPoolItem(ppool,pii, DSI_LISTPOOL_PANEL_TYPE, (APTR)&typee,NULL );
			//	PDB(("%d %d\n",i,*type));
			//	if(*type == MV_Panel_Type_Drag) return 0;
			/*	switch (*type)
				{
					case MV_Panel_Type_SubPanel:*/
						o = CreatePanelitem(ppool,i,*type);
				
					//	o = CreatePanelitem(panel_lock,i,*type);
 			
				if(o) 
				{
					ULONG *sub;
					APTR subtbar;
					/* old files don't have DSI_LISTPOOL_PANEL_SUBPANEL so it must be a root panel*/
					if((GetPPoolItem(ppool, pi, DSI_LISTPOOL_PANEL_SUBPANEL, (APTR)&sub, NULL ) ) && ( *sub != 0 ) )
					{
						if( ( subtbar = (APTR) DoMethod( data->tbar, MM_Panelgroup_FindSubPanel, *sub ) ) )
						{
							DoMethod( subtbar, MUIM_Group_InitChange );
							DoMethod( subtbar, OM_ADDMEMBER, o );
							DoMethod( subtbar, MUIM_Group_ExitChange );
						}
					}
					else
					{							
						DoMethod( data->tbar, MUIM_Group_InitChange ); /* needed because of our paneldrag repositioning */
						DoMethod( data->tbar, OM_ADDMEMBER, o );
						DoMethod( data->tbar, MUIM_Group_ExitChange );
					}
				}					
				i++;
			}
		}
		UnLockPPool(ppool);	
	}
	//PDB(("b\n"));
	return 0;
}

static ULONG mSetupPanel(struct IClass *cl,Object *obj,Msg msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	ULONG node,i = 0;
	ULONG root_node;
	APTR ppool,pll;
	if((ppool = LockPPool(data->panel_name,PPOOL_TYPE_PANEL)))
	{	
		ULONG *left,*top,*depth;
		ULONG *pos,*size,*zip,*horiz,*locked,*dockmode = 0;
		ULONG *auto_zip,*drag,*backmode,*backcolor;
		ULONG *hidedrag;
		STRPTR backdrop = 0;
		GetPPoolItem(ppool,0, DSI_PANELGROUP_X,(APTR*)&left,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_Y,(APTR*)&top,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_DEPTH,(APTR*)&depth,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_POSMODE,(APTR*)&pos,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_DOCKMODE,(APTR*)&dockmode,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_LOCKED,(APTR*)&locked,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_SIZE,(APTR*)&size,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_ZIPPING_ENABLED,(APTR*)&zip,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_ISHORIZ,(APTR*)&horiz,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_AUTOZIP,(APTR*)&auto_zip,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_DRAGGADGET_PLACEMENT,(APTR*)&drag,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_BACKMODE,(APTR*)&backmode,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_BACKCOLOR,(APTR*)&backcolor,NULL );
		GetPPoolItem(ppool,0, DSI_PANELGROUP_BACKDROP,(APTR*)&backdrop,NULL);
		GetPPoolItem(ppool,0, DSI_PANELGROUP_HIDEDRAGBAR,(APTR*)&hidedrag,NULL);
		data->depth    = *depth;
		data->position = *pos;
		UnLockPPool(ppool);
		SetAttrs(data->tbar,MA_Panelgroup_Locked     , *locked,
						MA_Panelgroup_Size       , *size,
						MA_Panelgroup_Zipping    , *zip,
						MA_Panelgroup_Horiz      , *horiz,
						MA_Panelgroup_AutoZip    , *auto_zip,
						MA_Panelgroup_DragMode   , *drag,
						MA_Panelgroup_BackMode   , *backmode,
						MA_Panelgroup_BackColor  , *backcolor,
						MA_Panelgroup_Backdrop   , backdrop,//getprefsstr_ctx ( pctx, DSI_PANELGROUP_BACKDROP ),
						MA_Panelgroup_HideDragBar, *hidedrag,
						MA_Panelgroup_DockMode	 , *dockmode,
						#warning GridMode allways on
						MA_Panelgroup_GridMode   , 1,TAG_DONE);
	}
	return 0;
}

static ULONG mFindObject(struct IClass *cl,Object *obj,struct MP_Panelwin_FindObject *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	BPTR searchlock,objlock;
	APTR child;
	APTR child_state;
	struct List *child_list;
	BOOL found = FALSE;
	if((searchlock = Lock(msg->pattern,SHARED_LOCK)))
	{
		if( ( GetAttr(MUIA_Group_ChildList,data->tbar,(ULONG*)&child_list ) ) && ( child_list ) )
		{					
			child_state = child_list->lh_Head;
			while( ( child = NextObject( &child_state ) ) && (found == FALSE))
			{
				STRPTR uri;
				if(( GetAttr(MA_Panel_URI,child,(ULONG*)&uri ) ) && ( uri ) )
				{
					if( ( objlock = Lock(uri,SHARED_LOCK) ) )
					{
						if( SameLock( searchlock,objlock ) == 	LOCK_SAME ) 
						{
							*msg->object = child;
							found = TRUE;
						}
						UnLock(objlock);
					}
				}
			}
			
		}
		UnLock(searchlock);
	}
	if (msg->object) return(TRUE);
	return ( FALSE );
}

static ULONG mPrefsUpdate(struct IClass *cl, Object *obj, struct MP_Panel_PrefsUpdate *msg )
{
	struct Data *data = INST_DATA( cl, obj );
	APTR target_obj = 0;
	ULONG attr,*value;
	if(msg->ppool)
	{
		switch(msg->prefs_ID)
		{
			case DSI_PANELGROUP_SIZE:
				attr = MA_Panelgroup_Size;
				target_obj = data->tbar;
				break;
			case DSI_PANELGROUP_ISHORIZ:
				attr = MA_Panelgroup_Horiz;
				target_obj = data->tbar;
				break;
			case DSI_PANELGROUP_ZIPPING_ENABLED:
				attr = 	MA_Panelgroup_Zipping;
				target_obj = data->tbar;
				break;
			case DSI_PANELGROUP_DOCKMODE:
				attr = 	MA_Panelgroup_DockMode;
				target_obj = data->tbar;
				break;
		 	case DSI_PANELGROUP_LOCKED:
				attr = 	MA_Panelgroup_Locked;
				target_obj = data->tbar;
				break;
			case DSI_PANELGROUP_AUTOZIP:
				attr = 	MA_Panelgroup_AutoZip;
				target_obj = data->tbar;
				break;
		 	case DSI_PANELGROUP_DRAGGADGET_PLACEMENT:
				attr = 	MA_Panelgroup_DragMode;
				target_obj = data->tbar;
				break;
			case DSI_PANELGROUP_BACKMODE:
				attr = 	MA_Panelgroup_BackMode;
				target_obj = data->tbar;
				break;
		 	case DSI_PANELGROUP_BACKCOLOR:
				attr = 	MA_Panelgroup_BackColor;
				target_obj = data->tbar;
				break;
		 	case DSI_PANELGROUP_BACKDROP:
				attr = 	MA_Panelgroup_Backdrop;
				target_obj = data->tbar;
				break;
		 	case DSI_PANELGROUP_HIDEDRAGBAR:
				attr = 	MA_Panelgroup_HideDragBar;
				target_obj = data->tbar;
				break;
			case DSI_PANELGROUP_POSMODE:
				attr = MA_Panelwin_Position;
				target_obj = obj;
				break;
			case DSI_PANELGROUP_DEPTH:
				attr = MA_Panelwin_Depth;
				target_obj = obj;
				break;
			
		

			
			
		}
		if((target_obj)&&(GetPPoolItem(msg->ppool, 0, msg->prefs_ID , (APTR) &value, NULL )))
		{
		
			SetAttrs(target_obj,attr,*value,TAG_DONE);
		}
	/*	kprintf("digiclock new color b\n");
		if(GetPPoolItem(msg->ppool, msg->object_ID, DSI_Clock_Color , (APTR) &color, NULL ))
		{	
			data->mcc_Color = *color;
			kprintf("digiclock new color %x\n",*color);
			MUI_Redraw(obj,MADF_DRAWUPDATE);
		}*/
	}
	return 0;
}

DISPATCHER(DefaultWindowClass)
{
	switch (msg->MethodID)
	{
		case OM_NEW             			: return(mNew			(cl,obj,(struct opSet*)msg));
		case OM_SET        	    			: return(mSet			(cl,obj,(struct opSet*)msg));
		case OM_GET			    			: return(mGet			(cl,obj,(struct opGet *)msg));
		case MUIM_Window_Setup				: return(mSetup			(cl,obj,(struct MUIP_Window_Setup *)msg));   
		case MM_Panelwin_FindRoot 			: return(mFindRoot		(cl,obj,(Msg)msg));
		case MM_Panelwin_SaveConfig 		: return(mSaveConfig	(cl,obj,(struct MP_Panelwin_SaveConfig *)msg));
		case MM_Panelwin_AttachToBorder		: return(mAttachToBorder(cl,obj,(struct MP_Panelwin_AttachToBorder *)msg));
		case MM_PanelWindow_LoadPanel		: return(mLoadPanel		(cl,obj,(Msg)msg));
		case MM_PanelWindow_SetupPanel		: return(mSetupPanel	(cl,obj,(Msg)msg));
		case MM_Panelwin_FindObject			: return(mFindObject	(cl,obj,(struct MP_Panelwin_FindObject *)msg));
		case MM_Panel_PrefsUpdate			: return(mPrefsUpdate	(cl,obj,(struct MP_Panel_PrefsUpdate*)msg));
	}
	return(DoSuperMethodA(cl,obj,msg));
}
DISPATCHER_END



struct MUI_CustomClass *DefaultWindowClass_Class(void)
{
	return MUI_CreateCustomClass(NULL,MUIC_Window,NULL,sizeof(struct Data),DISPATCHER_REF(DefaultWindowClass));
}