/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2016 Ambient Open Source Team
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
 * $Id: locale.c,v 1.9 2016/03/19 16:31:13 itix Exp $
 */

#include "ambient.h"

/* public */
#include <libraries/locale.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/locale.h>

/* private */
#include "locale.h"
#include "ambient_cat.h"

static struct Catalog *catalog;

struct Locale *locale;

LONG   locale_timezone_offset;

ULONG locale_init(void)
{
	/* Safe with NULL argument */

	locale = OpenLocale(NULL);

	locale_timezone_offset = -locale->loc_GMTOffset * 60;

	catalog = OpenCatalog(
		NULL, "Ambient.catalog",
		OC_BuiltInLanguage, "english",
		OC_Version, 7,  /* required catalog version */
		TAG_DONE
	);

	if (catalog)
	{
		int c;

		for (c = 0; c < NUMCATSTRING; c++)
		{
			((char**)__stringtable)[ c ] = GetCatalogStr(catalog, c, (char*)__stringtable[c]);
		}
	}
	return (TRUE);
}

void locale_cleanup(void)
{
	/* Both safe with NULL arguments */
	CloseLocale(locale);
	CloseCatalog(catalog);
}

static void PutCharFunc(struct Hook *h, UNUSED struct Locale *l, TEXT c)
{
	STRPTR p = h->h_Data;
	*p++ = c;
	h->h_Data = p;
}

void CreateDateString(CONST_STRPTR template, struct DateStamp *ds, STRPTR buf)
{
	struct Hook h;

	h.h_Entry = (HOOKFUNC)HookEntry;
	h.h_SubEntry = (HOOKFUNC)PutCharFunc,
	h.h_Data = buf;

	FormatDate(locale, template, ds, &h);
}

static size_t GetCharFunc(struct Hook *h)
{
	STRPTR p = h->h_Data;
	TEXT c = *p++;

	h->h_Data = p;
	return c;
}

BOOL ParseDateString(CONST_STRPTR template, struct DateStamp *ds, CONST_STRPTR datestr)
{
	struct Hook h;

	h.h_Entry = (HOOKFUNC)HookEntry;
	h.h_SubEntry = (HOOKFUNC)GetCharFunc,
	h.h_Data = (APTR)datestr;

	return ParseDate(locale, ds, template, &h);
}
