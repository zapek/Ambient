#ifndef AMBIENT_WB_LIB_H
#define AMBIENT_WB_LIB_H
/*
 * $Id: lib.h,v 1.7 2016/08/11 17:02:38 itix Exp $
 */

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <dos/dos.h>

extern struct Library *WorkbenchBase;

struct LibBase
{
	struct Library Lib;
	WORD Initialized;
	BPTR SegList;
	struct ExecBase *SBase;

	// App windows
	struct SignalSemaphore appwinsem;
	APTR appwindows;

	// Base protection
	struct SignalSemaphore basesem;
};

int lib_init(struct ExecBase *SBase, struct LibBase *base);
void lib_cleanup(struct LibBase *base);


struct AppWindow;
struct AppIcon;
struct AppMenuItem;
struct AppWindowDropZone;
struct Window;


struct Library *LIB_Open(void);
BPTR LIB_Close(void);
BPTR LIB_Expunge(void);
ULONG LIB_GetQueryAttr(void);

/* private stuff */
void LIB_UpdateWorkbench(void);
void LIB_QuoteWorkbench(void);
LONG LIB_StartWorkbench(void);

//struct AppWindow *LIB_AddAppWindowA(void);
//BOOL LIB_RemoveAppWindow(void);
struct AppIcon *LIB_AddAppIconA(void);
BOOL LIB_RemoveAppIcon(void);
struct AppMenuItem *LIB_AddAppMenuItemA(void);
BOOL LIB_RemoveAppMenuItem(void);

/* private stuff */
LONG LIB_WBConfig(void);

void LIB_WBInfo(void);

/* "OS" 3.5 shit  (V44) */
BOOL LIB_OpenWorkbenchObjectA(void);
BOOL LIB_CloseWorkbenchObjectA(void);
BOOL LIB_WorkbenchControlA(void);
struct AppWindowDropZone * LIB_AddAppWindowDropZoneA(void);
BOOL LIB_RemoveAppWindowDropZone(void);
BOOL LIB_ChangeWorkbenchSelectionA(void);
BOOL LIB_MakeWorkbenchObjectVisibleA(void);

/* App window stuff */
APTR LIB_AppWindowObtain(struct Window *win, struct LibBase *base);
VOID LIB_AppWindowRelease(struct LibBase *base);
struct AppWindow *LIB_AddAppWindowA(void);
BOOL LIB_RemoveAppWindow(void);

/* New V50 stuff */
BOOL LIB_ManageDesktopObjectA(CONST_STRPTR name, LONG action, const struct TagItem *tags);
BOOL LIB_CreateDrawerA(CONST_STRPTR drawer, const struct TagItem *tags);
BOOL LIB_CreateIconA(CONST_STRPTR name, struct TagItem *tags);

#endif /* AMBIENT_WB_LIB_H */
