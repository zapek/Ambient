/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2006 Ambient Open Source Team
 * graphclass.c, Copyright 2005 by Adam Waldenberg
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
 * $Id: graphclass.c,v 1.5 2007/09/29 13:35:30 fab Exp $
 */

#include "ambient.h"

/* public */
#include <exec/lists.h>
#include <exec/types.h>
#include <intuition/classes.h>
#include <graphics/rpattr.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/utility.h>

/* private */
#include <macros/vapor.h>
#include "gfx_linedraw.h"
#include "mui_func.h"

#define WRAP(a, b) ((a < 0) ? (b + a) : ((a) % (b)))

#define DRAW_GRID(a) gfx_linedraw_aa(_rp(obj), _left(obj), a, _right(obj), a, data->color.grid)

#define DRAW_LINE(a) gfx_linedraw_aa(_rp(obj), data->values[WRAP((a - 1), GRAPHCLASS_ACCURACY)].x, \
	data->values[WRAP((a - 1), GRAPHCLASS_ACCURACY)].y, \
	data->values[WRAP(a, GRAPHCLASS_ACCURACY)].x, \
	data->values[WRAP(a, GRAPHCLASS_ACCURACY)].y, \
	data->color.line)

typedef struct {
	struct MinNode n;
	int x, y, v;
} ValNode;

struct Data {
	ValNode values[GRAPHCLASS_ACCURACY];

	LONG           upper_value;
	LONG           scale_value;
	ULONG          numvalues;
	LONG           queuewrap;
	LONG           pos;
	struct MinList bysize;

	struct {
		ULONG background, grid, line;
	} color;
};

static inline void recalc_graphpoint(struct Data *data, APTR obj, int p, int v)
{
	data->values[p].x = (((float) _width(obj) / GRAPHCLASS_ACCURACY) * v) + _left(obj);
	data->values[p].y = 1 + _bottom(obj) - (((float) _height(obj) * data->values[p].v/
	                    max(data->upper_value, data->scale_value)) );
}

static inline void add_node(struct Data *data, int p)
{
	ValNode *node;

	ITERATELIST(node, &data->bysize)
	{
		if (data->values[p].v > node->v)
			break;
	}

	Insert((struct List *) &data->bysize, (struct Node *) &data->values[p].n, (struct Node *) node->n.mln_Pred);
}

static inline void rem_nodes(struct Data *data)
{
	int i;

	for (i = data->queuewrap; i < data->queuewrap + (GRAPHCLASS_ACCURACY >> 2); i++)
		REMOVE((struct Node *) &data->values[i].n);

	data->upper_value = ((ValNode *) data->bysize.mlh_Head)->v;
}

DEFNEW
{
	obj = DoSuperNew(cl, obj,
		InnerSpacing(0,0),
        MUIA_FillArea , FALSE,
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		struct Data *data;
		data = INST_DATA(cl, obj);

		data->pos = data->queuewrap = data->numvalues = 0;
		data->upper_value = data->scale_value = 0;
		data->color.background = 0x00333333;
		data->color.grid = 0x00555555;
		data->color.line = 0x00BBBBBB;
		NEWLIST(&data->bysize);

		FORTAG(INITTAGS)
		{
			case MA_Graph_BackgroundColor:
				data->color.background = tag->ti_Data;
				break;

			case MA_Graph_GridColor:
				data->color.grid = tag->ti_Data;
				break;

			case MA_Graph_LineColor:
				data->color.line = tag->ti_Data;
				break;

			case MA_Graph_ScaleValue:
				data->scale_value = tag->ti_Data;
		}
		NEXTTAG
	}

	return (ULONG) obj;
}

DEFMMETHOD(Draw)
{
	GETDATA;
	ULONG s = DOSUPER;
	int i;

	if (msg->flags & MADF_DRAWOBJECT)
	{
		/* Make our background... */
		FillPixelArray(_rp(obj), _left(obj), _top(obj), _width(obj), _height(obj), data->color.background);

		/* Draw the grid... */
		DRAW_GRID(_top(obj));
		DRAW_GRID((_height(obj) / 4) + _top(obj));
		DRAW_GRID((_height(obj) / 4) * 2 + _top(obj));
		DRAW_GRID((_height(obj) / 4) * 3 + _top(obj));

		/* ...And finaly update the graph itself. */
		recalc_graphpoint(data, obj, WRAP(data->queuewrap, GRAPHCLASS_ACCURACY), 0);
		for (i = 1; i < data->numvalues; i++)
		{
			recalc_graphpoint(data, obj, WRAP(i + data->queuewrap, GRAPHCLASS_ACCURACY), i);
			DRAW_LINE(WRAP(i + data->queuewrap, GRAPHCLASS_ACCURACY));
		}
	}
	else if ((msg->flags & MADF_DRAWUPDATE) && data->numvalues > 1)
	{
		DRAW_LINE(data->pos);
	}

	return s;
}

DEFMMETHOD(AskMinMax)
{
	DOSUPER;

	msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
	msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

	msg->MinMaxInfo->MinWidth += 128;
	msg->MinMaxInfo->MinHeight += 96;

	return 0;
}

DEFSMETHOD(Graph_Add)
{
	GETDATA;

	data->values[data->pos].v = msg->value;

	if (data->numvalues < GRAPHCLASS_ACCURACY)
	{
		/* If this is true we need to re-scale... */
		if (msg->value > max(data->upper_value, data->scale_value))
		{
			data->upper_value = msg->value;
			MUI_Redraw(obj, MADF_DRAWOBJECT);
		}

		add_node(data, WRAP(data->pos, GRAPHCLASS_ACCURACY));
		recalc_graphpoint(data, obj, WRAP(data->pos, GRAPHCLASS_ACCURACY), data->numvalues);
		data->numvalues++;

		DB(("New coordinate at (%ld,%ld).\n",
		data->values[data->pos].x, data->values[data->pos].y));

		MUI_Redraw(obj, MADF_DRAWUPDATE);

		if (++data->pos >= GRAPHCLASS_ACCURACY)
			data->pos = data->queuewrap;
	}
	else
	{
		rem_nodes(data);
		data->numvalues = data->numvalues - (GRAPHCLASS_ACCURACY >> 2);
		data->queuewrap = WRAP((data->queuewrap + (GRAPHCLASS_ACCURACY >> 2)), GRAPHCLASS_ACCURACY);
		MUI_Redraw(obj, MADF_DRAWOBJECT);
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECMMETHOD(Draw)
DECMMETHOD(AskMinMax)
DECSMETHOD(Graph_Add)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Area, graphclass)

