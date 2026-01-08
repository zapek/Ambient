/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
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
 * $Id: dosreq.c,v 1.3 2006/02/22 14:48:19 fab Exp $
 */

#include "ambient.h"

/* public */
#include <intuition/intuition.h>
#include <dos/dosextens.h>

/* private */
#include "dosreq.h"


static struct Window *w;


ULONG dosreq_init(void)
{
	w = dosreq_disable();

	return (TRUE);
}


void dosreq_cleanup(void)
{
	dosreq_enable(w);
}


/*
 * Disable DOS requesters.
 */
APTR dosreq_disable(void)
{
	struct Window *w;
	struct Process *pr;

	pr = (struct Process *)FindTask(NULL);
	ASSERT(pr);

	w = pr->pr_WindowPtr;
	pr->pr_WindowPtr = (APTR)-1;

	return ((APTR)w);
}

/*
 * Enables DOS requesters. Supply the result
 * of disable_dosreq() to it.
 */
void dosreq_enable(APTR w)
{
	struct Process *pr;

	pr = (struct Process *)FindTask(NULL);

	pr->pr_WindowPtr = (struct Window *)w;
}

