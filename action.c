/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2006 Michal Wozniak
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
 * $Id: action.c,v 1.8 2025/07/24 01:26:38 geit Exp $
 */

#include "ambient.h"

/* public */

#include <exec/lists.h>
#include <libraries/gadtools.h>

/* private */

#include "mimetype.h"
#include "name.h"
#include "listsort.h"
#include "mui_func.h"
#include "strings.h"
#include "action.h"
#include "command.h"
#include "rexx.h"
#include "mimeuri.h"
#include "threads.h"
#include "methodstack.h"
#include "tags.h"
#include "contextmenu.h"

struct commandlist
{
	struct MinList list;
};

struct commandnode
{
	struct MinNode n;

	ULONG type;	  				/* ACTION_COMMAND_TYPE_#? */
	STRPTR command;
};


struct actionnode
{
	struct MinNode n;
	ULONG event;				/* ACTION_EVENT_#? */
	ULONG qualifier; 			/* ACTION_QUALIFIER_#? */
	ULONG flags;				/* ACTION_FLAG_#? */
	STRPTR name;
	STRPTR menuname;
	APTR command_list;
	ULONG temporary;
};

APTR commandlist_create( void )
{
	struct commandlist *cl = malloc( sizeof( *cl ) );

	if ( cl )
	{
		NEWLIST( &cl->list );
	}

	return cl;
}

APTR actionnode_create( void )
{
	struct actionnode *an = malloc( sizeof( *an ) );

	if ( an )
	{
		if ((an->command_list = commandlist_create()))
		{
			an->qualifier = ACTION_QUALIFIER_NONE;
			an->event = ACTION_EVENT_MENU;
			an->name = NULL;
			an->menuname = NULL;
			an->flags = ACTION_FLAG_NONE;
			an->temporary = FALSE;
		}
		else
		{
			free( an );
			an = NULL;
		}
	}

	return an;
}

APTR actionnode_createtemporary( void )
{
	struct actionnode *an = actionnode_create();

	if ( an )
	{
		an->temporary = TRUE;
	}

	return an;
}

APTR actionnode_duplicate( APTR sourcean )
{
	struct actionnode *an = actionnode_create();

	if ( an )
	{
		struct actionnode *san = sourcean;
		struct commandnode *cn;

		if ( san->name )
		{
			an->name = name_build( san->name );
			if ( an->name == NULL )
			{
				actionnode_delete( an );
				return NULL;
			}
		}

		if ( san->menuname )
		{
			an->menuname = name_build( san->menuname );
			if ( an->menuname == NULL )
			{
				actionnode_delete( an );
				return NULL;
			}
		}

		an->qualifier = san->qualifier;
		an->event = san->event;
		an->flags = san->flags;
		an->temporary = san->temporary;

		ITERATELIST( cn, san->command_list )
		{
			if ( commandlist_addcommand( an->command_list, cn->type, cn->command ) == FALSE )
			{
				actionnode_delete( an );
				return NULL;
			}
		}
	}

	return an;
}


void commandlist_clear( APTR commandlist )
{
	struct commandlist *cl = commandlist;
	struct commandnode *cn, *nextcn;

	ASSERT( cl );

	ITERATELISTSAFE(cn, nextcn, &cl->list)
	{
		if ( cn->command )
			free( cn->command );

		free( cn );
	}
	NEWLIST(&cl->list);
}

void commandlist_delete( APTR commandlist )
{
	ASSERT( commandlist );

	commandlist_clear( commandlist );
	free( commandlist );
}

void actionnode_delete( APTR actionnode )
{
	struct actionnode *an = actionnode;

	ASSERT( an );

	if ( an )
	{
		commandlist_delete( an->command_list );
		
		if ( an->name )
			name_delete( an->name );

		if ( an->menuname )
			name_delete( an->menuname );

		free( an );
	}
}

void v_actionnode_setattrs(APTR actionnode, struct TagItem *tags)
{
	struct actionnode *an = actionnode;

	ASSERT( an );

	FORTAG(tags)
	{
		case ACTIONNODETAG_QUALIFIER:
			an->qualifier = tag->ti_Data;
			break;

		case ACTIONNODETAG_NAME:
			an->name = name_replace( an->name, (STRPTR)tag->ti_Data );
			break;

		case ACTIONNODETAG_EVENT:
			an->event = tag->ti_Data;
			D(MIMETYPE,bug("Event:%d\n",an->event));
			break;

		case ACTIONNODETAG_FLAGS:
			an->flags = tag->ti_Data;
			break;

		case ACTIONNODETAG_MENU_NAME:
			if(tag->ti_Data == 0)
			{
				if(an->menuname)
				{
					name_delete(an->menuname);
				}
				an->menuname = NULL;
			}
			else
			{
				an->menuname = name_replace( an->menuname, (STRPTR)tag->ti_Data );
			}
			break;

		case ACTIONNODETAG_TEMPORARY:
			an->temporary = tag->ti_Data;
			break;

		default:
			PDB(("unknown tag 0x%lx\n", tag->ti_Data));
			break;

	}
	NEXTTAG
}

ULONG commandlist_addcommand( APTR commandlist, ULONG type, STRPTR command )
{
	struct commandlist *cl = commandlist;
	struct commandnode *cn;

	ASSERT( cl );

	cn = malloc(sizeof( *cn ));

	if (cn)
	{
		cn->command =  malloc(strlen(command) + 1);

		if (cn->command)
		{
			D(MIMETYPE,bug("commandlist: add command <%s>\n", command ));
			cn->type = type;
			strcpy( cn->command, command );
			ADDTAIL( &cl->list, cn );

			return TRUE;
		}

		free(cn);
	}

	errormsg(ERR_NOMEM);

	return FALSE;
}

ULONG actionnode_addcommand( APTR actionnode, ULONG type, STRPTR command )
{
	struct actionnode *an = actionnode;

	return commandlist_addcommand( an->command_list, type, command );

}

void actionnode_remcommand(APTR commandnode)
{
	struct commandnode *cn = commandnode;

	REMOVE(cn);

	/* This one is touched by setattr, so we need to check it. */
	if (cn->command)
		free(cn->command);

	free(cn);
}

APTR actionnode_getattr(APTR actionnode, ULONG attr)
{
	struct actionnode *an = actionnode;

	ASSERT( an );

	switch (attr)
	{
		case ACTIONNODETAG_NAME:
			return an->name;

		case ACTIONNODETAG_EVENT:
			return (APTR)an->event;

		case ACTIONNODETAG_QUALIFIER:
			return (APTR)an->qualifier;

		case ACTIONNODETAG_FLAGS:
			return (APTR)an->flags;

		case ACTIONNODETAG_MENU_NAME:
			return an->menuname;

		case ACTIONNODETAG_COMMAND_LIST:
			return an->command_list;

		case ACTIONNODETAG_TEMPORARY:
			return (APTR)an->temporary;

		#ifdef DEBUG
		default:
			PDB(("unknown tag 0x%lx\n", attr));
			break;
		#endif
	}

	return (NULL);

}

void v_commandnode_setattrs(APTR commandnode, struct TagItem *tags)
{
	struct commandnode *cn = commandnode;

	ASSERT(cn);

	FORTAG(tags)
	{
		case COMMANDNODETAG_TYPE:
			cn->type = tag->ti_Data;
			break;

		case COMMANDNODETAG_COMMAND:
			free(cn->command);

			if ((cn->command = malloc(strlen((STRPTR) tag->ti_Data) + 1)))
				strcpy(cn->command, (STRPTR) tag->ti_Data);
			break;

		default:
			PDB(("unknown tag 0x%lx\n", tag->ti_Data));
			break;

	}
	NEXTTAG
}

APTR commandnode_getattr(APTR commandnode, ULONG attr)
{
	struct commandnode *cn = commandnode;

	ASSERT( cn );

	switch (attr)
	{
		case COMMANDNODETAG_TYPE:
			return (APTR)cn->type;

		case COMMANDNODETAG_COMMAND:
			return (APTR)cn->command;

		#ifdef DEBUG
		default:
			PDB(("unknown tag 0x%lx\n", attr));
			break;
		#endif
	}

	return (NULL);

}

/*
 * Function which checks action string for certain placeholders.
 * - destinations ones
 * - requester ones (user input)
 *
 * It also sets up default fields if they were not set by initialization routines.
 */

void actionnode_setup( APTR actionnode )
{
	struct actionnode *an = actionnode;


	D(MIMETYPE,bug("Setup action <%s> <%d>\n", an->name, an->event ));

	if ( an && an->command_list && !ISLISTEMPTY( an->command_list ) )
	{
		ULONG flags = 0;

		struct commandnode *cn;

		ITERATELIST( cn, an->command_list )
		{
			LONG i = 0;
			LONG len = strlen( cn->command );
			STRPTR command = cn->command;

			/*
			 * -1 to make our life easier.
			 * Won't need to handle end of string case where no placeholder can be anyway.
			 */

			for(i=0; i<len - 1; i++)
			{
				if ( command[ i ] == '%' )
				{
					switch( command[ i + 1 ] )
					{
						case 'd':
						case 'D':
							flags |= ACTION_FLAG_NEED_DEST;
							break;

						case 's':

							flags |= ACTION_FLAG_NEED_ENTRIES;

							if ( i < len - 2 )
							{
								switch ( command[ i + 2 ] )
								{
									case 'p': /* path */
										flags |= ACTION_FLAG_NEED_PATH;
										break;
									case 'f': /* filepart */
										flags |= ACTION_FLAG_NEED_FILEPART;
										break;
									default:
										flags |= ACTION_FLAG_NEED_URI;
										break;
								}
							}
							else
							{
								/* %s only */

								flags |= ACTION_FLAG_NEED_URI;
							}

							break;
					}
				}
			}
		}

		an->flags |= flags;

	}

	if ( !an->name )
	{
		an->name = name_build("Default");
	}

}

LONG commandlist_execute( APTR obj, APTR command_list )
{
	if ( command_list && !ISLISTEMPTY( command_list ) )
	{
		APTR cn;

		ITERATELIST( cn, command_list )
		{
			ULONG type = (ULONG)commandnode_getattr( cn, COMMANDNODETAG_TYPE );
			STRPTR command = commandnode_getattr( cn, COMMANDNODETAG_COMMAND );

			DB(("Parsed command:%s\n",command));
			execute_command(obj, type | AC_SYNCHRONOUS , command, NULL);
			DB(("Command finished...\n"));
		}
	}

	return 0;
}

APTR actionlist_getbyevent( APTR actionlist, ULONG event )
{

	if ( !ISLISTEMPTY( actionlist ) )
	{
		struct actionnode *an;

		ITERATELIST( an, actionlist )
		{
			if ( an->event == event )
				return an;
		}
	}

	return NULL;
}
