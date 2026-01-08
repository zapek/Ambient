#ifndef AMBIENT_AVCODEC_H
#define AMBIENT_AVCODEC_H
/*
 * $Id: avcodec.h,v 1.1 2006/08/08 13:19:51 fab Exp $
 */


#if USE_AVCODEC

ULONG libavcodec_open(APTR c, STRPTR filename,ULONG * width, ULONG * height, double * framerate);
void  libavcodec_close(APTR c, ULONG free);
APTR  libavcodec_decodeframe(APTR c, LONG percent);

ULONG libavcodec_init( void );
void  libavcodec_cleanup( void );

APTR  libavcodec_openlibraries(void);
void  libavcodec_closelibraries(APTR c);

/* helpers */
APTR  createvideobitmap(APTR c, STRPTR name, LONG percent, ULONG width, ULONG height);
ULONG video_validate(STRPTR path, APTR mimetype, LONG * percent);

#endif

#endif
