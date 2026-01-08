#ifndef AMBIENT_TRASHCAN_H
#define AMBIENT_TRASHCAN_H

#include <exec/types.h>

#define TRASHCAN_INFO "Trashcan:disk.info"

ULONG trashcan_init(void);
void trashcan_cleanup(void);
BOOL trashcan_is_running(void);
void trashcan_empty(void);
BOOL is_trashcan(CONST_STRPTR path);
void trashcan_updatediskinfo(void);

ULONG tr_trashall(APTR obj, APTR refwin, STRPTR *path, BOOL noicon);
ULONG tr_restoreall(APTR obj, APTR refwin, STRPTR *path, BOOL noicon);

#endif /* AMBIENT_TRASHCAN_H */
