/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2006 Ambient Open Source Team
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * $Id: ViewWatcherClass.c,v 1.1 2026/01/25 17:39:37 kronos Exp $
 */


/* ANSI C */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


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


#include <proto/ambient.h>


#include "paneltags.h"


#include "debug.h"

#include "muifuncs.h"
#include "MUIClasses.h"

struct MUI_CustomClass *ViewWatcherClass_Class(void);



/************************************************************************/

struct Data {
	APTR   cmenu;
};

/************************************************************************/



static ULONG mLaunch(struct IClass *cl,Object *obj,struct MP_Panelbutton_Launch *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	APTR menustrip, menu, rc,item;
	ULONG nr = 0;
	ULONG count = 1;
	#define buffer_size 500
	char buffer[buffer_size];

	menustrip = MenustripObject,
						Child, menu = MenuObject,
							MUIA_Menu_Title, " ",//GSI(MSG_CMENUTITLE_PANELPOPUPVIEWS),
						End,
                     End;
	do
	{
		ULONG buffer_pos = 0;
		nr = GetViews(nr,buffer,buffer_size);
	//	PDB(("%d\n",nr));
	//	PDB(("%c %c %c\n",buffer[0],buffer[1],buffer[2]));
		while((buffer[buffer_pos] != 0) && (buffer_pos < 500))
		{
		//	PDB(("bb %s\n",&buffer[buffer_pos]));
		//	PDB(("%d %x %d\n",buffer_pos,buffer[buffer_pos],strlen(&buffer[buffer_pos])));
			
			DoMethod( menu, OM_ADDMEMBER, ( MenuitemObject, MUIA_Menuitem_CopyStrings,TRUE,MUIA_Menuitem_Title, &buffer[buffer_pos], MUIA_UserData, count++, End ) );
			buffer_pos += strlen(&buffer[buffer_pos]);
			buffer_pos += + 1;
			//PDB(("%d %x %x\n",buffer_pos,buffer[buffer_pos],buffer[buffer_pos-1]));
		}
	} 
	while (nr);

	
	rc = (APTR) DoMethod( menustrip, MUIM_Menustrip_Popup, obj, 0 , _left(obj), _bottom(obj) - 1 );
	if((item = DoMethod(menu,MUIM_FindUData,rc)))
	{
		STRPTR sel_str;
		if((GetAttr(MUIA_Menuitem_Title,item,&sel_str)&&(sel_str)))
		{
			ULONG i = 0;
			BOOL path_end = FALSE;
			//PDB(("%s\n",sel_str));
			while(0)//!path_end)
			{
				if(sel_str[i] ==  '\0') path_end = TRUE;
				else if(isspace(sel_str[i]))
				{
					sel_str[i] = '\0';
					path_end = TRUE;
				}
				i++;
			}
		//	PDB(("%s\n",sel_str));
			if(strlen(sel_str))
			{
				AmbientOpen(sel_str);
			//	WBStartTags(WBStart_Name,sel_str,TAG_DONE);
			}
		}
	}
	//PDB(("rc %x %x\n",rc,item));
	MUI_DisposeObject( menustrip );

	//if( rc != NULL )
	//	set( rc, MUIA_Window_Open, TRUE );

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
		/*case MA_Panelbutton_DefaultImagePath:
			if( ( get( obj, MA_Panel_URI, &uripath ) ) && ( uripath ) && ( strlen(uripath) ) )
			{
				snprintf( buf, sizeof( buf ), "%s.info", uripath );
				*msg->opg_Storage = (ULONG) buf;
			}
			else
			{
				*msg->opg_Storage = 0;
			}
			break;*/
		case MA_Panel_Type:
			*msg->opg_Storage = MV_Panel_Type_ViewWatcher;
			break;
		case MA_Panel_Extern_DisplayName:
		//	*msg->opg_Storage = (ULONG) GSI(MSG_PANELITEM_VIEWWATCHER);
			*msg->opg_Storage = (ULONG) "ViewWatcher";
			break;
		case MA_Panel_Extern_Version:
			*msg->opg_Storage = 1;
			break;
		case MA_Panel_Extern_Revision:
			*msg->opg_Storage = 1;
			break;
		case MA_Panel_Extern_Author:
			*msg->opg_Storage = (ULONG) "Ambient Open Source Team";
			break;
		case MA_Panel_Extern_Description:
			*msg->opg_Storage = (ULONG) "None";
			//*msg->opg_Storage = (ULONG) GSI(MSG_PANELITEM_VIEWWATCHERDESC);
			break;
		default:
			result = DoSuperMethodA(cl,obj,msg);
			break;
	}
	return( result );
}

DISPATCHER(ViewWatcherClass)
{
	switch (msg->MethodID)
	{
		case OM_GET			    			: return(mGet				(cl,obj,(struct opGet *)msg));
		case MM_Panelbutton_Launch 			: return(mLaunch			(cl,obj,(struct MP_Panelbutton_Launch *)msg));
	}
	return(DoSuperMethodA(cl,obj,msg));
}
DISPATCHER_END



struct MUI_CustomClass *ViewWatcherClass_Class(void)
{
	return MUI_CreateCustomClass(NULL,NULL,GetMUIClass("BaseButton"),sizeof(struct Data),DISPATCHER_REF(ViewWatcherClass));
}
