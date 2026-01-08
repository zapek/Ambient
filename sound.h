#ifndef AMBIENT_SOUND_H
#define AMBIENT_SOUND_H
/*
 * $Id: sound.h,v 1.5 2007/02/11 22:35:30 fab Exp $
 */

#include <libraries/asound.h>

/*
 * SOUNDINFO_#? codec shared attributes
 */
enum {
	SOUNDINFO_CHANNELS = 0x80000001,
	SOUNDINFO_FREQUENCY,
};


ULONG sound_init(void);
void sound_cleanup(void);

APTR v_sound_create(ULONG mode, const struct TagItem *tags);
APTR sound_create(ULONG mode, ...);
void sound_delete(APTR ctx);
ULONG sound_play_sync(APTR ctx, CONST_APTR buf, ULONG len);
ULONG sound_play_async(APTR ctx, CONST_APTR buf, ULONG len);
ULONG sound_wait(APTR ctx);
void sound_abort(APTR ctx);
APTR sound_getattr(APTR ctx, ULONG attr);

#endif /* AMBIENT_SOUND_H */
