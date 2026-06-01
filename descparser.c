/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2005-2006 Michal Wozniak
 * Copyright 2006-2011 Ambient Open Source Team
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
 * $Id: descparser.c,v 1.10 2026/05/14 22:40:54 geit Exp $
 */

#include "ambient.h"

#include <proto/dos.h>

#include "name.h"
#include "mimetype.h"
#include "recog.h"
#include "command.h"
#include "mimeuri.h"
#include "action.h"
#include "strings.h"
#include "descparser.h"
#include "locale.h"

/*
 * EXPERIMENTAL: This is a function which is creating mimetype descriptors according to
 * their descriptors stored in :Prefs/Ambient/filetypes/ path.
 */

static	char	linewords[16][512];
static	int		numLineWords;


/* this one is reading one line and it's words */

static int readline( BPTR in )
{
	TEXT line[1024];

	for(;;)
	{
		if	( FGets( in, line, sizeof(line)) )
		{
			LONG l;

			/* check for comment */

			if ( line[ 0 ] != ';' )
			{
				/*
				 * Remove EOL.
				 */

				l = strlen( line );
				if ( l > 1 )
				{
					if  (line[ l - 1] == '\n' )
					{
						line [l-1] = 0;
					}
				}

				/*
				 * Read line's words.
				 */

				numLineWords = sscanf(line, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
					linewords[0], linewords[1], linewords[2], linewords[3],
					linewords[4], linewords[5], linewords[6], linewords[7],
					linewords[8], linewords[9], linewords[10], linewords[11],
					linewords[12], linewords[13], linewords[14], linewords[15]);

				return	1;
			}
		}
		else
		{
			/*
			 * End of file.
			 */

			numLineWords = 0;
			return	0;
		}
	}
}
static ULONG parse_match( BPTR file, struct internal_mimetype_node *imn )
{
	if ( !imn->rctx )
		imn->rctx = recog_create();

	if ( !imn->rctx )
		return 1;

	while( readline( file ) )
	{
		if ( !stricmp( linewords[ 0 ], "End" ) )
		{
			D(MIMETYPE,bug("\tEnd of recognition block reached\n"));
			return 0;
		}
		else if ( !stricmp( linewords[ 0 ], "Name" ) )
		{
			D(MIMETYPE,bug("\t\tMatch name:%s\n", linewords[ 1 ] ));
			recog_add(imn->rctx, RECOGCMD_FILENAME, linewords[ 1 ] );
		}
		else if ( !stricmp( linewords[ 0 ], "Match" ) )
		{
			D(MIMETYPE,bug("\t\tMatch file contents: %s\n", linewords[ 1 ] ));
			recog_add(imn->rctx, RECOGCMD_MATCH, linewords[ 1 ] );
		}
		else if ( !stricmp( linewords[ 0 ], "MatchHunk" ) )
		{
			D(MIMETYPE,bug("\t\tMatchhunk hunk contents: %s\n", linewords[ 1 ] ));
			recog_add(imn->rctx, RECOGCMD_MATCHHUNK, linewords[ 1 ] );
		}
		else if ( !stricmp( linewords[ 0 ], "Symbol" ) )
		{
			D(MIMETYPE,bug("\t\tMatch symbol: %s\n", linewords[ 1 ] ));
			recog_add(imn->rctx, RECOGCMD_FINDSTRING, linewords[ 1 ] );
		}
		else if ( !stricmp( linewords[ 0 ], "File" ) )
		{
			D(MIMETYPE,bug("\t\tMatch File\n" ));
			recog_add(imn->rctx, RECOGCMD_FILE, NULL );
		}
		else if ( !stricmp( linewords[ 0 ], "Directory" ) )
		{
			D(MIMETYPE,bug("\t\tMatch Directory\n" ));
			recog_add(imn->rctx, RECOGCMD_DIRECTORY, NULL );
		}
		else if ( !stricmp( linewords[ 0 ], "FileSize" ) )
		{
			D(MIMETYPE,bug("\t\tMatch FileSize: %s\n", linewords[ 1 ] ));
			recog_add(imn->rctx, RECOGCMD_FILESIZE, linewords[ 1 ] );
		}
		else if ( !stricmp( linewords[ 0 ], "ContentType" ) )
		{
			D(MIMETYPE,bug("\t\tGuess Content Type: %s\n", linewords[ 1 ] ));
			recog_add(imn->rctx, RECOGCMD_CONTENT, linewords[ 1 ] );
		}
		else if ( !stricmp( linewords[ 0 ], "Device" ) )
		{
			D(MIMETYPE,bug("\t\tMatch Device\n" ));
			recog_add(imn->rctx, RECOGCMD_DEVICE, NULL );
		}
		else if ( !stricmp( linewords[ 0 ], "Comment" ) )
		{
			D(MIMETYPE,bug("\t\tMatch Comment\n" ));
			recog_add(imn->rctx, RECOGCMD_COMMENT, linewords[ 1 ] );
		}
		else if ( !strcmp( linewords[ 0 ], "(" ) )
		{
			D(MIMETYPE,bug("\t\t(\n"));
			recog_add(imn->rctx, RECOGCMD_BLOCKIN, NULL );
		}
		else if ( !strcmp( linewords[ 0 ], ")" ) )
		{
			D(MIMETYPE,bug("\t\t)\n"));
			recog_add(imn->rctx, RECOGCMD_BLOCKOUT, NULL );
		}
		else if ( !stricmp( linewords[ 0 ], "OR" ) )
		{
			D(MIMETYPE,bug("\t\tOR\n"));
			recog_add(imn->rctx, RECOGCMD_OR, NULL );
		}
		else if ( !stricmp( linewords[ 0 ], "AND" ) )
		{
			D(MIMETYPE,bug("\t\tAND\n"));
			recog_add(imn->rctx, RECOGCMD_AND, NULL );
		}
		else if ( !stricmp( linewords[ 0 ], "PatternHint" ) )
		{
			D(MIMETYPE,bug("\t\tPattern hint:%s\n", linewords[ 1 ] ));
			recog_sethintpattern(imn->rctx, linewords[ 1 ] );
		}
		else if ( !stricmp( linewords[ 0 ], "Protection" ) )
		{
			D(MIMETYPE,bug("\t\tMatch file protection: %s\n", linewords[ 1 ] ));
			recog_add(imn->rctx, RECOGCMD_PROTECTION, linewords[ 1 ] );
		}
	}

	/*
	 * Error. Premature end of file.
	 */
	recog_delete(imn->rctx);
	imn->rctx = NULL;

	return 1;
}

static ULONG parse_action(  BPTR file, struct internal_mimetype_node *imn )
{
	APTR an = actionnode_create();
	ULONG flags = 0;
	ULONG qualifier = ACTION_QUALIFIER_NONE;

	if ( !an )
		return 1;

	while( readline( file ) )
	{
		if ( !stricmp( linewords[ 0 ], "End" ) )
		{
			D(MIMETYPE,bug("  End of action block reached\n"));

			actionnode_setattrs( an, ACTIONNODETAG_FLAGS, flags, TAG_DONE );
			actionnode_setup( an );

			ADDTAIL( imn->action_list, an );
			return 0;
		}
		else if ( !stricmp( linewords[ 0 ], "Name" ) )
		{
			String *name = string_new(NULL);
			if ( name )
			{
				int i;
				for(i=1; i<numLineWords; i++)
				{
					if ( i > 1 )
						string_append_printf( name, " %s", linewords[ i ] );
					else
						string_append( name, linewords[ i ] );
				}
#if USE_RECOGTRANSLATION
				actionnode_setattrs( an, ACTIONNODETAG_NAME, locale_translationget( name->str ), TAG_DONE );
#else
				actionnode_setattrs( an, ACTIONNODETAG_NAME, name->str, TAG_DONE );
#endif
				D(MIMETYPE,bug("    Name:<%s>\n", name->str ));
				string_free( name, FALSE );
			}
		}
		else if ( !stricmp( linewords[ 0 ], "Menu" ) )
		{
			String *name = string_new(NULL);
			if ( name )
			{
				int i;
				for(i=1; i<numLineWords; i++)
				{
					if ( i > 1 )
						string_append_printf( name, " %s", linewords[ i ] );
					else
						string_append( name, linewords[ i ] );
				}
#if USE_RECOGTRANSLATION
				actionnode_setattrs( an, ACTIONNODETAG_MENU_NAME, locale_translationget( name->str ), TAG_DONE );
#else
				actionnode_setattrs( an, ACTIONNODETAG_MENU_NAME, name->str, TAG_DONE );
#endif
				D(MIMETYPE,bug("    Menu Name:<%s>\n", name->str ));
				string_free( name, FALSE );
			}
		}
		else if ( !stricmp( linewords[ 0 ], "Event" ) )
		{
			D(MIMETYPE,bug("    Event:%s\n", linewords[ 1 ] ));

			if ( !stricmp( linewords[ 1 ], "doubleclick" ) )
			{
				actionnode_setattrs( an, ACTIONNODETAG_EVENT, ACTION_EVENT_DOUBLECLICK, TAG_DONE );
			}
			else if ( !stricmp( linewords[ 1 ], "dragndrop" ) )
			{
				actionnode_setattrs( an, ACTIONNODETAG_EVENT, ACTION_EVENT_DRAGNDROP, TAG_DONE );
			}
			else if ( !stricmp( linewords[ 1 ], "menu" ) )
			{
				actionnode_setattrs( an, ACTIONNODETAG_EVENT, ACTION_EVENT_MENU, TAG_DONE );
			}
			else
			{
				D(MIMETYPE,bug("    ERROR: Unrecognized event!\n"));
			}
		}
		else if ( !stricmp( linewords[ 0 ], "Command" ) )
		{
			String *cmd;
			LONG type = -1;

			D(MIMETYPE,bug("    Type:%s\n", linewords[ 1 ] ));
			if ( !stricmp( linewords[ 1 ], "amigados" ) )
			{
				type = AC_AMIGADOS;
			}
			else if ( !stricmp( linewords[ 1 ], "workbench" ) )
			{
				type = AC_WORKBENCH;
			}
			else if ( !stricmp( linewords[ 1 ], "script" ) )
			{
				type = AC_SCRIPT;
			}
			else if ( !stricmp( linewords[ 1 ], "arexx" ) )
			{
				type = AC_AREXX;
			}
			else if ( !stricmp( linewords[ 1 ], "internal" ) )
			{
				type = AC_INTERNAL;
			}
			else
			{
				D(MIMETYPE,bug("    ERROR: Unrecognized type (<%s>)\n", linewords[ 1 ]));
			}

			if ( type != -1 )
			{
				cmd = string_new( NULL );

				if ( cmd )
				{
					int i;
					for(i = 2; i < numLineWords; i++)
					{
						if ( i > 2 )
							string_append_printf( cmd, " %s", linewords[ i ] );
						else
							string_append( cmd, linewords[ i ] );
					}

					actionnode_addcommand( an, type, cmd->str);

					D(MIMETYPE,bug("    Command:<%s>:<%s>\n", linewords[ 1 ], cmd->str ));

					string_free( cmd, FALSE );
				}
			}
		}
		else if ( !stricmp( linewords[ 0 ], "Flag" ) && numLineWords > 1 )
		{
			D(MIMETYPE,bug("    Flags:%s\n", linewords[ 1 ] ));
			if ( !stricmp( linewords[ 1 ], "unquoted" ) )
			{
				flags |= ACTION_FLAG_UNQUOTED;
			}
			else if ( !stricmp( linewords[ 1 ], "cd" ) && numLineWords == 3 )
			{
				if ( !stricmp( linewords[ 2 ], "source" ) )
					flags |= ACTION_FLAG_CD_SOURCE;
				else if ( !stricmp( linewords[ 2 ], "destination" ) )
					flags |= ACTION_FLAG_CD_DESTINATION;
			}
			else if( !stricmp( linewords[ 1 ], "multiple" ) )
			{
				flags |= ACTION_FLAG_MULTIPLE;
			}
			else if ( !stricmp( linewords[ 1 ], "asynchronous" ) )
			{
				/* XXX: Don't enable for now */
				//flags |= ACTION_FLAG_ASYNC;
			}
			else
			{
				D(MIMETYPE,bug("    ERROR: Unrecognized flag\n"));
			}
		}
		else if ( !stricmp( linewords[ 0 ], "Qualifier" ) && numLineWords > 1 )
		{
			D(MIMETYPE,bug("    Qualifier:%s\n", linewords[ 1 ] ));

			if ( !stricmp( linewords[ 1 ], "ALT" ) )
			{
				qualifier = ACTION_QUALIFIER_ALT;
			}
			else if ( !stricmp( linewords[ 1 ], "SHIFT" ))
			{
				qualifier = ACTION_QUALIFIER_SHIFT;
			}
			else if ( !stricmp( linewords[ 1 ], "CONTROL" ))
			{
				qualifier = ACTION_QUALIFIER_CONTROL;
			}
			else
			{
				D(MIMETYPE,bug("    ERROR: Unrecognized qualifier\n"));
			}

			actionnode_setattrs( an, ACTIONNODETAG_QUALIFIER, qualifier, TAG_DONE );
		}
	}

	/*
	 * Error. Premature end of file.
	 */
	actionnode_delete(an);

	return 1;
}

APTR descriptor_parse( CONST_STRPTR fname, BPTR fhandle )
{
	ULONG complete = FALSE;
	struct internal_mimetype_node *imn;
	ULONG error = 0;

	/*
	 * Skip leading empty lines (for multi-descriptor files)
	 */

	while ( readline( fhandle ) && numLineWords == -1 )
		{}

	/*
	 * Check ID and version.
	 */

	if ( numLineWords )
	{
		if ( stricmp( linewords[ 0 ], "AMTD" ) )
		{
			D(MIMETYPE,bug("Incorrect filetype descriptor (%s)\n", linewords[ 0 ] ));
			return NULL;
		}
	}

	if ( readline( fhandle ) )
	{
		if ( strcmp( linewords[ 0 ], "1" ) )
		{
			D(MIMETYPE,bug("Incorrect filetype descriptor version (%s)\n", linewords[ 0 ] ));
			return NULL;
		}
	}

	imn = malloc(sizeof(*imn));

	if ( !imn )
	{
		DB(("Not enough memory for internal mimetype node\n"));
		return NULL;
	}

	if ( fname )
	{
		if (!(imn->descriptor = malloc(strlen(fname) + 1)))
		{
			D(MIMETYPE,bug("Not enough memory for descriptor file string\n"));
			free(imn);

			return NULL;
		}

		strcpy(imn->descriptor, fname);
	}
	else
	{
		imn->descriptor = NULL;
	}

	imn->rctx        = NULL;
	imn->action      = MIMEACTION_USER; /* XXX */
	imn->newwin      = TRUE;
	imn->mimetype    = NULL;
	imn->flags       = 0;
	imn->description = NULL;
	imn->priority    = 0;

	NEWLIST( &imn->internal_action_list );
	imn->action_list = &imn->internal_action_list;

	/*
	 * Parse file (hope no bastard edited it by hand...).
	 */

	while( !complete && readline( fhandle ) )
	{
		if ( !stricmp( linewords[ 0 ], "End" ) )
		{
			D(MIMETYPE,bug("End of file reached!\n"));
			complete = TRUE;
		}
		else if ( !stricmp( linewords[ 0 ], "Name" ) && numLineWords > 1 )
		{
			String *name = string_new(NULL);
			if ( name )
			{
				int i;
				for(i=1; i<numLineWords; i++)
				{
					if ( i > 1 )
						string_append_printf( name, " %s", linewords[ i ] );
					else
						string_append( name, linewords[ i ] );
				}

				imn->description = string_free( name, TRUE );
			}
			D(MIMETYPE,bug("  Name:%s\n", linewords[ 1 ] ));
		}
		else if ( !stricmp( linewords[ 0 ], "Type" ) && numLineWords == 2 )
		{
			D(MIMETYPE,bug("  Type: %s\n", linewords[ 1 ] ));
			imn->mimetype = name_build( linewords[ 1 ] );
		}
		else if ( !stricmp( linewords[ 0 ], "Match" ) )
		{
			D(MIMETYPE,bug("  Recognition block...\n"));
			error = parse_match( fhandle, imn );
		}
		else if ( !stricmp( linewords[ 0 ], "Action" ) )
		{
			D(MIMETYPE,bug("  Action definition block...\n"));
			error = parse_action( fhandle, imn );
		}
		else if ( !stricmp( linewords[ 0 ], "Priority" ) && numLineWords == 2 )
		{
			D(MIMETYPE,bug("  Priority: %ld\n", strtol(linewords[1], NULL, 0)));
			imn->priority = strtol( linewords[ 1 ], NULL, 0 );
		}

		if ( error )
		{
			break;
		}
	}

	if ( !complete )
	{
		if (error)
		{
			D(MIMETYPE,bug("Error while parsing file.\n"));
		}

		/*  clean up
		 */
		{
			APTR an, nextan;

			ITERATELISTSAFE(an, nextan, imn->action_list)
			{
				actionnode_delete(an);
			}
			NEWLIST(imn->action_list); /* must not be omitted! */

			if (imn->descriptor) free(imn->descriptor);

			free (imn);
		}

		imn = NULL;
	}

	return imn;
}

APTR descriptor_parse_filename( CONST_STRPTR fname )
{
	BPTR fhandle = Open( fname, MODE_OLDFILE );
	APTR imn;

	if ( !fhandle )
	{
		D(MIMETYPE,bug("Failed to open file:%s\n",fname ));
		return NULL;
	}

	D(MIMETYPE,bug("Processing file:   %s\n", fname ));

	imn = descriptor_parse( fname, fhandle );

	Close( fhandle );

	return imn;
}
