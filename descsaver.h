#ifndef AMBIENT_DESCSAVER_H

#include <exec/types.h>

#include "mimetype.h"

STRPTR get_commandtypestr(ULONG);
BOOL descriptor_save(struct internal_mimetype_node *);
BOOL descriptor_change_defaultaction(struct internal_mimetype_node *, int);

#endif /* AMBIENT_DESCSAVER_H */
