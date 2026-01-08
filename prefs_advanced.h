#ifndef AMBIENT_PREFS_ADVANCED_H
#define AMBIENT_PREFS_ADVANCED_H
/*
 * $Id: prefs_advanced.h,v 1.15 2022/01/31 12:55:26 geit Exp $
 */

#include "ambient.h"

/*  Advanced.conf types + c type mapping
 */
enum {
	PA_STRING = 0,    /* STRPTR             */
	PA_BOOL,          /* BOOL               */
	PA_INT,           /* LONG               */
	PA_UINT,          /* ULONG              */
	PA_COLOUR,        /* ULONG (0xAARRGGBB) */
};

/*  see advanced_prefs.c (must be in sync).
 */
struct advanced_prefs 
{
	STRPTR sfx_boot;
	STRPTR sfx_panelzip;
	STRPTR sfx_panelunzip;
	BOOL   popuprename;
	STRPTR commodityfilter;
	STRPTR exchangehotkey;
	BOOL   autosort;
	BOOL   mmkeyevents;
	ULONG  progresswindowdelay;
	BOOL   progresswindowfocus;
	BOOL   infowinautoversion;
	BOOL   infowinautomd5sum;
	ULONG  infowinautolimit;
	BOOL   nodatatypesmimename;
	ULONG  panelspacersize;
	BOOL   donotputrefwindowtosleep;
#if USE_AVCODEC
	BOOL   videopreview;
#endif
	BOOL   differentinactivewindowtitles;
	BOOL   iconzoomslider;
};

#define PA_MAXVARIABLENAMELENGTH 32  /* XXX: should be dynamical (e.g. on default init) */

extern struct advanced_prefs *aprefs;


/*  save access functions (semaphore protected)
 */
#define _ap_bool(n)   ((BOOL)  prefs_advanced_getvalue( #n ))
#define _ap_string(n) ((STRPTR)prefs_advanced_getvalue( #n ))
#define _ap_colour(n) (        prefs_advanced_getvalue( #n ))
#define _ap_uint(n)   (        prefs_advanced_getvalue( #n ))
#define _ap_int(n)    ((LONG)  prefs_advanced_getvalue( #n ))

/*  quick access (unprotected)
 */
#define _aprefs(n)    aprefs->n


/*  flags
 */
#define PAF_INITONLY    (1<<0)  /* variable needs restart of Ambient to have effect           */
#define PAF_UPDATED     (1<<1)  /* temp. flag used to reset variables on Advanced.conf reload */
#define PAF_USERDEFINED (1<<2)  /* if 1 it's the same as (n_default != n_content)             */


struct pa_node {
	struct Node n_node;

	/* fixed */
	STRPTR      n_name;         
	UBYTE       n_type;         
	ULONG       n_offset;       /* offset in struct advanced_prefs                  */
	STRPTR      n_parameter;    /* describing string for optional parameters        */
	ULONG       n_default;      /* Ambients internal default value                  */ 

	/* dynamic */
	ULONG       n_value;        /* current value of variable                        */
	ULONG       n_flags;        /* current flag set                                 */
};


/*  protos 
 */
ULONG prefs_advanced_init(void);
void  prefs_advanced_cleanup(void);
void  prefs_advanced_refresh(BOOL);
ULONG prefs_advanced_getvalue(CONST_STRPTR);
BOOL  prefs_advanced_setvalue(struct pa_node *, CONST ULONG v);
BOOL  prefs_advanced_namesetvalue(CONST_STRPTR, CONST ULONG v);
BOOL  prefs_advanced_resetvalue(struct pa_node *);
BOOL  prefs_advanced_save(void);

/*  extern access to private prefs storage data
 */
struct MinList * prefs_advanced_lockstorage(void);
void             prefs_advanced_unlockstorage(void);


#endif /* AMBIENT_PREFS_ADVANCED_H */
