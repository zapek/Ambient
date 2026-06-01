/*************************************************************/
/* Includes and other common stuff for the MUI demo programs */
/*************************************************************/

/*
** $Id: muifuncs.h,v 1.1 2026/01/25 17:39:37 kronos Exp $
*/


/* MUI */
#ifdef MAINVERSION // built from mui makefile
#include "mui.h"
#else
#include <libraries/mui.h>
#endif


/* ANSI C */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* System */
#include <dos/dos.h>
#include <graphics/gfxmacros.h>
#include <workbench/workbench.h>

/* Prototypes */


#ifdef __MORPHOS__

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/utility.h>
#include <proto/asl.h>
#include <proto/muimaster.h>

#else

#include <clib/alib_protos.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/icon_protos.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <clib/gadtools_protos.h>
#include <clib/utility_protos.h>
#include <clib/asl_protos.h>
#ifndef __GNUC__
#include <clib/muimaster_protos.h>
#else
#include <inline/muimaster.h>
#endif

#endif

/* Compiler specific stuff */

#ifdef __MORPHOS__

#define REG(x)

#ifndef DISPATCHER
#define DISPATCHER(Name) \
static ULONG Name##_Dispatcher(void); \
struct EmulLibEntry GATE ##Name##_Dispatcher = { TRAP_LIB, 0, (void (*)(void)) Name##_Dispatcher }; \
static ULONG Name##_Dispatcher(void) { struct IClass *cl=(struct IClass*)REG_A0; Msg msg=(Msg)REG_A1; Object *obj=(Object*)REG_A2;
#define DISPATCHER_REF(Name) &GATE##Name##_Dispatcher
#define DISPATCHER_END }
#endif

#else

#define REG(x) register __ ## x

#define DISPATCHER(Name) ULONG ASM SAVEDS Name##Dispatcher(REG(a0) struct IClass *cl,REG(a2),REG(a1) Msg msg)
#define DISPATCHER_REF(Name) Name##Dispatcher
#define DISPATCHER_END

#endif

#if defined __MAXON__ || defined __GNUC__
	#define ASM
	#define SAVEDS
	#else
	#define ASM    __asm
	#define SAVEDS __saveds
#endif



/*************************/
/* Init & Fail Functions */
/*************************/

#include "MUIClasses.h"
 
//#define _between(a,x,b) ((x)>=(a) && (x)<=(b))
//#define _isinobject(x,y) (_between(_mleft(obj),(x),_mright(obj)) && _between(_mtop(obj),(y),_mbottom(obj)))



#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d) ((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))
#endif



#ifndef MUI_EHF_GUIMODE
#define MUI_EHF_GUIMODE  (1<<1)
#endif
