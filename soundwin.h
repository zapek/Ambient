#ifndef AMBIENT_SOUNDWIN_H
#define AMBIENT_SOUNDWIN_H
/*
 * $Id: soundwin.h,v 1.2 2007/09/29 13:35:31 fab Exp $
 */


/*
 * Attributes for soundwin.
 */

enum {
	SOUNDWINTAG_TITLE = TAG_USER + 1, /* [.S.] title of tune played */
};

ULONG soundwin_init( void );
void soundwin_cleanup( void );

void soundwin_pause(void);
void soundwin_play(void);
void soundwin_stop(void);

void soundwin_setattr(ULONG attr, ULONG value);

#endif

