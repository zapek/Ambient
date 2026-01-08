#ifndef AMBIENT_ICONCHUNK_H
#define AMBIENT_ICONCHUNK_H
/*
 * $Id: iconchunk.h,v 1.3 2006/02/22 14:48:20 fab Exp $
 */

struct iconchunk {
	struct MinNode n;
	ULONG size;
	UBYTE data[0];
};

#endif /* AMBIENT_ICONCHUNK_H */
