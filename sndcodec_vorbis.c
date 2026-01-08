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
 * $Id: sndcodec_vorbis.c,v 1.8 2020/02/22 12:34:59 piru Exp $
 */

#include "ambient.h"

#if USE_VORBIS

/* public */
#include <libraries/vorbisfile.h>
#include <proto/vorbisfile.h>

/* private */
#include "sndcodec_vorbis.h"
#include "file_io.h"
#include "sound.h"


#define IOBUFFERSIZE (8192 * 2)

struct Library *VorbisFileBase; /* I guess the library can be shared amongst processes */

struct vorbis_ctx {
	OggVorbis_File vf;
	int bitstream;
	long eof_or_error;
};
#define NO_EOF_OR_ERROR 1


ULONG snd_vorbis_init(void)
{
	return (TRUE);
}


void snd_vorbis_cleanup(void)
{
	CloseLibrary(VorbisFileBase);
}


/*
 * Callbacks.
 */
static size_t vorbiscb_fread(void *buf, size_t size, size_t number, void *file)
{
	LONG retval;

	retval = file_readmost(file, buf, size * number);

	if (retval < 0)
	{
		return (0);
	}
	else
	{
		return (retval / size);
	}
}

static int vorbiscb_fseek(void *file, ogg_int64_t offset, int base)
{
	long long int retval = 0;

	if (file)
	{
		switch (base)
		{
			case SEEK_SET:
				retval = file_seek(file, offset, OFFSET_BEGINNING);
				break;

			case SEEK_CUR:
				retval = file_seek(file, offset, OFFSET_CURRENT);
				break;

			case SEEK_END:
				retval = file_seek(file, offset, OFFSET_END);
				break;
		}

		if (retval >= 0)
		{
			return (0);
		}
	}
	return (-1);
}

static int vorbiscb_fclose(void *file)
{
	file_close(file);
	return (0);
}

static long vorbiscb_ftell(void *file)
{
	/* XXX: doesn't work right with >2G files */
	return (file_seek(file, 0, OFFSET_CURRENT));
}


APTR snd_vorbis_open_file(CONST_STRPTR filename)
{
	struct vorbis_ctx *ct;

	ASSERT(filename);

	if (!VorbisFileBase)
	{
		VorbisFileBase = OpenLibrary("vorbisfile.library", 1);
	}

	if (VorbisFileBase)
	{
		if ((ct = malloc(sizeof(*ct))))
		{
			APTR fh;

			memset(ct, 0, sizeof(*ct));
			ct->eof_or_error = NO_EOF_OR_ERROR;

			if ((fh = file_open(filename, MODE_OLDFILE)))
			{
				static const ov_callbacks cb = {
					vorbiscb_fread,
					vorbiscb_fseek,
					vorbiscb_fclose,
					vorbiscb_ftell
				};

				if (ov_open_callbacks(fh, &ct->vf, NULL, 0, (ov_callbacks *)&cb) == 0)
				{
					return (ct);
				}
				file_close(fh);
			}
			free(ct);
		}
	}
	/* XXX */
	return (NULL);
}


void snd_vorbis_close_file(APTR ctx)
{
	struct vorbis_ctx *ct = ctx;

	ASSERT(ct);

	ov_clear(&ct->vf); /* that call closes the file handle */

	free(ct);
}


APTR snd_vorbis_getattr(APTR ctx, ULONG attr)
{
	struct vorbis_ctx *ct = ctx;
	vorbis_info *vi;

	ASSERT(ct);

	vi = ov_info(&ct->vf, -1);

	if (vi)
	{
		switch (attr)
		{
			case VORBISINFO_VERSION:
				return ((APTR)vi->version);

			case VORBISINFO_CHANNELS:
				return ((APTR)vi->channels);

			case VORBISINFO_FREQUENCY:
				return ((APTR)vi->rate);

			case VORBISINFO_BITRATE_UPPER:
				return ((APTR)vi->bitrate_upper);

			case VORBISINFO_BITRATE_NOMINAL:
				return ((APTR)vi->bitrate_nominal);

			case VORBISINFO_BITRATE_LOWER:
				return ((APTR)vi->bitrate_lower);

			case VORBISINFO_BITRATE_WINDOW:
				return ((APTR)vi->bitrate_window);
		
			#ifdef DEBUG
			default:
				PDB(("nothing for 0x%lx\n", attr));
				break;
			#endif
		}
	}
	return (NULL);
}


LONG snd_vorbis_read(APTR ctx, UBYTE *buf, ULONG len)
{
	LONG l = 0;
	struct vorbis_ctx *ct = ctx;

	ASSERT(ct);
	ASSERT(buf);
	ASSERT(len);

	if (ct->eof_or_error != NO_EOF_OR_ERROR)
	{
		return (LONG) ct->eof_or_error;
	}

	while (len)
	{
		/*
		 * ov_read() returns what it wants
		 * so we need to call it multiple times
		 */
		long r = ov_read(&ct->vf, (char *)(buf + l), len, 1, 2, 1, &ct->bitstream);

		if (r == OV_HOLE) continue;

		if (r <= 0)
		{
			ct->eof_or_error = r;
			if (r < 0)
				return (-1); /* error */
			break; /* EOF */
		}

		l += r;
		len -= r;
	}
	return (l);
}


#endif /* USE_VORBIS */
