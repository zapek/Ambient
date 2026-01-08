/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * © 2001-2004 by David Gerber <zapek@morphos.net>
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
 * $Id: pngimage.c,v 1.8 2007/07/18 12:58:24 fab Exp $
 */

#include "globals.h"

#if USE_ICONLIB_PNGLIB

/* public */
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/dos.h>

/* private */
#include "pngimage.h"
#include "icon_internal.h"
#include "pngicon.h"


struct GfxBase *GfxBase;
struct Library *CyberGfxBase;
#if USE_SHARED_LIBPNG
struct Library *PNGBase;
#else
struct Library *PNGLibBase;
#endif

ULONG pngimage_create(STRPTR name, struct OwnDiskObject *odo)
{
	ULONG rc = FALSE;

	#if USE_SHARED_LIBPNG
	if ((PNGBase = OpenLibrary("png.library", 50)))
	#else
	if ((PNGLibBase = OpenLibrary("MOSSYS:Ambient/libs/png.alib", 1)))
	#endif
	{
		if ((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 50)))
		{
			if ((CyberGfxBase = OpenLibrary("cybergraphics.library", 50)))
			{
				BPTR f;
					
				if ((f = Open(name, MODE_OLDFILE)))
				{
					if (pngicon_read((APTR)f, odo, odo->fl))
					{
						rc = TRUE;
					}
					Close(f);
				}

				CloseLibrary(CyberGfxBase);
			}
			CloseLibrary((struct Library *)GfxBase);
		}
		#if USE_SHARED_LIBPNG
		CloseLibrary(PNGBase);
		#else
		CloseLibrary(PNGLibBase);
		#endif
	}
	return (rc);
}


void pngimage_delete(struct OwnDiskObject *odo)
{
	if (odo->pngimage || odo->pngimage2)
	{
		if ((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 50)))
		{
			if(odo->pngimage)
			{
				FreeBitMap(odo->pngimage);
			}

			if(odo->pngimage2)
			{
				FreeBitMap(odo->pngimage2);
			}

			CloseLibrary((struct Library *)GfxBase);
		}
		/* ouch.. oh well, let's leak memory then */
		odo->pngimage = NULL;
		odo->pngimage2 = NULL;
	}
}

#endif
