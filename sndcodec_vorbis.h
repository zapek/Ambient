#ifndef AMBIENT_SND_VORBIS_H
#define AMBIENT_SND_VORBIS_H
/*
 * $Id: sndcodec_vorbis.h,v 1.4 2006/08/08 13:31:36 fab Exp $
 */

#if USE_VORBIS

/*
 * snd_vorbis_getattr() attributes.
 */
enum {
	VORBISINFO_VERSION,
	VORBISINFO_BITRATE_UPPER,
	VORBISINFO_BITRATE_NOMINAL,
	VORBISINFO_BITRATE_LOWER,
	VORBISINFO_BITRATE_WINDOW,
};

/*
 * snd_vorbis_getattr() shared attributes.
 */
#define VORBISINFO_CHANNELS SOUNDINFO_CHANNELS
#define VORBISINFO_FREQUENCY SOUNDINFO_FREQUENCY


ULONG snd_vorbis_init(void);
void snd_vorbis_cleanup(void);

APTR snd_vorbis_open_file(CONST_STRPTR filename);
void snd_vorbis_close_file(APTR ctx);
APTR snd_vorbis_getattr(APTR ctx, ULONG attr);
LONG snd_vorbis_read(APTR ctx, UBYTE *buf, ULONG len);

#endif /* USE_VORBIS */

#endif /* AMBIENT_SND_VORBIS_H */
