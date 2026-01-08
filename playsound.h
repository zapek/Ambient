#ifndef AMBIENT_PLAYSOUND_H
#define AMBIENT_PLAYSOUND_H
/*
 * $Id: playsound.h,v 1.4 2006/08/08 13:31:35 fab Exp $
 */

enum {
	PS_UNKNOWN, /* tries to find out */
	PS_VORBIS,
	PS_MPEGA,
	PS_MULTIMEDIA,
	PS_DATATYPES,
};

#define PSF_QUEUED           (1UL << 31)
#define PSF_QUEUED_IMMEDIATE (1UL << 30)

ULONG playsound_init(void);
void playsound_cleanup(void);

ULONG tr_playsound(CONST_STRPTR path, ULONG mode);

extern volatile ULONG sound_stop;
extern volatile ULONG sound_pause;


/*  XXX: this is a bit experimental atm., dunno if do_Action() with NULL 
 *       object is legal, but it works at least for me (tm) (tokai)
 */
#include "config.h"

#if USE_DATATYPES_SOUND 
#include "prefs_advanced.h"
#define PLAYSFX(n) { STRPTR _mysfx = (STRPTR)prefs_advanced_getvalue( "sfx_" #n ); if (_mysfx) do_action(NULL, TA_Sound_Play, TT_Sound_Play_Path, _mysfx, TT_Sound_Play_Mode, (PS_DATATYPES | PSF_QUEUED_IMMEDIATE), TAG_DONE); }
#else
#define PLAYSFX(n) { PDB(("This Ambient doesn't support datatype sounds, please enable it for the build to use sfx effects.\n")); }
#endif

#endif /* AMBIENT_PLAYSOUND_H */
