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
 * $Id: BookmarksClass.c,v 1.3 2026/03/16 17:53:49 kronos Exp $
 */



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


#include <proto/ambient.h>


#include "paneltags.h"


#include "debug.h"

#include "muifuncs.h"
#include "MUIClasses.h"

struct MUI_CustomClass *BookmarksClass_Class(void);



/************************************************************************/

struct Data {
	APTR   cmenu;
};



enum
{
	STORAGE_STRING,
	STORAGE_STRARRAY,

	/* these types won't be saved on disk */
	
	STORAGE_MSTRING,
	STORAGE_MSTRARRAY
};

#define STORAGE_BOOKMARKS                   MAKE_ID('0', '0', '0', '7')

static ULONG mLaunch(struct IClass *cl,Object *obj,struct MP_Panelbutton_Launch *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	ULONG nr = 0;
	ULONG count = 1;
	STRPTR *entries;

	#define buffer_size 500
	char buffer[buffer_size];

//	nr = GetBookmarks(nr,buffer,buffer_size);
	
	StorageGet(STORAGE_BOOKMARKS, STORAGE_STRARRAY, (APTR *)&entries);
	if (entries)
	{
		LONG i;
		APTR menustrip, menu;
		APTR mu;
		
		
		menu = MenuObject,
					MUIA_Menu_Title, "BookMarks",//GSI(MSG_BOOKMARKS_MENUTITLE),
					
				End;
#if 1
//	if (menustrip == NULL)
//		return NULL;


		for (i = 0; entries[ i ] != NULL; i+=2)
		{
			APTR mi;
			mi = MenuitemObject,MUIA_Menuitem_Title,entries[ i ],	MUIA_Menuitem_CopyStrings, TRUE,
				MUIA_UserData,entries[ i ],End;
			if( mi ) 
			{
				DoMethod(menu, MUIM_Family_AddTail, mi);
			//	PDB(("%d %s %s\n",i,entries[ i ],entries[ i + 1]));
			
			}
		
		}
		menustrip = MenustripObject, Child, menu, End;
		if( ( mu = (APTR) DoMethod( menustrip, MUIM_Menustrip_Popup, obj, 0 , _left(obj), _bottom(obj) - 1 ) ) )
		{
			ULONG r;
			AmbientOpen(mu);
			//r = WBStartTags(WBStart_Name,mu,TAG_DONE);
		//	kprintf("menu %s %x\n",mu,r);
		}
	#endif
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
		#warning default image?
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
			*msg->opg_Storage = MV_Panel_Type_Bookmarks;
			break;
		case MA_Panel_Extern_DisplayName:
			#warning add locale
		//	*msg->opg_Storage = (ULONG) GSI(MSG_PANELITEM_BOOKMARKS);
			*msg->opg_Storage = (ULONG) "Bookmarks";
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
			//*msg->opg_Storage = (ULONG) GSI(MSG_PANELITEM_BOOKMARKSDESC);
			break;
		default:
			result = DoSuperMethodA(cl,obj,msg);
			break;
	}
	return( result );
}

DISPATCHER(BookmarksClass)
{
	switch (msg->MethodID)
	{
		case OM_GET			    			: return(mGet				(cl,obj,(struct opGet *)msg));
		case MM_Panelbutton_Launch 			: return(mLaunch			(cl,obj,(struct MP_Panelbutton_Launch *)msg));
	}
	return(DoSuperMethodA(cl,obj,msg));
}
DISPATCHER_END



struct MUI_CustomClass *BookmarksClass_Class(void)
{
	return MUI_CreateCustomClass(NULL,NULL,GetMUIClass("BaseButton"),sizeof(struct Data),DISPATCHER_REF(BookmarksClass));
}

