
#ifndef AMBIENT_ACTION_H
#define AMBIENT_ACTION_H

/*
 * ActionNode is based on Node struct.
 * CommandList is based on List struct;
 */

/*
 * Attributes describing action which is a member of mimetype descriptor.
 * They are loadingtime attributes used with action_xxx functions.
 */

enum {
	ACTIONNODETAG_NAME = TAG_USER + 1,/* [SG] name of an action */
	ACTIONNODETAG_FLAGS,              /* [SG] flags for action (ACTION_FLAG_xxx) */
	ACTIONNODETAG_EVENT,              /* [SG] event on which action should be triggered (ACTION_EVENT_xxx) */
	ACTIONNODETAG_QUALIFIER,          /* [SG] optional qualifier */
	ACTIONNODETAG_MENU_NAME,          /* [SG] associated parent menu name, optional */
	ACTIONNODETAG_COMMAND_LIST,       /* [.G] command list containing commands to execute (before parsing) */
	ACTIONNODETAG_TEMPORARY,          /* [SG] action will be disposed after it's passed to actiondispatcher */
};

/*
 * Attributes for command node.
 */

enum {
	COMMANDNODETAG_TYPE = TAG_USER + 1, /* [SG] type of command */
	COMMANDNODETAG_COMMAND,             /* [SG] command template */
};


APTR actionnode_create( void );
APTR actionnode_createtemporary( void );
void actionnode_delete( APTR actionnode );
APTR actionnode_duplicate( APTR sourcean );

void v_actionnode_setattrs( APTR actionnode, struct TagItem *tags);
void actionnode_setattrs( APTR actionnode, ...);
APTR actionnode_getattr(APTR actionnode, ULONG attr);

ULONG actionnode_addcommand( APTR actionnode, ULONG type, STRPTR command );
void actionnode_remcommand(APTR commandnode);
void actionnode_setup( APTR actionnode );
APTR actionlist_getbyevent( APTR actionlist, ULONG event );

/*
 * commandlist is assigned to action.
 */

APTR commandlist_create( void );
void commandlist_delete( APTR commandlist );
void commandlist_clear( APTR commandlist );
ULONG commandlist_addcommand( APTR commandlist, ULONG type, STRPTR command );
LONG commandlist_execute( APTR obj, APTR commandlist );

void v_commandnode_setattrs(APTR commandnode, struct TagItem *tags);
void commandnode_setattrs(APTR commandnode, ...);
APTR commandnode_getattr(APTR commandnode, ULONG attr);

#endif
