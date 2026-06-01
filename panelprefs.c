/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006 Ambient Open Source Team
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
 * $Id: panelprefs.c,v 1.16 2026/03/16 17:53:49 kronos Exp $
 */

#include "ambient.h"

#if USE_INTERNAL_PANELS

/* public */
#include <exec/semaphores.h>
#include <dos/exall.h>
#include <dos/dosextens.h>
#include <proto/dos.h>

/* private */
#include "panelprefs.h"
#include "prefs.h"
#include "exdir.h"
#include "threads.h"
#include "mui_func.h"
#include "file_func.h"
#include "methodstack.h"
#include "iconio.h"
#include "paneltags.h"


static struct SignalSemaphore ppsem;
static struct MinList pplist;

extern ULONG panel_modus;

struct ppnode {
	struct Node n;
	ULONG loaded;
	APTR prefspool;
	ULONG uid;
	ULONG num;
	APTR panelwinobj;
	struct SignalSemaphore sem;
};


ULONG panelprefs_init(void)
{
	InitSemaphore(&ppsem);
	NEWLIST(&pplist);

	return (TRUE);
}


void panelprefs_cleanup(void)
{
	/* nothing yet */
}


/*
 * To be called from Ambient's main thread so that
 * the semaphore prevents saving threads.
 */
void panelprefs_loadall(void)
{
	MAINTASK;
	ObtainSemaphore(&ppsem);

	do_action(app, TA_Panels_LoadAll,
	TAG_DONE);
}


/*
 * To be called once the loading thread finished its
 * work.
 */
void panelprefs_loaded(void)
{
	ReleaseSemaphore(&ppsem);
}

void panelprefs_add(APTR obj,APTR prefspool)
{
	struct ppnode *ppn = 0;

	if ( (ppn = malloc(sizeof(*ppn))) )
	{
		InitSemaphore(&ppn->sem);
		ppn->prefspool = prefspool;
		{
			ppn->n.ln_Name =  NULL;
			ppn->uid = prefspool_uid(ppn->prefspool);
			ppn->num = ppn->uid;
			ppn->panelwinobj = obj;
			ADDTAIL(&pplist, ppn);
		}
	}
}


/*
 * To be called when one wants to save a panel.
 */
void panelprefs_save(APTR pctx)
{
	MAINTASK;
	do_action(app, TA_Panels_Save, TT_Panels_Save_Ctx, pctx, TAG_DONE); /* XXX: ObtainSemaphoreShared(&ppsem) from the thread!! */
}

/* just a helper-function to name panels in prefswin */
ULONG panelprefs_getnewnum()
{
	struct ppnode *ppn = 0;
	ULONG uid = 0;
	BOOL free = FALSE;
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
	}
	return uid;
}



void panelprefs_getname(APTR pctx,STRPTR name)
{
	struct ppnode *ppn = 0;
    ULONG uid;
	ASSERT(pctx);
	name[0] = '\0';
	ObtainSemaphoreShared(&ppsem);
	uid = prefspool_uid(pctx);
	ITERATELIST(ppn, &pplist)
	{
		if (ppn->uid == uid)
		{
			ULONG len;
			if(!(ppn->n.ln_Name))
			{
				WORD No = 0;
				STRPTR name;
				name = malloc(PATH_SIZE);
				do
				{
					No++;
					sprintf(name,"New_Panel_%d.prefs",No);
				}
				while(FindName((struct List*)&pplist,name));
				ppn->n.ln_Name = name;
				//strcpy(ppn->n.ln_Name,"Leistenbruch.prefs");
			}
			len = strlen(ppn->n.ln_Name);
			if(len > 6)
			{
				strncpy(name,ppn->n.ln_Name,len-6);
				name[len-6] = '\0';
			}
		}
	}
	ReleaseSemaphore(&ppsem);
}


static ULONG addpanelfunc(APTR obj UNUSED, CONST_STRPTR path, struct ExAllData *ead, APTR userdata UNUSED)
{
	struct ppnode *ppn;
//	  STRPTR p;
	ULONG num;

	if (ead->ed_Type > 0 &&
	    ead->ed_Type != ST_LINKFILE)
	{
		return (TRUE); /* skip directories */
	}

	/*
	 * Parse the number (we rely on the pattern parsing to really work)
	 */
#if 0
	p = ead->ed_Name + 6;
	num = 1000 * (*p++ - '0');
	num += 100 * (*p++ - '0');
	num += 10 * (*p++ - '0');
	num += *p - '0';

	/*
	 * We check if it already exists because
	 * someone could fool us with a case sensitive
	 * FS. I don't like people trying to fool me.
	 */
	ITERATELIST(ppn, &pplist)
	{
		if (ppn->num == num)
		{
			/* XXX: tell so */
			PDB(("already in list\n"));
			return (FALSE);
		}
	}
#endif
	num = panelprefs_getnewnum();
	if ( (ppn = malloc(sizeof(*ppn))) )
	{
		InitSemaphore(&ppn->sem);
		if ( (ppn->prefspool = prefspool_create(num)) )
		{
			TEXT fullpath[PATH_SIZE];

			stccpy(fullpath, path, sizeof(fullpath));
			AddPart(fullpath, ead->ed_Name, sizeof(fullpath));
			ppn->n.ln_Name =  malloc(PATH_SIZE);
			strcpy(ppn->n.ln_Name,ead->ed_Name);
			if ( (ppn->loaded = (prefspool_read(ppn->prefspool, fullpath, PANELPREFSID, TRUE) == PREFSPOOL_IO_OK)) )
			{
				ppn->uid = prefspool_uid(ppn->prefspool);
				ppn->num = num;
				ADDTAIL(&pplist, ppn);
				return (TRUE);
			}
			else
			{
				/* continue loading anyway.. XXX: we should make the slot as "problematic" to make sure the user doesn't overwrite it with a new panel */
				return (TRUE);
			}
		}
		/* XXX */
	}
	/* XXX */
	return (FALSE); /* severe failure */
}



/*
 * Called from TA_PANELS_LOADALL.
 */
ULONG tr_panels_loadall(void)
{
	APTR panelobj;
	APTR pl, pi,issub; /* list, item */
	ULONG *type;
	ULONG i;
	struct ppnode *ppn,*pn;
	ULONG *sub;
	if((panel_modus == 2)) 
	{
		Execute("mossys:Ambient/PanelApp",0,0);
		return 0;
	}
	THREAD;
	if(!(ISLISTEMPTY(&pplist)))
	{
		ITERATELISTSAFE(ppn,pn, &pplist)
		{
			REMOVE(ppn);
		}
	}
	exdir(NULL, PREFS_PATH "Panels", "*.prefs", ED_TYPE, 0, addpanelfunc, NULL, NULL, &pplist); /* XXX: retcode! if we didn't succeed we can't risk to overwrite them */

	/* creating all panelwins 1st makes it easier to connect subpanels later */
	ITERATELIST(ppn, &pplist)
	{
		ppn->panelwinobj = (APTR)methodstack_push_sync(app, 2, MM_Application_CreatePanelwin, ppn->prefspool);
	}



	ITERATELIST(ppn, &pplist)
	{
		i = 0;

		/* XXX: hm, perhaps we could put the number of the panel.. and open it *AFTER* the stuff has been added */
		if ( ppn->panelwinobj )//(APTR)methodstack_push_sync(app, 2, MM_Application_CreatePanelwin, ppn->prefspool)) )
		{
		/* XXX: let's see if doing this here is the best way :) XXX: we should copy the prefs!! otherwise 'path' below could go away.. hm.. or extend the structure to have some semaphores perhaps ? */
			methodstack_push_sync(ppn->panelwinobj, 3, OM_GET, MA_Panelwin_Group, &panelobj);
		//	  


			if ( (pl = prefspool_item_get(ppn->prefspool, NULL, DSI_LISTPOOL_PANEL, NULL, NULL)) )
			{    
				while ((pi = prefspool_item_get(ppn->prefspool, pl, i | DSF_LISTPOOL, NULL, NULL)) && prefspool_item_get(ppn->prefspool, pi, DSI_LISTPOOL_PANEL_TYPE, (APTR)&type, NULL))
				{
					switch (*type)
					{
						case MV_Panel_Type_Drag:
							break;

						/* classes which need special handing come first */
						case MV_Panel_Type_SubPanel:
						case MV_Panel_Type_DirPanel:
						case MV_Panel_Type_Button:
							{
								APTR o;
								if ( (o = (APTR)methodstack_push_sync(app, 5, MM_Application_CreatePanelitem, *type, panelobj, ppn->prefspool, i)) )
								{
									STRPTR path;

									methodstack_push_sync(o, 3,
										OM_GET, MA_Panel_Imagepath, &path
									);

									if (path && path[0])
									{
										icon_read(path, o,
											ICONTAG_Position, FALSE,
											ICONTAG_Deficon, TRUE,
										TAG_DONE); /* XXX */
									}
								}
		                    }
		                    break;

						case MV_Panel_Type_External:

							methodstack_push_sync(app, 5, MM_Application_CreatePanelitem, *type, panelobj, ppn->prefspool, i);
							break;
						/* other 'simple' classes come here */

						case MV_Panel_Type_Spacer:
						case MV_Panel_Type_Separator:
						case MV_Panel_Type_ViewWatcher:
						case MV_Panel_Type_Bookmarks:
							methodstack_push_sync(app, 5, MM_Application_CreatePanelitem, *type, panelobj, ppn->prefspool, i);
		                    break;

						#ifdef DEBUG
						default:
							PDB(("urgl, type out of bound: %ld\n", *type));
							break;
						#endif
					}
					i++;
				}
				if (( (issub = prefspool_item_get(ppn->prefspool, NULL, DSI_PANELGROUP_ISSUBPANEL  , (APTR)&sub, NULL)) )
				 &&(*sub))
				{
					PDB(("issub %ld %ld\n", issub,*sub));
					methodstack_push(ppn->panelwinobj, 3, MUIM_Set, MA_Panelwin_Type,MV_Panelwin_Type_SubPanel , *sub);
				}
				else
				{
					/* only root-panels get opened automaticly */
					methodstack_push(ppn->panelwinobj, 3, MUIM_Set, MUIA_Window_Open, TRUE);//, MUIA_Window_Open, TRUE);
				//	  set(ppn->panelwinobj, MUIA_Window_Open, TRUE);
				}
			}
			/* XXX */
		}
		else
		{
			PDB(("failed to create panel\n"));
		}
		/* XXX */
	}

	/* XXX: panel must also know it has been loaded, send some MA_Panelgroup_Prefspool ? hm, why should it.. not needed anymore I think */

	return (TRUE); /* XXX */
}



ULONG tr_panels_save(APTR obj UNUSED, APTR pctx) /* XXX: do we really need 'obj' ? */
{
	TEXT path[PATH_SIZE];
	struct ppnode *ppn = 0; /* shut the fuck up, gcc */
	ULONG found = FALSE;
	ULONG uid;
	STRPTR pname;
	THREAD;
	ASSERT(pctx);

	/* XXX: argh! this is all wrong! sigh */

	/*
	 * Make sure panelprefs_loadall() is finished.
	 */

	ObtainSemaphoreShared(&ppsem);

	uid = prefspool_uid(pctx);

	ITERATELIST(ppn, &pplist)
	{
		if (ppn->uid == uid)
		{
			found = TRUE;
			break;
		}
	}
	ASSERT(ppn);
	if(!(ppn->panelwinobj)) return 0;
	pname = (STRPTR)getv(ppn->panelwinobj,MA_Panelwin_Name);

	snprintf(path, sizeof(path),"%sPanels", PREFS_PATH);
	AddPart(path, ppn->n.ln_Name, sizeof(path));
	if(strncmp(ppn->n.ln_Name,pname,strlen(ppn->n.ln_Name)-6))
	{
		TEXT tpath[PATH_SIZE];
		TEXT filename[PATH_SIZE];
		TEXT backpath[PATH_SIZE];
		sprintf(filename,"%s.prefs",pname);
		snprintf(tpath, sizeof(tpath),"%sPanels", PREFS_PATH);
		AddPart(tpath, filename, sizeof(tpath));
		sprintf(backpath,"%s.bak",path);
		DeleteFile(backpath);
		Rename(path,tpath);
		strcpy(path,tpath);
	}
	/* Make sure nothing saves twice (right, async stuff can get nasty :) */
	ObtainSemaphore(&ppn->sem);
	//PDB(("name : %s %s %s %s\n",path,PREFS_PATH,ppn->n.ln_Name,pname));
	if (prefspool_write(pctx, path, PANELPREFSID, TRUE) > 0)
	{
		found = TRUE;
	}

	ReleaseSemaphore(&ppn->sem);

	ReleaseSemaphore(&ppsem);

	prefspool_delete(pctx); /* since it's a backup we can get rid of it */

	return (found);
}


ULONG tr_panels_delete(APTR obj UNUSED, APTR pctx) /* XXX: do we really need 'obj' ? no because it's gone then */
{
	TEXT path[PATH_SIZE];
	STRPTR pname;
	struct ppnode *ppn;
	ULONG found = FALSE;

	THREAD;
	ASSERT(pctx);

	ITERATELIST(ppn, &pplist)
	{
		if (ppn->uid == prefspool_uid(pctx))
		{
			found = TRUE;
			break;
		}
	}

	if (found)
	{
	    ObtainSemaphore(&ppsem);

		ObtainSemaphore(&ppn->sem);
	
	
		prefspool_delete(pctx);
		if(!(ppn->panelwinobj)) return 0;
		pname = (STRPTR)getv(ppn->panelwinobj,MA_Panelwin_Name);
		snprintf(path, sizeof(path),"%sPanels", PREFS_PATH);
		AddPart(path, ppn->n.ln_Name, sizeof(path));
		if(!(strncmp(ppn->n.ln_Name,pname,strlen(ppn->n.ln_Name)-6)))
		{
		//	  TEXT tpath[PATH_SIZE];
			TEXT filename[PATH_SIZE];
			//TEXT backpath[PATH_SIZE];
			sprintf(filename,"%s.prefs",pname);
			snprintf(path, PATH_SIZE,"%sPanels", PREFS_PATH);
			AddPart(path, filename, PATH_SIZE);
			DeleteFile(path); /* XXX */
			/* XXX: hm.. perhaps deleting the backups later would make sense ? */
			strcat(path, ".bak");
			DeleteFile(path); /* XXX */
		}
		REMOVE(ppn);
		free(ppn);

		ReleaseSemaphore(&ppsem);
	}
	else
	{
		/*
		 * That one was created but
		 * never saved to.
		 */
		prefspool_delete(pctx);
	}

	return (TRUE); /* XXX */
}


void panelprefs_fix(APTR pctx)
{
	/*
	 * Ambient used to save every piece
	 * of the frame itself. We now build
	 * a framespec out of them.
	 */
	if (!prefspool_item_get(pctx, NULL, DSI_PANELGROUP_FRAMESPEC, NULL, NULL) && prefspool_item_get(pctx, NULL, DSI_PANELGROUP_FRAMETYPE, NULL, NULL))
	{
		TEXT t[7];
		ULONG v;

		if ( (v = getprefslong_ctx(pctx, DSI_PANELGROUP_FRAMETYPE)) )
		{
			t[0] = (v < 'A') ? ('0' + v) : ('A' + v - 10);
		}
		else
		{
			t[0] = '0';
		}

		t[1] = '0';

		if ( (v = getprefslong_ctx(pctx, DSI_PANELGROUP_FRAMELEFT)) )
		{
			t[2] = '0' + v;
		}
		else
		{
			t[2] = '0';
		}

		if ( (v = getprefslong_ctx(pctx, DSI_PANELGROUP_FRAMERIGHT)) )
		{
			t[3] = '0' + v;
		}
		else
		{
			t[3] = '0';
		}

		if ( (v = getprefslong_ctx(pctx, DSI_PANELGROUP_FRAMETOP)) )
		{
			t[4] = '0' + v;
		}
		else
		{
			t[4] = '0';
		}

		if ( (v = getprefslong_ctx(pctx, DSI_PANELGROUP_FRAMEBOTTOM)) )
		{
			t[5] = '0' + v;
		}
		else
		{
			t[5] = '0';
		}

		t[6] = '\0';

		setprefsstr_ctx(pctx, DSI_PANELGROUP_FRAMESPEC, t);
	}

	if (!prefspool_item_get(pctx, NULL, DSI_PANELGROUP_IMAGESPEC, NULL, NULL) && prefspool_item_get(pctx, NULL, DSI_PANELGROUP_IMAGESPECOLD, NULL, NULL))
	{
		STRPTR t;

		if ( (t = getprefs_ctx(pctx, DSI_PANELGROUP_IMAGESPECOLD)) )
		{
			setprefsstr_ctx(pctx, DSI_PANELGROUP_IMAGESPEC, t);
		}
	}
}
#endif