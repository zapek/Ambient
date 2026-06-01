#ifndef AMBIENT_SCREEN_H
#define AMBIENT_SCREEN_H
/*
 * $Id: screen.h,v 1.13 2025/09/12 16:06:01 jacadcaps Exp $
 */

struct Screen *get_screen(void);
APTR lock_screen(struct Screen *scr);
BOOL is_screen_visible(void);
void unlock_screen(APTR handle);

struct Screen *get_screen_hold(void);
void get_screen_release(void);

const char *active_screen_name(void);
void flush_screen(void);

struct Screen *screen_lock(void);
struct Screen *screen_lock_by_id(ULONG sid);
void screen_unlock(struct Screen *locked_screen);

ULONG get_screen_id(CONST_STRPTR pubname);
CONST_STRPTR get_screen_pubname(Object *muiArea);

#endif /* AMBIENT_SCREEN_H */
