#ifndef AMBIENT_SND_DATATYPES_H
#define AMBIENT_SND_DATATYPES_H
/*
 * $Id: sndcodec_datatypes.h,v 1.4 2006/08/08 13:31:36 fab Exp $
 */

/*
 * snd_datatypes_getattr() attributes.
 */
enum {
	DATATYPESINFO_CODEC
};

/*
 * snd_datatypes_getattr() shared attributes.
 */
#define DATATYPESINFO_CHANNELS SOUNDINFO_CHANNELS
#define DATATYPESINFO_FREQUENCY SOUNDINFO_FREQUENCY


ULONG snd_datatypes_init(void);
void snd_datatypes_cleanup(void);

APTR snd_datatypes_open_file(CONST_STRPTR filename);
void snd_datatypes_close_file(APTR ctx);
APTR snd_datatypes_getattr(APTR ctx, ULONG attr);
LONG snd_datatypes_read(APTR ctx, UBYTE *buf, ULONG len);

#endif /* AMBIENT_SND_DATATYPES_H */
