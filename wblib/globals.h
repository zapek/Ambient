#ifndef AMBIENT_WB_GLOBALS_H
#define AMBIENT_WB_GLOBALS_H
/*
 * $Id: globals.h,v 1.3 2006/08/08 13:31:43 fab Exp $
 */

#include "config.h"

#include "debug.h"

#include <macros/compilers.h>
#include <exec/types.h>

#define FORTAG(_tagp) \
	{ \
		struct TagItem *tag, *_tags = (struct TagItem *)(_tagp); \
		while ((tag = NextTagItem(&_tags))) switch ((int)tag->ti_Tag)
#define NEXTTAG }

#endif /* AMBIENT_WB_GLOBALS_H */
