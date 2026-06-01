/* ANSI C */
#include <stdlib.h>
#include <string.h>

/* System */
#include <dos/dos.h>
#include <graphics/gfxmacros.h>
#include <workbench/workbench.h>
#include <libraries/mui.h>
#include <libraries/wbstart.h>

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
#include <proto/wbstart.h>
#include <datatypes/pictureclass.h>
#include <cybergraphx/cybergraphics.h>
#include <clib/datatypes_protos.h>
#include <workbench/startup.h>
#include <proto/ambient.h>
#include <libraries/ambient.h>


#include "paneltags.h"







#include "debug.h"

#include "muifuncs.h"
#include "MUIClasses.h"
#include "../classes.h"

struct MUI_CustomClass *CommandButtonClass_Class(void);

struct Data
{
	STRPTR help;
};


static ULONG mLaunch(struct IClass *cl,Object *obj,struct MP_Panelbutton_Launch *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	STRPTR uripath;
	TEXT buf[ PATH_SIZE + 256 ]; /* should be enough (tm) */
	ULONG win_x,win_y,win_xs,win_ys,viewmode;
	CONST_STRPTR mode = "";
	CONST_STRPTR view = "ICON";
	ULONG r;
	if(!DoSuperMethodA(cl,obj,msg)) 
	{
		if( get( obj, MA_Panel_URI, &uripath ) )
		{
			if( ( get( obj, MA_Icon_WindowLeft, &win_x) )
			 && ( get( obj, MA_Icon_WindowTop, &win_y) )
			 && ( get( obj, MA_Icon_WindowWidth, &win_xs) ) 
			 && ( get( obj, MA_Icon_WindowHeight, &win_ys) ) 
			 && ( get( obj, MA_Icon_ViewMode, &viewmode) ))
		//	 && (viewmode != MV_Icon_ViewMode_None  ) )
			{
				#warning viewmode
				mode = "ICON";
			//	PDB(("!!!%d %d %d %d %s\n",win_x,win_y,win_xs,win_ys,uripath)); 
				snprintf(buf, sizeof(buf), "LoadURI file:///%s?left=%ld&top=%ld&width=%ld&height=%ld&mode=%s&view=%s TOFRONT",
						uripath,
						win_x,
						win_y,
						win_xs,
						win_ys,
						mode,
						view);
			}
			else
			{
				snprintf( buf, sizeof( buf ), "LoadURI \"%s\" TOFRONT", uripath );
			}
				

			ExecuteURI(buf);
		}
	}
	return( 0 );
}


/************************************************************************/

static ULONG mGet(struct IClass *cl,Object *obj,struct opGet *msg)
{
	ULONG result = TRUE;
	STRPTR uripath;
	static TEXT buf[ PATH_SIZE + 256 ]; /* should be enough (tm) */
	
	switch( msg->opg_AttrID )
	{
		case MA_Panelbutton_DefaultImagePath:
			if( ( get( obj, MA_Panel_URI, &uripath ) ) && ( uripath ) && ( strlen(uripath) ) )
			{
				snprintf( buf, sizeof( buf ), "%s.info", uripath );
				*msg->opg_Storage = (ULONG) buf;
			}
			else
			{
				*msg->opg_Storage = 0;
			}
			break;
		case MA_Panel_Type:
			*msg->opg_Storage = MV_Panel_Type_Button;
			break;
		case MA_Panel_Extern_DisplayName:
			*msg->opg_Storage = (ULONG) "CommandButton";
			break;
		case MA_Panel_Extern_Revision:
		case MA_Panel_Extern_Version:
			*msg->opg_Storage = 1;
			break;
		case MA_Panel_Extern_Author:
			*msg->opg_Storage = (ULONG) "Ambient Open Source Team";
			break;
		case MA_Panel_Extern_Description:
			*msg->opg_Storage = (ULONG) "None";
			break;
		default:
			result = DoSuperMethodA(cl,obj,msg);
			break;
	}
	return( result );
}
   
/************************************************************************/


DISPATCHER(CommandButtonClass)
{
	switch (msg->MethodID)
	{
		case OM_GET			    			: return(mGet				(cl,obj,(struct opGet *)msg));
		case MM_Panelbutton_Launch 			: return(mLaunch			(cl,obj,(struct MP_Panelbutton_Launch *)msg));
	}
	return(DoSuperMethodA(cl,obj,msg));
}
DISPATCHER_END



struct MUI_CustomClass *CommandButtonClass_Class(void)
{
	return MUI_CreateCustomClass(NULL,NULL,GetMUIClass("BaseButton"),sizeof(struct Data),DISPATCHER_REF(CommandButtonClass));
}
