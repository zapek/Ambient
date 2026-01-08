#ifndef AMBIENT_DEVICEIO_H
#define AMBIENT_DEVICEIO_H
/*
 * $Id: devices.h,v 1.7 2006/08/08 13:31:33 fab Exp $
 */

#include <exec/types.h>

struct device_information
{
	QUAD  used;
	QUAD  total;
	ULONG state;
};

ULONG tr_devices_show(APTR obj, BOOL assigns, BOOL only_auxillary, BOOL fade);
ULONG tr_devices_add(APTR obj, ULONG fade);
ULONG tr_devices_remove(APTR obj);
ULONG tr_devices_removeall(APTR obj);
ULONG tr_devices_updateinfo(APTR obj);

ULONG devices_init(void);
void devices_cleanup(void);

ULONG device_get_information(CONST_STRPTR path, struct device_information * inf);

#endif /* AMBIENT_DEVICEIO_H */
