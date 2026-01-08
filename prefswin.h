#ifndef AMBIENT_PREFSWIN_H
#define AMBIENT_PREFSWIN_H
/*
 * $Id: prefswin.h,v 1.11 2014/03/26 17:05:54 geit Exp $
 */

#include <exec/types.h>

extern APTR prefswin;

#define NUMPAGES 10

#define PREFSWIN_NUMPAGES (NUMPAGES +  USE_DROP_EFFECT_PREFS)

static inline void setupprefs(APTR o, ULONG tag, ULONG defval)
{
	set(o, tag, defval);

	#if USE_INSTANTAPPLY_PREFS
	DoMethod(o, MUIM_Notify, tag, tag == MUIA_Pressed ? FALSE : MUIV_EveryTime,
		app, 5, MUIM_Application_PushMethod, prefswin, 2,
		MM_Prefswin_Main_Close, MV_Prefswin_Main_Close_Test
	);
	#endif
}

static inline void setupprefs_noset(APTR o, ULONG tag, ULONG defval)
{
	#if USE_INSTANTAPPLY_PREFS
	DoMethod(o, MUIM_Notify, tag, defval,
		app, 5, MUIM_Application_PushMethod, prefswin, 2,
		MM_Prefswin_Main_Close, MV_Prefswin_Main_Close_Test
	);
	#endif
}

typedef APTR (*GRPFUNC)(void);

struct prefsgroup {
	STRPTR english_name;
	ULONG labelid;
	GRPFUNC class;
	unsigned long *logo;
};

extern const struct prefsgroup prefsgrp[PREFSWIN_NUMPAGES];

APTR pstring(ULONG pid, ULONG maxlen, CONST_STRPTR label);
void storestring(APTR strobj, ULONG pid);
void storeattr(APTR obj, ULONG attr, ULONG pid);
void storepath(APTR strobj, ULONG pid);

void set_update(APTR o, ULONG attr, ULONG group, ULONG entity);

#endif /* AMBIENT_PREFSWIN_H */
