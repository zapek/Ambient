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
 * $Id: paneleffects.c,v 1.10 2025/09/16 15:52:29 kronos Exp $
 */

#include "ambient.h"

#if USE_INTERNAL_PANELS

/* public */
#include <proto/dos.h>
#include <proto/intuition.h>
#include <math.h>

/* private */
#include "paneltags.h"
#include "paneleffects.h"
#include "time_func.h"
#include "classes.h"
#include "qport.h"


#define TIME_TO_TAKE 100 /* 100, 500 and 1000 in ms */

ULONG tr_panels_move(APTR obj UNUSED, struct Window *win, ULONG x, ULONG y)
{
	ULONG from_x, from_y;
	LONG delta_x, delta_y;
	ULONG i;

	THREAD;
	ASSERT(win);

	from_x = win->LeftEdge;
	from_y = win->TopEdge;

	delta_x = ((LONG)x - from_x);
	delta_y = ((LONG)y - from_y);

	if (delta_x || delta_y)
	{
		struct MsgPort mp;
		struct timerequest *tr;

		CreateQPort(&mp);

		if ( (tr = timer_create(UNIT_WAITCPUCLOCK, &mp)) )
		{
			UQUAD addcpuclk;
			UQUAD lastclk;

			addcpuclk = ANIM_REFRESH_MS * cpu_clock / 1000;

			for (i = 0; i < (TIME_TO_TAKE / ANIM_REFRESH_MS); i++)
			{
				getlocalclock(&lastclk);

				ChangeWindowBox(win, from_x + (LONG)i * delta_x / (TIME_TO_TAKE / ANIM_REFRESH_MS), from_y + (LONG)i * delta_y / (TIME_TO_TAKE / ANIM_REFRESH_MS), win->Width, win->Height);

				lastclk += addcpuclk;
				timer_addclock_sync(tr, lastclk);
			}
			timer_delete(tr);
		}
		ChangeWindowBox(win, x, y, win->Width, win->Height);

		DeleteQPort(&mp);

		return (TRUE);
	}
	return (FALSE);
}


ULONG tr_panels_zip(APTR obj UNUSED, struct Window *win, ULONG xs, ULONG ys, ULONG reversed, ULONG zipspeed)
{
	ULONG from_xs, from_ys;
	LONG delta_xs, delta_ys;

	THREAD;
	ASSERT(win);
	ASSERT(xs);
	ASSERT(ys);

	if (zipspeed == MV_Panel_ZipSpeed_Instant)
	{
		zipspeed = ANIM_REFRESH_MS;
	}

	from_xs = win->Width;
	from_ys = win->Height;

	delta_xs = ((LONG)xs - from_xs);
	delta_ys = ((LONG)ys - from_ys);

	if (delta_xs || delta_ys)
	{
		struct MsgPort mp;
		struct timerequest *tr;
		double i;
		double r;
		
		CreateQPort(&mp);

		if ( (tr = timer_create(UNIT_WAITCPUCLOCK, &mp)) ) /* XXX: hm. there should be a global timer for that one.. what happens if a user starts clicking on tons of panels ? */
		{
			UQUAD addcpuclk;
			UQUAD lastclk;

			/* XXX: should detect when the resize was really too slow and use a skip then */
			addcpuclk = ANIM_REFRESH_MS * cpu_clock / 1000;

			if (reversed)
			{
				ULONG x, y;

				x = win->LeftEdge;
				y = win->TopEdge;

				for (i = -M_PI / 2; i < M_PI / 2; i	+= M_PI / (zipspeed / ANIM_REFRESH_MS))
				{
					getlocalclock(&lastclk);

					r = (sin(i) + 1.0) / 2;
					ChangeWindowBox(win, x + (LONG)(r * -delta_xs), y + (LONG)(r * -delta_ys), from_xs + (LONG)(r * delta_xs), from_ys + (LONG)(r * delta_ys));

					lastclk += addcpuclk;
					timer_addclock_sync(tr, lastclk);
				}
				
				ChangeWindowBox(win, x - delta_xs, y -delta_ys, xs, ys);
			}
			else
			{
				for (i = -M_PI / 2; i < M_PI / 2; i	+= M_PI / (zipspeed / ANIM_REFRESH_MS))
				{
					getlocalclock(&lastclk);

					r = (sin(i) + 1.0) / 2;
					ChangeWindowBox(win, win->LeftEdge, win->TopEdge, from_xs + (LONG)(r * delta_xs), from_ys + (LONG)(r * delta_ys));

					lastclk += addcpuclk;
					timer_addclock_sync(tr, lastclk);
				}
			
				ChangeWindowBox(win, win->LeftEdge, win->TopEdge, xs, ys);
			}
			timer_delete(tr);

			DeleteQPort(&mp);
			return (TRUE);
		}
		#ifdef DEBUG
		else
		{
			PDB(("Couldn't create timer\n"));
		}
		#endif

		DeleteQPort(&mp);
	}
	return (FALSE);
}
#endif
