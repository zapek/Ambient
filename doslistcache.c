/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber
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
 * $Id: doslistcache.c,v 1.23 2025/08/17 17:18:57 piru Exp $
 */

#include "ambient.h"

/* public */
#include <exec/execbase.h>
#include <exec/semaphores.h>
#include <dos/dos.h>
#include <dos/filehandler.h>
#include <devices/trackdisk.h>
#include <proto/exec.h>
#include <proto/dos.h>

/* private */
#include "doslistcache.h"
#include "hash.h"
#include "device_func.h"
#include "iostdreq.h"

#define HASH_ENTRIES 32

#define SCAN_RETRIES 9

#define UNMOUNTABLES "(UMSD#[%|0-9]|ISO#[0-9]|SMBFS#[%|0-9]|SMB2FS#[%|0-9]|SSH#[0-9]|NETFS#[%|0-9]|KR#[0-9](.#[0-9]|%)|FILE#[0-9](.#[0-9]|%))"

static ULONG updatecount; /* counter to find out entries which need to be removed. grows with each update_doslistcache() */

struct dlc_htnode { /* hash table */
	struct MinNode n;
	ULONG used;
	ULONG index; /* dlc to use for inserting the entry */
	struct dlc_ptr dlc[HASH_ENTRIES];
};


struct MinList dlclist;

static struct SignalSemaphore dlcsem;
static struct MinList dlchashlist;
static APTR dlcpool;

ULONG doslistcache_init(void)
{
	if ( (dlcpool = CreatePool(MEMF_ANY, 4096, 2048)) )
	{
		InitSemaphore(&dlcsem);
		NEWLIST(&dlclist);
		NEWLIST(&dlchashlist);

		/* XXX: call updatelist */

		return (TRUE);
	}
	return (FALSE);
}


void doslistcache_cleanup(void)
{
	if (dlcpool)
	{
		DeletePool(dlcpool);
	}
}


static CONST CONST_STRPTR handler_blacklist [] = {
	"IXPIPE", /* causes problems for Zuikkis.. can't reproduce */
	"AUX", /* we have no native version and the original crashes as it's BCPL and jsr to (a5) */
	"TCP", /* can block in network access */
	NULL
};


static ULONG is_blacklisted(STRPTR s)
{
	ULONG i = 0;

	while (handler_blacklist[i])
	{
		if (!stricmp(s, handler_blacklist[i]))
		{
			D(DOSLISTCACHE,bug("found blacklisted device <%s>. Disabled.\n", s));
			return (TRUE);
		}
		i++;
	}
	return (FALSE);
}


static struct dlcnode * find_dlcentry(STRPTR name, LONG type)
{
	ULONG hash;
	ULONG i;
	struct dlc_htnode *dlch, *dlchn;

	ASSERT(name);

	/* XXX: we should upper/lower the case of name if we want to be able to find entries by name.. */

	hash = hash_nocase(name);

	for ( (dlch = FIRSTNODE(&dlchashlist)) ; (dlchn = NEXTNODE(dlch)) ; dlch = dlchn)
	{
		for (i = 0; i < HASH_ENTRIES; i++)
		{
			if (dlch->dlc[i].used)
			{
				/* remove dead nodes */
				if (dlch->dlc[i].dlcn->state == DLC_REMOVED)
				{
					D(DOSLISTCACHE,bug("removing node <%s> %p at index %lu..\n", dlch->dlc[i].dlcn->name, dlch->dlc[i].dlcn, i));

					REMOVE(dlch->dlc[i].dlcn);

					if (dlch->dlc[i].dlcn->drivername)
						FreeVecPooled(dlcpool, dlch->dlc[i].dlcn->drivername);

					FreeVecPooled(dlcpool, dlch->dlc[i].dlcn->name);
					FreePooled(dlcpool, dlch->dlc[i].dlcn, sizeof(struct dlcnode));
					
					if (dlch->used == 1)
					{
						/* remove full slot */
						D(DOSLISTCACHE,bug("removing hash slot %p..\n", dlch));
						REMOVE(dlch);
						FreePooled(dlcpool, dlch, sizeof(*dlch));
					}
					else
					{
						D(DOSLISTCACHE,bug("shrinking hash slot %p to %ld entries..\n", dlch, dlch->used - 1));
						dlch->used--;
						dlch->dlc[i].used = FALSE;
					}
					continue;
				}

				if (hash == dlch->dlc[i].h)
				{
					if (type == dlch->dlc[i].dlcn->type)
					{
						if (!stricmp(name, dlch->dlc[i].dlcn->name))
						{
							/* found a match */
							D(DOSLISTCACHE,bug("bumping <%s> updatecount: %lu\n", dlch->dlc[i].dlcn->name, updatecount));
							dlch->dlc[i].dlcn->updcnt = updatecount;
							return (dlch->dlc[i].dlcn);
						}
					}
				}
			}
		}
	}
	return (NULL);
}


/*
 * Returns a dlc_htnode and sets i to the
 * indice where to add the new entry.
 */
static struct dlc_htnode * get_dlcentry(void)
{
	struct dlc_htnode *dlch;

	if (ISLISTEMPTY(&dlchashlist))
	{
		/* list is empty, allocate a new one */
		if ( (dlch = AllocPooled(dlcpool, sizeof(*dlch))) )
		{
			memset(dlch->dlc, 0, sizeof(dlch->dlc)); /* XXX: could be optimized by skipping the first one */
			dlch->used = 1;
			dlch->index = 0;
			dlch->dlc[0].used = TRUE;
			ADDTAIL(&dlchashlist, dlch);
			return (dlch);
		}
		/* XXX: ouch.. */
	}
	else
	{
		dlch = LASTNODE(&dlchashlist);

		/* find out an empty slot */
		if (dlch->used < HASH_ENTRIES)
		{
			ULONG i;

			for (i = 0;; i++)
			{
				if (!dlch->dlc[i].used)
				{
					break;
				}
			}
			dlch->index = i;
			dlch->used++;
			dlch->dlc[i].used = TRUE;
			return (dlch);
		}
		else
		{
			/* allocate a new one, it's full */
			if ( (dlch = AllocPooled(dlcpool, sizeof(*dlch))) )
			{
				memset(dlch->dlc, 0, sizeof(dlch->dlc)); /* XXX: could be optimized by skipping the first one */
				dlch->used = 1;
				dlch->index = 0;
				dlch->dlc[0].used = TRUE;
				ADDTAIL(&dlchashlist, dlch);
				return (dlch);
			}
			/* XXX: ouch.. */
		}
	}
	return (NULL);
}

/*
 * Used by doslistcache_update() to create a dummy device node.
 */
static struct dlcnode * add_dummydevice(struct dlcnode *dlc, struct InfoData *id)
{
	struct dlcnode *dln;
	struct dlc_htnode *dlch;
	UQUAD numblocks, numblocksused;
	
	D(DOSLISTCACHE,bug("adding dummy device '%s'\n", dlc->name));

	if ( !(dln = AllocPooled(dlcpool, sizeof(*dln))) ) return NULL;

	if ( !(dln->name = AllocVecPooled(dlcpool, strlen(dlc->name)+1)) ) {
		FreePooled(dlcpool, dln, sizeof(*dln));
		return NULL;
	}	
	
	strcpy(dln->name, dlc->name);

#if !USE_LEGACY
	if ( /*!LIB_MINVER(&DOSBase->dl_lib, 51, 8) ||*/
	     !DoPkt(dlc->mp, ACTION_QUERY_ATTR, FQA_NumBlocks, (LONG) &numblocks, sizeof(numblocks), 0, 0) ||
	     !DoPkt(dlc->mp, ACTION_QUERY_ATTR, FQA_NumBlocksUsed, (LONG) &numblocksused, sizeof(numblocksused), 0, 0)) {
#endif
		numblocks = (ULONG) id->id_NumBlocks;
		numblocksused = (ULONG) id->id_NumBlocksUsed;
#if !USE_LEGACY
	}
#endif

	dln->type = DLT_DEVICE;
	dln->drivername = NULL;
	
	dln->disktype = dlc->disktype;
	dln->blocksize = id->id_BytesPerBlock;
	dln->size = numblocks * (ULONG) id->id_BytesPerBlock;
	dln->free = dln->size - (numblocksused * (ULONG) id->id_BytesPerBlock);
	dln->medium = DLC_MEDIUM_OTHER;

	dln->unitnum = 0;
	dln->unitflags = 0;
	dln->is_fs = DLC_FS_FILESYSTEM;
	dln->flags = DLF_DUMDEVICE;

	memcpy(&dln->volumedate, &dlc->volumedate, sizeof(dln->volumedate));
			
	dln->state = DLC_NEW;
	dln->mp = dlc->mp;
	dln->updcnt = updatecount;

	/* add the hash */
	
	dlch = get_dlcentry(); /* XXX: check retval */
	dlch->dlc[dlch->index].h = hash_nocase(dlc->name);
	dlch->dlc[dlch->index].dlcn = dln;
	ADDTAIL(&dlclist, dln);
	D(DOSLISTCACHE,bug("entry <%s> %p added at index num %lu\n", dln->name, dln, dlch->index));
	
	D(DOSLISTCACHE,bug("dummy device added ok\n"));
	
	return dln;
	
}

/*
 * Adds a doslist entry into the cache.
 */
static void add_dlcentry(struct DosList *dl)
{
	struct dlcnode *dln;
	STRPTR p;

	ASSERT(dl);

	/* kiero: This is some weird case when volume device is mounted but not activated. don't add it here. */

	if (dl->dol_Type == DLT_DEVICE && dl->dol_Task == NULL)
	{
		D(DOSLISTCACHE, bug("\"Incomplete\" dos entry discarded\n"));
		return;
	}

	if ( (dln = AllocPooled(dlcpool, sizeof(*dln))) )
	{
		dln->flags = 0;
		dln->size = dln->free = 0;

		p = (STRPTR)BADDR(dl->dol_Name);

		if ( (dln->name = AllocVecPooled(dlcpool, *p + 1)) ) /* I think that one is supposed to be NULL terminated but some handlers don't do it properly */
		{
			struct FileSysStartupMsg *fssm;
			struct dlc_htnode *dlch;

			stccpy(dln->name, p + 1, *p + 1);
			D(DOSLISTCACHE,bug("adding <%s>..\n", dln->name));

			dln->type = dl->dol_Type;
			dln->drivername = NULL;

			if (dln->type == DLT_DEVICE)
			{
				dln->disktype = 0; /* we find out that one at the filesystem detection stage */
			}

			if (dln->type == DLT_VOLUME)
			{
				memcpy(&dln->volumedate, &dl->dol_misc.dol_volume.dol_VolumeDate, sizeof(dln->volumedate));
				D(DOSLISTCACHE,bug("got datestamp: %ld days, %ld minutes, %ld ticks\n", dln->volumedate.ds_Days, dln->volumedate.ds_Minute, dln->volumedate.ds_Tick));
				dln->disktype = dl->dol_misc.dol_volume.dol_DiskType;
			}


			/*
			 * Try to find out if we are a filesystem.
			 * Avoid IsFileSystem() as it might be nasty with some
			 * buggy handlers. We'll resort to that only if needed and
			 * if there's a fssm.
			 */
			fssm = BADDR(dl->dol_misc.dol_handler.dol_Startup);
			if (dln->type == DLT_DEVICE && dl->dol_Task && isfssm(fssm) && !is_blacklisted(dln->name))
			{
				struct DosEnvec *denv;
				UBYTE *n;
				ULONG len;

				D(DOSLISTCACHE,bug("might be a filesystem\n"));
				dln->is_fs = DLC_FS_UNKNOWN;

				denv = (struct DosEnvec *)BADDR(fssm->fssm_Environ);
				dln->unitnum = fssm->fssm_Unit;
				dln->unitflags = fssm->fssm_Flags;

				n = ((UBYTE *)BADDR(fssm->fssm_Device));

				if (n && (len = *n))
				{
					dln->drivername = AllocVecPooled(dlcpool, len + 1);

					if (dln->drivername)
					{
						stccpy(dln->drivername, n + 1, len + 1);
					}
				}

				if (denv)
				{
					dln->blocksize = denv->de_SizeBlock * 4 * denv->de_SectorPerBlock;

					if (denv->de_TableSize > DE_DOSTYPE)
					{
						/*
						 * If the table has no such entry
						 * it'll fallback to the old way.
						 */
						dln->disktype = denv->de_DosType;
					}
					else
					{
						dln->disktype = 0; //dl->dol_misc.dol_volume.dol_DiskType; <- device dont have that
					}
				}
			}
			else
			{
				D(DOSLISTCACHE,bug("not a filesystem\n"));
				dln->is_fs = DLC_FS_OTHER;
				dln->flags = DLF_AUXILLARY;
				dln->blocksize = 0;

				/* special handling for RAM DISK */
				if (dln->type == DLT_DEVICE && !stricmp(dln->name, "RAM"))
					dln->flags = DLF_AUXILLARY | DLF_HIGHSPEED;
			}
			
			if (dln->type == DLT_DEVICE)
			{
				char buf[2 * (sizeof(UNMOUNTABLES) + 1)];
				ParsePattern(UNMOUNTABLES, buf, sizeof(buf));
				if (MatchPattern(buf, dln->name))
					dln->flags |= DLF_UNMOUNTABLE | DLF_AUXILLARY;
			}

			dln->state = DLC_NEW;
			dln->mp = dl->dol_Task;
			dln->updcnt = updatecount;

			/* add the hash */
			dlch = get_dlcentry(); /* XXX: check retval */
			dlch->dlc[dlch->index].h = hash_nocase(p + 1);
			dlch->dlc[dlch->index].dlcn = dln;
			ADDTAIL(&dlclist, dln);
			D(DOSLISTCACHE,bug("entry <%s> %p added at index num %lu\n", dln->name, dln, dlch->index));
			return;
		}
		FreePooled(dlcpool, dln, sizeof(*dln));
	}
}


#if USE_ISFILESYSTEM_TIMEOUT
static ULONG dopkt(struct DosPacket *pkt, struct MsgPort *replyport, struct MsgPort *fsport, ULONG ticks)
{
	struct Message *msg;

	SendPkt(pkt, fsport, replyport);
	while (ticks)
	{
		if (!IsMsgPortEmpty(replyport))
		{
			WaitPkt();
			return TRUE;
		}
		Delay(1);
		ticks--;
	}
	/*
	 * Here comes the trick: If the dospacket is still in the filesystem msgport
	 * remove it and simulate an error. If on the other hand the dospacket is gone it
	 * means that the filesystem is processing it and will respond to it eventually.
	 */
	Disable();
	for (msg = (struct Message *)&fsport->mp_MsgList;
	     msg->mn_Node.ln_Succ;
	     msg = (struct Message *)msg->mn_Node.ln_Succ)
	{
		if (msg == pkt->dp_Link)
		{
			REMOVE(msg);
			Enable();
			pkt->dp_Res1 = DOSFALSE;
			pkt->dp_Res2 = ERROR_LOCK_TIMEOUT;
			return TRUE;
		}
	}
	Enable();
	/*
	 * In theory it might be possible that the filesystem gets stuck in which case
	 * this call never returns. In such scenario the system is in a very bad state
	 * regardless. Another option would be to disable the WaitPkt() and return
	 * FALSE here and leak the dospacket.
	 */
	WaitPkt();
	return TRUE;
}

/*
 * This function tries to do the same as
 * IsFileSystem() but with a timeout. It appears there
 * are broken handlers which just block when you send them
 * that packet so Ambient would have no chance to scan the
 * devicelist. We don't call AbortPkt() because that function
 * could introduce a potential problem (and it does nothing
 * in the current implementation anyway :)
 */
static ULONG isfilesystem_timeout(STRPTR name, ULONG ticks)
{
	struct DevProc *dvp;
	ULONG retval = FALSE;

	ASSERT(name);
	ASSERT(ticks);

	if ( (dvp = GetDeviceProc(name, NULL)) )
	{
		struct DosPacket *pkt;

		if ( (pkt = AllocDosObject(DOS_STDPKT, NULL)) )
		{
			struct Process *me = (struct Process *)FindTask(NULL);

			pkt->dp_Type = ACTION_IS_FILESYSTEM;
			if (dopkt(pkt, &me->pr_MsgPort, dvp->dvp_Port, ticks))
			{
				if (pkt->dp_Res1 == DOSFALSE && pkt->dp_Res2 == ERROR_ACTION_NOT_KNOWN)
				{
					/*
					 * Sigh, we have to fallback to
					 * Lock("name:", ACCESS_READ);
					 */
					pkt->dp_Type = ACTION_LOCATE_OBJECT;
					pkt->dp_Arg1 = 0; /* zero lock */
					pkt->dp_Arg2 = MKBADDR("");
					pkt->dp_Arg3 = SHARED_LOCK;
					if (dopkt(pkt, &me->pr_MsgPort, dvp->dvp_Port, ticks))
					{
						if (pkt->dp_Res1)
						{
							/*
							 * Fallback worked, unlock
							 * and return success.
							 */
							pkt->dp_Type = ACTION_FREE_LOCK;
							pkt->dp_Arg1 = pkt->dp_Res1;
							if (dopkt(pkt, &me->pr_MsgPort, dvp->dvp_Port, ticks))
							{
								if (pkt->dp_Res1)
								{
									retval = TRUE;
								}
							}
							else pkt = 0;
						}
					}
					else pkt = 0;
				}
				else if (pkt->dp_Res1)
				{
					retval = TRUE;
				}
			}
			else pkt = 0;

			FreeDosObject(DOS_STDPKT, pkt);
		}
		FreeDeviceProc(dvp);
	}
	return retval;
}
#endif /* USE_ISFILESYSTEM_TIMEOUT */

/*
 * Updates the doslistcache. Called for every
 * lookup.
 */
ULONG doslistcache_update(void)
{
	struct DosList *dl;
	ULONG scanned = FALSE;
	ULONG i = 0;
	struct InfoData id;
	struct dlcnode *n;
	struct dlcnode *dlc;
	struct Process *pr;
	APTR winptr;

	D(DOSLISTCACHE,bug("ObtainSemaphore()..\n"));
	ObtainSemaphore(&dlcsem);
	D(DOSLISTCACHE,bug("got it\n"));

	while (!scanned && i < SCAN_RETRIES)
	{
		if ( (dl = AttemptLockDosList(LDF_ALL | LDF_READ)) )
		{
			
			updatecount++;

			scanned = TRUE;

			while ( (dl = NextDosEntry(dl, LDF_ALL)) )
			{
				/* XXX: remove the dln->state before, in find_dlcentry !! */
				if (!find_dlcentry((STRPTR)BADDR(dl->dol_Name) + 1, dl->dol_Type))
				{
					add_dlcentry(dl);
				}
			}
			UnLockDosList(LDF_ALL | LDF_READ);

			/* LS 2012: give lonely volumes a device to make ambient happy */
			
			ITERATELIST(dlc, &dlclist)
			{
				if ((dlc->type == DLT_VOLUME) && dlc->updcnt == updatecount) 
				{
					struct dlcnode *dln = doslistcache_find_dlcdevice(dlc->mp);
					if (!dln) 
					{
						/* this volume is in need of a dummy device */
						if (DoPkt(dlc->mp, ACTION_DISK_INFO, MKBADDR((ULONG)&id), 0, 0, 0, 0)
						   && id.id_DiskState != ID_UNREADABLE_DISK) 
						{
							dlc->medium = DLC_MEDIUM_OTHER;
							/* second, add new entry */
							n = add_dummydevice(dlc, &id);
							if (!n) dlc->state = DLC_DEAD;
						}
						else 
						{
							/* 
							** Highly inlikely, but seems volume failed filesstem test
							** Begone volume..
							*/
							D(DOSLISTCACHE,bug("dropping invalid volume '%'\n", dlc->name));
							dlc->state = DLC_DEAD;
						}		
					} 
					else if (dln->flags & DLF_DUMDEVICE) 
					{
						/* 
						** make sure dummy device has updated updcnt so it wont get 
						** removed in next stage
						*/ 
						dln->updcnt = updatecount;
					}									
				}
			}

			/*
			 * Scan and remove the unused nodes.
			 */
			pr = (struct Process *)FindTask(NULL); /* avoid silly requesters */
			winptr = pr->pr_WindowPtr;
			pr->pr_WindowPtr = (APTR)-1;

			ITERATELIST(dlc, &dlclist)
			{
				struct dlcnode *dln;

				if (dlc->updcnt != updatecount)
				{
					D(DOSLISTCACHE,bug("setting <%s> %p to state DLC_DEAD\n", dlc->name, dlc));
					dlc->state = DLC_DEAD;

					/* find volume/device pair and set it to DLC_REMOVED, so that it's removed from cache at next update */
					ITERATELIST(dln, &dlclist)
					{
						if(dlc->type == DLT_DEVICE)
						{
							if (dln->type == DLT_VOLUME && dln->mp == dlc->mp)
							{
								D(DOSLISTCACHE,bug("setting <%s> %p to state DLC_DEAD\n", dln->name, dln));
								dln->state = DLC_DEAD;
							}
						}
						else if(dlc->type == DLT_VOLUME)
						{
							if (dln->type == DLT_DEVICE && dln->mp == dlc->mp)
							{
								D(DOSLISTCACHE,bug("setting <%s> %p to state DLC_DEAD\n", dln->name, dln));
								dln->state = DLC_DEAD;
							}
						}
					}

					continue;
				}
			
				/*
				 * We need to scan only now because
				 * most handlers lock the doslist upon
				 * packet receiving and that obviously
				 * kills dos.
				 */
				if (dlc->is_fs == DLC_FS_UNKNOWN)
				{
					TEXT t[VOLUME_SIZE];

					stccpy(t, dlc->name, sizeof(t) - 1);
					strcat(t, ":");
					D(DOSLISTCACHE,bug("trying to find out if <%s> is a filesystem..\n", t));
					
					#if USE_ISFILESYSTEM_TIMEOUT
					if (isfilesystem_timeout(t, 50))
					#else
					if (IsFileSystem(t))
					#endif
					{
						D(DOSLISTCACHE,bug("it is, blocksize: %ld\n", dlc->blocksize));

						dlc->is_fs = DLC_FS_FILESYSTEM;
						dlc->medium = DLC_MEDIUM_OTHER;
						/*
						 * Find out if it's a removable.
						 */

						D(DOSLISTCACHE,bug("device %s is being tested\n", dlc->drivername));

						if (dlc->drivername)
						{
							struct IOStdReq *ioreq;

							if ((ioreq = iostd_open_device(dlc->drivername, dlc->unitnum, dlc->unitflags)))
							{
								struct DriveGeometry dg;

								memset(&dg, 0, sizeof(dg));

								ioreq->io_Command = TD_GETGEOMETRY;
								ioreq->io_Length  = sizeof(dg);
								ioreq->io_Data    = &dg;

								if (DoIO((struct IORequest *)ioreq) == 0)
								{
									if (dg.dg_SectorSize > 0 && dg.dg_TotalSectors > 0)
									{
										D(DOSLISTCACHE,bug("%s is a block media and gets fast classification\n", dlc->drivername));

										/* PowerUp users must suffer */
										if (!SysBase->MaxLocMem)
											dlc->flags = DLF_HIGHSPEED;
									}

									if (dg.dg_DeviceType == DG_CDROM)
									{
										dlc->medium = DLC_MEDIUM_CDROM;
										dlc->flags = 0;
									}

									if (dg.dg_Flags & DGF_REMOVABLE)
									{
										D(DOSLISTCACHE,bug("<%s> is a removable\n", t));
										dlc->flags = DLF_REMOVABLE | DLF_AUXILLARY;
									}
								}
								iostd_close_device(ioreq);
							}
						}

						{
							char buf[2 * (sizeof(UNMOUNTABLES) + 1)];
							ParsePattern(UNMOUNTABLES, buf, sizeof(buf));
							if (MatchPattern(buf, dlc->name))
								dlc->flags |= DLF_UNMOUNTABLE | DLF_AUXILLARY;
						}
					}
					else
					{
						D(DOSLISTCACHE,bug("is not\n"));
						dlc->is_fs = DLC_FS_OTHER;
					}
				}
 
				
				/* Update corresponding volume node, could need some optimizing */

				if (dlc->is_fs == DLC_FS_FILESYSTEM || dlc->is_fs == DLC_FS_OTHER)
				{
					if (!(dlc->flags & DLF_DUMDEVICE)) 
					{
						ITERATELIST(dln, &dlclist)
						{
							if (dln->type == DLT_VOLUME && dln->mp == dlc->mp)
							{
								dln->medium = dlc->medium;
								dln->flags = dlc->flags;
								break;
							}
						}
					}	
				}
			}

			pr->pr_WindowPtr = winptr;
		}

		if (!scanned)
		{
			D(DOSLISTCACHE,bug("list was locked.. delaying.. attempt: %ld\n", i));
			Delay(2);
		}
		i++;
	}
	
	#ifdef DEBUG
	if (!scanned)
	{
		DB(("ouch.. DosList already locked\n"));
	}
	#endif
	/* XXX: ouch.. on first try it bites.. */
	
	D(DOSLISTCACHE,bug("releasing semaphore..\n"));
	ReleaseSemaphore(&dlcsem);

	return (0);
}


struct dlcnode * doslistcache_find_dlcdevice(struct MsgPort *mp)
{
	struct dlcnode *dln;

	ASSERT(mp);

	D(DOSLISTCACHE,bug("trying to find device..\n"));

	ITERATELIST(dln, &dlclist)
	{
		if (dln->type == DLT_DEVICE && dln->mp == mp)
		{
			D(DOSLISTCACHE,bug("found at %p\n", dln));
			return (dln);
		}
	}
	D(DOSLISTCACHE,bug("not found\n"));
	return (NULL);
}


struct dlcnode * doslistcache_find_dlcvolume_by_devicename(CONST_STRPTR name)
{
	struct dlcnode *dln1, *dln2;
	STRPTR p;
	TEXT t[VOLUME_SIZE];

	ASSERT(name);

	/*
	 * Remove the trailing ':'
	 */
	stccpy(t, name, VOLUME_SIZE);

	if ( (p = strchr(t, ':')) )
	{
		*p = '\0';
	}

	if ( (dln1 = find_dlcentry(t, DLT_DEVICE)) )
	{
		ITERATELIST(dln2, &dlclist)
		{
			if ( (dln2->type == DLT_VOLUME && dln2->mp == dln1->mp) )
			{
				return (dln2);
			}
		}
	}
	return (NULL);
}


struct dlcnode * doslistcache_find_dlcdevice_by_devicename(CONST_STRPTR name)
{
	STRPTR p;
	TEXT t[VOLUME_SIZE];

	ASSERT(name);

	/*
	 * Remove the trailing ':'
	 */
	stccpy(t, name, VOLUME_SIZE);

	if ( (p = strchr(t, ':')) )
	{
		*p = '\0';
	}

	return find_dlcentry(t, DLT_DEVICE);
}


struct dlcnode * doslistcache_find_dlcdevice_by_volumename(CONST_STRPTR name)
{
	struct dlcnode *dln1, *dln2;
	STRPTR p;
	TEXT t[VOLUME_SIZE];

	ASSERT(name);

	/*
	 * Remove the trailing ':'
	 */
	stccpy(t, name, VOLUME_SIZE);

	if ( (p = strchr(t, ':')) )
	{
		*p = '\0';
	}

	if ( (dln1 = find_dlcentry(t, DLT_VOLUME)) )
	{
		ITERATELIST(dln2, &dlclist)
		{
			if (dln2->type == DLT_DEVICE && dln2->mp == dln1->mp)
			{
				return (dln2);
			}
		}
	}
	return (NULL);
}


void doslistcache_lock(void)
{
	ObtainSemaphore(&dlcsem);
}


void doslistcache_unlock(void)
{
	ReleaseSemaphore(&dlcsem);
}

ULONG doslistcache_fastdevice(CONST_STRPTR path)
{
	struct DevProc *dvp;
	ULONG rc = FALSE;

	if ((dvp = GetDeviceProc(path, NULL)))
	{
		struct dlcnode *dlc;

		while ((dvp->dvp_Flags & DVPF_ASSIGN))
			dvp = GetDeviceProc(path, dvp);

		doslistcache_lock();

		if ((dlc = doslistcache_find_dlcdevice(dvp->dvp_Port)))
		{
			rc = dlc->flags & DLF_HIGHSPEED;
		}

		doslistcache_unlock();
	}

	return (rc);
}

