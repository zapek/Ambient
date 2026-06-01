/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2006 Ambient Open Source Team
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
 * $Id: mimetype.c,v 1.27 2025/08/14 19:07:30 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>
#include <dos/dosextens.h>
#include <devices/trackdisk.h>

/* private */
#include "mimetype.h"
#include "mui_func.h"
#include "iconio.h"
#include "methodstack.h"
#include "recog.h"
#include "mimeuri.h"
#include "hash.h"
#include "name.h"
#include "mimeprefs.h"
#include "file_func.h"
#include "time_func.h"
#include "datatypes.h"
#include "multimedia.h"
#include "getdirsize.h"
#include "mimetype.h"
#include "command.h"
#include "actiondispatcherclass.h"
#include "action.h"
#include "descparser.h"
#include "prefs.h"
#include "prefs_advanced.h"
#include "dosnotify.h"
#include "cache.h"
#include "ambient_cat.h"
#include "devices.h"
#include "examine64.h"
#include "info64.h"
#include "listsort.h"

struct mimetype_mediatype {
	ULONG type;
	STRPTR name;
};

static const struct mimetype_mediatype mimetype_mt[] = {
	{MEDIATYPE_INTERNAL,    "internal/"},
	{MEDIATYPE_APPLICATION, "application/"},
	{MEDIATYPE_AUDIO,       "audio/"},
	{MEDIATYPE_IMAGE,       "image/"},
	{MEDIATYPE_MESSAGE,     "message/"},
	{MEDIATYPE_MULTIPART,   "multipart/"},
	{MEDIATYPE_TEXT,        "text/"},
	{MEDIATYPE_VIDEO,       "video/"},
	{0, NULL}
};

struct mimetype_ctx {
	ULONG mediatype; /* MEDIATYPE_#? */
	STRPTR mimetype; /* application/foobar etc.. */
	STRPTR description;
	ULONG action;    /* MIMEACTION_#? */
	ULONG newwin;
	ULONG subtype;   /* used by datatypes and mimetypes */
	ULONG fileinfo;
	ULONG seconds;
	UQUAD filesize;
};

static struct MinList internal_mimetype_list;
static struct MinList external_mimetype_list;
static struct MinList notifyctx_list;

struct notifyctx_node  {
	struct MinNode n;
	APTR notifyctx;
};

static struct internal_mimetype_node *get_mimetypenode(
	ULONG recogcmd, CONST_STRPTR recogstr, CONST_STRPTR mtype,
	CONST_STRPTR description, ULONG action)
{
	struct internal_mimetype_node *imn;
	APTR rctx = NULL;

	if ( (imn = malloc(sizeof(*imn))) )
	{
		if (recogcmd != RECOGCMD_NONE)
		{
			if ( (rctx = recog_create()) )
			{
				recog_add(rctx, recogcmd, recogstr);
			}
		}

		/* XXX: actually.. check imn->rctx because it needs to be set for cleanup_mimetype() to not freakout  */
		imn->rctx = rctx;
		imn->mimetype = name_build( mtype );
		imn->description = name_build( description );
		imn->descriptor = NULL;
		imn->action = action;
		imn->newwin = FALSE;
		imn->priority = -512;
		imn->flags = MIMETYPEFLAG_INTERNAL;
		NEWLIST( &imn->internal_action_list );
		imn->action_list = &imn->internal_action_list;
		ADDTAIL(&internal_mimetype_list, imn);
	}

	return imn;
}

struct internal_mimetype_node *imn_create(CONST_STRPTR mimetype, CONST_STRPTR description)
{
	struct internal_mimetype_node * imn;

	if ( (imn = malloc(sizeof(*imn))) )
	{
		ULONG success = TRUE;

		imn->rctx = NULL;
/*
		imn->rctx = recog_create();

		if(!imn->rctx)
		{
			success = FALSE;
		}
*/
		if(mimetype)
		{
			imn->mimetype = name_build(mimetype);
		}
		else
		{
			imn->mimetype = NULL;
		}

		if(description)
		{
			imn->description = name_build(description);
		}
		else
		{
			imn->description = NULL;
		}

		imn->descriptor = NULL;
		imn->action     = MIMEACTION_USER;
		imn->newwin     = FALSE;
		imn->priority   = 0;
		imn->flags      = 0;

		NEWLIST( &imn->internal_action_list );
		imn->action_list = &imn->internal_action_list;

		if(!success)
		{
			imn_delete(imn);
			imn = NULL;
		}
	}

	return imn;
}

void imn_delete(struct internal_mimetype_node *imn)
{
	if ( !imn )
		return;

	if ( imn->rctx && !(imn->flags & MIMETYPEFLAG_FOREIGNRECOG) )
	{
		recog_delete( imn->rctx );
	}

	/* XXX: really ? */
	if( (imn->flags & MIMETYPEFLAG_FOREIGNACTIONS) == 0)
	{
		if ( !ISLISTEMPTY(imn->action_list) )
		{
			APTR an, nextan;

			ITERATELISTSAFE(an, nextan, imn->action_list)
			{
				actionnode_delete(an);
			}
		}
	}

	if ( imn->flags & MIMETYPEFLAG_INTERNAL )
	{

	}
	else
	{
		if ( imn->description )
			name_delete( imn->description );

		if ( imn->mimetype )
			name_delete( imn->mimetype );

		if ( imn->descriptor )
			name_delete( imn->descriptor );

	}

	free( imn );
}

struct internal_mimetype_node *imn_duplicate(struct internal_mimetype_node * simn)
{
	struct internal_mimetype_node *dimn;

	if ( (dimn = malloc(sizeof(*simn))) )
	{
		ULONG success = TRUE;

		if(simn->flags & MIMETYPEFLAG_FOREIGNRECOG)
		{
			dimn->rctx = simn->rctx;
		}
		else
		{
			dimn->rctx = recog_duplicate(simn->rctx);

			if(simn->rctx && !dimn->rctx)
			{
				success = FALSE;
			}
		}

		dimn->mimetype = simn->mimetype ? name_build( simn->mimetype ) : NULL;
		if(simn->mimetype && !dimn->mimetype)
		{
			success = FALSE;
		}

		dimn->description = simn->description ? name_build( simn->description ) : NULL;
		if(simn->description && !dimn->description)
		{
			success = FALSE;
		}

		dimn->descriptor = simn->descriptor ? name_build( simn->descriptor) : NULL;
		if(simn->descriptor && !dimn->descriptor)
		{
			success = FALSE;
		}

		dimn->action   = simn->action;
		dimn->newwin   = simn->newwin;
		dimn->priority = simn->priority;
		dimn->flags    = simn->flags;

		NEWLIST( &dimn->internal_action_list );

		/* if type has its own actions copy them */
		if ( !ISLISTEMPTY( &simn->internal_action_list ) )
		{
			APTR san = NULL;
			APTR dan = NULL;

			ITERATELIST( san, &simn->internal_action_list )
			{
				dan = actionnode_duplicate( san );

				if(dan)
				{
					ADDTAIL(&dimn->internal_action_list, dan);
				}
				else
				{
					success = FALSE;
				}
			}

			dimn->action_list = &dimn->internal_action_list;
		}
		else /* otherwise, just assign action_list (external actions) */
		{
			if ( !ISLISTEMPTY( simn->action_list ) )
			{
				dimn->action_list = simn->action_list;
			}
			else
			{
				dimn->action_list = &dimn->internal_action_list;
			}
		}

		if(!success)
		{
			imn_delete(dimn);
			dimn = NULL;
		}
	}

	return dimn;
}

/*
 * Sort types list according to few criteria:
 * - internals have their own priorities and they are not adjusted during sorting (by default it's -512 to be below all user types)
 * - externals have priority depending if they have patternhint:
 *   - if yes, +128 is added
 *   - if not, then we sub 127 from it (assuming valid range is -128...127)
*/

static int comparefunc( struct MinNode *e1, struct MinNode *e2 )
{
	struct internal_mimetype_node *imn1 = ( struct internal_mimetype_node* )e1;
	struct internal_mimetype_node *imn2 = ( struct internal_mimetype_node* )e2;

	LONG pri1 = imn1->priority;
	LONG pri2 = imn2->priority;

	if ( !(imn1->flags & MIMETYPEFLAG_INTERNAL) )
	{
		struct recog_ctx *rctx = imn1->rctx;
		if ( rctx && rctx->pattern )
			pri1 += 128;
		else
			pri1 -= 127;
	}

	if ( !(imn2->flags & MIMETYPEFLAG_INTERNAL) )
	{
		struct recog_ctx *rctx = imn2->rctx;
		if ( rctx && rctx->pattern )
			pri2 += 128;
		else
			pri2 -= 127;
	}

	if ( pri1 > pri2 )
		return -1;
	else if ( pri1 < pri2 )
		return 1;
	else
		return 0;
}

static void sort_types_list( void )
{
	mergesortlist( &internal_mimetype_list, comparefunc );
}

/*
 * For imns without recognition routine, we check if it can be found one list of external ones.
 * If found, we assign it. Also, if type doesn't exist on internal list and complete type
 * exists on external one, then we move it from external list to internal one.
 */

static void merge_types_lists(void)
{
	struct internal_mimetype_node *imn;
	struct internal_mimetype_node *emn;

	D(MIMETYPE,bug("Merging recognition routines\n"));

	/*
	 * Pass 1: assign recognition routines only.
	 */

	D(MIMETYPE,bug("1:Assigning regonition routines...\n"));

	ITERATELIST( imn, &internal_mimetype_list )
	{
		/*
		 * Items we can inherit from external database.
		 */

		if ( imn->rctx == NULL || imn->description == NULL )
		{
			/*
			 * Match by mimetype.
			 */

			D(MIMETYPE,bug("Looking for recognition routine for <%s>...\n", imn->description ));

			ITERATELIST( emn, &external_mimetype_list )
			{
				if ( emn->rctx && emn->mimetype && imn->mimetype )
				{
					if ( !stricmp( emn->mimetype, imn->mimetype ) )
					{
						if ( imn->rctx == NULL )
						{
							imn->rctx = emn->rctx;
							imn->flags |= MIMETYPEFLAG_FOREIGNRECOG;
							D(MIMETYPE,bug("Assigned external recognition routine for <%s>\n", imn->mimetype ));
						}

						if ( imn->description == NULL && emn->description )
						{
							imn->description = name_build( emn->description );
							D(MIMETYPE,bug("Assigned external description for <%s>\n", imn->mimetype ));
						}

						break;
					}
				}
			}
		}
	}

	/*
	 * Pass 2: check for full types on external list, and if they meet the requirements
	 * add to internal list.
	 */

	D(MIMETYPE,bug("2:Adding foreign types...\n"));

	emn	= FIRSTNODE( &external_mimetype_list );
	while( NEXTNODE( emn ) )
	{
		APTR next_emn = NEXTNODE( emn );

		/*
		 * We add it even if it has no actions. actions should be looked up from next matching type then.
		 */

		if ( emn->mimetype && emn->rctx /* && emn->action_list && !ISLISTEMPTY( emn->action_list )*/ )
		{

			LONG l = strlen( emn->mimetype );

			/*
			 * Types like picture/ * are ONLY action donors. not added to user lists.
			 */

			if ( emn->mimetype[ l - 1 ] != '*' )
			{

				/*
				 * Check if such type doesn't already exist on internal list
				 */

				LONG found = FALSE;

				D(MIMETYPE,bug("Checking if not a duplicate. <%s>...\n", emn->description ));

				ITERATELIST( imn, &internal_mimetype_list )
				{
					if ( imn->mimetype )
					{
						if ( !stricmp( emn->mimetype, imn->mimetype ) )
						{
							found = TRUE;
							break;
						}
					}
				}

				if ( !found )
				{
					/*
					 * Move it from external list
					 */

					D(MIMETYPE,bug("Adding. <%s> (%ld)...\n", emn->description, emn->priority ));

					REMOVE( emn );
					ADDHEAD( &internal_mimetype_list, emn );	/* XXX: we add to head to make them appear before internal types. should maybe use Enqueue? */
				}
			}
		}

		emn = next_emn;
	}

	/*
	 * Pass 3: if some types don't have actions, then we look for family/ * types
	 * and get actions from them.
	 */

	D(MIMETYPE,bug("3:Assigning generic actions...\n"));

	imn	= FIRSTNODE( &internal_mimetype_list );
	while( NEXTNODE( imn ) )
	{
		APTR next_imn = NEXTNODE( imn );

		if ( imn->action_list && ISLISTEMPTY( imn->action_list ) )
		{
			/*
			 * Check if generic type exists on external list
			 */

			ITERATELIST( emn, &external_mimetype_list )
			{
				if ( emn->mimetype )
				{
					LONG l = strlen( emn->mimetype );

					if ( l > 0 && emn->mimetype[ l - 1 ] == '*' )
					{
						/*
						 * Generic one found, check family (faster to check above one first).
						 */

						if ( !strncmp( imn->mimetype, emn->mimetype, l - 2 ) )
						{
							/*
							 * Assign actions.
							 */

							D(MIMETYPE,bug("Assigning actions from %s to %s\n", emn->mimetype, imn->mimetype ));

							imn->flags |= MIMETYPEFLAG_FOREIGNACTIONS;
							imn->action_list = emn->action_list;
							break;
						}
					}
				}
			}
		}

		imn = next_imn;
	}

	D(MIMETYPE,bug("Done...\n"));

}

static void mimetype_load_user_descriptors(CONST_STRPTR dir)
{
	BPTR dirLock;
	APTR notifyctx = NULL;

	/*
	 * Scan given directory for mimetypes.
	 */

	D(MIMETYPE,bug("Scand dir: <%s>\n", dir));

	if ( (dirLock = Lock(dir, ACCESS_READ)) )
	{
		struct FileInfoBlock *fib;

		if ( (fib = malloc(sizeof(*fib) + PATH_SIZE)) )
		{
			UBYTE *path = (APTR) (fib + 1);

			if ( Examine(dirLock , fib) &&
			     fib->fib_DirEntryType > 0 && /* must be a dir */
			     NameFromLock(dirLock, path, PATH_SIZE - (sizeof(fib->fib_FileName) + 1)) )
			{
				UBYTE *filepos;
				BPTR oldLock;

				/* Prepare for adding filename to path */
				filepos = path + strlen(path);
				if ( filepos > path && filepos[-1] != ':' && filepos[-1] != '/' )
				{
					*filepos++ = '/';
				}

				/*
				 * Fine. We can start examining the entries
				 */

				oldLock  = CurrentDir(dirLock);

				while ( ExNext(dirLock, fib) )
				{
					if ( fib->fib_DirEntryType > 0 )
					{
						/*
						 * Recurse to a directory.
						 */
						D(MIMETYPE,bug("recurse to dir <%s>\n", fib->fib_FileName));
						mimetype_load_user_descriptors(fib->fib_FileName);
					}
					else
					{
						struct internal_mimetype_node *imn;

						/*
						 * Load the file. Unfortunately descriptor_parse_filename()
						 * is lame and wants full path, so arrange for it. If it
						 * didn't we could just use fib->fib_FileName and not need
						 * the NameFromLock() above. - piru
						 */

						strcpy(filepos, fib->fib_FileName);

						D(MIMETYPE,bug("parse file <%s> path <%s>\n", fib->fib_FileName, path));

						imn = descriptor_parse_filename(path);
						if ( imn )
						{
							ULONG l = imn->mimetype ? strlen( imn->mimetype ) : 0;

							D(MIMETYPE,bug("<%s> ok\n", fib->fib_FileName));

							/*
							 * If it's generic type, then we add it to external list (same as types from recognition.db).
							 */

							if ( l && imn->mimetype[ l - 1 ] == '*' )
							{
								ADDTAIL(&external_mimetype_list, imn);
							}
							else
							{
								ADDTAIL(&internal_mimetype_list, imn);
							}

							/*
							 * Setup DOS notification for this file.
							 */

							if ( !notifyctx )
							{
								/* filepos is path-file breakpoint */

								*filepos = '\0';

								notifyctx = dosnotify_start( path, 0 );

								if ( notifyctx )
								{
									struct notifyctx_node *ncn = malloc( sizeof(*ncn) );
									if ( ncn )
									{
										ncn->notifyctx = notifyctx;
										ADDTAIL( &notifyctx_list, ncn );
									}
								}
							}
						}
					}
				}

				(void) CurrentDir(oldLock);
			}

			free(fib);
		}

		UnLock(dirLock);
	}
}

static void mimetype_load_builtin(void)
{
	struct internal_mimetype_node *imn;

	/*
	 *   Relocatable ELF PPC
	 *
	 *   XXX: shall we detect __abox__ or __amigappc__ here? because there could be non-morphos
	 *   	  relocatable elf binaries too (but thats probably over the top) :)
	 */
	if ( (imn = get_mimetypenode(RECOGCMD_NONE, NULL, MIMETYPE_INTERNAL_EXECUTABLE_PPC,
		 GSI( MSG_MIMETYPE_MORPHOSEXECUTABLE ), MIMEACTION_LOADSEG)) )
	{
		APTR rctx;

		if ( (rctx = recog_create()) )
		{
			recog_add(rctx, RECOGCMD_MATCH, "$7f454c4601020100000000000000000000010014");
			recog_add(rctx, RECOGCMD_OR,    NULL);
			recog_add(rctx, RECOGCMD_MATCH, "$7f4d4f5300");
		}
		imn->rctx = rctx;
	}

	/*
	 *   Relocatable HUNK 68k
	 *
	 *   In addition to executable programs this will also match libraries,
	 *   devices, fonts, keymaps and other things using executable file
	 *   format.
	 */
	get_mimetypenode(RECOGCMD_MATCHHUNK, "$??", MIMETYPE_INTERNAL_EXECUTABLE_68K,
		GSI( MSG_MIMETYPE_AMIGAOS68KEXECUTABLE), MIMEACTION_LOADSEG
	);

	#if USE_OS4
	/*
	 *   Executable ELF PPC (OS4).
	 *
	 *   XXX: some OS4 binaries don't have __amigaos4__ symbol; maybe additionally
	 *        matching for VBBC strings required?
	 */
	if ( (imn = get_mimetypenode(RECOGCMD_NONE, NULL, MIMETYPE_INTERNAL_EXECUTABLE_OS4,
		 GSI( MSG_MIMETYPE_AMIGAOS4PPCEXECUTABLE ), MIMEACTION_OS4)) )
	{
		APTR rctx;

		if ( (rctx = recog_create()) )
		{
			recog_add(rctx, RECOGCMD_MATCH,     "$7f454c4601020100000000000000000000020014");
			recog_add(rctx, RECOGCMD_AND,        NULL);
			recog_add(rctx, RECOGCMD_FINDSTRING, "__amigaos4__");
		}
		imn->rctx = rctx;
	}
	#endif

	/*
	 * Scripts
	 */
	if ( (imn = get_mimetypenode(RECOGCMD_NONE, NULL, MIMETYPE_INTERNAL_SCRIPT,
		GSI( MSG_MIMETYPE_SCRIPT ), MIMEACTION_EXECUTE)) )
	{
		APTR rctx;

		if ( (rctx = recog_create()) )
		{
			recog_add(rctx, RECOGCMD_PROTECTION,"+s");
			recog_add(rctx, RECOGCMD_AND,        NULL);
			recog_add(rctx, RECOGCMD_CONTENT,   "TEXT");
		}

		imn->rctx     = rctx;
		imn->priority = 0;
	}

	/*
	 * Ogg Vorbis
	 */
	if ( (imn = get_mimetypenode(RECOGCMD_MATCH, "$4f676753", MIMETYPE_INTERNAL_VORBIS,
		 GSI( MSG_MIMETYPE_OGGVORBIS ), MIMEACTION_PLAYSOUND)) )
	{
		if (imn->rctx)
		{
			recog_sethintpattern(imn->rctx, "#?.ogg");
		}

		imn->priority = -100;
	}

	/*
	 * MP3
	 */
	if ( (imn = get_mimetypenode(RECOGCMD_FILENAME, "*.mp3", MIMETYPE_INTERNAL_MPEGA,
		GSI( MSG_MIMETYPE_MPEGLAYER3 ), MIMEACTION_PLAYSOUND)) )
	{
		if (imn->rctx)
		{
			recog_sethintpattern(imn->rctx, "#?.mp3");
		}
	}

	#if USE_MULTIMEDIA
	/*
	 * Multimedia subsystem.
	 */
	if ( (imn = get_mimetypenode(RECOGCMD_MULTIMEDIA, NULL, MIMETYPE_INTERNAL_MULTIMEDIA,
		GSI( MSG_MIMETYPE_MULTIMEDIAOBJECT ), MIMEACTION_MULTIMEDIA)) )
	{
		imn->priority = -1023;
	//	recog_add(imn->rctx, RECOGCMD_MULTIMEDIA, NULL);
	}
	#endif

	/*
	 * Directory.
	 */
	if ( (imn = get_mimetypenode(RECOGCMD_NONE, NULL, MIMETYPE_INTERNAL_DIRECTORY,
		GSI( MSG_MIMETYPE_DIRECTORY ), MIMEACTION_DATATYPES)) )
	{
		APTR an, rctx;

		imn->priority = 512;	/* We give directory high priority. */

		if ( (rctx = recog_create()) )
		{
			recog_add(rctx, RECOGCMD_DIRECTORY, NULL);
			recog_add(rctx, RECOGCMD_OR, NULL);
			recog_add(rctx, RECOGCMD_DEVICE, NULL);
		}
		/* XXX */
		imn->rctx = rctx;

		/*
		 * Actions.
		 */
		if ( (an = actionnode_create()) )
		{
			actionnode_setattrs( an,
				ACTIONNODETAG_EVENT, ACTION_EVENT_DOUBLECLICK,
				ACTIONNODETAG_QUALIFIER, ACTION_QUALIFIER_NONE,
				ACTIONNODETAG_NAME, GSI( MSG_CMENU_MIME_OPEN ),
				TAG_DONE );

			actionnode_addcommand( an, AC_INTERNAL, "LoadURI %s VIEWID %Si" );

			ADDTAIL( imn->action_list, an );
		}

		if ( (an = actionnode_create()) )
		{
			actionnode_setattrs( an,
				ACTIONNODETAG_EVENT, ACTION_EVENT_DOUBLECLICK,
				ACTIONNODETAG_QUALIFIER, ACTION_QUALIFIER_NONE,
				ACTIONNODETAG_NAME, GSI( MSG_CMENU_MIME_OPEN ),
				ACTIONNODETAG_FLAGS, ACTION_FLAG_SPATIALMODE,		/* same action (criteria) as above but for spatial */
				TAG_DONE );

			actionnode_addcommand( an, AC_INTERNAL, "LoadURI %s VIEWID %Si" ); /* View will decide itself if NEWWIN is preferred */

			ADDTAIL( imn->action_list, an );
		}


		if ( (an = actionnode_create()) )
		{
			actionnode_setattrs( an,
				ACTIONNODETAG_EVENT, ACTION_EVENT_DOUBLECLICK,
				ACTIONNODETAG_QUALIFIER, ACTION_QUALIFIER_ALT,
				ACTIONNODETAG_NAME, GSI( MSG_CMENU_MIME_OPENINNEWWINDOW ),
				TAG_DONE );

			actionnode_addcommand( an, AC_INTERNAL, "LoadURI %s NEWWIN" );

			ADDTAIL( imn->action_list, an );
		}

		if ( (an = actionnode_create()) )
		{
			actionnode_setattrs( an,
				ACTIONNODETAG_EVENT, ACTION_EVENT_DOUBLECLICK,
				ACTIONNODETAG_QUALIFIER, ACTION_QUALIFIER_ALT,
				ACTIONNODETAG_NAME, GSI( MSG_CMENU_MIME_OPENANDCLOSEOLD ),
				ACTIONNODETAG_FLAGS, ACTION_FLAG_SPATIALMODE,       /* same action (criteria) as above but for spatial */
				TAG_DONE );

			actionnode_addcommand( an, AC_INTERNAL, "LoadURI %s" );
			actionnode_addcommand( an, AC_INTERNAL, "ViewClose %Si" );

			ADDTAIL( imn->action_list, an );
		}

/* Clashes with multiselection, disabled */
/*
		if ( (an = actionnode_create()) )
		{
			actionnode_setattrs( an,
				ACTIONNODETAG_EVENT, ACTION_EVENT_DRAGNDROP,
				ACTIONNODETAG_QUALIFIER, ACTION_QUALIFIER_SHIFT,
				ACTIONNODETAG_NAME, GSI( MSG_CMENU_MIME_OPENINDESTINATION ),
				TAG_DONE );

			actionnode_addcommand( an, AC_INTERNAL, "LoadURI %s VIEWID %Di" );

			ADDTAIL( imn->action_list, an );
		}
*/
		if ( (an = actionnode_create()) )
		{
			actionnode_setattrs( an,
				ACTIONNODETAG_EVENT, ACTION_EVENT_MENU,
				ACTIONNODETAG_QUALIFIER, ACTION_QUALIFIER_NONE,
				ACTIONNODETAG_NAME, GSI( MSG_CMENU_MIME_OPENSHELL ),
				ACTIONNODETAG_FLAGS, ACTION_FLAG_ASYNC,
				TAG_DONE );

            actionnode_addcommand( an, AC_SCRIPT, "MOSSYS:Ambient/scripts/openshellindir %sp" );

			ADDTAIL( imn->action_list, an );
		}

		if ( (an = actionnode_create()) )
		{
			actionnode_setattrs( an,
				ACTIONNODETAG_EVENT, ACTION_EVENT_MENU,
				ACTIONNODETAG_QUALIFIER, ACTION_QUALIFIER_NONE,
				ACTIONNODETAG_NAME, GSI( MSG_CMENU_MIME_FIND ),
				ACTIONNODETAG_FLAGS, ACTION_FLAG_ASYNC | ACTION_FLAG_MULTIPLE,
				TAG_DONE );

			actionnode_addcommand( an, AC_INTERNAL, "Find LOCATION %sp" );

			ADDTAIL( imn->action_list, an );
		}
	}

	/*
	 * Datatypes. (last - lowest priority)
	 */

	if ( (imn = get_mimetypenode(RECOGCMD_NONE, NULL, MIMETYPE_INTERNAL_DATATYPES,
				(_aprefs(nodatatypesmimename) ? "" : (const char *)GSI(MSG_MIMETYPE_DATATYPE)), MIMEACTION_DATATYPES)) )
	{
		APTR an, rctx;

		imn->priority = -1024;	/* that one really should be last one */

		if ( (rctx = recog_create()) )
		{
			recog_add(rctx, RECOGCMD_DATATYPES, NULL);
			recog_add(rctx, RECOGCMD_OR, NULL);
			recog_add(rctx, RECOGCMD_DIRECTORY, NULL);	/* XXX: Datatypes subsystem doesn't recognize directories? */
			recog_add(rctx, RECOGCMD_OR, NULL);
			recog_add(rctx, RECOGCMD_FILENAME, "#?" );	/* XXX: Maybe include just this one for this step? */
		}
		/* XXX */
		imn->rctx = rctx;

		/*
		 * Actions.
		 */
		if ( ( an = actionnode_create() ) )
		{
			actionnode_setattrs( an,
				ACTIONNODETAG_EVENT, ACTION_EVENT_DOUBLECLICK,
				ACTIONNODETAG_QUALIFIER, ACTION_QUALIFIER_NONE,
				ACTIONNODETAG_NAME, GSI( MSG_CMENU_MIME_OPEN ),
				TAG_DONE );

			/* wait for complete commit...*/
			//actionnode_addcommand( an, AC_INTERNAL, "LoadURI %s VIEWID %Si" );
			actionnode_addcommand( an, AC_INTERNAL, "LoadURI %s" );

			ADDTAIL( imn->action_list, an );
		}

		if(!_conf(misc_smartfileoperations))
		{
			if ( (an = actionnode_create()) )
			{
				actionnode_setattrs( an,
					ACTIONNODETAG_EVENT, ACTION_EVENT_DRAGNDROP,
					ACTIONNODETAG_QUALIFIER, ACTION_QUALIFIER_NONE,
					ACTIONNODETAG_NAME, GSI( MSG_CMENU_MIME_COPYTO ),
					ACTIONNODETAG_FLAGS, ACTION_FLAG_MULTIPLE,
					TAG_DONE );

				actionnode_addcommand( an, AC_INTERNAL, "Copy TO %D FROM %sp" );

				ADDTAIL( imn->action_list, an );
			}

			if ( (an = actionnode_create()) )
			{
				actionnode_setattrs( an,
					ACTIONNODETAG_EVENT, ACTION_EVENT_DRAGNDROP,
					ACTIONNODETAG_QUALIFIER, ACTION_QUALIFIER_ALT,
					ACTIONNODETAG_NAME, GSI( MSG_CMENU_MIME_MOVETO ),
					ACTIONNODETAG_FLAGS, ACTION_FLAG_MULTIPLE,
					TAG_DONE );

				actionnode_addcommand( an, AC_INTERNAL, "Move TO %D FROM %sp" );

				ADDTAIL( imn->action_list, an );
			}

			if ( (an = actionnode_create()) )
			{
				actionnode_setattrs( an,
					ACTIONNODETAG_EVENT, ACTION_EVENT_DRAGNDROP,
					ACTIONNODETAG_QUALIFIER, ACTION_QUALIFIER_SHIFT,
					ACTIONNODETAG_NAME, GSI( MSG_CMENU_MIME_MOVETO ),
					ACTIONNODETAG_FLAGS, ACTION_FLAG_MULTIPLE,
					TAG_DONE );

				actionnode_addcommand( an, AC_INTERNAL, "Copy TO %D FROM %sp" );

				ADDTAIL( imn->action_list, an );
			}

			if ( (an = actionnode_create()) )
			{
				actionnode_setattrs( an,
					ACTIONNODETAG_EVENT, ACTION_EVENT_DRAGNDROP,
					ACTIONNODETAG_QUALIFIER, ACTION_QUALIFIER_ALT | ACTION_QUALIFIER_SHIFT,
					ACTIONNODETAG_NAME, GSI( MSG_CMENU_MIME_MOVETO ),
					ACTIONNODETAG_FLAGS, ACTION_FLAG_MULTIPLE,
					TAG_DONE );

				actionnode_addcommand( an, AC_INTERNAL, "Move TO %D FROM %sp" );

				ADDTAIL( imn->action_list, an );
			}

		}
		else
		{
			if ( (an = actionnode_create()) )
			{
				actionnode_setattrs( an,
					ACTIONNODETAG_EVENT, ACTION_EVENT_DRAGNDROP,
					ACTIONNODETAG_QUALIFIER, ACTION_QUALIFIER_NONE,
					ACTIONNODETAG_NAME, GSI( MSG_CMENU_MIME_COPYTO ),
					ACTIONNODETAG_FLAGS, ACTION_FLAG_MULTIPLE,
					TAG_DONE );

				actionnode_addcommand( an, AC_INTERNAL, "DropFile TO %D FROM %sp" );

				ADDTAIL( imn->action_list, an );
			}

			if ( (an = actionnode_create()) )
			{
				actionnode_setattrs( an,
					ACTIONNODETAG_EVENT, ACTION_EVENT_DRAGNDROP,
					ACTIONNODETAG_QUALIFIER, ACTION_QUALIFIER_ALT,
					ACTIONNODETAG_NAME, GSI( MSG_CMENU_MIME_COPYTO ),
					ACTIONNODETAG_FLAGS, ACTION_FLAG_MULTIPLE,
					TAG_DONE );

				actionnode_addcommand( an, AC_INTERNAL, "DropFile TO %D INVERT FROM %sp" );

				ADDTAIL( imn->action_list, an );
			}

			if ( (an = actionnode_create()) )
			{
				actionnode_setattrs( an,
					ACTIONNODETAG_EVENT, ACTION_EVENT_DRAGNDROP,
					ACTIONNODETAG_QUALIFIER, ACTION_QUALIFIER_SHIFT,
					ACTIONNODETAG_NAME, GSI( MSG_CMENU_MIME_COPYTO ),
					ACTIONNODETAG_FLAGS, ACTION_FLAG_MULTIPLE,
					TAG_DONE );

				actionnode_addcommand( an, AC_INTERNAL, "DropFile TO %D FROM %sp" );

				ADDTAIL( imn->action_list, an );
			}

			if ( (an = actionnode_create()) )
			{
				actionnode_setattrs( an,
					ACTIONNODETAG_EVENT, ACTION_EVENT_DRAGNDROP,
					ACTIONNODETAG_QUALIFIER, ACTION_QUALIFIER_SHIFT | ACTION_QUALIFIER_ALT,
					ACTIONNODETAG_NAME, GSI( MSG_CMENU_MIME_COPYTO ),
					ACTIONNODETAG_FLAGS, ACTION_FLAG_MULTIPLE,
					TAG_DONE );

				actionnode_addcommand( an, AC_INTERNAL, "DropFile TO %D INVERT FROM %sp" );

				ADDTAIL( imn->action_list, an );
			}

		}
	}
}

static void mimetype_load_recognitiondb( STRPTR filename )
{
	BPTR fhandle;

	/*
	 * recognition.db
	 */

	if (!filename)
	{
		filename = "MOSSYS:Ambient/" RECOGNITION_FILE;
	}

	fhandle	= Open( filename , MODE_OLDFILE );

	if ( fhandle )
	{
		struct internal_mimetype_node *ern;

		D(MIMETYPE,bug("Scanning recognition database <%s>\n", filename ));

		do
		{
			ern = descriptor_parse( NULL, fhandle );

			if ( ern )
			{
				//struct recog_ctx *rctx; // bitRocky: maybe it will be used later?

				D(MIMETYPE,bug("New regonition pattern <%s>\n", ern->description ));
				ADDTAIL(&external_mimetype_list, ern);

				/* Mark it as external one, so won't be deleted when assigned to other imn XXX: make some function to set it */

				//rctx = ern->rctx;

				ern->flags |= MIMETYPEFLAG_FOREIGN;
			}
		}
		while( ern );

		Close( fhandle );


	}
	else
	{
		D(MIMETYPE,bug("No external recognition database found <%s>\n", filename ));
	}

	/*   Setup DOS notification for this file, even if it doesn't exists yet.
	 *
	 *   XXX: Maybe better if recognition.db uses an own global ctx again (instead
	 *   beeing part of the list for single filetype notifyctxs)? Makes not much sense
	 *   to delete and recreate the notify all the time, IMHO. (tokai)
	 */
	{
		APTR notifyctx = dosnotify_start( filename , 0 );

		if ( notifyctx )
		{
			struct notifyctx_node *ncn = malloc( sizeof(*ncn) );
			if ( ncn )
			{
				ncn->notifyctx = notifyctx;
				ADDTAIL( &notifyctx_list, ncn );
			}
		}
	}
}

static void mimetype_reinitialize(void);

void mimetype_load_database(CONST_STRPTR filename UNUSED)
{
	struct internal_mimetype_node *imn;
#ifdef DEBUG
	ULONG time0 = timedm();
#endif

	/*
	 * delete all internal nodes, external nodes and notify contexts.
	 */

	mimetype_reinitialize();

	/*
	 * Build new list.
	 */

	mimetype_load_user_descriptors( PREFS_PATH "filetypes/" );

	mimetype_load_recognitiondb( NULL );

	merge_types_lists();

	mimetype_load_builtin();

	sort_types_list();

	/*
	 * Final step. We call actionnode_setup() for all actions.
	 * easier to do here. less code.
	 */

	ITERATELIST( imn, &internal_mimetype_list )
	{
		APTR an;

		if ( !ISLISTEMPTY( imn->action_list ) )
		{
			ITERATELIST( an, imn->action_list )
			{
				actionnode_setup( an );
			}
		}
	}

	DB(("Mimetype loading time:%f\n", (float)(timedm() - time0) / 1000.0f ));

}

ULONG mimetype_init(void)
{
	NEWLIST(&notifyctx_list);
	NEWLIST(&internal_mimetype_list);
	NEWLIST(&external_mimetype_list);

	return (TRUE);
}


static void mimetype_free_listnodes(void)
{
	struct internal_mimetype_node *imn, *nextimn;
	struct notifyctx_node *ncn, *nextncn;

	ITERATELISTSAFE(imn, nextimn, &internal_mimetype_list)
	{
		imn_delete( imn );
	}

	ITERATELISTSAFE(imn, nextimn, &external_mimetype_list)
	{
		imn_delete( imn );
	}

	ITERATELISTSAFE(ncn, nextncn, &notifyctx_list)
	{
		dosnotify_stop( ncn->notifyctx );
		free( ncn );
	}
}


static void mimetype_reinitialize(void)
{
	mimetype_free_listnodes();

	NEWLIST(&notifyctx_list);
	NEWLIST(&internal_mimetype_list);
	NEWLIST(&external_mimetype_list);
}


void mimetype_cleanup(void)
{
	mimetype_free_listnodes();

	/* no need to NEWLIST here */
}


static ULONG mimetype_recog_internal(struct mimetype_ctx *ct, APTR rfh, ULONG fileio)
{
	struct internal_mimetype_node *imn;

	ITERATELIST(imn, &internal_mimetype_list)
	{
		D(RECOG,bug("examining:%s\n", imn->description));
		D(RECOG,bug("proding entry <%s>..\n", imn->mimetype));
		if ( imn->rctx && recog_prod( rfh, imn->rctx, fileio ) == RECOG_TRUE )
		{
			ULONG matched = FALSE;
			APTR an;

			D(RECOG,bug("recognized, checking default action\n"));

			/*
			 * This function is the one called by mimeuri_gather. It lookups type to
			 * get default action from it. We make sure returned type has default action to avoid
			 * infinite looping (no default->execute loaduri->nodefault...). Experimental. Could
			 * break things. NOTE: Internal types (flags & MIMETYPEFLAG_INTERNAL) don't have actions!
			 */

			if ( imn->flags & MIMETYPEFLAG_INTERNAL )
			{
				matched = TRUE;
			}
			else if ( !ISLISTEMPTY( imn->action_list ) )
			{
				ITERATELIST(an, imn->action_list)
				{
					if ( (ULONG)actionnode_getattr( an, ACTIONNODETAG_EVENT ) == ACTION_EVENT_DOUBLECLICK )
					{
						matched = TRUE;
						break;
					}
				}
			}

			if ( matched )
			{

				mimetype_setattrs(ct,
					MIMETYPETAG_MIME, imn->mimetype,
					MIMETYPETAG_DESCRIPTION, imn->description,
					MIMETYPETAG_ACTION, imn->action,
					MIMETYPETAG_NEWWIN, imn->newwin,
				TAG_DONE);

				if (imn->action == MIMEACTION_MULTIMEDIA || imn->action == MIMEACTION_DATATYPES)
				{
					TEXT t[256]; /* should be enough */
					STRPTR s = s; /* shut up gcc */
					ULONG userdata;

					userdata = recog_getfhattr(rfh, RECOGFHATTR_USERDATA);

					/* XXX: there should be some generic way for that but well.. */
					switch (imn->action)
					{
						case MIMEACTION_MULTIMEDIA:
							s = multimedia_nametype(userdata);
							break;

						case MIMEACTION_DATATYPES:
							s = datatypes_nametype(userdata);
							break;
					}

					snprintf(t, sizeof(t), "%s (%s)", imn->description, s);

					mimetype_setattrs(ct,
						MIMETYPETAG_SUBTYPE, userdata,
						MIMETYPETAG_DESCRIPTION, t,
					TAG_DONE);
				}
				return (TRUE);
			}
		}
		else
		{
			D(RECOG,bug("Failed\n"));
		}
		/* XXX: handle the recog errors.. */
	}
	return (FALSE);
}

APTR mimetype_find(CONST_STRPTR scheme, CONST_STRPTR path, ULONG flags)
{
	return mimetype_find_pattern( scheme, path, flags, NULL );
}

APTR mimetype_find_pattern(CONST_STRPTR scheme UNUSED, CONST_STRPTR path, ULONG flags, CONST_STRPTR mimetypepattern)
{
	struct internal_mimetype_node *imn;

	ULONG fast = TRUE;
	ULONG fileio = flags & MTF_FILEIO;
	CONST_STRPTR path_file = FilePart( path );

	#ifdef DEBUG
	ULONG checked = 0;
	ULONG skipped = 0;
	#endif

	APTR rfh = recog_open(path);

	if ( !rfh )
	{
		D(RECOG,bug("failed to open <%s>. Disabling FILEIO\n", path ));

		#if 0
			/*   XXX:  the whole fileio=FALSE/TRUE handling between mimetype.c and recog.c
			 *         makes zero sense currently. All recogcmd_#? functions need the result of
			 *         recog_open(), so we return NULL and fail here for now until the one who
			 *         thought about fileio=FALSE/TRUE handling initially finishes
			 *         to implement his idea (or document it, so others can) -- tokai
			 *
			 *   INFO: currently fileio is set to FALSE in recog_prod when there is no
			 *         filehandle.
			 */
			fileio = FALSE;
		#else
			return NULL;
		#endif
	}

	imn = FIRSTNODE( &internal_mimetype_list );
	while( NEXTNODE(imn) )
	{
		ULONG namematch = TRUE;

		/* filter by name pattern */

		if ( mimetypepattern != NULL && imn->mimetype != NULL && name_match( imn->mimetype, mimetypepattern ) == FALSE )
			namematch = FALSE;

		/* in fast pass we only check types which have hinting pattern */

		if ( namematch && imn->rctx != NULL )
		{
			if ( fast == FALSE || recog_checkhint( imn->rctx, path_file ) )
			{
				/*
				 * Check if it matches.
				 */

				D(RECOG,bug("proding entry <%s>..\n", imn->mimetype));

				#ifdef DEBUG
				checked++;
				#endif

				if ( recog_prod( rfh, imn->rctx, fileio ) == RECOG_TRUE )
				{
					D(RECOG,bug("recognized(fast pass:%s, checked:%d, skipped:%d)\n", fast ? "Yes" : "No", checked, skipped));

					/*
					 * Return node.
					 */

					if ( rfh )
						recog_close(rfh);

					return imn;
				}
				/* XXX: handle the recog errors.. */
			}
			#ifdef DEBUG
			else
			{
				skipped++;
			}
			#endif
		}

		imn = NEXTNODE( imn );

		if ( fast && !NEXTNODE(imn) )
		{
			/* second pass will check all nodes */

			fast = FALSE;
			imn = FIRSTNODE( &internal_mimetype_list );
			D(RECOG,bug("Starting second pass\n"));

		}
	}

	if ( rfh )
		recog_close(rfh);

	return (NULL);
}

APTR mimetype_findaction(CONST_STRPTR scheme UNUSED, CONST_STRPTR path, ULONG event, ULONG qualifier, ULONG flags)
{
	struct internal_mimetype_node *imn;

	ULONG fast = TRUE;
	ULONG fileio = flags & MTF_FILEIO;
	ULONG spatial = _conf( toolbar_browsermode ) ? 0 : ACTION_FLAG_SPATIALMODE;

	APTR rfh = recog_open(path);

	if ( !rfh )
	{
		D(RECOG,bug("failed to open <%s>. Disabling FILEIO\n", path ));
		fileio = FALSE;
	}

	imn = FIRSTNODE( &internal_mimetype_list );
	while( NEXTNODE(imn) )
	{
		APTR an;
		APTR action = NULL;
		APTR def_action = NULL;
		ULONG hasevent = FALSE;

		/* in fast pass we only check types which have hinting pattern */

		if ( fast == FALSE || ( imn->rctx && recog_checkhint( imn->rctx, path ) ) )
		{
			D(RECOG,bug("proding entry 0x%x<%s>..\n",imn, imn->mimetype));

			/*
			 * Before examining mimetype, we check if it contains event we need.
			 */

			if ( !ISLISTEMPTY( imn->action_list ) )
			{
				ITERATELIST(an, imn->action_list)
				{
					D(RECOG,bug("action:%s (event:%d)\n", actionnode_getattr( an, ACTIONNODETAG_NAME ), actionnode_getattr( an, ACTIONNODETAG_EVENT ) ) );
					if ( (ULONG)actionnode_getattr( an, ACTIONNODETAG_EVENT ) == event )
					{
						#if 0
						if ( (ULONG)actionnode_getattr( an, ACTIONNODETAG_QUALIFIER ) == ACTION_QUALIFIER_NONE )
							def_action = action;
						#endif

						if ( (ULONG)actionnode_getattr( an, ACTIONNODETAG_QUALIFIER ) == qualifier )
						{
							hasevent = TRUE;
							action = an;

							/* check if we have exact match also for spatial-mode flag */

							if ( spatial == ( (ULONG)actionnode_getattr( an, ACTIONNODETAG_FLAGS ) & ACTION_FLAG_SPATIALMODE ) )
							{
								break;
							}
						}
					}
				}
			}

			D(RECOG,bug("found matching event:%s\n", hasevent ? "yes" : "no" ));

			if ( hasevent )
			{
				/*
				 * Check if it matches.
				 */

				if ( imn->rctx && recog_prod( rfh, imn->rctx, fileio ) == RECOG_TRUE )
				{
					D(RECOG,bug("recognized(fast pass:%s)\n", fast ? "Yes" : "No"));

					/*
					 * Return action.
					 */

					if ( rfh )
						recog_close(rfh);

					return action ? action : def_action;
				}
			}
			/* XXX: handle the recog errors.. */
		}

		imn = NEXTNODE( imn );

		if ( fast && !NEXTNODE(imn) )
		{
			/* second pass will check all nodes */

			fast = FALSE;
			imn = FIRSTNODE( &internal_mimetype_list );
			D(RECOG,bug("Starting second pass\n"));
		}
	}

	if ( rfh )
		recog_close(rfh);

	return (NULL);
}


static ULONG mimetype_recog_external(struct mimetype_ctx *ct UNUSED, APTR rfh UNUSED, ULONG fileio UNUSED)
{
	mimeprefs_lock_shared();
	/* XXX: we need a semaphore to prevent a prefs transfer while we work on them */

	/* XXX: have to really figure out that recog language.. sigh. recog_add() doesn't fit too.. sigh */

	mimeprefs_unlock();

	return (TRUE); /* XXX */
}


void v_mimetype_setattrs(APTR ctx, struct TagItem *tags)
{
	struct mimetype_ctx *ct = ctx;

	ASSERT(ct);

	FORTAG(tags)
	{
		case MIMETYPETAG_MIME:
			ct->mediatype = MEDIATYPE_UNKNOWN;

			if (ct->mimetype)
			{
				free(ct->mimetype);
			}
			if ((ct->mimetype = malloc(strlen((STRPTR)tag->ti_Data) + 1)) )
			{
				const struct mimetype_mediatype *mtmt = mimetype_mt;
				strcpy(ct->mimetype, (STRPTR)tag->ti_Data);

				while (mtmt->name)
				{
					if (!strncmp(mtmt->name, (STRPTR)tag->ti_Data, strlen(mtmt->name)))
					{
						ct->mediatype = mtmt->type;
						break;
					}
					mtmt++;
				}
			}
			break;

		case MIMETYPETAG_ACTION:
			ct->action = tag->ti_Data;
			break;

		case MIMETYPETAG_NEWWIN:
			ct->newwin = tag->ti_Data;
			break;

		case MIMETYPETAG_SUBTYPE:
			ct->subtype = tag->ti_Data;
			break;

		case MIMETYPETAG_DESCRIPTION:
			if (ct->description)
			{
				free(ct->description);
			}
			if ( (ct->description = malloc(strlen((STRPTR)tag->ti_Data) + 1)) )
			{
				strcpy(ct->description, (STRPTR)tag->ti_Data);
			}
			break;

		case MIMETYPETAG_FILEINFO:
			ct->fileinfo = tag->ti_Data;
			break;

		case MIMETYPETAG_SECONDS:
			ct->seconds = tag->ti_Data;
			break;

		case MIMETYPETAG_FILESIZE:
			ct->filesize = tag->ti_Data;
			break;

		case MIMETYPETAG_FILESIZEPTR:
			ct->filesize = *(UQUAD*)(tag->ti_Data);
			break;

		#ifdef DEBUG
		default:
			PDB(("unknown tag 0x%lx\n", tag->ti_Data));
			break;
		#endif

	}
	NEXTTAG
}


APTR mimetype_getattr(APTR ctx, ULONG attr)
{
	struct mimetype_ctx *ct = ctx;

	ASSERT(ct);

	switch (attr)
	{
		case MIMETYPETAG_MEDIA:
			return ((APTR)ct->mediatype);

		case MIMETYPETAG_MIME:
			return (ct->mimetype);

		case MIMETYPETAG_ACTION:
			return ((APTR)ct->action);

		case MIMETYPETAG_NEWWIN:
			return ((APTR)ct->newwin);

		case MIMETYPETAG_SUBTYPE:
			return ((APTR)ct->subtype);

		case MIMETYPETAG_DESCRIPTION:
			return (ct->description);

		case MIMETYPETAG_FILEINFO:
			return ((APTR)ct->fileinfo);

		case MIMETYPETAG_SECONDS:
			return ((APTR)ct->seconds);

		case MIMETYPETAG_FILESIZE:
			return ((APTR)(ULONG)ct->filesize);

		case MIMETYPETAG_FILESIZEPTR:
			return ((APTR)&ct->filesize);

		#ifdef DEBUG
		default:
			PDB(("unknown tag 0x%lx\n", attr));
			break;
		#endif
	}
	return (NULL);
}


APTR mimetype_create(CONST_STRPTR scheme, CONST_STRPTR path, ULONG flags)
{
	struct mimetype_ctx *ct;

	THREAD;

	if ( (ct = malloc(sizeof(*ct))) )
	{
		memset(ct, 0, sizeof(*ct));

		/* run make printhash then printhash <string> to find out the values (which aren't really useful for 4 byte strings) */
		switch (hash(scheme))
		{
			case 0x7c7902e3: /* "root" */
				mimetype_setattrs(ct,
					MIMETYPETAG_MIME, MIMETYPE_INTERNAL_ROOTVIEW,
					MIMETYPETAG_ACTION, MIMEACTION_VIEW,
					MIMETYPETAG_NEWWIN, FALSE,
				TAG_DONE);
				break;

			case 0x9e0bfaae: /* devices */
				mimetype_setattrs(ct,
					MIMETYPETAG_MIME, MIMETYPE_INTERNAL_VOLUMES,
					MIMETYPETAG_ACTION, MIMEACTION_VIEW,
					MIMETYPETAG_NEWWIN, FALSE,
				TAG_DONE);
				break;

			case 0xb877a86: /* vfs */
				mimetype_setattrs(ct,
					MIMETYPETAG_MIME, MIMETYPE_INTERNAL_VFS,
					MIMETYPETAG_ACTION, MIMEACTION_VIEW,
					MIMETYPETAG_NEWWIN, FALSE,
				TAG_DONE);
				break;

			case 0x7c6e34c3: /* "file" */
				if (path)
				{
					struct fileinfo64 fi;
					LONG ex;

					/*
					 * Date, time and size.
					 */
					ex = examine64(path, &fi);

					if (ex)
					{
						mimetype_setattrs(ct,
							MIMETYPETAG_FILEINFO, TRUE,
							MIMETYPETAG_SECONDS, datestamp_to_seconds(&fi.fi_Date),
							MIMETYPETAG_FILESIZEPTR, &fi.fi_Size,
							TAG_DONE
						);
					}

					if (ex && (fi.fi_Type > 0)) /* XXX: softlinks.. */
					{
						/*
						 * This is a directory.
						 */

						if (flags & MTF_RECURSE)
						{
							ULONG rc;

							if (isdevicename(path))
							{
								struct device_information di;

								rc = device_get_information( path, &di );
								if (!rc)
								{
									/* If we can't get info display 0 */
									di.used = 0LL;
								}

								mimetype_setattrs(ct,
									MIMETYPETAG_FILESIZEPTR, &di.used,
									TAG_DONE
								);
							}
							else
							{
								struct dirsize ds;

								rc = getdirsize( path, &ds );
								if (!rc)
								{
									/* If we can't get info display 0 */
									ds.totalsize = 0LL;
								}

								mimetype_setattrs(ct,
									MIMETYPETAG_FILESIZEPTR, &ds.totalsize,
									TAG_DONE
								);
							}
						}

						mimetype_setattrs(ct,
							MIMETYPETAG_MIME, MIMETYPE_INTERNAL_DIRECTORY,
							MIMETYPETAG_DESCRIPTION, isdevicename( path ) ? GSI( MSG_MIMETYPE_DEVICE ) : GSI( MSG_MIMETYPE_DIRECTORY ),
							MIMETYPETAG_ACTION, MIMEACTION_VIEW,
							MIMETYPETAG_NEWWIN, FALSE,				/* XXX: For now force dirs to use same window */
						TAG_DONE);
					}
					else
					{
						STRPTR iconname;
						ULONG is_project = FALSE;

						if (!(flags & MTF_NETWORK))
						{
							struct devinfo64 di;

							if (info64(path, &di))
							{
								if (di.di_DeviceType == DG_COMMUNICATION)
								{
									flags &= ~MTF_FILEIO;
								}
							}
						}

						if ( (iconname = name_build_info(path)) )
						{
							BPTR l;

							if ( (l = Lock(iconname, ACCESS_READ)) )
							{
								APTR o;

								if ( (o = (APTR)methodstack_push_sync(app, 3, MM_Application_CreateIcon, FALSE, MV_ViewID_Unknown)) )
								{
									if (icon_read(iconname, o, ICONTAG_GetImage, FALSE, TAG_DONE))
									{
										STRPTR s;

										methodstack_push_sync(o, 3,
											OM_GET, MA_Icon_DefaultTool, &s
										);

										if (s)
										{
											/*
											 * Trap 'multiview' and 'more'.
											 */
											s = FilePart(s);

											if ((!_conf(misc_trapmultiview) || stricmp("multiview",s)) &&
												(!_conf(misc_trapmore)      || stricmp("more",     s)))
											{
												/*
												 * This is a project.
												 */
												mimetype_setattrs(ct,
													MIMETYPETAG_MIME, MIMETYPE_INTERNAL_PROJECT,
													MIMETYPETAG_DESCRIPTION, GSI( MSG_MIMETYPE_PROJECT ),
													MIMETYPETAG_ACTION, MIMEACTION_LOADSEG,
													MIMETYPETAG_NEWWIN, FALSE,
												TAG_DONE);
												is_project = TRUE;
											}
										}
									}
									methodstack_push(o, 1, OM_RELEASE);
								}

								UnLock(l);
							}
							name_delete(iconname);
							/* XXX: report error if anything fails.. */
						}

						if (!is_project && ex)
						{
							APTR rh;

							/*
							 * Try to recognize it through
							 * mimes then.
							 */
							if ( (rh = recog_open(path)) )
							{
								if (!mimetype_recog_internal(ct, rh, flags & MTF_FILEIO))
								{
									mimetype_recog_external(ct, rh, flags & MTF_FILEIO);
								}
								recog_close(rh);
							}
						}
					}
				}
				break;
		}
	}
	return (ct);
}

APTR mimetype_find_by_description(CONST_STRPTR description)
{
	struct internal_mimetype_node *imn;

	ITERATELIST( imn, &internal_mimetype_list )
	{
		if ( !stricmp( imn->description, description ) )
			return (APTR)imn;
	}

	return NULL;
}

APTR mimetype_find_by_mimetype(CONST_STRPTR mimetype)
{
		struct internal_mimetype_node *imn;

		ITERATELIST( imn, &internal_mimetype_list )
		{
			if ( !stricmp( imn->mimetype, mimetype ) )
				return (APTR)imn;
		}

	return NULL;
}

APTR mimetype_find_foreign_actions_by_mimetype(CONST_STRPTR mimetype)
{
	struct internal_mimetype_node *imn;
	LONG l = strlen( mimetype );

	if ( l > 1 && mimetype[ l - 1 ] == '*' )
	{
		ITERATELIST( imn, &internal_mimetype_list )
		{
			if ( !strncmp( imn->mimetype, mimetype, l - 2 ) && (imn->flags & MIMETYPEFLAG_FOREIGNACTIONS))
				return (APTR)imn->action_list;
		}
	}

	return NULL;
}

APTR mimetype_find_generic_by_mimetype(CONST_STRPTR mimetype)
{
	struct internal_mimetype_node *imn;

	ITERATELIST( imn, &external_mimetype_list )
	{
		if ( !stricmp( imn->mimetype, mimetype ) )
			return (APTR)imn;
	}

	return NULL;
}

void mimetype_clear_actions(CONST_STRPTR mimetype)
{
	struct internal_mimetype_node *imn;

	ITERATELIST( imn, &internal_mimetype_list )
	{
		if ( !stricmp( imn->mimetype, mimetype ) )
		{
			/* XXX: is it safe to delete them ? */
			if(!ISLISTEMPTY(&imn->internal_action_list))
			{
				APTR an, nextan;

				ITERATELISTSAFE(an, nextan, &imn->internal_action_list)
				{
					actionnode_delete(an);
				}
				NEWLIST(&imn->internal_action_list);
			}

			imn->action_list = &imn->internal_action_list;
			imn->flags &= ~MIMETYPEFLAG_FOREIGNACTIONS;
		}
	}
}

ULONG mimetype_checkpath(APTR mimetype, CONST_STRPTR path)
{
	struct internal_mimetype_node *imn = mimetype;
	APTR rfh = recog_open(path);
	ULONG fileio = TRUE;
	ULONG rc = FALSE;

	if ( !rfh )
	{
		D(RECOG,bug("failed to open <%s>. Disabling FILEIO\n", path ));
		fileio = FALSE;
	}

	if ( imn->rctx && recog_prod( rfh, imn->rctx, fileio ) == RECOG_TRUE )
	{
		rc = TRUE;
	}

	if ( rfh )
		recog_close( rfh );

	return rc;
}

APTR mimetype_duplicate(APTR ctx)
{
	struct mimetype_ctx *ct = ctx;
	struct mimetype_ctx *out;
	
	if (!ctx)
		return NULL;
	
	out = malloc(sizeof(*out));
	if (out)
	{
		ULONG length;
		memcpy(out, ct, sizeof(*out));
		out->mimetype = out->description = NULL;

#define MIME_STRCOPY(__field__) \
	if (ct->__field__) { \
	length = strlen(ct->__field__); \
	out->__field__ = malloc(length + 1); \
	if (!out->__field__) { mimetype_delete(out); return NULL; } \
	memcpy(out->__field__, ct->__field__, length + 1); } else { \
	out->__field__ = NULL; }
		
		MIME_STRCOPY(mimetype);
		MIME_STRCOPY(description);
	}
	return out;
}

void mimetype_delete(APTR ctx)
{
	struct mimetype_ctx *ct = ctx;
	ASSERT(ct);

	if (ct->mimetype)
	{
		if ( ct->description )
			free( ct->description );

		free(ct->mimetype);

	}
	free(ct);
}

/*
 * Returns NULL-terminated array of loaded types. internal tells if we
 * want also internal types to be on the list.
 */

static int cmp_name(STRPTR *n1, STRPTR *n2)
{
	return (stricmp(*n1, *n2));
}

STRPTR *mimetype_gettypesarray(LONG internal, LONG mimetypes)
{

	LONG cnt = 0;
	STRPTR *array = NULL;
	struct internal_mimetype_node *imn;

	ITERATELIST( imn, &internal_mimetype_list )
	{
		if ( internal || imn->action == MIMEACTION_USER )
			cnt++;
	}

	if ( cnt )
	{
		LONG n = 0;

		array = malloc( ( cnt + 1 ) * sizeof( STRPTR ) );

		if ( array )
		{
			array[ cnt ] = NULL;

			ITERATELIST( imn, &internal_mimetype_list )
			{
				if ( internal || imn->action == MIMEACTION_USER )
				{
					if(mimetypes)
					{
						array[ n ] = name_build( imn->mimetype );
					}
					else
					{
						array[ n ] = name_build( imn->description );
					}
					if ( !array[ n ] )
					{
						while( --n >= 0 )
						{
							name_delete( array[ n ] );
						}

						free( array );
						return NULL;
					}

					n++;
				}
			}

			/* sort array by name */

			qsort(array, n, sizeof( STRPTR ), (const void *)cmp_name);

		}
	}

	return array;
}

void mimetype_freetypesarray(STRPTR *array)
{
	if (array != NULL)
	{
		int n = 0;
		for(;array[n] != NULL; n++)
		{
			name_delete(array[n]);
		}

		free(array);
	}
}

/*
 * Invalidates mimetype for all items in all views.
 */

void mimetype_invalidate(APTR imn, CONST_STRPTR mimetype)
{

	/* null imn - lookup based on mimetype */

	if ( !imn && mimetype )
	{
		struct internal_mimetype_node *iimn;

		ITERATELIST( iimn, &internal_mimetype_list )
		{
			if ( iimn->mimetype && !stricmp( iimn->mimetype, mimetype ) )
			{
				imn = iimn;
				break;
			}
		}
	}

	/* null imn and mimetype - invalidate all */

	DoMethod(app, MM_Application_WindowDoMethodByAttr, MA_Window_Type, MV_Window_Type_View,
			MM_Window_DoView, NULL, MM_View_InvalidateMimeType, mimetype);

	DoMethod(app, MM_Application_DoMethodByAttr, MV_Window_ID_Root, NULL, NULL,
			MM_Window_DoView, NULL, MM_View_InvalidateMimeType, mimetype);


	/* invalidate cached mimetypes. for now all */

	cache_invalidate( 0, CACHETAG_MIMETYPE );
}

