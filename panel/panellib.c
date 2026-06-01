
/* public */



#define AROS_ALMOST_COMPATIBLE 1
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

#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h> 


#include <exec/lists.h>
#include <proto/ambient.h>

#include <libraries/panel.h>

/* private */
#include "panellib.h"
#include "../mui_func.h"  
#include "paneltags.h"
#include "gfx_bitmap.h"
#include "muifuncs.h"

#include "name.h"
#include "debug.h"
#include "MUIClasses.h"
#include "methodstack.h"
extern APTR app;
#warning pfusch

#define FIRSTNODE(l) ((APTR)((struct List*)l)->lh_Head)
#define NEXTNODE(n) ((APTR)((struct Node*)n)->ln_Succ)
#define ITERATELISTSAFE(node,nextnode,list) for(node=FIRSTNODE(list);(nextnode=NEXTNODE(node));node=nextnode)

static struct SignalSemaphore mainprefssem;
static APTR lib_pool;


#define PANELNODE_FLG_DELETE		0x00000001

#warning move prefspool stuff here
#if 0
struct prefspool_ctx {
	struct MinList l;
	APTR pool;
	ULONG uid; /* unique id */
};


struct prefsnode {
	struct MinNode n;
	ULONG id;
	ULONG size;
	UBYTE data[0];
};
#endif
struct PanelNode
{
	struct Node n;
	struct SignalSemaphore semaphore;
	APTR ctx;
	ULONG flags;
};

struct PanelBitMap
{
	struct SignalSemaphore semaphore;
	struct BitMap *bm;	
};

struct ExtClassNode
{
	struct Node n;
	STRPTR path;
};



static struct List panel_list;
static struct List class_list;

struct PanelLibBase
{
	struct Library lib;
};

struct LibBase
{
	struct Library  Lib;
	BPTR            SegList;
	struct ExecBase *sysBase;
	struct Library  *utilityBase;
};



struct PanelLibBase *PanelLibBase;

static BPTR libexpunge(void);
static struct Library * LIB_Open(void);
static BPTR LIB_Close(void);
static BPTR LIB_Expunge(void);
void LIB_Reserved(void);


static ULONG LIB_PanelPrefsApp(APTR app,APTR obj);
static ULONG LIB_PanelHasChanged(STRPTR panel_name,ULONG mode,ULONG pos1, ULONG pos2);
static ULONG LIB_PrefsPoolUpdate(STRPTR panel_name,ULONG object_id,ULONG prefs_id);
static STRPTR LIB_NextPanelName(STRPTR lastname);
static APTR LIB_LockPanel(STRPTR name);
static ULONG LIB_UnLockPanel(APTR panel_lock);
static STRPTR LIB_AddPanel(STRPTR);
static ULONG LIB_SavePanel(APTR panel_lock,STRPTR old_name);
static ULONG LIB_CreatePanelItem(ULONG type_ID, STRPTR class_name);
static ULONG LIB_AddPanelItem(APTR panel_lock, APTR pitem, ULONG id, CONST_APTR data, ULONG size);
static ULONG LIB_RemovePanelItem(APTR panel_lock,APTR pitem, ULONG id);
static ULONG LIB_GetPanelAttr(APTR panel_lock,APTR pitem, ULONG id,APTR *p, ULONG *size);
static ULONG LIB_SetPanelAttr(APTR panel_lock,APTR pitem, ULONG id,APTR data, ULONG size);
static ULONG LIB_SetPanelObjectAttr(APTR pobj,ULONG ti_Tag, ULONG ti_Data);
static ULONG LIB_GetPanelObjectAttr(ULONG Tag, APTR pobj, ULONG *storage);
static ULONG LIB_NewPanel(STRPTR p_name);
static ULONG LIB_RemovePanelObject(APTR panel_object);
static ULONG LIB_InsertPanelObject(APTR panel_win,APTR panel_object,LONG pos);
static ULONG LIB_MovePanelObject(APTR panel_win,ULONG src_id,ULONG target_id,LONG pos);
static ULONG LIB_FindPanelObject(APTR Arg1,ULONG id);

static ULONG LIB_SaveAll(void);
static ULONG LIB_ReLoadPanels(void);

static void LIB_AddExternClass(STRPTR path,STRPTR class_name);
static STRPTR LIB_NextExternClass(STRPTR last_name);


static ULONG LIB_ObtainPanelObject(APTR panel,struct TagItem  *tags);
static void LIB_ReleasePanelObject(APTR panelobject);
static APTR LIB_AllocPanelBitMap(ULONG width,ULONG height);
static void LIB_FreePanelBitMap(APTR bitmap);

static ULONG LIB_SetPanelObjectBitMap(APTR panelobject,struct BitMap *bm,ULONG width,ULONG height);
static ULONG LIB_PanelMode(APTR panel_lock,ULONG modus);

ULONG LibFuncTable[]=
{
	FUNCARRAY_BEGIN,
		FUNCARRAY_32BIT_NATIVE,

		// basename PanelBase
		(ULONG) &LIB_Open,       /* Old ABOX Library ABI Function Block */
		(ULONG) &LIB_Close,
		(ULONG) &LIB_Expunge,
		(ULONG) &LIB_Reserved,
		0xffffffff,

		FUNCARRAY_32BIT_SYSTEMV, /* New SystemV ABI Function Block..can't be called from 68k anymore */

		// prefix LIB_
		// includefrom ../includes-private/clib/panel_protos.h
		// callmode sysv
		(ULONG)&LIB_PanelPrefsApp,
		(ULONG)&LIB_PanelHasChanged,
		(ULONG)&LIB_PrefsPoolUpdate,
		(ULONG)&LIB_NextPanelName,
		(ULONG)&LIB_LockPanel,
		(ULONG)&LIB_UnLockPanel,
		(ULONG)&LIB_AddPanel,
		(ULONG)&LIB_SavePanel,
		(ULONG)&LIB_CreatePanelItem,
		(ULONG)&LIB_AddPanelItem,
		(ULONG)&LIB_RemovePanelItem,
		(ULONG)&LIB_SetPanelObjectAttr,
		(ULONG)&LIB_GetPanelObjectAttr,
		(ULONG)&LIB_NewPanel,
		(ULONG)&LIB_RemovePanelObject,
		(ULONG)&LIB_InsertPanelObject,
		(ULONG)&LIB_MovePanelObject,
		(ULONG)&LIB_SaveAll,
		(ULONG)&LIB_ReLoadPanels,
		(ULONG)&LIB_AddExternClass,
		(ULONG)&LIB_NextExternClass,
		(ULONG)&LIB_ObtainPanelObject,
		(ULONG)&LIB_ReleasePanelObject,
		(ULONG)&LIB_AllocPanelBitMap,
		(ULONG)&LIB_FreePanelBitMap,
		(ULONG)&LIB_SetPanelObjectBitMap,
		(ULONG)&LIB_PanelMode,
		0xffffffff,
	FUNCARRAY_END
};

void LIB_Reserved(void)
{
}



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
static const TEXT idstring[] = "panel.library 1.3 ";// REVDATE " © 2015-" COMPILEYEAR " Ambient Open Source Team";

#if !USE_LEGACY
static const struct TagItem querytags[] = {
	{QUERYINFOATTR_NAME, (ULONG)libname},
	{QUERYINFOATTR_DESCRIPTION, (ULONG)"Library to interact panel system.."},
	{QUERYINFOATTR_COPYRIGHT, (ULONG)"© 2026 by Ambient Open Source Team"},
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
			LIBTAG_FUNCTIONINIT, LibFuncTable,
			LIBTAG_BASESIZE, sizeof(struct PanelLibBase),
			LIBTAG_TYPE, NT_LIBRARY,
			LIBTAG_NAME, libname,
			LIBTAG_FLAGS, LIBF_CHANGED | LIBF_SUMUSED
			#if USE_LEGACY
			,
			#else 
			| LIBF_QUERYINFO,
			#endif
			LIBTAG_VERSION, PANEL_LIB_VERSION,
			LIBTAG_REVISION,  PANEL_LIB_REVISION,
			LIBTAG_IDSTRING, idstring,
			LIBTAG_PUBLIC, TRUE,
			TAG_DONE);

		if(PanelLibBase)
		{
			PanelLibBase->lib.lib_OpenCnt = 0;
			InitSemaphore(&mainprefssem);
			NEWLIST(&panel_list);
			NEWLIST(&class_list);
			lib_pool = CreatePool(MEMF_PUBLIC,4096,4096);
			if(!lib_pool) return (FALSE);
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
			//PDB(("should not happen (tm)\n")); /* XXX: actually it can but well .. */
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
	if(lib_pool) DeletePool(lib_pool);
	return (TRUE);
}




static struct Library * LIB_Open(void)
{
	struct LibBase *MyLibBase = (struct LibBase*)REG_A6;
	Forbid();
	PanelLibBase->lib.lib_Flags &= ~LIBF_DELEXP;
	PanelLibBase->lib.lib_OpenCnt++;
	Permit();
	//PDB(("%d\n",PanelLibBase->lib.lib_OpenCnt));
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

static APTR prefs_app = NULL;
static APTR prefs_obj = NULL;

struct PanelMessage mm;

static ULONG LIB_PanelPrefsApp(APTR app,APTR obj)
{
	//PDB(("new %x %x old %x %x\n",app,obj,prefs_app ,prefs_obj));
	prefs_app = app;
	prefs_obj = obj;
	return 0;
}

static ULONG LIB_PanelHasChanged(STRPTR panel_name,ULONG mode,ULONG pos1, ULONG pos2)
{

	if((prefs_app)&&(prefs_obj))
	{
		if(panel_name)strcpy(mm.pm_Name,panel_name);
		else mm.pm_Name[0] = 0;
		mm.pm_Mode = mode;
		mm.pm_Data1 = pos1;
		DoMethod(prefs_app,MUIM_Application_PushMethod,prefs_obj,2,MM_PanelPrefs_PanelMessage,&mm);
	/*	if(mode == PanelPrefsClose)
		{
			ULONG n = 0;
			while(((prefs_app)||(prefs_obj))&&(n < 10))
			{
				Delay(50);
				n++;
				PDB(("%d\n",n));
			}
		}*/
	}
	return 0;
}

static ULONG LIB_PrefsPoolUpdate(STRPTR panel_name,ULONG object_id,ULONG prefs_id)
{
	DoMethod(app,MUIM_Application_PushMethod,app,4,MM_Application_PrefsPoolUpdate,panel_name,object_id,prefs_id);
	return 0;	
}
static STRPTR LIB_NextPanelName(STRPTR lastname)
{
	struct Node *n,*nn = 0;
	if(lastname) 
	{
		n = FindName(&panel_list,lastname);
		if(n == panel_list.lh_TailPred)
		{
			return 0;			
		}
		if(n) 
		{
			nn = n->ln_Succ;
		}
	}
	else nn = panel_list.lh_Head;
	if(nn) 
	{
		struct PanelNode *pn = (struct PanelNode*)nn;
		if(pn->flags & PANELNODE_FLG_DELETE)
		{
			return LIB_NextPanelName(nn->ln_Name);
		}
		return nn->ln_Name;
	}
	return 0;
}

static APTR LIB_LockPanel(STRPTR name)
{
	ULONG retval;	
	retval = methodstack_push_sync_safe(app,2,MM_Application_FindPanel,name);
	return retval;
}

static ULONG LIB_UnLockPanel(APTR panel_lock)
{
	struct PanelNode *pn = (struct PanelNode*)panel_lock;
	#warning MIA
	return 0;
	if(pn)
	{
		ReleaseSemaphore(&pn->semaphore);
	}
	return 0;
}



#warning check existing panels for real
static maxuid = 10;
static ULONG panelprefs_getnewnum()
{
	//struct ppnode *ppn = 0;
//	ULONG uid = 4;
/*	BOOL free = FALSE;
	while(!(free))
	{
		free = TRUE;
		uid++;
		ITERATELIST(ppn, &pplist)
		{
			if (ppn->uid == uid)
			{
				free = FALSE;
				break;
			}
		}
	}*/
	return maxuid++;
}
static char fpath[PATH_SIZE];
#warning use ambient.library
static STRPTR LIB_AddPanel(STRPTR path)
{
	APTR ctx = 0;
	APTR truncation;
	STRPTR _path = path;
	char npath[PATH_SIZE];
	char f_name[PATH_SIZE];
	STRPTR path_cpy = 0;
	#warning remove prefs before adding
#if 0
	if(!path)
	{
		ULONG uid = panelprefs_getnewnum();
		APTR window;
		APTR truncation;
		sprintf(f_name,"New_Panel_%d.prefs",uid);
		sprintf(npath,"%s%s",PANEL_PATH,f_name);
		ctx = prefspool_create(uid);
		_path =f_name;
	}
	else
	{
		sprintf(fpath,"%s%s",PANEL_PATH,_path);
		ctx =	prefspool_open(fpath,0);
	}
	truncation = name_truncateprefs(_path);
	if(ctx)
	{
		struct PanelNode *node = (struct PanelNode *) AllocPooled(lib_pool,sizeof(struct PanelNode));
		path_cpy = (STRPTR) AllocVecPooled(lib_pool,strlen(_path)+1);
		if((node)&&(path_cpy))
		{
			node->n.ln_Name = path_cpy;
			node->n.ln_Succ = node->n.ln_Pred = NULL;
			strcpy(path_cpy,_path);
			node->flags = 0;
			InitSemaphore(&node->semaphore);
			ADDTAIL(&panel_list,&node->n);
		}
	}
	name_restoreprefs(_path, truncation);
	return path_cpy;
#endif
	return 0;
}

#warning define proper panel dir
#warning save via ambient.library
static char paneldir[] = "PROGDIR:";

static ULONG LIB_SavePanel(APTR panel_lock,STRPTR old_name)
{
#if 0
	struct PanelNode *pn = (struct PanelNode *) panel_lock;
	ULONG l;
	if((pn->n.ln_Name)&&(l = strlen(pn->n.ln_Name)))
	{
		ULONG size;
		STRPTR path;
		BOOL old_file = FALSE;

		if((old_name)&&(strcmp(old_name,pn->n.ln_Name)))
		{
			old_file = TRUE;
	
			if(strlen(old_name) > l) l = strlen(old_name);
		}
		size = strlen(paneldir) + l + 10;
		path = (STRPTR) AllocPooled(lib_pool,size);
		strcpy(path,paneldir);
		AddPart(path,pn->n.ln_Name,size);
		sprintf(path,"%s.prefs",path);
		prefspool_write(pn->ctx, path, PANELPREFSID, 0);
		if(old_file)
		{
			strcpy(path,paneldir);
			AddPart(path,old_name,size);
			sprintf(path,"%s.prefs",path);
			DeleteFile(path);
		}
		FreePooled(lib_pool,path,size);
	}
#endif
	return 0;	
}

static ULONG LIB_CreatePanelItem(ULONG type_ID, STRPTR class_name)
{
	APTR obj;
	switch(type_ID)
	{
		case MV_Panel_Type_Spacer:
			obj = NewObject( GetClass("Spacer"), NULL, TAG_DONE );
			return (ULONG) obj;
		case MV_Panel_Type_Separator:
			obj = NewObject( GetClass("Separator"), NULL, TAG_DONE );
			return (ULONG) obj;
		case MV_Panel_Type_ViewWatcher:
			obj = NewObject( GetClass("ViewWatcher"), NULL, TAG_DONE );
			return (ULONG) obj;
		case MV_Panel_Type_Bookmarks:
			obj = NewObject( GetClass("Bookmarks"), NULL, TAG_DONE );
			return (ULONG) obj;
		case MV_Panel_Type_SubPanel :
			obj = NewObject( GetClass("SubPanelButton"), NULL, TAG_DONE );
			return (ULONG) obj;
		case MV_Panel_Type_External:
		//default:
	#warning build class path

			obj = MUI_NewObject("PROGDIR:Classes/Panel/DigiClock.pobj",TAG_DONE);
			return obj;
	}
	return 0;
}


#warning warning
static ULONG LIB_AddPanelItem(APTR panel_lock, APTR pitem, ULONG id, CONST_APTR data, ULONG size)
{
	struct PanelNode *pn = (struct PanelNode *) panel_lock;
	return 0;//(ULONG) prefspool_item_add(pn->ctx, pitem, id, data, size);
}


static ULONG LIB_RemovePanelItem(APTR panel_lock,APTR pitem, ULONG id)
{
	struct PanelNode *pn = (struct PanelNode *) panel_lock;
	//prefspool_item_remove(pn->ctx, pitem, id);
	
	return 0;	
}
#if 0

static ULONG LIB_GetPanelAttr(APTR panel_lock,APTR pitem, ULONG id,APTR *p, ULONG *size)
{
	ULONG retval;
	struct PanelNode *pn = (struct PanelNode*)panel_lock;
	if(pn)
	{
		retval = (ULONG)prefspool_item_get(pn->ctx,pitem, id, p,size);
		return retval;
	}
	return 0;
}


static ULONG LIB_SetPanelAttr(APTR panel_lock,APTR pitem, ULONG id,APTR data, ULONG size)
{
	ULONG retval;
	struct PanelNode *pn = (struct PanelNode*)panel_lock;
	if(pn)
	{
		ULONG r = (ULONG)prefspool_item_add(pn->ctx, pitem, id, data, size);
		return r;
	}
	return 0;
}
#endif
static ULONG LIB_SetPanelObjectAttr(APTR pobj,ULONG ti_Tag, ULONG ti_Data)
{
//	methodstack_push(pobj,3,MUIM_Set, ti_Tag, ti_Data);
	DoMethod(app,MUIM_Application_PushMethod,pobj,3,MUIM_Set, ti_Tag, ti_Data);
#warning imagepath
#if 0
	if((ti_Tag == MA_Panel_Imagepath) && (ti_Data))
	{
	
	}
	if(ti_Tag == MA_Panelwin_Name)
	{
		struct PanelNode *pn = 0;
		pn = (struct PanelNode*)FindName(&panel_list,p_name);
		if((pn)&&(ti_Data)&&(strlen((STRPTR)ti_Data)))
		{
			STRPTR str_cpy = (STRPTR) AllocVecPooled(lib_pool,strlen((STRPTR)ti_Data)+1);	
			if((pn)&&(str_cpy))
			{
				if(pn->n.ln_Name) FreeVecPooled(lib_pool,pn->n.ln_Name);
				pn->n.ln_Name = str_cpy;
				strcpy(str_cpy,(STRPTR)ti_Data);
			}
		}
	}
	//else
	{
		
		DoMethod(app,MUIM_Application_PushMethod,app,5,MM_Application_SetObjectAttr,p_name,obj_id,ti_Tag,ti_Data);
	}
	
#endif
	return 0;
}

static ULONG LIB_GetPanelObjectAttr(ULONG Tag, APTR pobj, ULONG *storage)
{
	if(pobj)
	{
		return methodstack_push_sync(pobj,3,OM_GET,Tag,storage);
	}
	return 0;
}

static ULONG LIB_NewPanel(STRPTR p_name)
{
	DoMethod(app,MUIM_Application_PushMethod,app,2,MM_Application_NewPanel,p_name);
	
	return 0;
}

static ULONG LIB_RemovePanelObject(APTR panel_object)
{
	if(panel_object)
	{
		APTR parent_object;
		methodstack_push_sync(panel_object,3,OM_GET,MUIA_Parent,&parent_object);
		if(parent_object)
		{
			methodstack_push_sync(parent_object,2,OM_REMMEMBER,panel_object);
		}
	}
	return 0;
}

static ULONG LIB_InsertPanelObject(APTR panel_win,APTR panel_object,LONG pos)
{
	if((panel_win) && (panel_object))
	{
		APTR group_object;
		methodstack_push_sync_safe(panel_win,3,OM_GET,MA_Panelwin_Group,&group_object);
		if(group_object)
		{
			methodstack_push_sync_safe(group_object,1,MUIM_Group_InitChange);
			methodstack_push_sync_safe(group_object,2,OM_ADDMEMBER,panel_object);
			methodstack_push_sync_safe(group_object,1,MUIM_Group_ExitChange);
		}
	}
	return 0;
}

static ULONG LIB_MovePanelObject(APTR panel_win,ULONG src_id,ULONG target_id,LONG pos)
{
//	struct PanelNode *dst_pn = (struct PanelNode*)dst_panel_lock;
//	struct PanelNode *src_pn = (struct PanelNode*)src_panel_lock;
	//PDB(("%x %x %x %x\n",panel_win,src_id,target_id,pos));
	return 0;
//	if((dst_pn) && (src_pn))
	{
//		DoMethod(app,MUIM_Application_PushMethod,app,6,MM_Application_MoveObject,dst_pn->n.ln_Name,dst_id,pos,src_pn->n.ln_Name,src_id);
	}
	return 0;
}
/*
static ULONG LIB_SetGlobalPrefs(ULONG ID, ULONG value)
{
	ObtainSemaphore(&mainprefssem);
	setprefslong_ctx(mainprefspool,ID,value);
	ReleaseSemaphore(&mainprefssem);
	return 0;
}

static ULONG LIB_GetGlobalPrefs(ULONG ID)
{
	ULONG retval;
	ObtainSemaphoreShared(&mainprefssem);
	retval = getprefslong_ctx(mainprefspool,ID);
	ReleaseSemaphore(&mainprefssem);
	return retval;
}
*/
static ULONG LIB_SaveAll(void)
{
	struct Node *n,*n2;
	ForeachNodeSafe(&panel_list,n,n2)
	{
		struct PanelNode *pn = (struct PanelNode*)n;
		if(pn->flags & PANELNODE_FLG_DELETE)
		{
			ULONG l;
			if((pn->n.ln_Name)&&(l = strlen(pn->n.ln_Name)))
			{
				ULONG size;
				STRPTR path;
				size = strlen(paneldir) + l + 10;
				path = (STRPTR) AllocPooled(lib_pool,size);
				strcpy(path,paneldir);
				AddPart(path,pn->n.ln_Name,size);
				sprintf(path,"%s.prefs",path);
				DeleteFile(path);
				FreePooled(lib_pool,path,size);
			}
		}
		else
		{
			DoMethod(app,MUIM_Application_PushMethod,app,2,MM_Application_SavePanel,n->ln_Name);
		}
		DoMethod(app,MUIM_Application_PushMethod,app,1,MM_Application_SaveMainprefs);
	}	
	return 0;
}

static void LIB_AddExternClass(STRPTR path,STRPTR class_name)
{
	if( !( FindName( &class_list, class_name ) ) )
	{
		struct ExtClassNode *node = (struct ExtClassNode *) AllocPooled(lib_pool,sizeof(struct ExtClassNode));
		STRPTR path_cpy = (STRPTR) AllocVecPooled(lib_pool,strlen(path)+1);
		STRPTR name_cpy = (STRPTR) AllocVecPooled(lib_pool,strlen(class_name)+1);
		if((node)&&(path_cpy)&&(name_cpy))
		{
			node->n.ln_Name = name_cpy;
			node->n.ln_Succ = node->n.ln_Pred = NULL;
			node->path = path_cpy;
			strcpy(name_cpy,class_name);
			strcpy(path_cpy,path);
		
			ADDTAIL(&class_list,&node->n);
		}
	}
}

static STRPTR LIB_NextExternClass(STRPTR lastname)
{
	struct Node *n,*nn = 0;
	if(lastname) 
	{
		n = FindName(&class_list,lastname);
		if(n == class_list.lh_TailPred)
		{
			return 0;			
		}
		if(n) 
		{
			nn = n->ln_Succ;
		}
	}
	else nn = class_list.lh_Head;
	if(nn) 
	{
		struct PanelNode *pn = (struct PanelNode*)nn;
		if(pn->flags & PANELNODE_FLG_DELETE)
		{
			return LIB_NextPanelName(nn->ln_Name);
		}
		return nn->ln_Name;
	}
	return 0;
}


static ULONG LIB_ReLoadPanels(void)
{
	while(REMHEAD(&panel_list));
	DoMethod(app,MUIM_Application_PushMethod,app,1,MM_Application_LoadAll);
	return 0;
}

static ULONG LIB_ObtainPanelObject(APTR panel_win,struct TagItem  *tags)
{
	APTR panel_group,panel_object;
	struct TagItem *id_Tag;
	if(!panel_win) return 0;
	
	if((id_Tag = FindTagItem(PanelObjectID,tags)))
	{
		if(id_Tag->ti_Data == 0) return panel_win;
		methodstack_push_sync_safe(panel_win,3,OM_GET,MA_Panelwin_Group,&panel_group);
		if(panel_group)
		{
			methodstack_push_sync_safe(panel_group,4,MM_Panelgroup_FindID,id_Tag->ti_Data,0,&panel_object);
		#warning OM_RETAIN
			//	if(panel_object)methodstack_push_sync_safe(panel_object,1,OM_RETAIN);
	
			return panel_object;
		}
	}
	return 0;
}


#if 0
static ULONG LIB_ObtainPanelObject(struct TagItem  *tags)
{
	APTR win;
	ULONG type;

	struct TagItem *typeTag = 0;
	struct TagItem *predTag= 0;
	struct TagItem *winTag = 0;
	struct TagItem *win_nameTag = 0;
	struct TagItem *uriTag = 0;
	APTR panel_win = 0;
	APTR targetobject = 0;
	APTR panelobject = 0;

	APTR win_state;
	struct List *win_list;

//	struct List *child_list;
	BOOL pred_found = FALSE;
	APTR pred = NULL;
	STRPTR objectname = 0;
//	PDB(("%x %x\n",app,tags));

	panelobject = methodstack_push_sync_safe( app,2,MM_Application_ObtainObject,tags );
//	PDB(("%x\n",panelobject));
#warning
//	if(panelobject)	methodstack_push_sync_safe(panelobject,1,OM_RETAIN);
	return panelobject;
	
	
	
	
	if((winTag = FindTagItem(PanelObjectWindow,tags))&&(winTag->ti_Data))
	{
		panel_win = (APTR)winTag->ti_Data;
	}
	else if((win_nameTag = FindTagItem(PanelObjectWindowName,tags)))//&&(win_nameTag->ti_Data))
	{
		APTR win_state;
		STRPTR p_name;
		struct List *win_list;
		APTR win;
		ULONG type;
		GetAttr( MUIA_Application_WindowList, app, (ULONG*) &win_list );
		win_state = win_list->lh_Head;
		#warning
		while(0)// ( win = NextObject( &win_state ) ) )
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
	
	//return 0;
	if((uriTag = FindTagItem(PanelObjectURI,tags)))	objectname = (STRPTR) uriTag->ti_Data;
	typeTag = FindTagItem(PanelObjectType,tags);
	predTag = FindTagItem(PanelObjectStartObject,tags);
	if((predTag) && (predTag->ti_Data != 0L))
	{
		//methodstack_push_sync_safe( (APTR) predTag->ti_Data,3,OM_GET,MA_Panelmessenger_PanelObject,(ULONG*) &pred );
	}
	//if(objectname) PDB(("%s\n",objectname));
/*	if((winTag = FindTagItem(PanelObjectWin,tags))&& (winTag->ti_Data ))
	{	
		Object *win;
		//methodstack_push_sync_safe( (APTR) winTag->ti_Data,3,OM_GET,MA_Panelmessenger_PanelObject,(ULONG*) &win);
		//if(win) return check_by_pred(win,pred);
	}*/
	#if 1
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
							//	PDB(("win target %x\n",win));
							//	panelobject = (APTR) methodstack_push_sync_safe(PanelLibBase->object_list,2,MM_Panelmessengerfamily_NewObject,targetobject);
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
							//	PDB((" target %x\n",targetobject));
						//	panelobject = (APTR) methodstack_push_sync_safe(PanelLibBase->object_list,2,MM_Panelmessengerfamily_NewObject,targetobject);
						}	
					}
				}
			}
		}
	}
	if(targetobject)	methodstack_push_sync_safe(targetobject,1,OM_RETAIN);
	#endif
	return (ULONG) panelobject;
}
#endif

void LIB_ReleasePanelObject(APTR panelobject)
{
	#warning
	//if(panelobject)	methodstack_push_sync_safe(panelobject,1,OM_RELEASE);
}


static APTR LIB_AllocPanelBitMap(ULONG width,ULONG height)
{
	struct BitMap *bm;	
	struct PanelBitMap *pbm = (struct PanelBitMap *) AllocPooled(lib_pool,sizeof(struct PanelBitMap));
	if(!pbm) return 0;
	if((bm = AllocBitMap(width,height,32, BMB_SPECIALFMT| PIXFMT_ARGB32,0)))
	{
		InitSemaphore(&pbm->semaphore);
		pbm->bm = bm;
		return pbm;		
	}
	return 0;
}

static void LIB_FreePanelBitMap(APTR bitmap)
{
}

static ULONG LIB_SetPanelObjectBitMap(APTR panelobject,struct BitMap *bm,ULONG width,ULONG height)
{
	if(panelobject)
	{
		methodstack_push_sync_safe(panelobject,4,MM_Panelbutton_ExtraBitmap,bm,width,height);
	}
	return 0;
}



ULONG preclose_panellib(void)
{
	struct ExtClassNode *node, *nnode;
		#define PANELNAME_SIZEOF 200
	struct Library *flush;
	ULONG n = 0;
	TEXT panelpath[ PANELNAME_SIZEOF ];
	#warning
//	PDB(("PanelLib opencnt %d\n",PanelLibBase->lib.lib_OpenCnt));
	//if(prefs_obj)	
		
	while((PanelLibBase->lib.lib_OpenCnt)&&(prefs_app)&&(prefs_obj)&&(n < 500))
	{
	//	mm.pm_Name = 0;
	//	mm.pm_Mode =  PanelPrefsClose;
	//	mm.pm_Data1 = 0;

		DoMethod(prefs_app,MUIM_Application_PushMethod,prefs_obj,2,MM_PanelPrefs_PanelMessage,&mm);
	
		Delay(10);
		n++;
	}

//	PDB(("PanelLib opencnt %d delay %d\n",PanelLibBase->lib.lib_OpenCnt,n));
#warning proper path
					
					//	strcpy( panelpath, PANEL_DISKPATHMOSSYS );
				
#if 0

	ITERATELISTSAFE( node, nnode, &class_list )
	{
		//FreeVecPooled(lib_pool,node->n.ln_Name);/* o.k. than remove from list*/
		//FreeVecPooled(lib_pool,node->path);
		strcpy( panelpath, "PROGDIR:Classes/Panel" );
		AddPart( panelpath, node->n.ln_Name, PANELNAME_SIZEOF );
		if( !( flush = OpenLibrary( panelpath, 0 ) ) ) /* try to open from MOSSYS: */
		{  
		//	flush = OpenLibrary( &panelpath[3], 0 ) ) ) { /* try to open from SYS: */
		}
		if( flush ) 
		{
			RemLibrary( flush );
			CloseLibrary( flush );
		}
	//	PDB(("%s\n",panelpath));
		REMOVE( node );
	//	FreeVecPooled(lib_pool,node);
	}
#endif
#if 0
	
					{ struct Library *flush;
					if( !( flush = OpenLibrary( panelpath, 0 ) ) ) {  /* try to open from MOSSYS: */
						if( !( flush = OpenLibrary( &panelpath[3], 0 ) ) ) { /* try to open from SYS: */
							PanelItem_Delete( pi ); /* no class found, so kill our self */
							pi = NULL;
							break;
						}
					}
				//	kprintf("b %x %s\n",pi,panelpath);
					if( flush ) {
						RemLibrary( flush );
						CloseLibrary( flush );
					}
					}
#endif

	/*
	if (PanelLibBase && PanelLibBase->lib.lib_OpenCnt)
	{
		smartreq_request(NULL, NULL, NULL, 0, 0, "Ok", MV_Notification_Warning, "panel.library's opencount is %ld.\nClose the processes using it.", PanelLibBase->lib.lib_OpenCnt);
		return (FALSE);
	}*/
	return (TRUE);
}

static ULONG LIB_PanelMode(APTR panel_lock,ULONG modus)
{
	struct PanelNode *pn = (struct PanelNode*) panel_lock;
	if((panel_lock)&&(modus == PANEL_MODE_PREFS))
	{
		DoMethod(app,MUIM_Application_PushMethod,app,3,MM_Application_Modus,pn->n.ln_Name,modus);
	}
	return 0;
}

