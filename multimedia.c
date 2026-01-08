/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2015 Ambient Open Source Team
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
 * $Id: multimedia.c,v 1.14 2015/08/14 19:09:20 itix Exp $
 */

#include "ambient.h"

#if USE_MULTIMEDIA
#define USE_INLINE_STDARG

/* public */
#include <proto/multimedia.h>

/* private */
#include "multimedia.h"
#include "mui_func.h" /* for getv().. */


struct Library *MultimediaBase;

#warning multimedia_init() should be called?
ULONG multimedia_init(void)
{
	return (TRUE);
}


void multimedia_cleanup(void)
{
	/* zZz */
}


ULONG multimedia_open(void)
{
	struct Library *lib;
	ULONG rc = FALSE;

		/*
		 *  [krashan] multimedia.class is now v52
		 */

		MultimediaBase = lib = OpenLibrary("multimedia/multimedia.class", 55);

		if (lib)
		{
			if (lib->lib_Version > 55 || (lib->lib_Version == 55 && lib->lib_Revision >= 9))
			{
				MultimediaBase = lib;
				rc = TRUE;
			}
			else
			{
				CloseLibrary(lib);
			}
		}

	return (rc);
}


void multimedia_close(void)
{
	CloseLibrary(MultimediaBase);
}

ULONG multimedia_findtype(STRPTR path)
{
	APTR obj;
	ULONG type = MULTIMEDIATYPE_NONE;

	THREAD;

	if (multimedia_open())
	{
		#if 1
		struct TagItem tags[5];

		tags[0].ti_Tag = MMA_StreamType;
		tags[0].ti_Data = (ULONG)"file.stream";
		tags[1].ti_Tag = MMA_StreamName;
		tags[1].ti_Data = (ULONG)path;
		tags[2].ti_Tag = MMA_Recognition;
		tags[2].ti_Data = MMREC_HEAVY; /* XXX: I should tune that.. not sure MMREC_HEAVY is right.. */
		tags[3].ti_Tag = MMA_Decode;   /* turn off decoding */
		tags[3].ti_Data = FALSE;
		tags[4].ti_Tag = TAG_DONE;

		if ((obj = MediaNewObjectTagList(tags)))
		#else
		if (obj = MediaNewObjectTags(MMA_StreamType, (ULONG)"file.stream",
									 MMA_StreamName, (ULONG)path,
									 MMA_Recognition, MMREC_HEAVY,
		TAG_DONE))
		#endif
		{ 
			LONG mediatype = getv(obj, MMA_MediaType);
			if (mediatype & MMT_SOUND)
				type = MULTIMEDIATYPE_SOUND;
			else if  ((mediatype & MMT_VIDEO)|| (mediatype & MMT_PICTURE))
					type = MULTIMEDIATYPE_VIDEO;

			DisposeObject(obj);
		}
		multimedia_close();
	}
	return (type);
}


STRPTR multimedia_nametype(ULONG type)
{
	switch (type)
	{
		case MULTIMEDIATYPE_SOUND:
			return ("sound");

		case MULTIMEDIATYPE_VIDEO:
			return ("picture");

		case MULTIMEDIATYPE_NONE:
			return ("unknown");

		#ifndef DEBUG
		default:
			PDB(("unknown type\n"));
			break;
		#endif
	}
	return (NULL);
}

#endif
