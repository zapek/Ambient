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
 * $Id: snddrv_ahi.c,v 1.9 2026/01/27 21:28:49 bitrocky Exp $
 */

#include "ambient.h"

/* public */
#include <devices/ahi.h>

/* private */
#include "snddrv_ahi.h"
#include "sound.h"
#include "mui_func.h" /* FORTAG.. */


struct ahidrv_ctx {
	struct MsgPort *mp;
	struct AHIRequest *io;
	struct AHIRequest *ioclone;
	struct AHIRequest *ioa;
	struct AHIRequest *iob;
	struct AHIRequest *link;
	ULONG reqsent;
	LONG pri;
	ULONG frequency;
	ULONG type;
	ULONG volume;
};


APTR snddrv_ahi_open(const struct TagItem *tags)
{
	struct ahidrv_ctx *ct;

	D(SNDDRV,bug("using ahi driver\n"));

	if ( (ct = malloc(sizeof(*ct))) )
	{
		memset(ct, 0, sizeof(*ct));

		ct->mp = CreateMsgPort();

		{
			/* CreateIORequest() fails if ct->mp is NULL */
			if ( (ct->io = (struct AHIRequest *)CreateIORequest(ct->mp, sizeof(*ct->io))) )
			{
				ct->io->ahir_Version = 4;

				if (!OpenDevice(AHINAME, AHI_DEFAULT_UNIT, (struct IORequest *)ct->io, 0))
				{
					ULONG channels = 1;
					ULONG resolution = SOUNDVAL_Resolution_8;

					/* default */
					ct->pri = 80;
					ct->frequency = 8000;
					ct->volume = 0x10000;

					FORTAG(tags)
					{
						case SOUNDTAG_Channels:
							channels = tag->ti_Data;
							break;

						case SOUNDTAG_Resolution:
							resolution = tag->ti_Data;
							break;

						case SOUNDTAG_Frequency:
							ct->frequency = minmax(512, tag->ti_Data, 192000); /* XXX: no clue what to put as max */
							break;

						case SOUNDTAG_Volume:
							ct->volume = minmax(0, tag->ti_Data, 0x10000); /* no amp support yet */
							break;

						case SOUNDTAG_Priority:
							switch (tag->ti_Data)
							{
								case SOUNDVAL_Priority_Background:
									ct->pri = -80;
									break;

								case SOUNDVAL_Priority_Music:
									ct->pri = -50;
									break;

								case SOUNDVAL_Priority_Important:
									ct->pri = 100;
									break;

								case SOUNDVAL_Priority_Maximum:
									ct->pri = 127;
									break;
							}
							break;
					}
					NEXTTAG

					switch (channels)
					{
						case 2:
		                {
							switch (resolution)
							{
								case SOUNDVAL_Resolution_8:
									ct->type = AHIST_S8S;
									break;

								case SOUNDVAL_Resolution_16:
									ct->type = AHIST_S16S;
									break;

								case SOUNDVAL_Resolution_32:
									ct->type = AHIST_S32S;
									break;
							}
						}
						break;

						case 1:
						{
							switch (resolution)
							{
								case SOUNDVAL_Resolution_8:
									ct->type = AHIST_M8S;
									break;

								case SOUNDVAL_Resolution_16:
									ct->type = AHIST_M16S;
									break;

								case SOUNDVAL_Resolution_32:
									ct->type = AHIST_M32S;
									break;
							}
						}
						break;

						#ifdef DEBUG
						default:
							PDB(("unsupported number of channels %ld\n", channels));
							/* XXX */
							break;
						#endif
					}

					/*
					 * Copy the IO request for double buffering.
					 */
					if ( (ct->ioclone = malloc(sizeof(*ct->ioclone))) )
					{
						memcpy(ct->ioclone, ct->io, sizeof(*ct->io));

						D(SNDDRV,bug("pri: %ld\n", ct->pri));
						D(SNDDRV,bug("frequency: %ld\n", ct->frequency));
						D(SNDDRV,bug("type: %ld\n", ct->type));
						D(SNDDRV,bug("volume: %ld\n", ct->volume));

						ct->ioa = ct->io;
						ct->iob = ct->ioclone;

						return (ct);
					}
				}

				DeleteIORequest((struct IORequest *)ct->io);
			}
			DeleteMsgPort(ct->mp);
		}
		free (ct);
	}
	return (NULL);
}


void snddrv_ahi_close(APTR ctx)
{
	struct ahidrv_ctx *ct = ctx;

	ASSERT(ct);

	D(SNDDRV,bug("closing unit for ctx %p\n", ct));
	snddrv_ahi_abort(ct);
	free(ct->ioclone);

	CloseDevice((struct IORequest *)ct->io);
	DeleteIORequest((struct IORequest *)ct->io);
	DeleteMsgPort(ct->mp);

	D(SNDDRV,bug("unit for ctx %p closed\n", ct));
	free(ct);
}


/*
 * Sends a buffer to play, returns as soon as possible and
 * you might not reuse 'buf' before the 2nd request is sent.
 * Then you can alternate.
 * Returns TRUE or FALSE.
 */
LONG snddrv_ahi_write_async(APTR ctx, CONST_APTR buf, ULONG len)
{
	struct ahidrv_ctx *ct = ctx;

	ASSERT(ct);

	ct->ioa->ahir_Std.io_Message.mn_Node.ln_Pri = ct->pri;
	ct->ioa->ahir_Std.io_Command = CMD_WRITE;
	ct->ioa->ahir_Std.io_Data    = (APTR)buf;
	ct->ioa->ahir_Std.io_Length  = len;
	ct->ioa->ahir_Std.io_Offset  = 0;
	ct->ioa->ahir_Frequency      = ct->frequency;
	ct->ioa->ahir_Type           = ct->type;
	ct->ioa->ahir_Volume         = ct->volume;
	ct->ioa->ahir_Position       = 0x8000; /* centered */
	ct->ioa->ahir_Link           = ct->link;

	SendIO((struct IORequest *)ct->ioa);

	ct->reqsent++;

	D(SNDDRV,bug("sent request: %p, buf: %p, length: %ld\n", ct->ioa, buf, len));

	if (ct->link)
	{
		/* wait for the last buffer to empty */
		if (WaitIO((struct IORequest *)ct->link))
		{
			/* I/O error */
			return (FALSE);
		}
	}
	ct->link = ct->ioa;

	swap(ct->ioa, ct->iob);

	return (TRUE);
}


ULONG snddrv_ahi_wait(APTR ctx)
{
	struct ahidrv_ctx *ct = ctx;

	ASSERT(ct);

	if (ct->link)
	{
		D(SNDDRV,bug("waiting for request %p..\n", ct->link));
		if (WaitIO((struct IORequest *)ct->link))
		{
			return (FALSE);
		}
	}
	return (TRUE);
}


ULONG snddrv_ahi_check(APTR ctx)
{
	struct ahidrv_ctx *ct = ctx;

	ASSERT(ct);

	if (ct->link)
	{
		if (!CheckIO((struct IORequest *)ct->link))
		{
			return (ct->link->ahir_Std.io_Length - ct->link->ahir_Std.io_Actual);
		}
	}
	return (0);
}


void snddrv_ahi_abort(APTR ctx)
{
	struct ahidrv_ctx *ct = ctx;

	ASSERT(ct);

	if (ct->reqsent)
	{
		if (ct->link)
		{
			D(SNDDRV,bug("waiting linked request %p..\n", ct->link));
			if (!CheckIO((struct IORequest *)ct->link))
			{
				D(SNDDRV,bug("pending request.. aborting..\n"));
				AbortIO((struct IORequest *)ct->link);
			}
			WaitIO((struct IORequest *)ct->link);
		}

		if (ct->reqsent >= 2)
		{
			if (ct->link == ct->ioa)
			{
				ct->link = ct->iob;
			}
			else
			{
				ct->link = ct->ioa;
			}

			D(SNDDRV,bug("waiting request %p..\n", ct->link));
			if (!CheckIO((struct IORequest *)ct->link))
			{
				D(SNDDRV,bug("pending request.. aborting..\n"));
				AbortIO((struct IORequest *)ct->link);
			}
			WaitIO((struct IORequest *)ct->link);
		}
		ct->reqsent = 0;

		GetMsg(ct->mp);
		GetMsg(ct->mp);
	}

	D(SNDDRV,bug("aborted everything\n"));
}
