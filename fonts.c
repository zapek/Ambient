/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006 Ambient Open Source Team
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
 * $Id: fonts.c,v 1.9 2017/12/13 22:21:53 bitrocky Exp $
 */

#include "ambient.h"

/* public */
#include <graphics/text.h>
#include <graphics/gfxbase.h>
#include <proto/graphics.h>
#include <proto/diskfont.h>

/* private */
#include "fonts.h"
#include "mui_func.h"
#include "methodstack.h"
#include "screen.h"


struct atextfont * font_open(STRPTR name)
{
	struct TextAttr ta;
	struct atextfont *atf;
	TEXT buf[PATH_SIZE];
	STRPTR p;

	THREAD;
	ASSERT(name);

	memset(&ta, 0, sizeof(ta));

	if( (atf = malloc(sizeof(*atf))) )
	{
		int len;

		stccpy(buf, name, sizeof(buf));
		if ( (p = strchr(buf, '/')) )
		{
			*p++ = '\0';
			ta.ta_YSize = atol(p);
		}
		else
		{
			ta.ta_YSize = 8;
		}

		ta.ta_Name = buf;
		
		if (p && strstr(p, "/b"))
			ta.ta_Style |= FSF_BOLD;

		if (p && strstr(p, "/i"))
			ta.ta_Style |= FSF_ITALIC;

		if (p && strstr(p, "/u"))
			ta.ta_Style |= FSF_UNDERLINED;

		len = strlen(buf);
		if (len <  5 || stricmp(&buf[len - 5], ".font"))
		{
			strcat(buf, ".font");
		}

		if (!(atf->tf = OpenDiskFont(&ta)))
		{
		#if 1
			struct Screen *scr = get_screen();
			if (scr && scr->Font)
			{
				ta.ta_Name  = scr->Font->ta_Name;
				ta.ta_YSize = scr->Font->ta_YSize;
				ta.ta_Style = scr->Font->ta_Style;
				ta.ta_Flags = scr->Font->ta_Flags;

				atf->tf = OpenDiskFont(&ta);
			}
			if (!atf->tf)
			{
				/* Use a default.. */
				ta.ta_Name = "topaz.font";
				ta.ta_YSize = 8;
				ta.ta_Flags = 0;
				atf->tf = OpenFont(&ta);				
			}
		#else
			/* try system default font, borrowed from boopsiviewclass.c  */
			Forbid(); /* yeah, this sucks.. but the font could change anytime */
			ta.ta_Name  = GfxBase->DefaultFont->tf_Message.mn_Node.ln_Name;
			ta.ta_YSize = GfxBase->DefaultFont->tf_YSize;
			ta.ta_Style = GfxBase->DefaultFont->tf_Style;
			ta.ta_Flags = GfxBase->DefaultFont->tf_Flags;

			if (atf->tf = OpenDiskFont(&ta))
			{
				Permit();
			}
			else
			{
				Permit();
				/* Use a default.. */
				ta.ta_Name = "topaz.font";
				ta.ta_YSize = 8;
				ta.ta_Flags = 0;

				atf->tf = OpenFont(&ta);
			}
		#endif
		}

		if (ta.ta_Style != atf->tf->tf_Style)
		{
			atf->style = ta.ta_Style;
		}
		else
		{
			atf->style = 0; /* XXX: hm.. is that right ? */
		}
	}
	return (atf);
}


void font_close(struct atextfont *atf)
{
	ASSERT(atf);

	CloseFont(atf->tf);
	free(atf);
}


ULONG tr_font_load(APTR obj, STRPTR fontname, ULONG type)
{
	struct atextfont *at;

	THREAD;

	if( (at = font_open(fontname)) )
	{
		methodstack_push_sync(obj, 3, MM_Application_SetFont, type, at);
	
		return (TRUE);
	}
	return (FALSE);
}
