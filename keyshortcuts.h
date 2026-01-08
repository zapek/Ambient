/*
 * $Id: keyshortcuts.h,v 1.6 2016/08/01 18:51:29 itix Exp $
 */

#ifndef KEYSHORTCUTS_H
#define KEYSHORTCUTS_H

struct IntuiMessage;

/* shortcut definition */

typedef ULONG input_code;

#define SEQ_MAX 16

typedef struct _seq_t
{
	input_code code[SEQ_MAX];
} seq_t;

typedef struct _action_t
{
	ULONG  type;
	STRPTR command;
	ULONG  flags;
	ULONG  selectionmask;    /* See viewclass.h FVS_#? for mask, 0 if not applicable */
} action_t;

struct key_shortcut_t
{
	ULONG    id;             /* identifier for this shortcut */
	STRPTR   name;           /* shortcut label */
	ULONG    msgid;          /* localized name, used first if != 0 */
	seq_t    sequence;       /* shortcut array (input_code array) */
	action_t action;         /* action */
	ULONG    flags;          /* shortcut flags */
};

struct key_shortcut_node
{
	struct MinNode n;
	struct key_shortcut_t shortcut;
};

/* shortcuts flags */
enum
{
	SHORTCUT_FLAG_BUILTIN  = 0x1, /* if set, user can only edit the sequence */
	SHORTCUT_FLAG_ENABLED  = 0x2, /* must be set to enable shortcut */
	SHORTCUT_FLAG_READONLY = 0x4  /* if set, shortcut is not user-configurable at all */
};

/* shortcuts identifiers (for builtin hotkeys) */
enum
{
	SHORTCUT_ID_NONE = 0,

	/* these id could be used to find a shortcut for particular builtin actions */
	SHORTCUT_ID_DELETE,
	SHORTCUT_ID_RENAME,
	SHORTCUT_ID_INFORMATION,

	SHORTCUT_ID_SELECTALL,
	SHORTCUT_ID_SELECTNONE,
	SHORTCUT_ID_SELECTINVERT,
	SHORTCUT_ID_SELECTPATTERN,

	SHORTCUT_ID_CLIPBOARDCOPY,
	SHORTCUT_ID_CLIPBOARDCUT,
	SHORTCUT_ID_CLIPBOARDPASTE,

	SHORTCUT_ID_CYCLEMODE,
	SHORTCUT_ID_CYCLESUBMODE,

	SHORTCUT_ID_FIND,

	SHORTCUT_ID_CLIPBOARDPASTEAS,
	SHORTCUT_ID_PARENT,
	SHORTCUT_ID_LEAVEOUT,
	SHORTCUT_ID_PUTAWAY,
	SHORTCUT_ID_RELOAD
};

ULONG keyshortcuts_init(void);
void  keyshortcuts_cleanup(void);

void keyshortcuts_reload(void);
void keyshortcuts_clear(void);

struct key_shortcut_t *	keyshortcut_create(STRPTR name);
void keyshortcut_delete(struct key_shortcut_t * shortcut);
ULONG keyshortcut_copy(struct key_shortcut_t * dst, struct key_shortcut_t * src);
ULONG keyshortcut_sequence_to_string(seq_t * seq, STRPTR string, ULONG maxlen);
ULONG keyshortcut_string_to_sequence(STRPTR string, seq_t * seq);

/* traverse list */
struct key_shortcut_node * keyshortcuts_next(struct key_shortcut_node * n);
struct key_shortcut_node * keyshortcuts_findbyname(CONST_STRPTR name);
struct key_shortcut_node * keyshortcuts_findbyid(ULONG id);
struct key_shortcut_node * keyshortcuts_findbydefinition(CONST_STRPTR definition);

/* manipulate list */
ULONG keyshortcuts_add(STRPTR name, ULONG msgid, ULONG id, STRPTR definition, ULONG flags, ULONG cmdtype, STRPTR cmdstring, ULONG cmdflags);
ULONG keyshortcuts_remove(STRPTR name);

/* shortcut handler, to be used in event handlers */
ULONG keyshortcut_handle(APTR obj, struct IntuiMessage * imsg);

#endif



