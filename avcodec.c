/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2006 Ambient Open Source Team
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
 * $Id: avcodec.c,v 1.5 2025/07/23 23:54:26 geit Exp $
 */

#include "ambient.h"

#if USE_AVCODEC

/* public */
#include <proto/exec.h>
#define __MORPHOS_SHAREDLIBS
#include <stdint.h>
#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>

/* private */
#include "avcodec.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "gfx_scale.h"
#include "mimetype.h"
#include "str.h"

static struct SignalSemaphore avcodecsem;

struct avcodec_ctx
{
	struct Library  * AVCodecBase;
	struct Library  * AVFormatBase;
	AVFormatContext * pFormatCtx;
	AVCodecContext  * pCodecCtx;
	AVFrame         * pFrame;
	AVFrame         * pFrameRGB;
	uint8_t         * buffer;
	int             videoStream;
	double          framerate;
	APTR            bm;
	APTR            bms;
};

#if DEBUG
static void libavcodec_debug_callback(void * dummy UNUSED, int dummy2 UNUSED, const char * fmt, va_list vl)
{
	kprintf(fmt, vl);
}
#endif

ULONG libavcodec_init( void )
{
	InitSemaphore(&avcodecsem);
	return (TRUE);
}

void libavcodec_cleanup( void )
{
}

APTR libavcodec_openlibraries(void)
{
	struct avcodec_ctx * ctx;

	ctx = (struct avcodec_ctx *) malloc(sizeof(*ctx));

	SDB((">> libavcodec_openlibraries\n"));
	ObtainSemaphore(&avcodecsem);

	if(ctx)
	{
		struct Library * AVFormatBase;
		struct Library * AVCodecBase UNUSED;

		memset(ctx, 0, sizeof(*ctx));

		AVFormatBase = ctx->AVFormatBase = OpenLibrary("avformat.library", 50);
		AVCodecBase  = ctx->AVCodecBase  = OpenLibrary("avcodec.library", 50);

		if(ctx->AVCodecBase && ctx->AVFormatBase)
		{
			struct Task * t UNUSED = FindTask(NULL);

			SDB(("Task : 0x%p (%s) opened AVCodecBase  = 0x%p\n", t, t->tc_Node.ln_Name, AVCodecBase));
			SDB(("Task : 0x%p (%s) opened AVFormatBase = 0x%p\n", t, t->tc_Node.ln_Name, AVFormatBase));


#if DEBUG
			av_log_set_callback(libavcodec_debug_callback);
			av_log_set_level(AV_LOG_DEBUG);
#endif
			av_register_all();
		}
		else
		{
			libavcodec_closelibraries((APTR) ctx);
			ctx = NULL;
		}

	}

	SDB(("<< libavcodec_openlibraries\n"));
	ReleaseSemaphore(&avcodecsem);

	return ctx;
}

void libavcodec_closelibraries(APTR c)
{
	struct avcodec_ctx * ctx = (struct avcodec_ctx *) c;

	SDB((">> libavcodec_closelibraries\n"));
	ObtainSemaphore(&avcodecsem);

	if(ctx)
	{
		if(ctx->AVFormatBase)
			CloseLibrary(ctx->AVFormatBase);

		if(ctx->AVCodecBase)
			CloseLibrary(ctx->AVCodecBase);

		free(ctx);
	}

	SDB(("<< libavcodec_closelibraries\n"));
	ReleaseSemaphore(&avcodecsem);
}

ULONG libavcodec_open(APTR c, STRPTR filename, ULONG * dstwidth, ULONG * dstheight, double * framerate)
{
	struct Library * AVFormatBase;
	struct Library * AVCodecBase;

	struct avcodec_ctx * ctx = NULL;
	char uri[PATH_SIZE];
	int  i;
	AVCodec  * pCodec;
	AVStream * pStream = NULL;
	int numBytes;
	ULONG rc = FALSE;

	ctx = (struct avcodec_ctx *) c;

	SDB((">> libavcodec_open\n"));
	ObtainSemaphore(&avcodecsem);

	if(ctx)
	{
		AVCodecBase  = ctx->AVCodecBase;
		AVFormatBase = ctx->AVFormatBase;

		/* reset context */
		memset(ctx, 0, sizeof(*ctx));
		ctx->AVCodecBase  = AVCodecBase;
		ctx->AVFormatBase = AVFormatBase;

		snprintf(uri, sizeof(uri), "file:%s", filename);

		if(av_open_input_file(&ctx->pFormatCtx, uri, NULL, 0, NULL) == 0)
		{
			if(av_find_stream_info(ctx->pFormatCtx) >= 0)
			{
				ctx->videoStream = -1;
				for(i=0; i<ctx->pFormatCtx->nb_streams; i++)
				{
					if(ctx->pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
					{
						ctx->videoStream = i;
						pStream = ctx->pFormatCtx->streams[i];
						break;
					}
				}

				if(ctx->videoStream != -1)
				{
					ctx->pCodecCtx = ctx->pFormatCtx->streams[ctx->videoStream]->codec;

					if(ctx->pCodecCtx)
					{
						pCodec = avcodec_find_decoder(ctx->pCodecCtx->codec_id);

						if(pCodec)
						{
							if(avcodec_open(ctx->pCodecCtx, pCodec) >= 0)
							{
								ctx->pFrame = avcodec_alloc_frame();
								ctx->pFrameRGB = avcodec_alloc_frame();

								ctx->framerate = * framerate = 1.0 / av_q2d(pStream->codec->time_base);

								if(ctx->pFrame && ctx->pFrameRGB)
								{
									/* XXX: allocating more because img_convert sometimes overflows.  */
									numBytes = 2 * avpicture_get_size(PIX_FMT_RGBA32, ctx->pCodecCtx->width, ctx->pCodecCtx->height);

									ctx->buffer = (uint8_t *) malloc(numBytes * sizeof(uint8_t));

									if(ctx->buffer)
									{
										avpicture_fill((AVPicture *)ctx->pFrameRGB, ctx->buffer, PIX_FMT_RGBA32, ctx->pCodecCtx->width, ctx->pCodecCtx->height);

										ctx->bm = gfx_bitmap_create( ctx->pCodecCtx->width, ctx->pCodecCtx->height, 32,
										                             BITMAPTAG_Format, BITMAPVAL_Format_ARGB32,
										                             TAG_DONE );

										if(ctx->bm)
										{
											gfx_scale_calc_aspect_constraints(gfx_bitmap_width(ctx->bm), gfx_bitmap_height(ctx->bm), dstwidth, dstheight);

											ctx->bms = gfx_bitmap_create( *dstwidth, *dstheight, 32,
											                              BITMAPTAG_Format, BITMAPVAL_Format_ARGB32,
											                              TAG_DONE );

											if(ctx->bms)
											{
												rc = TRUE;
											}
										}
									}
								}
							}
						}
						else
						{
							ctx->pCodecCtx = NULL;
						}
					}
				}
			}
		}
	}

	ReleaseSemaphore(&avcodecsem); /* hm */

	if(rc == FALSE)
	{
		libavcodec_close((APTR) ctx, TRUE);
	}

	SDB(("<< libavcodec_open\n"));

	return (rc);
}

void libavcodec_close(APTR c, ULONG free)
{
	struct Library * AVFormatBase;
	struct Library * AVCodecBase;
	struct avcodec_ctx * ctx = (struct avcodec_ctx *) c;

	SDB((">> libavcodec_close\n"));
	ObtainSemaphore(&avcodecsem);

	if(ctx)
	{
		AVCodecBase  = ctx->AVCodecBase;
		AVFormatBase = ctx->AVFormatBase;

		if(ctx->bms && free)
		{
			gfx_bitmap_delete(ctx->bms);
			ctx->bms = NULL;
		}

		if(ctx->bm)
		{
			gfx_bitmap_delete(ctx->bm);
			ctx->bm = NULL;
		}

		if(ctx->buffer)
		{
			free(ctx->buffer);
			ctx->buffer = NULL;
		}

		if(ctx->pFrameRGB)
		{
			av_free(ctx->pFrameRGB);
			ctx->pFrameRGB = NULL;
		}

		if(ctx->pFrame)
		{
			av_free(ctx->pFrame);
			ctx->pFrame = NULL;
		}

		if(ctx->pCodecCtx)
		{
			avcodec_close(ctx->pCodecCtx);
			ctx->pCodecCtx = NULL;
		}

		if(ctx->pFormatCtx)
		{
			av_close_input_file(ctx->pFormatCtx);
			ctx->pFormatCtx = NULL;
		}
	}

	SDB(("<< libavcodec_close\n"));
	ReleaseSemaphore(&avcodecsem);
}

APTR libavcodec_decodeframe(APTR c, LONG percent)
{
	struct Library * AVFormatBase;
	struct Library * AVCodecBase;
	APTR     bm = NULL;
	struct   avcodec_ctx * ctx = (struct avcodec_ctx *) c;
	int      frameFinished = 0;
	AVPacket packet;
	int readframe = 0;

	SDB((">> libavcodec_decodeframe\n"));
	ObtainSemaphore(&avcodecsem);

	if(ctx)
	{
		AVCodecBase  = ctx->AVCodecBase;
		AVFormatBase = ctx->AVFormatBase;

		if(percent >= 0)
		{
			float frac = (float) percent/100.f;
			int64_t seekpoint = (int64_t) (frac * ctx->pFormatCtx->duration);

			if (ctx->pFormatCtx->start_time != AV_NOPTS_VALUE)
				seekpoint += ctx->pFormatCtx->start_time;

			SDB(("Seeking to %lld s\n", seekpoint/AV_TIME_BASE));

			if(av_seek_frame(ctx->pFormatCtx, -1, seekpoint, AVSEEK_FLAG_BACKWARD) < 0)
			{
				SDB(("Seek failed\n"));
			}
		}

		while(!frameFinished && (readframe = av_read_frame(ctx->pFormatCtx, &packet)) >= 0)
		{
			if(packet.stream_index == ctx->videoStream)
			{
				avcodec_decode_video(ctx->pCodecCtx, ctx->pFrame, &frameFinished, packet.data, packet.size);

				if(frameFinished)
				{
					/* XXX : img_convert is sloooow, and buggy. write own converter someday */
					if(img_convert( (AVPicture *)ctx->pFrameRGB, PIX_FMT_RGBA32, (AVPicture*)ctx->pFrame,
								 ctx->pCodecCtx->pix_fmt, ctx->pCodecCtx->width, ctx->pCodecCtx->height) == 0)
					{
						gfx_blit( (UBYTE*) ctx->pFrameRGB->data[0], ctx->bm,
						          BLITTAG_SrcType, BLITVAL_SrcType_Array,
						          BLITTAG_SrcFormat, BLITVAL_SrcFormat_ARGB,
						          BLITTAG_DstWidth, ctx->pCodecCtx->width,
						          BLITTAG_DstHeight, ctx->pCodecCtx->height,
						          BLITTAG_Modulo, ctx->pFrameRGB->linesize[0], /* let's hope linesize[0] is always valid */
						          TAG_DONE );

						if (gfx_scale( ctx->bm, ctx->bms, gfx_bitmap_width(ctx->bms), gfx_bitmap_height(ctx->bms),
						               SCALETAG_Bilinear, TRUE,
						               SCALETAG_Average, TRUE,
						               TAG_DONE))
						{
							bm = ctx->bms;
						}
					}
				}
			}

			av_free_packet(&packet);
		}

		if(!frameFinished && readframe < 0)
		{
			av_seek_frame(ctx->pFormatCtx, -1, 0, AVSEEK_FLAG_BACKWARD);
		}
	}

	SDB(("<< libavcodec_decodeframe\n"));
	ReleaseSemaphore(&avcodecsem);

	return bm;
}

APTR createvideobitmap(APTR c, STRPTR name, LONG percent, ULONG width, ULONG height)
{
	APTR  bm  = NULL;
	ULONG dstwidth = width, dstheight = height;
	double framerate;
	struct avcodec_ctx * ctx = (struct avcodec_ctx *) c;

	if(ctx)
	{
		if(libavcodec_open(ctx, name, &dstwidth, &dstheight, &framerate))
		{
			bm = libavcodec_decodeframe(ctx, percent);

			if(!bm)
			{
				SDB(("avcodec_decodeframe failed\n"));
			}

			libavcodec_close(ctx, FALSE);
		}
	}

	return bm;
}

ULONG video_validate(STRPTR path, APTR mimetype, LONG * percent)
{
	if(mimetype && stristr( ((struct internal_mimetype_node *) mimetype)->mimetype, "video/") )
	{
		/* ok, this one is really lame, but seeking in mpeg/wmv doesn't work for now */
		if(strstr(path, ".avi"))
		{
			*percent = 50;
		}
		else
		{
			*percent = -1;
		}

		return (TRUE);
	}
	else
	{
		return (FALSE);
	}
}

#endif
