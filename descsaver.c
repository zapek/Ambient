/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2006 Ambient Open Source Team
 * descsaver.c, Copyright 2005 by Adam Waldenberg
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
 * $Id: descsaver.c,v 1.8 2026/05/14 22:40:54 geit Exp $
 */

#include "ambient.h"

#include <exec/types.h>
#include <libraries/mui.h>

#include "action.h"
#include "command.h"
#include "debug.h"
#include "descsaver.h"
#include "file_io.h"
#include "mimetype.h"
#include "recog.h"
#include "locale.h"

#define IOBUFFERSIZE 2048
#define DESCSAVER_WRITE(a, b) file_write(a, b, strlen(b))

static inline BOOL write_head(APTR file, struct internal_mimetype_node *imn)
{
	static CONST_STRPTR strings[] = {
		"AMTD\n", "1\n",
		"Type ", NULL,"\n",
	};
	
	int i;
	struct recog_ctx *ct = imn->rctx;
	struct recognode *rn;
	BOOL succ = TRUE;

	strings[3] = imn->mimetype;

	for (i = 0; i < 5; i++)
	{
		DB(("Trying to write: \"%s\".\n", strings[i]));

		if (!DESCSAVER_WRITE(file, strings[i]))
			succ = FALSE;
	}

	if(imn->description && *(imn->description))
	{
		TEXT line[256];
		snprintf( line, 256, "Name %s\n", imn->description );

		if (!DESCSAVER_WRITE(file, line))
			succ = FALSE;
	}

	if(imn->priority)
	{
		TEXT line[256];
		snprintf( line, 256, "Priority %ld\n", imn->priority );

		if (!DESCSAVER_WRITE(file, line))
			succ = FALSE;
	}

	if ( ct )
	{
		/* recognition name pattern hint */

		if (!DESCSAVER_WRITE(file, "Match\n"))
			succ = FALSE;

		if ( ct->pattern )
		{
			TEXT line[256];

			snprintf(line, 256, "\t%s %s\n", "PatternHint", ct->pattern);

			if (!DESCSAVER_WRITE(file, line))
				succ = FALSE;
		}

		/* recognition routine */

		if (!ISLISTEMPTY(&ct->l))
		{
			ITERATELIST(rn, &ct->l)
			{
				TEXT line[256];

				switch (rn->cmd)
				{
					case RECOGCMD_FILENAME:
						snprintf(line, 256, "\t%s %s\n", "Name", rn->data);
						break;

					case RECOGCMD_MATCH:
						snprintf(line, 256, "\t%s %s\n", "Match", rn->data);
						break;

					case RECOGCMD_MATCHHUNK:
						snprintf(line, 256, "\t%s %s\n", "MatchHunk", rn->data);
						break;

					case RECOGCMD_FINDSTRING:
						snprintf(line, 256, "\t%s %s\n", "Symbol", rn->data);
						break;

					case RECOGCMD_FILE:
						snprintf(line, 256, "\t%s\n", "File");
						break;

					case RECOGCMD_DIRECTORY:
						snprintf(line, 256, "\t%s\n", "Directory");
						break;

					case RECOGCMD_DEVICE:
						snprintf(line, 256, "\t%s\n", "Device");
						break;

					case RECOGCMD_FILESIZE:
						snprintf(line, 256, "\t%s %s\n", "Filesize", rn->data);
						break;

					case RECOGCMD_CONTENT:
						snprintf(line, 256, "\t%s %s\n", "ContentType", rn->data);
						break;

					case RECOGCMD_COMMENT:
						snprintf(line, 256, "\t%s %s\n", "Comment", rn->data);
						break;

					case RECOGCMD_BLOCKIN:
						snprintf(line, 256, "\t%s\n", "(");
						break;

					case RECOGCMD_BLOCKOUT:
						snprintf(line, 256, "\t%s\n", ")");
						break;


					case RECOGCMD_OR:
						snprintf(line, 256, "\t%s\n", "OR");
						break;

					case RECOGCMD_AND:
						snprintf(line, 256, "\t%s\n", "AND");
						break;

					#ifdef DEBUG
					default:
						PDB(("unknown cmd 0x%lx\n", rn->cmd));
						break;
					#endif
				}

				DB(("Trying to write: \"%s\".\n", line));

				if (!DESCSAVER_WRITE(file, line))
					succ = FALSE;
			}

			if (!DESCSAVER_WRITE(file, "\tEnd\n"))
				succ = FALSE;
		}
	}

	if (!succ)
	{
		PDB(("Something went wrong when writing descriptor header.\n"));
	}

	return succ;
}

static inline BOOL write_commands(APTR fh, APTR command_list)
{
	APTR cn;
	BOOL succ = TRUE;

	if (!ISLISTEMPTY( command_list))
	{
		ITERATELIST(cn, command_list)
		{
			TEXT line[256] = "\0";
			ULONG type = (ULONG) commandnode_getattr(cn, COMMANDNODETAG_TYPE);
			STRPTR command = commandnode_getattr(cn, COMMANDNODETAG_COMMAND);

			if (!command)
				continue;

			strcat(line, "\tCommand ");
			strcat(line, get_commandtypestr(type));
			strcat(line, " ");
			strcat(line, command);
			strcat(line, "\n");
			DB(("Command to save <%s>\n", command));
			
			if (!DESCSAVER_WRITE(fh, line))
				succ = FALSE;
		}
	}

	if (!succ)
	{
		PDB(("Something went wrong when writing action commands.\n"));
	}

	return succ;
}

static inline BOOL write_action(APTR fh, int atype)
{
	int wret;

	switch (atype)
	{
		case ACTION_EVENT_DOUBLECLICK:
			wret = DESCSAVER_WRITE(fh, "\tEvent DoubleClick\n");
			break;
		case ACTION_EVENT_DRAGNDROP:
			wret = DESCSAVER_WRITE(fh, "\tEvent DragNDrop\n");
			break;
		default: /* ACTION_EVENT_MENU */
			wret = DESCSAVER_WRITE(fh, "\tEvent Menu\n");
			break;
	}

	return !wret ? FALSE : TRUE;
}

/*
 * Pass num == -1 to not change default action and write the descriptor as-is.
 */
static inline BOOL change_file(STRPTR filename, struct internal_mimetype_node *imn, int num)
{
	APTR an;
	BOOL succ = TRUE;
	int i = 0;
	APTR fh;

	if ((fh = file_open(filename, MODE_NEWFILE)))
	{
		succ = write_head(fh, imn);

		ITERATELIST(an,  imn->action_list )
		{
			int actiontype = (int) actionnode_getattr(an, ACTIONNODETAG_EVENT);
			int qualifier = (int) actionnode_getattr(an, ACTIONNODETAG_QUALIFIER);
			int flags = (int) actionnode_getattr(an, ACTIONNODETAG_FLAGS);
			STRPTR acname = (STRPTR) actionnode_getattr(an, ACTIONNODETAG_NAME);
			STRPTR acmenuname = (STRPTR) actionnode_getattr(an, ACTIONNODETAG_MENU_NAME);
			APTR command_list = (APTR) actionnode_getattr(an, ACTIONNODETAG_COMMAND_LIST);

			if (!DESCSAVER_WRITE(fh, "Action\n\tName "))
				succ = FALSE;

#if USE_RECOGTRANSLATION
			if (!DESCSAVER_WRITE(fh, locale_translationgetenglish( acname ) ))
				succ = FALSE;
#else
			if (!DESCSAVER_WRITE(fh, acname))
				succ = FALSE;
#endif
			if (!DESCSAVER_WRITE(fh, "\n"))
				succ = FALSE;

			if(acmenuname)
			{
				if (!DESCSAVER_WRITE(fh, "\tMenu "))
					succ = FALSE;
#if USE_RECOGTRANSLATION
				if (!DESCSAVER_WRITE(fh, locale_translationgetenglish( acmenuname ) ))
					succ = FALSE;
#else
				if (!DESCSAVER_WRITE(fh, acmenuname))
					succ = FALSE;
#endif
				if (!DESCSAVER_WRITE(fh, "\n"))
					succ = FALSE;
			}

			if (num == -1 || (actiontype != ACTION_EVENT_DOUBLECLICK &&
			    actiontype != ACTION_EVENT_MENU))
			{
				succ = write_action(fh, actiontype);
			}
			else if (i == num)
			{
				succ = write_action(fh, ACTION_EVENT_DOUBLECLICK);
				actionnode_setattrs(an, ACTIONNODETAG_EVENT,
					ACTION_EVENT_DOUBLECLICK,
				TAG_DONE);
			}
			else
			{
				succ = write_action(fh, ACTION_EVENT_MENU);
				actionnode_setattrs(an, ACTIONNODETAG_EVENT,
					ACTION_EVENT_MENU,
				TAG_DONE);
			}

			if (command_list)
				succ = write_commands(fh, command_list);

			switch(qualifier)
			{
				case ACTION_QUALIFIER_ALT:
					if (!DESCSAVER_WRITE(fh, "\tQualifier ALT\n"))
						succ = FALSE;
					break;

				case ACTION_QUALIFIER_SHIFT:
					if (!DESCSAVER_WRITE(fh, "\tQualifier SHIFT\n"))
						succ = FALSE;
					break;
					
				case ACTION_QUALIFIER_CONTROL:
					if (!DESCSAVER_WRITE(fh, "\tQualifier CONTROL\n"))
						succ = FALSE;
					break;
			}

			if (flags)
			{					 
				if ((flags & ACTION_FLAG_CD_SOURCE))
				{
					if (!DESCSAVER_WRITE(fh, "\tFlag cd source\n"))
						succ = FALSE;
				}

				if ((flags & ACTION_FLAG_CD_DESTINATION))
				{
					if (!DESCSAVER_WRITE(fh, "\tFlag cd destination\n"))
						succ = FALSE;
				}
					
				if ((flags & ACTION_FLAG_UNQUOTED))
				{
					if (!DESCSAVER_WRITE(fh, "\tFlag unquoted\n"))
						succ = FALSE;
				}

				if ((flags & ACTION_FLAG_MULTIPLE))
				{
					if (!DESCSAVER_WRITE(fh, "\tFlag multiple\n"))
						succ = FALSE;
				}

				if ((flags & ACTION_FLAG_ASYNC))
				{
					if (!DESCSAVER_WRITE(fh, "\tFlag asynchronous\n"))
						succ = FALSE;
				}
			}

			if (!DESCSAVER_WRITE(fh, "\tEnd\n"))
				succ = FALSE;
				
			i++;
		}

		if (!DESCSAVER_WRITE(fh, "End\n"))
			succ = FALSE;
	}
	else
	{
		succ = FALSE;
	}


	if (fh)
		file_close(fh);

	if (!succ)
	{
		PDB(("Something went wrong when writing descriptor.\n"));
	}

	return succ;
}

STRPTR get_commandtypestr(ULONG type)
{
	switch (type)
	{
		case AC_INTERNAL:
			return "INTERNAL";
			break;

		case AC_AMIGADOS:
			return "AMIGADOS";
			break;

		case AC_WORKBENCH:
			return "WORKBENCH";
			break;

		case AC_SCRIPT:
			return "SCRIPT";
			break;

		case AC_AREXX:
			return "AREXX";
			break;

		#ifdef DEBUG
		default:
			PDB(("unknown cmd type 0x%lx\n", type));
			break;
		#endif
	}

	return "UNKNOWN";
}

BOOL descriptor_save(struct internal_mimetype_node *imn)
{
	ASSERT( ( imn->flags & MIMETYPEFLAG_FOREIGN ) == 0 );

	return change_file(imn->descriptor, imn, -1);
}

BOOL descriptor_change_defaultaction(struct internal_mimetype_node *imn, int num)
{
	return change_file(imn->descriptor, imn, num);
}
