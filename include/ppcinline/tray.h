/* Automatically generated header! Do not edit! */

#ifndef _PPCINLINE_TRAY_H
#define _PPCINLINE_TRAY_H

#ifndef __PPCINLINE_MACROS_H
#include <ppcinline/macros.h>
#endif /* !__PPCINLINE_MACROS_H */

#ifndef TRAY_BASE_NAME
#define TRAY_BASE_NAME TrayBase
#endif /* !TRAY_BASE_NAME */

#define tray_server_info(__p0, __p1) \
	LP2NR(72, tray_server_info, \
		tray_server *, __p0, a0, \
		struct Hook *, __p1, a1, \
		, TRAY_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define tray_client_applet(__p0, __p1) \
	LP2NR(102, tray_client_applet, \
		tray_client *, __p0, a0, \
		Object **, __p1, a1, \
		, TRAY_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define tray_client_registerA(__p0, __p1) \
	LP2(150, tray_client *, tray_client_registerA, \
		STRPTR , __p0, a0, \
		struct TagItem *, __p1, a1, \
		, TRAY_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define tray_client_unregister(__p0) \
	LP1NR(60, tray_client_unregister, \
		tray_client *, __p0, a0, \
		, TRAY_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define tray_client_icon(__p0, __p1) \
	LP2NR(30, tray_client_icon, \
		tray_client *, __p0, a0, \
		STRPTR , __p1, a1, \
		, TRAY_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define tray_server_popevent(__p0, __p1) \
	LP2NR(120, tray_server_popevent, \
		tray_server *, __p0, a0, \
		struct Hook *, __p1, a1, \
		, TRAY_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define tray_server_pushpopevent(__p0) \
	LP1NR(132, tray_server_pushpopevent, \
		tray_client *, __p0, a0, \
		, TRAY_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define tray_client_info(__p0, __p1) \
	LP2NR(36, tray_client_info, \
		tray_client *, __p0, a0, \
		STRPTR , __p1, a1, \
		, TRAY_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define tray_server_menu(__p0, __p1) \
	LP2NR(78, tray_server_menu, \
		tray_server *, __p0, a0, \
		struct Hook *, __p1, a1, \
		, TRAY_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define tray_server_notify(__p0, __p1) \
	LP2NR(84, tray_server_notify, \
		tray_server *, __p0, a0, \
		struct Hook *, __p1, a1, \
		, TRAY_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define tray_client_popevent(__p0, __p1, __p2) \
	LP3NR(108, tray_client_popevent, \
		tray_client *, __p0, a0, \
		struct Hook *, __p1, a1, \
		APTR , __p2, d0, \
		, TRAY_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define tray_client_getserverdataA(__p0, __p1) \
	LP2(126, BOOL , tray_client_getserverdataA, \
		tray_client *, __p0, a0, \
		struct TagItem *, __p1, a1, \
		, TRAY_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define tray_server_getclientdataA(__p0, __p1) \
	LP2NR(138, tray_server_getclientdataA, \
		tray_client *, __p0, a0, \
		struct TagItem *, __p1, a1, \
		, TRAY_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define tray_client_menu(__p0, __p1) \
	LP2NR(42, tray_client_menu, \
		tray_client *, __p0, a0, \
		Object *, __p1, a1, \
		, TRAY_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define tray_client_notify(__p0, __p1) \
	LP2NR(48, tray_client_notify, \
		tray_client *, __p0, a0, \
		STRPTR , __p1, a1, \
		, TRAY_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define tray_server_register(__p0, __p1, __p2) \
	LP3(90, tray_server *, tray_server_register, \
		STRPTR , __p0, a0, \
		struct Hook *, __p1, a1, \
		struct Hook *, __p2, a2, \
		, TRAY_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define tray_server_applet(__p0, __p1, __p2) \
	LP3(114, BOOL , tray_server_applet, \
		tray_client *, __p0, a0, \
		struct Hook *, __p1, a1, \
		ULONG , __p2, d0, \
		, TRAY_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define tray_server_unregister(__p0) \
	LP1NR(96, tray_server_unregister, \
		tray_server *, __p0, a0, \
		, TRAY_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define tray_server_icon(__p0, __p1) \
	LP2NR(66, tray_server_icon, \
		tray_server *, __p0, a0, \
		struct Hook *, __p1, a1, \
		, TRAY_BASE_NAME, 0, 0, 0, 0, 0, 0)

#define tray_client_pushmethodA(__p0, __p1, __p2) \
	LP3(144, ULONG , tray_client_pushmethodA, \
		tray_client *, __p0, a0, \
		Object *, __p1, a1, \
		Msg , __p2, a2, \
		, TRAY_BASE_NAME, 0, 0, 0, 0, 0, 0)

#ifdef USE_INLINE_STDARG

#include <stdarg.h>

#define tray_client_register(__p0, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	tray_client_registerA(__p0, (struct TagItem *)_tags);})

#define tray_server_getclientdata(__p0, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	tray_server_getclientdataA(__p0, (struct TagItem *)_tags);})

#define tray_client_getserverdata(__p0, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	tray_client_getserverdataA(__p0, (struct TagItem *)_tags);})

#define tray_client_pushmethod(__p0, __p1, ...) \
	({ULONG _tags[] = { __VA_ARGS__ }; \
	tray_client_pushmethodA(__p0, __p1, (Msg )_tags);})

#endif

#endif /* !_PPCINLINE_TRAY_H */
