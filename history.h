
#ifndef AMBIENT_HISTORY_H
#define AMBIENT_HISTORY_H

/*
 * Attributes for history.
 */
enum {
	HISTORYTAG_POSITION = TAG_USER + 1,/* [SG] position on history list */
	HISTORYTAG_SIZE,                   /* [.G] size of history list */
	HISTORYTAG_LOCKED,                 /* [SG] state of history lock */
	HISTORYTAG_NODE,                   /* [.G] node at current history position */
	HISTORYTAG_VIEWOBJ                 /* [.G] view object we can query about cirrently displayed contents */
};

/*
 * Attributes for history node.
 */
enum {
	HISTORYNODETAG_PATH = TAG_USER + 1,/* [SG] path to resource */
	HISTORYNODETAG_MODENAME,           /* [SG] viewmode index name associated with resource */
	HISTORYNODETAG_URI                 /* [.G] complete URI constructed from path and mode */
};


struct historynode
{
	struct Node node;
	STRPTR path;                       /* path to resource */
	STRPTR mode;                       /* mode to use when creating URI */
};

struct history
{
	struct MinList list;               /* list containing all history nodes */
	ULONG position;                    /* currently displayed history node */
	ULONG size;                        /* number of history nodes */
	ULONG lock;                        /* if set then history node can't be updated */
	APTR viewobj;                      /* viewobject history is associated to */
};

APTR history_create( APTR viewobj );
APTR historynode_create( CONST_STRPTR path, CONST_STRPTR mode );

void history_delete( APTR history );
void historynode_delete( APTR history );

void v_history_setattrs( APTR history, struct TagItem *tags );
ULONG v_historynode_setattrs( APTR node, struct TagItem *tags );
void history_setattrs( APTR history, ... );
ULONG historynode_setattrs( APTR node, ... );

APTR history_getattr(APTR history, ULONG attr);
APTR historynode_getattr(APTR historynode, ULONG attr);

APTR history_forward(APTR history );
APTR history_backward(APTR history );
void history_addnode(APTR history, APTR historynode);

void history_dump( APTR history );


#endif /* AMBIENT_HISTORY_H */
