
/***************************************************************************/

#define	VERSION	   1
#define	REVISION   15

#define PANELNAME   "PanelPrefs"
#define DISPLAYNAME "PanelPrefs"
#define AUTHORNAME  "Stefan Kleinheinrich"
#define DESCRIPTION "Panel Prefs Class"



/***************************************************************************/

#ifdef ENABLE_GEITDEBUG
#define DEBUG
//#include "debug.h"
//#include "debug.c"
#undef REG
#undef REGARGS
#undef STDARGS
#undef FAR
#undef INLINE
#else
#undef debug
//void debug() {}
#endif

/***************************************************************************/

#include <exec/types.h>
#include <clib/alib_protos.h>
#include <graphics/rpattr.h>
#include <cybergraphx/cybergraphics.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/panel.h>
#include <proto/cybergraphics.h>
#include <mui/Listtree_mcc.h>
#include <intuition/extensions.h>

#include <stdlib.h>
#include <string.h>
//#include <classes/ambient.h>


#include <libraries/panel.h>
//#include "prefspool.h"
#include "../panel/MUIClasses.h"
#include "../panel/paneltags.h"
#include "panelitem.h"
#include "../panel/panellib.h"
#include "prefs.h"
#include "locale.h"
#include "panelprefs_cat.h"
#include "name.h"

#include "MUIClasses_Prefs.h"






#include <proto/ambient.h>
#include <libraries/ambient.h>

#include "locale.h"


/***************************************************************************/


/*
 MSG_PREFSWIN_PANEL_EFFECTS
MSG_PREFSWIN_PANEL_HIGHLIGHT
MSG_PREFSWIN_PANEL_HIGHLIGHT_HELP
MSG_PREFSWIN_PANEL_DRAGDROP_EFFECT
MSG_PREFSWIN_PANEL_DRAGDROP_EFFECT_HELP
 MSG_PREFSWIN_PANEL_SELECTED_EFFECT
 MSG_PREFSWIN_PANEL_SELECTED_EFFECT_HELP
 MSG_PREFSWIN_PANEL_EFFECT_NONE
 MSG_PREFSWIN_PANEL_EFFECT_CLONE_ICONVIEW
 MSG_PREFSWIN_PANEL_EFFECT_LASSO
 MSG_PREFSWIN_PANEL_EFFECT_BRIGHTEN
 MSG_PREFSWIN_PANEL_EFFECT_DARKEN
 MSG_PREFSWIN_PANEL_EFFECT_TINT
 MSG_PREFSWIN_PANEL_EFFECT_BLUR
 MSG_PREFSWIN_PANEL_EFFECT_GREY
 MSG_PREFSWIN_PANEL_EFFECT_NEGATIVE
 MSG_PREFSWIN_PANEL_EFFECT_NEGATIVE_FADE
 MSG_PREFSWIN_PANEL_EFFECT_TINT_FADE
 MSG_PREFSWIN_PANEL_EFFECT_DELTA
 MSG_PREFSWIN_PANEL_EFFECT_TINT_REQ
 MSG_PREFSWIN_PANEL_EFFECT_TINT_FADE_REQ
 MSG_PREFSWIN_PANEL_FORCE_GRID
 MSG_PREFSWIN_PANEL_FORCE_GRID_HELP
 MSG_PREFSWIN_PANEL_CLASS_NAME
 MSG_PREFSWIN_PANEL_CLASS_NAME_HELP
 MSG_PREFSWIN_PANEL_CLASS_VERSION
 MSG_PREFSWIN_PANEL_CLASS_VERSION_HELP
 MSG_PREFSWIN_PANEL_CLASS_AUTHOR
 MSG_PREFSWIN_PANEL_CLASS_AUTHOR_HELP
 MSG_PREFSWIN_PANEL_CLASS_DESCGROUPTITLE
 MSG_PREFSWIN_PANEL_CLASS_DESCGROUPTITLE_HELP
 MSG_PREFSWIN_PANEL_CLASS_NOPREFSAVAILABLE
*/



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
	APTR obj_prefs_group;
	Object *listtree;
	Object *defaultwindow_prefs;
	Object *basebutton_prefs;
	Object *external_prefs;
	struct MUIS_Listtree_TreeNode *active_root_node;
	ULONG active_ID;
};



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


/***************************************************************************/

/*
	You MUST use your own mcc class serial no here! If you don't have one, email
	stefan@stuntz.com to obtain one. Classes using illegal serial numbers will
	be disabled!

*/

#define PANELTAGBASE ( TAG_USER|( ( 0xFED5L << 16 ) + 0 ) )




/***************************************************************************/

#define CLASS PANELNAME ".pobj"

#define I2S(x) I2S2(x)
#define I2S2(x) #x

char VERSION_LABEL[] = "\0$VER: " CLASS I2S(VERSION) "." I2S(REVISION) " (" __AMIGADATE__ ") © " __COPYRIGHTYEAR__ " " AUTHORNAME;

#define SUPERCLASS MUIC_Group
#define _Dispatcher PanelPrefs_Dispatcher
#define UserLibID VERSION_LABEL
#define MASTERVERSION 20

#define	LIBQUERYCLASS           QUERYCLASS_AMBIENT
#define	LIBQUERYSUBCLASS        QUERYSUBCLASS_AMBIENT_PANEL
#define	LIBQUERYID              &VERSION_LABEL[7]
#define	LIBQUERYDESCRIPTION     DESCRIPTION

#define D(x)
#define bug kprintf

#define DISPATCHERNAME_BEGIN(Name)			\
ULONG Name(void) { Class *cl=(Class*) REG_A0; Msg msg=(Msg) REG_A1; Object *obj=(Object*) REG_A2; switch (msg->MethodID) {

#define DISPATCHER_END } return DoSuperMethodA(cl,obj,msg);}
#define CATCOMP_NUMBERS
extern const char * const __stringtable[];
#ifdef __SASC
#define GSI(x) __stringtable[x]
#else
#define GSI(x) ( STRPTR )__stringtable[x]
#endif

/***************************************************************************/

/*
** init functions
*/

struct Library *CyberGfxBase;
struct Library *PanelBase = 0L;
struct Library *LocaleBase;
struct Library *AmbientBase;
/*  PostClassExitFunc()
**
*/

#define PostClassExit

/***************************************************************************/

void PostClassExitFunc( void )
{
	locale_cleanup();
	if( CyberGfxBase ) {
		CloseLibrary( CyberGfxBase );
		CyberGfxBase = NULL;
	}
	if( LocaleBase ) {
		CloseLibrary( LocaleBase );
		LocaleBase = NULL;
	}
	if( PanelBase ) {
		CloseLibrary( PanelBase );
		PanelBase = NULL;
	}
	if( AmbientBase ) {
		CloseLibrary( AmbientBase );
		AmbientBase = NULL;
	}
}
/*  */
/*  PreClassInitFunc()
**
*/

#define PreClassInit
void InitClasses(void);
/***************************************************************************/

BOOL PreClassInitFunc( void )
{

	#warning
	if( ( CyberGfxBase = OpenLibrary( "cybergraphics.library", 51 ) ) && (LocaleBase = OpenLibrary("locale.library",0)) && (AmbientBase = OpenLibrary("ambient.library",0))) {
		if( ( CyberGfxBase->lib_Version > 51 ) || ( ( CyberGfxBase->lib_Version == 51 ) && ( CyberGfxBase->lib_Revision >= 31 ) ) ) {
			PanelBase = OpenLibrary("panel.library",0);
			locale_init();
			InitClasses();
			return( TRUE );
		}
	}
	
	PostClassExitFunc();
	return( FALSE );
}
/*  */

#include <mui/mccheader.c>


static BOOL CheckPrefsFile(STRPTR fname)
{
	ULONG l;
	if((!fname)||(strlen(fname) < 6)) return FALSE;
	l = strlen(fname);
	if (!strcmp(&fname[l-6],".prefs")) return TRUE;
	return FALSE;	
}


static APTR ListTreeGroup (struct Data *data)
{
	return GroupObject, MUIA_Group_Horiz,FALSE, MUIA_HorizWeight,  70,
				Child,  data->listtree = (Object*)NewObject(GetClass("PanelListtree"),NULL,MUIA_VertWeight,  60,End,
				Child,HGroup,//MUIA_Disabled,TRUE,
					Child, data->new_panel = MUICreateButton( MSG_PREFSWIN_PANEL_NEW , NULL ),
					Child, data->del_object = MUICreateButton( MSG_PREFSWIN_PANEL_DELETE , NULL ),
					End, /*HGroup*/
				//	Child,VSpace(1),
				End; /*VGroup , 1st col*/
}

static APTR ObjectGroup (struct Data *data)
{
	return data->obj_prefs_group =GroupObject, MUIA_Group_Horiz,TRUE,
							Child,  HGroup,Child,HSpace(-1),End,
							Child,	data->defaultwindow_prefs = (Object*)NewObject(GetClass("DefaultWindowPrefs"),NULL,MUIA_ShowMe,FALSE,End,
							Child,	data->basebutton_prefs = (Object*)NewObject(GetClass("BaseButtonPrefs"),NULL,MUIA_ShowMe,FALSE,End,
						//	Child,	data->external_prefs = HGroup,MUIA_ShowMe,FALSE,End,
						End; /*object group*/
}



#if 1

static APTR GlobalGroup (struct Data *data, APTR main_ppool)
{
	ULONG *as_drop,*as_delete,*as_move,*as_windowpos;
	ULONG def = 0;
	static STRPTR cyc_effects[ MSG_PREFSWIN_PANEL_EFFECT_TINT_FADE - MSG_PREFSWIN_PANEL_EFFECT_NONE + 2 ];
	if(!(GetPPoolItem(main_ppool,0, DSI_PANEL_AUTOSAVE_DROP,(APTR*)&as_drop,NULL ))) as_drop = &def;
	if(!(GetPPoolItem(main_ppool,0, DSI_PANEL_AUTOSAVE_DELETE,(APTR*)&as_delete,NULL ))) as_delete = &def;
	if(!(GetPPoolItem(main_ppool,0, DSI_PANEL_AUTOSAVE_MOVE,(APTR*)&as_move,NULL ))) as_move = &def;
	if(!(GetPPoolItem(main_ppool,0, DSI_PANEL_AUTOSAVE_WINDOWPOS,(APTR*)&as_windowpos,NULL ))) as_windowpos = &def;

	return GroupObject, MUIA_Group_Horiz,FALSE,
	#warning MUIA_Disable?
	//MUIA_Disabled,TRUE,


					//	Child,VSpace(0),
					Child,HGroup, GroupFrameT(GSI(MSG_PREFSWIN_PANEL_LAYOUT)),
						
						Child, data->grid = MUICreateCheckbox( MSG_PREFSWIN_PANEL_FORCE_GRID, /*getprefslong( DSI_PANEL_LAYOUT_GRID )*/1,NULL ),
						Child, MUICreateLabel(MSG_PREFSWIN_PANEL_FORCE_GRID, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned ),
						Child,HSpace(0),
					End,

                Child,VSpace(0),

				Child, HGroup, GroupFrameT(GSI(MSG_PREFSWIN_PANEL_AUTOSAVE) ),
	#if 1
	Child, ColGroup(2),
						Child, data->as_drop =  MUICreateCheckbox( MSG_PREFSWIN_PANEL_AUTOSAVE_DROP, /*GetGlobalPrefs( DSI_PANEL_AUTOSAVE_DROP )*/*as_drop,NULL ),
						Child, MUICreateLabel( MSG_PREFSWIN_PANEL_AUTOSAVE_DROP, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned ),
						Child, data->as_delete =  MUICreateCheckbox( MSG_PREFSWIN_PANEL_AUTOSAVE_DELETE,  /*GetGlobalPrefs(  DSI_PANEL_AUTOSAVE_DELETE )*/*as_delete, NULL ),
						Child, MUICreateLabel(MSG_PREFSWIN_PANEL_AUTOSAVE_DELETE, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned ),
						Child, data->as_move =  MUICreateCheckbox( MSG_PREFSWIN_PANEL_AUTOSAVE_MOVE,  /*GetGlobalPrefs( DSI_PANEL_AUTOSAVE_MOVE )*/*as_move, NULL ),
						Child, MUICreateLabel( MSG_PREFSWIN_PANEL_AUTOSAVE_MOVE, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned ),
						Child, data->as_windowpos =  MUICreateCheckbox( MSG_PREFSWIN_PANEL_AUTOSAVE_WINDOWPOS ,  /*GetGlobalPrefs(  DSI_PANEL_AUTOSAVE_WINDOWPOS)*/*as_windowpos, NULL ),
						Child, MUICreateLabel( MSG_PREFSWIN_PANEL_AUTOSAVE_WINDOWPOS , MUIO_Label_SingleFrame | MUIO_Label_LeftAligned ),
					End,
					#endif
					Child, HSpace(-1),//Autosave group
				End,
		
			    Child,VSpace(0),
				Child, VGroup,GroupFrameT(GSI(MSG_PREFSWIN_PANEL_EFFECTS)),
				
#if 1		
					Child, ColGroup(3),
						Child, NSLabel2(MSG_PREFSWIN_PANEL_HIGHLIGHT),
						Child, data->highlight = MUICreateCycle( MSG_PREFSWIN_PANEL_HIGHLIGHT, cyc_effects, MSG_PREFSWIN_PANEL_EFFECT_NONE, MSG_PREFSWIN_PANEL_EFFECT_TINT_FADE, 0 ),
						Child, data->highlight_grp = PageGroup,

							/* NONE */
							Child, HVSpace,
							
							/*  CLONE */
							Child, HVSpace,

							/* LASSO */
							Child, HVSpace,

							/* BRIGHTEN */
							Child, HGroup,
								Child, NSLabel2(MSG_PREFSWIN_PANEL_EFFECT_DELTA),
								Child, data->highlight_brighten = NumericbuttonObject,
									MUIA_CycleChain, 1,
							//		MUIA_Numeric_Value, getprefslong(DSI_PANEL_HIGHLIGHT_BRIGHTEN),
									MUIA_Numeric_Max, 255,
								End,
							End,
							/* DARKEN */
							Child, HGroup,
								Child, NSLabel2(MSG_PREFSWIN_PANEL_EFFECT_DELTA),
								Child, data->highlight_darken = NumericbuttonObject,
								//	MUIA_Numeric_Value, getprefslong(DSI_PANEL_HIGHLIGHT_DARKEN),
									MUIA_CycleChain, 1,
									MUIA_Numeric_Max, 255,
								End,
							End,
							/* TINT */
							Child, HGroup,
								Child, NSLabel2(MSG_PREFSWIN_ICONDISPLAY_DESKTEXTCOLOR),
								Child, data->highlight_tint = PoppenObject,
									MUIA_CycleChain, 1,
								//	  MUIA_Window_Title, GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_TINT_REQ),
								//	MUIA_Pendisplay_Spec, getprefs(DSI_PANEL_HIGHLIGHT_TINT),//GetTagData(MA_DropEffect_TintVal, (ULONG)&deftint, INITTAGS),
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
								Child, data->highlight_tintfade = PoppenObject,
									MUIA_CycleChain, 1,
								//	  MUIA_Window_Title, GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_TINT_REQ),
								//	MUIA_Pendisplay_Spec, getprefs(DSI_PANEL_HIGHLIGHT_TINTFADE),//GetTagData(MA_DropEffect_TintVal, (ULONG)&deftint, INITTAGS),
								End,
							End,
						End,
						Child, NSLabel2(MSG_PREFSWIN_PANEL_DRAGDROP_EFFECT),
						Child, data->dragdrop = MUICreateCycle( MSG_PREFSWIN_PANEL_DRAGDROP_EFFECT , cyc_effects, MSG_PREFSWIN_PANEL_EFFECT_NONE, MSG_PREFSWIN_PANEL_EFFECT_TINT_FADE, 0 ),

						Child, data->dragdrop_grp = PageGroup,

							/* NONE */
							Child, HVSpace,

							/*  CLONE */
							Child, HVSpace,


							/* LASSO */
							Child, HVSpace,

							/* BRIGHTEN */
							Child, HGroup,
								Child, NSLabel2(MSG_PREFSWIN_PANEL_EFFECT_DELTA),
								Child, data->dragdrop_brighten = NumericbuttonObject,
																MUIA_CycleChain   , 1,
															//	MUIA_Numeric_Value, getprefslong( DSI_PANEL_DRAGDROP_BRIGHTEN ),
																MUIA_Numeric_Max  , 255,
																End,
							End,
							/* DARKEN */
							Child, HGroup,
								Child, NSLabel2(MSG_PREFSWIN_PANEL_EFFECT_DELTA),
								Child, data->dragdrop_darken = NumericbuttonObject,
																MUIA_CycleChain   , 1,
															//	MUIA_Numeric_Value, getprefslong( DSI_PANEL_DRAGDROP_DARKEN ),
																MUIA_Numeric_Max  , 255,
																End,
							End,
							/* TINT */
							Child, HGroup,
								Child, NSLabel2(MSG_PREFSWIN_ICONDISPLAY_DESKTEXTCOLOR),
								Child, data->dragdrop_tint = PoppenObject,
																MUIA_CycleChain     , 1,
															//	  MUIA_Window_Title, GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_TINT_REQ),
														//		MUIA_Pendisplay_Spec, getprefs( DSI_PANEL_DRAGDROP_TINT ),// GetTagData(MA_DropEffect_TintVal, (ULONG)&deftint, INITTAGS),
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
								Child, data->dragdrop_tintfade = PoppenObject,
																MUIA_CycleChain     , 1,
															//	  MUIA_Window_Title, GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_TINT_REQ),
															//	MUIA_Pendisplay_Spec, getprefs( DSI_PANEL_DRAGDROP_TINTFADE ),// GetTagData(MA_DropEffect_TintVal, (ULONG)&deftint, INITTAGS),
																End,
							End,
						End,

						Child, NSLabel2( MSG_PREFSWIN_PANEL_SELECTED_EFFECT ),
						Child, data->selected = MUICreateCycle( MSG_PREFSWIN_PANEL_SELECTED_EFFECT , cyc_effects, MSG_PREFSWIN_PANEL_EFFECT_NONE, MSG_PREFSWIN_PANEL_EFFECT_TINT_FADE, 0 ),

						Child, data->selected_grp = PageGroup,
						
							/* NONE */
							Child, HVSpace,
							
							/*  CLONE */
							Child, HVSpace,


							/* LASSO */
							Child, HVSpace,

							/* BRIGHTEN */
							Child, HGroup,
								Child, NSLabel2(MSG_PREFSWIN_PANEL_EFFECT_DELTA),
								Child, data->selected_brighten = NumericbuttonObject,
																MUIA_CycleChain   , 1,
															//	MUIA_Numeric_Value, getprefslong( DSI_PANEL_SELECTED_BRIGHTEN ),
																MUIA_Numeric_Max  , 255,
																End,
							End,
							/* DARKEN */
							Child, HGroup,
								Child, NSLabel2(MSG_PREFSWIN_PANEL_EFFECT_DELTA),
								Child, data->selected_darken = NumericbuttonObject,
																MUIA_CycleChain   , 1,
																//MUIA_Numeric_Value, getprefslong( DSI_PANEL_SELECTED_DARKEN ),
																MUIA_Numeric_Max  , 255,
																End,
							End,
							/* TINT */
							Child, HGroup,
								Child, NSLabel2( MSG_PREFSWIN_ICONDISPLAY_DESKTEXTCOLOR ),
								Child, data->selected_tint = PoppenObject,
																MUIA_CycleChain     , 1,
															//	  MUIA_Window_Title, GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_TINT_REQ),
															//	MUIA_Pendisplay_Spec,getprefs( DSI_PANEL_SELECTED_TINT ),// GetTagData(MA_DropEffect_TintVal, (ULONG)&deftint, INITTAGS),
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
								Child, data->selected_tintfade = PoppenObject,
									MUIA_CycleChain, 1,
								//	  MUIA_Window_Title, GSI(MSG_PREFSWIN_ICONDISPLAY_SELECTION_EFFECT_TINT_REQ),
								//	MUIA_Pendisplay_Spec, getprefs( DSI_PANEL_SELECTED_TINTFADE ),// GetTagData(MA_DropEffect_TintVal, (ULONG)&deftint, INITTAGS),
								End,
							End,
						End,
					
					End,
			//			Child,HSpace(0),
					End,
					#endif
					Child, HGroup,
					Child, NSLabel2(MSG_PREFSWIN_PANEL_ZIPSPEED),
					Child, data->sl_speed = /*SliderObject, MUIA_Slider_Level,2,MUIA_Slider_Max,4,End,*/(Object*)NewObject(GetClass("SliderSpeed"),NULL, TAG_DONE ), 
				
				End,
				End;
}

#endif



/*
** methods
*/


/*************************************************************************/

APTR prefs_obj;

static ULONG mNew( struct IClass *cl, Object *obj, struct opSet *msg )
{
	struct Data t;
	APTR main_ppool;
	APTR panellib_text;
	static STRPTR pages[ MSG_PREFSWIN_PANEL_GLOBAL - MSG_PREFSWIN_PANEL_PANELS + 2 ];
	MUIInitStringArray( (APTR) pages, MSG_PREFSWIN_PANEL_PANELS, MSG_PREFSWIN_PANEL_GLOBAL );
	if(!(main_ppool = LockPPool("MAIN_PPOOL",PPOOL_TYPE_MAIN))) return 0;
	if( (  prefs_obj = obj = DoSuperNew( cl, obj,
		Child, RegisterGroup( pages ),
			Child, ColGroup(2),
				Child, ListTreeGroup(&t),
					Child,VGroup,MUIA_HorizWeight,  30,
						Child, t.lv_item =	(Object*)NewObject(GetClass("ClassList"),NULL,
																MUIA_CycleChain,           TRUE,
																MUIA_Listview_DragType,    MUIV_Listview_DragType_Immediate,
																MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_None,
																MUIA_ShortHelp, GSI(MSG_PREFSWIN_PANEL_LIST_HELP) ,
																End,
						Child,HGroup,
							Child,HSpace(-1), 
							Child,t.rescan = MUICreateButton(MSG_PREFSWIN_PANEL_RESCAN , NULL),
							Child,HSpace(-1),
							End,	
				
					End,
		Child,VGroup, MUIA_HorizWeight,  70,
					Child, panellib_text = TextObject,MUIA_Text_Contents,"no panel.library found, please start MOSSYS:Ambient/PanelApp",MUIA_ShowMe,FALSE,End,
					Child, t.object_group = ObjectGroup(&t),
					
					End,
			
					Child,VGroup,MUIA_HorizWeight,  30,//MUIA_VertWeight,  30,MUIA_HorizWeight,  130,
					#if 1
					Child, ColGroup(2), MUIA_Group_HorizCenter,2,
						
						Child, MUICreateLabel( MSG_PREFSWIN_PANEL_CLASS_VERSION, /*MUIO_Label_LeftAligned|*/MUIO_Label_SingleFrame),
						Child, t.txt_version =  MUICreateTextNoFrame( MSG_PREFSWIN_PANEL_CLASS_VERSION, "" ),
						Child, MUICreateLabel( MSG_PREFSWIN_PANEL_CLASS_AUTHOR, /*MUIO_Label_LeftAligned|*/MUIO_Label_SingleFrame),
						Child, t.txt_author = MUICreateTextNoFrame( MSG_PREFSWIN_PANEL_CLASS_AUTHOR,  NULL  ),
						Child, HSpace(0),
						Child, t.txt_author2 = MUICreateTextNoFrame( MSG_PREFSWIN_PANEL_CLASS_AUTHOR,  NULL  ),
					End,
					#endif
					#if 1
					Child, MUI_MakeObject(MUIO_BarTitle,GSI(MSG_PANELITEMWINCLASS_DESCGROUPTITLE)),
				
						Child, t.ft_descr = FloattextObject, MUIA_List_HScrollerVisibility, MUIV_List_HScrollerVisibility_Never,
												TextFrame, MUIA_ShortHelp, GSI(MSG_PREFSWIN_PANEL_CLASS_DESCGROUPTITLE_HELP ),
												End,
												#endif			
	End,
		
					
				
			End,
				#warning
				Child, GlobalGroup(&t,main_ppool),	
	
	

		End,
		
	
								TAG_MORE, msg->ops_AttrList ) ) ) {
		struct Data *data = INST_DATA(cl,obj);
		
		ULONG tnode = 0;
		ULONG *zip_speed;						
		data = (struct Data*)INST_DATA(cl,obj);
		if(!PanelBase)
		{
			SetAttrs(panellib_text,MUIA_ShowMe,TRUE,TAG_DONE);
			SetAttrs(t.object_group,MUIA_ShowMe,FALSE,TAG_DONE);
		}
		CopyMem( &t, data, sizeof( struct Data ) );
		data->active_ID = 0;
		data->external_prefs = 0;

		if((GetPPoolItem(main_ppool,0, DSI_PANEL_ZIPSPEED,(APTR*)&zip_speed,NULL )))
		{
			SetAttrs(data->sl_speed, MUIA_Slider_Level,speed_to_slider(*zip_speed),TAG_DONE );
		}
			
		SetAttrs(data->listtree,MA_PanelPrefs_ClassListObj,data->lv_item,TAG_DONE);
		DoMethod(obj,MM_PanelPrefs_ReadPrefs);
		DoMethod(data->listtree,MUIM_Notify,MUIA_Listtree_Active,MUIV_EveryTime,obj,1,MM_PanelPrefs_NewActive);
		if((tnode = DoMethod(data->listtree,MUIM_Listtree_GetEntry, MUIV_Listtree_GetEntry_ListNode_Root,   MUIV_Listtree_GetEntry_Position_Head,0)))
		{
			SetAttrs(data->listtree,MUIA_Listtree_Active,tnode,TAG_DONE);
		}
		DoMethod(data->sl_speed,MUIM_Notify,MUIA_Slider_Level,MUIV_EveryTime,obj,3,MM_PanelPrefs_SetGlobalPrefs,DSI_PANEL_ZIPSPEED,MUIV_TriggerValue); 
		DoMethod(data->as_drop,MUIM_Notify,MUIA_Selected,MUIV_EveryTime,obj,3,MM_PanelPrefs_SetGlobalPrefs,DSI_PANEL_AUTOSAVE_DROP,MUIV_TriggerValue); 
		DoMethod(data->as_delete,MUIM_Notify,MUIA_Selected,MUIV_EveryTime,obj,3,MM_PanelPrefs_SetGlobalPrefs,DSI_PANEL_AUTOSAVE_DELETE,MUIV_TriggerValue); 
		DoMethod(data->as_move,MUIM_Notify,MUIA_Selected,MUIV_EveryTime,obj,3,MM_PanelPrefs_SetGlobalPrefs,DSI_PANEL_AUTOSAVE_MOVE,MUIV_TriggerValue); 
		DoMethod(data->as_windowpos,MUIM_Notify,MUIA_Selected,MUIV_EveryTime,obj,3,MM_PanelPrefs_SetGlobalPrefs,DSI_PANEL_AUTOSAVE_WINDOWPOS,MUIV_TriggerValue); 
		DoMethod(data->lv_item,MUIM_Notify,MUIA_List_Active,MUIV_EveryTime,obj,2,MM_PanelPrefs_ItemListChange,MUIV_TriggerValue );
		DoMethod(data->new_panel,MUIM_Notify,MUIA_Pressed,FALSE,obj,1,MM_PanelPrefs_NewPanel);
		DoMethod(data->del_object,MUIM_Notify,MUIA_Pressed,FALSE,obj,1,MM_PanelPrefs_DeleteObject);
		DoMethod(data->rescan,MUIM_Notify,MUIA_Pressed,FALSE,obj,1,MM_PanelPrefs_RescanClasses);
		
			set( t.highlight_grp , MUIA_Group_ActivePage, getv( t.highlight, MUIA_Cycle_Active ) );
	DoMethod( t.highlight, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, t.highlight_grp, 3, MUIM_Set, MUIA_Group_ActivePage, MUIV_TriggerValue );
	set( t.dragdrop_grp  , MUIA_Group_ActivePage, getv( t.dragdrop, MUIA_Cycle_Active ) );
	DoMethod( t.dragdrop , MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, t.dragdrop_grp, 3, MUIM_Set, MUIA_Group_ActivePage, MUIV_TriggerValue );
	set( t.selected_grp  , MUIA_Group_ActivePage, getv( t.selected, MUIA_Cycle_Active ) );
	DoMethod( t.selected , MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, t.selected_grp, 3, MUIM_Set, MUIA_Group_ActivePage, MUIV_TriggerValue );
	UnLockPPool(main_ppool);
	main_ppool = 0;
	}
	return( (ULONG) obj );
}

static ULONG mDispose( struct IClass *cl, Object *obj, Msg msg )
{
	ULONG retval;
	if(PanelBase)
	{
		CloseLibrary(PanelBase);
		PanelBase = 0;	
	}
	retval = DoSuperMethodA(cl, obj, (Msg)msg);
	return retval;
}

static ULONG mSetup(struct IClass *cl,Object *obj,struct MUIP_Setup *msg)
{
	ULONG retval= DoSuperMethodA(cl, obj, (Msg)msg);
#warning add version and revision check
	if(!PanelBase) PanelBase = OpenLibrary("panel.library",0);
	if(PanelBase) PanelPrefsApp(_app(obj),obj);
	return retval;
}

static ULONG mCleanup(struct IClass *cl,Object *obj,struct MUIP_Cleanup *msg)
{
	ULONG retval;
	//PanelBase = 0;
	if(PanelBase) PanelPrefsApp(0,0);
	retval = DoSuperMethodA(cl, obj, (Msg)msg);
	return retval;
}

static ULONG mReadPrefs(struct IClass *cl,Object *obj,Msg msg)
{
	struct Data *data;
	STRPTR panel_name = 0;
	ULONG i = 0;
	data = (struct Data*)INST_DATA(cl,obj);
	while((panel_name = NextPPoolName(panel_name,PPOOL_TYPE_PANEL)))
	{
		i++;
		DoMethod(data->listtree,MM_PanelListtree_AddPanel,panel_name);
		if(i>5) return 0;
	}
	return 0;
}



static ULONG mSetPrefsPool(struct IClass *cl,Object *obj,struct MP_PanelPrefs_SetPrefsPool *msg)
{
    struct Data *data = (struct Data*)INST_DATA(cl,obj);
	APTR ppool;	
	
	if((ppool = LockPPool(data->active_root_node->tn_Name,PPOOL_TYPE_PANEL)))
	{
		if(data->active_ID == 0)
		{
			if(msg->size == 0)
			{
				AddPPoolItem(ppool,0, msg->prefs_ID,&msg->data, 4);
			}
		}
		else
		{
			APTR pl,pi;
			if((pl = (APTR) GetPPoolItem(ppool, NULL, DSI_LISTPOOL_PANEL, NULL, (ULONG)NULL ))
			 &&(pi = (APTR) GetPPoolItem(ppool, pl, data->active_ID | DSF_LISTPOOL, NULL, NULL)))
			{
				AddPPoolItem(ppool, pi,msg->prefs_ID, msg->data , msg->size);
			}
		}
		UnLockPPool(ppool);
		if(PanelBase)
		{
			PrefsPoolUpdate(data->active_root_node->tn_Name,data->active_ID,msg->prefs_ID);
		}
	}

#warning MIA
#if 0
	if((panel_win = LockPanel(data->active_root_node->tn_Name)))
	{
		if((pobj = ObtainPanelObject(panel_win,ti)))
		{	
			SetPanelObjectAttr(pobj,msg->tag, msg->data);
			ReleasePanelObject(pobj);
			}
		UnLockPanel(panel_win);	
	}	
#endif
	return 0;
}
static ULONG mSetGlobalPrefs(struct IClass *cl,Object *obj,struct MP_PanelPrefs_SetGlobalPrefs *msg)
{
	#warning
	ULONG value = msg->value;
	APTR main_ppool;
	if(!(main_ppool = LockPPool("MAIN_PPOOL",PPOOL_TYPE_MAIN))) return 0;
	AddPPoolItem(main_ppool,NULL, msg->ID, msg->value , 4);
	UnLockPPool(main_ppool);
//	SetGlobalPrefs(msg->ID, value);
	return 0;
}



static ULONG mNewActive(struct IClass *cl,Object *obj,Msg msg)
{
	struct Data *data;
	struct MUIS_Listtree_TreeNode *treenode,*tn2;
	//struct List *button_list;
	//APTR object_state;
	APTR ppool;

	data = (struct Data*)INST_DATA(cl,obj);
	treenode = (struct MUIS_Listtree_TreeNode *)DoMethod(data->listtree,MUIM_Listtree_GetEntry,0, MUIV_Listtree_GetEntry_Position_Active,0);
	if(data->external_prefs) 
	{
		DoMethod(data->obj_prefs_group,MUIM_Group_Remove,data->external_prefs);
		data->external_prefs = 0;
	}
	if((treenode)&&(treenode->tn_Name))
	{
		tn2 = treenode;
		while(tn2)
		{
			data->active_root_node = tn2;
			tn2 = (struct MUIS_Listtree_TreeNode *)DoMethod(data->listtree,MUIM_Listtree_GetEntry,tn2, MUIV_Listtree_GetEntry_Position_Parent,0);
		}	
		DoMethod(data->object_group,MUIM_Group_InitChange);
		FORCHILD( data->object_group, MUIA_Group_ChildList )
		{
			SetAttrs( child, MUIA_ShowMe, FALSE, TAG_DONE );
		}
		NEXTCHILD
		if((treenode->tn_Flags & TNF_LIST)&&(treenode == data->active_root_node))
		{
			
			data->active_ID = 0;
			if((ppool = LockPPool(treenode->tn_Name,PPOOL_TYPE_PANEL)))
			{			
				ULONG *pos,*size,*zip,*horiz;
				ULONG *auto_zip,*drag,*backmode,*backcolor;
				ULONG *hidedrag,*dock_mode;
				STRPTR backdrop;
				PanelMode(ppool,PANEL_MODE_PREFS);
				GetPPoolItem(ppool,0, DSI_PANELGROUP_POSMODE,(APTR*)&pos,NULL );
				GetPPoolItem(ppool,0, DSI_PANELGROUP_SIZE,(APTR*)&size,NULL );
				GetPPoolItem(ppool,0, DSI_PANELGROUP_ZIPPING_ENABLED,(APTR*)&zip,NULL );
				GetPPoolItem(ppool,0, DSI_PANELGROUP_ISHORIZ,(APTR*)&horiz,NULL );
				GetPPoolItem(ppool,0, DSI_PANELGROUP_AUTOZIP,(APTR*)&auto_zip,NULL );
				GetPPoolItem(ppool,0, DSI_PANELGROUP_DRAGGADGET_PLACEMENT,(APTR*)&drag,NULL );
				GetPPoolItem(ppool,0, DSI_PANELGROUP_BACKMODE,(APTR*)&backmode,NULL );
				GetPPoolItem(ppool,0, DSI_PANELGROUP_BACKCOLOR,(APTR*)&backcolor,NULL );
				GetPPoolItem(ppool,0, DSI_PANELGROUP_BACKDROP,(APTR*)&backdrop,NULL);
				GetPPoolItem(ppool,0, DSI_PANELGROUP_HIDEDRAGBAR,(APTR*)&hidedrag,NULL);
				GetPPoolItem(ppool,0, DSI_PANELGROUP_DOCKMODE,(APTR*)&dock_mode,NULL);
				{
				struct TagItem ti[] = {
				{MA_Panelwin_Name,(ULONG)treenode->tn_Name},
				{MA_Panelwin_Position,*pos},
				{MA_Panelgroup_Size, *size},
				{MA_Panelgroup_Zipping,*zip}, 
				{MA_Panelgroup_Horiz, *horiz},
				{MA_Panelgroup_AutoZip,*auto_zip}, 
				{MA_Panelgroup_DragMode,*drag}, 
				{MA_Panelgroup_BackMode, *backmode},
				{MA_Panelgroup_BackColor,*backcolor },
				{MA_Panelgroup_Backdrop,(ULONG)backdrop},
				{MA_Panelgroup_HideDragBar,*hidedrag},
				{MA_Panelgroup_DockMode,*dock_mode},
				{TAG_DONE,0}
			};
		
				SetAttrsA(data->defaultwindow_prefs,ti);
				SetAttrs( data->defaultwindow_prefs, MUIA_ShowMe, TRUE, TAG_DONE );
		}
				UnLockPPool(ppool);
			}
		}
		else
		{
			STRPTR imagepath,paneluri;
			APTR pl, pi = NULL;
			data->active_ID = (ULONG)treenode->tn_User;
			if((ppool = LockPPool(data->active_root_node->tn_Name,PPOOL_TYPE_PANEL)))
			{
				if((pl = (APTR)GetPPoolItem(ppool,0,  DSI_LISTPOOL_PANEL ,NULL,NULL)))
				{
					if((pi = (APTR)GetPPoolItem(ppool, pl, data->active_ID  | DSF_LISTPOOL, NULL, NULL)))
					{
						ULONG *type;
						GetPPoolItem(ppool,pi, DSI_LISTPOOL_PANEL_TYPE, (APTR)&type, 0 );
						if(*type == MV_Panel_Type_External)
						{
							STRPTR class_name;
							struct PanelItem *pitem;
							GetPPoolItem(ppool, pi, DSI_LISTPOOL_PANEL_EXT_NAME, (APTR) &class_name , NULL );
							if(DoMethod(data->lv_item,MM_ClassListGetExternalObject,class_name,&pitem))
							{
								if (pitem->pi_Object)
								{
									//APTR prefs_panel;
									#warning
						//			data->external_prefs = DoMethod(pitem->pi_Object,MM_AmbientPanel_BuildSettingsPanel);
							
								
									DoMethod(data->obj_prefs_group,MUIM_Group_AddHead,data->external_prefs);
									SetAttrs(data->external_prefs,MA_PanelPrefs_Object,prefs_obj,TAG_DONE);
								//		DoMethod(prefs_panel,MM_PanelPrefs_ExternPrefsInit,panel_lock, pi);
									SetAttrs(data->external_prefs,MUIA_ShowMe,TRUE,TAG_DONE);		
									DoMethod(data->external_prefs,MM_PanelPrefs_ExternPrefsInit,ppool, pi);
															
								}
							}				
						}
						else
						{
							#warning
							if(!GetPPoolItem(ppool, pi, DSI_LISTPOOL_PANEL_IMAGEPATH,(APTR) &imagepath , NULL )) imagepath = 0;
							if(!GetPPoolItem(ppool, pi, DSI_LISTPOOL_PANEL_URI, (APTR) &paneluri , NULL )) paneluri = 0; 
							SetAttrs( data->basebutton_prefs,MA_Panel_URI,paneluri,TAG_DONE);
							SetAttrs( data->basebutton_prefs,MA_Panel_Imagepath,imagepath,TAG_DONE);
							SetAttrs( data->basebutton_prefs, MUIA_ShowMe, TRUE, TAG_DONE );
						}		
					}
				}
				UnLockPPool(ppool);
			}
		}
		DoMethod(data->object_group,MUIM_Group_ExitChange);
	}
	return 0;
}



static ULONG mItemListChange(struct IClass *cl,Object *obj,struct MP_PanelPrefs_ItemListChange *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
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
	set( data->txt_author,  MUIA_Text_Contents,  aut1[0]                      ? (STRPTR)&aut1[0]           : (STRPTR) "llll"/* GSI(MSG_PANELITEMWINCLASS_UNKNOWN)*/ );
	set( data->txt_author2, MUIA_Text_Contents,  aut2 );
	set( data->ft_descr,    MUIA_Floattext_Text, ( pi && pi->pi_Description ) ? pi->pi_Description : (STRPTR)"kkkkk" /*GSI(MSG_PANELITEMWINCLASS_NONE)*/ );
	if( pi ) {
		DoMethod( data->txt_version, MUIM_SetAsString, MUIA_Text_Contents, "%lu.%lu\n", pi->pi_Version, pi->pi_Revision );
	} else {
		set( data->txt_version, MUIA_Text_Contents, "" );
	}
	return 0;	
}

#if 1
static ULONG mPanelMessage(struct IClass *cl,Object *obj,struct MP_PanelPrefs_PanelMessage *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	struct PanelMessage *pm = (struct PanelMessage *) msg->pm;
#warning PanelPrefsClose

	//	if(pm->pm_Mode ==  PanelPrefsClose)
	{
		/* close prefswin to make sure PanelApp can exit safely */
		DoMethod(_win(obj),MM_Prefswin_Main_Close,MV_Prefswin_Main_Close_Cancel);	
		if(PanelBase)
		{
			PanelPrefsApp(0,0);
			CloseLibrary(PanelBase);
			PanelBase = 0L;
		}
	}
	return 0;
}

#endif
static ULONG mNewPanel(struct IClass *cl,Object *obj,Msg msg)
{
	struct Data *data;
	data = (struct Data*)INST_DATA(cl,obj);
	if( PanelBase ) NewPanel(0);
	return 0;
}

static ULONG mDeleteObject(struct IClass *cl,Object *obj,Msg msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	struct MUIS_Listtree_TreeNode *treenode,*tn2;
	APTR ppool;
	APTR panel_win;
	treenode = (struct MUIS_Listtree_TreeNode *)DoMethod(data->listtree,MUIM_Listtree_GetEntry,0, MUIV_Listtree_GetEntry_Position_Active,0);
	if((treenode)&&(treenode->tn_Name))
	{
		tn2 = treenode;
		while(tn2)
		{
			data->active_root_node = tn2;
			tn2 = (struct MUIS_Listtree_TreeNode *)DoMethod(data->listtree,MUIM_Listtree_GetEntry,tn2, MUIV_Listtree_GetEntry_Position_Parent,0);
		}
	/*	if((panel_win = LockPanel(data->active_root_node->tn_Name)))
		{
		
			if((treenode->tn_Flags & TNF_LIST)&&(treenode == data->active_root_node))
			{}
			else
			{
				APTR pobj;
				struct TagItem  ti[] = 
				{
					PanelObjectID, data->active_ID,
					TAG_DONE
				};

				if((pobj = ObtainPanelObject(panel_win,ti)))
				{
					RemovePanelObject(pobj);
					#warning ReleasePanelObject?
					MUI_DisposeObject(pobj);
				}
			}
			UnLockPanel(panel_win);
		}*/
		
		#if 1
		if((ppool = (APTR)LockPPool(data->active_root_node->tn_Name,PPOOL_TYPE_PANEL)))
		{
			APTR pl,pi,d =0;
			ULONG i = data->active_ID;// | DSF_LISTPOOL;
			ULONG size = 0;
			if((pl = (APTR)GetPPoolItem(ppool,0,  DSI_LISTPOOL_PANEL ,NULL,&size)))
			{
				//pi = (APTR)GetPPoolItem(ppool, pl,  i | DSF_LISTPOOL, &d, &size);
				//	kprintf("size %d %x\n",size,d);
				RemovePPoolItem(ppool,pl, i| DSF_LISTPOOL);// | DSF_LISTPOOL);		
				while((pi = (APTR)GetPPoolItem(ppool, pl,  (++i)| DSF_LISTPOOL , &d, &size)))
				{
					ChangePPoolItemID(ppool,pl, i| DSF_LISTPOOL,(i-1)| DSF_LISTPOOL);
				}
				SetAttrs(data->listtree,MUIA_Listtree_Quiet,TRUE,TAG_DONE);
				DoMethod(data->listtree,MUIM_Listtree_Remove, MUIV_Listtree_Remove_ListNode_Active , MUIV_Listtree_Remove_TreeNode_Active,0);
				SetAttrs(data->listtree,MUIA_Listtree_Quiet,FALSE,TAG_DONE);
			}
		/*	if((treenode->tn_Flags & TNF_LIST)&&(treenode == data->active_root_node))
			{
				//kprintf("delete panel\n");
				DeletePanelObject(panel_lock,0);
				SetAttrs(data->listtree,MUIA_Listtree_Quiet,TRUE,TAG_DONE);
				DoMethod(data->listtree,MUIM_Listtree_Remove, MUIV_Listtree_Remove_ListNode_Root ,treenode,0);
				SetAttrs(data->listtree,MUIA_Listtree_Quiet,FALSE,TAG_DONE);
			}
			else
			{
			//	kprintf("delete panel obj\n");
				DeletePanelObject(panel_lock,(ULONG)treenode->tn_User);
			}*/
			UnLockPPool(ppool);
		}
		#endif
	}
	return 0;
}

static ULONG mRescanClasses(struct IClass *cl,Object *obj,Msg msg)
{
	struct Data *data;
	data = (struct Data*)INST_DATA(cl,obj);
	return 0;
}



static ULONG mMain_Close(struct IClass *cl,Object *obj,struct MP_Prefswin_Main_Close *msg)
{
//	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	//if(PanelBase)
	#warning no_use_save_cancel
	//return 0;
	switch( msg->mode )
	{
		case MV_Prefswin_Main_Close_Test:
			//DoMethod( obj, MM_Prefswin_Store );
			//if(PanelBase)ReLoadPanels();
			break;
		case MV_Prefswin_Main_Close_Save:
#warning no panellib
			if(PanelBase)SaveAll();
			break;
		case MV_Prefswin_Main_Close_Cancel:
			#warning no panellib
			if(PanelBase)ReLoadPanels();
			break;
	}
	#warning
	//tf("quit prefs\n");
	//DoMethod(_app(obj),MUIM_Application_ReturnID,MUIV_Application_ReturnID_Quit);
	return 0;
}


/*************************************************************************/

DISPATCHERNAME_BEGIN( PanelPrefs_Dispatcher )
		case OM_NEW								: return(mNew						( cl, obj, (APTR) msg ) );
#if 1
		case OM_DISPOSE							: return(mDispose					( cl, obj, (APTR) msg ) );
		case MUIM_Setup							: return(mSetup						( cl, obj, (struct MUIP_Setup *)msg));
		case MUIM_Cleanup						: return(mCleanup					( cl, obj, (struct MUIP_Cleanup *)msg));
		case MM_PanelPrefs_SetPrefsPool			: return(mSetPrefsPool	 			(cl,obj,(struct MP_PanelPrefs_SetPrefsPool *)msg));
		case MM_PanelPrefs_SetGlobalPrefs		: return(mSetGlobalPrefs    		(cl,obj,(struct MP_PanelPrefs_SetGlobalPrefs *)msg));
		case MM_PanelPrefs_ReadPrefs			: return(mReadPrefs					(cl,obj,(Msg)msg));
		case MM_PanelPrefs_NewActive			: return(mNewActive					(cl,obj,(Msg)msg));
		case MM_PanelPrefs_ItemListChange		: return(mItemListChange			(cl,obj,(struct MP_PanelPrefs_ItemListChange *)msg));
#warning
		case MM_PanelPrefs_PanelMessage			: return(mPanelMessage				(cl,obj,(struct MP_PanelPrefs_PanelMessage *)msg));
		case MM_PanelPrefs_NewPanel				: return(mNewPanel					(cl,obj,(Msg)msg));
		case MM_PanelPrefs_DeleteObject			: return(mDeleteObject				(cl,obj,(Msg)msg));
		case MM_PanelPrefs_RescanClasses		: return(mRescanClasses				(cl,obj,(Msg)msg));
		case MM_Prefswin_Main_Close				: return(mMain_Close				(cl,obj,(struct MP_Prefswin_Main_Close *)msg));
#endif
DISPATCHER_END
/*  */
