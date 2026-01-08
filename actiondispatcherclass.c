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
 * $Id: actiondispatcherclass.c,v 1.17 2017/07/23 20:54:20 cyfm Exp $
 */

#include "ambient.h"

/* public */

#include <exec/lists.h>
#include <libraries/gadtools.h>
#include <proto/dos.h>

/* private */

#include "mui_func.h"
#include "mimetype.h"
#include "name.h"
#include "listsort.h"
#include "mui_func.h"
#include "strings.h"
#include "actiondispatcherclass.h"
#include "command.h"
#include "rexx.h"
#include "mimeuri.h"
#include "threads.h"
#include "methodstack.h"
#include "tags.h"
#include "contextmenu.h"
#include "action.h"
#include "prefs_advanced.h"


struct entrynode
{
	struct MinNode n;

	APTR actionnode;		/* action executed for this uri */
	STRPTR location[ 3 ];

	LONG idx;               /* helper to workaround sorting function instability */
};

#define LOCATION_PATH 0
#define LOCATION_FILEPART 1
#define LOCATION_URI 2
#define LOCATION_MAX 3

struct Data
{
	/* settable attributes on which action will be decided */

	ULONG qualifier;		/* used to select action object */
	ULONG event;			/* ... */
	STRPTR src_uri;
	STRPTR src_uri_unquoted;
	STRPTR dst_uri;
	STRPTR dst_uri_unquoted;
	APTR refwin;
	LONG dstid;
	LONG srcid;

	/* list of files we have to process */

	struct MinList entries;

	/* we try to group entries into a group with same action */

	ULONG sorted;

	/* action supplied by user */

	APTR action;

	/* helper to workaround sorting function instability */

	LONG idx;
};

static void delete_node( struct entrynode *en );
static struct entrynode *build_node( STRPTR uri, ULONG encodeuri );


static void doset(APTR obj UNUSED, struct Data *data, struct TagItem *tags)
{
	FORTAG(tags)
	{
		case MA_ActionDispatcher_Qualifier:
			data->qualifier = tag->ti_Data;
			break;

		case MA_ActionDispatcher_IQualifier:
			D(ACTIONDISP,bug("Qualifier:0x%x\n", tag->ti_Data));

			data->qualifier = 0;

			/* Translate intuition qualifier */
			if ( tag->ti_Data & IEQUALIFIER_LALT )
				data->qualifier |= ACTION_QUALIFIER_ALT;
			if ( tag->ti_Data & IEQUALIFIER_RALT )
				data->qualifier |= ACTION_QUALIFIER_ALT;
			if ( tag->ti_Data & IEQUALIFIER_MIDBUTTON )
				data->qualifier |= ACTION_QUALIFIER_ALT;

			if ( tag->ti_Data & IEQUALIFIER_CONTROL )
				data->qualifier |= ACTION_QUALIFIER_CONTROL;

			if ( tag->ti_Data & IEQUALIFIER_LSHIFT )
				data->qualifier |= ACTION_QUALIFIER_SHIFT;
			if ( tag->ti_Data & IEQUALIFIER_RSHIFT )
				data->qualifier |= ACTION_QUALIFIER_SHIFT;

			break;

		case MA_ActionDispatcher_Event:
			data->event = tag->ti_Data;
			break;

		case MA_ActionDispatcher_RefWin:
			data->refwin = (APTR)tag->ti_Data;
			break;

		case MA_ActionDispatcher_SrcID:
			data->srcid = tag->ti_Data;
			break;

		case MA_ActionDispatcher_DstID:
			data->dstid = tag->ti_Data;
			break;

		case MA_ActionDispatcher_SrcURI:
			{
				if (tag->ti_Data)
				{
					if (!data->src_uri_unquoted || strcmp((STRPTR)tag->ti_Data, data->src_uri_unquoted))
					{
						if (data->src_uri != NULL)
							name_delete(data->src_uri);

						data->src_uri = name_build_readargs_quoted((STRPTR)tag->ti_Data, NULL, 0);
						data->src_uri_unquoted = name_replace(data->src_uri_unquoted, (STRPTR)tag->ti_Data);
					}
				}
			}
			break;

		case MA_ActionDispatcher_DstURI:
			{
				if (tag->ti_Data)
				{
					if (!data->dst_uri_unquoted || strcmp((STRPTR)tag->ti_Data, data->dst_uri_unquoted))
					{
						if (data->dst_uri != NULL)
							name_delete(data->dst_uri);

						data->dst_uri = name_build_readargs_quoted((STRPTR)tag->ti_Data, NULL, 0);
						data->dst_uri_unquoted = name_replace(data->dst_uri_unquoted, (STRPTR)tag->ti_Data);
					}
				}
			}
			break;

		case MA_ActionDispatcher_Action:
			{
				ASSERT( tag->ti_Data );

				data->action = (APTR)tag->ti_Data;
			}
			break;

		default:
			PDB(("unknown tag 0x%lx\n", tag->ti_Data));
			break;
	}
	NEXTTAG
}

DEFNEW
{
	obj = DoSuperNew(cl, obj,
		TAG_DONE
	);

	if (obj)
	{
		GETDATA;

		data->qualifier = ACTION_QUALIFIER_NONE;
		data->event = ACTION_EVENT_NONE;
		data->src_uri = NULL;
		data->src_uri_unquoted = NULL;
		data->dst_uri = NULL;
		data->dst_uri_unquoted = NULL;
		data->refwin = NULL;
		data->srcid = -1;
		data->dstid = -1;
		data->action = NULL;

		NEWLIST( &data->entries );

		data->sorted = FALSE;

		/* setup initial attrs */

		doset( obj, data, INITTAGS );

	}

	return (ULONG) obj;
}

DEFDISP
{
	GETDATA;
	struct entrynode *en, *nexten;

	if ( data->src_uri )
		name_delete( data->src_uri );
	if ( data->src_uri_unquoted )
		name_delete( data->src_uri_unquoted );

	if ( data->dst_uri )
		name_delete( data->dst_uri );
	if ( data->dst_uri_unquoted )
		name_delete( data->dst_uri_unquoted );


	ITERATELISTSAFE(en, nexten, &data->entries)
	{
		delete_node( en );
	}

	if ( data->action && actionnode_getattr( data->action, ACTIONNODETAG_TEMPORARY ) )
	{
		actionnode_delete( data->action );
	}

	return (DOSUPER);
}

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_ActionDispatcher_RefWin:
			*msg->opg_Storage = (ULONG)data->refwin;
			return (TRUE);

		case MA_View_HandleIcons:
		{
			ULONG handleicons = FALSE;
			ULONG ret = FALSE;

			if(data->refwin)
			{
				APTR viewobj;

				methodstack_push_sync( data->refwin, 3, OM_GET, MA_Window_Viewobj, &viewobj );

				if(viewobj)
				{
					APTR target = NULL;

					methodstack_push_sync( viewobj, 3, OM_GET, MA_Viewgroup_CurrentView, &target );

					if(target)
					{
						APTR target2 = NULL;

						methodstack_push_sync( target, 3, OM_GET, MA_View_SubObject, &target2 );

						if(target2)
						{
							methodstack_push_sync( target2, 3, OM_GET, MA_View_HandleIcons, &handleicons );
							ret = TRUE;
						}
						else
						{
							methodstack_push_sync( target, 3, OM_GET, MA_View_HandleIcons, &handleicons );
							ret = TRUE;
						}
					}
				}
			}

			*msg->opg_Storage = handleicons;
			return (ret);
		}
	}

	return DOSUPER;
}

DEFSET
{
	GETDATA;

	doset(obj, data, INITTAGS);
	return DOSUPER;
}

static STRPTR name_build_encoded(CONST_STRPTR name)
{
	STRPTR n;
	int len;

	ASSERT(name);

	len = mimeuri_encodepath(NULL, 0, name);

	if ( (n = malloc(len + 32)) )
	{
		TEXT buf[len];
		mimeuri_encodepath(buf, len, name);
		snprintf(n, len + 32, "file:///%s", buf); /* this scheme shouldn't be hardcoded as it can depend on view scheme. */
	}
	else
	{
		errormsg(ERR_NOMEM);
	}
	return (n);
}

/*
 * Well, nasty name but will never be used.
 * The only use is in ActionDispatcher_AddURI below.
 */

static struct entrynode *build_node(STRPTR uri, ULONG encodeuri)
{
	struct entrynode *en;

	if ((en = malloc(sizeof(*en))))
	{
		en->location[ LOCATION_URI ] = encodeuri ? name_build_encoded( uri ) : name_build( uri );
		en->location[ LOCATION_PATH ] = NULL;
		en->location[ LOCATION_FILEPART ] = NULL;
		en->actionnode = NULL;
	}

	return en;
}

static void delete_node( struct entrynode *en )
{
	if ( en )
	{
		LONG i;

		for(i=0; i<LOCATION_MAX; i++)
		{
			if ( en->location[ i ] )
				name_delete( en->location[ i ] );
		}

		free( en );
	}
}


DEFSMETHOD(ActionDispatcher_AddURI)
{
	/*
	 * If the uri to be added is direct pathname (no file:///), encode
	 * the path special characters. It it's file:/// format, the encoding
	 * has already been done by the caller. - Piru
	 */
	struct entrynode *en = build_node( msg->uri, strncmp(msg->uri, "file:/", 6) != 0);

	if ( en )
	{
		GETDATA;

		en->idx = data->idx++;				/* XXX: Workaround for sorting function instability */
		ADDTAIL( &data->entries, en );
		data->sorted = FALSE;
	}

	return en ? TRUE : FALSE;
}


static void action_lookup( struct Data *data, APTR entry, APTR actionnode )
{
	struct entrynode *en = entry;

	/*
	 * We find action for this new uri matching event and qualifier.
	 * We can be given full uri so path have to be extracted.
	 */

	{
		APTR ctx;

		D(ACTIONDISP,bug("Action lookup for <%s>\n", en->location[ LOCATION_URI ] ));

		if ((ctx = mimeuri_create()))
		{
			if ( mimeuri_gather(ctx,en->location[ LOCATION_URI ],
				MIMEURIGATHERTAG_Extension, FALSE,
				MIMEURIGATHERTAG_Protocol, FALSE,
				MIMEURIGATHERTAG_FileIO, FALSE,
				MIMEURIGATHERTAG_Network, FALSE,
				TAG_DONE
			))
			{
				if ( !actionnode )
				{
					CONST_STRPTR path = mimeuri_getattr(ctx, MIMEURIATTR_PATH);
					if (path)
					{
						actionnode = mimetype_findaction( mimeuri_getattr(ctx, MIMEURIATTR_SCHEME), path, data->event, data->qualifier, MTF_FILEIO);
					}
					else
					{
						D(ACTIONDISP,bug("URI <%s> MIMEURIATTR_PATH is NULL!\n", en->location[ LOCATION_URI ]));
					}
				}

				if ( actionnode )
				{
					STRPTR olduri = en->location[ LOCATION_URI ];
					ULONG flags = (ULONG)actionnode_getattr( actionnode, ACTIONNODETAG_FLAGS );

					/*
					 * Add uri with it's action to the list.
					 */

					if (!(flags & ACTION_FLAG_UNQUOTED)) /* This stuff should probably die later on. - Piru */
					{
						en->location[ LOCATION_URI ] = flags & ACTION_FLAG_NEED_URI ? name_build_readargs_quoted( mimeuri_getattr(ctx, MIMEURIATTR_URI), NULL, 0) : NULL;
						en->location[ LOCATION_PATH ] = flags & ACTION_FLAG_NEED_PATH ? name_build_readargs_quoted( mimeuri_getattr(ctx, MIMEURIATTR_PATH), NULL, 0 ) : NULL;
						en->location[ LOCATION_FILEPART ] = flags & ACTION_FLAG_NEED_FILEPART ? name_build_readargs_quoted( FilePart( mimeuri_getattr(ctx, MIMEURIATTR_PATH) ), NULL, 0 ) : NULL;
					}
					else
					{
						en->location[ LOCATION_URI ] = flags & ACTION_FLAG_NEED_URI ? name_build( mimeuri_getattr(ctx, MIMEURIATTR_URI) ) : NULL;
						en->location[ LOCATION_PATH ] = flags & ACTION_FLAG_NEED_PATH ? name_build( mimeuri_getattr(ctx, MIMEURIATTR_PATH) ) : NULL;
						en->location[ LOCATION_FILEPART ] = flags & ACTION_FLAG_NEED_FILEPART ? name_build( FilePart( mimeuri_getattr(ctx, MIMEURIATTR_PATH) ) ) : NULL;
					}

					en->actionnode = actionnode;

					name_delete( olduri );
				}
				else
				{
					D(ACTIONDISP,bug("No Action for event:%d, qualifier:%d\n", data->event, data->qualifier ));
				}

			}
			mimeuri_delete( ctx );

		}
	}

	D(ACTIONDISP,bug("Done\n"));
}

/*
 * This function needs to gather all needed params for function execution.
 * Returns commandlist containing parsed command strings.
 */

static STRPTR getdsturi( struct Data *data )
{
	THREAD;

	if ( data->dst_uri == NULL )
	{
		APTR destselectwin = (APTR)methodstack_push_sync( app, 3, MM_Application_CreateDestSelectorwin, thread_get(), data->refwin );

		if ( destselectwin )
		{
			STRPTR dest = NULL;
			methodstack_push( destselectwin, 3, MUIM_Set, MUIA_Window_Open, TRUE );

			thread_wait();

			methodstack_push_sync( destselectwin, 3, OM_GET, MA_DestSelector_Path, &dest );
			if ( dest != NULL )
			{
				data->dst_uri_unquoted = name_build( dest );
				data->dst_uri = name_build_readargs_quoted(data->dst_uri_unquoted, NULL, 0);
			}

			methodstack_push( app, 2, MM_Application_DisposeWindow, destselectwin );
		}
	}

	return data->dst_uri;
}

static LONG string_append_location(String *string, STRPTR location, STRPTR separator)
{
	LONG error = FALSE;
	ASSERT(location);

	string_append_printf(string, "%s%s", location, separator ? separator : (STRPTR)"");

	return error;
}

static APTR entry_parsecommands( struct Data *data, APTR entry )
{
	struct entrynode *en = entry;
	APTR command_list;
	APTR parsed_command_list = commandlist_create();
	ULONG error = FALSE;

	/*
	 * Build list.
	 */

	command_list = actionnode_getattr( en->actionnode, ACTIONNODETAG_COMMAND_LIST );

	if ( !ISLISTEMPTY( command_list ) )
	{
		APTR cn;

		ITERATELIST( cn, command_list )
		{
			ULONG type = (ULONG)commandnode_getattr( cn, COMMANDNODETAG_TYPE );
			STRPTR command = commandnode_getattr( cn, COMMANDNODETAG_COMMAND );

			ULONG iterator = 0;
			ULONG len = strlen( command );
			String *newcommand = string_new_len( NULL, len );

			if ( newcommand )
			{
				D(ACTIONDISP,bug("Command to parse <%s>\n", command ));

				while ( iterator < len && !error )
				{
					switch ( command[ iterator ] )
					{
						case '%':
							{
								/*
								 * Check for known placeholders.
								 */

								ULONG primary = ( len > iterator + 1 ) ? command[ iterator + 1 ] : 0;
								ULONG secondary =  ( len > iterator + 2 ) ? command[ iterator + 2 ] : 0;
								ULONG size = 1;

								if ( primary == 's' ) /* selected entries in source lister. */
								{
									if ( secondary == 'p' ) /* path */
									{
										error = string_append_location(newcommand, en->location[ LOCATION_PATH ], NULL);
										size++;
									}
									else if ( secondary == 'f' ) /* filename only */
									{
										error = string_append_location(newcommand, en->location[ LOCATION_FILEPART ], NULL);
										size++;
									}
									else /* uri */
									{
										ASSERT( en->location[ LOCATION_URI ] );
										
										//DB(("%%s: LOCATION_URI <%s>\n", en->location[ LOCATION_URI ]));

										/*
										 * Don't decode uri if it's file:/// form.
										 * Quoting sucks here again, for some shitty reason
										 * the location_uri might come quoted... - Piru
										 */
										if (strncmp(en->location[ LOCATION_URI ], "file:/", 6) != 0 &&
										    !(strncmp(en->location[ LOCATION_URI ], "\"file:/", 7) == 0 && en->location[ LOCATION_URI ][strlen(en->location[ LOCATION_URI ]) - 1] == '"'))
										{
											UBYTE decodeduri[strlen(en->location[ LOCATION_URI ]) + 1];
											mimeuri_decodepath(decodeduri, sizeof(decodeduri), en->location[ LOCATION_URI ], -1);

											error = string_append_location(newcommand, decodeduri, NULL);
										}
										else
										{
											error = string_append_location(newcommand, en->location[ LOCATION_URI ], NULL);
										}
									}
								}
								else if ( primary == 'S' ) /* source lister's path */
								{
									if ( secondary == 'i' ) /* identifier */
									{
										string_append_printf( newcommand, "%d", data->srcid );
										size++;
									}
									else                    /* path */
									{
										error = string_append_location(newcommand, data->src_uri, NULL);
									}

								}
								else if ( primary == 'D' ) /* destination lister */
								{
									if ( secondary == 'i' ) /* identifier */
									{
										string_append_printf( newcommand, "%d", data->dstid );
										size++;
									}
									if ( secondary == 'p' ) /* path. that will include / at the end of name */
									{
										STRPTR dsturi = getdsturi( data );
										STRPTR separator = "";

										if ( dsturi == NULL )
										{
											error = TRUE;
										}
										else
										{
											LONG l = strlen( dsturi );

											if ( len && dsturi[ l - 1 ] != '/' && dsturi[ l - 1 ] != ':' )
												separator = "/";

											error = string_append_location(newcommand, data->dst_uri, separator);
										}
										
										size++;
									}
									else                    /* path */
									{
										STRPTR dsturi = getdsturi( data );

										if ( dsturi == NULL )
										{
											error = TRUE;
										}
										else
										{
											error = string_append_location(newcommand, data->dst_uri, NULL);
										}
									}
								}
								else
								{
									/* unknown placeholder. copy */
									string_append_len( newcommand, command + iterator, 2 );
								}

								iterator += 1 + size;  /* skip % + primary + secondary (if recognized) */

							}
							break;

						default:
							{
								/* just copy */
								string_append_c( newcommand, command[ iterator ] );
								iterator++;
							}
					}
				}

				if ( error )
				{
					string_free( newcommand, FALSE );
					commandlist_delete( parsed_command_list );
					return NULL;
				}

				/*
				 * Add to parsed command list.
				 */

				commandlist_addcommand( parsed_command_list, type, newcommand->str );

				string_free( newcommand, FALSE );
			}
		}

	}

	return parsed_command_list;
}



static int comparefunc( struct MinNode *e1, struct MinNode *e2 )
{
	struct entrynode *en1 = ( struct entrynode* )e1;
	struct entrynode *en2 = ( struct entrynode* )e2;

	/*
	 * We sort by action. It's enough to compare pointers.
	 */

	if ( en1->actionnode > en2->actionnode )
		return 1;
	else if ( en1->actionnode < en2->actionnode )
		return -1;
	else
		return en1->idx > en2->idx ? 1 : -1;
}

DEFTMETHOD(ActionDispatcher_Execute)
{

	/*
	 * Spawn a thread which will execute all the actions
	 */

	D(ACTIONDISP,bug("Spawning thread for 0x%x\n",obj));

	do_action( obj, TA_ActionDispatcher_Execute,
			TAG_DONE);

	return 0;

}


ULONG tr_actiondispatcher_execute( APTR obj )
{
	struct IClass *cl = getactiondispatcherclass();
	struct entrynode *en, *nexten;
	GETDATA;

	ULONG abort = FALSE;
	ULONG rc = TRUE;
	APTR refwin = data->refwin;

	THREAD;

	/*
	 * If ref window was given we put it to sleep
	 */

	if ( refwin && !_aprefs(donotputrefwindowtosleep))
	{
		static const struct TagItem tags[] = { {MUIA_Window_Sleep, TRUE}, {TAG_DONE, 0} };
		methodstack_push( refwin, 2, OM_SET, tags );
	}

	D(ACTIONDISP,bug("Executing action dispatcher 0x%x...\n", obj ));

	/*
	 * Lookup actions for entries on the list.
	 */

	ITERATELIST(en, &data->entries)
	{
		if (threads_check_abort())
		{
			abort = TRUE;
			rc = ABORTED;
			goto aborted;
		}

		if ( !abort )
			action_lookup( data, en, data->action );
	}

	/*
	 * If list is not sorted then we do it.
	 */

	if ( !data->sorted )
		mergesortlist( &data->entries, comparefunc );

	data->sorted = TRUE;

	/*
	 * Perform merging. It's done my concatenating URI's into longer, space separated
	 * strings.
	 */

	ITERATELIST(en, &data->entries)
	{
		if (threads_check_abort())
		{
			abort = TRUE;
			rc = ABORTED;
			goto aborted;
		}

		if ( !abort && en->actionnode )
		{
			LONG flags = (LONG)actionnode_getattr( en->actionnode, ACTIONNODETAG_FLAGS );
			LONG mergable = flags & ACTION_FLAG_MULTIPLE;

			if ( mergable )
			{
				/*
				 * For all following uris which have same action...
				 */

				struct entrynode *last_en = en;
				struct entrynode *next_en = en;
				ULONG len[ LOCATION_MAX ] = {0, 0, 0};
				ULONG num = 0;
				ULONG location_i;

				while( NEXTNODE(next_en) && next_en->actionnode == en->actionnode )
				{
					for( location_i=0; location_i<LOCATION_MAX; location_i++)
					{
						if ( next_en->location[ location_i ] )
							len[ location_i ] += strlen( next_en->location[ location_i ] );
					}

					last_en = next_en;
					num++;

					next_en	= NEXTNODE( next_en );
				}

				if ( last_en != en )
				{
					ULONG off[ LOCATION_MAX ] = {0, 0, 0 };
					ULONG location_i;

					STRPTR new_location[ LOCATION_MAX ] = {NULL, NULL, NULL};
					ULONG allocated = TRUE;

					for( location_i=0; location_i<LOCATION_MAX; location_i++)
					{
						if ( len[ location_i ] )
						{
							new_location[ location_i ] = malloc( len[ location_i ] + num + 1 );
							if ( !new_location[ location_i ] )
							{
								allocated = FALSE;
							}
						}
					}

					D(ACTIONDISP,bug("Lengths:path: %d filepart: %d uri: %d\n", len[ LOCATION_PATH ], len[ LOCATION_FILEPART ], len[ LOCATION_URI ] ));

					if ( allocated )
					{
						ULONG i;
						for(i=0; i<num; i++ )
						{
							struct entrynode *nn = NEXTNODE( en );
							ULONG location_i;

							/* add new location to merged string */

							for( location_i=0; location_i<LOCATION_MAX; location_i++)
							{
								if ( en->location[ location_i ] )
								{
									strcpy( new_location[ location_i ] + off[ location_i ], en->location[ location_i ] );
									off[ location_i ] += strlen( en->location[ location_i ] );

									if ( en != last_en )
									{
										/* separator and endofline */

										new_location[ location_i ][ off[ location_i ]++ ] = ' ';
										new_location[ location_i ][ off[ location_i ] ] = 0;
									}

									name_delete( en->location[ location_i ] );
								}
							}

							if ( en != last_en )
							{
								REMOVE( en );
								free( en );
							}

							en = nn;
						}

						/*
						 * Setup new node (replace last one).
						 */

						D(ACTIONDISP,bug("Merged paths:\n"));
						for( location_i=0; location_i<LOCATION_MAX; location_i++)
						{
							if ( new_location[ location_i ] )
							{
								D(ACTIONDISP,bug("  %d:<%s>\n", location_i, new_location[ location_i ] ));
							}
						}

						en = last_en;

						for( location_i=0; location_i<LOCATION_MAX; location_i++)
							en->location[ location_i ] = new_location[ location_i ];

						/*
						 * We can also skip this node from further checks.
						 */

						if ( en != LASTNODE( &data->entries ) )
						{
							en = NEXTNODE( en );
						}
					}
				}
			}
		}
	}

	D(ACTIONDISP,bug("Action:Source uri:<%s>\n", data->src_uri ));
	D(ACTIONDISP,bug("Action:Source id:<%d>\n", data->srcid ));
	D(ACTIONDISP,bug("Action:Destination uri:<%s>\n", data->dst_uri ));
	D(ACTIONDISP,bug("Action:Destination id:<%d>\n", data->dstid ));

	/* If aborted we come here for cleanup (free nodes) */
aborted:
	ITERATELISTSAFE(en, nexten, &data->entries)
	{
		if (!abort && threads_check_abort())
		{
			abort = TRUE;
			rc = ABORTED;
		}

		if ( !abort )
		{

			/*
			 * Check if action has flag informing that it can't take
			 * multiple files in one go.
			 */

			/* XXX: For now action is always performed on single file. */

			/*
			 * Expand action template string.
			 */

			if ( en->actionnode )
			{
				APTR command_list = entry_parsecommands(data, en );

				if ( command_list )
				{
					BPTR curdir = (BPTR)NULL;
					BPTR newdir = (BPTR)NULL;
					STRPTR path = NULL;

					/* if needed, change current dir to source or dest path */

					if ((LONG)actionnode_getattr( en->actionnode, ACTIONNODETAG_FLAGS ) & ACTION_FLAG_CD_SOURCE)
						path = data->src_uri_unquoted;
					else if ((LONG)actionnode_getattr( en->actionnode, ACTIONNODETAG_FLAGS ) & ACTION_FLAG_CD_DESTINATION)
						path = data->dst_uri_unquoted;

					if (path != NULL)
					{
						newdir = Lock( path, ACCESS_READ );

						if ( newdir != (BPTR)NULL )
							curdir = CurrentDir( newdir );
					}

					commandlist_execute( obj, command_list );

					commandlist_delete( command_list );

					/*
					 * Revert to old current dir.
					 */

					if ( newdir )
					{
						UnLock(CurrentDir(curdir));
					}
				}
			}

			/*
			 * Remove node from a list.
			 */

		}

		delete_node( en );
	}
	NEWLIST(&data->entries);

	/*
	 * Execute action for each file in a group.
	 */


	/*
	 * Remove group from the list.
	 */


	/*
	 * If window was put to sleep, last thing we do is 'unsleep' it.
	 */

	if ( refwin && !_aprefs(donotputrefwindowtosleep))
	{
		static const struct TagItem tags[] = { {MUIA_Window_Sleep, FALSE}, {TAG_DONE, 0} };
		methodstack_push( refwin, 2, OM_SET, tags );
	}

	return rc;

}

DEFSMETHOD(Thread_Finished)
{
	switch (msg->action)
	{
		case TA_ActionDispatcher_Execute:
			{
			
				/*
				 * Execution finished. Dispose object.
				 */

				D(ACTIONDISP,bug("Dispose actiondispatcher obj:0x%x\n", obj));
				DoMethod(app, MUIM_Application_PushMethod, app, 2, MM_Application_DeleteActionDispatcher, obj);
				break;
			}
	}

	return (0);
}




BEGINMTABLE
DECNEW
DECSET
DECGET
DECDISP
DECSMETHOD(ActionDispatcher_AddURI)
DECSMETHOD(Thread_Finished)
DECTMETHOD(ActionDispatcher_Execute)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Notify, actiondispatcherclass)
