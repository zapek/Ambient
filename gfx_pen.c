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
 * $Id: gfx_pen.c,v 1.5 2006/04/12 14:01:53 fab Exp $
 */

#include "ambient.h"

/* public */
#include <proto/graphics.h>

/* private */
#include "gfx_pen.h"
#include "mui_func.h"


static __inline__ ULONG htol(STRPTR hex)
{
	#define mytoupper(c) ((c)&~32)
	ULONG result = 0;
	int i=8;

	while (i-- && *hex)
	{
		result<<=4;

		if (*hex >= '0' && *hex <= '9')
		{
			result += *hex - '0';
		}
		else if (*hex >= 'A' && mytoupper(*hex) <= 'F')
		{
			result += mytoupper(*hex) - 'A' + 10;
		}
		else
		{
			/*
			 * ToolManager Prefs causes this
			 */
			result>>=4;
			break;
		}

		hex++;
	}
	return(result);
}

ULONG gfx_get_penspec_value(struct MUI_RenderInfo *mri, struct MUI_PenSpec *spec)
{
	LONG p = -1;
	STRPTR buf = (STRPTR)spec;
	ULONG t[3];

	ASSERT(mri);
	ASSERT(spec);

	switch (*buf)
	{
		case 's': /* system pen */
			p = atol(buf + 1);
			if (p >= 0 && p < mri->mri_DrawInfo->dri_NumPens)
			{
				p = mri->mri_DrawInfo->dri_Pens[p];
			}
			else
			{
				p = BACKGROUNDPEN;
			}
			break;

		case 'm': /* MUI pen */
			p = atol(buf + 1);
			if (p >= 0 && p < MPEN_COUNT)
			{
				p = mri->mri_Pens[p];
			}
			break;

		case 'p': /* palette entry */
			{
				ULONG numcols = min(256, 1 << GetBitMapAttr(mri->mri_Screen->RastPort.BitMap, BMA_DEPTH));
				p = atol(buf + 1);
				if (p < 0 && numcols > 4)
				{
					p += numcols;
				}
				if (p < 0 || p >= numcols)
				{
					p = 0;
				}
			}
			break;

		default:
			{
				ULONG r = 0;
				ULONG l;
				
				if (*buf == 'r')
				{
					buf++;
				}

				l = strlen(buf);

				if (l == 6) /* "RRGGBB" */
				{
					ULONG x = htol(buf);

					r = ((x << 8) & 0xff000000) >> 8;
					r |= ((x << 16) & 0xff000000) >> 16;
					r |= x & 0xff;
				}
				else
				{
					if (l >= 8)
						r = (htol(buf) & 0xff000000) >> 8;
					if (l >= 17)
						r |= (htol(buf + 9) & 0xff000000) >> 16;
					if (l >= 26)
						r |= (htol(buf + 18) & 0xff000000) >> 24;
				}
				return (r);
			}
			break;
	}
	GetRGB32(mri->mri_Screen->ViewPort.ColorMap, p, 1, (ULONG *)&t);
	return (((t[0] & 0xff) << 16) | ((t[1] & 0xff) << 8) | (t[2] & 0xff));
}
