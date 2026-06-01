#ifndef AMBIENT_PREFS_H
#define AMBIENT_PREFS_H
/*
 * $Id: prefs.h,v 1.24 2026/04/25 23:05:00 jacadcaps Exp $
 */

/*
 * HOWTO: add a prefs item
 * -----------------------
 *
 * - add a DSI in prefs.h
 * - add a gp_bla var if needed here and in globalprefs.c
 * - set the default in prefs.c/set_default_prefs()
 * - add it to appclass.c/Application_LoadPrefs if it's a prefswindow item
 * - add it to updatelist.c if it's a prefswindow item
 *
 * HOWTO: add a prefs setting in prefswindow
 * -----------------------------------------
 *
 * HOWTO: remove a prefs item
 * --------------------------
 * just add // in front of it and don't touch it anymore
 *
 */

#include "mui_func.h" /* for MUI_PenSpec */
#include "prefspool.h"

/*
 * Path
 */
#define PREFS_OLDPATH        "PROGDIR:config/"    /* this is obsolete (tm) but still works for loading MAINPREFS_FILE */
#define PREFS_PATH           "SYS:Prefs/Ambient/"
#define VARS_PATH PREFS_PATH "vars/"              /* variable path */
#define MAINPREFS_FILE       "Ambient.prefs"
#define SHORTCUTS_FILE       "Shortcuts.prefs"
#define MIME_FILE            "MIME.prefs"
#define DESKTOP_FILE         "Desktop.prefs"
#define ADVANCED_FILE        "Advanced.conf"
#define STARTUP_FILE         "Startup.prefs"

/*
 * Prefs IDs
 */
#define MAINPREFSID         MAKE_ID('A','M','B',1) /* Global prefs (Ambient.prefs) */
#define PANELPREFSID        MAKE_ID('P','A','N',1) /* Panel prefs (<num>.prefs) */
#define SHORTCUTPREFSID     MAKE_ID('S','C','T',1) /* Shortcut prefs (Shortcuts.prefs) */
#define MIMEPREFSID         MAKE_ID('M','I','M',1) /* MIME prefs (MIME.prefs) */
#define DESKTOPPREFSID      MAKE_ID('D','S','K',1) /* Dekstop prefs (Desktop.prefs) */
#define STARTUPPREFSID      MAKE_ID('S','T','R',1) /* Startup prefs (Startup.prefs) */


/* Listpools (they're special) */
#define DSF_LISTPOOL (1 << 31UL)

/*
 * Prefs items
 */
#define DSI_GROUP(x) (x << 16)

/*
 * Dataspace IDs
 */

/* Global */
#define DSI_GLOBAL     (DSI_GROUP(0))

/* Background */
#define DSI_BACKGROUND                    (DSI_GROUP(1))
#define DSI_BACKGROUND_ROOT               (DSI_BACKGROUND + 1)
#define DSI_BACKGROUND_WINDOW             (DSI_BACKGROUND + 2)
#define DSI_BACKGROUND_ROOT_BGRENDER      (DSI_BACKGROUND + 3)
#define DSI_BACKGROUND_WINDOW_BGRENDER    (DSI_BACKGROUND + 4)
#define DSI_BACKGROUND_ROOT_BGCOLOR       (DSI_BACKGROUND + 5)
#define DSI_BACKGROUND_WINDOW_BGCOLOR     (DSI_BACKGROUND + 6)
#define DSI_BACKGROUND_ROOT_REFRESH_DELAY (DSI_BACKGROUND + 7)
#define DSI_BACKGROUND_TRANSITION         (DSI_BACKGROUND + 8)

/* Colors */
#define DSI_COLOR                     (DSI_GROUP(2))
#define DSI_COLOR_ROOT                (DSI_COLOR + 1)
#define DSI_COLOR_WINDOW              (DSI_COLOR + 2)
#define DSI_COLOR_ROOT_BACK_ENABLED   (DSI_COLOR + 3) /* obsolete */
#define DSI_COLOR_ROOT2               (DSI_COLOR + 4)
#define DSI_COLOR_WINDOW_BACK_ENABLED (DSI_COLOR + 5) /* obsolete */
#define DSI_COLOR_WINDOW2             (DSI_COLOR + 6)
#define DSI_COLOR_LASSO               (DSI_COLOR + 7)

/* Fonts */
#define DSI_FONT                (DSI_GROUP(3))
#define DSI_FONT_ROOT           (DSI_FONT + 1)
#define DSI_FONT_WINDOW         (DSI_FONT + 2)
#define DSI_FONT_ROOTSPACE      (DSI_FONT + 3)
#define DSI_FONT_WINDOWSPACE    (DSI_FONT + 4)
#define DSI_FONT_ROOT_EFFECT    (DSI_FONT + 5)
#define DSI_FONT_WINDOW_EFFECT  (DSI_FONT + 6)
#define DSI_FONT_ROOT_SMALL     (DSI_FONT + 7)

/* Debugging */
#define DSI_DEBUG               (DSI_GROUP(4))

/* Miscellaneous */
#define DSI_MISC                         (DSI_GROUP(5))
#define DSI_MISC_WBSTARTUP_PATH          (DSI_MISC + 1)
//#define DSI_MISC_EXECUTE_HISTORY_SAVE    (DSI_MISC + 2)
//#define DSI_MISC_NO_ICONFADE_ON_STARTUP  (DSI_MISC + 3) /* moved to advanced.conf */
//#define DSI_MISC_POPUPRENAME             (DSI_MISC + 4) /* moved to advanced.conf */
#define DSI_MISC_DRIVE_INFO  	         (DSI_MISC + 5)
#define DSI_MISC_REMEMBER_WINDOWS      (DSI_MISC + 6)
#define DSI_MISC_REMEMBER_DOCUMENTS    (DSI_MISC + 7)
#define DSI_MISC_MYMORPHOSICON         (DSI_MISC + 8)
#define DSI_MISC_DESKTOPDOUBLECLICK    (DSI_MISC + 9)
#define DSI_MISC_SMARTFILEOPERATIONS   (DSI_MISC + 10)
#define DSI_MISC_TRAPMORE              (DSI_MISC + 11)
#define DSI_MISC_TRAPMULTIVIEW         (DSI_MISC + 12)
#define DSI_MISC_CONTEXTMENUIMAGES     (DSI_MISC + 13)
#define DSI_MISC_CREATEICONFORNEWDRAWER (DSI_MISC + 14)
#define DSI_MISC_HIDEDOTFILENAMES      (DSI_MISC + 15)

/* Icon options */
#define DSI_ICON                (DSI_GROUP(6))
//#define DSI_ICON_NEWICONS       (DSI_ICON + 1)
//#define DSI_ICON_GLOWICONS      (DSI_ICON + 2)
//#define DSI_ICON_CACHE          (DSI_ICON + 3) /* might return.. */
#define DSI_ICON_SELECTEFFECT   (DSI_ICON + 4)
#define DSI_ICON_TINTVAL        (DSI_ICON + 5)
#define DSI_ICON_BRIGHTENVAL    (DSI_ICON + 6)
#define DSI_ICON_DARKENVAL      (DSI_ICON + 7)
#define DSI_ICON_TINTFADEVAL    (DSI_ICON + 8)
#define DSI_ICON_MINSIZE        (DSI_ICON + 9)
#define DSI_ICON_MAXSIZE        (DSI_ICON + 10)
#define DSI_ICON_HOVER          (DSI_ICON + 11) /* added by kiero */
//#define DSI_ICON_GHOSTY         (DSI_ICON + 12) /* moved to advanced.conf */
//#define DSI_ICON_ARROWS         (DSI_ICON + 13) /* arrows for shortcuts. moved to advanced.conf */
#define DSI_ICON_SEPARATEFILES  (DSI_ICON + 12) /* added by geit (kiero: do NOT override old items next time) */
#define DSI_ICON_SHORTCUTSIDENTIFIER (DSI_ICON + 14)
#define DSI_ICON_APPICONSIDENTIFIER (DSI_ICON + 15)
#define DSI_ICON_DEFGHOSTED     (DSI_ICON + 16)
#define DSI_ICON_AUTOSNAPSHOT   (DSI_ICON + 17)
#define DSI_ICON_DUALPNG        (DSI_ICON + 18)
#define DSI_ICON_FADE           (DSI_ICON + 19)

/* CLI launching */
#define DSI_CLI                 (DSI_GROUP(7))
#define DSI_CLI_DEVICE          (DSI_CLI + 1)
#define DSI_CLI_STACK           (DSI_CLI + 2)
#define DSI_CLI_NEWSHELL        (DSI_CLI + 3)

/* Drag & Drop */
#define DSI_DRAGDROP             (DSI_GROUP(8))
#if USE_SOLIDDRAG
#define DSI_DRAGDROP_DISPLAY     (DSI_DRAGDROP + 1)
#endif
//#define DSI_DRAGDROP_START_X     (DSI_DRAGDROP + 2)
//#define DSI_DRAGDROP_START_Y     (DSI_DRAGDROP + 3)
#define DSI_DRAGDROP_DROPEFFECT  (DSI_DRAGDROP + 4)
enum {
	DE_TINT,
	DE_BLUR,
	DE_BRIGHTEN,
	DE_DARKEN,
	DE_GREY,
	DE_NEGATIVE,
	DE_NEGFADE,
	DE_TINTFADE,
};

#define DSI_DRAGDROP_TINTVAL     (DSI_DRAGDROP + 5)
#define DSI_DRAGDROP_BRIGHTENVAL (DSI_DRAGDROP + 6)
#define DSI_DRAGDROP_DARKENVAL   (DSI_DRAGDROP + 7)
#define DSI_DRAGDROP_TINTFADEVAL (DSI_DRAGDROP + 8)

/* Copy */
//#define DSI_COPY                      (DSI_GROUP(9))
//#define DSI_COPY_MODE                 (DSI_COPY + 1)
//#define DSI_COPY_BUFSIZE            (DSI_COPY + 2)
//#define DSI_COPY_BUFFERSIZE           (DSI_COPY + 3)
//#define DSI_COPY_READAHEAD_BUFFERSIZE (DSI_COPY + 4)

/* Formatwin */
#define DSI_FORMAT                   (DSI_GROUP(10))
//#define DSI_FORMAT_FS                (DSI_FORMAT + 1)
#define DSI_FORMAT_ICON              (DSI_FORMAT + 2)
//#define DSI_FORMAT_FFS_INTERNATIONAL (DSI_FORMAT + 3)
#define DSI_FORMAT_SFS_CASE          (DSI_FORMAT + 4)
#define DSI_FORMAT_SFS_RECYCLED      (DSI_FORMAT + 5)
#define DSI_FORMAT_SFS_SHOWRECYCLED  (DSI_FORMAT + 6)

/* Fastlist */
#define DSI_FASTLIST                          (DSI_GROUP(11))
#define DSI_FASTLIST_COLOR_FILE_FG            (DSI_FASTLIST + 1)
#define DSI_FASTLIST_COLOR_FILE_BG            (DSI_FASTLIST + 2)
#define DSI_FASTLIST_COLOR_FILE_SEL_FG        (DSI_FASTLIST + 3)
#define DSI_FASTLIST_COLOR_FILE_SEL_BG        (DSI_FASTLIST + 4)
#define DSI_FASTLIST_COLOR_DIRECTORY_FG       (DSI_FASTLIST + 5)
#define DSI_FASTLIST_COLOR_DIRECTORY_BG       (DSI_FASTLIST + 6)
#define DSI_FASTLIST_COLOR_DIRECTORY_SEL_FG   (DSI_FASTLIST + 7)
#define DSI_FASTLIST_COLOR_DIRECTORY_SEL_BG   (DSI_FASTLIST + 8)
#define DSI_FASTLIST_COLOR_SOFTLINK_FG        (DSI_FASTLIST + 9)
#define DSI_FASTLIST_COLOR_SOFTLINK_BG        (DSI_FASTLIST + 10)
#define DSI_FASTLIST_COLOR_HARDLINK_FG        (DSI_FASTLIST + 11)
#define DSI_FASTLIST_COLOR_HARDLINK_BG        (DSI_FASTLIST + 12)
#define DSI_FASTLIST_COLOR_VOLUME_FG          (DSI_FASTLIST + 13)
#define DSI_FASTLIST_COLOR_VOLUME_BG          (DSI_FASTLIST + 14)
#define DSI_FASTLIST_COLOR_ASSIGN_FG          (DSI_FASTLIST + 15)
#define DSI_FASTLIST_COLOR_ASSIGN_BG          (DSI_FASTLIST + 16)
#define DSI_FASTLIST_COLOR_SOURCE_FG          (DSI_FASTLIST + 17)
#define DSI_FASTLIST_COLOR_SOURCE_BG          (DSI_FASTLIST + 18)
#define DSI_FASTLIST_COLOR_DESTINATION_FG     (DSI_FASTLIST + 19)
#define DSI_FASTLIST_COLOR_DESTINATION_BG     (DSI_FASTLIST + 20)
#define DSI_FASTLIST_COLOR_COLUMN_FG          (DSI_FASTLIST + 21)
#define DSI_FASTLIST_WINDOW_BG                (DSI_FASTLIST + 22)
#define DSI_FASTLIST_ICON_DISPLAY             (DSI_FASTLIST + 23) /* obsolete */
#define DSI_FASTLIST_HANDLE_ICONS             (DSI_FASTLIST + 24)
#define DSI_FASTLIST_COMPACT_SIZE_DISPLAY     (DSI_FASTLIST + 25)
#define DSI_FASTLIST_BOLD_DIRECTORIES         (DSI_FASTLIST + 26)
#define DSI_FASTLIST_ASSIGN_DISPLAY           (DSI_FASTLIST + 27)
#define DSI_FASTLIST_DEFAULT_WIDTH            (DSI_FASTLIST + 28) /* obsolete */
#define DSI_FASTLIST_DEFAULT_HEIGHT           (DSI_FASTLIST + 29) /* obsolete */
#define DSI_FASTLIST_DEFAULT_LEFT             (DSI_FASTLIST + 30) /* obsolete */
#define DSI_FASTLIST_DEFAULT_TOP              (DSI_FASTLIST + 31) /* obsolete */
#define DSI_FASTLIST_DEFAULT_FORMAT_DEVICES   (DSI_FASTLIST + 32)
#define DSI_FASTLIST_DEFAULT_FORMAT_FILES     (DSI_FASTLIST + 33)
#define DSI_FASTLIST_DEFAULT_MODE_FILES       (DSI_FASTLIST + 34)
#define DSI_FASTLIST_DEFAULT_MODE_DEVICES     (DSI_FASTLIST + 35)
#define DSI_FASTLIST_INLINE_EDIT_MODE         (DSI_FASTLIST + 36)
#define DSI_FASTLIST_FONT                     (DSI_FASTLIST + 37)
#define DSI_FASTLIST_ALTERNATED_ROWS          (DSI_FASTLIST + 38)
#define DSI_FASTLIST_HILIGHTED_SORTING_COLUMN (DSI_FASTLIST + 39)
#define DSI_FASTLIST_SELECTION_MODE           (DSI_FASTLIST + 40)

/* Panelgroup */
#define DSI_PANELGROUP                        (DSI_GROUP(12))
#define DSI_PANELGROUP_SIZE                   (DSI_PANELGROUP + 1)
#define DSI_PANELGROUP_ISHORIZ                (DSI_PANELGROUP + 2)
#define DSI_PANELGROUP_DRAGGADGET_PLACEMENT   (DSI_PANELGROUP + 3)
#define DSI_PANELGROUP_ZIPPING_ENABLED        (DSI_PANELGROUP + 4)
#define DSI_PANELGROUP_FRAMETYPE              (DSI_PANELGROUP + 5) /* obsolete */
#define DSI_PANELGROUP_FRAMELEFT              (DSI_PANELGROUP + 6) /* obsolete */
#define DSI_PANELGROUP_FRAMETOP               (DSI_PANELGROUP + 7) /* obsolete */
#define DSI_PANELGROUP_FRAMERIGHT             (DSI_PANELGROUP + 8) /* obsolete */
#define DSI_PANELGROUP_FRAMEBOTTOM            (DSI_PANELGROUP + 9) /* obsolete */
#define DSI_PANELGROUP_POSMODE                (DSI_PANELGROUP + 10)
#define DSI_PANELGROUP_X                      (DSI_PANELGROUP + 11)
#define DSI_PANELGROUP_Y                      (DSI_PANELGROUP + 12)
enum {
	BA_NONE,
	BA_TOP,
	BA_BOTTOM,
	BA_LEFT,
	BA_RIGHT,
};
#define DSI_PANELGROUP_BORDERATTACH           (DSI_PANELGROUP + 13)
#define DSI_PANELGROUP_LOCKED                 (DSI_PANELGROUP + 14)
#define DSI_PANELGROUP_IMAGESPECOLD           (DSI_PANELGROUP + 15) /* obsolete */
#define DSI_PANELGROUP_DEPTH                  (DSI_PANELGROUP + 16)
#define DSI_PANELGROUP_FRAMESPEC              (DSI_PANELGROUP + 17)
#define DSI_PANELGROUP_IMAGESPEC              (DSI_PANELGROUP + 18)
#define DSI_PANELGROUP_AUTOZIP                (DSI_PANELGROUP + 19)
#define DSI_PANELGROUP_ISSUBPANEL             (DSI_PANELGROUP + 20)
#define DSI_PANELGROUP_BACKMODE		          (DSI_PANELGROUP + 21)
#define DSI_PANELGROUP_BACKCOLOR	          (DSI_PANELGROUP + 22)
#define DSI_PANELGROUP_BACKDROP		          (DSI_PANELGROUP + 23)
#define DSI_PANELGROUP_HIDEDRAGBAR            (DSI_PANELGROUP + 24)
#define DSI_PANELGROUP_DOCKMODE               (DSI_PANELGROUP + 25)

/* Panel (listpool) */
#define DSI_LISTPOOL_PANEL                    (DSI_GROUP(13) | DSF_LISTPOOL)
#define DSI_LISTPOOL_PANEL_TYPE               (1)
#define DSI_LISTPOOL_PANEL_IMAGEPATH          (2)
#define DSI_LISTPOOL_PANEL_DISTANCE           (3)
#define DSI_LISTPOOL_PANEL_URI                (4)
#define DSI_LISTPOOL_TRAYPANEL_OBSOLETE1      (5) /* obsolete */
#define DSI_LISTPOOL_TRAYPANEL_OBSOLETE2      (6) /* obsolete */
#define DSI_LISTPOOL_PANEL_SUBPANEL		      (7) /* panelobject is in subpanel x */
#define DSI_LISTPOOL_PANEL_SUBPANEL_ROOT      (8) /* subpanel x is connected here */
#define DSI_LISTPOOL_PANEL_DIR                (9) /* dir for dirpanel */
#define DSI_LISTPOOL_PANEL_EXT_NAME           (10)   

/* Shortcuts (listpool) */
#define DSI_LISTPOOL_SHORTCUT                 (DSI_GROUP(14) | DSF_LISTPOOL)
#define DSI_LISTPOOL_SHORTCUT_PATH            (1)
#define DSI_LISTPOOL_SHORTCUT_X               (2)
#define DSI_LISTPOOL_SHORTCUT_Y               (3)

/* Panel (prefswin) */
#define DSI_PANEL                             (DSI_GROUP(15))
#define DSI_PANEL_ZIPSPEED                    (DSI_PANEL + 1) /* in miliseconds */
#define DSI_PANEL_AUTOSAVE_DROP               (DSI_PANEL + 2)
#define DSI_PANEL_AUTOSAVE_DELETE             (DSI_PANEL + 3)
#define DSI_PANEL_AUTOSAVE_MOVE               (DSI_PANEL + 4)
#define DSI_PANEL_AUTOSAVE_WINDOWPOS          (DSI_PANEL + 5)
#define DSI_PANEL_LAYOUT_GRID                 (DSI_PANEL + 6)
#define DSI_PANEL_HIGHLIGHT_EFFECT            (DSI_PANEL + 7)
#define DSI_PANEL_DRAGDROP_EFFECT	          (DSI_PANEL + 8)
#define DSI_PANEL_HIGHLIGHT_BRIGHTEN          (DSI_PANEL + 9)
#define DSI_PANEL_HIGHLIGHT_DARKEN            (DSI_PANEL + 10)
#define DSI_PANEL_DRAGDROP_BRIGHTEN	          (DSI_PANEL + 11)
#define DSI_PANEL_DRAGDROP_DARKEN	          (DSI_PANEL + 12)
#define DSI_PANEL_HIGHLIGHT_TINT           	  (DSI_PANEL + 13)
#define DSI_PANEL_DRAGDROP_TINT	     	      (DSI_PANEL + 14)
#define DSI_PANEL_HIGHLIGHT_TINTFADE          (DSI_PANEL + 15)
#define DSI_PANEL_DRAGDROP_TINTFADE   	      (DSI_PANEL + 16)
#define DSI_PANEL_SELECTED_EFFECT             (DSI_PANEL + 17)
#define DSI_PANEL_SELECTED_BRIGHTEN           (DSI_PANEL + 18)
#define DSI_PANEL_SELECTED_DARKEN             (DSI_PANEL + 20)
#define DSI_PANEL_SELECTED_TINT               (DSI_PANEL + 21)
#define DSI_PANEL_SELECTED_TINTFADE           (DSI_PANEL + 22)

enum {
	PANEL_EFFECT_NONE = 0,
	PANEL_EFFECT_CLONE_ICONVIEW,
	PANEL_EFFECT_LASSO,
	PANEL_EFFECT_BRIGHTEN,
	PANEL_EFFECT_DARKEN,
	PANEL_EFFECT_TINT,
	PANEL_EFFECT_BLUR,
	PANEL_EFFECT_GREY,
	PANEL_EFFECT_NEGATIVE,
	PANEL_EFFECT_NEGFADE,
	PANEL_EFFECT_TINTFADE,
};

/* MIME (listpool) */
#define DSI_LISTPOOL_MIME                     (DSI_GROUP(16) | DSF_LISTPOOL)
#define DSI_LISTPOOL_MIME_MEDIATYPE           (1) /* MEDIATYPE_#? defines */
#define DSI_LISTPOOL_MIME_SUBTYPE_NAME        (2) /* eg. "octet-stream" */
#define DSI_LISTPOOL_MIME_ICONPATH            (3) /* absolute path to the icon. if it's not absolute it's a theme path */
#define DSI_LISTPOOL_MIME_DESCRIPTION         (4) /* eg "JPEG pictures" */
#define DSI_LISTPOOL_MIME_RECOG               (5) /* recog "string" in recog language */
#define DSI_LISTPOOL_MIME_ACTION              (6) /* action to perform */
#define DSI_LISTPOOL_MIME_ACTION_NAME         (7) /* name of the action (viewname or program path) */
#define DSI_LISTPOOL_MIME_ACTION_FLAGS        (8) /* action flags */

/* Toolbar */
#define DSI_TOOLBAR                           (DSI_GROUP(17))
#define DSI_TOOLBAR_DISPLAYMODE               (DSI_TOOLBAR + 1)
enum {
	DM_IMAGETEXT = 0,
	DM_IMAGE_ONLY,
	DM_TEXT_ONLY
};
#define DSI_TOOLBAR_BROWSERMODE               (DSI_TOOLBAR + 3)
#define DSI_TOOLBAR_DEFINITION                (DSI_TOOLBAR + 4)

#define DSI_WINDOW_DEFAULT_WIDTH              (DSI_TOOLBAR + 5)
#define DSI_WINDOW_DEFAULT_HEIGHT             (DSI_TOOLBAR + 6)
#define DSI_WINDOW_DEFAULT_LEFT               (DSI_TOOLBAR + 7)
#define DSI_WINDOW_DEFAULT_TOP                (DSI_TOOLBAR + 8)

#define DSI_TOOLBAR_BUTTONFRAMESPEC           (DSI_TOOLBAR + 9)
#define DSI_TOOLBAR_BUTTONIMAGESPEC           (DSI_TOOLBAR + 10)

#define DSI_WINDOW_STATUSBARFORMAT            (DSI_TOOLBAR + 11)
#define DSI_WINDOW_DEFAULT_VIEW               (DSI_TOOLBAR + 12)
#define DSI_WINDOW_DEFAULT_VIEWMODE           (DSI_TOOLBAR + 13)
#define DSI_WINDOW_INHERIT_VIEWMODE           (DSI_TOOLBAR + 14)
#define DSI_WINDOW_STATUSBARCOLOR             (DSI_TOOLBAR + 15) // bitRocky


/* Key Shortcuts (listpool) */
#define DSI_LISTPOOL_KEYSHORTCUT                 (DSI_GROUP(19) | DSF_LISTPOOL)
#define DSI_LISTPOOL_KEYSHORTCUT_NAME            (1)
#define DSI_LISTPOOL_KEYSHORTCUT_MSGID           (2)
#define DSI_LISTPOOL_KEYSHORTCUT_DEFINITION      (3)
#define DSI_LISTPOOL_KEYSHORTCUT_ID              (4)
#define DSI_LISTPOOL_KEYSHORTCUT_FLAGS           (5)
#define DSI_LISTPOOL_KEYSHORTCUT_CMDTYPE         (6)
#define DSI_LISTPOOL_KEYSHORTCUT_CMDSTRING       (7)
#define DSI_LISTPOOL_KEYSHORTCUT_CMDFLAGS        (8)

#define DSI_LISTPOOL_KEYSHORTCUT_CHANGED         (DSI_GROUP(20)) /* changes are not signaled for listpools */

/* Bookmarks */
#define DSI_BOOKMARKS                            (DSI_GROUP(21))
#define DSI_BOOKMARKS_SHOWONLY                   1
#define DSI_PANEL_CLASSES                        (DSI_GROUP(22))
enum {
	BM_NAMESANDLOCATION = 0,
	BM_NAMESONLY,
	BM_LOCATIONONLY,
};

extern APTR mainprefspool;
extern APTR cloneprefspool;

/*
 * Global prefs
 */
#define _conf(X) gprefs->X

struct global_prefs {
	/* iconview fonts */
	struct atextfont *root_font;
	struct atextfont *root_font_small;
	struct atextfont *window_font;

	ULONG root_font_effect;
	ULONG window_font_effect;

	struct MUI_PenSpec root_pen1;
	struct MUI_PenSpec window_pen1;
	struct MUI_PenSpec root_pen2;
	struct MUI_PenSpec window_pen2;
	ULONG root_spaceline;
	ULONG window_spaceline;

	ULONG window_bgrender;
	struct MUI_PenSpec window_bgcolor;

	ULONG pushid;
	STRPTR screentitle_backup;

	STRPTR cli_device;
	ULONG cli_stack;

	ULONG misc_driveinfo;
	ULONG misc_mymorphosicon;
	ULONG misc_desktopdoubleclick;
	ULONG misc_smartfileoperations;
	ULONG misc_trapmultiview;
	ULONG misc_trapmore;
	ULONG misc_contextmenuimages;
	ULONG misc_createiconfornewdrawer;

	#if USE_SOLIDDRAG
	ULONG dragdrop_display;
	#endif

	#if USE_DROP_EFFECT_PREFS
	ULONG dragdrop_dropeffect;
	#endif
	struct MUI_PenSpec dragdrop_tintval;
	#if USE_DROP_EFFECT_PREFS
	ULONG dragdrop_brightenval;
	ULONG dragdrop_darkenval;
	struct MUI_PenSpec dragdrop_tintfadeval;
	#endif

	#if USE_DROP_EFFECT_PREFS
	ULONG icon_selecteffect;
	#endif
	struct MUI_PenSpec icon_tintval;
	#if USE_DROP_EFFECT_PREFS
	ULONG icon_brightenval;
	ULONG icon_darkenval;
	struct MUI_PenSpec icon_tintfadeval;
	#endif

	ULONG icon_minsize;
	ULONG icon_maxsize;

	ULONG icon_hover;

	ULONG icon_shortcutsidentifier;
	ULONG icon_autosnapshot;
	ULONG icon_dualpng;
	ULONG icon_defghosted;
	ULONG icon_fade;
	ULONG icon_appiconsidentifier;

	ULONG panel_zipspeed;

	ULONG toolbar_browsermode;
	ULONG toolbar_displaymode;
	STRPTR toolbar_definition;
	STRPTR toolbar_framespec;
	STRPTR toolbar_imagespec;

	ULONG  window_default_width;
	ULONG  window_default_height;
	ULONG  window_default_left;
	ULONG  window_default_top;
	ULONG  window_default_view;
	ULONG  window_default_viewmode;
	ULONG  window_inherit_viewmode;

	STRPTR window_statusbarformat;

	struct MUI_PenSpec window_statusbarcolor; // bitRocky

	struct MUI_PenSpec fl_color_file_fg;
	struct MUI_PenSpec fl_color_file_bg;
	struct MUI_PenSpec fl_color_file_sel_fg;
	struct MUI_PenSpec fl_color_file_sel_bg;
	struct MUI_PenSpec fl_color_directory_fg;
	struct MUI_PenSpec fl_color_directory_bg;
	struct MUI_PenSpec fl_color_directory_sel_fg;
	struct MUI_PenSpec fl_color_directory_sel_bg;
	struct MUI_PenSpec fl_color_softlink_fg;
	struct MUI_PenSpec fl_color_softlink_bg;
	struct MUI_PenSpec fl_color_hardlink_fg;
	struct MUI_PenSpec fl_color_hardlink_bg;
	struct MUI_PenSpec fl_color_volume_fg;
	struct MUI_PenSpec fl_color_volume_bg;
	struct MUI_PenSpec fl_color_assign_fg;
	struct MUI_PenSpec fl_color_assign_bg;
	struct MUI_PenSpec fl_color_source_fg;
	struct MUI_PenSpec fl_color_source_bg;
	struct MUI_PenSpec fl_color_destination_fg;
	struct MUI_PenSpec fl_color_destination_bg;
	struct MUI_PenSpec fl_color_column_fg;
	LONG   fl_window_bg;
	ULONG  fl_handle_icons;
	ULONG  fl_compact_size_display;
	ULONG  fl_bold_directories;
	ULONG  fl_assign_display;
	STRPTR fl_default_format_devices;
	STRPTR fl_default_format_files;
	STRPTR fl_default_mode_files;
	STRPTR fl_default_mode_devices;
	ULONG  fl_selection_mode;
	struct atextfont *fl_lister_font;

	struct MUI_PenSpec lasso_pen;

	ULONG  icon_separatefiles;

	/* bookmarks */
	ULONG bookmarks_showonly;

	/* startup prefs */
	ULONG misc_remember_windows;
	ULONG misc_remember_documents;

	/* here because fuck you, getprefs_ctx isn't O(1) */
	ULONG hide_dot_filenames;
};

extern struct global_prefs *gprefs;

void set_default_prefs(void);

void setprefs_ctx(APTR ctx, ULONG id, ULONG size, CONST_APTR data);
void setprefsstr_ctx(APTR ctx, ULONG id, CONST_STRPTR data);
void setprefslong_ctx(APTR ctx, ULONG id, ULONG v);

void setprefs_default_ctx(APTR ctx, ULONG id, ULONG size, CONST_APTR data);
void setprefsstr_default_ctx(APTR ctx, ULONG id, CONST_STRPTR data);
void setprefslong_default_ctx(APTR ctx, ULONG id, ULONG v);

STRPTR getprefsstr_ctx(APTR ctx, ULONG id);
APTR getprefs_ctx(APTR ctx, ULONG id);
ULONG getprefslong_ctx(APTR ctx, ULONG id);

#ifdef AMBIENT_PREFSCLONE_H
#error Sigh, you included prefsclone.h and prefs.h.. only include one of them and make sure you understand why
#endif

#define setprefs(id, size, data) setprefs_ctx(mainprefspool, id, size, data)
#define setprefslong(id, v) setprefslong_ctx(mainprefspool, id, v)
#define setprefsstr(id, data) setprefsstr_ctx(mainprefspool, id, data)
#define setprefs_default(id, size, data) setprefs_default_ctx(mainprefspool, id, size, data)
#define setprefslong_default(id, v) setprefslong_default_ctx(mainprefspool, id, v)
#define setprefsstr_default(id, data) setprefsstr_default_ctx(mainprefspool, id, data)

#define getprefs(id) getprefs_ctx(mainprefspool, id)
#define getprefsstr(id) getprefsstr_ctx(mainprefspool, id)
#define getprefslong(id) getprefslong_ctx(mainprefspool, id)

ULONG setprefsstr_lp(APTR ctx, APTR pitem, ULONG id, CONST_STRPTR s);
ULONG setprefslong_lp(APTR ctx, APTR pitem, ULONG id, ULONG v);

ULONG prefsclone_init(void);
void prefsclone_cleanup(ULONG writeback, ULONG remove);
void setprefs_clone(ULONG id, ULONG size, APTR data);
void setprefsstr_clone(ULONG id, STRPTR data);
void setprefslong_clone(ULONG id, ULONG v);
STRPTR getprefsstr_clone(ULONG id);
APTR getprefs_clone(ULONG id);
ULONG getprefslong_clone(ULONG id);


ULONG prefs_init(void);
void prefs_cleanup(void);

ULONG backup_prefs(void);
void restore_prefs(void);

#endif /* AMBIENT_PREFS_H */
