/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2015 Ambient Open Source Team
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
 * $Id: libs.c,v 1.15 2018/01/27 07:13:10 bigfoot Exp $
 */

#include "ambient.h"

/* public */
#include <exec/libraries.h>
#include <proto/exec.h>

/* private */
#include "libs.h"
#include "errorreq.h"


struct libdesc {
	struct Library **base;
	CONST_STRPTR name;
	ULONG version;
	ULONG revision;
	ULONG optional;
};

struct IntuitionBase *IntuitionBase;
struct Library       *MUIMasterBase;
struct Library       *AsyncIOBase;
struct Library       *CGXDitherBase;
struct Library       *CyberGfxBase;
struct Library       *OpenURLBase;
struct DosLibrary    *DOSBase;
struct Library       *DataTypesBase;
struct UtilityBase   *UtilityBase;
struct GfxBase       *GfxBase;
struct Library       *DiskfontBase;
struct LocaleBase    *LocaleBase;
struct Library       *IFFParseBase;
struct Library       *LayersBase;
struct Library       *CxBase;
struct Library       *RexxSysBase;
struct Library       *KeymapBase;
struct Library       *WorkbenchBase;
#if USE_PNGICONS
#if USE_SHARED_LIBZ
struct Library       *ZBase;
#else
struct Library       *ZLibBase;
#endif
#if USE_SHARED_LIBPNG
struct Library       *PNGBase;
#else
struct Library       *PNGLibBase;
#endif
#endif
#if USE_RANDOM_LIB
struct Library       *RandomBase;
#endif
#if !USE_LEGACY
struct Library	     *LogBase;
#endif
struct Library       *BTreeBase;
struct Library		 *QueryBase;
#if USE_THREADPOOL
struct Library       *ThreadPoolBase;
#endif
#if USE_SVGICONS
struct Library       *VGraphicsBase;
#endif

static const struct libdesc ld[] = {
	{(struct Library **)&IntuitionBase, "intuition.library",     50,   11, 0},
	#if USE_LEGACY
	{(struct Library **)&DOSBase,       "dos.library",           50,   59, 0},
	#else
	{(struct Library **)&DOSBase,       "dos.library",           50,   65, 0},
	#endif
	{&LayersBase,                       "layers.library",        50,    1, 0},
	{&DataTypesBase,                    "datatypes.library",     38,    0, 0},
	{(struct Library **)&UtilityBase,   "utility.library",       50,    1, 0},
	{(struct Library **)&GfxBase,       "graphics.library",      41,  100, 0},
	{&DiskfontBase,                     "diskfont.library",      39,    0, 0},
	#if USE_LEGACY
	{&MUIMasterBase,                    "muimaster.library",     20, 2602, 0},
	#else
	{&MUIMasterBase,                    "muimaster.library",     20, 3819, 0},
	#endif
	{(struct Library **)&LocaleBase,    "locale.library",        39,    0, 0},
	#if USE_LEGACY
	{&AsyncIOBase,                      "asyncio.library",       40,    0, 0},
	#else
	{&AsyncIOBase,                      "asyncio.library",       50,    0, 0},
	#endif
	{&CyberGfxBase,                     "cybergraphics.library", 50,   17, 0},
	{&CGXDitherBase,                    "cgxdither.library",     50,    5, 0},
	{&IFFParseBase,                     "iffparse.library",      37,    0, 0},
	{&CxBase,                           "commodities.library",   37,    0, 0},
	{&RexxSysBase,                      "rexxsyslib.library",    36,    0, 1},
	{&KeymapBase,                       "keymap.library",        50,    4, 0},
	{&WorkbenchBase,                    "workbench.library",     51,    0, 0},
	#if USE_PNGICONS
	#if USE_SHARED_LIBZ
	{&ZBase,                            "z.library",             51,    0, 0},
	#else
	{&ZLibBase,                         "PROGDIR:libs/z.alib",    1,    0, 0},
	#endif
	#if USE_SHARED_LIBPNG
	{&PNGBase,                          "png.library",           50,    0, 0},
	#else
	{&PNGLibBase,                       "PROGDIR:libs/png.alib",  1,    0, 0},
	#endif
	#endif
	{&OpenURLBase,                      "openurl.library",        2,    0, 1},
	#if USE_RANDOM_LIB
	{&RandomBase,                       "random.library",         1,    0, 0},
	#endif
	#if !USE_LEGACY
	{&LogBase,                          "log.library",           51,    0, 0},
	#endif
/*	  {&UserGroupBase,                    "usergroup.library",      4,    0, 0}, */ /* prevents miami from starting if opened here (if usergroup.library is present on disk (mosnet, netstack)) */
	{&BTreeBase,                        "btree.library",          0,    0, 0},
	{&QueryBase,                        "query.library",          0,    0, 0},
#if USE_THREADPOOL
	{&ThreadPoolBase,                   "threadpool.library",     0,    0, 0},
#endif
	#if USE_SVGICONS
	{&VGraphicsBase,                    "vgraphics.library",      1,    0, 1},
	#endif

	{0, 0, 0, 0, 0} /* terminate */
};

#define EXEC_MINVER 50
#if USE_LEGACY
#define EXEC_MINREV 60
#else
#define EXEC_MINREV 64
#endif

ULONG libs_init(void)
{
	ULONG i;

	/*
	 * Check exec first
	 */
	if (((struct Library *)SysBase)->lib_Version > EXEC_MINVER || (((struct Library *)SysBase)->lib_Version == EXEC_MINVER && ((struct Library *)SysBase)->lib_Revision >= EXEC_MINREV))
	{
		for (i = 0; ld[i].name; i++)
		{
			D(LIB, bug("opening %s\n", ld[i].name));
			if (!((*ld[i].base = OpenLibrary(ld[i].name, ld[i].version)) && ((*ld[i].base)->lib_Version > ld[i].version || !ld[i].revision || ((*ld[i].base)->lib_Version == ld[i].version && (*ld[i].base)->lib_Revision >= ld[i].revision))) && !ld[i].optional)
			{
				if (IntuitionBase)
				{
					if (ld[i].revision && (*ld[i].base)->lib_Version == ld[i].version)
					{
						errorreq("libinit", "Version %ld of %s is correct\nbut you need revision %ld.\nYou have revision %ld.", NULL, ld[i].version, ld[i].name, ld[i].revision, (*ld[i].base)->lib_Revision);
					}
					else
					{
						errorreq("libinit", "Couldn't open %s version %ld or higher.", NULL, ld[i].name, ld[i].version);
					}
				}
				D(LIB, bug("error!\n"));
				return (FALSE);
			}
		}
	}
	#if 0
	else
	{
		/* try to be nice.. */
		IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 0);
		if (IntuitionBase)
		{
			errorreq("Ambient libinit error", "At least version %ld.%ld of exec is needed\n but you have version %ld.%ld.", NULL, EXEC_MINVER, EXEC_MINREV, ((struct Library *)SysBase)->lib_Version, ((struct Library *)SysBase)->lib_Revision);
			CloseLibrary((struct Library *)IntuitionBase);
		}
		return (FALSE);
	}
	#endif
	return (TRUE);
}


void libs_cleanup(void)
{
	LONG i;

	for (i = sizeof(ld) / sizeof(struct libdesc) - 2; i >= 0; i--)
	{
		D(LIB, bug("closing %s\n", ld[i].name));
		if (*ld[i].base)
		{
			CloseLibrary(*ld[i].base);
		}
	}
}
