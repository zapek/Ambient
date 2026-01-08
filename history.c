/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2007 Ambient Open Source Team
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
 * $Id: history.c,v 1.7 2013/10/28 20:02:39 geit Exp $
 */

#include "ambient.h"


/* public */
#include <exec/types.h>
#include <proto/dos.h>

/* private */

#include "history.h"
#include "name.h"
#include "mui_func.h"

#define MAX_NODES 10

APTR history_create( APTR viewobj )
{
	struct history *hist = malloc( sizeof( struct history ) );

	if ( hist )
	{
		NEWLIST( &hist->list );
		hist->position = 0;
		hist->size = 0;
		hist->lock = 0;
		hist->viewobj = viewobj;
	}

	return (APTR)hist;
}

APTR historynode_create( CONST_STRPTR path, CONST_STRPTR mode )
{
	struct historynode *node = malloc( sizeof( struct historynode ) );

	if ( node )
	{
		node->path = name_build( path );
		node->mode = name_build( mode );

		if ( !node->path || !node->mode )
		{
			historynode_delete( node );
			node = NULL;
		}
	}

	return (APTR)node;
}

void history_delete( APTR history )
{
	struct history *hist = history;

	if ( hist )
	{
		/*
		 * Free all history nodes.
		 */

		struct historynode *n, *nextn;

		ITERATELISTSAFE( n, nextn, &hist->list )
		{
			historynode_delete( n );
		}

		free( hist );
	}
}

void historynode_delete( APTR historynode )
{
	struct historynode *node = historynode;

	if ( node )
	{
		if ( node->path )
			name_delete( node->path );

		if ( node->mode )
			name_delete( node->mode );

		free( node );
	}
}

APTR history_getattr(APTR history, ULONG attr)
{
	struct history *hist = history;

	ASSERT( hist );

	switch (attr)
	{
		case HISTORYTAG_POSITION:
			return (APTR)hist->position;

		case HISTORYTAG_SIZE:
			return (APTR)hist->size;

		case HISTORYTAG_LOCKED:
			return (APTR)hist->lock;

		case HISTORYTAG_NODE:
			{
				if ( hist->size )
				{
					struct historynode *n;
					int i = 0;
					ITERATELIST( n, &hist->list )
					{
						if ( i == hist->position )
						{
							return (APTR)n;
						}
						i++;
					}
				}
				return NULL;
			}
		case HISTORYTAG_VIEWOBJ:
			return (APTR)hist->viewobj;

		#ifdef DEBUG
		default:
			PDB(("unknown tag 0x%lx\n", attr));
			break;
		#endif
	}
	return (NULL);
}

APTR historynode_getattr(APTR historynode, ULONG attr)
{
	struct historynode *node = historynode;

	ASSERT( node );

	switch (attr)
	{
		case HISTORYNODETAG_PATH:
			return ( node->path );
        
		case HISTORYNODETAG_MODENAME:
			return ( node->mode );

		#ifdef DEBUG
		default:
			PDB(("unknown tag 0x%lx\n", attr));
			break;
		#endif
	}
	return (NULL);
}

void v_history_setattrs(APTR history, struct TagItem *tags)
{
	struct history *hist = history;

	ASSERT( hist );

	FORTAG(tags)
	{
		case HISTORYTAG_POSITION:
			/* check if it's in bounds */

			if (( /* tag->ti_Data >= 0 && */ tag->ti_Data < hist->size))
			{
				hist->position = tag->ti_Data;
			}
			else
			{
				PDB(("history position out of bounds %ld( size: %ld )\n", tag->ti_Data, hist->position));
			}
			break;

		case HISTORYTAG_LOCKED:
			hist->lock = tag->ti_Data;
			break;

		#ifdef DEBUG
		default:
			PDB(("unknown tag 0x%lx\n", tag->ti_Data));
			break;
		#endif

    }
    NEXTTAG
}

ULONG v_historynode_setattrs(APTR historynode, struct TagItem *tags)
{
	struct historynode *node = historynode;
	ULONG changed = FALSE;

	ASSERT( node );

	FORTAG(tags)
	{
		case HISTORYNODETAG_MODENAME:

			ASSERT( tag->ti_Data );

			if ( strcmp( (STRPTR)tag->ti_Data, node->mode ) )
			{
				STRPTR newmode;

				newmode = name_build( (STRPTR)tag->ti_Data );

				if ( newmode )
				{
					name_delete( node->mode );
					node->mode = newmode;

					changed = TRUE;
				}
			}
			break;

		case HISTORYNODETAG_PATH:

			ASSERT( tag->ti_Data );

			if ( strcmp( (STRPTR)tag->ti_Data, node->path ) )
			{
				STRPTR newpath;

				newpath = name_build( (STRPTR)tag->ti_Data );

				if ( newpath )
				{
					name_delete( node->path );
					node->path = newpath;

					changed = TRUE;
				}
			}
			break;

		#ifdef DEBUG
		default:
			PDB(("unknown tag 0x%lx\n", tag->ti_Data));
			break;
		#endif

	}
	NEXTTAG

	return changed;
}

static LONG historynode_checkequal( struct historynode *n1, struct historynode *n2 )
{
	if ( n1->path != NULL && n2->path != NULL && stricmp( n1->path, n2->path ) == 0 )
	{
		/* mode might be NULL */
	
		if ( n1->mode == NULL && n2->mode == NULL )
			return TRUE;
		if ( n1->mode != NULL && n2->mode != NULL && stricmp( n1->mode, n2->mode ) == 0 )
			return TRUE;
	}

	return FALSE;
}

void history_addnode( APTR history, APTR historynode )
{ 
	struct history *hist = history;
	struct historynode *node = historynode;

	ASSERT( hist );
	ASSERT( node );

	DB(("Add node\n"));

	if ( hist->size != 0 )
	{

		/* check neighbours, if any matches then insert into it's place */

		{
			struct historynode *n = history_getattr( hist, HISTORYTAG_NODE );
			struct historynode *prevn = ( hist->position > 0 ) ? PREVNODE( n ) : NULL;
			struct historynode *nextn = ( hist->position < hist->size ) ? NEXTNODE( n ) : NULL;

			DB(("Previous:0x%x, Next:0x%x\n", prevn, nextn));

			if ( prevn != NULL && historynode_checkequal( node, prevn ) )
			{
				n = PREVNODE( prevn );
				REMOVE( prevn );
				historynode_delete( prevn );
				Insert( (struct List*)&hist->list, (struct Node*)node, (struct Node*)n );
				hist->position = max( 0, hist->position - 1 );
				return;
			}
			else if ( nextn != NULL && historynode_checkequal( node, nextn ) )
			{
				REMOVE( nextn );
				historynode_delete( nextn );
				Insert( (struct List*)&hist->list, (struct Node*)node, (struct Node*)n );
				hist->position++;
				return;
			}
		}

		/* check position, and if there are any uris after position, remove them */

		if ( hist->position < hist->size - 1 )
		{
			/* remove data->historySize - data->historyPos nodes */

			LONG i;

			DB(("Removing nodes:position:%d, size:%d...\n", hist->position, hist->size));

			for (i = hist->size - 1; hist->position < hist->size - 1; i--)
			{
				struct historynode *n = LASTNODE( &hist->list );

				/* remove this node */
				REMOVE( n );
				historynode_delete( n );
				hist->size--;
			}
		}

		hist->position++;
	}

	/* check if we reached max number of nodes. if yes, remode oldest one */

	if ( hist->size == MAX_NODES )
	{
		struct historynode *n = FIRSTNODE( &hist->list );
		REMOVE( n );
		historynode_delete( n );
		hist->size--;
		hist->position--;
	}

	/* insert as last node */

	ADDTAIL( &hist->list, node );

	hist->size++;
}

APTR history_backward( APTR history  )
{
	struct history *hist = history;

	ASSERT( hist );

	if ( hist->position == 0 )
	{
		return NULL;
	}

	/* return previous node and update position */

	{
		struct historynode *n;
		int i = 0;

		ITERATELIST( n, &hist->list )
		{
			if ( i == hist->position - 1 )
			{
				hist->position--;
				return n;
			}
			i++;
		}
	}

	/* for some weird reason... */

	return NULL;
}

APTR history_forward( APTR history )
{
	struct history *hist = history;

	ASSERT( hist );

	if ( hist->position == hist->size - 1 )
	{
		return NULL;
	}

	/* return next node and update position */

	{
		struct historynode *n;
		int i = 0;

		ITERATELIST( n, &hist->list )
		{
			if ( i == hist->position + 1 )
			{
				hist->position++;
				return n;
			}
			i++;
		}
	}

	/* for some weird reason */

	return NULL;
}

void history_dump( APTR history )
{
	struct history *hist = history;

	SDB(("HistoryDump...(%ld,%ld,%ld)\n", hist->lock, hist->position, hist->size));
	{
		struct historynode *n;
		int i = 0;

		ITERATELIST( n, &hist->list )
		{
			SDB(("Node:%s:%s(%s)\n", n->path, n->mode, i == hist->position ? "Current" : "" ));
			i++;
		}
	}
	SDB(("HistoryDump...Done\n"));
}
