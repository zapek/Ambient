#ifndef CLIB_TRAYPLUGIN_PROTOS_H
#define CLIB_TRAYPLUGIN_PROTOS_H

/*
        trayplugin prototypes

        Copyright © 2005 Adam Waldenberg, OnyxSoft, All Rights Reserved.
*/

#ifndef EXEC_TYPES_H
# include <exec/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

ULONG trayplugin_version(void);
STRPTR trayplugin_description(void);
STRPTR trayplugin_author(void);
STRPTR trayplugin_name(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CLIB_TRAYPLUGIN_PROTOS_H */
