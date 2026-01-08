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
 * $Id: quark.c,v 1.3 2006/02/22 14:48:22 fab Exp $
 */

#include "ambient.h"

/*
 * This is used when doing heavy debugging. Needs proper includes, etc..
 */

#if USE_DABR

/* public */
#include <public/quark/types.h>
#include <public/quark/tags.h>
#include <public/quark/task.h>
#include <public/quark/mmu.h>
#include <public/quark/id.h>
#include <public/quark/quark.h>

/* private */


u_int32_t	QGetThreadAttr(q_tid_t,
							   void*,
							   u_int32_t,
							   u_int32_t,
							   ...);

u_int32_t	QSetThreadAttr(q_tid_t,
							   void*,
							   u_int32_t,
							   u_int32_t,
							   ...);


void setdabr(APTR p)
{
	q_tid_t tid;

	if (QGetThreadAttr(QID_NULL,
		&tid,
		sizeof(q_tid_t),
		THREADATTRTAG_TID) == sizeof(q_tid_t))
	{
		u_int32_t dabr;

		if (p)
		{
			dprintf("DABR set to 0x%lx (orig: 0x%lx)\n", ((size_t)p) & ~0x7UL, (size_t)p);
			dabr = (((size_t)p) & ~0x7UL) | 0x4 | 0x2;
		}
		else
		{
			dprintf("DABR removed\n");
			dabr = 0;
		}

		QSetThreadAttr(tid,
			&dabr,
			sizeof(u_int32_t),
			THREADATTRTAG_PPC_DABR);

	}
	else
	{
		dprintf("no tid\n");
	}
}



#endif /* USE_DABR */
