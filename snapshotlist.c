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
 * $Id: snapshotlist.c,v 1.6 2006/08/08 13:31:36 fab Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "snapshotlist.h"
#include "snapshot.h"


ULONG tr_snapshot_list(struct MinList *l)
{
	struct snapshotnode *sn, *nextsn;

	THREAD;
	ASSERT(l);

	ITERATELISTSAFE(sn, nextsn, l)
	{
		tr_snapshot_icon(sn->name, sn->x, sn->y);

		free(sn);
	}
	free(l);
	
	return (TRUE); /* XXX: hm.. */
}
