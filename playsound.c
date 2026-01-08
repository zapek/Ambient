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
 * $Id: playsound.c,v 1.8 2009/09/30 17:38:29 kiero Exp $
 */

#include "ambient.h"

/* public */
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>

/* private */
#include "playsound.h"
#include "sound.h"
#include "soundwin.h"
#include "sndcodec_vorbis.h"
#include "sndcodec_mpega.h"
#include "sndcodec_datatypes.h"
#include "sndcodec_multimedia.h"
#include "threads.h" /* ABORTED */
#include "mimeuri.h"
#include "cpu.h"


#define BUFLEN 32768 /* beware with the stack. must be a multiple of 4 */
#define MIXPRIORITY 10


static struct SignalSemaphore qsem;
static struct MinList qlist;
static struct Task *q_master;
static APTR codec_ctx;

volatile ULONG sound_stop;
volatile ULONG sound_pause;

struct qnode {
	struct MinNode n;
	ULONG mode;
	TEXT path[0];
};


ULONG playsound_init(void)
{
	InitSemaphore(&qsem);
	NEWLIST(&qlist);

	return (TRUE);
}


void playsound_cleanup(void)
{
	/* zZz */
}


ULONG tr_playsound(CONST_STRPTR path, ULONG mode)
{
	APTR vctx;
	APTR (*sound_open_func)(CONST_STRPTR) = NULL;
	void (*sound_close_func)(APTR) = NULL;
	APTR (*sound_getattr_func)(APTR, ULONG) = NULL;
	LONG (*sound_read_func)(APTR, UBYTE *, ULONG) = NULL;
	ULONG rc = FALSE;
	ULONG loop = 1;
	ULONG sigs;
	TEXT pathcopy[PATH_SIZE];

	THREAD;
	ASSERT(path);

	if (mode & (PSF_QUEUED | PSF_QUEUED_IMMEDIATE))
	{
		ObtainSemaphore(&qsem);

		if (q_master)
		{
			struct qnode *qn;
			/*
			 * There's already a master, just queue
			 * the request so he can process it
			 * when he's done.
			 */
			if ((qn = malloc(sizeof(*qn) + strlen(path) + 1)))
			{
				qn->mode = mode;
				strcpy(qn->path, path);
				ADDTAIL(&qlist, qn);
				
				if (mode & PSF_QUEUED_IMMEDIATE)
				{
					Signal(q_master, SIGBREAKF_CTRL_D);
				}
				ReleaseSemaphore(&qsem);
				return (TRUE);
			}
			else
			{
				ReleaseSemaphore(&qsem);
				return (FALSE);
			}
		}
		else
		{
			/*
			 * We become a master.
			 */
			q_master = FindTask(NULL);
			strcpy(pathcopy, path);
			soundwin_setattr(SOUNDWINTAG_TITLE, (ULONG)FilePart(pathcopy));
		}

		ReleaseSemaphore(&qsem);
	}

	while (loop)
	{
		if ((mode & 0xfffff) == PS_UNKNOWN)
		{
			APTR mctx;

			if ((mctx = mimeuri_create()))
			{
				if (mimeuri_gather(mctx, pathcopy, TAG_DONE))
				{
					STRPTR s = mimeuri_getattr(mctx, MIMEURIATTR_MIMETYPE);

					if (!strcmp(s, MIMETYPE_INTERNAL_VORBIS))
					{
						mode = PS_VORBIS | (mode & 0xff000000);
					}
					else if (!strcmp(s, MIMETYPE_INTERNAL_MPEGA))
					{
						mode = PS_MPEGA | (mode & 0xff000000);
					}
					else if (!strcmp(s, MIMETYPE_INTERNAL_MULTIMEDIA))
					{
						mode = PS_MULTIMEDIA | (mode & 0xff000000); /* XXX: that's not really internal.. */
					}
					else if (!strcmp(s, MIMETYPE_INTERNAL_DATATYPES))
					{
						mode = PS_DATATYPES | (mode & 0xff000000); /* XXX: hm.. ditto ? */
					}
					/*  XXX: unknown.. tell about it maybe 
                     */
				}
				mimeuri_delete(mctx);
			}
		}

		switch (mode & 0xfffff)
		{
			#if USE_VORBIS
			case PS_VORBIS:
				sound_open_func    = snd_vorbis_open_file;
				sound_close_func   = snd_vorbis_close_file;
				sound_getattr_func = snd_vorbis_getattr;
				sound_read_func    = snd_vorbis_read;
				break;
			#endif

			#if USE_MPEGA
			case PS_MPEGA:
				sound_open_func    = snd_mpega_open_file;
				sound_close_func   = snd_mpega_close_file;
				sound_getattr_func = snd_mpega_getattr;
				sound_read_func    = snd_mpega_read;
				break;
			#endif
		
			#if USE_MULTIMEDIA
			case PS_MULTIMEDIA:
				sound_open_func    = snd_multimedia_open_file;
				sound_close_func   = snd_multimedia_close_file;
				sound_getattr_func = snd_multimedia_getattr;
				sound_read_func    = snd_multimedia_read;
				break;
			#endif

			#if USE_DATATYPES_SOUND
			case PS_DATATYPES:
				sound_open_func    = snd_datatypes_open_file;
				sound_close_func   = snd_datatypes_close_file;
				sound_getattr_func = snd_datatypes_getattr;
				sound_read_func    = snd_datatypes_read;
				break;
			#endif

			#ifdef DEBUG
			default:
				PDB(("mode %ld not supported\n", mode));
				break;
			#endif
		}

		if (sound_open_func &&  /* make sure we don't jump into non existent code, when init above failed */ 
           (vctx = sound_open_func(pathcopy)))
		{
			APTR sctx;
			ULONG channels;

			D(SOUND,bug("stream file <%s> opened\n", pathcopy));

			channels = (ULONG)sound_getattr_func(vctx, SOUNDINFO_CHANNELS);

			if (channels > 0 && channels <= 2)
			{
				if ((sctx = sound_create(SOUNDMODE_NORMAL,
										SOUNDTAG_Channels, channels,
										SOUNDTAG_Resolution, SOUNDVAL_Resolution_16,
										SOUNDTAG_Frequency, sound_getattr_func(vctx, SOUNDINFO_FREQUENCY),
										SOUNDTAG_Priority, SOUNDVAL_Priority_Music,
										TAG_DONE))
				)
				{
					LONG len = -1;
					BYTE *buf[2];
					ULONG i = 0;
					struct Task *me = FindTask(NULL);
					BYTE oldpri;

					if ((buf[0] = cpu_cache_malloc(BUFLEN)))
					{
						if ((buf[1] = cpu_cache_malloc(BUFLEN)))
						{
							rc = TRUE;

							D(SOUND,bug("sound context created\n"));

							oldpri = SetTaskPri(me, MIXPRIORITY);
							sound_stop = FALSE;
							sound_pause = FALSE;

							if (mode & PSF_QUEUED)
							{
								codec_ctx = sctx;
							}

							#if USE_CPU_CACHEHINTS
							/* NOTE: memory is always cacheable from cpu_cache_malloc */
							cpu_cache_zero(buf[i], BUFLEN);
							#endif
							
							while ((len = sound_read_func(vctx, buf[i], BUFLEN)) > 0)
							{
								D(SOUND,bug("read %lu bytes of sound data\n", (ULONG)BUFLEN));
								if (!sound_play_async(sctx, buf[i], len))
								{
									/* XXX */
									rc = FALSE;
									break;
								}

								i ^= 1;

								if (i)
								{
									waitagain:

									sigs = SetSignal(0, SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);

									if (sigs & SIGBREAKF_CTRL_C || sound_stop)
									{
										rc = ABORTED;
										loop = FALSE;
										break;
									}
									else if (sigs & SIGBREAKF_CTRL_D)
									{
										if (mode & PSF_QUEUED_IMMEDIATE)
										{
											break; /* abort and try the next sound */
										}
									}

									if (sound_pause)
									{
										Delay(12);
										goto waitagain;
									}
								}
								#if USE_CPU_CACHEHINTS
								/* NOTE: memory is always cacheable from cpu_cache_malloc */
								cpu_cache_zero(buf[i], BUFLEN);
								#endif
							}

							if (len == -1)
							{
								rc = FALSE;
							}

							if (rc == TRUE)
							{
								sound_wait(sctx);
							}

							D(SOUND,bug("end of sound stream: %ld\n", rc));

							SetTaskPri(me, oldpri);
							
							cpu_cache_free(buf[1]);
						}
						else
						{
							D(SOUND,bug("not enough memory for sound buffer\n"));
						}
						cpu_cache_free(buf[0]);
					}
					else
					{
						D(SOUND,bug("not enough memory for sound buffer\n"));
					}
					sound_delete(sctx);
				}
			}
			/* XXX */
			sound_close_func(vctx);
		}
	
		ObtainSemaphore(&qsem);
		if (q_master)
		{
			codec_ctx = NULL;

			if (rc == ABORTED)
			{
				loop = FALSE;
			}
			else
			{
				struct qnode *qn;

				if (mode & PSF_QUEUED_IMMEDIATE)
				{
					qn = REMTAIL(&qlist); /* LIFO */
				}
				else
				{
					qn = REMHEAD(&qlist); /* FIFO */
				}

				if (qn)
				{
					mode = qn->mode;

					strcpy(pathcopy, qn->path);
					soundwin_setattr(SOUNDWINTAG_TITLE, (ULONG)FilePart(pathcopy));

					free(qn);
				}
				else
				{
					loop = FALSE;
				}
			}

			if (!loop)
			{
				q_master = NULL;
			}
		}
		ReleaseSemaphore(&qsem);
	}

	soundwin_stop();

	return (rc);
}


#if 0
static APTR notifyobj;


void playsound_register(APTR obj)
{
	ObtainSemaphore(&qsem);

	notifyobj = obj;

	if (q_master)
	{
		/*
		 * Give infos to the freshly opened
		 * window.
		 */
		/* XXX: ok that sucks.. how do I get the info eh? */
		methodstack_push_sync(notifyobj, 2, MM_Soundwin_Info_Channels,       /* XXX: maybe not synced later.. */

	}

	ReleaseSemaphore(&qsem);
}


void playsound_unregister(APTR obj)
{
	ObtainSemaphore(&qsem);

	killpushedmethod(obj);
	notifyobj = NULL;

	ReleaseSemaphore(&qsem);
}
#endif
