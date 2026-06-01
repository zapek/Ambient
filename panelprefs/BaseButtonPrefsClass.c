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
#include "paneltags.h"

#include "locale.h"
#include "panelprefs_cat.h"

#include "MUIClasses_Prefs.h"
//#include "prefspool.h"

#include "prefs.h"


#include "name.h"


extern APTR prefs_obj;
#define CATCOMP_NUMBERS
extern const char * const __stringtable[];
#ifdef __SASC
#define GSI(x) __stringtable[x]
#else
#define GSI(x) ( STRPTR )__stringtable[x]
#endif
struct Data
{
	APTR iconasl,iconasl_str,icon_modus;
	STRPTR uri,imagepath;
	TEXT store[300];
};

struct Data data2;

static void CheckPaths(struct Data *data)
{
	APTR truncation = 0;
	BOOL custom_image = FALSE;

	if((data->uri)&&(data->imagepath))
	{
		if(name_isinfo(data->imagepath))
		{
			if((truncation = name_truncateinfo(data->imagepath)))
			{
			//	PDB(("%s\n",data->imagepath));
			}
		}
		if(strcmp(data->uri,data->imagepath))
		{
			custom_image = TRUE;
		}
		if(truncation) name_restoreinfo( data->imagepath,truncation);
	}
	if(custom_image)
	{
		SetAttrs(data->iconasl_str, MUIA_NoNotify,TRUE,MUIA_String_Contents,data->imagepath,TAG_DONE);
		SetAttrs(data->icon_modus,MUIA_Cycle_Active,1,TAG_DONE);
	}
	else
	{
		SetAttrs(data->iconasl_str, MUIA_NoNotify,TRUE,MUIA_String_Contents,"",TAG_DONE);
		SetAttrs(data->icon_modus,MUIA_Cycle_Active,0,TAG_DONE);
	}			
			//	PDB(("%s\n",data->imagepath));
	//	PDB(("\n"));
	
}

//TEXT tt[100] = "bal.info";

static void doset( APTR obj, struct Data *data,struct TagItem *tags )
{
	struct TagItem *tstate = tags, *tag;
	BOOL c_image = FALSE;
	while ((tag = (struct TagItem *) NextTagItem(&tstate)))
	{	
		switch (tag->ti_Tag)
		{		
			
			case MA_Panel_URI:
				data->uri = (STRPTR)tag->ti_Data;
				break;
			case MA_Panel_Imagepath:
				
				if(tag->ti_Data)
				{
					strcpy(data->store,(STRPTR)tag->ti_Data);
					data->imagepath = data->store;
					//kprintf("imagepath %s\n",data->imagepath);
				}
				else data->imagepath = 0;
			//	set( data->icon_modus, MUIA_Cycle_Active, ( ( ( data->imagepath ) && strlen( data->imagepath )) ? 0 : 1 ) );
				if((data->imagepath)&&(strlen(data->imagepath))) //kprintf("len %d %d\n",strlen(data->imagepath),( ( ( data->imagepath ) && strlen( data->imagepath )) ? 0 : 1 ));
				{
					ULONG uri_l;
					SetAttrs(data->iconasl_str,MUIA_NoNotify,TRUE,MUIA_String_Contents,data->imagepath,TAG_DONE);
					if((data->uri)&&((uri_l = strlen(data->uri)))&&(strncmp(data->uri,data->imagepath,uri_l)))
					{
						c_image = TRUE;
					}
				}
				set( data->icon_modus, MUIA_Cycle_Active,c_image);
				break;
			case MUIA_ShowMe:
				if(!tag->ti_Data)data->uri = data->imagepath = 0;
				//else SetAttrs(data->iconasl_str,MUIA_NoNotify,TRUE,MUIA_String_Contents,/*data->imagepath*/tt,TAG_DONE);
				//else CheckPaths(data);
				break;
		}
	}
//	CheckPaths(data);
}


static ULONG mNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
	#define BUTTONPATTERN_SIZEOF 10
	UBYTE buttonpattern[ BUTTONPATTERN_SIZEOF ] = "#?.info";
	APTR iconasl, iconasl_str, icon_modus;
	//STRPTR def_path;
	static STRPTR cyc_iconmodes[ 3 ];
	cyc_iconmodes[ 0 ] = GSI(MSG_PANELBASEBUTTON_ICON_DEFAULT);
	cyc_iconmodes[ 1 ] = GSI(MSG_PANELBASEBUTTON_ICON_CUSTOM);
	cyc_iconmodes[ 2 ] = 0;
	if( ( obj = DoSuperNew( cl, obj,
		MUIA_Group_Horiz,TRUE,
		Child,HSpace(-1),
		Child, NSLabel1(MSG_PANELBASEBUTTON_ICON),
		Child, icon_modus = CycleObject,
					MUIA_Cycle_Entries, cyc_iconmodes,
				//	MUIA_Disabled,TRUE,
					End,
		Child, iconasl = PopaslObject, MUIA_ShowMe, FALSE,
											ASLFR_InitialPattern, &buttonpattern,
											MUIA_Popstring_String, iconasl_str = KeyString( "dumm dumm"/*data->imagepath*/, 60, "n" ),
											MUIA_Popstring_Button, PopButton(MUII_PopUp),
											End,
		Child,HSpace(-1),
				TAG_MORE			,	msg->ops_AttrList,TAG_DONE)) ) 
		{
			struct Data *data = (struct Data*) INST_DATA( cl, obj );
			
			data->iconasl_str  = iconasl_str;
			data->iconasl  = iconasl;
			data->icon_modus = icon_modus;
			data->uri = data->imagepath = 0;
#warning
			//DoMethod( iconasl_str, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj    , 3, MUIM_Set, MA_Panel_Imagepath, MUIV_TriggerValue );
			DoMethod( icon_modus , MUIM_Notify, MUIA_Cycle_Active   , 0             , iconasl, 3, MUIM_Set, MUIA_ShowMe, FALSE );
			DoMethod( icon_modus , MUIM_Notify, MUIA_Cycle_Active   , 1             , iconasl, 3, MUIM_Set, MUIA_ShowMe, TRUE );
		//	DoMethod( icon_modus , MUIM_Notify, MUIA_Cycle_Active   , 1             , data->iconasl_str,3,MUIM_Set,MUIA_String_Contents,data->imagepath,TAG_DONE);
			//DoMethod( icon_modus , MUIM_Notify, MUIA_Cycle_Active   , 0             , obj    , 3, MUIM_Set, MA_Panel_Imagepath,( ( def_path)? def_path : (STRPTR)"" ) );
		//	set( icon_modus, MUIA_Cycle_Active, ( ( ( data->imagepath ) && strlen( data->imagepath )) ? 0 : 1 ) );

		}

	return( (ULONG) obj );

}



static ULONG mSetup(struct IClass *cl,Object *obj,struct MUIP_Setup *msg)
{
	ULONG rc;
	struct Data *data = (struct Data*)INST_DATA(cl,obj);

	if( ( rc = DoSuperMethodA(cl,obj,msg) ) && _win(obj) )
	{
		DoMethod( data->iconasl_str, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 2, MM_BasebuttonPrefs_ImagePath, MUIV_TriggerValue );
		DoMethod( data->icon_modus , MUIM_Notify, MUIA_Cycle_Active   ,0,    obj, 2, MM_BasebuttonPrefs_ImagePath, 0 );
	}
	return (rc);
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

static ULONG mImagePath(struct IClass *cl,Object *obj,struct MV_BasebuttonPrefs_ImagePath *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	if((msg->image) && (strlen(msg->image)))
	{
		strcpy(data->store,msg->image);
	//	kprintf("basebuttonprefs imagepath %s\n",data->store);
	}
	else
	{
		sprintf(data->store,"%s.info",data->uri);
	}
	DoMethod( prefs_obj,MM_PanelPrefs_SetPrefsPool,DSI_LISTPOOL_PANEL_IMAGEPATH,data->store,strlen(data->store)+1);
//	DoMethod(prefs_obj, MM_PanelPrefs_ObjectSetAttr,Target_Object, MA_Panel_Imagepath, data->store );
	return 0;
}

DISPATCHER(BaseButtonPrefsClass)
{
 	switch (msg->MethodID)
	{
		case OM_NEW               				: return(mNew    					(cl,obj,(struct opSet *)msg));
		case MUIM_Setup							: return(mSetup						(cl,obj,(struct MUIP_Setup *)msg));
		//	case OM_DISPOSE           				: return(mDispose					(cl,obj,(Msg)msg));
		case OM_SET                 			: return(mSet           			(cl,obj,(struct opSet *)msg));
		case MM_BasebuttonPrefs_ImagePath		: return(mImagePath					(cl,obj,(struct  MV_BasebuttonPrefs_ImagePath *)msg));
	}
	return(DoSuperMethodA(cl,obj,msg));
}
DISPATCHER_END


struct MUI_CustomClass *BaseButtonPrefs_Class(void)
{
	return MUI_CreateCustomClass(NULL,MUIC_Group,NULL,sizeof(struct Data),DISPATCHER_REF(BaseButtonPrefsClass));
}