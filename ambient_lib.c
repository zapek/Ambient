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
 */

#include "ambient.h"

#if USE_AMBIENT_LIB 
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
#include "ambient_lib.h"
#include "wbstart.h"
#include "mui_func.h"  
#include "copyright.h"
#include "smartreq.h"
#include "gfx_bitmap.h"
#include "mui_func.h"
#include "methodstack.h"
#include "rev.h"
#include "prefspool.h"
#include "prefs.h"
#include "name.h"
#include "storage.h"
#include "prefswin.h"


#include "rexx.h"
#include "command.h"

#include "panelprefs.h"

#ifndef AMBIENT_LIB_H


#define PPOOL_TYPE_MAIN		1
#define PPOOL_TYPE_PANEL	2

#endif

ULONG panel_modus = 1;

extern APTR app;

ULONG check_by_pred(Object *win,Object *pred);

static APTR lib_pool;
static struct List ppool_list;

struct AmbientLibBase
{
	struct Library lib;
	APTR object_list;
};


struct PPoolNode
{
	struct Node n;
	struct SignalSemaphore semaphore;
	APTR prefspool;
	ULONG type;
	ULONG flags;
};


struct AmbientLibBase *AmbientLibBase;

static BPTR libexpunge(void);
static struct Library * LIB_Open(void);
static BPTR LIB_Close(void);
static BPTR LIB_Expunge(void);
static ULONG LIB_GetQueryAttr(void);

static ULONG LIB_AmbientOpen(STRPTR path);
static ULONG LIB_GetViews(ULONG nr,STRPTR buffer,ULONG buffer_size);
static ULONG LIB_StorageGet(ULONG id, ULONG type, APTR *data);

static APTR LIB_AddPrefsPool(STRPTR file_name,ULONG type);

static APTR LIB_LockPPool(STRPTR name,ULONG type);
static ULONG LIB_UnLockPPool(APTR ppool_lock);
static STRPTR LIB_NextPPoolName(STRPTR prev,ULONG type);
static ULONG LIB_AddPPoolItem(APTR ctx, APTR pitem, ULONG id, CONST_APTR data, ULONG size);
static ULONG LIB_ChangePPoolItemID(APTR ppool_lock,APTR pitem, ULONG id,ULONG new_id);
static ULONG LIB_RemovePPoolItem(APTR ctx,APTR pitem, ULONG id);
static ULONG LIB_GetPPoolItem(APTR ctx,APTR pitem, ULONG id,APTR *p, ULONG *size);
static ULONG LIB_SavePPool(APTR ppool_lock,STRPTR old_name);
static ULONG LIB_ReReadPPool(APTR ppool_lock);
static ULONG LIB_OpenPrefs(STRPTR p);
static ULONG LIB_ExecuteURI(STRPTR uri);

static const ULONG libfunctable[] =
{
	FUNCARRAY_BEGIN,
	FUNCARRAY_32BIT_NATIVE,
	// basename AmbientBase
	(ULONG)&LIB_Open,
	(ULONG)&LIB_Close,
	(ULONG)&LIB_Expunge,
	(ULONG)&LIB_GetQueryAttr,
	0xffffffff,
	FUNCARRAY_32BIT_SYSTEMV,
	// prefix LIB_
	// includefrom includes-private/clib/ambient_protos.h
	// callmode sysv
	(ULONG)&LIB_AmbientOpen,
	(ULONG)&LIB_GetViews,	
	(ULONG)&LIB_AddPrefsPool,
	(ULONG)&LIB_LockPPool,
	(ULONG)&LIB_UnLockPPool,
	(ULONG)&LIB_NextPPoolName,
	(ULONG)&LIB_AddPPoolItem,
	(ULONG)&LIB_RemovePPoolItem,
	(ULONG)&LIB_ChangePPoolItemID,
	(ULONG)&LIB_GetPPoolItem,
	(ULONG)&LIB_SavePPool,
	(ULONG)&LIB_ReReadPPool,
	(ULONG)&LIB_OpenPrefs,
	(ULONG)&LIB_StorageGet,
	(ULONG)&LIB_ExecuteURI,
	0xffffffff,
	FUNCARRAY_END
};

extern struct ExecBase *SysBase;

static ULONG flush_ambientlib(void)
{
	ULONG retval = FALSE;
	struct Library *lib;
	
	/*
	 * Check if another alien library already exists
	 * and try to flush it.
	 */
	Forbid();
	if ( (lib = (struct Library *)FindName(&SysBase->LibList, "ambient.library")) )
	{
		if (!lib->lib_OpenCnt)
		{
			RemLibrary(lib);

			if (!FindName(&SysBase->LibList, "ambient.library"))
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


static const TEXT libname[] = "ambient.library";
static const TEXT idstring[] = "ambient.library 1.0 " REVDATE " © 2025-" COMPILEYEAR " Ambient Open Source Team";

#if !USE_LEGACY
static const struct TagItem querytags[] = {
	{QUERYINFOATTR_NAME, (ULONG)libname},
	{QUERYINFOATTR_DESCRIPTION, (ULONG)"Library to interact with Ambient.."},
	{QUERYINFOATTR_COPYRIGHT, (ULONG)"© 2026 by Ambient Open Source Team"},
	{QUERYINFOATTR_AUTHOR, (ULONG)"Ambient Open Source Team"},
	{QUERYINFOATTR_SUBTYPE, QUERYSUBTYPE_LIBRARY},
	{QUERYINFOATTR_CLASS, QUERYCLASS_NONE},
	{QUERYINFOATTR_SUBCLASS, QUERYSUBCLASS_NONE},
	{TAG_DONE, 0}
};
#endif


static TEXT main_ppool[] = "MAIN_PPOOL";
/* if both systems are aczive use extra set of data files for testing*/
static TEXT PanelPath[] = "sys:Prefs/Ambient/Panels2";
//#define PANELPATH "sys:Prefs/Ambient/Panels"





ULONG ambientlib_init(void)
{
#if USE_INTERNAL_PANELS && USE_EXTERNAL_PANELS
	{
		TEXT buf[5];
		
		if ((GetVar("panel_modus",  buf, sizeof(buf), GVF_GLOBAL_ONLY) > 2) )
		{
			if(!strncmp(buf,"INT",3)) panel_modus = 1;
			if(!strncmp(buf,"EXT",3)) panel_modus = 2;
			if(!strncmp(buf,"BOTH",4)) panel_modus = 3;
		}
	}
#endif    
	if (flush_ambientlib())
	{
		AmbientLibBase = (struct AmbientLibBase*) NewCreateLibraryTags(
			LIBTAG_FUNCTIONINIT, libfunctable,
			LIBTAG_BASESIZE, sizeof(struct AmbientLibBase),
			LIBTAG_TYPE, NT_LIBRARY,
			LIBTAG_NAME, libname,
			LIBTAG_FLAGS, LIBF_CHANGED | LIBF_SUMUSED
			#if USE_LEGACY
			,
			#else 
			| LIBF_QUERYINFO,
			#endif
			LIBTAG_VERSION, AMBIENT_LIB_VERSION,
			LIBTAG_REVISION, AMBIENT_LIB_REVISION,
			LIBTAG_IDSTRING, idstring,
			LIBTAG_PUBLIC, TRUE,
			TAG_DONE);
		if(AmbientLibBase)
		{
			struct PPoolNode *ppn;
			LONG more = 0;
			BPTR lock;
			AmbientLibBase->lib.lib_OpenCnt = 0;
		
			NEWLIST(&ppool_list);
			if(!(lib_pool = CreatePool(MEMF_PUBLIC,4096,4096))) return FALSE;
			if ( (ppn = AllocPooled(lib_pool,sizeof(struct PPoolNode))) )
			{
				memset(ppn,0,sizeof(struct PPoolNode));
				InitSemaphore(&ppn->semaphore);
				ppn->type = PPOOL_TYPE_MAIN;
				ppn->n.ln_Name =  main_ppool;
				ppn->prefspool = mainprefspool;
				ADDTAIL(&ppool_list, ppn);
			}
#if USE_EXTERNAL_PANELS
			if(panel_modus == 2)	PanelPath[strlen(PanelPath) -1] = 0;
			if((lock = Lock(PanelPath, ACCESS_READ)))
			{
				struct ExAllControl *eac;
   				eac = AllocDosObject(DOS_EXALLCONTROL,NULL);
  				eac->eac_LastKey = 0;
				do {
		 			struct ExAllData *ead,EAData;
					more = ExAll(lock, &EAData, sizeof(EAData), ED_NAME, eac);
					if ((!more) && (IoErr() != ERROR_NO_MORE_ENTRIES)) {
				        //   * ExAll failed abnormally *
						break;
 						}
      				if (eac->eac_Entries == 0) {
          			/* ExAll failed normally with no entries */
        			   continue;                   /* ("more" is *usually* zero) */
       				}
					ead = (struct ExAllData *) &EAData;
       				do 
					{        
						if(!strcmp(&ead->ed_Name[strlen(ead->ed_Name)-6],".prefs"))
						{
							char path[PATH_SIZE];
							sprintf(path,"%s/%s",PanelPath,ead->ed_Name);
							LIB_AddPrefsPool(ead->ed_Name,PPOOL_TYPE_PANEL);
						}
						ead = ead->ed_Next;
					} while (ead);
  				 } while (more);
    			FreeDosObject(DOS_EXALLCONTROL,eac);
			 }
#endif
		}
	}
	/* XXX: maybe add some debugging to know what happens.. */
	return (TRUE); /* we don't fail if some lib is already here. better than nothing */
}


void ambientlib_cleanup(void)
{
	if (AmbientLibBase)
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
	if (AmbientLibBase->lib.lib_OpenCnt)
	{
		AmbientLibBase->lib.lib_Flags |= LIBF_DELEXP;
		Permit();
		return (0);
	}

	REMOVE(&AmbientLibBase->lib.lib_Node);
	Permit();

	FreeMem(((UBYTE *)AmbientLibBase) - AmbientLibBase->lib.lib_NegSize,
	        AmbientLibBase->lib.lib_NegSize + AmbientLibBase->lib.lib_PosSize);
	if(lib_pool) DeletePool(lib_pool);
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
	AmbientLibBase->lib.lib_Flags &= ~LIBF_DELEXP;
	AmbientLibBase->lib.lib_OpenCnt++;

	return ((struct Library *)AmbientLibBase);
}


static BPTR LIB_Expunge(void)
{
	return (0); /* do not let anything expunge us, except ourself */
}


static BPTR LIB_Close(void)
{
	if ((--AmbientLibBase->lib.lib_OpenCnt) == 0)
	{
		if (AmbientLibBase->lib.lib_Flags & LIBF_DELEXP)
		{
			return (0); /* same here */
		}
	}
	return (0);
}

static ULONG LIB_AmbientOpen(STRPTR path)
{
	ULONG type;
	struct List *win_list;
	APTR win_state;
	TEXT buf[ PATH_SIZE + 256 ]; /* should be enough (tm) */
	if((methodstack_push_sync_safe(app,3,OM_GET,MUIA_Application_WindowList,&win_list))&&(win_list))
	{
		APTR win;
		win_state = win_list->lh_Head;
		while( ( win = NextObject( &win_state ) ) )
		{
			methodstack_push_sync_safe(win,3,OM_GET,MA_Window_Type,&type);
			if( type == MV_Window_Type_View )
			{
				STRPTR title = 0;
				methodstack_push_sync_safe(win,3,OM_GET,MUIA_Window_Title,&title);
				if(title)
				{
					if(!strcmp(title,path))
					{
						SetAttrs(win, MUIA_Window_Open, TRUE ,TAG_DONE);
						return 0;
					}
				}
			}
		}
	}
	snprintf( buf, sizeof( buf ), "LoadURI \"%s\" TOFRONT", path );
	execute_command( 0, AC_INTERNAL, buf, NULL );
	return 0;
}

static ULONG LIB_GetViews(ULONG nr,STRPTR buffer,ULONG buffer_size)
{
	ULONG type;
	struct List *win_list;
	APTR win_state;
	ULONG buffer_pos = 0;
	ULONG n_nr = 0;
	if((!buffer_size)||(!buffer)) return 0; 
	buffer[buffer_pos] = '\0';
	if((methodstack_push_sync_safe(app,3,OM_GET,MUIA_Application_WindowList,&win_list))&&(win_list))
	{
		APTR win;
	
		win_state = win_list->lh_Head;
		while( ( win = NextObject( &win_state ) ) )
		{
			methodstack_push_sync_safe(win,3,OM_GET,MA_Window_Type,&type);
			if( type == MV_Window_Type_View )
			{
				STRPTR title = 0;
				
				if(n_nr >= nr)
				{
					methodstack_push_sync_safe(win,3,OM_GET,MUIA_Window_Title,&title);
					if(title)
					{
						ULONG title_length = strlen(title);
						if(buffer_pos + title_length + 2 < buffer_size)
						{
							strcpy(&buffer[buffer_pos],title);
							buffer_pos += title_length;
							buffer[buffer_pos++] = '\0';
							buffer[buffer_pos] = '\0';
						}
						else
						{
							buffer[buffer_pos++] = '\0';
							buffer[buffer_pos] = '\0';
							return n_nr;
						}						
					}
				}
			}
			n_nr++;
		}
	}

	return (0);
}


static ULONG LIB_StorageGet(ULONG id, ULONG type, APTR *data)
{
	storage_get(id,type,data);
	return 0;	
}

static APTR LIB_AddPrefsPool(STRPTR file_name,ULONG type)
{
	ULONG num;
	TEXT path[PATH_SIZE];
	struct PPoolNode *ppn;
	APTR truncation;
	truncation = name_truncateprefs(file_name);
	ppn = (struct PPoolNode*)FindName(&ppool_list,file_name);
	name_restoreprefs(file_name,truncation);
	if(ppn)
	{
		return ppn;
	}
	if ( (ppn = malloc(sizeof(struct PPoolNode ))) )
	{
		InitSemaphore(&ppn->semaphore);
		num = panelprefs_getnewnum();
		if((AddPart(path,PanelPath,PATH_SIZE))
		 &&(AddPart(path,file_name,PATH_SIZE))
		 &&((ppn->prefspool = prefspool_create(num)) ))
		{
			if((prefspool_read(ppn->prefspool, path, PANELPREFSID, TRUE)))
			{
				ppn->n.ln_Name =  malloc(PATH_SIZE);
				truncation = name_truncateprefs(file_name);
				strcpy(ppn->n.ln_Name,file_name);
				name_restoreprefs(file_name,truncation);
				ppn->type = type;
				ADDTAIL(&ppool_list, ppn);
				return ppn;
			}
			ppn->n.ln_Name =  malloc(PATH_SIZE);
			strcpy(ppn->n.ln_Name,file_name);
			if ( (prefspool_read(ppn->prefspool, file_name, PANELPREFSID, TRUE)))// == PREFSPOOL_IO_OK)) 
			{
				return (APTR)ppn;
			}
		}
	}
	return 0;
}




static APTR LIB_LockPPool(STRPTR name,ULONG type)
{
	struct PPoolNode *ppn = 0;
	ppn = (struct PPoolNode*)FindName(&ppool_list,name);
	if(ppn)
	{
		ObtainSemaphore(&ppn->semaphore);
		return (APTR) ppn;
	}
	return NULL;
}

static ULONG LIB_UnLockPPool(APTR ppool_lock)
{
	struct PPoolNode *ppn = (struct PPoolNode *) ppool_lock;
	if(ppn)
	{
		ReleaseSemaphore(&ppn->semaphore);
	}
	return 0;
}


static STRPTR LIB_NextPPoolName(STRPTR prev,ULONG type)
{
	struct PPoolNode *ppn = (struct PPoolNode*)ppool_list.lh_Head;
	if((prev)&&(strlen(prev)))
	{
		ppn = (struct PPoolNode*)FindName(&ppool_list,prev);
		if(ppn == 0L) return 0;
		if((struct Node*)ppn == ppool_list.lh_TailPred) return 0;
		ppn = (struct PPoolNode*)ppn->n.ln_Succ;		
	}
	while(ppn) 
	{
		if((ppn->type == type)) return ppn->n.ln_Name;
		if((struct Node*)ppn == ppool_list.lh_TailPred)
		{
			return 0;
		}
		ppn = (struct PPoolNode*)ppn->n.ln_Succ;
	}
	return 0;
}

static ULONG LIB_AddPPoolItem(APTR ppool_lock, APTR pitem, ULONG id, CONST_APTR data, ULONG size)
{
	struct PPoolNode *ppn = (struct PPoolNode *) ppool_lock;
	return (ULONG) prefspool_item_add(ppn->prefspool, pitem, id, data, size);
}

static ULONG LIB_ChangePPoolItemID(APTR ppool_lock,APTR pitem, ULONG id,ULONG new_id)
{
	struct PPoolNode *ppn = (struct PPoolNode *) ppool_lock;
	prefspool_item_id_change(ppn->prefspool,pitem, id,new_id);
	return 0;
}
static ULONG LIB_RemovePPoolItem(APTR ppool_lock,APTR pitem, ULONG id)
{
	struct PPoolNode *ppn = (struct PPoolNode *) ppool_lock;
	prefspool_item_remove(ppn->prefspool, pitem, id);
	return 0;	
}


static ULONG LIB_GetPPoolItem(APTR ppool_lock,APTR pitem, ULONG id,APTR *p, ULONG *size)
{
	ULONG retval;
	struct PPoolNode *ppn = (struct PPoolNode *) ppool_lock;
	retval = (ULONG)prefspool_item_get(ppn->prefspool,pitem, id, p,size);
	return retval;
}


static ULONG LIB_SavePPool(APTR ppool_lock,STRPTR old_name)
{
	struct PPoolNode *ppn = (struct PPoolNode *) ppool_lock;
	ULONG l;
	if((ppn->n.ln_Name)&&(l = strlen(ppn->n.ln_Name)))
	{
		ULONG size;
		STRPTR path;
		BOOL old_file = FALSE;	
		if((old_name)&&(strcmp(old_name,ppn->n.ln_Name)))
		{
			old_file = TRUE;
			if(strlen(old_name) > l) l = strlen(old_name);
		}
		size = strlen(PanelPath) + l + 10;
		path = (STRPTR) AllocPooled(lib_pool,size);
		strcpy(path,PanelPath );
		AddPart(path,ppn->n.ln_Name,size);
		sprintf(path,"%s.prefs",path);
		prefspool_write(ppn->prefspool, path, PANELPREFSID, 0);
		if(old_file)
		{
			strcpy(path,PanelPath );
			AddPart(path,old_name,size);
			sprintf(path,"%s.prefs",path);
			DeleteFile(path);
		}
		FreePooled(lib_pool,path,size);
	}
	return 0;	
}

static ULONG LIB_ReReadPPool(APTR ppool_lock)
{
	ULONG num;
	char path[PATH_SIZE];
	struct PPoolNode *ppn = (struct PPoolNode *) ppool_lock;
	num = prefspool_uid(ppn->prefspool);
	sprintf(path,"%s/%s.prefs",PanelPath,ppn->n.ln_Name);
	prefspool_delete(ppn->prefspool);
	if((ppn->prefspool = prefspool_create(num)))
	{
		prefspool_read(ppn->prefspool, path, PANELPREFSID, TRUE);
	}
	return 0;
}

static ULONG LIB_OpenPrefs(STRPTR p)
{

#if 1
	if (!prefswin)
	{
		if ( (prefswin = NewObject(getprefswin_mainclass(), NULL, TAG_DONE)) )
		{
			methodstack_push_sync_safe(app, 2,OM_ADDMEMBER, prefswin);
		}
	}
	if (prefswin)
	{
		methodstack_push_sync_safe(prefswin,2, MM_Prefswin_Main_SetPage, p);
		methodstack_push_sync_safe(prefswin,3,MUIM_Set, MUIA_Window_Open, TRUE);
	}
#endif
	return 0;
}

ULONG LIB_ExecuteURI(STRPTR uri)
{
	execute_command( NULL, AC_INTERNAL, uri, NULL );
	return 0;
}

ULONG preclose_ambientlib(void)
{
	if (AmbientLibBase && AmbientLibBase->lib.lib_OpenCnt)
	{
		smartreq_request(NULL, NULL, NULL, 0, 0, "Ok", MV_Notification_Warning, "ambient.library's opencount is %ld.\nClose the processes using it.", AmbientLibBase->lib.lib_OpenCnt);
		return (FALSE);
	}
	return (TRUE);
}
#endif /* USE_AMBIENT_LIB */ 
