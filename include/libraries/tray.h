#ifndef LIBRARIES_TRAY_H
#define LIBRARIES_TRAY_H

/*
        tray.library include

        Copyright © 2005 Adam Waldenberg, OnyxSoft, All Rights Reserved.
*/

#ifndef EXEC_LIBRARIES_H
# include <exec/libraries.h>
#endif

#ifndef EXEC_TYPES_H
# include <exec/types.h>
#endif

#ifndef LIBRARIES_MUI_H
# include <libraries/mui.h>
#endif

#ifndef UTILITY_HOOKS_H
# include <utility/hooks.h>
#endif

#if defined(__GNUC__)
# pragma pack(4)
#endif

/*
        Some macros for commonly used operations that clients and servers
        can use when they need to set attributes in each others context.
*/

#define tray_client_setex(z,a,b,c)      tray_client_pushmethod(z, a, 3, MUIM_Set, b, c)

#define tray_server_method(z,o,n,m, ...) \
{ULONG _tags[] = {MUIM_Application_PushMethod, (ULONG) o, (ULONG) n, (ULONG) m, __VA_ARGS__ }; \
DoMethodA(z, (APTR) _tags);}

#define tray_server_setex(z,a,b,c)   tray_server_method(z, a, 3, MUIM_Set, b, c)
#define tray_server_beginupdate(z,a) tray_server_method(z, a, 1, MUIM_Group_InitChange)
#define tray_server_endupdate(z,a)   tray_server_method(z, a, 1, MUIM_Group_ExitChange)

/*
        Tags for tray_server_applet().
*/

#define TRAY_VERTICALAPPLET             0x00000001

/*
        Tags for tray_client_getserverdataA().
*/

#define TRAY_SDATA_VERTICALAPPLET       0x20000001
#define TRAY_SDATA_APPLETSERVERFOUND    0x20000002

/*
        Tags for tray_server_getclientdataA().
*/

#define TRAY_CDATA_NAME                 0x40000001
#define TRAY_CDATA_ICONWIDTH            0x40000010
#define TRAY_CDATA_ICONHEIGHT           0x40000020
#define TRAY_CDATA_ICON                 0x40000040
#define TRAY_CDATA_APPLETONLY           0x40000080
#define TRAY_CDATA_AUTHOR               0x40000100
#define TRAY_CDATA_DESCRIPTION          0x40000200
#define TRAY_CDATA_APPLETLIST		0x40000400

/*
        Tags for tray_cleint_register().
*/

#define TRAY_CREG_APPLETONLY            0x60000001
#define TRAY_CREG_ICON                  0x60000002
#define TRAY_CREG_AUTHOR                0x60000010
#define TRAY_CREG_DESCRIPTION           0x60000020

struct TrayBase {
        struct Library  base; 
};

typedef void tray_client;
typedef void tray_server;

#if defined(__GNUC__)
# pragma pack()
#endif

#endif /* LIBRARIES_TRAY_H */
