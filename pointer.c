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
 * $Id: pointer.c,v 1.4 2006/08/08 13:31:35 fab Exp $
 */

#include "ambient.h"

/* public */
#include <intuition/pointerclass.h>
#include <intuition/extensions.h>
#include <proto/intuition.h>

/* private */
#include "pointer.h"


#define USE_CUSTOM_POINTERS 0

#if USE_CUSTOM_POINTERS
/*
 * Lame image for now.. struct BitMap
 * Ripped off from MUI because I have no editor for
 * planar stuff :o)
 */
#define POINTER_XS 16
#define POINTER_YS 16
#define POINTER_DEPTH 2

static const USHORT xhairData[32] = {
	/* BitPlane 0 */
	0x0100,0x0280,0x0440,0x0000,0x0000,0x2008,0x4004,0x8002,
	0x4004,0x2008,0x0000,0x0000,0x0440,0x0280,0x0100,0x0000,
	/* BitPlane 1 */
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};

/* BitMap for "xhair" */
static struct BitMap xhairBitMap = {
	2, 16, 0, 2, 0, {
		(PLANEPTR)&xhairData[0],
		(PLANEPTR)&xhairData[16]
	}
};

struct pointers_objects {
	APTR move;
	/* more pointers to follow, XXX: this should be an array really.. */
};

static struct pointers_objects *pointers;

ULONG pointer_init(void)
{
	if ( (pointers = malloc(sizeof(*pointers))) )
	{
		memclr(pointers, sizeof(*pointers));
		return (TRUE);
	}
	return (FALSE);
}


void pointer_cleanup(void)
{
	if (pointers)
	{
		if (pointers->move) /* XXX: I should have some recursion or something.. well.. use the array */
		{
			DisposeObject(pointers->move);
		}
		free(pointers);
	}
}


void pointer_set(struct Window *win, ULONG type)
{
	APTR *pointer = pointer; /* shut up gcc */
	struct BitMap *bm = bm; /* shut up gcc */

	ASSERT(win);

	switch (type)
	{
		case POINTER_MOVE:
			pointer = &pointers->move;
			bm = &xhairBitMap;
			break;

		#ifdef DEBUG
		default:
			PDB(("unsupported pointer type %ld\n", type));
			return;
		#endif
	}

	if (!*pointer)
	{
		*pointer = NewObject(NULL, "pointerclass",
			POINTERA_BitMap, bm,
			POINTERA_WordWidth, bm->BytesPerRow,
			POINTERA_XOffset, (ULONG)-7, /* XXX */
			POINTERA_YOffset, (ULONG)-7,
		TAG_DONE);
	}

	if (*pointer)
	{
		SetWindowPointer(win, WA_Pointer, *pointer, TAG_DONE);
	}
}


void pointer_clear(struct Window *win)
{
	ASSERT(win);

	SetWindowPointer(win, TAG_DONE);
}

#else


ULONG pointer_init(void)
{
	return (TRUE);
}


void pointer_cleanup(void)
{
	/* zZz */
}


void pointer_set(struct Window *win, ULONG type)
{
	#if USE_POINTERS
	ULONG intuitype;

	ASSERT(win);

	switch (type)
	{
		case POINTER_MOVE:
			intuitype = POINTERTYPE_MOVE;
			break;

		case POINTER_CROSSAIR:
			intuitype = POINTERTYPE_AIMING;
			break;

		case POINTER_TARGET:
			intuitype = POINTERTYPE_SELECTLINK;
			break;

		default:
			intuitype = POINTERTYPE_NORMAL;
			break;
	}

	SetWindowPointer(win, WA_PointerType, intuitype, TAG_DONE);
	#endif
}


void pointer_clear(struct Window *win)
{
	#if USE_POINTERS
	ASSERT(win);

	SetWindowPointer(win, TAG_DONE);
	#endif
}

#endif
