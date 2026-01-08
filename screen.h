#ifndef AMBIENT_SCREEN_H
#define AMBIENT_SCREEN_H
/*
 * $Id: screen.h,v 1.9 2018/07/26 15:19:46 itix Exp $
 */

struct Screen *get_screen(void);
APTR lock_screen(struct Screen *scr);
BOOL is_screen_visible(void);
void unlock_screen(APTR handle);

struct Screen *get_screen_hold(void);
void get_screen_release(void);

char *active_screen_name(void);
#if USE_MULTIPLE_DESKTOP
void create_screen(char *name);
#endif
void flush_screen(void);

struct Screen *screen_lock(void);
void screen_unlock(struct Screen *locked_screen);

#endif /* AMBIENT_SCREEN_H */
