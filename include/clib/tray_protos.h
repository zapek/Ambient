#ifndef CLIB_TRAY_PROTOS_H
#define CLIB_TRAY_PROTOS_H

/*
        tray.library prototypes

        Copyright © 2005 Adam Waldenberg, OnyxSoft, All Rights Reserved.
*/

#ifndef INTUITION_CLASSUSR_H
# include <intuition/classusr.h>
#endif

#ifndef EXEC_TYPES_H
# include <exec/types.h>
#endif

#ifndef GRAPHICS_GFX_H
# include <graphics/gfx.h>
#endif

#ifndef UTILITY_HOOKS_H
# include <utility/hooks.h>
#endif

#ifndef UTILITY_TAGITEM_H
# include <utility/tagitem.h>
#endif

#ifndef LIBRARIES_TRAY_H
# include <libraries/tray.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void tray_client_applet(tray_client *, Object **);
BOOL tray_client_getserverdataA(tray_client *, struct TagItem *);
#if !defined(USE_INLINE_STDARG)
void tray_client_getserverdata(tray_client *, Tag, ... );
#endif
void tray_client_icon(tray_client *, STRPTR);
void tray_client_info(tray_client *, STRPTR);
void tray_client_menu(tray_client *, Object *);
void tray_client_notify(tray_client *, STRPTR);
void tray_client_popevent(tray_client *, struct Hook *, APTR userdata);
ULONG tray_client_pushmethodA(tray_client *, Object *, Msg);
#if !defined(USE_INLINE_STDARG)
ULONG tray_client_pushmethod(tray_client *, Object *, ULONG, ... );
#endif
tray_client *tray_client_registerA(STRPTR, struct TagItem *);
#if !defined(USE_INLINE_STDARG)
tray_client *tray_client_register(STRPTR, Tag, ... );
#endif
void tray_client_unregister(tray_client *);

BOOL tray_server_applet(tray_client *, struct Hook *, ULONG);
void tray_server_getclientdataA(tray_client *, struct TagItem *);
#if !defined(USE_INLINE_STDARG)
void tray_server_getclientdata(tray_client *, Tag, ... );
#endif
void tray_server_icon(tray_server *, struct Hook *);
void tray_server_info(tray_server *, struct Hook *);
void tray_server_menu(tray_server *, struct Hook *);
void tray_server_notify(tray_server *, struct Hook *);
void tray_server_popevent(tray_server *, struct Hook *);
void tray_server_pushpopevent(tray_client *c);
tray_server *tray_server_register(STRPTR, struct Hook *, struct Hook *);
void tray_server_unregister(tray_server *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CLIB_TRAY_PROTOS_H */
