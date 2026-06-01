
#include <proto/intuition.h>
#include <proto/wb.h>

#include <math.h>


#include "muifuncs.h"


#include <proto/panel.h>
#include <libraries/panel.h>


#include "debug.h" 
#include "paneltags.h"

#warning merge with proper header
#include "panellib.h"


#include "../prefs.h"
#include "../methodstack.h"
#include "../name.h"


#include <proto/ambient.h>
#include <libraries/ambient.h>

#warning merge with proper header
#include "ambient_lib.h"

struct MUI_CustomClass *Application_Class(void);

#warning needs to be public?
APTR CreatePanelitem(APTR panel_lock,ULONG index,ULONG type);


#define	VERSION	    0
#define	REVISION    10

#define APPLICATIONNAME "PanelApp"
#define AUTHORNAME "Stefan Kleinheinrich"

#define I2S(x) I2S2(x)
#define I2S2(x) #x

BYTE *version = (BYTE *) "$VER: 0.0"; 
//APPLICATIONNAME " " I2S(VERSION) "." I2S(REVISION) " (" __AMIGADATE__ ") © " __COPYRIGHTYEAR__ " " AUTHORNAME;


APTR app;
ULONG panel_modus = 1;
#warning
struct Screen *screen = 0;
struct MsgPort *panel_port;

//#define PANEL_PREFS_PATH	"PROGDIR:" 
#define PANEL_FILE       	"Panel.prefs"
#define GLOBALPANELPREFSID   MAKE_ID('G','P','A',1) /* Global Panel prefs (Panel.prefs) */

 
struct Library *VGraphicsBase;
struct Library   *CGXDitherBase;
struct Library *PanelBase;
struct Library *WBStartBase;
#define MAX_ZIP_CT 10


//#define ZIP_STEP 8
//#define ZIP_STEPS 20
ULONG zip_speeds[] = {60,40,16,1};

struct ZipData
{
	Object *group_obj;
	ULONG direction,type,horiz;
	LONG start_width,start_height;
	LONG target_width,target_height;
	LONG start_left,start_top;
	LONG drag;
	ULONG start_tick;
};

struct Data
{
	struct MUI_InputHandlerNode ihnode;
	ULONG zip_ct;
//	struct List classlist;
	struct ZipData zip_d[MAX_ZIP_CT];
};



static BOOL CheckPrefsFile(STRPTR fname)
{
	ULONG l;
	if((!fname)||(strlen(fname) < 6)) return FALSE;
	if(!strcmp(fname,PANEL_FILE)) return FALSE;
	l = strlen(fname);
	if (!strcmp(&fname[l-6],".prefs")) return TRUE;
	return FALSE;	
}
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
 		
		}
	}
}

static ULONG mNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
	struct Data *data;
	ULONG i;
	DOUBLE zs;
	APTR main_ppool;
	
	if (!(obj = (Object *)DoSuperMethodA(cl,obj,msg)))
		return(0);
	data = (struct Data*)INST_DATA(cl,obj);
	data->zip_ct = 0;
	doset( obj, data, msg->ops_AttrList, TRUE );
	memset( &data->ihnode, 0, sizeof( struct MUI_InputHandlerNode ) );
	for(i = 0; i++; i < MAX_ZIP_CT)
	{	
		memset( &data-> zip_d[i], 0, sizeof( struct ZipData ) );
	}

	if((main_ppool = LockPPool("MAIN_PPOOL",PPOOL_TYPE_MAIN)))
	{
		ULONG *brighten = NULL;
		GetPPoolItem(main_ppool,0, DSI_PANEL_DRAGDROP_BRIGHTEN,(APTR*)&brighten,NULL );
		UnLockPPool(main_ppool);
	}
	return(ULONG)obj;
}

static ULONG mDispose(struct IClass *cl,Object *obj,Msg msg)
{
	struct Data *data;
	data = (struct Data*)INST_DATA(cl,obj);

	if( data->ihnode.ihn_Object != NULL )
	{
		DoMethod( obj, MUIM_Application_RemInputHandler, &data->ihnode );
	}
	return DoSuperMethodA(cl,obj,msg);
}



static ULONG mSet(struct IClass *cl,Object *obj,struct opSet *msg)
{
	struct TagItem *tstate, *tag;
    struct Data *data;

	data = (struct Data*)INST_DATA(cl,obj);
	doset( obj, data,msg->ops_AttrList,FALSE); 
	return DoSuperMethodA(cl,obj,msg);
}

static ULONG mGet(struct IClass *cl,Object *obj,struct opGet *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	
	/*switch (msg->opg_AttrID)
	{
		case MA_Application_ActiveDisplay:
			*msg->opg_Storage  = (ULONG)data->activedisplay;
			return TRUE;		
	}*/
	return DoSuperMethodA(cl, obj, (Msg)msg);
}


static ULONG mStartZipping(struct IClass *cl,Object *obj,struct MP_Application_StartZipping *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	ULONG zip_nr;


	if(data->ihnode.ihn_Object == 0)
	{
		data->ihnode.ihn_Millis = 10;
		data->ihnode.ihn_Object = obj;
		data->ihnode.ihn_Flags  = MUIIHNF_TIMER;
		data->ihnode.ihn_Method = MM_Application_ZipTimer;
		DoMethod( obj, MUIM_Application_AddInputHandler, &data->ihnode );
	}
	for(zip_nr = 0; data->zip_d[zip_nr].group_obj;zip_nr++)
	{
		if(zip_nr == MAX_ZIP_CT) 
		{
			return FALSE;
		}
		if(data->zip_d[zip_nr].group_obj == msg->group_obj)
		{
			return FALSE;
		}
	}
	data->zip_d[zip_nr].group_obj = msg->group_obj;
	data->zip_d[zip_nr].start_width = _width( msg->group_obj);//msg->width;
	data->zip_d[zip_nr].start_height = _height( msg->group_obj);//msg->height;
	data->zip_d[zip_nr].target_width = msg->width;
	data->zip_d[zip_nr].target_height = msg->height;
	data->zip_d[zip_nr].direction = msg->direction;
	data->zip_d[zip_nr].type = msg->type;
	data->zip_d[zip_nr].drag = msg->drag;
	GetAttr(MA_Panelgroup_Horiz,msg->group_obj,&data->zip_d[zip_nr].horiz);
	GetAttr(MUIA_Window_LeftEdge,_win(msg->group_obj),&data->zip_d[zip_nr].start_left);
	GetAttr(MUIA_Window_TopEdge,_win(msg->group_obj),&data->zip_d[zip_nr].start_top);
	data->zip_d[zip_nr].start_tick = data->zip_ct;
	return TRUE;
}




static ULONG mDZipTimer(struct IClass *cl,Object *obj,Msg msg)
{
	struct Data *data;
	LONG new_width,new_height;
	LONG new_left,new_top;
	ULONG zip_nr;
	data = (struct Data*)INST_DATA(cl,obj);
	data->zip_ct++;

	for(zip_nr = 0;zip_nr < MAX_ZIP_CT;zip_nr++)
	{
		if(data->zip_d[zip_nr].group_obj)
		{
			BOOL finished = FALSE;
			LONG step = (data->zip_d[zip_nr].start_height - data->zip_d[zip_nr].target_height)/6;
			LONG step_ct = data->zip_ct - data->zip_d[zip_nr].start_tick;
			new_top = data->zip_d[zip_nr].start_top + step_ct *step;
			new_height = data->zip_d[zip_nr].start_height - step_ct*step;
			new_left = data->zip_d[zip_nr].start_left;
			new_width = data->zip_d[zip_nr].start_width;
			if(data->zip_d[zip_nr].direction)
			{
				if ((new_height <= data->zip_d[zip_nr].target_height)||(new_height < 1))
				{
					new_height = data->zip_d[zip_nr].target_height;
					finished = TRUE;
				}				
			}
			else
			{
				if (new_height >= data->zip_d[zip_nr].target_height)
				{
					new_height = data->zip_d[zip_nr].target_height;
					finished = TRUE;
				}				
			}
			ChangeWindowBox(_window(data->zip_d[zip_nr].group_obj),new_left,new_top,new_width,new_height);	
			if(finished)
			{
				DoMethod(data->zip_d[zip_nr].group_obj,MM_Panelgroup_ZippingFinished,data->zip_d[zip_nr].direction);
				memset( &data->zip_d[zip_nr], 0, sizeof( struct ZipData ) );
			}
		}
	}
}


#warning zip_speed
static ULONG mZipTimer(struct IClass *cl,Object *obj,Msg msg)
{
	struct Data *data;
	LONG new_width,new_height;
	LONG new_left,new_top;
	ULONG zip_nr;
	ULONG zip_steps,zip_speed = 2;
	BOOL active = FALSE;
	data = (struct Data*)INST_DATA(cl,obj);
	data->zip_ct++;
//	zip_speed = getprefslong_ctx(mainprefspool,DSI_PANEL_ZIPSPEED);

	if(zip_speed > 3) zip_speed = 2;
	zip_steps = zip_speeds[zip_speed]; 
	
	for(zip_nr = 0;zip_nr < MAX_ZIP_CT;zip_nr++)
	{
		if(data->zip_d[zip_nr].group_obj)
		{
			active = TRUE;
			if(data->zip_d[zip_nr].type == DIR_BOTTOM_DOCK)
			{
				BOOL finished = FALSE;
				LONG step = (data->zip_d[zip_nr].start_height - data->zip_d[zip_nr].target_height)/6;
				LONG step_ct = data->zip_ct - data->zip_d[zip_nr].start_tick;
				new_top = data->zip_d[zip_nr].start_top + step_ct *step;
				new_height = data->zip_d[zip_nr].start_height - step_ct*step;
				new_left = data->zip_d[zip_nr].start_left;
				new_width = data->zip_d[zip_nr].start_width;
				if(data->zip_d[zip_nr].direction)
				{
					if ((new_height <= data->zip_d[zip_nr].target_height)||(new_height < 1))
					{
						new_height = data->zip_d[zip_nr].target_height;
						finished = TRUE;
					}				
				}
				else
				{
					if (new_height >= data->zip_d[zip_nr].target_height)
					{
						new_height = data->zip_d[zip_nr].target_height;
						finished = TRUE;
					}				
				}
				ChangeWindowBox(_window(data->zip_d[zip_nr].group_obj),new_left,new_top,new_width,new_height);	
				if(finished)
				{
					DoMethod(data->zip_d[zip_nr].group_obj,MM_Panelgroup_ZippingFinished,data->zip_d[zip_nr].direction);
					memset( &data->zip_d[zip_nr], 0, sizeof( struct ZipData ) );
				}
			}
			else
			{
				BOOL finished = FALSE;
				LONG step_ct = data->zip_ct - data->zip_d[zip_nr].start_tick;
				double zs = (M_PI/zip_steps)*((double)step_ct - zip_steps +2);
				double r = (cos(zs) + 1.0) / 2;
				if(data->zip_d[zip_nr].horiz)
				{
					LONG step2 = (data->zip_d[zip_nr].start_width - data->zip_d[zip_nr].target_width)*r;
					new_width = data->zip_d[zip_nr].start_width - step2;
					new_height = data->zip_d[zip_nr].start_height;
					if(r > 0.99)
					{
						new_width = data->zip_d[zip_nr].target_width;
						finished = TRUE;
					}
				}
				else
				{
					LONG step2 = (data->zip_d[zip_nr].start_height - data->zip_d[zip_nr].target_height)*r;
					new_width = data->zip_d[zip_nr].start_width;
					new_height = data->zip_d[zip_nr].start_height - step2;
					if(r > 0.99)
					{
						new_height = data->zip_d[zip_nr].target_height;
						finished = TRUE;
					}
				}	
				new_left = data->zip_d[zip_nr].start_left;
				new_top = data->zip_d[zip_nr].start_top;
				if(data->zip_d[zip_nr].type == DIR_RIGHTBOTTOM)
				{
					if(data->zip_d[zip_nr].direction)
					{
						new_left = data->zip_d[zip_nr].start_left + data->zip_d[zip_nr].start_width - new_width;
						new_top = data->zip_d[zip_nr].start_top  + data->zip_d[zip_nr].start_height - new_height;
					}
					else 
					{
						if(data->zip_d[zip_nr].horiz)
						{
							new_left = data->zip_d[zip_nr].start_left - new_width + data->zip_d[zip_nr].drag;
						}
						else
						{
							new_top = data->zip_d[zip_nr].start_top - new_height + data->zip_d[zip_nr].drag;
						}
					}
				}
				ChangeWindowBox(_window(data->zip_d[zip_nr].group_obj),new_left,new_top,new_width,new_height);	
				if(finished)
				{
					DoMethod(data->zip_d[zip_nr].group_obj,MM_Panelgroup_ZippingFinished,data->zip_d[zip_nr].direction);
					memset( &data->zip_d[zip_nr], 0, sizeof( struct ZipData ) );
				}
			}
		}
	}	
	if((!active))//||(data->zip_ct == 180))
	{
		if( data->ihnode.ihn_Object != NULL )
		{
			DoMethod( obj, MUIM_Application_RemInputHandler, &data->ihnode );
		}
		data->zip_ct = 0;
		memset( &data->ihnode, 0, sizeof( struct MUI_InputHandlerNode ) );
	}
	return 0;
}


APTR CreatePanelitem(APTR ppool,ULONG index,ULONG type)
{
	APTR o = NULL;
	APTR pl, pi = NULL; /* list, item */

	if((pl = (APTR)GetPPoolItem(ppool,0,  DSI_LISTPOOL_PANEL ,NULL,NULL)))
	{
		pi = (APTR)GetPPoolItem(ppool, pl, index | DSF_LISTPOOL, NULL, NULL);
	}

#if 1
	switch( type )
	{
		#ifdef DEBUG
		case MV_Panel_Type_Drag:
		//	PDB(("this is not supposed to happen\n"));
			break;
		#endif

		case MV_Panel_Type_Button:
			{
				STRPTR imagepath, paneluri;
				#warning ??
				GetPPoolItem(ppool, pi, DSI_LISTPOOL_PANEL_URI, (APTR) &paneluri , NULL );  
				GetPPoolItem(ppool, pi, DSI_LISTPOOL_PANEL_IMAGEPATH, (APTR) &imagepath , NULL );  
			
				o = NewObject( GetClass("CommandButton"), NULL,
					( ( pi && GetPPoolItem(ppool, pi, DSI_LISTPOOL_PANEL_IMAGEPATH, (APTR) &imagepath, NULL ) ) ? MA_Panel_Imagepath : TAG_IGNORE ), imagepath,
					( ( pi && GetPPoolItem(ppool, pi, DSI_LISTPOOL_PANEL_URI, (APTR) &paneluri, NULL ) ) ? MA_Panel_URI : TAG_IGNORE ), paneluri,
						TAG_DONE );
			}
			break;

		case MV_Panel_Type_SubPanel:
			{
				APTR subpanel, group;
				STRPTR imagepath, backdrop;
				ULONG *subpanelroot, *backmode, *backcolor;
			//	ULONG a,b,c;
				GetPPoolItem(ppool, pi, DSI_LISTPOOL_PANEL_SUBPANEL_ROOT , (APTR) &subpanelroot, NULL );
				o = NewObject( GetClass("SubPanelButton"), NULL,
				#warning	default image
					( ( pi && GetPPoolItem(ppool, pi, DSI_LISTPOOL_PANEL_IMAGEPATH, (APTR) &imagepath, NULL ) ) ? MA_Panel_Imagepath : TAG_IGNORE ), imagepath,
				//						MA_Panel_Imagepath,0,//imagepath,//"sys:tools/format.info", 	
						MA_SubPanel_ID , *subpanelroot,
						TAG_DONE );
				
				}

			break;

#if 1
		case MV_Panel_Type_External:
			{
				APTR sobj;
				STRPTR classname;
				//STRPTR class_name;
				#define EXTCLASSPATH_SIZEOF 0x100
				//TEXT extclasspath[ EXTCLASSPATH_SIZEOF ] = "MOSSYS:Classes/Panels";
				TEXT extclasspath[ EXTCLASSPATH_SIZEOF ] = "sys:Classes/Panels2";
				//ASSERT( pi );
				GetPPoolItem(ppool, pi, DSI_LISTPOOL_PANEL_EXT_NAME, (APTR) &classname , NULL );
			
			//	PDB(("%s\n",class_name));
			//	sobj = NewObject( getpanelexternalsupportclass(), NULL, MA_Panelsupport_PPool, msg->prefspool, MA_Panelsupport_PItem, pi, TAG_DONE );
			//	prefspool_item_get( msg->prefspool, pi, DSI_LISTPOOL_PANEL_EXT_NAME, (APTR) &classname, NULL );
				AddPart( extclasspath, classname, EXTCLASSPATH_SIZEOF );
			//	PDB(("%x %x\n",panel_lock,pi));
				{
					ULONG *x,xx;
				//	xx = GetPPoolItem(ppool, pi,  11 , (APTR) &x, NULL );	
				//	PDB(("%x %x %x\n",x,xx,*x));
				}
				if( !( o = MUI_NewObject( extclasspath, MA_Panel_PrefsPool,ppool,MA_Panel_PrefsIndex,pi,TAG_DONE ) ) ) {
				//	if( !( o = MUI_NewObject( &extclasspath[3], MA_Panelextern_SupportObject, sobj,MA_Panelsupport_PPool, msg->prefspool, MA_Panelsupport_PItem, pi,  TAG_DONE ) ) ) {
				//	}
				}
				//PDB(("%x %s\n",o,extclasspath));
				if( o )
				{
				//	set( sobj, MA_Panelsupport_Object, o ); /* tell support object which object to support */
				}
			}
			break;
#endif
		case MV_Panel_Type_Spacer:
			{
				o = NewObject( GetClass("Spacer"), NULL, TAG_DONE );
			}
			break;

		case MV_Panel_Type_Separator:
			{
				o = NewObject( GetClass("Separator"), NULL, TAG_DONE );
			}
			break;

		case MV_Panel_Type_ViewWatcher:
			{
				STRPTR imagepath;
				o = NewObject( GetClass("ViewWatcher"), NULL,
					( ( pi && GetPPoolItem(ppool, pi, DSI_LISTPOOL_PANEL_IMAGEPATH, (APTR) &imagepath, NULL ) ) ? MA_Panel_Imagepath : TAG_IGNORE ), imagepath,
					TAG_DONE );
			}
			break;

		case MV_Panel_Type_Bookmarks:
			{
                STRPTR imagepath;
				o = NewObject( GetClass("Bookmarks"), NULL,
					( ( pi && GetPPoolItem(ppool, pi, DSI_LISTPOOL_PANEL_IMAGEPATH, (APTR) &imagepath, NULL ) ) ? MA_Panel_Imagepath : TAG_IGNORE ), imagepath,
					TAG_DONE );
			}
			break;

		#ifdef DEBUG
		default:
			//PDB(("wrong type %d\n"));
			break;
		#endif

	}
#endif
	return (  o );
}


static ULONG mSetObjectAttr(struct IClass *cl,Object *obj,struct MP_Application_SetObjectAttr *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);	
	APTR win;
	ULONG type,open;
	STRPTR p_name;
	ULONG retval = 0;
	APTR win_state;
	struct List *win_list;
	GetAttr( MUIA_Application_WindowList, app, (ULONG*) &win_list );
	win_state = win_list->lh_Head;
	while( ( win = NextObject( &win_state ) ) )
	{
		if( ( GetAttr( MA_Panelwin_Type, win, &type ) ) && ( type == MV_Panelwin_Type_Root ) )
		{
			if(  GetAttr( MA_Panelwin_Name, win, (ULONG*)&p_name))
			{
				APTR truncation = name_truncateprefs(p_name);
				if(!strcmp(p_name,msg->p_name))
				{
					APTR tbar = 0;
					GetAttr(MA_Panelwin_Group,win,(ULONG*)&tbar);
					if(tbar)
					{
						if(msg->obj_id == 0)
						{
							SetAttrs(tbar,msg->ti_Tag,msg->ti_Data,TAG_DONE);
							SetAttrs(win,msg->ti_Tag,msg->ti_Data,TAG_DONE);
						}
						else
						{
							APTR target = 0;
							DoMethod(tbar,MM_Panelgroup_FindID,msg->obj_id,0,&target);
							if(target) 
							{
								SetAttrs(target,msg->ti_Tag,msg->ti_Data,TAG_DONE);
							}
						}
					}
				}
				name_restoreprefs(p_name, truncation);
			}
		}
	
	}
	
	
	
	return retval;
}

static BOOL FindPos(Object *obj,STRPTR panel_name,LONG id,ULONG pos,APTR *root_win, APTR *group_obj,ULONG *target_pos)
{
	APTR win_state;
	struct List *win_list;
	ULONG type;
	APTR win;
	
	GetAttr( MUIA_Application_WindowList, obj, (ULONG*) &win_list );
	win_state = win_list->lh_Head;
	while( ( win = NextObject( &win_state ) ) )
	{
		if( ( GetAttr( MA_Panelwin_Type, win, &type ) ) && ( type == MV_Panelwin_Type_Root ) )
		{
			STRPTR win_name;
			if(GetAttr(MA_Panelwin_Name,win,(ULONG*)&win_name))
			{
				if(!strcmp(win_name,panel_name))
				{
					APTR tbar = 0;
					ULONG real_pos = 0;
					*root_win = win;
					GetAttr(MA_Panelwin_Group,win,(ULONG*)&tbar);
					if(tbar)
					{	
						APTR target = 0;
						DoMethod(tbar,MM_Panelgroup_FindID,id,0,&target);
						if(pos == MV_Application_InsertObject_PosRootHead)
						{
							*group_obj = tbar;
							*target_pos = 0;
							return TRUE;
						}
						if((pos == MV_Application_InsertObject_PosHead) &&(target))// && (msg->id >= 0))
						{
							ULONG type;
							APTR subpanel_win;
							GetAttr(MA_Panel_Type,target,&type);
							if((type == MV_Panel_Type_SubPanel)
							 &&(GetAttr(MA_Panelbutton_AttachedObject,target,(ULONG*)&subpanel_win))&&(subpanel_win)
							 &&(GetAttr(MA_Panelwin_Group,subpanel_win,(ULONG*)&tbar))&&(tbar))
							{
								*group_obj = tbar;
								*target_pos = 0;
								return TRUE;
							}
							else return FALSE;
						}
						else
						{
							if(target) GetAttr(MUIA_Parent,target,(ULONG*)&tbar);
							if(pos == MV_Application_InsertObject_PosTail) real_pos = -1;
							else if(pos == MV_Application_InsertObject_PosNone)
							{
								APTR object_state;
								struct List *button_list = 0L;
								Object *button;
								
								GetAttr( MUIA_Group_ChildList, tbar , (ULONG*) &button_list );
								if(!(button_list)||!(target)||(!(object_state = button_list->lh_Head))) return 0;
								while( ( button = (Object*) NextObject( &object_state ) ) && ( ( button != target )  ) )
								{
									real_pos++;
								}
							}
						}
						*group_obj = tbar;
						*target_pos = real_pos;
						return TRUE;
					}
				}
			}
		}
	}
	return FALSE;
}

static ULONG mInsertObject(struct IClass *cl,Object *obj,struct MP_Application_InsertObject *msg)
{
	ULONG real_pos;
	APTR tbar = 0;
	APTR root_win = 0;
	if((FindPos(obj,msg->panel_name,msg->id,msg->pos,&root_win,&tbar,&real_pos))&&(tbar)&&(root_win))
	{
		DoMethod(tbar, MUIM_Group_InitChange);
		DoMethod(tbar, OM_ADDMEMBER, msg->obj );
		DoMethod(tbar, MUIM_Group_MoveMember, msg->obj, real_pos );
		DoMethod(tbar, MUIM_Group_ExitChange );
		SetAttrs(root_win, MUIA_Window_Width, MUIV_Window_Width_MinMax( 0 ), TAG_DONE );	
		SetAttrs(root_win, MUIA_Window_Height, MUIV_Window_Height_MinMax( 0 ), TAG_DONE );
		DoMethod(root_win, MM_Panelwin_SaveConfig,0);
		PanelHasChanged(msg->panel_name,PanelHasChanged_Insert,msg->pos,0);
	}
	return 0;
}

static ULONG mDeleteObject(struct IClass *cl,Object *obj,struct MP_Application_DeleteObject *msg)
{
//	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	APTR win_state;
	struct List *win_list;
	ULONG type;
	APTR win;
	
//	return 0;
	GetAttr( MUIA_Application_WindowList, obj, (ULONG*) &win_list );
	win_state = win_list->lh_Head;
	while( ( win = NextObject( &win_state ) ) )
	{
		if( ( GetAttr( MA_Panelwin_Type, win, &type ) ) && ( type == MV_Panelwin_Type_Root ) )
		{
			STRPTR win_name;
			if(GetAttr(MA_Panelwin_Name,win,(ULONG*)&win_name))
			{
				if(!strcmp(win_name,msg->panel_name))
				{
					if(msg->id == 0)
					{
						SetAttrs(win,MUIA_Window_Open,FALSE);
						DoMethod(obj, OM_REMMEMBER, win );
						MUI_DisposeObject(win);
						return 0;
					}
					else
					{
						APTR root_tbar = 0;
						APTR tbar;
						GetAttr(MA_Panelwin_Group,win,(ULONG*)&root_tbar);
						if(root_tbar)
						{	
							APTR target = 0;
							DoMethod(root_tbar,MM_Panelgroup_FindID,msg->id,0,&target);
							if(target)
							{
								GetAttr(MUIA_Parent,target,(ULONG*)&tbar);
								DoMethod(tbar, MUIM_Group_InitChange);
								DoMethod(tbar, OM_REMMEMBER, target );
								DoMethod(tbar, MUIM_Group_ExitChange );
								SetAttrs(win, MUIA_Window_Width, MUIV_Window_Width_MinMax( 0 ), TAG_DONE );
								DoMethod(win, MM_Panelwin_SaveConfig,0);
								PanelHasChanged(msg->panel_name,PanelHasChanged_Remove,0,0);
								return 0;
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

static ULONG mMoveObject(struct IClass *cl,Object *obj,struct MP_Application_MoveObject *msg)
{
	ULONG dst_real_pos,src_real_pos;
	APTR dst_tbar = 0;
	APTR dst_root_win = 0;
	APTR src_tbar = 0;
	APTR src_root_win = 0;
	
	if((FindPos(obj,msg->dst_panel_name,msg->dst_id,msg->pos,&dst_root_win,&dst_tbar,&dst_real_pos))&&(dst_tbar)&&(dst_root_win)
	 &&(FindPos(obj,msg->src_panel_name,msg->src_id,0,&src_root_win,&src_tbar,&src_real_pos))&&(src_tbar)&&(src_root_win))
	{
		APTR move_obj;
		APTR root_tbar = 0;
		GetAttr(MA_Panelwin_Group,src_root_win,(ULONG*)&root_tbar);
		if(root_tbar)
		{
			DoMethod(root_tbar,MM_Panelgroup_FindID,msg->src_id,0,&move_obj);
			if(move_obj)
			{
				if(dst_tbar == src_tbar)
				{
					if(dst_real_pos > src_real_pos) dst_real_pos--;
					DoMethod(dst_tbar, MUIM_Group_InitChange);
					DoMethod(dst_tbar, MUIM_Group_MoveMember, move_obj, dst_real_pos );
					DoMethod(dst_tbar, MUIM_Group_ExitChange );
					SetAttrs(dst_root_win, MUIA_Window_Width, MUIV_Window_Width_MinMax( 0 ), TAG_DONE );
				}
				else
				{
					DoMethod(src_tbar, MUIM_Group_InitChange);
					DoMethod(src_tbar, OM_REMMEMBER, move_obj );
					DoMethod(src_tbar, MUIM_Group_ExitChange);
					DoMethod(dst_tbar, MUIM_Group_InitChange);					
					DoMethod(dst_tbar, OM_ADDMEMBER, move_obj );
					DoMethod(dst_tbar, MUIM_Group_MoveMember, move_obj, dst_real_pos );
					DoMethod(dst_tbar, MUIM_Group_ExitChange);
					if(dst_root_win != src_root_win)
					{
						SetAttrs(src_root_win, MUIA_Window_Width, MUIV_Window_Width_MinMax( 0 ), TAG_DONE );
						DoMethod(src_root_win, MM_Panelwin_SaveConfig,0);
						PanelHasChanged(msg->src_panel_name,PanelHasChanged_Remove,0,0);
					}
				}
				SetAttrs(dst_root_win, MUIA_Window_Width, MUIV_Window_Width_MinMax( 0 ), TAG_DONE );
				DoMethod(dst_root_win, MM_Panelwin_SaveConfig,0);
				PanelHasChanged(msg->dst_panel_name,PanelHasChanged_Remove,0,0);
			}
		}
	}
	return 0;
}


static ULONG mSavePanel(struct IClass *cl,Object *obj,struct MP_Application_SavePanel *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);	
	APTR win;
	APTR win_state;
	struct List *win_list;
	ULONG type;
	GetAttr( MUIA_Application_WindowList, app, (ULONG*) &win_list );
	win_state = win_list->lh_Head;
	while( ( win = NextObject( &win_state ) ) )
	{
		if( ( GetAttr( MA_Panelwin_Type, win, &type ) ) && ( type == MV_Panelwin_Type_Root ) )
		{
			STRPTR win_name,file_name;
			if(GetAttr(MA_Panelwin_Name,win,(ULONG*)&win_name))
			{
				#warning too complex
				if((GetAttr(MA_Panelwin_FileName,win,(ULONG*)&file_name))&&(file_name))
				{
					if((!strcmp(file_name,msg->p_name)))//&&(strcmp(win_name,file_name)))
					{
						DoMethod(win,MM_Panelwin_SaveConfig,MV_Panelwin_SaveConfig_Write);
						return 0;
					}
				
				}
				if(!strcmp(win_name,msg->p_name))
				{
					DoMethod(win,MM_Panelwin_SaveConfig,MV_Panelwin_SaveConfig_Write);
					return 0;
				}
			}
		}
	}
	return 0;
}

static ULONG mFindPanel(struct IClass *cl,Object *obj,struct MP_Application_FindPanel *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);	
	APTR win;
	APTR win_state;
	struct List *win_list;
	ULONG type;
	GetAttr( MUIA_Application_WindowList, app, (ULONG*) &win_list );
	win_state = win_list->lh_Head;
	while( ( win = NextObject( &win_state ) ) )
	{
		if( ( GetAttr( MA_Panelwin_Type, win, &type ) ) && ( type == MV_Panelwin_Type_Root ) )
		{
			STRPTR win_name,file_name;
			if(GetAttr(MA_Panelwin_Name,win,(ULONG*)&win_name))
			{
				if(!strcmp(win_name,msg->p_name))
				{
					return win;
				}
			}
		}
	}
	return 0;
}


static ULONG mDeletePanel(struct IClass *cl,Object *obj,struct MP_Application_DeletePanel *msg)
{
	return 0;
}

static ULONG mNewPanel(struct IClass *cl,Object *obj,struct MP_Application_NewPanel *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);	
	APTR win;
	APTR win_state;
	struct List *win_list;
	ULONG type;
	TEXT new_win_name[30];	
	TEXT prefs_name[30];
	ULONG prefs_nr = 0;
	BOOL found = FALSE;
	APTR ppool;
	GetAttr( MUIA_Application_WindowList, app, (ULONG*) &win_list );
	do
	{
		prefs_nr++;
		found = FALSE;
		sprintf(new_win_name,"New_Panel_%d\0",prefs_nr);
		win_state = win_list->lh_Head;
		while( ( win = NextObject( &win_state ) ) )
		{
			if( ( GetAttr( MA_Panelwin_Type, win, &type ) ) && ( type == MV_Panelwin_Type_Root ) )
			{
				STRPTR win_name,file_name;
				if(GetAttr(MA_Panelwin_Name,win,(ULONG*)&win_name))
				{
					if(!strcmp(win_name,new_win_name))
					{
						found = TRUE;
					}
				}
			}
		}
	}
	while(found == TRUE);
	sprintf(prefs_name,"%s.prefs\0",new_win_name);
	if((win =  NewObject(GetClass("DefaultWindow"),NULL, MA_Panelwin_Name,new_win_name,TAG_DONE)))
	{
		DoMethod(app,OM_ADDMEMBER,win);
		DoMethod(win,MM_PanelWindow_LoadPanel);
		SetAttrs(win,MUIA_Window_Open,TRUE,TAG_DONE);
	}
	return 0;
}

static ULONG mLoadAll(struct IClass *cl,Object *obj,Msg msg)
{
	
	
#if 1
	
	APTR ctx = 0;
	LONG more = 0;
	BPTR lock;
	APTR ppool;
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	APTR win;
	APTR win_state;
	struct List *win_list;
	ULONG type;
	STRPTR ppool_name = 0;
	if((GetAttr( MUIA_Application_WindowList, app, (ULONG*) &win_list ))&&(win_list))
	{
		if( data->ihnode.ihn_Object != NULL )
		{
			DoMethod( obj, MUIM_Application_RemInputHandler, &data->ihnode );
		}
		data->zip_ct = 0;
		memset( &data->ihnode, 0, sizeof( struct MUI_InputHandlerNode ) );
		win_state = win_list->lh_Head;
		while( ( win = NextObject( &win_state ) ) )
		{
			if( ( GetAttr( MA_Panelwin_Type, win, &type ) ) && ( type == MV_Panelwin_Type_Root ) )
			{
				SetAttrs(win,MUIA_Window_Open,FALSE);
				if( ( GetAttr(MA_Panelwin_Name,win,(ULONG*)&ppool_name ) ) && ( ppool_name ) )
				{
					APTR ppool;
					if((ppool = LockPPool(ppool_name,PPOOL_TYPE_PANEL)))
					{
						ReReadPPool(ppool);
						UnLockPPool(ppool);
					}
				}
				DoMethod(obj, OM_REMMEMBER, win );
				MUI_DisposeObject(win);
			}
		}
	}
	ppool_name = 0;
	while((ppool_name = NextPPoolName(ppool_name,PPOOL_TYPE_PANEL)))
	{
		if((win =  NewObject(GetClass("DefaultWindow"),NULL, MA_Panelwin_Name,ppool_name,TAG_DONE)))
		{
			DoMethod(app,OM_ADDMEMBER,win);
			DoMethod(win,MM_PanelWindow_LoadPanel);
			SetAttrs(win,MUIA_Window_Open,TRUE,TAG_DONE);
		}
	}
#endif
	
 
}

#if 1

static ULONG mScanDirectory(struct IClass *cl,Object *obj,struct MP_Application_ScanDirectory *msg)
{
	#warning
	#if 0
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	struct Node *node;
	APTR PathNode = 0;
	#define SCANFILENAME_SIZEOF 0x200
	TEXT filename[ SCANFILENAME_SIZEOF ];

	ULONG QueryTags[] =
	{
		QUERYFINDATTR_CLASS     , QUERYCLASS_AMBIENT,
		QUERYFINDATTR_SUBCLASS  , QUERYSUBCLASS_AMBIENT_PANEL,
		TAG_DONE
	};

	void *queryinfo = NULL;
	BPTR tick;
	if((tick = Open("SYS:Classes/Panels2/DigiClock.pobj",MODE_OLDFILE)))
	{
		AddExternClass("SYS:Classes/Panels2","DigiClock.pobj");
		Close(tick);
	}	
	if(0)//( PathNode = QueryCreatePathNode( msg->path, 0, QUERYPATHFLAGF_ALL ) ) )
	{

		while(( queryinfo = QueryObtainTagList( queryinfo, (struct TagItem *) QueryTags ) ) )
		{
			char  *class_name;
			//PDB(("\n"));
			
			if( QueryGetAttr( queryinfo, (ULONG *) &class_name, QUERYINFOATTR_NAME ) )
			{
				//PDB(("\n"));
				if( class_name && !( strcmp( &class_name[ strlen( class_name ) - 5 ],".pobj" ) ) )
				{
					strcpy( filename, msg->path );
					AddPart(filename, class_name, SCANFILENAME_SIZEOF );
				//	PDB(("%s\n",class_name));
					AddExternClass(msg->path,class_name);
				}
			}
	
		}
#warning will crash if Ambient is closed/reopened add check for stuntzi env
	//	QueryDeletePathNode( PathNode );
	}
#endif
	return( 0 );
}
#endif
#if 0
static ULONG mObtainObject(struct IClass *cl,Object *obj,struct MP_Application_ObtainObject *msg)
{
	//struct Data *data = (struct Data*)INST_DATA(cl,obj);
	struct TagItem *winTag = 0;
	struct TagItem *win_nameTag = 0;
	struct TagItem *typeTag = 0;
	struct TagItem *targetTag = 0;
	struct TagItem *uriTag = 0;
	struct TagItem *startTag = 0;
	APTR panel_win = 0;
	APTR panel_obj = 0;
	APTR ret_obj = 0;
//	PDB(("%x %x %x %x\n",cl,obj,PanelObjectURI,PanelObjectID));
//	PDB(("%x %x %x %x %x %x\n",msg->tags[0].ti_Tag,msg->tags[0].ti_Data,msg->tags[1].ti_Tag,msg->tags[1].ti_Data,msg->tags[2].ti_Tag,msg->tags[2].ti_Data));
//	PDB(("%x %x\n",&msg->tags[0],&msg->tags[1]));
	typeTag = FindTagItem(PanelObjectType,msg->tags);

	if((startTag = FindTagItem(PanelObjectStartObject,msg->tags))&& (startTag->ti_Data))
	{
	//	PDB(("%x start obj %x\n",startTag,startTag->ti_Data));
		if(typeTag)
		{
		//	PDB(("type %x\n",typeTag->ti_Data));
			switch (typeTag->ti_Data)
			{
				case  PanelObject_Group:
					GetAttr( MUIA_Parent, (APTR)startTag->ti_Data, &ret_obj );
			//		PDB(("%x\n",ret_obj));
					return ret_obj;
			}
		}
	}
	if(typeTag)// = FindTagItem(PanelObjectType,msg->tags)))
	{
		if(typeTag->ti_Data == PanelObjectType_Command)
		{
			if((uriTag = FindTagItem(PanelObjectURI,msg->tags))&&(uriTag->ti_Data))
			{
				APTR win_state;
				STRPTR p_name;
				struct List *win_list;
				APTR win;
				APTR o_obj;
				ULONG type;
			//	PDB(("%s\n",uriTag->ti_Data));
				GetAttr( MUIA_Application_WindowList, app, (ULONG*) &win_list );
				win_state = win_list->lh_Head;
				while( ( win = NextObject( &win_state ) ) )
				{
					if( ( GetAttr( MA_Panelwin_Type, win, &type ) ) && ( type == MV_Panelwin_Type_Root ) )
					{
						if(DoMethod(win,MM_Panelwin_FindObject,0,uriTag->ti_Data,&o_obj))
						{
						//	PDB(("%x\n",o_obj));
							return o_obj;
						}
					}
				}
		
			}
		}
	}
	
	if((winTag = FindTagItem(PanelObjectWindow,msg->tags))&&(winTag->ti_Data))
	{
		panel_win = (APTR)winTag->ti_Data;
	}
	else if((win_nameTag = FindTagItem(PanelObjectWindowName,msg->tags))&&(win_nameTag->ti_Data))
	{
		APTR win_state;
		STRPTR p_name;
		struct List *win_list;
		APTR win;
		ULONG type;
	//	PDB(("\n"));
		GetAttr( MUIA_Application_WindowList, app, (ULONG*) &win_list );
		win_state = win_list->lh_Head;
		while( ( win = NextObject( &win_state ) ) )
		{
			if( ( GetAttr( MA_Panelwin_Type, win, &type ) ) && ( type == MV_Panelwin_Type_Root ) )
			{
				if(  GetAttr( MA_Panelwin_Name, win, (ULONG*)&p_name))
				{
					APTR truncation = name_truncateprefs(p_name);
					if(!strcmp(p_name,(STRPTR)win_nameTag->ti_Data))
					{
						panel_win = win;
					//	PDB(("pwin %x\n",panel_win));
					}
				}
			}
		}
	}
	if((panel_win))//&&(typeTag == FindTagItem(PanelObjectType,msg->tags)))
	{
		
		struct TagItem *uriTag = 0;
		struct TagItem *idTag = 0;
		//if(typeTag = PanelObject_Window) return panel_win;
		if((uriTag = FindTagItem(PanelObjectURI,msg->tags))&&(uriTag->ti_Data))
		{
		//	PDB(("\n"));
			DoMethod(panel_win,MM_Panelwin_FindObject,PanelObject_Object,(STRPTR)uriTag->ti_Data,&panel_obj); 
		}
		else if((idTag = FindTagItem(PanelObjectID,msg->tags)))
		{
			APTR tbar;
		//	PDB(("id %d\n",idTag->ti_Data));
			if(idTag->ti_Data == 0)
			{
				ret_obj = panel_win;
			}
			if((GetAttr(MA_Panelwin_Group,panel_win,(ULONG*)&tbar))&&(tbar))
			{
				DoMethod(tbar,MM_Panelgroup_FindID,idTag->ti_Data,0,&panel_obj);
				ret_obj = panel_obj;
			}				
		}
		else
		{
			ret_obj = panel_win;
		}
		if((targetTag = FindTagItem(PanelObjectTarget,msg->tags)))
		{
			switch(targetTag->ti_Data)
			{
				case Target_Group:
					GetAttr(MA_Panelwin_Group,panel_win,(ULONG*)&ret_obj);
					break;
				case Target_Window:
					ret_obj = panel_win;
					break;
			}
		}
	}
	return ret_obj;
}

static ULONG mReleaseObject(struct IClass *cl,Object *obj,struct MP_Application_ReleaseObject *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	return 0;
}
#endif
static ULONG mSaveMainprefs(struct IClass *cl,Object *obj,Msg msg)
{
	//PDB(("\n"));
//	prefspool_write(mainprefspool, PANEL_PREFS_PATH PANEL_FILE, GLOBALPANELPREFSID , FALSE);
	return 0;
}

static ULONG mPrefsPoolUpdate(struct IClass *cl,Object *obj,struct MP_Application_PrefsPoolUpdate *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	APTR win;
	APTR win_state;
	struct List *win_list;
	ULONG type;
	APTR pl,pi;
//	PDB(("%s %d\n",msg->panel_name,msg->object_ID));	
	if((GetAttr( MUIA_Application_WindowList, app, (ULONG*) &win_list ))&&(win_list))
	{
		win_state = win_list->lh_Head;
		while( ( win = NextObject( &win_state ) ) )
		{
			if( ( GetAttr( MA_Panelwin_Type, win, &type ) ) && ( type == MV_Panelwin_Type_Root ) )
			{
				STRPTR win_name;
				if((GetAttr(MA_Panelwin_Name,win,(ULONG*)&win_name))&&(win_name))
				{
				//	PDB(("%s %s\n",win_name,msg->panel_name));
					if(!strcmp(win_name,msg->panel_name))
					{
						if(msg->object_ID == 0)
						{
							APTR ppool;
							if((ppool = LockPPool(msg->panel_name,PPOOL_TYPE_PANEL)))
							{	
								
								DoMethod(win,MM_Panel_PrefsUpdate,ppool,0,msg->prefs_ID);
								UnLockPPool(ppool);
							}
						}
						else
						{
							APTR tbar,target_obj;
							if(GetAttr(MA_Panelwin_Group,win,(ULONG*)&tbar)&&(tbar))
							{
								if(DoMethod(tbar,MM_Panelgroup_FindID,msg->object_ID,0,&target_obj))
								{
									APTR ppool;
									if((ppool = LockPPool(msg->panel_name,PPOOL_TYPE_PANEL)))
									{	
										
										if((pl = (APTR)GetPPoolItem(ppool,0,  DSI_LISTPOOL_PANEL ,NULL,NULL)))
										{
											if((pi = (APTR)GetPPoolItem(ppool,pl, msg->object_ID | DSF_LISTPOOL, NULL, NULL)))
											{
												DoMethod(target_obj,MM_Panel_PrefsUpdate,ppool,pi,msg->prefs_ID);
											}
										}
										UnLockPPool(ppool);
									}
								}
							}								
						}
					}
				}
			}
		}
	}
	return 0;
}


static ULONG mModus(struct IClass *cl,Object *obj,struct MP_Application_Modus *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	APTR win;
	APTR win_state;
	struct List *win_list;
	ULONG type;
	APTR pl,pi;
	if((GetAttr( MUIA_Application_WindowList, app, (ULONG*) &win_list ))&&(win_list))
	{
		win_state = win_list->lh_Head;
		while( ( win = NextObject( &win_state ) ) )
		{
			if( ( GetAttr( MA_Panelwin_Type, win, &type ) ) && ( type == MV_Panelwin_Type_Root ) )
			{
				STRPTR win_name;
				if((GetAttr(MA_Panelwin_Name,win,(ULONG*)&win_name))&&(win_name))
				{
					APTR tbar;
					if(GetAttr(MA_Panelwin_Group,win,(ULONG*)&tbar)&&(tbar))
					{	
						if((msg->panel_name)&&(!strcmp(win_name,msg->panel_name)))
						{
							SetAttrs(tbar,MA_Panelgroup_FixedMode,TRUE);
						}
						else
						{
							SetAttrs(tbar,MA_Panelgroup_FixedMode,FALSE);
						}
					}
				}
			}
		}
	}
	return 0;
}

DISPATCHER(ApplicationClass)
{
    struct Data *data = (struct Data*)INST_DATA(cl,obj);
	switch (msg->MethodID)
	{
		case OM_NEW               				: return(mNew    					(cl,obj,(struct opSet *)msg));
		case OM_DISPOSE           				: return(mDispose					(cl,obj,(Msg)msg));
		case OM_SET                 			: return(mSet           			(cl,obj,(struct opSet *)msg));
		case OM_GET			    				: return(mGet						(cl,obj,(struct opGet *)msg));
		case MM_Application_NewPanel			: return(mNewPanel					(cl,obj,(struct MP_Application_NewPanel *)msg));
		case MM_Application_StartZipping		: return(mStartZipping				(cl,obj,(struct MP_Application_StartZipping *)msg));
		case MM_Application_ZipTimer			: return(mZipTimer 					(cl,obj,(Msg)msg));
	//??	case MM_Application_CreatePanelitem		: return(mCreatePanelitem			(cl,obj,(struct MP_Application_CreatePanelitem *)msg));
		case MM_Application_SetObjectAttr		: return(mSetObjectAttr				(cl,obj,(struct MP_Application_SetObjectAttr*)msg));
		case MM_Application_InsertObject		: return(mInsertObject				(cl,obj,(struct MP_Application_InsertObject*)msg));
		case MM_Application_DeleteObject		: return(mDeleteObject				(cl,obj,(struct MP_Application_DeleteObject*)msg));
		case MM_Application_MoveObject			: return(mMoveObject				(cl,obj,(struct MP_Application_MoveObject*)msg));
		case MM_Application_SavePanel			: return(mSavePanel					(cl,obj,(struct MP_Application_SavePanel*)msg));
		case MM_Application_FindPanel			: return(mFindPanel					(cl,obj,(struct MP_Application_FindPanel*)msg));
		case MM_Application_DeletePanel			: return(mDeletePanel				(cl,obj,(struct MP_Application_DeletePanel*)msg));
		case MM_Application_LoadAll				: return(mLoadAll					(cl,obj,(Msg)msg));
		case MM_Application_SaveMainprefs		: return(mSaveMainprefs				(cl,obj,(Msg)msg));
		case MM_Application_ScanDirectory		: return(mScanDirectory				(cl,obj,(struct MP_Application_ScanDirectory*)msg));
		case MM_Application_PrefsPoolUpdate		: return(mPrefsPoolUpdate			(cl,obj,(struct MP_Application_PrefsPoolUpdate*)msg));
		case MM_Application_Modus				: return(mModus						(cl,obj,(struct MP_Application_Modus*)msg));
	//	case MM_Application_ObtainObject		: return(mObtainObject				(cl,obj,(struct MP_Application_ObtainObject*)msg));
	//	case MM_Application_ReleaseObject		: return(mReleaseObject				(cl,obj,(struct MP_Application_ReleaseObject*)msg));
	//	default: kprintf("app %x\n",msg->MethodID);
	}
  return(DoSuperMethodA(cl,obj,msg));
 }
DISPATCHER_END

 

ULONG icon_init(void);

struct Library *AmbientBase;
int main(int argc,char **argv)
{
	struct Data *data;
	ULONG sigs = 0;
	init(); 
	TEXT buf[5];
	if ((GetVar("panel_modus",  buf, sizeof(buf), GVF_GLOBAL_ONLY) > 2) )
	{
		if(!strncmp(buf,"INT",3)) panel_modus = 1;
		if(!strncmp(buf,"EXT",3)) panel_modus = 2;
		if(!strncmp(buf,"BOTH",4)) panel_modus = 3;
	}
	if(panel_modus == 1)
	{
		struct EasyStruct modusES = {
		    sizeof (struct EasyStruct),
		    0,
		    "PanelApp",
		    "You need to set the ENV variable \"panel_modus\"\nto either \"EXT\" for replacing buildin panels\nor \"BOTH\" to use both buildin and PanelApp",
		    "O.K",
		};
		EasyRequest( 0, &modusES, 0,0 );
		return 0;	
	}
	screen = LockPubScreen("Workbench");
	icon_init();
	AmbientBase = OpenLibrary("ambient.library", AMBIENT_LIB_VERSION);
	if(AmbientBase == 0) exit(20);
	if(AmbientBase->lib_Revision != AMBIENT_LIB_REVISION)
	{
		PDB(("ambient.lib rev got %d want %d\n",AmbientBase->lib_Revision,AMBIENT_LIB_REVISION));
		CloseLibrary(AmbientBase);
		exit(220);
	}
	if((VGraphicsBase = OpenLibrary("vgraphics.library", 0))
	 &&(CGXDitherBase = OpenLibrary("cgxdither.library", 50))
	 &&(WBStartBase	= OpenLibrary("wbstart.library", 0)))
	{


	panellib_init();
	
	PanelBase = OpenLibrary("panel.library",0);
	if(PanelBase == 0) exit(20);
	if(!methodstack_init()) exit(20);
		
	app =  (Object*)NewObject(GetClass("Application"),NULL,
		MUIA_Application_Title      , "PANELAPP",
	#warning autocreate version and date
		MUIA_Application_Version    , "Panel_App_0.12",
		MUIA_Application_Copyright  , "©2026 Stefan Kleinheinrich\0",
		MUIA_Application_Author     , "Stefan Kleinheinrich",
		MUIA_Application_SingleTask	, TRUE,
	
		End;
	if (!app) exit(0);
	if((panel_port = CreateMsgPort()))
	{
		DoMethod(app,MM_Application_LoadAll);
		DoMethod(app,MM_Application_ScanDirectory,"sys:classes/Panels2");
	}
	{
	#warning
	//DoMethod(app,MM_Application_ScanDirectory,"PROGDIR:Classes/Panel");
	//DoMethod(app,MUIM_Notify,MUIA_Application_DoubleStart,TRUE,window,3,MUIM_Set,MUIA_Window_Open,TRUE);
	
	
		while (DoMethod(app,MUIM_Application_NewInput,&sigs) != MUIV_Application_ReturnID_Quit)
		{
			methodstack_check(FALSE);
			if (sigs)
			{
				sigs = Wait(sigs | SIGBREAKF_CTRL_C |  (1<< panel_port->mp_SigBit));
				if (sigs & (1<<panel_port->mp_SigBit)) 	
				{
					
					
					struct AppMessage *appmsg;
					if((appmsg = (struct AppMessage *)GetMsg(panel_port)))
					{
					//	PDB(("%x %x %d %d\n",appmsg->am_Class,appmsg->am_UserData,appmsg->am_MouseX,appmsg->am_MouseY));
						DoMethod(appmsg->am_UserData,MM_Panelgroup_AppMessage,appmsg);
						
						ReplyMsg((struct Message *)appmsg);
					}
				
				//	DoMethod(prefsclass,MM_PanelPrefs_FilesChanged);
				}	
				if (sigs & SIGBREAKF_CTRL_C) break;
			}
		}
		methodstack_cleanup();
		DeleteMsgPort(panel_port);
	}

	
	//SetAttrs(window,MUIA_Window_Open,FALSE,TAG_DONE);


/*
** Shut down...
*/
//	methodstack_cleanup();
	MUI_DisposeObject(app);     /* dispose all objects. */
	
	}
	if(PanelBase)CloseLibrary(PanelBase);
	
	preclose_panellib();
	panellib_cleanup();
	RemClasses(); 
	if(VGraphicsBase) CloseLibrary(VGraphicsBase);
	if(CGXDitherBase) CloseLibrary(CGXDitherBase);
	if(AmbientBase) CloseLibrary(AmbientBase);
	if(WBStartBase) CloseLibrary(WBStartBase);
	
	app = 0;

	fail(NULL,NULL);            /* exit, app is already disposed. */
	return 0;
}

struct MUI_CustomClass *Application_Class(void)
{
	return MUI_CreateCustomClass(NULL,MUIC_Application,NULL,sizeof(struct Data),DISPATCHER_REF(ApplicationClass));
}

