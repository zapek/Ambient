#ifndef AMBIENT_SND_MULTIMEDIA_H
#define AMBIENT_SND_MULTIMEDIA_H
/*
 * $Id: sndcodec_multimedia.h,v 1.4 2006/08/08 13:31:36 fab Exp $
 */

/*
 * snd_multimedia_getattr() shared attributes.
 */
#define MULTIMEDIAINFO_CHANNELS SOUNDINFO_CHANNELS
#define MULTIMEDIAINFO_FREQUENCY SOUNDINFO_FREQUENCY


ULONG snd_multimedia_init(void);
void snd_multimedia_cleanup(void);

APTR snd_multimedia_open_file(CONST_STRPTR filename);
void snd_multimedia_close_file(APTR ctx);
APTR snd_multimedia_getattr(APTR ctx, ULONG attr);
LONG snd_multimedia_read(APTR ctx, UBYTE *buf, ULONG len);

#endif /* AMBIENT_SND_MULTIMEDIA_H */
