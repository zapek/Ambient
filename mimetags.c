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
 * $Id: mimetags.c,v 1.4 2006/08/08 13:31:35 fab Exp $
 */

#include "ambient.h"

/* public */
#include <utility/tagitem.h>

/* private */
#include "mimetags.h"


struct TagItem *mimetags_clone(const struct TagItem *srcti)
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
			if (sti->ti_Tag & TAGF_STRING)
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


void mimetags_free(struct TagItem *srcti)
{
	struct TagItem *ti;

	ASSERT(srcti);

	ti = srcti;

	while (ti->ti_Tag != TAG_DONE)
	{
		if (ti->ti_Data)
		{
			if (ti->ti_Tag & TAGF_STRING)
			{
				free((STRPTR)ti->ti_Data);
			}
		}
		ti++;
	}
	free(srcti);
}

