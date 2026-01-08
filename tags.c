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
 * $Id: tags.c,v 1.6 2006/08/08 13:31:36 fab Exp $
 */

#include "ambient.h"

/* public */
#include <utility/tagitem.h>

/* private */
#include "tags.h"


/*
 * Minimal CloneTagItems() replacement. Does not support TAG_SKIP or TAG_MORE.
 * Use this on taglists passed by someone outside of our own world and die.
 */
struct TagItem *tags_clone(const struct TagItem *srcti)
{
	ULONG count = 1;
	const struct TagItem *sti;
	struct TagItem *dti;

	ASSERT(srcti);

	sti = srcti;
	while (sti->ti_Tag != TAG_DONE)
	{
		count += 2;
		sti++;
	}

	if ( (dti = malloc(count * sizeof(ULONG))) )
	{
		memcpy(dti, srcti, count * sizeof(ULONG));
		return (dti);
	}

	return (NULL);
}


void tags_free(struct TagItem *srcti)
{
	free(srcti);
}


ULONG tags_nth_tagdata(Tag tagval, ULONG defaultval, const struct TagItem *taglist, ULONG num)
{
	const struct TagItem *ti = taglist;
	ULONG found = FALSE;

	ASSERT(taglist);
	ASSERT(tagval);
	ASSERT(num);


	fetchtag:
	num--;

	while (ti->ti_Tag)
	{
		if (ti->ti_Tag == tagval)
		{
			if (num)
			{
				/* next tag please */
				ti++;
				goto fetchtag;
			}
			found = TRUE;
			break;
		}
		ti++;
	}
	return (found ? ti->ti_Data : defaultval);
}

