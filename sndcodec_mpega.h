#ifndef AMBIENT_SND_MPEGA_H
#define AMBIENT_SND_MPEGA_H
/*
 * $Id: sndcodec_mpega.h,v 1.4 2006/08/08 13:31:36 fab Exp $
 */

#if USE_MPEGA

/*
 * mpega_getattr() attributes.
 */
enum {
	MPEGAINFO_VERSION,
	MPEGAINFO_LAYER,
	MPEGAINFO_MODE,
	MPEGAINFO_BITRATE,
	MPEGAINFO_DURATION,
	MPEGAINFO_PRIVATE,
	MPEGAINFO_COPYRIGHT,
	MPEGAINFO_ORIGINAL,
};

/*
 * mpega_getattr() shared attributes.
 */
#define MPEGAINFO_CHANNELS SOUNDINFO_CHANNELS
#define MPEGAINFO_FREQUENCY SOUNDINFO_FREQUENCY


ULONG snd_mpega_init(void);
void snd_mpega_cleanup(void);

APTR snd_mpega_open_file(CONST_STRPTR filename);
void snd_mpega_close_file(APTR ctx);
APTR snd_mpega_getattr(APTR ctx, ULONG attr);
LONG snd_mpega_read(APTR ctx, UBYTE *buf, ULONG len);

#endif /* USE_MPEGA */

#endif /* AMBIENT_MPEGA_H */
