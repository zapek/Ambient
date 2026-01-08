/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2007 Ambient Open Source Team
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
 * $Id: wbarg.c,v 1.9 2008/03/10 18:57:20 fab Exp $
 */

#include "ambient.h"

/* public */
#include <workbench/startup.h>
#include <proto/dos.h>

/* private */
#include "wbarg.h"
#include "mui_func.h"
#include "dragdrop.h"



struct wbargs *wba_create(APTR obj)
{
	struct wbargs *wba = NULL;

	ULONG i;

	if(GetAttr(MA_View_NumSelected, obj, &i))
	{
		if(i > 1)
		{
			/*
			 * Multiple d&d
			 */
			if ( (wba = malloc(sizeof(*wba) + sizeof(struct WBArg) * (i + 1))) )
			{
				STRPTR p;

				p = (STRPTR)getv(obj, MA_DragDrop_Path); /* XXX: argh! and how do I handle the case of multiple devices drag ? sigh */

				if ( p != NULL && (wba->basepath = malloc(strlen(p) + 1)) )
				{
					struct MinList ml;
					ULONG j = 0;
					struct dragdropnode *ddn, *nextddn;

					strcpy(wba->basepath, p);

					NEWLIST(&ml);

					DoMethod(_view(obj), MM_View_GetSelectionList, &ml);

					wba->count = 0;

					ITERATELISTSAFE(ddn, nextddn, &ml)
					{
						wba->wba[j].wa_Lock = 0;

						if(!*p)
						{
							if ( !(wba->wba[j].wa_Name = malloc(strlen(ddn->path) + 1)) )
							{
								goto error;
							}

							strcpy(wba->wba[j].wa_Name, ddn->path);
						}
						else
						{
							CONST_STRPTR fp;

							fp = FilePart(ddn->path);
							if ( !(wba->wba[j].wa_Name = malloc(strlen(fp) + 1)) )
							{
								goto error;
							}

							strcpy(wba->wba[j].wa_Name, fp);
						}
						j++;
						wba->count++;

						free(ddn);
					}

					wba->wba[j].wa_Lock = 0;
					wba->wba[j].wa_Name = NULL;
				}
				else
				{
					free(wba);
					wba = NULL;
				}
			}
		}
		else
		{
			/*
			 * Unique d&d
			 */
			if ( (wba = malloc(sizeof(*wba) + sizeof(struct WBArg) * 2)) )
			{
				STRPTR p, q;

				p = (STRPTR)getv(obj, MA_Icon_Path);

				if ( p != NULL && (wba->basepath = malloc(strlen(p) + 1)) )
				{
					strcpy(wba->basepath, p);

					q = FilePart(wba->basepath);

					if (q == wba->basepath)
					{
						/* device */
						q = NULL;
					}
					else
					{
						/* filename. cut the basepath then supply the FilePart() below */
						*q = '\0';
					}

					wba->count = 0;
					wba->wba[0].wa_Lock = 0;

					if(q)
					{
						CONST_STRPTR fp;

						fp = FilePart(p);
						if ( !(wba->wba[0].wa_Name = malloc(strlen(fp) + 1)) )
						{
							goto error;
						}

						strcpy(wba->wba[0].wa_Name, fp);
					}
					else
					{
						if ( !(wba->wba[0].wa_Name = malloc(1)) )
						{
							goto error;
						}
						
						wba->wba[0].wa_Name[0] = '\0';
					}

					wba->count = 1;
					wba->wba[1].wa_Lock = 0;
					wba->wba[1].wa_Name = NULL;
				}
				else
				{
					free(wba);
					wba = NULL;
				}
			}
		}

		return (wba);
	}

	error:
	if (wba)
	{
		wba_delete(wba);
	}
	return (NULL);
}


void wba_delete(struct wbargs *wba)
{
	int i, imax;

	ASSERT(wba);

	if (wba->basepath)
	{
		free(wba->basepath);
	}

	imax = wba->count;
	for(i = 0; i < imax; i++)
	{
		if(wba->wba[i].wa_Name)
		{
			free(wba->wba[i].wa_Name);
		}
	}

	free(wba);
}
