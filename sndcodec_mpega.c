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
 * $Id: sndcodec_mpega.c,v 1.7 2006/08/08 13:31:36 fab Exp $
 */

#include "ambient.h"

#if USE_MPEGA

/* public */
#include <dos/dos.h>
#include <libraries/mpega.h>
#include <proto/dos.h>
#include <proto/mpega.h>

/* private */
#include "sndcodec_mpega.h"
#include "file_io.h"
#include "sound.h"


#define IOBUFFERSIZE (8192 * 2)

struct Library *MPEGABase;

struct mpega_ctx {
	MPEGA_STREAM *ms;
	UBYTE *pcm[2];
	ULONG pcm_left;
	ULONG pcm_offs;
};


ULONG snd_mpega_init(void)
{
	return (TRUE);
}


void snd_mpega_cleanup(void)
{
	CloseLibrary(MPEGABase);
}


/*
 * Callbacks.
 */
MUI_HOOK(mpegacb, APTR handle, MPEGA_ACCESS *access)
{
	switch (access->func)
	{
		case MPEGA_BSFUNC_OPEN:
			{
				APTR fh;

				if ( (fh = file_open(access->data.open.stream_name, MODE_OLDFILE)) )
				{
					D_S(struct FileInfoBlock, fib);

					if (ExamineFH(file_handle(fh), fib))
					{
						access->data.open.stream_size = fib->fib_Size; /* mpega can't do 64bit anyway... */
					}
				}
				return ((ULONG)fh);
			}
			break;

		case MPEGA_BSFUNC_CLOSE:
			if (handle)
			{
				file_close(handle);
			}
			break;

		case MPEGA_BSFUNC_READ:
			if (handle)
			{
				return (file_readmost(handle, access->data.read.buffer, access->data.read.num_bytes));
			}
			break;

		case MPEGA_BSFUNC_SEEK:
			if (handle)
			{
				if (file_seek(handle, access->data.seek.abs_byte_seek_pos, OFFSET_BEGINNING) == -1LL)
				{
					return (1); /* error */
				}
			}
			break;
	}
	return (0);
}


APTR snd_mpega_open_file(CONST_STRPTR filename)
{
	struct mpega_ctx *ct;

	ASSERT(filename);

	if (!MPEGABase)
	{
		MPEGABase = OpenLibrary("mpega.library", 2);
	}

	if (MPEGABase)
	{
		if ( (ct = malloc(sizeof(*ct))) )
		{
			memset(ct, 0, sizeof(*ct));
			
			if ( (ct->pcm[0] = malloc(MPEGA_PCM_SIZE * 2)) )
			{
				if ( (ct->pcm[1] = malloc(MPEGA_PCM_SIZE * 2)) )
				{
					MPEGA_CTRL mpa_ctrl;

					mpa_ctrl.bs_access                 = &mpegacb_hook;
					mpa_ctrl.layer_1_2.force_mono      = 0;
					mpa_ctrl.layer_1_2.mono.freq_div   = 1;
					mpa_ctrl.layer_1_2.mono.quality    = MPEGA_QUALITY_HIGH;
					mpa_ctrl.layer_1_2.mono.freq_max   = 48000;
					mpa_ctrl.layer_1_2.stereo.freq_div = 1;
					mpa_ctrl.layer_1_2.stereo.quality  = MPEGA_QUALITY_HIGH;
					mpa_ctrl.layer_1_2.stereo.freq_max = 48000;
					mpa_ctrl.layer_3.force_mono        = 0;
					mpa_ctrl.layer_3.mono.freq_div     = 1;
					mpa_ctrl.layer_3.mono.quality      = MPEGA_QUALITY_HIGH;
					mpa_ctrl.layer_3.mono.freq_max     = 48000;
					mpa_ctrl.layer_3.stereo.freq_div   = 1;
					mpa_ctrl.layer_3.stereo.quality    = MPEGA_QUALITY_HIGH;
					mpa_ctrl.layer_3.stereo.freq_max   = 48000;
					mpa_ctrl.check_mpeg                = 1;
					mpa_ctrl.stream_buffer_size        = 8192;

					if ( (ct->ms = MPEGA_open((STRPTR)filename, &mpa_ctrl)) )
					{
						return (ct);
					}
					free(ct->pcm[1]);
				}
				free(ct->pcm[0]);
			}
			free(ct);
		}
	}
	return (NULL);
}


void snd_mpega_close_file(APTR ctx)
{
	struct mpega_ctx *ct = ctx;

	ASSERT(ct);

	MPEGA_close(ct->ms);

	free(ct->pcm[1]);
	free(ct->pcm[0]);
	free(ct);
}


APTR snd_mpega_getattr(APTR ctx, ULONG attr)
{
	struct mpega_ctx *ct = ctx;

	ASSERT(ct);

	switch (attr)
	{
		case MPEGAINFO_VERSION:
			return ((APTR)(ULONG)ct->ms->norm);

		case MPEGAINFO_LAYER:
			return ((APTR)(ULONG)ct->ms->layer);

		case MPEGAINFO_MODE:
			return ((APTR)(ULONG)ct->ms->mode); /* XXX: own defines ? */

		case MPEGAINFO_BITRATE:
			return ((APTR)(ULONG)ct->ms->bitrate);

		case MPEGAINFO_FREQUENCY:
			return ((APTR)ct->ms->frequency);
	
		case MPEGAINFO_CHANNELS:
			return ((APTR)(ULONG)ct->ms->channels);

		case MPEGAINFO_DURATION:
			return ((APTR)ct->ms->ms_duration);

		case MPEGAINFO_PRIVATE:
			return ((APTR)(ULONG)ct->ms->private_bit);

		case MPEGAINFO_COPYRIGHT:
			return ((APTR)(ULONG)ct->ms->copyright);

		case MPEGAINFO_ORIGINAL:
			return ((APTR)(ULONG)ct->ms->original);
	
		#ifdef DEBUG
		default:
			PDB(("nothing for %ld\n", attr));
			break;
		#endif
	}
	return (NULL);
}


#define COPYMPEGBUF \
	({ \
		if (ct->ms->channels == 2) \
		{ \
			((ULONG *)buf)[0] = (*(UWORD *)(ct->pcm[0] + ct->pcm_offs / 2) << 16) | (*(UWORD *)(ct->pcm[1] + ct->pcm_offs / 2)); \
			buf += sizeof (ULONG); \
			len -= sizeof (ULONG); \
			rc += sizeof (ULONG); \
		} \
		else \
		{ \
			*buf++ = ct->pcm[0][ct->pcm_offs / 2]; \
			*buf++ = ct->pcm[0][ct->pcm_offs / 2 + 1]; \
			len -= 2; \
			rc += 2; \
		} \
		ct->pcm_offs += 4; \
		ct->pcm_left--; \
	})


LONG snd_mpega_read(APTR ctx, UBYTE *buf, ULONG len)
{
	LONG rc = 0;
	struct mpega_ctx *ct = ctx;
	LONG cnt;

	/*
	 * Empty a possible previous buffer.
	 */
	if (ct->pcm_left)
	{
		while (len && ct->pcm_left)
		{
			COPYMPEGBUF;
		}
	}

	while (len)
	{
		cnt = MPEGA_decode_frame(ct->ms, (WORD **)ct->pcm);
		
		ct->pcm_offs = 0;

		if (cnt > 0)
		{
			ct->pcm_left = cnt;

			while (len && ct->pcm_left)
			{
				COPYMPEGBUF;
			}
		}
		else
		{
			if (cnt == MPEGA_ERR_BADFRAME || cnt == MPEGA_ERR_NO_SYNC || cnt == MPEGA_ERR_NONE)
			{
				continue;
			}
			else
			{
				rc = cnt;
				break;
			}
		}
	}
	
	if (rc >= 0)
	{
		return (rc);
	}
	else if (rc == -1)
	{
		return (0); /* EOF */
	}
	else
	{
		return (-1);
	}
}

#endif /* USE_MPEGA */
