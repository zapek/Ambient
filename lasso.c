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
 * $Id: lasso.c,v 1.10 2013/10/28 12:04:05 geit Exp $
 */

/* In fact this cant be disabled (this source file wont build) but leaving this
** flag gives an idea how things work.
*/
#define USE_SMARTLASSO 1

#include "ambient.h"

/* public */
#include <graphics/gfx.h>
#include <graphics/rpattr.h>
#include <graphics/gfxmacros.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
/* private */
#include "lasso.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"

struct lasso_ctx {
	struct RastPort *rp;
	ULONG color;
	LONG x;           /* starting point */
	LONG y;
	LONG minx;        /* minimum boundaries */
	LONG miny;
	LONG maxx;        /* maximum boundaries */
	LONG maxy;
	ULONG ARGB;
	ULONG vx;
	ULONG vy;
	ULONG x0[4];
	ULONG x1[4];
	ULONG y0[4];
	ULONG y1[4];
	#if USE_SMARTLASSO
	ULONG s0[4];   /* lowest in the buffer */
	ULONG s1[4];   /* highest in the buffer */
	#endif
	ULONG has_backup;
	APTR bm[4];
};

#define TYPE_INNER_HORIZ 0
#define TYPE_OUTER_HORIZ 1
#define TYPE_INNER_VERT 2
#define TYPE_OUTER_VERT 3


/**********************************************************************
	lasso_create()

	x,y  - click pos
	minx - area start x
	miny - area start y
	maxx - area end x
	maxy - area end y
	vx   - virtgroup x
	vy   - virtgroup y

	If not used in a virtgroup always pass vx=0, vy=0 everywhere.

	If inside virtgroup then lasso_render() takes mouse x/y coordinates
	with vx,vy already added to them.
**********************************************************************/

void lasso_pixel(APTR p)
{
	struct lasso_ctx *ct = (struct lasso_ctx*)p;
	
	WriteRGBPixel(ct->rp, ct->x, ct->y, ct->ARGB);
}

APTR lasso_create(struct RastPort *rp, LONG x, LONG y, LONG minx, LONG miny, LONG maxx, LONG maxy, LONG vx, LONG vy, struct BitMap *fri, ULONG color)
{
	struct lasso_ctx *ct;

	ASSERT(rp);
	ASSERT(maxx);
	ASSERT(maxy);
	ASSERT(fri);

	#ifdef DEBUG
	if (x < minx || y < miny || x > maxx || y > maxy)
	{
		PDB(("wrong dimensions: x: %ld, y: %ld, minx: %ld, miny: %ld, maxx: %ld, maxy: %ld\n", x, y, minx, miny, maxx, maxy));
		return (NULL);
	}
	#endif

	if ((ct = malloc(sizeof(*ct))))
	{
		ULONG i;

		memset(ct, 0, sizeof(*ct)); /* let's be lazy */

		x += vx;
		y += vy;

		ct->rp = rp;
		ct->color = color;
		ct->minx = minx;
		ct->miny = miny;
		ct->maxx = maxx;
		ct->maxy = maxy;
		ct->x = x - ct->minx;
		ct->y = y - ct->miny;

		ct->vx = vx;
		ct->vy = vy;

		ct->ARGB = ReadRGBPixel(rp, ct->x, ct->y);

		// create backup buffers for x lines
		for (i = 0; i < 2; i++)
		{
			ct->bm[i] = gfx_bitmap_create(maxx - minx + 1, 1, BITMAPDEPTH_Clone, BITMAPTAG_VMem, TRUE, BITMAPTAG_ExtFriend, fri, TAG_DONE); /* XXX: +1 is for safety.. remove then */
			ct->s0[i] = ct->s1[i] = ct->x - vx;
		}

		// create backup buffers for y lines

		for (i = 2; i < 4; i++)
		{
			ct->bm[i] = gfx_bitmap_create(1, maxy - miny + 1, BITMAPDEPTH_Clone, BITMAPTAG_VMem, TRUE, BITMAPTAG_ExtFriend, fri, TAG_DONE); /* XXX: +1 is for safety.. remove then */
			ct->s0[i] = ct->s1[i] =  ct->y - vy;
		}

		if (!ct->bm[0] || !ct->bm[1] || !ct->bm[2] || !ct->bm[3])
		{
			lasso_delete(ct);
			return (NULL);
		}
	}
	return (ct);
}


void lasso_delete(APTR ctx)
{
	struct lasso_ctx *ct;
	ULONG i;

	ct = (struct lasso_ctx *)ctx;
	ASSERT(ct);

	for (i = 0; i < 4; i++)
	{
		gfx_bitmap_delete(ct->bm[i]);
	}
	free(ctx);
}

#define isneg(x) (0x80000000 & (x))

/*
 * Puts the small var in v1 and the big one in v2
 */
static void adjust_coordinates(LONG *v1, LONG *v2, ULONG max UNUSED)
{
#if 0
	if (*v2 < *v1)
	{
		*v1 ^= *v2;
		*v2 ^= *v1;
		*v1 ^= *v2;
	}
#else
	if (*v2 < *v1)
	{
		LONG t = *v2;
		*v2 = *v1;
		*v1 = t;
	}
#endif

#if 0
	// -itix

	if (*v2 < 0)
	{
		*v2 = 0;
	}
	else if (*v2 > max)
	{
		*v2 = max;
	}

	if (*v1 < 0)
	{
		*v1 = 0;
	}
	else if (*v1 > max)
	{
		*v1 = max;
	}
#endif
}


#if USE_SMARTLASSO
static void bblit(struct lasso_ctx *ct, ULONG type, ULONG perp, ULONG s0, ULONG s1)
{
	struct RastPort rp;
	InitRastPort(&rp);
	rp.BitMap = gfx_bitmap_bm(ct->bm[type]);

	if (s0 == s1)
	{
		return;
	}

	switch (type)
	{
		case TYPE_INNER_HORIZ:
		case TYPE_OUTER_HORIZ:
			if (perp == ct->y0[type])
			{
				/* partial backup */
				if (s0 < ct->s0[type]) /* left */
				{
					ClipBlit(ct->rp, ct->minx + s0, ct->miny + perp, &rp, s0, 0, ct->s0[type] - s0, 1, 0xc0);
					ct->s0[type] = s0;
				}
				else if (s1 > ct->s1[type])
				{
					ClipBlit(ct->rp, ct->minx + ct->s1[type] + 1, ct->miny + perp, &rp, ct->s1[type] + 1, 0, s1 - ct->s1[type], 1, 0xc0);
					ct->s1[type] = s1;
				}
			}
			else
			{
				/* new backup */
				ClipBlit(ct->rp, ct->minx + s0, ct->miny + perp, &rp, s0, 0, s1 - s0 + 1, 1, 0xc0);
				ct->s0[type] = s0;
				ct->s1[type] = s1;
			}
			break;

		case TYPE_INNER_VERT:
		case TYPE_OUTER_VERT:
			if (perp == ct->x0[type])
			{
				/* partial backup (expand to up) */
				if (s0 < ct->s0[type]) /* up */
				{
					ClipBlit(ct->rp, ct->minx + perp, ct->miny + s0, &rp, 0, s0, 1, ct->s0[type] - s0, 0xc0);
					ct->s0[type] = s0;
				}
				else if (s1 > ct->s1[type])
				{
					ClipBlit(ct->rp, ct->minx + perp, ct->miny + ct->s1[type] + 1, &rp, 0, ct->s1[type] + 1, 1, s1 - ct->s1[type], 0xc0);
					ct->s1[type] = s1;
				}
			}
			else
			{
				ClipBlit(ct->rp, ct->minx + perp, ct->miny + s0, &rp, 0, s0, 1, s1 - s0 + 1, 0xc0);
				ct->s0[type] = s0;
				ct->s1[type] = s1;
			}
			break;
	}
}
#else
static void bblit(struct lasso_ctx *ct, ULONG type, ULONG perp, ULONG s0, ULONG s1)
{
	struct RastPort rp;
	InitRastPort(&rp);
	rp.BitMap = gfx_bitmap_bm(ct->bm[type]);

	if (s0 == s1)
	{
		return;
	}

	switch (type)
	{
		case TYPE_INNER_HORIZ:
		case TYPE_OUTER_HORIZ:
			ClipBlit(ct->rp, ct->minx + s0, ct->miny + perp, &rp, s0, 0, s1 - s0 + 1, 1, 0xc0);
			break;

		case TYPE_INNER_VERT:
		case TYPE_OUTER_VERT:
			ClipBlit(ct->rp, ct->minx + perp, ct->miny + s0, &rp, 0, s0, 1, s1 - s0 + 1, 0xc0);
			break;
	}
}
#endif

static void rblit(struct lasso_ctx *ct, ULONG type, LONG perp, LONG s0, LONG s1)
{
	if (s0 == s1)
	{
		return;
	}

	switch (type)
	{
		case TYPE_INNER_HORIZ:
		case TYPE_OUTER_HORIZ:
			gfx_blit(ct->bm[type], ct->rp,
				BLITTAG_DstType, BLITVAL_DstType_RastPort,
				BLITTAG_SrcX, s0,
				BLITTAG_DstX, ct->minx + s0,
				BLITTAG_DstY, ct->miny + perp,
				BLITTAG_DstWidth, s1 - s0 + 1,
			TAG_DONE);
			break;

		case TYPE_INNER_VERT:
		case TYPE_OUTER_VERT:
			gfx_blit(ct->bm[type], ct->rp,
				BLITTAG_DstType, BLITVAL_DstType_RastPort,
				BLITTAG_SrcY, s0,
				BLITTAG_DstX, ct->minx + perp,
				BLITTAG_DstY, ct->miny + s0,
				BLITTAG_DstHeight, s1 - s0 + 1,
			TAG_DONE);
			break;
	}
}

static void restore_line(struct lasso_ctx *ct, ULONG type, ULONG perp, ULONG s0, ULONG s1)
{
	/* XXX: hm.. I'm not sure.. should we substract -1 here ? (and add). I think so but it doesn't seem to work */

	switch (type)
	{
		case TYPE_INNER_HORIZ:
		case TYPE_OUTER_HORIZ:
			if (perp == ct->y0[type])
			{
				/* partial refresh */
				if (s0 > ct->x0[type]) /* left */
				{
					/* XXX: I think I have to substract and add -1 here.. not sure */
					rblit(ct, type, ct->y0[type], ct->x0[type], s0);
				}
				else if (s1 < ct->x1[type]) /* right */
				{
					rblit(ct, type, ct->y0[type], s1, ct->x1[type]);
				}
			}
			else
			{
				/* full refresh */
				rblit(ct, type, ct->y0[type], ct->x0[type], ct->x1[type]);
			}
			break;

		case TYPE_INNER_VERT:
		case TYPE_OUTER_VERT:
			if (perp == ct->x0[type])
			{
				/* partial refresh */
				if (s0 > ct->y0[type]) /* up */
				{
					rblit(ct, type, ct->x0[type], ct->y0[type], s0);
				}
				else if (s1 < ct->y1[type]) /* down */
				{
					rblit(ct, type, ct->x0[type], s1, ct->y1[type]);
				}
			}
			else
			{
				/* full refresh (XXX: hm, when ct->y0[type] == ct->y1[type].. does it fuck up anything ? */
				rblit(ct, type, ct->x0[type], ct->y0[type], ct->y1[type]);
			}
			break;
	}
}


static void restore_lines(struct lasso_ctx *ct)
{
	ULONG type;

	if (ct->has_backup)
	{
		for (type = 0; type < 4; type++)
		{
			#if USE_SMARTLASSO
			if (type == TYPE_OUTER_HORIZ && (ct->y0[type] == ct->y0[TYPE_INNER_HORIZ])) /* same line cases, they must not grab already marked areas */
			{
				continue;
			}
			else if (type == TYPE_OUTER_VERT && (ct->x0[type] == ct->x0[TYPE_INNER_VERT]))
			{
				continue;
			}
			#endif
			/*
			 * Restore.
			 */
			switch (type)
			{
				case TYPE_INNER_HORIZ:
				case TYPE_OUTER_HORIZ:
					rblit(ct, type, ct->y0[type], ct->x0[type], ct->x1[type]);
					break;

				case TYPE_INNER_VERT:
				case TYPE_OUTER_VERT:
					rblit(ct, type, ct->x0[type], ct->y0[type], ct->y1[type]);
					break;
			}
		}
	}
}


static void draw_lines(struct lasso_ctx *ct, LONG x, LONG y)
{
	LONG x0[4], y0[4], x1[4], y1[4]; /* x0 < x1 and y0 < y1 */
	ULONG max_x = ct->maxx - ct->minx;
	ULONG max_y = ct->maxy - ct->miny;
	ULONG perp[4];
	ULONG s0[4], s1[4];
	ULONG type;

	for (type = 0; type < 4; type++)
	{
		switch (type)
		{
			case TYPE_INNER_HORIZ:
				x0[type] = ct->x;
				x1[type] = x;
				adjust_coordinates(&x0[type], &x1[type], max_x);
				y0[type] = y1[type] = ct->y - ct->vy;
				adjust_coordinates(&y0[type], &y1[type], max_y);
				perp[type] = y0[type];
				s0[type] = x0[type];
				s1[type] = x1[type];
				break;

			case TYPE_OUTER_HORIZ:
				x0[type] = ct->x;
				x1[type] = x;
				adjust_coordinates(&x0[type], &x1[type], max_x);
				y0[type] = y1[type] = y - ct->vy;
				adjust_coordinates(&y0[type], &y1[type], max_y);
				perp[type] = y0[type];
				s0[type] = x0[type];
				s1[type] = x1[type];
				break;

			case TYPE_INNER_VERT:
				x0[type] = x1[type] = ct->x;
				adjust_coordinates(&x0[type], &x1[type], max_x);
				y0[type] = ct->y - ct->vy;
				y1[type] = y - ct->vy;

				if (y0[type] < 0)
				{
					y0[type] = 0;
				}
				if (y1[type] < 0)
				{
					y1[type] = 0;
				}

				adjust_coordinates(&y0[type], &y1[type], max_y);
				perp[type] = x0[type];
				s0[type] = y0[type];
				s1[type] = y1[type];
				break;

			case TYPE_OUTER_VERT:
				x0[type] = x1[type] = x;
				adjust_coordinates(&x0[type], &x1[type], max_x);
				y0[type] = ct->y - ct->vy;
				y1[type] = y - ct->vy;

				if (y0[type] < 0)
				{
					y0[type] = 0;
				}
				if (y1[type] < 0)
				{
					y1[type] = 0;
				}

				adjust_coordinates(&y0[type], &y1[type], max_y);
				perp[type] = x0[type];
				s0[type] = y0[type];
				s1[type] = y1[type];
				break;
		}
	}

	#if USE_SMARTLASSO
	if (ct->has_backup)
	{
		for (type = 0; type < 4; type++)
		{
			if (type == TYPE_OUTER_HORIZ && (ct->y0[type] == ct->y0[TYPE_INNER_HORIZ])) /* same line cases, they must not grab already marked areas */
			{
				continue;
			}
			else if (type == TYPE_OUTER_VERT && (ct->x0[type] == ct->x0[TYPE_INNER_VERT]))
			{
				continue;
			}

			restore_line(ct, type, perp[type], s0[type], s1[type]);
		}
	}
	#else
	restore_lines(ct);
	#endif

	if (y0[TYPE_INNER_HORIZ] != y0[TYPE_OUTER_HORIZ] || x0[TYPE_INNER_VERT] != x0[TYPE_OUTER_VERT])
	{
		/*
		 * Backup.
		 */
		for (type = 0; type < 4; type++)
		{
			bblit(ct, type, perp[type], s0[type], s1[type]);
		}
		ct->has_backup = TRUE;

		/*
		 * Rendering.
		 */
		for (type = 0; type < 4; type++)
		{
			#if 1
			Move(ct->rp, ct->minx + x0[type], ct->miny + y0[type]);
			Draw(ct->rp, ct->minx + x1[type], ct->miny + y1[type]);
			#else
			/* XXX: hm.. missing pixels in PPA.. erm.. and I would have to only redraw new parts.. basically rip off the backup stuff without putting the old values and put it here also */
			ProcessPixelArray(ct->rp, ct->minx + x0[type], ct->miny + y0[type],
				x1[type] - x0[type] + 1,
				y1[type] - y0[type] + 1, POP_NEGATIVE, 0, NULL);
			#endif
		}

		/*
		 * Storing backup (could be above)
		 */
		for (type = 0; type < 4; type++)
		{
			ct->x0[type] = x0[type];
			ct->x1[type] = x1[type];
			ct->y0[type] = y0[type];
			ct->y1[type] = y1[type];
		}
	}
}

void lasso_render(APTR ctx, LONG x, LONG y, LONG vx, LONG vy)
{
	struct lasso_ctx *ct = ctx;

	ASSERT(ct);

	SetRPAttrs(ct->rp,
		RPTAG_PenMode, FALSE,
		RPTAG_FgColor, ct->color,
		TAG_DONE
	);

	if (x > ct->maxx)
		x = ct->maxx;

	if (y > ct->maxy + vy)
		y = ct->maxy + vy;

	/*
	 * Recalibrate as 0/0.
	 */
	x -= ct->minx;
	y -= ct->miny;

	if (x < 0)
		x = 0;

	if (y < 0)
		y = 0;

	if (ct->vx != vx || ct->vy != vy)
	{
		/* Note: if vx/vy changes we assume someone flushed lasso buffers already */

		ct->vx = vx;
		ct->vy = vy;
		ct->y0[TYPE_INNER_HORIZ] = -1;
		ct->y0[TYPE_OUTER_HORIZ] = -1;
		ct->x0[TYPE_INNER_VERT] = -1;
		ct->x0[TYPE_OUTER_VERT] = -1;
		ct->has_backup = FALSE;
	}

	draw_lines(ct, x, y);
}


void lasso_clear(APTR ctx)
{
	struct lasso_ctx *ct = ctx;

	restore_lines(ct);
}

void lasso_invalidate(APTR ctx)
{
	struct lasso_ctx *ct = ctx;
	LONG i;

	ct->has_backup = FALSE;

	for (i = 0; i < 2; i++)
		ct->s0[i] = ct->s1[i] = ct->x - ct->vx;

	for (i = 2; i < 4; i++)
		ct->s0[i] = ct->s1[i] =  ct->y - ct->vy;
}
