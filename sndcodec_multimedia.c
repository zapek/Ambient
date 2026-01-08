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
 * $Id: sndcodec_multimedia.c,v 1.7 2017/08/11 23:32:05 cyfm Exp $
 */

#include "ambient.h"

#if USE_MULTIMEDIA

/* public */
#include <datatypes/soundclass.h>
#include <classes/multimedia/sound.h>
#include <proto/datatypes.h>
#include <proto/intuition.h>
#include <proto/multimedia.h>

/* private */
#include "sndcodec_multimedia.h"
#include "sound.h"
#include "multimedia.h"
#include "mui_func.h" /* XXX: for getv() */


struct multimedia_ctx {
	APTR obj;
	ULONG port;
	float *buf;
	ULONG buflen;
	ULONG framesize;
	ULONG channels;
	ULONG is16bit;
};


ULONG snd_multimedia_init(void)
{
	return (TRUE);
}


void snd_multimedia_cleanup(void)
{
}


APTR snd_multimedia_open_file(CONST_STRPTR filename)
{
	struct multimedia_ctx *ct;

	ASSERT(filename);

	if (multimedia_open())
	{
		if ((ct = malloc(sizeof(*ct))))
		{
			#if 1
			struct TagItem tags[6];
			#endif
			memclr(ct, sizeof(*ct));

			#if 1
			/* XXX: needs support in cvinclude.pl .. perl drives me mad.. ask Emm for base,sysv support in it */
			tags[0].ti_Tag  = MMA_MediaType;
			tags[0].ti_Data = MMT_SOUND;
			tags[1].ti_Tag  = MMA_StreamType;
			tags[1].ti_Data = (ULONG)"file.stream";
			tags[2].ti_Tag  = MMA_StreamName;
			tags[2].ti_Data = (ULONG)filename;
			tags[3].ti_Tag  = MMA_Recognition;
			tags[3].ti_Data = MMREC_HEAVY;
			tags[4].ti_Tag = MMA_TaskPriority;
			tags[4].ti_Data = 10;
			tags[5].ti_Tag  = TAG_DONE;

			if ((ct->obj = MediaNewObjectTagList(tags)))
			#else
			if (ct->obj = MediaNewObjectTags(MMA_MediaType, MMT_SOUND,
										 MMA_StreamType, (ULONG)"file",
										 MMA_StreamName, (ULONG)filename,
										 MMA_Recognition, MMREC_HEAVY,
										 //MMA_ErrorCode, (ULONG)&error, /* XXX */
			 TAG_DONE))
			#endif
			{
				ULONG *fmt;
				
				ct->port = 0;
				ct->channels = MediaGetPort(ct->obj, ct->port, MMA_Sound_Channels);

				/*
				 * Try to get 16-bit output
				 * if the decoder supports it.
				 */
				if ((fmt = (ULONG *)MediaGetPort(ct->obj, ct->port, MMA_Port_FormatsTable)))
				{
					while (*fmt)
					{
						if (*fmt == MMFC_AUDIO_INT16)
						{
							ct->is16bit = TRUE;
							MediaSetPort(ct->obj, ct->port, MMA_Port_Format, MMFC_AUDIO_INT16); /* XXX */
							break;
						}
						fmt++;
					}
				}

				if (!ct->is16bit)
				{
					MediaSetPort(ct->obj, ct->port, MMA_Port_Format, MMFC_AUDIO_FLOAT32);
				}
				return (ct);
			}
		}
		multimedia_close();
		free(ct);
	}
	/* XXX */
	return (NULL);
}


void snd_multimedia_close_file(APTR ctx)
{
	struct multimedia_ctx *ct = ctx;

	ASSERT(ct);
	ASSERT(ct->obj);

	DisposeObject(ct->obj);
	multimedia_close();

	free(ct);
}


APTR snd_multimedia_getattr(APTR ctx, ULONG attr)
{
	struct multimedia_ctx *ct = ctx;

	ASSERT(ct);

	if (ct->obj)
	{
		switch (attr)
		{
			case MULTIMEDIAINFO_CHANNELS:
				return ((APTR)ct->channels);

			case MULTIMEDIAINFO_FREQUENCY:
				return ((APTR)MediaGetPort(ct->obj, ct->port, MMA_Sound_SampleRate));

			#ifdef DEBUG
			default:
				PDB(("nothing for 0x%lx\n", attr));
				break;
			#endif
		}
	}
	return (NULL);
}


LONG snd_multimedia_read(APTR ctx, UBYTE *buf, ULONG len)
{
	struct multimedia_ctx *ct = ctx;
	ULONG totsize = len * 2;
	ULONG val;

	ASSERT(ct);
	ASSERT(buf);
	ASSERT(len);

	if (ct->is16bit)
	{
		if ((val = DoMethod(ct->obj, MMM_Pull, ct->port, buf, len)))
		{
			return (val);
		}
	}
	else
	{
		/*
		 * 32-bit float samples.
		 */
		if (ct->buflen < totsize)
		{
			if (ct->buf)
			{
				free(ct->buf);
			}
			ct->buf = malloc(totsize);
			ct->buflen = totsize;
		}

		if (ct->buf)
		{
			if ((val = DoMethod(ct->obj, MMM_Pull, ct->port, ct->buf, len)))
			{
				ULONG cnt;
				ULONG size = val / sizeof(float);
				float *sp = ct->buf;
				WORD *dp = (WORD *)buf;

				cnt = size;

				while (size--)
				{
					*dp++ = (WORD)(*sp++ * 32767.0);
				}
				return (cnt * sizeof(WORD));
			}
		}
	}
	return (0);
}

#endif /* USE_MULTIMEDIA */
