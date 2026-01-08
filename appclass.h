#ifndef AMBIENT_APPCLASS_H
#define AMBIENT_APPCLASS_H

#include <intuition/classes.h>

enum {
	RXCMD_Delete = 1,
	RXCMD_Exchange,
	RXCMD_Format,
	RXCMD_Eject,
	RXCMD_Flash,
	RXCMD_IconInfo,
	RXCMD_LoadURI,
	RXCMD_Makedir,
	RXCMD_MakeLink,
	RXCMD_ParseURI,
	RXCMD_Panel,
	RXCMD_PlaySound,
	RXCMD_Snapshot,
	RXCMD_Sound,
	RXCMD_Unsnapshot,
	RXCMD_RefreshVars,
	RXCMD_Random,
	RXCMD_Rename,
	RXCMD_Run,
	RXCMD_ScreenToBack,
	RXCMD_ScreenToFront,
	RXCMD_Select,
	RXCMD_Settings,
	RXCMD_Shortcut,
	RXCMD_Sort,
	RXCMD_TBH,
	RXCMD_Version,
	RXCMD_Viewmode,
	RXCMD_Move,
	RXCMD_Copy,
	RXCMD_ClipboardCut,
	RXCMD_ClipboardCopy,
	RXCMD_ClipboardAdd,
	RXCMD_ClipboardPaste,
	RXCMD_ViewList,
	RXCMD_GetSelectedNames,
	RXCMD_LoadBackground,
	RXCMD_Parent,
	RXCMD_HistoryPrev,
	RXCMD_HistoryNext,
	RXCMD_Find,
	RXCMD_ViewClose,
	RXCMD_DropFile,
	RXCMD_CopySelectionToClipboard,
	RXCMD_AddBookmark,
	RXCMD_GetBookmarks,
	RXCMD_RemoveBookmark,
	RXCMD_EditMimeType,
	RXCMD_Menu,
	RXCMD_IconSelect,
	RXCMD_GetMimeType,
	RXCMD_GetDeficonPath,
	RXCMD_SetPrefs,
	RXCMD_GetPrefs,
	RXCMD_SavePrefs,
	RXCMD_Trash,
	RXCMD_RestoreFromTrash,
	RXCMD_EmptyTrashcan,
	RXCMD_Unmount,
	RXCMD_NetworksSettings,
	RXCMD_NetworksConnect,
};

struct ambient_command {
	STRPTR name;
	ULONG argnum;
	STRPTR args;
	ULONG id;
};

extern const struct ambient_command rexxcmds[];

struct MP_Application_DoRexx;

LONG application_dorexx( Class *cl, Object *obj, struct MP_Application_DoRexx *msg );
ULONG tr_dispose_objects( APTR obj, APTR *array, ULONG freearray );

#endif
