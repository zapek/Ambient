/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2007 Ambient Open Source Team
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
 * $Id: subdata.c,v 1.4 2007/02/11 22:35:30 fab Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "subdata.h"
#include "mui_func.h"


struct subdata_ctx {
	ULONG type;
	STRPTR descr;
};


APTR subdata_create(void)
{
	struct subdata_ctx *ct;

	if (ct = malloc(sizeof(*ct)))
	{
		memclr(ct, sizeof(*ct));
	}
	return (ct);
}


void subdata_delete(APTR ctx)
{
	struct subdata_ctx *ct = ctx;

	ASSERT(ct);

	if (ct->descr)
	{
		free(ct->descr);
	}
	free(ct);
}


void v_subdata_setattrs(APTR ctx, struct TagItem *tags)
{
	struct subdata_ctx *ct = ctx;

	ASSERT(ct);

	FORTAG(tags)
	{
		case SUBDATA_Type:
			ct->type = tag->ti_Data;
			break;

		case SUBDATA_Description:
			{
				STRPTR s = (STRPTR)tag->ti_Data;
			
				if (ct->descr)
				{
					free(ct->descr);
					ct->descr = NULL;
				}

				if (s && *s)
				{
					if (ct->descr = malloc(strlen(s) + 1))
					{
						strcpy(ct->descr, s);
					}
				}
			}
			break;

		#ifdef DEBUG
		default:
			PDB(("unknown tag: 0x%lx\n", tag->ti_Tag));
			break;
		#endif
	}
	NEXTTAG
}


APTR subdata_getattr(APTR ctx, ULONG attr)
{
	struct subdata_ctx *ct = ctx;

	ASSERT(ct);

	switch (attr)
	{
		case SUBDATA_Type:
			return ((APTR)ct->type);

		case SUBDATA_Description:
			if (ct->descr)
			{
				return (ct->descr);
			}
			else
			{
				return ("");
			}

		#ifdef DEBUG
		default:
			PDB(("argh\n"));
			break;
		#endif
	}
	return (NULL);
}
