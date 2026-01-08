#ifndef AMBIENT_COMMAND_H
#define AMBIENT_COMMAND_H
/*
 * $Id: command.h,v 1.10 2022/12/27 19:39:59 jacadcaps Exp $
 */

struct MUIP_FindUData;

/* Ambient Commands */
enum {
	AC_TITLE, /* just a label */
	AC_INTERNAL,
	AC_AMIGADOS,
	AC_WORKBENCH,
	AC_SCRIPT,
	AC_AREXX,
	AC_SYNCHRONOUS = 0x100				/* can be specified with AC_INTERNAL. Will result in synchronous execution if possible */
};

#define AC_SYNC_MASK 0x100
#define AC_TYPE_MASK 0xff

/* Allow switches (none means all) */
#define AS_ANY                     0
#define AS_DEVICE           (1L << 0)
#define AS_DRAWER           (1L << 1)
#define AS_PROJECT          (1L << 2)
#define AS_TOOL             (1L << 3)
#define AS_APPICON          (1L << 4)
#define AS_HASICONS         (1L << 5)  /* iconview has icons */
#define AS_PANEL_UNLOCKED   (1L << 6)  /* for menus which support locking mode, disables them if it's locked */
#define AS_SHORTCUT         (1L << 7)  /* for shortcuts */
#define AS_PANEL_PROPERTIES (1L << 8)
#define AS_PANEL_ABOUT      (1L << 9)
#define AS_PANEL_HELP       (1L << 10)
#define AS_FORMATABLE       (1L << 11)
#define AS_REMOVABLE        (1L << 12)
#define AS_MYCOMPUTER       (1L << 13)
#define AS_ASSIGN           (1L << 14)
#define AS_FILEVIEW         (1L << 15) /* allowed for file management */
#define AS_DEVICEVIEW       (1L << 16) /* allowed for device management (my morphos window) */
#define AS_ICONVIEW         (1L << 17) /* allowed for iconview */
#define AS_LISTVIEW         (1L << 18) /* allowed for listview */
#define AS_ICONIFIED_VIEW   (1L << 19) /* iconified view icon object */
#define AS_HASTRASHCAN      (1L << 21)
#define AS_ISTRASHCAN       (1L << 22)
#define AS_INTRASHCAN       (1L << 23)
#define AS_UNMOUNTABLE      (1L << 24)
#define AS_NETWORKSFS       (1L << 25)
#define AS_DISABLED         (1L << 31) /* that flag is special: if set, the menu is disabled if the condition is not met instead of being removed */

/* Flags */
#define AF_SUB         1 /* depth is limited to 16 levels */
#define AF_SUBSUB      2
#define AF_SUBSUBSUB   3
#define AF_TOGGLE      (1UL << 5) /* set the toggle flag */
#define AF_CHECKIT     (1UL << 6) /* set to have the checkit flag */
#define AF_CHECKED     (1UL << 7) /* set to have the checked flag put */
#define AF_ENABLED     (1UL << 8) /* set when the menu is enabled */
#define AF_NO_GROUPING (1UL << 9) /* set when the menu item doesnt support grouping */

#define FLAGS_DEPTH(x) (x & 0xf)


struct command_menu { /* XXX: hm, we should have a shortcut keys there */
	STRPTR name;
	ULONG  type;
	ULONG  allow;
	ULONG  flags;
	ULONG  mutex;
	CONST_STRPTR args;
	APTR   actionnode;          /* if != NULL then we have to execute it instead of single type + args. attached commandlist is parsed */
	BOOL   (*enablefunc)(void); /* if != NULL, this callback func is called, to check if an item should be enabled */
	BOOL   (*allowfunc)(ULONG allow); /* if != NULL, this callback func is called, to check if an item should be allowed */
	ULONG  shortcut_msg_id;     /* Localized shortcut catalog id */
};

struct command_shortcut {
	ULONG allow;
	CONST_STRPTR shortcut;
	CONST_STRPTR args;
};

APTR findudata(APTR obj, struct MUIP_FindUData *msg); /* to share code between the menu classes XXX: find a better place maybe.. */

#endif /* AMBIENT_COMMAND_H */
