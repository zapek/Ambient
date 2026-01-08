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
 * $Id: sndcodec_datatypes.c,v 1.8 2017/08/11 23:32:05 cyfm Exp $
 */

#include "ambient.h"

#if USE_DATATYPES_SOUND

/* public */
#include <datatypes/soundclass.h>
#include <proto/datatypes.h>
#include <proto/intuition.h>

/* private */
#include "sndcodec_datatypes.h"
#include "mui_func.h" /* for DoMethod().. */
#include "sound.h"
#include "legacy.h"


struct datatypes_ctx {
	APTR obj;
	ULONG sampletype;
};


ULONG snd_datatypes_init(void)
{
	return (TRUE);
}


void snd_datatypes_cleanup(void)
{
	/* zZz */
}


APTR snd_datatypes_open_file(CONST_STRPTR filename)
{
	struct datatypes_ctx *ct;

	if ((ct = malloc(sizeof(*ct))))
	{
		memset(ct, 0, sizeof(*ct));

		if ((ct->obj = NewDTObject((STRPTR)filename,
		                           DTA_GroupID, GID_SOUND,
		                           SDTA_Mode, SDTA_Mode_Extended,
		                           TAG_DONE))
		)
		{
			if (GetAttr(SDTA_SampleType, ct->obj, &ct->sampletype))
			{
				switch (ct->sampletype)
				{
					case SDTST_M8S:
					case SDTST_M16S:
					case SDTST_S8S:
					case SDTST_S16S:
						return (ct);

					default:
						PDB(("unhandled format\n")); /* XXX */
						break;
				}
			}
			else
			{
				PDB(("datatype doesn't support sampletype\n")); /* XXX */
			}
		}
		free(ct);
	}
	return (NULL);
}


void snd_datatypes_close_file(APTR ctx)
{
	struct datatypes_ctx *ct = ctx;

	ASSERT(ct);
	ASSERT(ct->obj);

	DisposeDTObject(ct->obj);

	free(ct);
}


APTR snd_datatypes_getattr(APTR ctx, ULONG attr)
{
	struct datatypes_ctx *ct = ctx;
	ULONG v;

	ASSERT(ct);

	switch (attr)
	{
		case DATATYPESINFO_CHANNELS:
			switch (ct->sampletype)
			{
				case SDTST_M8S:
				case SDTST_M16S:
					return ((APTR)1);

				case SDTST_S8S:
				case SDTST_S16S:
					return ((APTR)2);
			}
			break;

		case DATATYPESINFO_FREQUENCY:
			if (GetAttr(SDTA_Frequency, ct->obj, &v))
			{
				return ((APTR)v);
			}
			break;

		case DATATYPESINFO_CODEC:
			#if !USE_LEGACY
			if (GetAttr(SDTA_Codec, ct->obj, &v))
			{
				return ((APTR)v);
			}
			#endif
			break;

		#ifdef DEBUG
		default:
			PDB(("nothing for %ld\n", attr));
			break;
		#endif
	}
	return (NULL);
}


LONG snd_datatypes_read(APTR ctx, UBYTE *buf, ULONG len)
{
	struct datatypes_ctx *ct = ctx;
	struct _sdtFetch msg;

	ASSERT(ct);
	ASSERT(buf);
	ASSERT(len);

	msg.MethodID = SDTM_FETCH;
	msg.sdtf_EndOfStream = FALSE;
	msg.sdtf_Actual = 0;

	switch (ct->sampletype)
	{
		case SDTST_M16S:
		case SDTST_S16S:
			{
				/*
				 * 16-bit, no conversion
				 */
				msg.sdtf_Buffer = buf;
				msg.sdtf_Length = len;

				if (DoMethodA(ct->obj, (Msg)&msg))
				{
					return (msg.sdtf_Actual);
				}
			}
			break;

		case SDTST_M8S:
		case SDTST_S8S:
			{
				/*
				 * 8-bit, conversion to 16-bit
				 */
				LONG rlen = len / 2;

				msg.sdtf_Buffer = buf;
				msg.sdtf_Length = rlen;

				if (DoMethodA(ct->obj, (Msg)&msg))
				{
					rlen = msg.sdtf_Actual;

					while (rlen >= 0)
					{
						buf[rlen * 2] = buf[rlen];
						buf[rlen * 2 + 1] = 0;

						rlen--;
					}
					return (msg.sdtf_Actual * 2);
				}
			}
			break;
	}
	return (0);
}

#endif
