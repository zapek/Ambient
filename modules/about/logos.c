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
 * $Id: logos.c,v 1.6 2025/10/03 02:58:18 cyfm Exp $
 */

#include "globals.h"

/* public */

/* private */
#include "libraries/about.h"
#include "clib/about_protos.h"
#include "ambient_logo.h"
#include "morphos_logo.h"
#include "mui_logo.h"
#include "altivec_logo.h"
#include "zlib_logo.h"
#include "libpng_logo.h"


APTR About_GetLogo(ULONG type, ULONG *width, ULONG *height, ULONG *depth)
{
	switch (type)
	{
		case ABOUT_LOGO_AMBIENT:
			width ? (*width = AMBIENT_WIDTH) : 0;
			height ? (*height = AMBIENT_HEIGHT) : 0;
			depth ? (*depth = AMBIENT_DEPTH) : 0;
			return (ambient);
		
		case ABOUT_LOGO_MORPHOS:
			width ? (*width = MORPHOS_WIDTH) : 0;
			height ? (*height = MORPHOS_HEIGHT) : 0;
			depth ? (*depth = MORPHOS_DEPTH) : 0;
			return (morphos);
	
		case ABOUT_LOGO_MUI:
			width ? (*width = MUI_WIDTH) : 0;
			height ? (*height = MUI_HEIGHT) : 0;
			depth ? (*depth = MUI_DEPTH) : 0;
			return (mui);

		case ABOUT_LOGO_ALTIVEC:
			width ? (*width = ALTIVEC_WIDTH) : 0;
			height ? (*height = ALTIVEC_HEIGHT) : 0;
			depth ? (*depth = ALTIVEC_DEPTH) : 0;
			return (altivec);
	
		case ABOUT_LOGO_ZLIB:
			width ? (*width = ZLIB_WIDTH) : 0;
			height ? (*height = ZLIB_HEIGHT) : 0;
			depth ? (*depth = ZLIB_DEPTH) : 0;
			return (zlib);

		case ABOUT_LOGO_LIBPNG:
			width ? (*width = LIBPNG_WIDTH) : 0;
			height ? (*height = LIBPNG_HEIGHT) : 0;
			depth ? (*depth = LIBPNG_DEPTH) : 0;
			return (libpng);
	}
	return (NULL);
}
