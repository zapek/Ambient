#ifndef AMBIENT_POINTER_H
#define AMBIENT_POINTER_H
/*
 * $Id: pointer.h,v 1.3 2006/02/22 14:48:22 fab Exp $
 */

enum {
	POINTER_MOVE,
	POINTER_CROSSAIR,
	POINTER_TARGET,
};

struct Window;

ULONG pointer_init(void);
void pointer_cleanup(void);
void pointer_set(struct Window *win, ULONG type);
void pointer_clear(struct Window *win);

#endif /* AMBIENT_POINTER_H */
