#ifndef AMBIENT_VARS_H
#define AMBIENT_VARS_H
/*
 * $Id: vars.h,v 1.5 2006/08/08 13:31:37 fab Exp $
 */

/*
 * Global vars
 *
 * How to add a var:
 * -----------------
 * - add a var in the structure (order doesn't matter)
 * - add it in vars.c/refresh_vars() as well
 */
#define _var(X) gvars->X

struct global_vars {
	ULONG prefsio_partial;    /* tries to keep half broken prefs files */
	ULONG prefsio_ignore_crc; /* ignores the prefs integrity */
	ULONG nowildstar;         /* doesn't automatically set dos.library's '*' as wildcard */
	ULONG audiodev;           /* use audio.device as default sound driver */
	ULONG nocachepretouch;    /* no cache hintings */
	ULONG noaltivec;          /* disables altivec */
};

extern struct global_vars *gvars;

ULONG vars_init(void);
void vars_cleanup(void);
void refresh_vars(void);

#endif /* AMBIENT_VARS_H */
