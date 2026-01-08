#ifndef AMBIENT_VIEW_FUNC_H
#define AMBIENT_VIEW_FUNC_H
/*
 * $Id: view_func.h,v 1.4 2013/10/29 23:30:08 geit Exp $
 */

#include "ambient.h"

#define FORWARD_GET(attr) case (attr): \
	{ \
		return (get(_view(obj), (attr), msg->opg_Storage));	\
	}

#define FORWARD_SET(attr) case (attr): \
	{ \
		set(_view(obj), (attr), tag->ti_Data); \
	} \
	break;

#define FORWARD_METHOD(name) \
	static ULONG handleMM_##name(struct IClass *cl UNUSED, Object *obj, Msg msg) \
	{ \
		return (DoMethodA(_view(obj), msg)); \
	}

#define DECFORWARDER(name) case MM_##name: return(handleMM_##name(cl, obj, (APTR)msg));

#endif /* AMBIENT_VIEW_FUNC_H */
