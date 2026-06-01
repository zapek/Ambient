#include "muifuncs.h"

//#include "debug.h" 


#define USE_INLINE_STDARG


#include <exec/libraries.h>
#include <libraries/commodities.h>
#include <dos/dos.h>
#include <clib/exec_protos.h>
#include <proto/asound.h>
#include <clib/alib_protos.h>
#include <clib/commodities_protos.h>
#include <mui/Listtree_mcc.h>
#include <proto/panel.h>
#include "debug.h"

#include "../panel/paneltags.h"

#include "locale.h"
#include "panelprefs_cat.h"




#include "prefs.h"
#include "MUIClasses_Prefs.h"



//#include "name.h"
extern APTR prefs_obj;
#define CATCOMP_NUMBERS
extern const char * const __stringtable[];
#ifdef __SASC
#define GSI(x) __stringtable[x]
#else
#define GSI(x) ( STRPTR )__stringtable[x]
#endif

#warning just for locale_parser.lua
#warning MSG_PANELWIN_MODES_ATTACHED
#warning MSG_PANELWIN_PLACEMENTH_BEGINNING
#warning MSG_PANELWIN_PLACEMENTH_END
#warning MSG_PANELWIN_DEPTHS_BACK


struct Data
{
	APTR backmode, alpha, backpen;
	APTR backasl,backasl_str;
	APTR cyc_ori, cyc_pos, sl_size, cyc_depth, cyc_placement;
	APTR bt_autozip, hide_dragbar, bt_zip, dock_mode;
	APTR name;
};


static ULONG sizeslidera_w[] = {
									MV_Panelgroup_Size_Micro,
									MV_Panelgroup_Size_Small,
									MV_Panelgroup_Size_Medium,
									MV_Panelgroup_Size_Large,
									MV_Panelgroup_Size_Huge,
									0
};

static ULONG panel_size_to_slider( ULONG size )
{
	ULONG *p = sizeslidera_w;
	ULONG i = 0;

	while( *p )
	{
		if( *p == size )
		{
			break;
		}
		i++;
		p++;
	}
	return( i );
}

static ULONG panel_slider_to_size( ULONG slider )
{
	return( sizeslidera_w[ slider ] );
}

static void doset( APTR obj, struct Data *data,struct TagItem *tags )
{
	struct TagItem *tstate = tags, *tag;
	
	while ((tag = (struct TagItem *) NextTagItem(&tstate)))
	{	
		switch (tag->ti_Tag)
		{		
		
			case MA_Panelgroup_Size:
				SetAttrs(data->sl_size,MUIA_NoNotify, TRUE, MUIA_Slider_Level,panel_size_to_slider(tag->ti_Data),TAG_DONE);
				break;
			case MA_Panelgroup_Zipping:
				SetAttrs(data->bt_zip,MUIA_NoNotify, TRUE, MUIA_Selected,tag->ti_Data,TAG_DONE);
				break;
			case MA_Panelgroup_Horiz:
				SetAttrs(data->cyc_ori,MUIA_NoNotify, TRUE, MUIA_Cycle_Active,tag->ti_Data,TAG_DONE);
				break;
			case MA_Panelgroup_AutoZip:
				SetAttrs(data->bt_autozip,MUIA_NoNotify, TRUE, MUIA_Selected,tag->ti_Data,TAG_DONE);
				break;
			case MA_Panelgroup_DragMode:
				SetAttrs(data->cyc_placement,MUIA_NoNotify, TRUE, MUIA_Cycle_Active,tag->ti_Data,TAG_DONE);
				break;
			case MA_Panelgroup_BackMode:
				SetAttrs(data->backmode,MUIA_NoNotify, TRUE, MUIA_Cycle_Active,tag->ti_Data,TAG_DONE);
				if(tag->ti_Data == 	MV_Panelgroup_BackMode_Color)
				{
					SetAttrs(data->backpen,MUIA_ShowMe,TRUE,TAG_DONE); 
					SetAttrs(data->backasl,MUIA_ShowMe,FALSE,TAG_DONE);
				}
				if(tag->ti_Data == 	MV_Panelgroup_BackMode_Picture)
				{
					SetAttrs(data->backpen,MUIA_ShowMe,FALSE,TAG_DONE); 
					SetAttrs(data->backasl,MUIA_ShowMe,TRUE,TAG_DONE);
				}
				break;
			case MA_Panelgroup_BackColor:
				{
					ULONG RGB[3];
					RGB[ 0 ] = ( tag->ti_Data >> 16 ) << 24;
					RGB[ 1 ] = ( tag->ti_Data >>  8 ) << 24;
					RGB[ 2 ] = ( tag->ti_Data << 24 );
				//	PDB(("%x %x %x %x\n",tag->ti_Data,RGB[ 0 ], RGB[ 1 ], RGB[ 2 ]));
				//	PDB(("%x\n",tag->ti_Data >> 24 ));
					DoMethod( data->backpen      , MUIM_Pendisplay_SetRGB, RGB[ 0 ], RGB[ 1 ], RGB[ 2 ] );
					SetAttrs( data->alpha        , MUIA_NoNotify, TRUE,		MUIA_Slider_Level, tag->ti_Data >> 24,TAG_DONE);
				}
				break;
			case MA_Panelgroup_Backdrop:
			//	kprintf("new background %x\n",tag->ti_Data);
				SetAttrs( data->backasl_str, MUIA_NoNotify, TRUE,	MUIA_String_Contents,tag->ti_Data,TAG_DONE);
#warning no image background
				break;
			case MA_Panelgroup_HideDragBar:
			//	kprintf("dock mode %d %x\n",tag->ti_Data,data->dock_mode);
				SetAttrs(data->hide_dragbar,MUIA_NoNotify, TRUE, MUIA_Selected,tag->ti_Data,TAG_DONE);
				break;
			case MA_Panelgroup_DockMode:
			//	kprintf("dock mode %d %x\n",tag->ti_Data,data->dock_mode);
				SetAttrs(data->dock_mode,MUIA_NoNotify, TRUE, MUIA_Selected,tag->ti_Data,TAG_DONE);
				break;
			case MA_Panelwin_Name:
				SetAttrs(data->name,MUIA_NoNotify, TRUE, MUIA_String_Contents,tag->ti_Data,TAG_DONE);
				break;
			case MA_Panelwin_Position:
				SetAttrs(data->cyc_pos,MUIA_NoNotify, TRUE, MUIA_Cycle_Active,tag->ti_Data,TAG_DONE);
				break;
		}
	}
	
}


static ULONG mNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
	//struct Data *data,t;
	
	APTR name, reggroup;
	APTR backmode, alpha, backpen;
	APTR backasl,backasl_str;
	APTR cyc_ori, cyc_pos, sl_size, cyc_depth, cyc_placement;
	APTR dock_mode;
	APTR bt_autozip;
	APTR hide_dragbar, bt_zip;
	
	static STRPTR pages[ MSG_PANELWIN_BEHAVIOUR  - MSG_PANELWIN_NAME_LOOK + 2 ];
	
	static STRPTR cyc_modes     [ MSG_PANELWIN_MODES_FIXED    - MSG_PANELWIN_MODES_FLOATING  + 2 ];
	static STRPTR cyc_titles    [ MSG_PANELWIN_TITLES_HORIZONTAL - MSG_PANELWIN_TITLES_VERTICAL + 2 ];
	static STRPTR cyc_depths    [ MSG_PANELWIN_DEPTHS_FRONT      - MSG_PANELWIN_DEPTHS_NORMAL   + 2 ];
	static STRPTR cyc_placementh[ MSG_PANELWIN_PLACEMENTH_BOTH   - MSG_PANELWIN_PLACEMENTH_NONE + 2 ];
	static STRPTR cyc_backmodes [ MSG_PANELWIN_BACKMODE_IMAGE    - MSG_PANELWIN_BACKMODE_COLOR  + 2 ];
	MUIInitStringArray( (APTR) pages, MSG_PANELWIN_NAME_LOOK , MSG_PANELWIN_BEHAVIOUR  );
	if( ( obj = DoSuperNew( cl, obj,
		Child,
	reggroup = RegisterGroup( pages ),
		Child, ColGroup(2),
		Child, NSLabel1(MSG_PANELWIN_NAME),
			Child, name = MUICreateString( MSG_PANELWIN_NAME, /*PANELNAME_SIZEOF - 2*/20, GSI(MSG_PANELWIN_NAME) ),
			Child, NSLabel1(MSG_PANELWIN_BACKGROUND),
			Child, HGroup,
#warning no image backround
				Child, backmode = MUICreateCycle( MSG_PANELWIN_BACKGROUND, cyc_backmodes, MSG_PANELWIN_BACKMODE_COLOR, MSG_PANELWIN_BACKMODE_IMAGE, "PANEBACK" ),
				Child, backpen = PoppenObject, End,
				Child, backasl = PopaslObject,
							MUIA_Popstring_String, backasl_str = KeyString(/* getv( data->tbar, MA_Panelgroup_Backdrop)*/"", 60, "n" ),
							MUIA_Popstring_Button, PopButton(MUII_PopUp),
							MUIA_ShowMe,FALSE,
							End,
				End,
				Child, NSLabel1(MSG_PANELWIN_ALPHA),
				Child, alpha = MUICreateSlider( MSG_PANELWIN_ALPHA, 0, 255, 255, "PANEALPH" ),
				Child, NSLabel(MSG_PANELWIN_ORIENTATION),
				Child, cyc_ori = MUICreateCycle( MSG_PANELWIN_ORIENTATION, cyc_titles, MSG_PANELWIN_TITLES_VERTICAL, MSG_PANELWIN_TITLES_HORIZONTAL, "PANEORIE" ),
				Child, NSLabel(MSG_PANELWIN_SIZE),
				Child, sl_size = NewObject( GetClass("SliderSize"), NULL, MUIA_ControlChar, MUIGetUnderScore( MSG_PANELWIN_SIZE ), MUIA_ShortHelp, GSI(MSG_PANELWIN_SIZE_HELP), TAG_DONE ),
		//	End,
		End,
		
			Child, ColGroup(2),
				Child, NSLabel(MSG_PANELWIN_POSITIONING),
				Child, cyc_pos = MUICreateCycle( MSG_PANELWIN_POSITIONING, cyc_modes, MSG_PANELWIN_MODES_FLOATING, MSG_PANELWIN_MODES_FIXED, "PANEPOSI" ),
				Child, NSLabel(MSG_PANELWIN_DEPTH_ARRANGEMENT),
				Child, cyc_depth = MUICreateCycle( MSG_PANELWIN_DEPTH_ARRANGEMENT, cyc_depths, MSG_PANELWIN_DEPTHS_NORMAL, MSG_PANELWIN_DEPTHS_FRONT, "PANEDEPT" ),
				Child, NSLabel(MSG_PANELWIN_DRAG_GADGET),
				Child, cyc_placement = MUICreateCycle( MSG_PANELWIN_DRAG_GADGET, cyc_placementh, MSG_PANELWIN_PLACEMENTH_NONE, MSG_PANELWIN_PLACEMENTH_BOTH, "PANEDRAG" ),
				Child, HSpace(-1),
			
				Child, VGroup,
					Child, HGroup,
						Child, bt_zip  = MUICreateCheckbox( MSG_PANELWIN_ALLOW_ZIPPING, FALSE, "PANEZIPP" ),
						Child, NSLabel1(MSG_PANELWIN_ALLOW_ZIPPING),
						Child, HSpace(-1),
						Child, bt_autozip  = MUICreateCheckbox( MSG_PANELWIN_AUTO_ZIPPING, FALSE, "PANEAZIP" ),
						Child, NSLabel1(MSG_PANELWIN_AUTO_ZIPPING),
						Child, HSpace(-1),
					End,
					Child, HGroup,
						Child,  hide_dragbar = MUICreateCheckbox(MSG_PANELWIN_HIDE_DRAGBAR, FALSE, NULL ),
						Child,  NSLabel1(MSG_PANELWIN_HIDE_DRAGBAR),
					#warning dock mode
						Child,  dock_mode = CheckMark(FALSE), 
						Child, Label("Dockmode"),
			
						//Child, HSpace(-1),
					End,
				End,
			End,
		
		
		End,	
		
	TAG_MORE			,	msg->ops_AttrList,
					TAG_DONE ) ) )
	{
		struct Data *data = (struct Data*) INST_DATA( cl, obj );
		data->backmode = backmode;
		data->alpha = alpha;
		data->backpen = backpen;
		data->backasl = backasl;
		data->backasl_str = backasl_str; 
		data->cyc_ori = cyc_ori; 
		data->cyc_pos = cyc_pos; 
		data->sl_size = sl_size; 
		data->cyc_depth = cyc_depth, 
		data->cyc_placement = cyc_placement;
		data->bt_autozip = bt_autozip;  
		data->dock_mode = dock_mode;
		data->hide_dragbar = hide_dragbar; 
		data->bt_zip = bt_zip;
		data->cyc_placement = cyc_placement;
		data->name = name;
		//SetAttrs(name,MUIA_Disabled,TRUE, TAG_DONE);	
	//	SetAttrs(bt_autozip,MUIA_Disabled,TRUE, TAG_DONE);
	//	SetAttrs(cyc_depth,MUIA_Disabled,TRUE, TAG_DONE);
		//SetAttrs(cyc_pos,MUIA_Disabled,TRUE, TAG_DONE);
	}
//	return reggroup;	
	return (ULONG)obj;
}

static ULONG mSet(struct IClass *cl,Object *obj,struct opSet *msg)
{
	struct TagItem *tstate;
    struct Data *data;
	data = (struct Data*)INST_DATA(cl,obj);
	tstate = msg->ops_AttrList;
	doset( obj, data,msg->ops_AttrList); 
	return DoSuperMethodA(cl,obj,msg);
}

static ULONG mSetup(struct IClass *cl,Object *obj,struct MUIP_Setup *msg)
{
	ULONG rc;
	struct Data *data = (struct Data*)INST_DATA(cl,obj);

	if( ( rc = DoSuperMethodA(cl,obj,msg) ) && _win(obj) )
	{
		DoMethod( data->sl_size, MUIM_Notify, MUIA_Numeric_Value  ,  MUIV_EveryTime, obj       , 2, MM_Panelwin_Size, MUIV_TriggerValue );
		DoMethod( data->cyc_ori, MUIM_Notify, MUIA_Cycle_Active   , MUIV_EveryTime, prefs_obj,4, MM_PanelPrefs_SetPrefsPool,DSI_PANELGROUP_ISHORIZ,MUIV_TriggerValue,0);
		DoMethod( data->alpha, MUIM_Notify, MUIA_Slider_Level   , MUIV_EveryTime, obj   , 1, MM_Panelwin_BackPen);
		DoMethod( data->backpen , MUIM_Notify, MUIA_Pendisplay_Spec, MUIV_EveryTime, obj   , 1, MM_Panelwin_BackPen);
		DoMethod( data->cyc_placement, MUIM_Notify, MUIA_Cycle_Active   , MUIV_EveryTime, prefs_obj, 4, MM_PanelPrefs_SetPrefsPool,DSI_PANELGROUP_DRAGGADGET_PLACEMENT, MUIV_TriggerValue,0 );
		DoMethod( data->bt_zip       , MUIM_Notify, MUIA_Selected       , MUIV_EveryTime, prefs_obj, 4, MM_PanelPrefs_SetPrefsPool,DSI_PANELGROUP_ZIPPING_ENABLED , MUIV_TriggerValue,0 );

		DoMethod( data->bt_zip       , MUIM_Notify, MUIA_Selected       , MUIV_EveryTime, data->bt_autozip, 3, MUIM_Set, MUIA_Disabled, MUIV_NotTriggerValue );
	
		DoMethod( data->bt_autozip   , MUIM_Notify, MUIA_Selected       , MUIV_EveryTime, prefs_obj, 4, MM_PanelPrefs_SetPrefsPool, DSI_PANELGROUP_AUTOZIP, MUIV_TriggerValue,0 );
		DoMethod( data->hide_dragbar , MUIM_Notify, MUIA_Selected       , MUIV_EveryTime, prefs_obj, 4, MM_PanelPrefs_SetPrefsPool, DSI_PANELGROUP_HIDEDRAGBAR , MUIV_TriggerValue,0 );
	
		DoMethod( data->dock_mode	 , MUIM_Notify, MUIA_Selected       , MUIV_EveryTime, prefs_obj, 4, MM_PanelPrefs_SetPrefsPool, DSI_PANELGROUP_DOCKMODE, MUIV_TriggerValue,0 );
		DoMethod( data->cyc_pos      , MUIM_Notify, MUIA_Cycle_Active   , MUIV_EveryTime, prefs_obj, 4, MM_PanelPrefs_SetPrefsPool, DSI_PANELGROUP_POSMODE, MUIV_TriggerValue,0 );
		DoMethod( data->cyc_depth    , MUIM_Notify, MUIA_Cycle_Active   , MUIV_EveryTime, prefs_obj, 4, MM_PanelPrefs_SetPrefsPool, DSI_PANELGROUP_DEPTH , MUIV_TriggerValue,0 );
#warning	
		//	DoMethod( data->name         , MUIM_Notify, MUIA_String_Acknowledge , MUIV_EveryTime, prefs_obj, 4, MM_PanelPrefs_ObjectSetAttr, Target_Window, MA_Panelwin_Name,MUIV_TriggerValue );
		
		DoMethod( data->backmode, MUIM_Notify, MUIA_Cycle_Active  ,0 ,data->backpen,3, MUIM_Set,MUIA_ShowMe,TRUE);
		DoMethod( data->backmode, MUIM_Notify, MUIA_Cycle_Active  ,1 ,data->backpen,3, MUIM_Set,MUIA_ShowMe,FALSE);
		DoMethod( data->backmode, MUIM_Notify, MUIA_Cycle_Active  ,1 ,data->backasl,3, MUIM_Set,MUIA_ShowMe,TRUE);
		DoMethod( data->backmode, MUIM_Notify, MUIA_Cycle_Active  ,0 ,data->backasl,3, MUIM_Set,MUIA_ShowMe,FALSE);
		DoMethod( data->backmode, MUIM_Notify, MUIA_Cycle_Active  , MUIV_EveryTime, prefs_obj, 4,MM_PanelPrefs_SetPrefsPool, DSI_PANELGROUP_BACKMODE, MUIV_TriggerValue,0 );
		DoMethod( data->backasl_str  , MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, prefs_obj, 4,MM_PanelPrefs_SetPrefsPool, DSI_PANELGROUP_BACKDROP, MUIV_TriggerValue,-1 );
	
		DoMethod( data->dock_mode	 , MUIM_Notify, MUIA_Selected  , MUIV_EveryTime,data->cyc_pos, 3, MUIM_Set,MUIA_Disabled,MUIV_TriggerValue );
		DoMethod( data->dock_mode	 , MUIM_Notify, MUIA_Selected  , TRUE,data->cyc_pos, 3, MUIM_Set,MUIA_Cycle_Active,MV_Panelwin_Position_Attached );
		DoMethod( data->dock_mode	 , MUIM_Notify, MUIA_Selected  , TRUE, data->hide_dragbar,3,MUIM_Set,MUIA_Selected  , TRUE);
		DoMethod( data->dock_mode	 , MUIM_Notify, MUIA_Selected  , MUIV_EveryTime, data->hide_dragbar,3,MUIM_Set,MUIA_Disabled,MUIV_TriggerValue );
		DoMethod( data->dock_mode	 , MUIM_Notify, MUIA_Selected  , TRUE,data->cyc_placement, 3, MUIM_Set,MUIA_Cycle_Active, MV_Panelgroup_DragMode_None );
		DoMethod( data->dock_mode	 , MUIM_Notify, MUIA_Selected  , MUIV_EveryTime,data->cyc_placement, 3, MUIM_Set,MUIA_Disabled,MUIV_TriggerValue );
	
	}
	return (rc);
}

static ULONG mSize(struct IClass *cl,Object *obj,struct MP_Panelwin_Size *msg)
{
	ULONG size = panel_slider_to_size( msg->size ) ;
//	DoMethod(prefs_obj , MM_PanelPrefs_ObjectSetAttr, Target_Group,MA_Panelgroup_Size, size );
	DoMethod( prefs_obj,MM_PanelPrefs_SetPrefsPool,DSI_PANELGROUP_SIZE,size,0);

	return( 0 );

}

static ULONG mBackPen(struct IClass *cl,Object *obj,Msg msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	ULONG pen,pen2;
	//ULONG RGB[4];
	ULONG RGB;
	ULONG alpha;
	struct MUI_RenderInfo *mri;
	GetAttr(MUIA_Pendisplay_ARGB,data->backpen,&RGB);
	GetAttr(MUIA_Slider_Level,data->alpha,&alpha);
//	kprintf("backpen %x %x\n",RGB,alpha);
	DoMethod(prefs_obj , MM_PanelPrefs_SetPrefsPool,DSI_PANELGROUP_BACKCOLOR, (~alpha << 24) + RGB ,0);
	/*if(( mri = muiRenderInfo( msg->poppen ) ))
	{
		kprintf("backpen mri\n");
		pen = MUI_ObtainPen( mri, msg->penspec, 0 );
		pen2 = MUIPEN(pen);
		GetRGB32( mri->mri_Screen->ViewPort.ColorMap, pen2, 1, RGB );
		MUI_ReleasePen( mri, pen );
		RGB[ 0] = RGB[ 0 ] >> 24;
		RGB[ 1] = RGB[ 1 ] >> 24;
		RGB[ 2] = RGB[ 2 ] >> 24;
		DoMethod(prefs_obj , MM_PanelPrefs_SetPrefsPool,DSI_PANELGROUP_BACKCOLOR, (0xff000000 +( RGB[ 0 ] << 16 ) + ( RGB[ 1 ] << 8 ) + RGB[ 2 ] ),0);
	}*/
	return( 0 );
}

DISPATCHER(DefaultWindowPrefsClass)
{
 	switch (msg->MethodID)
	{
		case OM_NEW               				: return(mNew    					(cl,obj,(struct opSet *)msg));
		case MUIM_Setup							: return(mSetup						(cl,obj,(struct MUIP_Setup *)msg));
		//	case OM_DISPOSE           				: return(mDispose					(cl,obj,(Msg)msg));
		case OM_SET                 			: return(mSet           			(cl,obj,(struct opSet *)msg));
	//	case OM_GET			    				: return(mGet						(cl,obj,(struct opGet *)msg));
		case MM_Panelwin_Size 					: return(mSize						(cl,obj,(struct MP_Panelwin_Size *)msg));
		case MM_Panelwin_BackPen				: return(mBackPen					(cl,obj,(Msg)msg));
	}
	return(DoSuperMethodA(cl,obj,msg));
}
DISPATCHER_END


struct MUI_CustomClass *DefaultWindowPrefs_Class(void)
{
	return MUI_CreateCustomClass(NULL,MUIC_Group,NULL,sizeof(struct Data),DISPATCHER_REF(DefaultWindowPrefsClass));
}