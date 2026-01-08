#ifndef AMBIENT_DRAGDROP_H
#define AMBIENT_DRAGDROP_H

#include <libraries/mui.h>

struct dragdropnode {
	struct MinNode n;
	LONG type;
	LONG x;
	LONG y;
	TEXT path[0];
};

/* Extended DragImage structure. */

struct MUI_AmbientDragImage{
	struct MUI_DragImage di;
	APTR bitmap;                  /* ambient bitmap struct (wrapping bitmap used for d&d image */
};

/* WB/DOpus seem to add some strange values */
#define POSX_CORRECTION 4L
#define POSY_CORRECTION 3L

/* Mouse movement triggering in pixels */
#define DRAGDROP_START_X 1
#define DRAGDROP_START_Y 1

/* XXX: Maybe configurable one day.. */
#define TRANSPARENT_VAL 0x80

#endif /* AMBIENT_DRAGDROP_H */
