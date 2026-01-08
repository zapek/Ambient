/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2016 Ambient Open Source Team
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
 * $Id: notify.c,v 1.13 2017/08/21 06:17:44 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <exec/semaphores.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stddef.h> /* for offsetof() */

/* private */
#include "notify.h"
#include "mui_func.h"
#include "methodstack.h"
#include "name.h"


struct notify_action_node {
	struct MinNode n;
	ULONG action;
	ULONG mode; /* TRUE/FALSE/UNRELIABLE */
};

struct notify_path_node {
	struct MinNode n;
	ULONG type; /* device or file */
	struct MinList na; /* contains notify_action_nodes */
	ULONG depth;       /* depth of the pattern */
	TEXT pattern[0];
};

struct notify_regged_node {
	struct MinNode n;
	struct notifyact *nact;
};

struct notify_ctx {
	struct MinNode n;
	struct SignalSemaphore sem;
	struct MinList nl; /* contains notify_path_nodes */
	struct MinList rl; /* contains notify_regged_node */
	APTR obj; /* object to notify */
	struct Hook hook;
	struct notify_regged_node *last_rn;
};


static ULONG init_done;
static struct MinList ctxlist;

ULONG notify_init(void)
{
	NEWLIST(&ctxlist);
	init_done = TRUE;

	return (TRUE);
}


void notify_cleanup(void)
{
	#ifdef DEBUG
	if (init_done && !ISLISTEMPTY(&ctxlist))
	{
		PDB(("still notify jobs left..\n"));
	}
	#endif
}


APTR notify_create(void)
{
	struct notify_ctx *ct;

	if ( (ct = malloc(sizeof(*ct))) )
	{
		ct->obj = NULL;
		memset(&ct->hook, 0, sizeof(struct Hook));
		ct->last_rn = NULL;

		InitSemaphore(&ct->sem);
		NEWLIST(&ct->nl);
		NEWLIST(&ct->rl);
		ADDTAIL(&ctxlist, ct);
	}
	return (ct);
}


/*
 * If 'type' is NULL it removes everything
 * If 'path' is NULL it removes all the path of the defined 'type'
 * If 'action' is NULL it removes all the actions from the path, then the path itself.
 */
static void ln_remove_action(struct notify_ctx *ct, ULONG type, STRPTR path, ULONG action)
{
	struct notify_path_node *np, *nnp;
	struct notify_action_node *na, *nextna;

	ASSERT(ct);

	for ( (np = FIRSTNODE(&ct->nl)) ; (nnp = NEXTNODE(np)) ; (np = nnp) )
	{
		if ((np->type == type) || !type)
		{
			if (path)
			{
				ASSERT(np->pattern);
				if (MatchPatternNoCase(np->pattern, path))
				{
					if (action)
					{
						/* found */
						ITERATELIST(na, &np->na)
						{
							if (na->action == action)
							{
								REMOVE(na);
								break;
							}
						}
						break;
					}
					else
					{
						ITERATELISTSAFE(na, nextna, &np->na)
						{
							free(na);
						}
						REMOVE(np);
						free(np);
					}
				}
			}
			else
			{
				ITERATELISTSAFE(na, nextna, &np->na)
				{
					free(na);
				}
				REMOVE(np);
				free(np);
			}
		}
	}
}


void notify_delete(APTR ctx)
{
	struct notify_ctx *ct = ctx;
	struct notify_regged_node *nrn, *nextnrn;

	ASSERT(ct);

	ObtainSemaphore(&ct->sem);

	ln_remove_action(ct, (ULONG)NULL, NULL, (ULONG)NULL);

	ITERATELISTSAFE(nrn, nextnrn, &ct->rl)
	{
		ASSERT(nrn->nact);
		free(nrn->nact);
		free(nrn);
	}

	if (ct->last_rn)
	{
		free(ct->last_rn->nact);
		free(ct->last_rn);
	}

	ReleaseSemaphore(&ct->sem); /* XXX: beware of that one.. */

	REMOVE(ct);
	free(ct);
}


ULONG v_notify_register(APTR ctx, struct TagItem *tags)
{
	struct notify_ctx *ct = ctx;
	struct notify_path_node *cnp = NULL; /* current notify path */
	ASSERT(ct);

	ObtainSemaphore(&ct->sem);

	FORTAG(tags)
	{
		case NOTIFYTAG_Monitor_File:
		case NOTIFYTAG_Monitor_Device:
			{
				CONST_STRPTR name;
				ASSERT(tag->ti_Data);

				name = (CONST_STRPTR)tag->ti_Data;

				if (name && *name)
				{
					struct notify_path_node *np;
					ULONG len = strlen(name) * 2 + 4;

					if ((np = malloc(sizeof(*np) + len)))
					{
						if (ParsePatternNoCase( name, np->pattern, len))
						{
							CONST_STRPTR p;
							ULONG depth;

							np->type = tag->ti_Tag;

							depth = 0;
							p = name;
							while (*p && *(p + 1) && *(p + 2) && (p = strchr(p + 1, '/')))
							{
								depth++;
							}
							np->depth = depth;

							/*
							 * Special case for shortcuts on root window.
							 */

							if (name[0] == '*' && name[1] == '\0')
								np->depth = 999;

							NEWLIST(&np->na);
							ADDTAIL(&ct->nl, np);
							cnp = np;
						}
						else
						{
							/* Should not get here. */
							free(np);
						}
					}
				}
				/* XXX */
			}
			break;

		case NOTIFYTAG_Monitor_File_Create:
		case NOTIFYTAG_Monitor_File_Delete:
		case NOTIFYTAG_Monitor_File_Name:
		case NOTIFYTAG_Monitor_File_Date:
		case NOTIFYTAG_Monitor_File_Comment:
		case NOTIFYTAG_Monitor_File_Size:
		case NOTIFYTAG_Monitor_File_Flags:
		case NOTIFYTAG_Monitor_File_UID:
		case NOTIFYTAG_Monitor_File_GID:
		case NOTIFYTAG_Monitor_File_Icon:
		case NOTIFYTAG_Monitor_Device_Mount:
		case NOTIFYTAG_Monitor_Device_UnMount:
		case NOTIFYTAG_Monitor_Device_Name:
		case NOTIFYTAG_Monitor_Enable:
			{
				struct notify_action_node *na;
				ASSERT(cnp);

				if ( (na = malloc(sizeof(*na))) )
				{
					na->action = tag->ti_Tag;
					na->mode = tag->ti_Data;
					ADDTAIL(&cnp->na, na);
				}
			}
			break;

		case NOTIFYTAG_Inform_Object:
			{
				ct->obj = (APTR)tag->ti_Data;
			}
			break;

		case NOTIFYTAG_Inform_Hook:
			{
				memcpy(&ct->hook, (UBYTE *)tag->ti_Data, sizeof(struct Hook));
			}
			break;

		#ifdef DEBUG
		default:
			PDB(("unknown tag 0x%lx\n", tag->ti_Tag));
			break;
		#endif
	}
	NEXTTAG

	ReleaseSemaphore(&ct->sem);

	return (TRUE); /* XXX */
}


void v_notify_unregister(APTR ctx, struct TagItem *tags)
{
	struct notify_ctx *ct = ctx;
	ULONG ctype = ctype; /* shut up gcc */
	STRPTR cpath = NULL;
	ULONG tagcnt = 0;
	ASSERT(ct);

	ObtainSemaphore(&ct->sem);

	FORTAG(tags)
	{
		case NOTIFYTAG_Monitor_File:
		case NOTIFYTAG_Monitor_Device:
			{
				if (!tagcnt && cpath)
				{
					/* clear the previous path then */
					ln_remove_action(ct, ctype, cpath, (ULONG)NULL);
					cpath = NULL;
				}
				tagcnt = 0;

				if (tag->ti_Data == NOTIFYVAL_Monitor_File_ClearAll) /* works with NOTIFYVAL_Monitor_Device_ClearAll too */
				{
					ln_remove_action(ct, tag->ti_Tag, NULL, (ULONG)NULL);
				}
				else
				{
					ctype = tag->ti_Tag;
					cpath = (STRPTR)tag->ti_Data;
				}
			}
			break;

		case NOTIFYTAG_Monitor_File_Create:
		case NOTIFYTAG_Monitor_File_Delete:
		case NOTIFYTAG_Monitor_File_Name:
		case NOTIFYTAG_Monitor_File_Date:
		case NOTIFYTAG_Monitor_File_Comment:
		case NOTIFYTAG_Monitor_File_Size:
		case NOTIFYTAG_Monitor_File_Flags:
		case NOTIFYTAG_Monitor_File_UID:
		case NOTIFYTAG_Monitor_File_GID:
		case NOTIFYTAG_Monitor_File_Icon:
		case NOTIFYTAG_Monitor_Device_Mount:
		case NOTIFYTAG_Monitor_Device_UnMount:
		case NOTIFYTAG_Monitor_Device_Name:
		case NOTIFYTAG_Monitor_Enable:
			{
				tagcnt++;

				ln_remove_action(ct, ctype, cpath, tag->ti_Tag);
			}
			break;

		case NOTIFYTAG_Inform_Object:
			{
				ct->obj = NULL; /* XXX: check previous obj? well, there's only one object for now.. */
			}
			break;

		case NOTIFYTAG_Inform_Hook:
			{
				memset(&ct->hook, 0, sizeof(struct Hook));
			}
			break;

		#ifdef DEBUG
		default:
			PDB(("unknown tag 0x%lx\n", tag->ti_Tag));
			break;
		#endif

	}
	NEXTTAG

	if (!tagcnt && cpath)
	{
		ln_remove_action(ct, ctype, cpath, (ULONG)NULL);
	}

	ReleaseSemaphore(&ct->sem);
}


#ifdef DEBUG
#define ACTIONSTR(x) \
	case x: \
	return (#x);

static STRPTR get_actionstr(ULONG action)
{
	switch (action)
	{
		ACTIONSTR(NOTIFYTAG_Monitor_File_Create)
		ACTIONSTR(NOTIFYTAG_Monitor_File_Delete)
		ACTIONSTR(NOTIFYTAG_Monitor_File_Name)
		ACTIONSTR(NOTIFYTAG_Monitor_File_Date)
		ACTIONSTR(NOTIFYTAG_Monitor_File_Comment)
		ACTIONSTR(NOTIFYTAG_Monitor_File_Size)
		ACTIONSTR(NOTIFYTAG_Monitor_File_Flags)
		ACTIONSTR(NOTIFYTAG_Monitor_File_UID)
		ACTIONSTR(NOTIFYTAG_Monitor_File_Icon)
		ACTIONSTR(NOTIFYTAG_Monitor_Device_Mount)
		ACTIONSTR(NOTIFYTAG_Monitor_Device_UnMount)
		ACTIONSTR(NOTIFYTAG_Monitor_Device_Name)
		ACTIONSTR(NOTIFYTAG_Monitor_Enable)
		default:
			return ((STRPTR)"unknown");
	}
}
#endif

#define SETACT nrn->nact = (struct notifyact *)nal; \
	nal->action = action; \
	nal->uri = (STRPTR)(nal + 1); \
	strcpy(nal->uri, path)

void notify_action(CONST_STRPTR path, ULONG type, ...)
{
	struct notify_ctx *ct;
	struct notify_path_node *np;
	struct notify_action_node *na;
	ULONG depth;

	if (path && *path)
	{
		CONST_STRPTR p = path;
		depth = 0;

		while (*p && *(p + 1) && *(p + 2) && (p = strchr(p + 1, '/')))
		{
			depth++;
		}

		ITERATELIST(ct, &ctxlist)
		{
			ULONG action;
			va_list va;

			/* We need to reinitialize it here to restore arg 'cursor' */

			va_start(va, type);

			action = va_arg(va, ULONG);

			ObtainSemaphore(&ct->sem); /* XXX: beware of deadlocks.. that methodstack_push() at the end can be nasty.. most probably is.. put those obtain/release in more intelligent places! and check the list after obtaining the semaphore */

			ITERATELIST(np, &ct->nl)
			{
				if (np->type == type)
				{
					if (depth <= np->depth)
					{
						if (MatchPatternNoCase(np->pattern, (STRPTR)path))
						{
							ITERATELIST(na, &np->na)
							{
								if (na->action == action && na->mode != FALSE)
								{
									struct notify_regged_node *nrn;

									D(NOTIFY,bug("match for action <%s>, storing action for path <%s>\n", get_actionstr(action), path));

									if ( (nrn = malloc(sizeof(*nrn))) )
									{
										ULONG pathlen, rc = FALSE;

										pathlen = strlen(path) + 1;

										switch (action)
										{
											case NOTIFYTAG_Monitor_File_Create:
											case NOTIFYTAG_Monitor_File_Delete:
												{
													struct notifyact_file_create *nal;

													if ( (nal = malloc(sizeof(*nal) + pathlen)) )
													{
														SETACT;

														ADDTAIL(&ct->rl, nrn);
														rc = TRUE;
													}
													/* XXX */
												}
												break;

											case NOTIFYTAG_Monitor_File_Icon:
												{
													struct notifyact_file_icon *nal;

													if ( (nal = malloc(sizeof(*nal) + pathlen)) )
													{
														SETACT;

														ADDTAIL(&ct->rl, nrn);
														rc = TRUE;
													}
													/* XXX */
												}
												break;

											case NOTIFYTAG_Monitor_Device_Mount:
											case NOTIFYTAG_Monitor_Device_UnMount:
												{
													struct notifyact_device_mount *nal;

													if ( (nal = malloc(sizeof(*nal) + pathlen)) )
													{
														SETACT;

														ADDTAIL(&ct->rl, nrn);
														rc = TRUE;
													}
													/* XXX */
												}
												break;

											case NOTIFYTAG_Monitor_File_Name:
												{
													struct notifyact_file_name *nal;
													STRPTR p;

													p = va_arg(va, STRPTR);

													if ( (nal = malloc(sizeof(*nal) + pathlen + strlen(p) + 1)) )
													{
														SETACT;

														nal->newname = (STRPTR)((UBYTE *)(nal + 1) + pathlen);
														strcpy(nal->newname, p);

														ADDTAIL(&ct->rl, nrn);
														rc = TRUE;
													}
													/* XXX */
												}
												break;

											case NOTIFYTAG_Monitor_Enable:
												{
													struct notifyact_file_enable *nal;
													ULONG b;

													b = va_arg(va, ULONG);

													if ( (nal = malloc(sizeof(*nal) + pathlen + sizeof(ULONG))) )
													{
														SETACT;

														nal->enable = (ULONG) b;

														ADDTAIL(&ct->rl, nrn);
														rc = TRUE;
													}
													/* XXX */
												}
												break;

											case NOTIFYTAG_Monitor_Device_Name:
												{
													struct notifyact_device_name *nal;
													STRPTR p;

													p = va_arg(va, STRPTR);

													if ( (nal = malloc(sizeof(*nal) + pathlen + strlen(p) + 1)) )
													{
														SETACT;

														nal->newname = (STRPTR)((UBYTE *)(nal + 1) + pathlen);
														strcpy(nal->newname, p);

														ADDTAIL(&ct->rl, nrn);
														rc = TRUE;
													}
													/* XXX */
												}
												break;

											case NOTIFYTAG_Monitor_File_Comment:
												{
													struct notifyact_file_comment *nal;
													STRPTR p;

													p = va_arg(va, STRPTR);

													if ( (nal = malloc(sizeof(*nal) + pathlen + strlen(p) + 1)) )
													{
														SETACT;

														nal->comment = (STRPTR)((UBYTE *)(nal + 1) + pathlen);
														strcpy(nal->comment, p);

														ADDTAIL(&ct->rl, nrn);
														rc = TRUE;
													}
													/* XXX */
												}
												break;

											case NOTIFYTAG_Monitor_File_Size:
												{
													struct notifyact_file_size *nal;
													UQUAD p;

													p = va_arg(va, UQUAD);

													if ( (nal = malloc(sizeof(*nal) + pathlen + sizeof(UQUAD))) )
													{
														SETACT;

														nal->size = p;

														ADDTAIL(&ct->rl, nrn);
														rc = TRUE;
													}
													/* XXX */
												}
												break;

											case NOTIFYTAG_Monitor_File_Flags:
												{
													struct notifyact_file_flags *nal;
													ULONG p;

													p = va_arg(va, ULONG);

													if ( (nal = malloc(sizeof(*nal) + pathlen + sizeof(ULONG))) )
													{
														SETACT;

														nal->flags = p;

														ADDTAIL(&ct->rl, nrn);
														rc = TRUE;
													}
													/* XXX */
												}
												break;

											case NOTIFYTAG_Monitor_File_Date:
												{
													struct notifyact_file_date *nal;
													struct DateStamp *p = va_arg(va, struct DateStamp *);

													if ( (nal = malloc(sizeof(*nal) + pathlen + sizeof(ULONG))) )
													{
														SETACT;

														nal->datestamp = *p;

														ADDTAIL(&ct->rl, nrn);
														rc = TRUE;
													}

													/* XXX: finish that date field-o-crap */
												}
												break;

											#ifdef DEBUG
											default:
												PDB(("unknown mode\n"));
												break;
											#endif
										}


										/*
										 * Send the notification.
										 */
										if (rc)
										{
											if (ct->obj)
											{
												D(NOTIFY,bug("sending MM_Notify_Change to object %p\n", ct->obj));
												methodstack_push_sync(ct->obj, 2, MM_Notify_Change, ct); /* has to be synced or the context could go away */
											}

											if (ct->hook.h_Entry || ct->hook.h_SubEntry)
											{
												CallHookPkt(&ct->hook, ct, NULL);
											}
										}

									}
									/* XXX */
								}
							}
						}
					}
				}
			}
			ReleaseSemaphore(&ct->sem);

			va_end(va);
		}
	}
}


/*
 * We don't need a semaphore because that function can only be called within
 * MM_Notify_Change and the semaphore is held there.
 */
struct notifyact * notify_action_get(APTR ctx)
{
	struct notify_ctx *ct = ctx;
	struct notify_regged_node *rn;
	struct  notifyact *nact;

	ASSERT(ct);

	if ( (rn = (struct notify_regged_node *)REMHEAD(&ct->rl)) )
	{
		nact = rn->nact;
	}
	else
	{
		nact = NULL;
	}

	if (ct->last_rn)
	{
		free(ct->last_rn->nact);
		free(ct->last_rn);
	}
	ct->last_rn = rn;

	return (nact);
}

