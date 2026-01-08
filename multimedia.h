#ifndef AMBIENT_MULTIMEDIA_H
#define AMBIENT_MULTIMEDIA_H
/*
 * $Id: multimedia.h,v 1.5 2014/01/13 18:10:55 rzookol Exp $
 */

#if USE_MULTIMEDIA

ULONG multimedia_init(void);
void multimedia_cleanup(void);

enum {
	MULTIMEDIATYPE_NONE,
	MULTIMEDIATYPE_SOUND,
	MULTIMEDIATYPE_VIDEO,
};

ULONG multimedia_open(void);
void multimedia_close(void);
ULONG multimedia_findtype(STRPTR path);
STRPTR multimedia_nametype(ULONG type);

#endif

#endif /* AMBIENT_MULTIMEDIA_H */
