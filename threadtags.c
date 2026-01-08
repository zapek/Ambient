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
 * $Id: threadtags.c,v 1.6 2006/08/08 13:31:36 fab Exp $
 */

#include "ambient.h"

/* public */
#include <utility/tagitem.h>
#include <workbench/startup.h>

/* private */
#include "threadtags.h"
#include "threads.h"


/*
 * Minimal CloneTagItems() replacement. Does not
 * support TAG_MORE or TAG_SKIP. Does also copy the
 * TT_PATH, TT_COMMENT and TT_WBARGLIST tags.
 *
 * For the moment doesn't DupLock() TT_WBARGLIST wa_Lock.
 * This is no problem as long as wa_Lock is not used for any
 * of the TT_WBARGLIST taglists. - piru
 */
struct TagItem *threadtags_clone(const struct TagItem *srcti)
{
	ULONG count = 1;
	const struct TagItem *sti;
	struct TagItem *taglist, *dti;

	ASSERT(srcti);

	sti = srcti;
	while (sti->ti_Tag != TAG_DONE)
	{
		count += 2;
		sti++;
	}

	if ( !(taglist = malloc(count * sizeof(ULONG))) )
	{
		goto error;
	}

	sti = srcti;
	dti = taglist;

	while (sti->ti_Tag != TAG_DONE)
	{
		dti->ti_Tag  = sti->ti_Tag;
		dti->ti_Data = sti->ti_Data;

		if (sti->ti_Data)
		{
			if (sti->ti_Tag & TAG_STRING)
			{
				CONST_STRPTR str = (CONST_STRPTR)sti->ti_Data;
				if ( !(dti->ti_Data = (IPTR)malloc(strlen(str) + 1)) )
				{
					/* abort at this tag */
					dti->ti_Tag = TAG_DONE;
					goto error;
				}
				strcpy((STRPTR)dti->ti_Data, str);
			}
			else if (sti->ti_Tag & TAG_WBLIST)
			{
				ULONG i = 0;
				struct WBArg *wba = (struct WBArg *)sti->ti_Data;
				struct WBArg *wbt;

				while (wba->wa_Name)
				{
					i++;
					wba++;
				}
				wba -= i;

				if ( !(dti->ti_Data = (IPTR)malloc((i + 1) * sizeof(*wba))) )
				{
					/* abort at this tag */
					dti->ti_Tag = TAG_DONE;
					goto error;
				}

				wbt = (struct WBArg *)dti->ti_Data;

				while (i--)
				{
					if ( !(wbt->wa_Name = malloc(strlen(wba->wa_Name) + 1)) )
					{
						/* note: the wbt array is NULL terminated now */
						/* abort *after* this tag */
						dti++;
						dti->ti_Tag = TAG_DONE;
						goto error;
					}
					wbt->wa_Lock = wba->wa_Lock; /* XXX: no DupLock? */
					strcpy(wbt->wa_Name, wba->wa_Name);
					wbt++;
					wba++;
				}
				wbt->wa_Lock = 0;
				wbt->wa_Name = NULL;
			}
		}

		sti++;
		dti++;
	}
	dti->ti_Tag = TAG_DONE;
	return (taglist);

	error:

	if (taglist)
	{
		threadtags_free(taglist);
	}
	return (NULL);
}


void threadtags_free(struct TagItem *srcti)
{
	struct TagItem *ti;

	ASSERT(srcti);

	ti = srcti;

	while (ti->ti_Tag != TAG_DONE)
	{
		if (ti->ti_Data)
		{
			if (ti->ti_Tag & TAG_STRING)
			{
				free((STRPTR)ti->ti_Data);
			}
			else if (ti->ti_Tag & TAG_WBLIST)
			{
				struct WBArg *wba = (struct WBArg *)ti->ti_Data;

				while (wba->wa_Name)
				{
					free(wba->wa_Name);
					wba++;
				}
				free((APTR)ti->ti_Data);
			}
		}
		ti++;
	}
	free(srcti);
}

