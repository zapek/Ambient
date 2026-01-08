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
 * $Id: snd_codecs.c,v 1.4 2006/08/08 13:31:36 fab Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "snd_codecs.h"
#include "sndcodec_vorbis.h"
#include "sndcodec_mpega.h"
#include "sndcodec_datatypes.h"
#include "sndcodec_multimedia.h"


struct initdesc {
	#ifdef DEBUG
	STRPTR initname;
	#endif
	ULONG (*initfunc)(void);
	void (*exitfunc)(void);
};

#ifdef DEBUG
#define INITENTRY(name) { #name	, name##_init, name##_cleanup}
#define ENDENTRY {NULL, NULL, NULL}
#else
#define INITENTRY(name) { name##_init, name##_cleanup}
#define ENDENTRY {NULL, NULL}
#endif

static const struct initdesc id[] = {
	#if USE_VORBIS
	INITENTRY(snd_vorbis),
	#endif
	#if USE_MPEGA
	INITENTRY(snd_mpega),
	#endif
	#if USE_MULTIMEDIA
	INITENTRY(snd_multimedia),
	#endif
	#if USE_DATATYPES_SOUND
	INITENTRY(snd_datatypes),
	#endif

	ENDENTRY
};


ULONG snd_codecs_init(void)
{
	ULONG i;

	for (i = 0; id[i].initfunc || id[i].exitfunc; i++)
	{
		if (id[i].initfunc)
		{
			D(SOUND,bug("initializing codec %s..\n", id[i].initname));
			if (!id[i].initfunc())
			{
				D(SOUND,bug("initialization of codec %s failed, bailing out..\n", id[i].initname));
				return (FALSE);
			}
		}
	}
	return (TRUE);
}


void snd_codecs_cleanup(void)
{
	LONG i;

	for (i = sizeof(id) / sizeof(struct initdesc) - 2; i >= 0; i--)
	{
		if (id[i].exitfunc)
		{
			D(SOUND,bug("cleaning up codec %s..\n", id[i].initname));
			id[i].exitfunc();
		}
	}
}
