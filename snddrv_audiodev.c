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
 * $Id: snddrv_audiodev.c,v 1.7 2006/08/08 13:31:36 fab Exp $
 */

#include "ambient.h"

/* public */
#include <devices/audio.h>

/* private */
#include "snddrv_audiodev.h"
#include "sound.h"
#include "mui_func.h" /* FORTAG.. */

#define LEFT 0
#define RIGHT 1

struct audiodevdrv_ctx {
	struct IOAudio *io[2][2];
	struct MsgPort *mp[2][2];
	ULONG frequency;
	ULONG channels;
	ULONG resolution;
	ULONG volume;
	UBYTE *buf[2];
	ULONG buflen[2];
	ULONG dbufnum;
	ULONG reqsent;
};

#define BEGINIOPRI 70

APTR snddrv_audiodev_open(const struct TagItem *tags)
{
	struct audiodevdrv_ctx *ct;

	D(SNDDRV,bug("using audiodev driver\n"));

	if ( (ct = malloc(sizeof(*ct))) )
	{
		memset(ct, 0, sizeof(*ct));

		/*
		 * audio.device rules. 4 signals wasted
		 * to play a stereo sound.
		 *
		 * I really think these should be combined... - piru
		 */
		if ( (ct->mp[LEFT][0] = CreateMsgPort()) )
		{
			if ( (ct->mp[LEFT][1] = CreateMsgPort()) )
			{
				if ( (ct->mp[RIGHT][0] = CreateMsgPort()) )
				{
					if ( (ct->mp[RIGHT][1] = CreateMsgPort()) )
					{
						if ( (ct->io[LEFT][0] = (struct IOAudio *)CreateIORequest(ct->mp[LEFT][0], sizeof(*ct->io[LEFT][0]))) )
						{
							if ( (ct->io[RIGHT][0] = (struct IOAudio *)CreateIORequest(ct->mp[RIGHT][0], sizeof(*ct->io[RIGHT][0]))) )
							{
								STATIC CONST UBYTE chans0[] = { 2, 4 }; /* left */
								STATIC CONST UBYTE chans1[] = { 1, 8 }; /* right */

								/* default */
								ct->io[LEFT][0]->ioa_Request.io_Message.mn_Node.ln_Pri = 80;
								ct->io[RIGHT][0]->ioa_Request.io_Message.mn_Node.ln_Pri = 80;
								ct->frequency = 8000;
								ct->volume = 63;

								FORTAG(tags)
								{
									case SOUNDTAG_Channels:
										ct->channels = tag->ti_Data;
										break;

									case SOUNDTAG_Resolution:
										ct->resolution = tag->ti_Data;
										break;

									case SOUNDTAG_Frequency:
										ct->frequency = minmax(512, tag->ti_Data, 120000); /* XXX: not sure about the maximum for audio.device.. hm.. */
										break;

									case SOUNDTAG_Volume:
										ct->volume = minmax(0, tag->ti_Data, 63);
										break;

									case SOUNDTAG_Priority:
										switch (tag->ti_Data)
										{
											case SOUNDVAL_Priority_Background:
												ct->io[LEFT][0]->ioa_Request.io_Message.mn_Node.ln_Pri  = -80;
												ct->io[RIGHT][0]->ioa_Request.io_Message.mn_Node.ln_Pri = -80;
												break;

											case SOUNDVAL_Priority_Music:
												ct->io[LEFT][0]->ioa_Request.io_Message.mn_Node.ln_Pri  = -50;
												ct->io[RIGHT][0]->ioa_Request.io_Message.mn_Node.ln_Pri = -50;
												break;

											case SOUNDVAL_Priority_Important:
												ct->io[LEFT][0]->ioa_Request.io_Message.mn_Node.ln_Pri  = 100;
												ct->io[RIGHT][0]->ioa_Request.io_Message.mn_Node.ln_Pri = 100;
												break;

											case SOUNDVAL_Priority_Maximum:
												ct->io[LEFT][0]->ioa_Request.io_Message.mn_Node.ln_Pri  = 127;
												ct->io[RIGHT][0]->ioa_Request.io_Message.mn_Node.ln_Pri = 127;
												break;
										}
										break;
								}
								NEXTTAG

								ct->io[LEFT][0]->ioa_Request.io_Command = ADCMD_ALLOCATE;
								ct->io[LEFT][0]->ioa_Request.io_Flags   = ADIOF_NOWAIT;
								ct->io[LEFT][0]->ioa_AllocKey = 0;
								ct->io[LEFT][0]->ioa_Data     = (APTR)chans0;
								ct->io[LEFT][0]->ioa_Length   = sizeof(chans0);

								if (!OpenDevice(AUDIONAME, 0L, (struct IORequest *)ct->io[LEFT][0], 0))
								{
									ct->io[RIGHT][0]->ioa_Request.io_Command = ADCMD_ALLOCATE;
									ct->io[RIGHT][0]->ioa_Request.io_Flags   = ADIOF_NOWAIT;
									ct->io[RIGHT][0]->ioa_AllocKey = 0;
									ct->io[RIGHT][0]->ioa_Data     = (APTR)chans1;
									ct->io[RIGHT][0]->ioa_Length   = sizeof(chans1);

									if (!OpenDevice(AUDIONAME, 0L, (struct IORequest *)ct->io[RIGHT][0], 0))
									{
										if ( (ct->io[LEFT][1] = malloc(sizeof(*ct->io[LEFT][1]))) )
										{
											memcpy(ct->io[LEFT][1], ct->io[LEFT][0], sizeof(*ct->io[LEFT][1]));

											ct->io[LEFT][1]->ioa_Request.io_Message.mn_ReplyPort = ct->mp[LEFT][1];

											if ( (ct->io[RIGHT][1] = malloc(sizeof(*ct->io[RIGHT][1]))) )
											{
												memcpy(ct->io[RIGHT][1], ct->io[RIGHT][0], sizeof(*ct->io[RIGHT][1]));

												ct->io[RIGHT][1]->ioa_Request.io_Message.mn_ReplyPort = ct->mp[RIGHT][1];

												D(SNDDRV,bug("pri: %ld\n", (LONG)ct->io[LEFT][0]->ioa_Request.io_Message.mn_Node.ln_Pri));
												D(SNDDRV,bug("frequency: %ld\n", ct->frequency));
												D(SNDDRV,bug("resolution: %ld\n", ct->resolution));
												D(SNDDRV,bug("channels: %ld\n", ct->channels));
												D(SNDDRV,bug("volume: %ld\n", ct->volume));

												return (ct);
											}
											free(ct->io[LEFT][1]);
										}
										CloseDevice((struct IORequest *)ct->io[RIGHT][0]);
									}
									CloseDevice((struct IORequest *)ct->io[LEFT][0]);
								}
								DeleteIORequest(ct->io[RIGHT][0]);
							}
							DeleteIORequest(ct->io[LEFT][0]);
						}
						DeleteMsgPort(ct->mp[RIGHT][1]);
					}
					DeleteMsgPort(ct->mp[RIGHT][0]);
				}
				DeleteMsgPort(ct->mp[LEFT][1]);
			}
			DeleteMsgPort(ct->mp[LEFT][0]);
		}
		free(ct);
	}
	return (NULL);
}


void snddrv_audiodev_close(APTR ctx)
{
	struct audiodevdrv_ctx *ct = ctx;

	ASSERT(ct);

	D(SNDDRV,bug("closing units for ctx %p\n", ct));
	snddrv_audiodev_abort(ct);

	if (ct->buf[RIGHT])
	{
		FreeVec(ct->buf[RIGHT]);
	}
	if (ct->buf[LEFT])
	{
		FreeVec(ct->buf[LEFT]);
	}

	free(ct->io[RIGHT][1]);
	free(ct->io[LEFT][1]);

	CloseDevice((struct IORequest *)ct->io[RIGHT][0]);
	CloseDevice((struct IORequest *)ct->io[LEFT][0]);

	DeleteIORequest((struct IORequest *)ct->io[RIGHT][0]);
	DeleteIORequest((struct IORequest *)ct->io[LEFT][0]);

	DeleteMsgPort(ct->mp[RIGHT][0]);
	DeleteMsgPort(ct->mp[LEFT][0]);
	DeleteMsgPort(ct->mp[RIGHT][1]);
	DeleteMsgPort(ct->mp[LEFT][1]);

	D(SNDDRV,bug("unit for ctx %p closed\n", ct));
	free (ct);
}


LONG snddrv_audiodev_write_async(APTR ctx, CONST_APTR _buf, ULONG len)
{
	CONST UBYTE *buf = _buf;
	ULONG buflen = buflen; /* shut up gcc */
	ULONG i;
	ULONG mul = mul; /* shut up gcc */
	struct audiodevdrv_ctx *ct = ctx;

	ASSERT(ct);

	/*
	 * buflen is multiplied by 2 because I use
	 * it for both channels. Multiplication is included
	 * in the following. But since audio.device doesn't
	 * support a period < 124 I divide by 2.
	 * XXX: mono probably doesn't work!
	 */
	switch (ct->resolution)
	{
		case SOUNDVAL_Resolution_8:
			buflen = len / 2;
			break;

		case SOUNDVAL_Resolution_16:
			buflen = len / 4;
			break;

		case SOUNDVAL_Resolution_32:
			buflen = len / 8;
			break;

		#ifdef DEBUG
		default:
			PDB(("unsupported resolution %ld\n", ct->resolution));
			break;
		#endif
	}

	if (ct->buflen[ct->dbufnum] < buflen)
	{
		if (ct->buf[ct->dbufnum])
		{
			FreeVec(ct->buf[ct->dbufnum]);
		}

		if ( (ct->buf[ct->dbufnum] = AllocVec(buflen, MEMF_CHIP)) ) /* whot's an omega? */
		{
			ct->buflen[ct->dbufnum] = buflen;
		}
	}

	/*
	 * Convert to sucky 8-bit format.
	 * This is unoptimized on purpose.
	 */
	if (ct->buf[ct->dbufnum])
	{
		struct Task *me = FindTask(NULL);
		BYTE oldpri;

		switch (ct->resolution)
		{
			case SOUNDVAL_Resolution_8:
				mul = 1;
				break;

			case SOUNDVAL_Resolution_16:
				mul = 2;
				break;

			case SOUNDVAL_Resolution_32:
				mul = 4;
				break;

			#ifdef DEBUG
			default:
				PDB(("fasjklfdsj\n"));
				break;
			#endif
		}

		/*
		 * audio.device doesn't support periods
		 * smaller than 124 so we resample by dividing
		 * the frequency by 2.
		 */
		for (i = 0; i < buflen / 2; i++)
		{
			ct->buf[ct->dbufnum][i] = buf[i * ct->channels * 2 * mul];
			if (ct->channels == 1)
			{
				ct->buf[ct->dbufnum][i + ct->buflen[ct->dbufnum] / 2] = buf[i * ct->channels * 2 * mul];
			}
			else /* assume 2 */
			{
				ct->buf[ct->dbufnum][i + ct->buflen[ct->dbufnum] / 2] = buf[i * ct->channels * 2 * mul + mul];
			}
		}

		ct->io[LEFT][ct->dbufnum]->ioa_Request.io_Command = CMD_WRITE;
		ct->io[LEFT][ct->dbufnum]->ioa_Request.io_Flags   = ADIOF_PERVOL;
		ct->io[LEFT][ct->dbufnum]->ioa_Data   = ct->buf[ct->dbufnum];
		ct->io[LEFT][ct->dbufnum]->ioa_Length = buflen / 2;
		ct->io[LEFT][ct->dbufnum]->ioa_Period = 3546895 / (ct->frequency / 2);
		ct->io[LEFT][ct->dbufnum]->ioa_Volume = ct->volume;
		ct->io[LEFT][ct->dbufnum]->ioa_Cycles = 1;

		ct->io[RIGHT][ct->dbufnum]->ioa_Request.io_Command = CMD_WRITE;
		ct->io[RIGHT][ct->dbufnum]->ioa_Request.io_Flags   = ADIOF_PERVOL;
		ct->io[RIGHT][ct->dbufnum]->ioa_Data   = ct->buf[ct->dbufnum] + ct->buflen[ct->dbufnum] / 2;
		ct->io[RIGHT][ct->dbufnum]->ioa_Length = buflen / 2;
		ct->io[RIGHT][ct->dbufnum]->ioa_Period = 3546895 / (ct->frequency / 2);
		ct->io[RIGHT][ct->dbufnum]->ioa_Volume = ct->volume;
		ct->io[RIGHT][ct->dbufnum]->ioa_Cycles = 1;

		/*
		 * Try to get both requests to be sent at
		 * the same time to get stereo and not
		 * surround sound.
		 */
		oldpri = SetTaskPri(me, BEGINIOPRI);
		BeginIO((struct IORequest *)ct->io[LEFT][ct->dbufnum]);
		BeginIO((struct IORequest *)ct->io[RIGHT][ct->dbufnum]);
		SetTaskPri(me, oldpri);

		ct->reqsent = TRUE;

		D(SNDDRV,bug("sent requests: %p (l) %p (r), length: %ld\n", ct->io[LEFT][ct->dbufnum], ct->io[RIGHT][ct->dbufnum], ct->buflen[ct->dbufnum] / 2));

		ct->dbufnum = (ct->dbufnum + 1) % 2;

		if (ct->buf[1]) /* there was a first run */
		{
			/* wait for the other to complete */
			if (WaitIO((struct IORequest *)ct->io[LEFT][ct->dbufnum])) return (FALSE);
			if (WaitIO((struct IORequest *)ct->io[RIGHT][ct->dbufnum])) return (FALSE);
		}
		return (TRUE);
	}
	return (FALSE);
}


ULONG snddrv_audiodev_wait(APTR ctx)
{
	struct audiodevdrv_ctx *ct = ctx;

	ASSERT(ct);

	if (ct->buf[1])
	{
		D(SNDDRV,bug("waiting for request..\n"));
		if (WaitIO((struct IORequest *)ct->io[LEFT][ct->dbufnum])) return (FALSE);
		if (WaitIO((struct IORequest *)ct->io[RIGHT][ct->dbufnum])) return (FALSE);
	}
	return (TRUE);
}


ULONG snddrv_audiodev_check(APTR ctx)
{
	struct audiodevdrv_ctx *ct = ctx;

	ASSERT(ct);

	/*
	 * No way of knowing where the buffer
	 * is with audio.device.. so we cheat.
	 */
	if (ct->buf[1])
	{
		if (!CheckIO((struct IORequest *)ct->io[LEFT][ct->dbufnum])) return (1);
		if (!CheckIO((struct IORequest *)ct->io[RIGHT][ct->dbufnum])) return (1);
	}
	return (0);
}


void snddrv_audiodev_abort(APTR ctx)
{
	struct audiodevdrv_ctx *ct = ctx;

	ASSERT(ct);

	if (ct->reqsent)
	{
		ULONG dbufnum;

		if (ct->buf[1])
		{
			D(SNDDRV,bug("waiting linked request..\n"));
			if (!CheckIO((struct IORequest *)ct->io[LEFT][ct->dbufnum]))
			{
				D(SNDDRV,bug("pending request (left).. aborting..\n"));
				AbortIO((struct IORequest *)ct->io[LEFT][ct->dbufnum]);
			}
			if (!CheckIO((struct IORequest *)ct->io[RIGHT][ct->dbufnum]))
			{
				D(SNDDRV,bug("pending request (right).. aborting..\n"));
				AbortIO((struct IORequest *)ct->io[RIGHT][ct->dbufnum]);
			}
			WaitIO((struct IORequest *)ct->io[LEFT][ct->dbufnum]);
			WaitIO((struct IORequest *)ct->io[RIGHT][ct->dbufnum]);
		}

		dbufnum = (ct->dbufnum + 1) % 2;

		D(SNDDRV,bug("waiting request..\n"));
		if (!CheckIO((struct IORequest *)ct->io[LEFT][dbufnum]))
		{
			D(SNDDRV,bug("pending request (left).. aborting..\n"));
			AbortIO((struct IORequest *)ct->io[LEFT][dbufnum]);
		}
		if (!CheckIO((struct IORequest *)ct->io[RIGHT][dbufnum]))
		{
			D(SNDDRV,bug("pending request (right).. aborting..\n"));
			AbortIO((struct IORequest *)ct->io[RIGHT][dbufnum]);
		}
		WaitIO((struct IORequest *)ct->io[LEFT][dbufnum]);
		WaitIO((struct IORequest *)ct->io[RIGHT][dbufnum]);

		ct->reqsent = FALSE;
	}

	D(SNDDRV,bug("aborted everything\n"));
}
