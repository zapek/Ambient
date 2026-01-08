/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2007 Ambient Open Source Team
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
 * $Id: layout.c,v 1.15 2017/08/11 23:32:05 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <limits.h>
#include <exec/memory.h>
#include <proto/exec.h>

/* private */
#include "dragdrop.h"
#include "fonts.h"
#include "layout.h"
#include "mui_func.h"
#include "icondata.h"


/* XXX: the code is NOT asynchronous.. check if it makes sense to make it so later on */

#define USE_LAYOUT_FASTRESIZE    0 /* XXX: enable that once it works (well, might not necessarily be useful) */
#define USE_LAYOUT_FASTLOOKUP    0 /* uses some cache too speed up lookups (XXX: NYI) */

#define USE_LAYOUT_NOOVERLAP     1 /* tries to avoid overlapping snapshoted icons (XXX: too much issues..) */

#define USE_LAYOUT_CENTERTILE    1 /* places objects in the center of the tiles */

#define TILE_SIZE               16 /* tile size in pixels, has to be a power of 2 because we round it down below */
#define TILE_POOLSIZE         1024 /* pool size (that's about the number of tiles for a 1600x1200 screen with a TILE_SIZE of 16) */

#define LAYOUT_SIZE             96 /* default size for autolayout (horizontal, vertical can vary) */

/*
 * Tile manipulation. 'c' is a layout_ctx
 */
#define SET_TILE(c,x,y)   (c->a[((x) / 8) + (y) * ((c->maxwidth / TILE_SIZE + 7) / 8)] |= (1UL << (7 - ((x) % 8))))
#define CLEAR_TILE(c,x,y) (c->a[((x) / 8) + (y) * ((c->maxwidth / TILE_SIZE + 7) / 8)] &= ~(1UL << (7 - ((x) % 8))))
#define CHECK_TILE(c,x,y) (c->a[((x) / 8) + (y) * ((c->maxwidth / TILE_SIZE + 7) / 8)] & (1UL << (7 - ((x) % 8))))

#define TILE_BYTESIZE(x)  (((x) + 7) / 8)

#define PIXELS_TO_TILE(x) ((x) / TILE_SIZE)
#define TILE_TO_PIXELS(x) ((x) * TILE_SIZE)

#define PIXELS_TO_TILESIZE(x) ((x) / TILE_SIZE + 1)


typedef enum
{
	MV_Layout_Icon_Drawer = 0,
	MV_Layout_Icon_File,
} LAYOUTICON;

struct layout_ctx {
	APTR pool;
	UBYTE *a;                  /* array of tiles */
	ULONG flags;               /* flags, see below */
	ULONG width, height;       /* in pixels */
	ULONG x, y;                /* origin (in pixels) */
	ULONG cols;                /* autolayout: columns (in objects) */
	ULONG lastrow, lastcol;    /* autolayout: where to add the next object */
	ULONG rowpos;              /* autolayout: to know where to place the new row */
	ULONG currentrowheight;    /* autolayout: speed up */
	ULONG currentrowheighttot; /* autolayout: same but with the label */
	ULONG maxwidth, maxheight; /* tilelayout: extension */
	ULONG sortmode;
	ULONG reversed;
	ULONG changed;
	ULONG num;
	LAYOUTICON icontype;       /* last icon type layouted */
	APTR grp;
	ULONG wbmode;

	IPTR  *layoutmap;
	ULONG floating_icons;
	ULONG sizex;
	ULONG sizey;
	ULONG x_units;
	ULONG textheight;

	LONG hspacing;
	LONG vspacing;
};

/* flags */
#define LCF_TRASHED (1L << 0) /* needs reconstruction */
#define LCF_ISAUTO  (1L << 1) /* autolayout */

/*
 * These values are for WB mode layout.
 * We handle up to 32000 icons in one dir.
 */
#define MASKBITS           (sizeof(IPTR) * CHAR_BIT)
#define MAX_DISPLAY_ICONS  32000
#define MAX_DISPLAY_INDEX  (MAX_DISPLAY_ICONS / MASKBITS - 1)
#define ICON_BITFIELD_SIZE (MAX_DISPLAY_ICONS / CHAR_BIT)

static void ll_set_tiles(struct layout_ctx *ct, ULONG x, ULONG y, ULONG xs, ULONG ys)
{
	ULONG i, j;

	D(LAYOUT,bug("marking tile at x: %lu, y: %lu, xs: %lu, ys: %lu as set\n", x, y, xs, ys));

	if (!ct->a)
		return;

	for (j = 0; j < ys; j++)
	{
		for (i = 0; i < xs; i++)
		{
			SET_TILE(ct, x + i, y + j);
		}
	}
}


static ULONG ll_check_tiles(struct layout_ctx *ct, ULONG x, ULONG y, ULONG xs, ULONG ys)
{
	ULONG i, j;
	ULONG rc = 0;

	for (j = 0; !rc && j < ys; j++)
	{
		for (i = 0; !rc && i < xs; i++)
		{
			rc = CHECK_TILE(ct, x + i, y + j);
		}
	}
	return (rc);
}


static ULONG ll_tile_alloca(struct layout_ctx *ct)
{
	APTR olda = ct->a;

	if ((!ct->a || (TILE_BYTESIZE(PIXELS_TO_TILE(ct->width)) * ct->height > TILE_BYTESIZE(PIXELS_TO_TILE(ct->maxwidth)) * ct->maxheight)) && ct->height && ct->width)
	{
		if (!(ct->a = AllocVecPooled(ct->pool, TILE_BYTESIZE(PIXELS_TO_TILE(ct->width)) * ct->height)))
		{
			ct->a = olda;
			return (FALSE); /* we are fucked, old tilearray is still valid though so use that (XXX: not always!) */
		}
	}
	else
	{
		olda = NULL;
	}

	ct->maxwidth = ct->width;
	ct->maxheight = ct->height;

	if (olda)
	{
		FreeVecPooled(ct->pool, olda);
	}
	return (TRUE);
}


static void ll_tile_free(struct layout_ctx *ct)
{
	if (ct->a)
	{
		FreeVecPooled(ct->pool, ct->a);
		ct->a = NULL;
		ct->maxwidth = 0;
		ct->maxheight = 0;
	}
}


static void ll_clear(struct layout_ctx *ct)
{
	if (ct->a)
	{
		memset(ct->a, 0, TILE_BYTESIZE(PIXELS_TO_TILE(ct->width)) * ct->height);
	}
	ct->lastrow = 0;
	ct->lastcol = 0;
	ct->currentrowheight = ct->vspacing;
	ct->currentrowheighttot = 0;
	ct->rowpos = 10;    /* XXX: spacing from upper border. Maybe make it adjustable with some attr? */
	ct->changed = TRUE;
	ct->icontype = MV_Layout_Icon_Drawer;
}

void layout_clear(APTR ctx)
{
	ll_clear(ctx);
}

static ULONG ll_find_closest(struct layout_ctx *ct, ULONG *tile_x, ULONG *tile_y, ULONG tiles_xs, ULONG tiles_ys, ULONG limx, ULONG limy);

static ULONG ll_place_icon(struct layout_ctx *ct, ULONG x, ULONG y, ULONG xs, ULONG ys)
{
	ULONG tiles_x = PIXELS_TO_TILE(x);
	ULONG tiles_y = PIXELS_TO_TILE(y);
	ULONG tiles_xs = PIXELS_TO_TILESIZE(xs);
	ULONG tiles_ys = PIXELS_TO_TILESIZE(ys);

	ULONG rc;

	/*
	 * Check if we fit. Let's start with
	 * the border area.
	 */
	#if USE_LAYOUT_NOOVERLAP
	if (tiles_x + tiles_xs < PIXELS_TO_TILE( ct->width ) && tiles_y + tiles_ys < PIXELS_TO_TILE( ct->height ))
	{
		D(LAYOUT,bug("checking if it fits in tile x: %lu (%lu), y: %lu (%lu), xs: %lu (%lu), ys: %lu (%lu)\n", tiles_x, x, tiles_y, y, tiles_xs, xs, tiles_ys, ys));

		rc = ll_check_tiles(ct, tiles_x, tiles_y, tiles_xs, tiles_ys);

		if (!rc)
		{
			D(LAYOUT,bug("fits, filling in tiles..\n"));

			ll_set_tiles(ct, tiles_x, tiles_y, tiles_xs, tiles_ys);
			return (TRUE);

		}
		else
		{
			D(LAYOUT,bug("tiles are already taken\n"));
		}
	}
	else
	{
		D(LAYOUT,bug("bordercheck failed\n"));
	}
	#else
	if (ll_check_tiles(ct, tiles_x, tiles_y, tiles_xs, tiles_ys))
	{
		ct->flags |= LCF_TRASHED; /* overlap, we need to rebuild it later */
	}
	ll_set_tiles(ct, tiles_x, tiles_y, tiles_xs, tiles_ys);
	return (TRUE);
	#endif
	D(LAYOUT,bug("didn't find any place\n"));
	return (FALSE);
}


static void ll_tiled_icon_remove(struct layout_ctx *ct, ULONG x, ULONG y, ULONG xs, ULONG ys)
{
	ULONG i, j;
	ULONG tiles_x = PIXELS_TO_TILE(x);
	ULONG tiles_y = PIXELS_TO_TILE(y);
	ULONG tiles_xs = PIXELS_TO_TILESIZE(xs);
	ULONG tiles_ys = PIXELS_TO_TILESIZE(ys);

	if (tiles_x + tiles_xs >= PIXELS_TO_TILE(ct->width) || tiles_y + tiles_ys >= PIXELS_TO_TILE(ct->height))
	{
		/* XXX: hm.. maybe we should output a warning because this shouldn't happen */
		D(LAYOUT,bug("It shouldn't happen (P:%d,%d S:%d,%d)\n",tiles_x,tiles_y,tiles_xs,tiles_ys));
		return;
	}

	for (j = 0; j < tiles_ys; j++)
	{
		for (i = 0; i < tiles_xs; i++)
		{
			CLEAR_TILE(ct, tiles_x + i, tiles_y + j);
		}
	}

	D(LAYOUT,bug("cleared zone at tile x: %lu, y: %lu, xs: %lu, ys: %lu\n", tiles_x, tiles_y, tiles_xs, tiles_ys))
}


static ULONG ll_find_leftright(struct layout_ctx *ct, ULONG *tile_x, ULONG *tile_y, ULONG tiles_xs, ULONG tiles_ys, ULONG limx, ULONG limy)
{
	ULONG i, j, k, l;
	ULONG rc;

	for (j = 0; j < limy - tiles_ys; j++)
	{
		for (i = 0; i < limx - tiles_xs; i++)
		{
			rc = 0;
			for (k = j; !rc && k < j + tiles_ys; k++)
			{
				for (l = i; !rc && l < i + tiles_xs; l++)
				{
					rc = CHECK_TILE(ct, l, k);
				}
			}
			if (!rc)
			{
				*tile_x = i;
				*tile_y = j;
				return (TRUE);
			}
		}
	}
	return (FALSE);
}

static ULONG ll_find_closest(struct layout_ctx *ct, ULONG *tile_x, ULONG *tile_y, ULONG tiles_xs, ULONG tiles_ys, ULONG limx, ULONG limy)
{
	ULONG i, j, k, l;
	ULONG rc;
	ULONG ox, oy;
	ULONG mindist = 0xffffffff;
	ULONG min_x = min_x, min_y = min_y;

	/*
	 * We do simpliest lookup possible. initial position is in tile_x and tile_y.
	 */

	ox = PIXELS_TO_TILE( *tile_x );
	oy = PIXELS_TO_TILE( *tile_y );

	for (j = 0; j < limy - tiles_ys; j++)
	{
		for (i = 0; i < limx - tiles_xs; i++)
		{
			rc = 0;
			for (k = j; !rc && k < j + tiles_ys; k++)
			{
				for (l = i; !rc && l < i + tiles_xs; l++)
				{
					rc = CHECK_TILE(ct, l, k);
				}
			}
			if (!rc)
			{
				ULONG dist = ( ox - i ) * ( ox - i ) + ( oy - j ) * ( oy - j );

				if ( dist < mindist )
				{
					mindist = dist;
					min_x = i;
					min_y = j;
				}
			}
		}
	}

	if ( mindist != 0xffffffff )
	{
		DB(("Closest tile from %d %d : %d %d\n",ox, oy, min_x, min_y ));

		*tile_x = min_x;
		*tile_y = min_y;
		return (TRUE);
	}

	return (FALSE);
}


#if 0
static ULONG ll_find_topbottom(struct layout_ctx *ct, ULONG *tile_x, ULONG *tile_y, ULONG tiles_xs, ULONG tiles_ys, ULONG limx, ULONG limy)
{
	ULONG i, j, k, l;
	ULONG rc;

	for (j = 0; j < limx - tiles_xs; j++)
	{
		for (i = 0; i < limy - tiles_ys; i++)
		{
			rc = 0;
			for (k = j; !rc && k < j + tiles_xs; k++)
			{
				for (l = i; !rc && l < i + tiles_ys; l++)
				{
					rc = CHECK_TILE(ct, k, l);
				}
			}
			if (!rc)
			{
				*tile_x = j;
				*tile_y = i;
				return (TRUE);
			}
		}
	}
	return (FALSE);
}


/*
 * 01 02 05 10
 * 03 04 06 11
 * 07 08 09 12
 * 13 14 15 16
 */
#define CHECK_TILE_SIZE \
	rc = 0; \
	for (k = j; !rc && k < j + tiles_xs; k++) \
	{ \
		for (l = i; !rc && l < i + tiles_ys; l++) \
		{ \
			rc = CHECK_TILE(ct, k, l); \
		} \
	} \
	if (!rc) \
	{ \
		*tile_x = j; \
		*tile_y = i; \
		return (TRUE); \
	}

static ULONG ll_find_grouped(struct layout_ctx *ct, ULONG *tile_x, ULONG *tile_y, ULONG tiles_xs, ULONG tiles_ys, ULONG limx, ULONG limy)
{
	ULONG i, i2, j, j2, k, l;
	ULONG rc;

	i = i2 = 0;
	j = j2 = 0;

	while (j2 < limx - tiles_xs || i2 < limy - tiles_ys)
	{
		j = j2;
		for (i = 0; i <= i2; i++) /* vertical search */
		{
			CHECK_TILE_SIZE
		}
		if (i2 < limy - tiles_ys)
		{
			i2++;
		}

		i = i2;
		for (j = 0; j <= j2; j++) /* horizontal search */
		{
			CHECK_TILE_SIZE
		}
		if (j2 < limx - tiles_xs)
		{
			j2++;
		}
	}
	return (FALSE);
}
#endif


/*
 * Finds an area where to put the icon.
 */
static ULONG ll_find_area(struct layout_ctx *ct, ULONG *x, ULONG *y, ULONG xs, ULONG ys, ULONG limx, ULONG limy, ULONG manualmove)
{
	ULONG rc = 0;
	ULONG tiles_xs = PIXELS_TO_TILESIZE(xs);
	ULONG tiles_ys = PIXELS_TO_TILESIZE(ys);

	/*
	 * for manual moves we use 'smarter' positioning.
	 */

	if ( manualmove )
	{
		rc = ll_find_closest(ct, x, y, tiles_xs, tiles_ys, PIXELS_TO_TILE( limx ), PIXELS_TO_TILE( limy ));
	}
	else
	{
		rc = ll_find_leftright(ct, x, y, tiles_xs, tiles_ys, PIXELS_TO_TILE( limx ), PIXELS_TO_TILE( limy ));
	}

	if (rc)
	{
		ASSERT(ct->a);
		D(LAYOUT,bug("found free zone at tile x: %lu, y: %lu\n", *x, *y))
		ll_set_tiles(ct, *x, *y, tiles_xs, tiles_ys);
		
		#if USE_LAYOUT_CENTERTILE
		*x = ct->x + TILE_TO_PIXELS(*x) + (TILE_TO_PIXELS(tiles_xs) - xs) / 2;
		*y = ct->y + TILE_TO_PIXELS(*y) + (TILE_TO_PIXELS(tiles_ys) - ys) / 2;
		#else
		*x = ct->x + TILE_TO_PIXELS(*x);
		*y = ct->y + TILE_TO_PIXELS(*y);
		#endif
	}

	*x = max(*x, 0xffff - (ICON_GRIDSIZE - 1));
	*y = max(*y, 0xffff - (ICON_GRIDSIZE - 1));

	return (rc);
}


static ULONG ll_search_place(struct layout_ctx *ct, ULONG *x, ULONG *y, ULONG xs, ULONG ys, ULONG manualmove )
{
	/*
	 * Try the different areas.
	 */
	D(LAYOUT,bug("checking prefered area (xs: %lu, ys: %lu)\n", ct->width, ct->height));
	/* XXX: hum.. this is wrong.. should extend and so on.. */
	if (!ll_find_area(ct, x, y, xs, ys, ct->width, ct->height, manualmove))
	{
		return (FALSE);

		#if 0 /* XXX: wtf was that for again? it's not set for isroot.. */
		if (!(ct->flags & LCF_ISVIRT))
		{
			return (FALSE); /* no virtgroup? 0/0 then. WHY?? */
		}

		D(LAYOUT,bug("checking limit area (xs: %lu, ys: %lu)\n", ct->xs2, ct->ys2));
		if (!ll_find_area(ct, x, y, xs, ys, ct->xs2, ct->ys2))
		{
			D(LAYOUT,bug("sigh, didn't find any zone\n")); /* XXX: we should extend at that point */
			return (FALSE);
		}
		#endif
	}
	return (TRUE);
}





static void ll_tiled_icon_add(APTR ct, APTR obj, ULONG manualmove) /* XXX: return x: 0, y: 0 if there's a failure somewhere.. and change the lame name! */
{
	struct IconData *idata = (struct IconData *)muiUserData(obj);
	ULONG x, y, xs, ys;

	ASSERT(ct);
	CHECKOBJECT(obj);

	xs = ICON_GETOBJ_WIDTH(obj);
	ys = ICON_GETOBJ_HEIGHT(obj);

	/* XXX: check if the icon is an appicon and avoid overlapping if so.. ATM we simply avoid overlaping without taking care of the "prefered" placement */

	//if ((idata->has_pos && idata->type != MV_Icon_Type_AppIcon) || manualmove)

	if (idata->has_pos || manualmove)
	{
		/* icon object position */

		x = max(ICON_GETOBJ_LEFT(obj), 0xffff - (ICON_GRIDSIZE - 1));
		y = max(ICON_GETOBJ_TOP(obj), 0xffff - (ICON_GRIDSIZE - 1));

		if (ll_place_icon(ct, x, y, xs, ys))
		{
			/* position icon */

			ICON_SETICON_LEFT_NOGRID(x + ICON_GETOBJ_WIDTH(obj) / 2 - ICON_GETICON_WIDTH(obj) / 2);
			ICON_SETICON_TOP_NOGRID(y);
		}
		else
		{
			if (ll_search_place(ct, &x, &y, xs, ys, manualmove))
			{				 
				ICON_SETICON_LEFT_NOGRID(x + ICON_GETOBJ_WIDTH(obj) / 2 - ICON_GETICON_WIDTH(obj) / 2);
				ICON_SETICON_TOP_NOGRID(y);
			}
			else
			{
				ICON_SETICON_LEFT_NOGRID(8);
				ICON_SETICON_TOP_NOGRID(8);
			}
		}
	}
	else
	{
		if (ll_search_place(ct, &x, &y, xs, ys, FALSE))
		{
			ICON_SETICON_LEFT(x + ICON_GETOBJ_WIDTH(obj) / 2 - ICON_GETICON_WIDTH(obj) / 2);
			ICON_SETICON_TOP(y);
		}
		else
		{
			ICON_SETICON_LEFT_NOGRID(8);
			ICON_SETICON_TOP_NOGRID(8);
		}
	}
}


#if 0
static void ll_multiadd_grid(struct layout_ctx *ct, Object **a, ULONG num)
{
	Object **o;
	ULONG n;
	ULONG xsize = 0;
	ULONG ysize = 0;
	ULONG ygrid;
	ULONG i, j, i2, j2;
	ULONG x, y;
	struct IconData *idata;

	o = a;
	n = num;

	/*
	 * Find out the maximum size of the objects.
	 */
	while (n--)
	{
		xsize = max(_minwidth(*o), xsize);
		ysize = max(_minheight(*o), ysize);
		o++;
	}

	/*
	 * Transform that to tiles.
	 */
	xsize = PIXELS_TO_TILESIZE(xsize);
	ysize = PIXELS_TO_TILESIZE(ysize);

	/*
	 * Get the size of the columns/lines
	 */
	ygrid = max(ct->ys1 / ysize, 1);

	/*
	 * Then fill it in from top
	 * to bottom and left to right.
	 */
	o = a;
	n = num;

	for (i = 0, i2 = 0; ; i++, i2 += xsize)
	{
		for (j = 0, j2 = 0; j < ygrid; j++, j2 += ysize)
		{
			if (n)
			{
				#if USE_LAYOUT_CENTERTILE
				x = TILE_TO_PIXELS(i2) + (TILE_TO_PIXELS(xsize) - _minwidth(*o)) / 2;
				y = TILE_TO_PIXELS(j2) + (TILE_TO_PIXELS(ysize) - _minheight(*o)) / 2;
				#else
				x = TILE_TO_PIXELS(i2);
				y = TILE_TO_PIXELS(j2);
				#endif

				if (ll_place_icon(ct, x, y, _minwidth(*o), _minheight(*o)))
				{
					idata = (struct IconData *)muiUserData(*o);

					ICON_SETX(x);
					ICON_SETY(y);
				}
				o++;
				n--;
			}
			else
			{
				goto out;
			}
		}
	}
	out:
}
#endif


void layout_icon_move(APTR ctx, APTR obj, LONG x, LONG y, LONG grid)
{
	struct IconData *idata = (struct IconData *)muiUserData(obj);
	struct layout_ctx *ct = ctx;

	ASSERT(ct);
	CHECKOBJECT(obj);

	/*
	 * We receive icon (read icondata.h) position. transform to object position for clipping.
	 */

	x = x - ( ICON_GETOBJ_WIDTH(obj) / 2 - ICON_GETICON_WIDTH(obj) / 2 );

	/*
	 * No negative moves for now. (XXX: fix that later)
	 */

	x = max(0, x);
	y = max(0, y);

    /*
	 * Back to icon coordinates.
	 */

	x = x + ( ICON_GETOBJ_WIDTH(obj) / 2 - ICON_GETICON_WIDTH(obj) / 2 );

	if (ct->flags & LCF_ISAUTO)
	{
		//ICON_SETX(x);
		//ICON_SETY(y);

		/* XXX: dunno what to do */
	}
	else if (ct->wbmode)
	{
		ICON_SETICON_LEFT_NOGRID(x);
		ICON_SETICON_TOP_NOGRID(y);
	}
	else
	{
		/* free tiles occupied by object */

		layout_icon_remove(ctx, obj);

		/* position it */

		if ( grid )
		{
			ICON_SETICON_LEFT(x);
			ICON_SETICON_TOP(y);
		}
		else
		{
			ICON_SETICON_LEFT_NOGRID(x);
			ICON_SETICON_TOP_NOGRID(y);
		}

		/* mark new tiles as occupied. *NOT* x/y.. needed because how d&d works */
		ll_tiled_icon_add(ct, obj, TRUE);
	}
	/* XXX: ponder something for ISAUTO.. */
}

#if 0
/*
 * XXX: rework that function to not kill already taken tills, etc.. it is used by
 * manual moving.. so as users are stupid they'll always screw things up..
 */
void layout_move_icon(APTR ctx, APTR obj, LONG x, LONG y)
{
	struct IconData *idata = (struct IconData *)muiUserData(obj);
	struct layout_ctx *ct = ctx; /* XXX: put asserts.. */

	/*
	 * Well, we don't want negative moves for now.
	 * Fix that later.
	 * XXX
	 */
	if (x < 0)
	{
		x = 0;
	}
	if (y < 0)
	{
		y = 0;
	}

	layout_remove_icon(ctx, obj);
	ICON_SETX(x);
	ICON_SETY(y);
	/* *NOT* x/y.. needed because how d&d works */
	ll_tiled_icon_add(ct, obj, TRUE);
}
#endif


#if 0
ULONG layout_sort_by_name(APTR ctx, ULONG grid)
{
	struct IconData *idata;
	ULONG num;
	ULONG rc = FALSE;
	struct layout_ctx *ct = ctx;
	
	ASSERT(ct);

	num = getv(ct->grp, MA_Iconview_NumIcons);

	if (num)
	{
		Object **a;

		if (a = AllocVecPooled(ct->pool, sizeof(Object *) * num))
		{
			int (*cmp)(Object **o1, Object **o2);
			Object **optr = a;

			FORCHILD(ct->grp, MUIA_Group_ChildList)
			{
				idata = (struct IconData *)muiUserData(child);
				idata->has_pos = FALSE; /* XXX: hack.. will generate wrong bubble help report. fix */
				*optr++ = child;
			}
			NEXTCHILD

			ll_clear(ct);

			ct->lsm = LSM_TOPBOTTOM;

			cmp = cmp_name_incr;

			qsort(a, num, sizeof(Object *), (const void *)cmp);

			if (grid)
			{
				ll_multiadd_grid(ct, a, num);
			}
			else
			{
				optr = a;
				while (num--)
				{
					layout_add_icon(ct, *optr++); /* XXX: there we should add by Icon_Top and not Icon_X otherwise the text goes to the left.. what a mess.. sigh */
				}
			}
			rc = TRUE;
			FreeVecPooled(ct->pool, a);
		}
		/* XXX */
	}
	return (rc);
}
#endif

static int cmp_name_incr(Object **o1, Object **o2)
{
	struct IconData *idata1 = (struct IconData *)muiUserData(*o1);
	struct IconData *idata2 = (struct IconData *)muiUserData(*o2);

	if (idata1->devicetype != MV_Icon_DeviceType_NotDevice)
	{
		if (idata1->devicetype != idata2->devicetype)
			return (idata1->devicetype - idata2->devicetype);
		else
			return (stricmp(idata1->iconname, idata2->iconname));
	}
	else if (((idata1->filetype == MV_Icon_FileType_Directory) && (idata2->filetype == MV_Icon_FileType_Directory)) ||
	         ((idata1->filetype != MV_Icon_FileType_Directory) && (idata2->filetype != MV_Icon_FileType_Directory)))
	{
		return (stricmp(idata1->iconname, idata2->iconname));
	}
	else
	{
		if (idata2->filetype == MV_Icon_FileType_Directory)
		{
			return (1);
		}
		else
		{
			return (-1);
		}
	}
}


/* XXX: I guess we'd better use mimetype as type value now rather than extension. */
static int cmp_type_incr(Object **o1, Object **o2)
{
	struct IconData *idata1 = (struct IconData *)muiUserData(*o1);
	struct IconData *idata2 = (struct IconData *)muiUserData(*o2);

	if (idata1->devicetype != MV_Icon_DeviceType_NotDevice)
	{
		if (idata1->devicetype != idata2->devicetype)
			return (idata1->devicetype - idata2->devicetype);
		else
			return (stricmp(idata1->iconname, idata2->iconname));
	}
	else if (((idata1->filetype == MV_Icon_FileType_Directory) && (idata2->filetype == MV_Icon_FileType_Directory)) ||
	         ((idata1->filetype != MV_Icon_FileType_Directory) && (idata2->filetype != MV_Icon_FileType_Directory)))
	{
		STRPTR ext1, ext2;
		int rc = 0;

		ext1 = strchr(idata1->iconname, '.');
		ext2 = strchr(idata2->iconname, '.');

		if(ext1 && ext2)
		{
			rc = stricmp(ext1, ext2);
		}

		if (!rc)
		{
			rc = stricmp(idata1->iconname, idata2->iconname);
		}

		return (rc);
	}
	else
	{
		if (idata2->filetype == MV_Icon_FileType_Directory)
		{
			return (1);
		}
		else
		{
			return (-1);
		}
	}
}


static int cmp_size_incr(Object **o1, Object **o2)
{
	struct IconData *idata1 = (struct IconData *)muiUserData(*o1);
	struct IconData *idata2 = (struct IconData *)muiUserData(*o2);

	if (idata1->devicetype != MV_Icon_DeviceType_NotDevice)
	{
		if (idata1->devicetype != idata2->devicetype)
			return (idata1->devicetype - idata2->devicetype);
		else
			return (stricmp(idata1->iconname, idata2->iconname));
	}
	else if (((idata1->filetype == MV_Icon_FileType_Directory) && (idata2->filetype == MV_Icon_FileType_Directory)) ||
		     ((idata1->filetype != MV_Icon_FileType_Directory) && (idata2->filetype != MV_Icon_FileType_Directory)))
	{
		QUAD diff;
		int ret;

		diff = idata1->filesize - idata2->filesize;
		ret  = diff > 0 ? 1 : -1;

		if (diff == 0)
		{
			ret = stricmp(idata1->iconname, idata2->iconname);
		}

		return ret;
	}
	else
	{
		if (idata2->filetype == MV_Icon_FileType_Directory)
		{
			return (1);
		}
		else
		{
			return (-1);
		}
	}
}


static int cmp_date_incr(Object **o1, Object **o2)
{
	struct IconData *idata1 = (struct IconData *)muiUserData(*o1);
	struct IconData *idata2 = (struct IconData *)muiUserData(*o2);

	if (idata1->devicetype != MV_Icon_DeviceType_NotDevice)
	{
		if (idata1->devicetype != idata2->devicetype)
			return (idata1->devicetype - idata2->devicetype);
		else
			return (stricmp(idata1->iconname, idata2->iconname));
	}
	else if (((idata1->filetype == MV_Icon_FileType_Directory) && (idata2->filetype == MV_Icon_FileType_Directory)) ||
	         ((idata1->filetype != MV_Icon_FileType_Directory) && (idata2->filetype != MV_Icon_FileType_Directory)))
	{
		QUAD diff;
		int ret;

		diff = idata1->filedate - idata2->filedate;
		ret  = diff > 0 ? 1 : -1;

		if (diff == 0)
		{
			ret = stricmp(idata1->iconname, idata2->iconname);
		}

		return ret;
	}
	else
	{
		if (idata2->filetype == MV_Icon_FileType_Directory)
		{
			return (1);
		}
		else
		{
			return (-1);
		}
	}
}


/**********************************************************************
	This is pretty dumb. cmp_#?_decr() functions are identical to
	cmp_#?_incr() functions. Only return code is swapped.
**********************************************************************/

static int cmp_name_decr(Object **o1, Object **o2)
{
	struct IconData *idata1 = (struct IconData *)muiUserData(*o1);
	struct IconData *idata2 = (struct IconData *)muiUserData(*o2);

	if (((idata1->filetype == MV_Icon_FileType_Directory) && (idata2->filetype == MV_Icon_FileType_Directory)) ||
	    ((idata1->filetype != MV_Icon_FileType_Directory) && (idata2->filetype != MV_Icon_FileType_Directory)))
	{
		return (stricmp(idata2->iconname, idata1->iconname));
	}
	else
	{
		if (idata2->filetype == MV_Icon_FileType_Directory)
		{
			return (1);
		}
		else
		{
			return (-1);
		}
	}
}


static int cmp_type_decr(Object **o1, Object **o2)
{
	struct IconData *idata1 = (struct IconData *)muiUserData(*o1);
	struct IconData *idata2 = (struct IconData *)muiUserData(*o2);

	if (((idata1->filetype == MV_Icon_FileType_Directory) && (idata2->filetype == MV_Icon_FileType_Directory)) ||
	    ((idata1->filetype != MV_Icon_FileType_Directory) && (idata2->filetype != MV_Icon_FileType_Directory)))
	{
		STRPTR ext1, ext2;
		int rc = 0;

		ext1 = strchr(idata1->iconname, '.');
		ext2 = strchr(idata2->iconname, '.');

		if(ext1 && ext2)
		{
			rc = stricmp(ext2, ext1);
		}

		if (!rc)
		{
			rc = stricmp(idata2->iconname, idata1->iconname);
		}

		return (rc);
	}
	else
	{
		if (idata2->filetype == MV_Icon_FileType_Directory)
		{
			return (1);
		}
		else
		{
			return (-1);
		}
	}
}


static int cmp_size_decr(Object **o1, Object **o2)
{
	struct IconData *idata1 = (struct IconData *)muiUserData(*o1);
	struct IconData *idata2 = (struct IconData *)muiUserData(*o2);

	if (((idata1->filetype == MV_Icon_FileType_Directory) && (idata2->filetype == MV_Icon_FileType_Directory)) ||
	    ((idata1->filetype != MV_Icon_FileType_Directory) && (idata2->filetype != MV_Icon_FileType_Directory)))
	{
		QUAD diff;
		int ret;

		diff = idata2->filesize - idata1->filesize;
		ret  = diff > 0 ? 1 : -1;

		if (diff == 0)
		{
			ret = stricmp(idata2->iconname, idata1->iconname);
		}

		return ret;
	}
	else
	{
		if (idata2->filetype == MV_Icon_FileType_Directory)
		{
			return (1);
		}
		else
		{
			return (-1);
		}
	}
}


static int cmp_date_decr(Object **o1, Object **o2)
{
	struct IconData *idata1 = (struct IconData *)muiUserData(*o1);
	struct IconData *idata2 = (struct IconData *)muiUserData(*o2);

	if (((idata1->filetype == MV_Icon_FileType_Directory) && (idata2->filetype == MV_Icon_FileType_Directory)) ||
	    ((idata1->filetype != MV_Icon_FileType_Directory) && (idata2->filetype != MV_Icon_FileType_Directory)))
	{
		QUAD diff;
		int ret;

		diff = idata2->filedate - idata1->filedate;
		ret  = diff > 0 ? 1 : -1;

		if (diff == 0)
		{
			ret = stricmp(idata2->iconname, idata1->iconname);
		}

		return ret;
	}
	else
	{
		if (idata2->filetype == MV_Icon_FileType_Directory)
		{
			return (1);
		}
		else
		{
			return (-1);
		}
	}
}


static ULONG ll_auto_icon_add(struct layout_ctx *ct, APTR obj);

ULONG layout_sort(APTR ctx)
{
	ULONG rc = FALSE;
	struct layout_ctx *ct = ctx;

	ASSERT(ct);

	if (ct->num)
	{
		Object **a;

		if ((a = AllocVecPooled(ct->pool, sizeof(Object *) * ct->num)))
		{
			#ifdef DEBUG
			ULONG numcheck = 0;
			#endif
			ULONG num = ct->num;
			int (*cmp)(Object **o1, Object **o2);
			Object **optr = a;

			DoMethod(ct->grp, MUIM_Group_InitChange);

			set(ct->grp, MA_Iconview_SortMode, TRUE);

			FORCHILD(ct->grp, MUIA_Group_ChildList)
			{
				*optr++ = child;
				DoMethod(ct->grp, OM_REMMEMBER, child);
				#ifdef DEBUG
				numcheck++;
				#endif
			}
			NEXTCHILD

			#ifdef DEBUG
			if (num != numcheck)
			{
				PDB(("internal consistency error: num: %ld, numcheck: %ld\n", num, numcheck));
			}
			#endif
			ll_clear(ct);

			switch (ct->sortmode)
			{
				default:
				case IVS_NAME:
					cmp = ct->reversed ? cmp_name_decr : cmp_name_incr; /* XXX: hardcoded hack */
					break;
				case IVS_SIZE:
					cmp = ct->reversed ? cmp_size_decr : cmp_size_incr;
					break;
				case IVS_TYPE:
					cmp = ct->reversed ? cmp_type_decr : cmp_type_incr;
					break;
				case IVS_DATE:
					cmp = ct->reversed ? cmp_date_decr : cmp_date_incr;
					break;
			}

			qsort(a, num, sizeof(Object *), (const void *)cmp);

			optr = a;

			while (num--)
			{
				DoMethod(ct->grp, OM_ADDMEMBER, *optr);

				if (ct->wbmode)
				{
					/*
					 * When emulating workbench mode we must layout icons here.
					 * Maybe calling MUI_Layout() is not safe (?) at this point
					 * but nevertheless setting position from iconview layout hook
					 * gets too complex now. -itix
					 */

					ll_auto_icon_add(ct, *optr);
				}

				optr++;
			}

			set(ct->grp, MA_Iconview_SortMode, FALSE);

			ct->changed = TRUE;

			DoMethod(ct->grp, MUIM_Group_ExitChange2, TRUE);

			rc = TRUE;
			FreeVecPooled(ct->pool, a);
		}
		/* XXX */
	}

	return (rc);
}

/*
 * Positions new icon in autolayout mode.
 */

static ULONG ll_auto_icon_add(struct layout_ctx *ct, APTR obj)
{
	struct IconData *idata = (struct IconData *)muiUserData(obj);
	ULONG newtype, oldtype;

	ASSERT(ct);
//	ASSERT(ct->flags & LCF_ISAUTO);

	ASSERT(!ct->a);
	ASSERT(ct->cols);

	ASSERT(ICON_GETIMG_WIDTH(obj) < ct->hspacing);
	//ASSERT(ICON_IMAGE_GETYS < ct->hspacing);

	if (idata->devicetype != MV_Icon_DeviceType_NotDevice)
		newtype = idata->devicetype;
	else
		newtype = idata->filetype == MV_Icon_FileType_Directory ? MV_Layout_Icon_Drawer : MV_Layout_Icon_File;

	/* initial icon type? */

	if ( ct->lastcol == 0 && ct->lastrow == 0 )
		oldtype = newtype;
	else
		oldtype = ct->icontype;

	ct->icontype = newtype;

	/* Check if we need to move to new row: reached end of last one or different icon type */

	if ( ct->lastcol == ct->cols || (oldtype != newtype && (_conf(icon_separatefiles))) )
	{
		/* new row */

		ct->rowpos += ct->currentrowheighttot + 10; /* XXX: that '10' needs some adjustement */

		ct->lastcol = 0;
		ct->lastrow++;
		ct->currentrowheight = ct->vspacing;
		ct->currentrowheighttot = ct->vspacing;
	}

	/* insert new item */

	/*
	 * Because now we have constant row height, this part is not really needed.
	 * Left it here for now if somebody didn't like my fix for the problem. (kiero)
	 */

	#if 0
	if ( _conf(icon_maxsize) > ct->currentrowheight)
	{
		ULONG lastcol = ct->lastcol;
		APTR o = obj;
		/* this icon is now the biggest of the row, realign everything */

		ct->currentrowheight = _conf(icon_maxsize);//ICON_IMAGE_GETYS;

		o = _OBJECT(o); /* get the node */

		while (lastcol-- && (o = PREVNODE(o)))
		{
			o = BASEOBJECT(o); /* and the object again */
			ICON_IMAGE_SETY2(o, ct->rowpos );
			if (!MUI_Layout(o, ICON_GETX2(o), ICON_GETY2(o), _minwidth(o), _minheight(o), 0))
			{
				return (FALSE);
			}
			o = _OBJECT(o);
		}
	}
	#endif

	ct->currentrowheighttot = max(ct->currentrowheighttot, ICON_GETOBJ_HEIGHT(obj));

	/* then place it. We adjust position using addx/y so resetting layout position will work correctly when addx/y changes */
	ICON_SETICON_TOP_NOGRID(ct->rowpos + ct->currentrowheight - ICON_GETICON_HEIGHT(obj));
	ICON_SETICON_LEFT_NOGRID(ct->hspacing * ct->lastcol + (ct->hspacing - ICON_GETICON_WIDTH(obj)) / 2);

	ct->lastcol++;

	return (TRUE);
}


static void ll_wb_icon_add(struct layout_ctx *ct, APTR obj)
{
	struct IconData *idata = (struct IconData *)muiUserData(obj);
	IPTR *map;

	if ((map = ct->layoutmap) && !idata->has_pos)
	{
		ULONG idx = 0, mask;

		while ((mask = map[idx]) == 0 && idx <= MAX_DISPLAY_INDEX)
		{
			idx++;
		}

		if (mask)
		{
			ULONG x, y, i, bitnum;

			for (i = 0; i < MASKBITS; i++)
			{
				/* Check is bit set */

				if (mask & (1 << i))
				{
					map[idx] = mask ^ (1 << i);

					bitnum = idx * MASKBITS + i;

					y = bitnum / ct->x_units;
					x = bitnum % ct->x_units;

					ICON_SETICON_LEFT((x * ct->sizex) + (ct->sizex - ICON_GETIMG_WIDTH(obj)) / 2);
					ICON_SETICON_TOP((y * ct->sizey) + (ct->sizey - ICON_GETIMG_HEIGHT(obj) - ct->textheight));
					break;
				}
			}
		}

		ct->floating_icons--;

		if (!ct->floating_icons)
		{
			FreePooled(ct->pool, ct->layoutmap, ICON_BITFIELD_SIZE);
			ct->layoutmap = NULL;     /* all done */
		}
	}
}


ULONG layout_icon_add(APTR ctx, APTR obj)
{
	struct layout_ctx *ct = ctx;
	//struct IconData *idata = (struct IconData *)muiUserData(obj);
	
	ASSERT(ct);
	CHECKOBJECT(obj);

	if (ct->changed )
	{
		if (ct->flags & LCF_ISAUTO)
		{
			ll_auto_icon_add(ct, obj); /* XXX: how to handle failure ? */
		}
		else
		{
			if (ct->wbmode)
				ll_wb_icon_add(ct, obj);
			else
				ll_tiled_icon_add(ct, obj, FALSE);
		}

	}

	/* ICON_GET_OBJECT returns position of upperleft edge of whole icon object */

	MUI_Layout(obj, ICON_GETOBJ_LEFT(obj), ICON_GETOBJ_TOP(obj), _minwidth(obj), _minheight(obj), 0);

	return (TRUE);
}


void layout_icon_remove(APTR ctx, APTR obj)
{
	//struct IconData *idata = (struct IconData *)muiUserData(obj);
	struct layout_ctx *ct = ctx;

	ASSERT(ct);
	CHECKOBJECT(obj);
	
	if (ct->flags & LCF_ISAUTO)
	{
		/* XXX: fix fix fix */
		//ll_auto_icon_remove(ct, obj);
	}
	else
	{
		ll_tiled_icon_remove(ct, ICON_GETOBJ_LEFT(obj), ICON_GETOBJ_TOP(obj), ICON_GETOBJ_WIDTH(obj), ICON_GETOBJ_HEIGHT(obj)); /* XXX: hm. what a broken api :) */
	}
}


static ULONG ll_auto_calcgrid(struct layout_ctx *ct)
{
	ULONG rc = FALSE;
	ULONG cols;

	cols = max(1, ct->width / max(ct->hspacing, 1));

	if (cols != ct->cols)
	{
		rc = TRUE;

		ct->cols = cols;
		ct->lastrow = 0;
		ct->lastcol = 0;
		ct->currentrowheight = ct->vspacing;
		ct->currentrowheighttot = ct->vspacing;
		ct->rowpos = 10;		/* XXX: spacing from upper border. Maybe make it adjustable with some attr? */
		ct->changed = TRUE;

	}
	return (rc);
}


static void ll_relayout(struct layout_ctx *ct)
{
	ll_clear(ct);
	#if 0 /* XXX: ahee.. this is useless since we calc it everytime on our layout hook.. sigh */
	FORCHILD(ct->grp, MUIA_Group_ChildList)
	{
		layout_icon_add(ct, child);
	}
	NEXTCHILD
	#endif
}


void layout_done(APTR ctx)
{
	struct layout_ctx *ct = ctx;

	ASSERT(ct);
	
	D(LAYOUT,bug("ctx: %p, commiting changes\n", ct));

	ct->changed = FALSE;
}


void v_layout_setattrs(APTR ctx, struct TagItem *tags)
{
	struct layout_ctx *ct = ctx;

	ULONG recalc = FALSE;
	ULONG newsize = FALSE;
	ULONG newsort = FALSE;

	ASSERT(ct);

	FORTAG(tags)
	{
		case LAYOUTTAG_Auto:
			if (tag->ti_Data)
			{
				if (!(ct->flags & LCF_ISAUTO)) recalc = TRUE;

				D(LAYOUT,bug("ctx: %p, autolayout enabled\n", ct));
				ct->flags |= LCF_ISAUTO;
			}
			else
			{
				if (ct->flags & LCF_ISAUTO)
				{
					recalc = TRUE;
					newsize = TRUE;
				}
				D(LAYOUT,bug("ctx: %p, autolayout disabled\n", ct));
				ct->flags &= ~LCF_ISAUTO;
			}
			break;
	
		case LAYOUTTAG_Width:
			/* XXX: we might need a boundaries extension in non autolayout. same for below and above */
			if (ct->width != tag->ti_Data) newsize = TRUE;
			ct->width = tag->ti_Data;
			D(LAYOUT,bug("ctx: %p, width: %lu\n", ct, ct->width));
			break;

		case LAYOUTTAG_Height:
			if (ct->height != tag->ti_Data) newsize = TRUE;
			ct->height = tag->ti_Data;
			D(LAYOUT,bug("ctx: %p, height: %lu\n", ct, ct->height));
			break;

		case LAYOUTTAG_OriginX:
			if (ct->x != tag->ti_Data) newsize = TRUE;
			ct->x = tag->ti_Data;
			D(LAYOUT,bug("ctx: %p, x: %lu\n", ct, ct->x));
			break;

		case LAYOUTTAG_OriginY:
			if (ct->y != tag->ti_Data) newsize = TRUE;
			ct->y = tag->ti_Data;
			D(LAYOUT,bug("ctx: %p, y: %lu\n", ct, ct->y));
			break;
	
		case LAYOUTTAG_HSpacing:
			if (ct->hspacing != tag->ti_Data) newsize = TRUE;
			ct->hspacing = max(tag->ti_Data + 10, LAYOUT_SIZE);
			D(LAYOUT,bug("ctx: %p, horizontal spacing: %lu\n", ct, ct->hspacing));
			break;

		case LAYOUTTAG_VSpacing:
			if (ct->vspacing != tag->ti_Data) newsize = TRUE;
			ct->vspacing = tag->ti_Data;
			break;

		case LAYOUTTAG_SortMode:
			if (ct->sortmode != tag->ti_Data || !(ct->flags & LCF_ISAUTO) ) newsort = TRUE;
			ct->sortmode = tag->ti_Data;
			D(LAYOUT,bug("ctx: %p, sortmode: %lu\n", ct, ct->sortmode));
			break;

		case LAYOUTTAG_SortReversed:
			if (ct->reversed != tag->ti_Data) newsort = TRUE;
			ct->reversed = tag->ti_Data;
			D(LAYOUT,bug("ctx: %p, sorting order %s\n", ct, ct->reversed ? "reversed" : "ordered"));
			break;

		case LAYOUTTAG_Num:
			if (ct->num != tag->ti_Data) recalc = TRUE;
			ct->num = tag->ti_Data;
			D(LAYOUT,bug("ctx: %p, setting number of objects to %lu\n", ct, ct->num));
			break;
	
		case LAYOUTTAG_Changed:
			ct->changed = tag->ti_Data;
			D(LAYOUT,bug("ctx: %p, set changing manually to %lu\n", ct, ct->changed));
			break;

		case LAYOUTTAG_WBMode:
			ct->wbmode = tag->ti_Data;
			break;

		#ifdef DEBUG
		default:
			PDB(("ctx: %p, unexisting tag 0x%lx\n", ct, tag->ti_Tag));
			break;
		#endif
	}
	NEXTTAG

	if (!ct->wbmode && ct->layoutmap)
	{
		FreePooled(ct->pool, ct->layoutmap, ICON_BITFIELD_SIZE);
	}

	if (!recalc && newsize)
	{
		if (ct->flags & LCF_ISAUTO)
		{
			if (ll_auto_calcgrid(ct))
			{
				newsize = FALSE; /* don't calc the grid twice */
				recalc = TRUE;
			}
		}
		else
		{
			ll_auto_calcgrid(ct);
			recalc = TRUE; /* XXX: lazyness.. or not */
		}
	}

	if (recalc)
	{
		if (ct->flags & LCF_ISAUTO)
		{
			ll_tile_free(ct);

			if (!newsize)
			{
				ll_auto_calcgrid(ct);
			}
		}
		else
		{
			if (newsize)
			{
				ll_tile_alloca(ct);
			}
		}
		ll_relayout(ct);
	}

	if (newsort)
	{
		layout_sort(ct);
	}
}


ULONG layout_getattr(APTR ctx, ULONG attr)
{
	struct layout_ctx *ct = ctx;

	ASSERT(ct);

	switch (attr)
	{
		case LAYOUTTAG_Auto:
			if (ct->flags & LCF_ISAUTO)
			{
				return (TRUE);
			}
			else
			{
				return (FALSE);
			}
			break;

		case LAYOUTTAG_Changed:
			return (ct->changed);

		#ifdef DEBUG
		default:
			PDB(("unknown attribute 0x%lx\n", attr));
			break;
		#endif
	}
	return (0);
}


APTR v_layout_create(APTR grp, struct TagItem *tags)
{
	APTR pool;

	if ((pool = CreatePool(MEMF_ANY | MEMF_CLEAR, TILE_POOLSIZE, TILE_POOLSIZE / 2)))
	{
		struct layout_ctx *ct;

		if ((ct = AllocPooled(pool, sizeof(*ct))))
		{
		
			D(LAYOUT,bug("layout context %p created\n", ct));

			ct->pool = pool;
			ct->grp = grp;
			ct->hspacing = LAYOUT_SIZE;
			ct->vspacing = _conf(icon_maxsize);
			ct->flags = LCF_ISAUTO;
			ct->changed = TRUE;
			ct->icontype = MV_Layout_Icon_Drawer;
			ct->x = 0;
			ct->y = 0;
			ct->wbmode = 0;
			ct->layoutmap = NULL;

			v_layout_setattrs(ct, tags);

			return (ct);
		}
	}
	D(LAYOUT,bug("failed to create layout context\n"));
	return (NULL);
}


void layout_delete(APTR ctx)
{
	struct layout_ctx *ct = ctx;

	ASSERT(ct);

	D(LAYOUT,bug("deleted layout context %p\n", ctx));

	DeletePool(ct->pool);
}


void layout_place_icons(APTR ctx)
{
	struct layout_ctx *ct = ctx;
	IPTR *map;

	D(LAYOUT,bug("Placing icons in manual order\n"));

	if (!ct->layoutmap)
		ct->layoutmap = AllocPooled(ct->pool, ICON_BITFIELD_SIZE);

	if ((map = ct->layoutmap))
	{
		ULONG bad_icons, x_units, sizex, sizey;
		struct atextfont *font;

		memset(map, -1, ICON_BITFIELD_SIZE);

		sizex = 90;                         /* taken from iconclass.c/MM_Icon_Generate() */
		sizey = _conf(icon_maxsize);

		if ((font = _conf(window_font)))
		{
			ct->textheight = (font->tf->tf_YSize + _conf(window_spaceline)) * 2;
			sizey += ct->textheight;
		}

		x_units = _mwidth(ct->grp) / sizex;   /* number of icons per line */

		if (x_units < 2)
			x_units = 2;

		ct->sizex   = sizex;
		ct->sizey   = sizey;
		ct->x_units = x_units;

		bad_icons = 0;

		FORCHILD(ct->grp, MUIA_Group_ChildList)
		{
			struct IconData *idata = (struct IconData *)muiUserData(child);
			LONG x, y;

			x = idata->x /*- POSX_CORRECTION*/;
			y = idata->y /*- POSY_CORRECTION*/;

			if (x < 0 || y < 0 || idata->isdefault || !idata->has_pos)
			{
				bad_icons++;
			}
			else
			{
				ULONG bitnum, idx, bit, i, j;

				idata->has_pos = TRUE;

				for (i = 0; i < 1; i++)
				{
					for (j = 0; j < 1; j++)
					{
						/* Sucky. Used to extend existense of an icon to surrounding fields. */

						bitnum = ((x+i*(sizex-1)) / sizex) + (((y+(j*sizey-1)) / sizey) * x_units);
						idx = bitnum / MASKBITS;
						bit = bitnum % MASKBITS;

						if (idx < MAX_DISPLAY_INDEX)
							map[idx] = map[idx] & (~(1 << bit));
					}
				}
			}
		}
		NEXTCHILD

		ct->floating_icons = bad_icons;

		if (!bad_icons)
		{
			FreePooled(ct->pool, ct->layoutmap, ICON_BITFIELD_SIZE);
			ct->layoutmap = NULL;
		}
	}
}
