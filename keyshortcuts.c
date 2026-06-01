/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2016 Ambient Open Source Team
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * $Id: keyshortcuts.c,v 1.16 2025/05/31 21:57:11 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <proto/exec.h>
#include <devices/rawkeycodes.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <devices/inputevent.h>

/* private */
#include "command.h"
#include "action.h"
#include "mimetype.h"
#include "mui_func.h"
#include "actiondispatcherclass.h"
#include "keyshortcuts.h"
#include "dragdrop.h"
#include "ambient_cat.h"
#include "prefs.h"
#include "viewclass.h"

typedef struct _code_info
{
	CONST_STRPTR name; /* key name */
	input_code   code; /* rawkey code */
} code_info;

#define MAX_TOKEN_LEN 32 /* update if needed */

/* map string <-> token foe special tokens (to parse mui keyadjust strings) */
static const code_info keylist[] =
{
	{ "space",     RAWKEY_SPACE },
	{ "backspace", RAWKEY_BACKSPACE},
	{ "tab",       RAWKEY_TAB },
	{ "return",    RAWKEY_RETURN },
	{ "esc",       RAWKEY_ESCAPE },
	{ "delete",    RAWKEY_DELETE },
	{ "insert",    RAWKEY_INSERT },
	{ "page_up",   RAWKEY_PAGEUP },
	{ "page_down", RAWKEY_PAGEDOWN },
	{ "f11",       RAWKEY_F11 },
	{ "up",        RAWKEY_UP },
	{ "down",      RAWKEY_DOWN },
	{ "right",     RAWKEY_RIGHT },
	{ "left",      RAWKEY_LEFT },

	{ "f1",        RAWKEY_F1 },
	{ "f2",        RAWKEY_F2 },
	{ "f3",        RAWKEY_F3 },
	{ "f4",        RAWKEY_F4 },
	{ "f5",        RAWKEY_F5 },
	{ "f6",        RAWKEY_F6 },
	{ "f7",        RAWKEY_F7 },
	{ "f8",        RAWKEY_F8 },
	{ "f9",        RAWKEY_F9 },
	{ "f10",       RAWKEY_F10 },
	
	{ "help",      RAWKEY_HELP },

	{ "lshift",    RAWKEY_LSHIFT },
	{ "rshift",    RAWKEY_RSHIFT },
	{ "capslock",  RAWKEY_CAPSLOCK },
	{ "control",   RAWKEY_CONTROL },
	{ "control",   RAWKEY_LCONTROL },
	{ "lalt",      RAWKEY_LALT },
	{ "ralt",      RAWKEY_RALT },
	{ "lcommand",  RAWKEY_LAMIGA },
	{ "rcommand",  RAWKEY_RAMIGA },

	{ "scrlock",   RAWKEY_SCRLOCK },
	{ "prtscr",    RAWKEY_PRTSCREEN },
	{ "numlock",   RAWKEY_NUMLOCK },
	{ "break",     RAWKEY_PAUSE },
	{ "f12",       RAWKEY_F12 },

	{ "home",      RAWKEY_HOME },
	{ "end",       RAWKEY_END },

/* keypad */

	{ "numpad 0",         RAWKEY_KP_0 },
	{ "numpad 1",         RAWKEY_KP_1 },
	{ "numpad 2",         RAWKEY_KP_2 },
	{ "numpad 3",         RAWKEY_KP_3 },
	{ "numpad 4",         RAWKEY_KP_4 },
	{ "numpad 5",         RAWKEY_KP_5 },
	{ "numpad 6",         RAWKEY_KP_6 },
	{ "numpad .",         RAWKEY_KP_DECIMAL },
	{ "numpad 7",         RAWKEY_KP_7 },
	{ "numpad 8",         RAWKEY_KP_8 },
	{ "numpad 9",         RAWKEY_KP_9 },
	{ "numpad -",         RAWKEY_KP_MINUS },
	{ "enter",            RAWKEY_KP_ENTER },
	{ "numpad /",         RAWKEY_KP_DIVIDE },
	{ "numpad *",         RAWKEY_KP_MULTIPLY },
	{ "numpad +",         RAWKEY_KP_PLUS },

/* media keys */

	{ "media_stop",       RAWKEY_CDTV_STOP },
	{ "media_play",       RAWKEY_CDTV_PLAY },
	{ "media_prev",       RAWKEY_CDTV_PREV },
	{ "media_next",       RAWKEY_CDTV_NEXT },
	{ "media_rewind",     RAWKEY_CDTV_REW },
	{ "media_forward",    RAWKEY_CDTV_FF },

/* mouse */

	{ "mouse_leftpress",   SELECTDOWN },
	{ "mouse_rightpress",  MENUDOWN },
	{ "mouse_middlepress", MIDDLEDOWN },
	{ "mouse_fourthpress", 0x7e },
	{ "wheel_down",       NM_WHEEL_DOWN },
	{ "wheel_up",         NM_WHEEL_UP },
	{ "wheel_right",      NM_WHEEL_RIGHT },
	{ "wheel_left",       NM_WHEEL_LEFT },

	{ 0,           0 },
};

enum { CODE_NONE = 0x8000 };

/* sequence  */

#define SEQ_DEF_6(a,b,c,d,e,f) {{ a, b, c, d, e, f, CODE_NONE, CODE_NONE, CODE_NONE, CODE_NONE, CODE_NONE, CODE_NONE, CODE_NONE, CODE_NONE, CODE_NONE, CODE_NONE }}
#define SEQ_DEF_5(a,b,c,d,e) SEQ_DEF_6(a,b,c,d,e,CODE_NONE)
#define SEQ_DEF_4(a,b,c,d) SEQ_DEF_5(a,b,c,d,CODE_NONE)
#define SEQ_DEF_3(a,b,c) SEQ_DEF_4(a,b,c,CODE_NONE)
#define SEQ_DEF_2(a,b) SEQ_DEF_3(a,b,CODE_NONE)
#define SEQ_DEF_1(a) SEQ_DEF_2(a,CODE_NONE)
#define SEQ_DEF_0 SEQ_DEF_1(CODE_NONE)

static ULONG seq_len(seq_t * seq)
{
	int codenum;
	for (codenum = 0; codenum < SEQ_MAX && seq->code[codenum] != CODE_NONE; codenum++);
	return codenum;
}

static void seq_set_0(seq_t *seq)
{
	int codenum;
	for (codenum = 0; codenum < SEQ_MAX; codenum++)
		seq->code[codenum] = CODE_NONE;
}

#if 0 /* a lot of unused stuff (geit) */
static void seq_set_1(seq_t *seq, input_code code)
{
	int codenum;
	seq->code[0] = code;
	for (codenum = 1; codenum < SEQ_MAX; codenum++)
		seq->code[codenum] = CODE_NONE;
}

static void seq_set_2(seq_t *seq, input_code code1, input_code code2)
{
	int codenum;
	seq->code[0] = code1;
	seq->code[1] = code2;
	for (codenum = 2; codenum < SEQ_MAX; codenum++)
		seq->code[codenum] = CODE_NONE;
}

static void seq_set_3(seq_t *seq, input_code code1, input_code code2, input_code code3)
{
	int codenum;
	seq->code[0] = code1;
	seq->code[1] = code2;
	seq->code[2] = code3;
	for (codenum = 3; codenum < SEQ_MAX; codenum++)
		seq->code[codenum] = CODE_NONE;
}

static void seq_set_4(seq_t *seq, input_code code1, input_code code2, input_code code3, input_code code4)
{
	int codenum;
	seq->code[0] = code1;
	seq->code[1] = code2;
	seq->code[2] = code3;
	seq->code[3] = code4;
	for (codenum = 4; codenum < SEQ_MAX; codenum++)
		seq->code[codenum] = CODE_NONE;
}

static void seq_set_5(seq_t *seq, input_code code1, input_code code2, input_code code3, input_code code4, input_code code5)
{
	int codenum;
	seq->code[0] = code1;
	seq->code[1] = code2;
	seq->code[2] = code3;
	seq->code[3] = code4;
	seq->code[4] = code5;
	for (codenum = 5; codenum < SEQ_MAX; codenum++)
		seq->code[codenum] = CODE_NONE;
}
static void seq_copy(seq_t *seqdst, CONST seq_t *seqsrc)
{
	*seqdst = *seqsrc;
}

static int seq_cmp(CONST seq_t *seqa, CONST seq_t *seqb)
{
	int codenum;
	for (codenum = 0; codenum < SEQ_MAX; codenum++)
		if (seqa->code[codenum] != seqb->code[codenum])
			return -1;
	return 0;
}
#endif

/* shortcut list */

static struct SignalSemaphore kscsem;
static struct MinList shortcut_list;

/* default shortcuts. these are examples, heavily subject to changes */

static const struct key_shortcut_t default_shortcuts[] =
{
	/* delete shortcut */
	{
		SHORTCUT_ID_DELETE,
		"Delete",  
		MSG_KEYSHORTCUT_DELETE,
		SEQ_DEF_1(RAWKEY_DELETE),
		{ AC_INTERNAL, "delete %sp", ACTION_FLAG_MULTIPLE, FVS_FS_OBJECTS },
		SHORTCUT_FLAG_BUILTIN | SHORTCUT_FLAG_ENABLED
	},

	/* rename shortcut */
	{
		SHORTCUT_ID_RENAME,
		"Rename",
		MSG_KEYSHORTCUT_RENAME,
		SEQ_DEF_2(RAWKEY_RAMIGA, RAWKEY_R),
		{ AC_INTERNAL, "rename %sp", ACTION_FLAG_MULTIPLE, FVS_FILES | FVS_DRAWERS | FVS_VOLUMES | FVS_SHORTCUTS },
		SHORTCUT_FLAG_BUILTIN | SHORTCUT_FLAG_ENABLED
	},

	/* information shortcut */
	{
		SHORTCUT_ID_INFORMATION,
		"Information",
		MSG_KEYSHORTCUT_INFORMATION,
		SEQ_DEF_2(RAWKEY_RAMIGA, RAWKEY_I),
		{ AC_INTERNAL, "iconinfo %sp", ACTION_FLAG_MULTIPLE, FVS_ALL },
		SHORTCUT_FLAG_BUILTIN | SHORTCUT_FLAG_ENABLED
	},

	/* parent shortcut */
	{
		SHORTCUT_ID_PARENT,
		"Parent",
		MSG_KEYSHORTCUT_PARENT,
		SEQ_DEF_1(RAWKEY_BACKSPACE),
		{ AC_INTERNAL, "parent VIEWID %Si", 0, FVS_ALL },
		SHORTCUT_FLAG_BUILTIN | SHORTCUT_FLAG_ENABLED
	},

#if 0
	// Doesnt work?
	/* reload shortcut */
	{
		SHORTCUT_ID_RELOAD,
		"Reload",
		MSG_KEYSHORTCUT_PARENT,
		SEQ_DEF_1(RAWKEY_F5),
		{ AC_INTERNAL, "loaduri reload nonewwin viewid %Si", 0, 0 },
		SHORTCUT_FLAG_BUILTIN | SHORTCUT_FLAG_ENABLED
	},
#endif

	/* Leave out shortcut */
	{
		SHORTCUT_ID_LEAVEOUT,
		"Leave out",
		MSG_KEYSHORTCUT_LEAVEOUT,
		SEQ_DEF_2(RAWKEY_RAMIGA, RAWKEY_L),
		{ AC_INTERNAL, "shortcut %sp add", 0, FVS_FILES | FVS_DRAWERS | FVS_VOLUMES },
		SHORTCUT_FLAG_BUILTIN | SHORTCUT_FLAG_ENABLED
	},

	/* Put away shortcut */
	{
		SHORTCUT_ID_PUTAWAY,
		"Put away",
		MSG_KEYSHORTCUT_PUTAWAY,
		SEQ_DEF_2(RAWKEY_RAMIGA, RAWKEY_P),
		{ AC_INTERNAL, "shortcut %sp remove", 0, FVS_SHORTCUTS },
		SHORTCUT_FLAG_BUILTIN | SHORTCUT_FLAG_ENABLED
	},

/* selection shortcuts */

	/* select all shortcut */
	{
		SHORTCUT_ID_SELECTALL,
		"Select all",
		MSG_KEYSHORTCUT_SELECTALL,
		SEQ_DEF_1(RAWKEY_KP_PLUS),
		{ AC_INTERNAL, "select VIEWID %Si ALL", 0, 0 },
		SHORTCUT_FLAG_BUILTIN | SHORTCUT_FLAG_ENABLED
	},

	/* select none shortcut */
	{
		SHORTCUT_ID_SELECTNONE,
		"Select none",
		MSG_KEYSHORTCUT_SELECTNONE,
		SEQ_DEF_1(RAWKEY_KP_MINUS),
		{ AC_INTERNAL, "select VIEWID %Si NONE", 0, 0 },
		SHORTCUT_FLAG_BUILTIN | SHORTCUT_FLAG_ENABLED
	},

	/* select invert shortcut */
	{
		SHORTCUT_ID_SELECTINVERT,
		"Select invert",
		MSG_KEYSHORTCUT_SELECTINVERT,
		SEQ_DEF_1(RAWKEY_NUMLOCK),
		{ AC_INTERNAL, "select VIEWID %Si", 0, 0 },
		SHORTCUT_FLAG_BUILTIN | SHORTCUT_FLAG_ENABLED
	},

	/* select pattern shortcut */
	{
		SHORTCUT_ID_SELECTPATTERN,
		"Select with pattern",
		MSG_KEYSHORTCUT_SELECTPATTERN,
		SEQ_DEF_1(RAWKEY_KP_MULTIPLY),
		{ AC_INTERNAL, "select VIEWID %Si PATTERN", 0, 0 },
		SHORTCUT_FLAG_BUILTIN | SHORTCUT_FLAG_ENABLED
	},

/* clipboard shortcuts */

	/* clipboard copy shortcut */
	{
		SHORTCUT_ID_CLIPBOARDCOPY,
		"Copy to clipboard",
		MSG_KEYSHORTCUT_CLIPBOARDCOPY,
		SEQ_DEF_2(RAWKEY_CONTROL, RAWKEY_C),
		{ AC_INTERNAL, "ClipboardCopy %sp", ACTION_FLAG_MULTIPLE, FVS_FILES | FVS_DRAWERS },
		SHORTCUT_FLAG_BUILTIN | SHORTCUT_FLAG_ENABLED
	},

	/* clipboard cut shortcut */
	{
		SHORTCUT_ID_CLIPBOARDCUT,
		"Cut to clipboard",
		MSG_KEYSHORTCUT_CLIPBOARDCUT,
		SEQ_DEF_2(RAWKEY_CONTROL, RAWKEY_X),
		{ AC_INTERNAL, "ClipboardCut %sp", ACTION_FLAG_MULTIPLE, FVS_FILES | FVS_DRAWERS },
		SHORTCUT_FLAG_BUILTIN | SHORTCUT_FLAG_ENABLED
	},

	/* clipboard paste shortcut */
	{
		SHORTCUT_ID_CLIPBOARDPASTE,
		"Paste from clipboard",
		MSG_KEYSHORTCUT_CLIPBOARDPASTE,
		SEQ_DEF_2(RAWKEY_CONTROL, RAWKEY_V),
		{ AC_INTERNAL, "ClipboardPaste VIEWID %Si", 0, 0 },
		SHORTCUT_FLAG_BUILTIN | SHORTCUT_FLAG_ENABLED
	},

	/* clipboard pasteas shortcut */
	{
		SHORTCUT_ID_CLIPBOARDPASTEAS,
		"Paste from clipboard as",
		MSG_KEYSHORTCUT_CLIPBOARDPASTEAS,
		SEQ_DEF_2(RAWKEY_CONTROL, RAWKEY_B),
		{ AC_INTERNAL, "ClipboardPaste VIEWID %Si AS", 0, 0 },
		SHORTCUT_FLAG_BUILTIN | SHORTCUT_FLAG_ENABLED
	},

/* modes */

	/* Next mode */
	{
		SHORTCUT_ID_CYCLEMODE,
		"Cycle mode",
		MSG_KEYSHORTCUT_CYCLEMODE,
		SEQ_DEF_1(RAWKEY_KP_5),
		{ AC_INTERNAL, "Viewmode \"next 0\" %Si", 0, 0 },
		SHORTCUT_FLAG_BUILTIN | SHORTCUT_FLAG_ENABLED
	},

	/* Next submode */
	{
		SHORTCUT_ID_CYCLESUBMODE,
		"Cycle submode",
		MSG_KEYSHORTCUT_CYCLESUBMODE,
		SEQ_DEF_1(RAWKEY_KP_8),
		{ AC_INTERNAL, "Viewmode \"0 next\" %Si", 0, 0 },
		SHORTCUT_FLAG_BUILTIN | SHORTCUT_FLAG_ENABLED
	},

/* misc */

	/* Find files */
	{
		SHORTCUT_ID_FIND,
		"Find files",
		MSG_KEYSHORTCUT_FINDFILES,
		SEQ_DEF_2(RAWKEY_RAMIGA, RAWKEY_F),
		{ AC_INTERNAL, "Find %S", 0, 0 },
		SHORTCUT_FLAG_BUILTIN | SHORTCUT_FLAG_ENABLED
	},
	/* complete ... */



	/* end marker */
	{
		0,
		NULL,
		0,
		SEQ_DEF_0,
		{ 0, NULL, 0, 0 },
		0
	}
};

/* functions */

static ULONG keyshortcuts_load(void);
static void keyshortcutnode_delete(struct key_shortcut_node * n);

struct key_shortcut_t *	keyshortcut_create(STRPTR name)
{
	ULONG rc = TRUE;
	struct key_shortcut_t * shortcut;

	shortcut = (struct key_shortcut_t *) AllocVecTaskPooled(sizeof(*shortcut));

	if(shortcut)
	{
		shortcut->id = 0;
		shortcut->name = AllocVecTaskPooled(strlen(name) + 1);
		if(shortcut->name)
		{
			strcpy(shortcut->name, name);
		}
		else
		{
			rc = FALSE;
		}
		shortcut->msgid = 0;
		seq_set_0(&shortcut->sequence);
		shortcut->flags = SHORTCUT_FLAG_ENABLED;

		shortcut->action.type = AC_INTERNAL;
		shortcut->action.command = NULL;
		shortcut->action.flags = 0;
		shortcut->action.selectionmask = 0;
	}

	if(rc == FALSE)
	{
		keyshortcut_delete(shortcut);
		shortcut = NULL;
	}

	return shortcut;
}

void keyshortcut_delete(struct key_shortcut_t * shortcut)
{
	if(shortcut)
	{
		if(shortcut->name)
		{
			FreeVecTaskPooled(shortcut->name);
		}

		if(shortcut->action.command)
		{
			FreeVecTaskPooled(shortcut->action.command);
		}

		FreeVecTaskPooled(shortcut);
	}
}

ULONG keyshortcut_copy(struct key_shortcut_t * dst, struct key_shortcut_t * src)
{
	ULONG len;
	ULONG rc = TRUE;

	dst->id    = src->id;
	dst->flags = src->flags;
	dst->msgid = src->msgid;

	memcpy(&dst->sequence, &src->sequence, sizeof(src->sequence));

	if(dst->name)
	{
		FreeVecTaskPooled(dst->name);
	}

	if(src->name)
	{
		len = strlen(src->name)+1;
		dst->name = AllocVecTaskPooled(len);

		if(dst->name)
		{
			strcpy(dst->name, src->name);
		}
		else
		{
			rc = FALSE;
		}
	}

	if(dst->action.command)
	{
		FreeVecTaskPooled(dst->action.command);
	}

	if(src->action.command)
	{

		len = strlen(src->action.command)+1;
		dst->action.command = AllocVecTaskPooled(len);

		if(dst->action.command)
		{
			strcpy(dst->action.command, src->action.command);
		}
		else
		{
			rc = FALSE;
		}
	}

	dst->action.type  = src->action.type;
	dst->action.flags = src->action.flags;
	dst->action.selectionmask = src->action.selectionmask;

	if(rc == FALSE)
	{
		if(dst->name)
		{
			FreeVecTaskPooled(dst->name);
			dst->name = NULL;
		}

		if(dst->action.command)
		{
			FreeVecTaskPooled(dst->action.command);
			dst->action.command = NULL;
		}

	}

	return (rc);
}

static struct key_shortcut_node * keyshortcutnode_build(const struct key_shortcut_t * shortcut)
{
	ULONG len;
	ULONG rc = FALSE;
	struct key_shortcut_node * n;

	n = (struct key_shortcut_node *) AllocVecTaskPooled(sizeof(*n));

	if(n)
	{
		n->shortcut.id    = shortcut->id;
		n->shortcut.flags = shortcut->flags;
		n->shortcut.msgid = shortcut->msgid;

		memcpy(&n->shortcut.sequence, &shortcut->sequence, sizeof(shortcut->sequence));

		len = strlen(shortcut->name)+1;
		n->shortcut.name = AllocVecTaskPooled(len);

		if(n->shortcut.name)
		{
			strcpy(n->shortcut.name, shortcut->name);

			len = strlen(shortcut->action.command)+1;
			n->shortcut.action.command = AllocVecTaskPooled(len);

			if(n->shortcut.action.command)
			{
				strcpy(n->shortcut.action.command, shortcut->action.command);
				n->shortcut.action.type  = shortcut->action.type;
				n->shortcut.action.flags = shortcut->action.flags;
				n->shortcut.action.selectionmask = shortcut->action.selectionmask;

				rc = TRUE;
			}		 
		}
	}

	if(rc == FALSE)
	{
		keyshortcutnode_delete(n);
		n = NULL;
	}

	return n;
}

static void keyshortcutnode_delete(struct key_shortcut_node * n)
{
	if(n)
	{
		if(n->shortcut.name)
		{
			FreeVecTaskPooled(n->shortcut.name);
		}

		if(n->shortcut.action.command)
		{
			FreeVecTaskPooled(n->shortcut.action.command);
		}

		FreeVecTaskPooled(n);
	}
}

ULONG keyshortcuts_init(void)
{
	InitSemaphore(&kscsem);
	NEWLIST(&shortcut_list);

	return (TRUE);
}

void keyshortcuts_cleanup()
{
	keyshortcuts_clear();
}

void keyshortcuts_clear(void)
{
	struct key_shortcut_node * n, * nextn;

	ObtainSemaphore(&kscsem);

	ITERATELISTSAFE(n, nextn, &shortcut_list)
	{
		keyshortcutnode_delete(n);
	}
	NEWLIST(&shortcut_list);

	ReleaseSemaphore(&kscsem);
}

void keyshortcuts_reload(void)
{
	const struct key_shortcut_t * ptr;
	struct key_shortcut_node * n, * nextn;

	ObtainSemaphore(&kscsem);

	/* delete everything */
	ITERATELISTSAFE(n, nextn, &shortcut_list)
	{
		keyshortcutnode_delete(n);
	}

	NEWLIST(&shortcut_list);

	/* load default shortcuts */
	for(ptr = default_shortcuts; ptr->name; ptr++)
	{
		n = keyshortcutnode_build(ptr);

		if(n)
		{
			ADDTAIL(&shortcut_list, n);
		}
	}

	ReleaseSemaphore(&kscsem);

	/* update default shortcuts with their definition, and add custom ones */
	keyshortcuts_load();
}

struct key_shortcut_node * keyshortcuts_findbyname(CONST_STRPTR name)
{
	struct key_shortcut_node * n;

	ITERATELIST(n, &shortcut_list)
	{
		if(!stricmp(n->shortcut.name, name))
		{
			return n;
		}
	}

	return NULL;
}

struct key_shortcut_node * keyshortcuts_findbyid(ULONG id)
{
	struct key_shortcut_node * n;

	ITERATELIST(n, &shortcut_list)
	{
		if(n->shortcut.id == id)
		{
			return n;
		}
	}

	return NULL;
}

struct key_shortcut_node * keyshortcuts_findbydefinition(CONST_STRPTR definition)
{
	char buffer[128];
	struct key_shortcut_node * n;

	ITERATELIST(n, &shortcut_list)
	{
		keyshortcut_sequence_to_string(&n->shortcut.sequence, buffer, 128);
		if(!stricmp(definition, buffer))
		{
			return n;
		}
	}

	return NULL;
}

struct key_shortcut_node * keyshortcuts_next(struct key_shortcut_node * n)
{
	ObtainSemaphore(&kscsem);

	if(n)
	{
		if(n != LASTNODE(&shortcut_list))
		{
			n = NEXTNODE(n);
		}
		else
		{
			n = NULL;
		}
	}
	else
	{
		n = FIRSTNODE(&shortcut_list);
	}

	ReleaseSemaphore(&kscsem);

	return (n);
}


/* interface to add/remove shortcuts */
ULONG keyshortcuts_add(STRPTR name, ULONG msgid, ULONG id, STRPTR definition, ULONG flags,
					   ULONG cmdtype, STRPTR cmdstring, ULONG cmdflags)
{
	ULONG rc = FALSE;
	struct key_shortcut_t shortcut;
	struct key_shortcut_node * n;

	//kprintf("Adding name : <%s> id : %d definition : <%s>\n", name, id, definition);

	shortcut.id    = id;
	shortcut.name  = name;
	shortcut.msgid = msgid;
	keyshortcut_string_to_sequence(definition, &shortcut.sequence);
	shortcut.flags = flags;
	shortcut.action.type    = cmdtype;
	shortcut.action.command = cmdstring;
	shortcut.action.flags   = cmdflags;
	shortcut.action.selectionmask = 0;

	n = keyshortcutnode_build(&shortcut);

	ObtainSemaphore(&kscsem);

	if(n)
	{
		/* if it's a builtin action, we just replace sequence */
		if(shortcut.id != SHORTCUT_ID_NONE)
		{
			struct key_shortcut_node * n2;

			n2 = keyshortcuts_findbyid(shortcut.id);

			if(n2)
			{
				//kprintf("Found id %d, replacing builtin sequence\n", shortcut.id);
				memcpy(&n2->shortcut.sequence, &n->shortcut.sequence, sizeof(n->shortcut.sequence));
				rc = TRUE;
			}
			else
			{
				//kprintf("Adding builtin to list\n");
				ADDTAIL(&shortcut_list, n);
				rc = TRUE;			  
			}

		}
		else
		{
			//kprintf("Adding custom to list\n");
			ADDTAIL(&shortcut_list, n);
			rc = TRUE;
		}
	}

	ReleaseSemaphore(&kscsem);

	return (rc);
}

ULONG keyshortcuts_remove(STRPTR name)
{
	ULONG rc = FALSE;
	struct key_shortcut_node * n;

	ObtainSemaphore(&kscsem);

	n = keyshortcuts_findbyname(name);

	if(n)
	{
		REMOVE(n);
		keyshortcutnode_delete(n);
		rc = TRUE;
	}

	ReleaseSemaphore(&kscsem);

	return (rc);
}

static ULONG keyshortcuts_load(void)
{
	ULONG rc = FALSE;
	ULONG i = 0;
	APTR pl, pi; /* list, item */

	STRPTR  name;
	ULONG * id;
	ULONG * msgid;
	STRPTR  definition;
	ULONG * flags;
	STRPTR  cmdstring;
	ULONG * cmdtype;
	ULONG * cmdflags;

	ObtainSemaphore(&kscsem);

	if ( (pl = prefspool_item_get(mainprefspool, NULL, DSI_LISTPOOL_KEYSHORTCUT, NULL, NULL)) )
	{
		while ((pi = prefspool_item_get(mainprefspool, pl, i | DSF_LISTPOOL, NULL, NULL)) && prefspool_item_get(mainprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_NAME, (APTR)&name, NULL))
		{
			if (name)
			{
				rc = TRUE;

				if (!prefspool_item_get(mainprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_ID, (APTR)&id, NULL))
				{
					rc = FALSE;
				}

				if (!prefspool_item_get(mainprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_MSGID, (APTR)&msgid, NULL))
				{
					rc = FALSE;
				}

				if (!prefspool_item_get(mainprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_DEFINITION, (APTR)&definition, NULL))
				{
					rc = FALSE;
				}

				if (!prefspool_item_get(mainprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_FLAGS, (APTR)&flags, NULL))
				{
					rc = FALSE;
				}

				if (!prefspool_item_get(mainprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_CMDSTRING, (APTR)&cmdstring, NULL))
				{
					rc = FALSE;
				}

				if (!prefspool_item_get(mainprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_CMDTYPE, (APTR)&cmdtype, NULL))
				{
					rc = FALSE;
				}

				if (!prefspool_item_get(mainprefspool, pi, DSI_LISTPOOL_KEYSHORTCUT_CMDFLAGS, (APTR)&cmdflags, NULL))
				{
					rc = FALSE;
				}

				if(rc)
				{
					keyshortcuts_add(name, *msgid, *id, definition, *flags, *cmdtype, cmdstring, *cmdflags);
				}
			}
			i++;
		}
	}

	ReleaseSemaphore(&kscsem);

	return (rc);
}

static input_code keyshortcut_is_key_pressed(input_code key, ULONG *qualifier, ULONG *code)
{
ULONG qualifiermask;

	switch(key)
	{
		/* as some keyboard don't have rcommand, better merge lcommand and rcommand at input, i guess */
		case RAWKEY_LAMIGA:
			qualifiermask = IEQUALIFIER_LCOMMAND;
			break;
		case RAWKEY_RAMIGA:
			qualifiermask = IEQUALIFIER_RCOMMAND;
			break;
		case RAWKEY_LSHIFT:
			qualifiermask = IEQUALIFIER_LSHIFT;
			break;
		case RAWKEY_RSHIFT:
			qualifiermask = IEQUALIFIER_RSHIFT;
			break;
		case RAWKEY_CONTROL:
			qualifiermask = IEQUALIFIER_CONTROL;
			break;
		case RAWKEY_LALT:
			qualifiermask = IEQUALIFIER_LALT;
			break;
		case RAWKEY_RALT:
			qualifiermask = IEQUALIFIER_RALT;
			break;
		case RAWKEY_CAPSLOCK:
			qualifiermask = IEQUALIFIER_CAPSLOCK;
			break;
		default:
			return( key == *code );
	}
	if( *qualifier & qualifiermask ) {
		*qualifier &= ~qualifiermask; /* remove qualifier */
		return( 1 );
	}
	return( 0 );
}

static ULONG keyshortcut_is_sequence_pressed(seq_t * sequence, struct IntuiMessage *imsg)
{
#define IEQUALIFIER_MASK (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT|IEQUALIFIER_CAPSLOCK|\
							IEQUALIFIER_CONTROL|IEQUALIFIER_LALT|IEQUALIFIER_RALT|\
							IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND)

	ULONG i;
	ULONG pressed = FALSE;
	ULONG qualifier = imsg->Qualifier & IEQUALIFIER_MASK;
	ULONG code      = imsg->Code;

	for(i=0; i < SEQ_MAX && sequence->code[i] != CODE_NONE; i++)
	{
		if(!keyshortcut_is_key_pressed(sequence->code[i], &qualifier, &code))
		{
			break;
		}
	}

	if( ( sequence->code[i] == CODE_NONE ) && !qualifier )
	{
		pressed = i;
	}

	return pressed;
}

static CONST_STRPTR code_to_token(input_code code, STRPTR token, ULONG len)
{
	int i;

	/* check special codes first */
	for(i=0; keylist[i].name; i++)
	{
		if(keylist[i].code == code)
		{
			stccpy(token, keylist[i].name, len);
			return token;
		}
	}

	/* fall back */
	if(keylist[i].name == NULL)
	{
		struct InputEvent ie;
		TEXT c;

		ie.ie_Class        = IECLASS_RAWKEY;
		ie.ie_SubClass     = 0;
		ie.ie_Code         = code;
		ie.ie_Qualifier    = 0;
		ie.ie_EventAddress = NULL;

		if (MapRawKey(&ie, &c, 1, NULL) == 1 && len > 1)
		{
			token[0] = c;
			token[1] = 0;

			return token;
		}
	}

	return NULL;
}

#define MAXPAIRS 3

static input_code token_to_code(STRPTR token)
{
	int i;
	LONG actual;
	TEXT buffer[MAXPAIRS*2];

	/* check special token first */
	for(i=0; keylist[i].name; i++)
	{
		if(!stricmp(keylist[i].name, token))
		{
			return keylist[i].code;
		}
	}

	actual = MapANSI(token, strlen(token), buffer, MAXPAIRS, 0);

	if(actual == 1)
	{
		return *buffer;
	}

	return CODE_NONE;
}

ULONG keyshortcut_sequence_to_string(seq_t * seq, STRPTR string, ULONG maxlen)
{
	int seqnum;

	*string = 0;

	for (seqnum = 0; seqnum < SEQ_MAX && seq->code[seqnum] != CODE_NONE; seqnum++)
	{
		TEXT token[MAX_TOKEN_LEN + 1];


		if(code_to_token(seq->code[seqnum], token, sizeof(token)))
		{
			if (strlen(string) + strlen(token) + (seqnum != 0) < maxlen)
			{
				if (seqnum != 0)
					strcat(string, " ");
				strcat(string, token);
			}
		}
	}
	return 0;
}

ULONG keyshortcut_string_to_sequence(STRPTR string, seq_t * seq)
{
	char token[MAX_TOKEN_LEN + 1];
	char prefix[MAX_TOKEN_LEN + 1];
	int tokenpos, seqnum = 0;

	prefix[0] = 0;
	seq_set_0(seq);

	while (1)
	{
		while (*string != 0 && isspace(*string))
			string++;

		if (*string == 0)
			break;

		tokenpos = 0;
		while (*string != 0 && !isspace(*string) && tokenpos < MAX_TOKEN_LEN)
			token[tokenpos++] = tolower(*string++);
		token[tokenpos] = 0;


		if(!stricmp(token, "numpad"))
		{
			strcpy(prefix, "numpad ");
		}
		else
		{
			strcat(prefix, token);

			seq->code[seqnum++] = token_to_code(prefix);
			prefix[0] = 0;
		}
	}
	return seqnum;
}

static action_t * keyshortcut_findaction(struct IntuiMessage *imsg)
{
	struct key_shortcut_node * n, *ret = NULL;
	ULONG maxlen = 0;
	ULONG len = 0;

	ObtainSemaphore(&kscsem);

	ITERATELIST(n, &shortcut_list)
	{
		if(n->shortcut.flags & SHORTCUT_FLAG_ENABLED)
		{
			if( (len = keyshortcut_is_sequence_pressed(&n->shortcut.sequence, imsg)) )
			{
				if(seq_len(&n->shortcut.sequence) > maxlen)
				{
					maxlen = len;
					ret = n;
				}
			}
		}
	}

	ReleaseSemaphore(&kscsem);

	if(ret)
	{
		return &ret->shortcut.action;
	}
	else
	{
		return NULL;
	}
}

static ULONG keyshortcut_executeaction(APTR obj, action_t * n)
{
	LONG windowtype;
	APTR wo = _win( obj );

	windowtype = getv( wo, MA_Window_Type );

	if ((windowtype == MV_Window_Type_View || windowtype == MV_Window_Type_Rootview) && _view(obj))
	{
		/* get view object */
		APTR vo = _view( obj );
		APTR action;

		if (n->selectionmask != FVS_NONE && n->selectionmask != FVS_ALL)
		{
			ULONG mask = getv(vo, MA_View_SelectionMask);

			if (((mask & n->selectionmask) == 0) || (mask & ~n->selectionmask))
				return 0;
		}

		action = actionnode_create();

		if ( action )
		{
			APTR action_copy;

			/* create action */

			actionnode_addcommand( action, n->type, n->command );

			actionnode_setup( action );

			actionnode_setattrs(action,
							ACTIONNODETAG_FLAGS, ((ULONG) actionnode_getattr(action, ACTIONNODETAG_FLAGS )) |  n->flags,
							TAG_DONE);

			action_copy = actionnode_duplicate( action );

			if (action_copy)
			{
				APTR dispatcher = (APTR)DoMethod( app, MM_Application_CreateActionDispatcher );

				if ( dispatcher )
				{
					struct MinList ml;
					ULONG needfiles = (ULONG)actionnode_getattr( action_copy, ACTIONNODETAG_FLAGS ) & ACTION_FLAG_NEED_ENTRIES;

					NEWLIST(&ml);

					if ( needfiles )
					{
						DoMethod(vo, MM_View_GetSelectionList, &ml);
					}

					SetAttrs(dispatcher,
						MA_ActionDispatcher_SrcURI, (APTR)getv(vo, MA_View_Path),
						MA_ActionDispatcher_SrcID, (APTR)getv(vo, MA_Viewgroup_ID),
						MA_ActionDispatcher_RefWin, _win(obj),
						MA_ActionDispatcher_Action, action_copy,
					TAG_DONE);

					if ( !ISLISTEMPTY( &ml ) )
					{
						struct dragdropnode *ddn, *nextddn;

						ITERATELISTSAFE(ddn, nextddn, &ml)
						{
							DoMethod( dispatcher, MM_ActionDispatcher_AddURI, ddn->path, TRUE );
							/* XXX: careful here, if you break out you must still free the nodes */
							free(ddn);
						}
					}
					else if ( !needfiles )
					{
						/*
						 * No files needed, but we need at least one, so give it dummy one.
						*/
						DoMethod( dispatcher, MM_ActionDispatcher_AddURI, "dummy", TRUE );
					}

					DoMethod( dispatcher, MM_ActionDispatcher_Execute );
				}
				else
				{
					actionnode_delete( action_copy );
				}
			}

			actionnode_delete( action );
		}
	}

	return 0;
}

ULONG keyshortcut_handle(APTR obj, struct IntuiMessage * imsg)
{
	action_t * n = keyshortcut_findaction(imsg);

	if(n)
	{
		keyshortcut_executeaction(obj, n);
	}

	return (n != NULL);
}

