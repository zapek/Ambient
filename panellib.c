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
 * $Id: panellib.c,v 1.9 2022/01/31 14:02:38 geit Exp $
 */

#include "ambient.h"

#if USE_PANEL_LIB 
/* public */
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#if !USE_LEGACY
#include <libraries/query.h>
#endif
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>

/* private */
#include "panellib.h"
#include "wbstart.h"
#include "mui_func.h"  
#include "copyright.h"
#include "smartreq.h"
#include "paneltags.h"
#include "gfx_bitmap.h"
#include "mui_func.h"
#include "methodstack.h"
#include "rev.h"
#include "prefspool.h"
#include "prefs.h"

extern APTR app;

ULONG check_by_pred(Object *win,Object *pred);


struct PanelLibBase
{
	struct Library lib;
	APTR object_list;
};

struct PanelLibBase *PanelLibBase;

static BPTR libexpunge(void);
static struct Library * LIB_Open(void);
static BPTR LIB_Close(void);
static BPTR LIB_Expunge(void);
static ULONG LIB_GetQueryAttr(void);

static ULONG LIB_SetPanelObjectBitMap(APTR panelobject,struct BitMap *bm,ULONG width,ULONG height);
static ULONG LIB_ObtainPanelObject(struct TagItem  *tags);
static ULONG LIB_ReleasePanelObject(APTR panelobject);
static ULONG LIB_SetPanelObjectAttrs(APTR panelobject,struct TagItem *tags);
static ULONG LIB_GetPanelAttr(ULONG attrID,APTR panelobject,ULONG *storage);

static ULONG LIB_WritePrefsStr(APTR pctx,APTR pi,ULONG ID,STRPTR value);
static ULONG LIB_WritePrefsLong(APTR pctx,APTR pi,ULONG ID,ULONG value);
static ULONG LIB_ReadPrefsStr(APTR pctx,APTR pi,ULONG ID,STRPTR *storage);
static ULONG LIB_ReadPrefsLong(APTR pctx,APTR pi,ULONG ID,ULONG *storage);

static const ULONG libfunctable[] =
{
	FUNCARRAY_BEGIN,
	FUNCARRAY_32BIT_NATIVE,
	(ULONG)&LIB_Open,
	(ULONG)&LIB_Close,
	(ULONG)&LIB_Expunge,
	(ULONG)&LIB_GetQueryAttr,
	//(ULONG)&LIB_ArexxMustDie,
	0xffffffff,
	FUNCARRAY_32BIT_SYSTEMV,
	
	(ULONG)&LIB_ObtainPanelObject,
	(ULONG)&LIB_ReleasePanelObject,
	(ULONG)&LIB_SetPanelObjectBitMap,
	(ULONG)&LIB_SetPanelObjectAttrs,	
	(ULONG)&LIB_GetPanelAttr,	
	(ULONG)&LIB_WritePrefsStr,
	(ULONG)&LIB_WritePrefsLong,
	(ULONG)&LIB_ReadPrefsStr,
	(ULONG)&LIB_ReadPrefsLong,
	0xffffffff,
	FUNCARRAY_END
};

extern struct ExecBase *SysBase;

static ULONG flush_panellib(void)
{
	ULONG retval = FALSE;
	struct Library *lib;
	
	/*
	 * Check if another alien library already exists
	 * and try to flush it.
	 */
	Forbid();
	if ( (lib = (struct Library *)FindName(&SysBase->LibList, "panel.library")) )
	{
		if (!lib->lib_OpenCnt)
		{
			RemLibrary(lib);

			if (!FindName(&SysBase->LibList, "panel.library"))
			{
				retval = TRUE;
			}
		}
	}
	else
	{
		retval = TRUE;
	}
	Permit();

	return (retval);
}


static const TEXT libname[] = "panel.library";
static const TEXT idstring[] = "panel.library 1.2 " REVDATE " © 2015-" COMPILEYEAR " Ambient Open Source Team";

#if !USE_LEGACY
static const struct TagItem querytags[] = {
	{QUERYINFOATTR_NAME, (ULONG)libname},
	{QUERYINFOATTR_DESCRIPTION, (ULONG)"Library to interact with Ambient's panel system.."},
	{QUERYINFOATTR_COPYRIGHT, (ULONG)"© 2018 by Ambient Open Source Team"},
	{QUERYINFOATTR_AUTHOR, (ULONG)"Ambient Open Source Team"},
	{QUERYINFOATTR_SUBTYPE, QUERYSUBTYPE_LIBRARY},
	{QUERYINFOATTR_CLASS, QUERYCLASS_NONE},
	{QUERYINFOATTR_SUBCLASS, QUERYSUBCLASS_NONE},
	{TAG_DONE, 0}
};
#endif

ULONG panellib_init(void)
{
	if (flush_panellib())
	{
		PanelLibBase = (struct PanelLibBase*) NewCreateLibraryTags(
			LIBTAG_FUNCTIONINIT, libfunctable,
			LIBTAG_BASESIZE, sizeof(struct PanelLibBase),
			LIBTAG_TYPE, NT_LIBRARY,
			LIBTAG_NAME, libname,
			LIBTAG_FLAGS, LIBF_CHANGED | LIBF_SUMUSED
			#if USE_LEGACY
			,
			#else 
			| LIBF_QUERYINFO,
			#endif
			LIBTAG_VERSION, 1,
			LIBTAG_REVISION, 2,
			LIBTAG_IDSTRING, idstring,
			LIBTAG_PUBLIC, TRUE,
			TAG_DONE);
		if(PanelLibBase)
		{
			PanelLibBase->object_list =  NewObject(getpanelmessengerfamilyclass(),NULL,TAG_DONE);
			PDB(("%x %x\n",PanelLibBase->object_list,app));
			if(PanelLibBase->object_list == NULL) return FALSE; 
		}
	}
	/* XXX: maybe add some debugging to know what happens.. */
	return (TRUE); /* we don't fail if some lib is already here. better than nothing */
}


void panellib_cleanup(void)
{
	if (PanelLibBase)
	{
		if (!libexpunge())
		{
			PDB(("should not happen (tm)\n")); /* XXX: actually it can but well .. */
		}
	}
}


static BPTR libexpunge(void)
{
	Forbid();
	if (PanelLibBase->lib.lib_OpenCnt)
	{
		PanelLibBase->lib.lib_Flags |= LIBF_DELEXP;
		Permit();
		return (0);
	}

	REMOVE(&PanelLibBase->lib.lib_Node);
	Permit();

	FreeMem(((UBYTE *)PanelLibBase) - PanelLibBase->lib.lib_NegSize,
	        PanelLibBase->lib.lib_NegSize + PanelLibBase->lib.lib_PosSize);

	return (TRUE);
}


static ULONG LIB_GetQueryAttr(void)
{
	#if !USE_LEGACY
	ULONG *data = (ULONG *)REG_A0;
	ULONG attr = REG_D0;

	if (data)
	{
		struct TagItem *ti;

		if ((ti = FindTagItem(attr, querytags)))
		{
			*data = ti->ti_Data;
			return (TRUE);
		}
	}
	#endif
	return (FALSE);
}


static struct Library * LIB_Open(void)
{
	PanelLibBase->lib.lib_Flags &= ~LIBF_DELEXP;
	PanelLibBase->lib.lib_OpenCnt++;

	return ((struct Library *)PanelLibBase);
}


static BPTR LIB_Expunge(void)
{
	return (0); /* do not let anything expunge us, except ourself */
}


static BPTR LIB_Close(void)
{
	if ((--PanelLibBase->lib.lib_OpenCnt) == 0)
	{
		if (PanelLibBase->lib.lib_Flags & LIBF_DELEXP)
		{
			return (0); /* same here */
		}
	}
	return (0);
}




static ULONG LIB_SetPanelObjectAttrs(APTR panelobject,struct TagItem *tags)
{
	if(panelobject)
	{
		struct TagItem *tag, *tagptr;
		tagptr = tags;
		while ((tag = NextTagItem(&tagptr)))
		{
			methodstack_push_sync_safe(panelobject,3,MUIM_Set,tag->ti_Tag,tag->ti_Data);
		}
	}

	return (0);
}

static ULONG LIB_GetPanelAttr(ULONG attrID,APTR panelobject,ULONG *storage)
{
	ULONG retval = 0;
	if(panelobject)
	{
		retval = methodstack_push_sync_safe(panelobject,3,MM_Panelbutton_GetAttr,attrID,storage);
	}
	return (retval);
}

ULONG check_by_pred(Object *win,Object *pred)
{
	Object *tbar;
	BOOL pred_found = FALSE;
	if(pred == 0) pred_found = TRUE;
	methodstack_push_sync_safe(win,3,OM_GET,MA_Panelwin_Group,(ULONG*) &tbar);
	if(tbar)
	{
		struct List *object_list;
		Object *panel_item;
		APTR object_state;
		if((GetAttr( MUIA_Group_ChildList, tbar, (ULONG*) &object_list )&&(object_list)))
		{
			object_state = object_list->lh_Head;
			while( ( panel_item = (Object*) NextObject( &object_state ))) 
			{
				if(pred_found == TRUE)
				{
					ULONG panelobject;
					panelobject = methodstack_push_sync_safe(PanelLibBase->object_list,2,MM_Panelmessengerfamily_NewObject,panel_item);
					return panelobject;
				}
				if(pred == panel_item) pred_found = TRUE;
			}
		}
	}
	return (0);
}

static ULONG LIB_ObtainPanelObject(struct TagItem  *tags)
{
	APTR win;
	ULONG type;

	struct TagItem  *typeTag,*uriTag = 0;
	struct TagItem *predTag,*winTag;
	APTR targetobject = 0;
	APTR panelobject = 0;

	APTR win_state;
	struct List *win_list;

//	struct List *child_list;
	BOOL pred_found = FALSE;
	APTR pred = NULL;
	STRPTR objectname = 0;

	if((uriTag = FindTagItem(PanelObjectURI,tags)))	objectname = (STRPTR) uriTag->ti_Data;
	predTag = FindTagItem(PanelObjectPred,tags);
	if((predTag) && (predTag->ti_Data != 0L))
	{
		methodstack_push_sync_safe( (APTR) predTag->ti_Data,3,OM_GET,MA_Panelmessenger_PanelObject,(ULONG*) &pred );
	}
	if((winTag = FindTagItem(PanelObjectWin,tags))&& (winTag->ti_Data ))
	{	
		Object *win;
		methodstack_push_sync_safe( (APTR) winTag->ti_Data,3,OM_GET,MA_Panelmessenger_PanelObject,(ULONG*) &win);
		if(win) return check_by_pred(win,pred);
	}
	if((typeTag = FindTagItem(PanelObjectType,tags)))
	{
		if( ( methodstack_push_sync_safe( app,3,OM_GET, MUIA_Application_WindowList, (ULONG*) &win_list ) )&& ( win_list) )
		{
			win_state = win_list->lh_Head;
			while(( win = NextObject( &win_state ) ) && ( targetobject == 0 ) )
			{
				if( ( methodstack_push_sync_safe(win,3,OM_GET, MA_Panelwin_Type, &type ) ) && ( type == MV_Panelwin_Type_Root ) )
				{
//					STRPTR panel_name;
					if( typeTag->ti_Data == PanelObject_Window)
					{
						if(predTag)
						{
							if((pred_found == TRUE)||(pred == NULL))
							{
								targetobject = win;
								panelobject = (APTR) methodstack_push_sync_safe(PanelLibBase->object_list,2,MM_Panelmessengerfamily_NewObject,targetobject);
							}
							else if(pred)
							{
								if(pred == win) pred_found = TRUE;
							}
							
						}
						/*if( methodstack_push_sync_safe(win,3,OM_GET, MA_Panelwin_Name,(ULONG*)&panel_name))
						{
							PDB(("panelname %s\n",panel_name));
						}*/
					}
					else
					{
						methodstack_push_sync_safe(win,4,MM_Panelwin_FindObject,PanelObject_Object,objectname,&targetobject); 
						if(targetobject)
						{
							panelobject = (APTR) methodstack_push_sync_safe(PanelLibBase->object_list,2,MM_Panelmessengerfamily_NewObject,targetobject);
						}	
					}
				}
			}
		}
	}
	if(targetobject)	methodstack_push_sync_safe(targetobject,1,OM_RETAIN);
	return (ULONG) panelobject;
}


static ULONG LIB_ReleasePanelObject(APTR panelobject)
{
	if(panelobject)
	{
		methodstack_push_sync_safe(panelobject,3,MUIM_Set,MA_Panelbutton_ExtraBitmap,NULL);
		methodstack_push_sync_safe(PanelLibBase->object_list,2,MM_Panelmessengerfamily_RemObject,panelobject);
		methodstack_push_sync_safe(panelobject,1,OM_RELEASE);
	}
	return 0;
}

static ULONG LIB_SetPanelObjectBitMap(APTR panelobject,struct BitMap *bm,ULONG width,ULONG height)
{
	if(panelobject)
	{
		APTR abm; 
		abm = gfx_bitmap_create_from_native(bm,width,height);
		methodstack_push_sync_safe(panelobject,3,MUIM_Set,MA_Panelbutton_ExtraBitmap,abm);
	}
	return 0;
}

static ULONG LIB_WritePrefsStr(APTR pctx UNUSED,APTR pi UNUSED,ULONG ID UNUSED,STRPTR value UNUSED)
{
	return 0;
}

static ULONG LIB_WritePrefsLong(APTR pctx,APTR pi,ULONG ID,ULONG value)
{
	PDB(("%x %x %x %x\n",pctx,pi,ID,value));
	setprefslong_lp(pctx,pi,DSI_PANEL_CLASSES + ID, value );
	return 0;
}

static ULONG LIB_ReadPrefsStr(APTR pctx UNUSED,APTR pi UNUSED,ULONG ID UNUSED,STRPTR *storage UNUSED)
{
	return 0;
}

static ULONG LIB_ReadPrefsLong(APTR pctx,APTR pi,ULONG ID,ULONG *storage)
{
	ULONG res UNUSED;
	ULONG *temp;
	PDB(("%x %x %x %x\n",pctx,pi,ID,storage));
	res = (ULONG) prefspool_item_get(pctx,pi, DSI_PANEL_CLASSES + ID, (APTR) &temp, NULL );
	*storage = *temp;
	PDB(("%x %x %x\n",res,*temp,*storage));
	return 0;
}

ULONG preclose_panellib(void)
{
	if (PanelLibBase && PanelLibBase->lib.lib_OpenCnt)
	{
		smartreq_request(NULL, NULL, NULL, 0, 0, "Ok", MV_Notification_Warning, "panel.library's opencount is %ld.\nClose the processes using it.", PanelLibBase->lib.lib_OpenCnt);
		return (FALSE);
	}
	return (TRUE);
}
#endif /* USE_PANEL_LIB */ 
