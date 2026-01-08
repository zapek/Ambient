#ifndef AMBIENT_PREFERENCES_UPDATE_H
#define AMBIENT_PREFERENCES_UPDATE_H
/*
 * $Id: preferences_update.h,v 1.3 2015/10/20 07:12:50 tcheko Exp $
 */

enum
{
	PREFS_TYPE_LONG,
	PREFS_TYPE_STRPTR,
	PREFS_TYPE_PENSPEC,
};

struct Key2DSI
{
	STRPTR	key;			/* SetPrefs ARexx argument n°1 */    
	ULONG	prefs_type;		/* uses PREFS_TYPE_enum, define the type of the second argument */
	LONG	lowerbound;		/* applies only to PREFS_TYPE_LONG */
	LONG	upperbound;		/* applies only to PREFS_TYPE_LONG */
	ULONG	dsi;			/* corresponding DSI_xxx preference */
};

ULONG preferences_apply_update(Object *app, ULONG id);
ULONG preferences_process_SetPrefs(Object *app, STRPTR key, STRPTR value);
STRPTR preferences_process_GetPrefs(STRPTR key);


#endif