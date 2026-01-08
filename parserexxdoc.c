/*
 * Parses a file for some defined commands and
 * outputs a structured text.
 *
 * (c) 2003 by David Gerber <zapek@meanmachine.ch>
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
 * $Id: parserexxdoc.c,v 1.8 2007/09/29 13:35:31 fab Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define LINE_BUFFER_SIZE 8192 /* longest line */

static const char verstr[] 
#if __GNUC__ > 2
__attribute__ ((used))
#endif
= "$VER: parserexxdoc 1.0 (21.06.2003) © 2003 David Gerber, © 2006 Ambient Open Source Team";


static int parse_level;

#define PLEVEL_NONE  0
#define PLEVEL_INTRO 1
#define PLEVEL_IN    2

static int handle_line(char *in, char *out, __attribute__((unused)) int size)
{
	char *p = in;

	while (*p && (*p == ' ' || *p == '\t')) p++;

	switch (parse_level)
	{
		case PLEVEL_NONE:
		{
			if (*p++ == '/' && *p == '*')
			{
				parse_level = PLEVEL_INTRO;
			}
		}
		break;

		case PLEVEL_INTRO:
		{
			if (*p++ == '*' && *p++ == ' ')
			{
				if (!strncmp(p, "Command:", 8))
				{
					parse_level = PLEVEL_IN;
					sprintf(out, "Command:\n");
					return (1);
				}
			}
			parse_level = PLEVEL_NONE;
		}
		break;

		case PLEVEL_IN:
		{
			if (*p++ == '*')
			{
				if (*p == '/')
				{
					parse_level = PLEVEL_NONE;
					sprintf(out, "\n");
					return (1);
				}
				else if (*p++ == ' ')
				{
					if (!strncmp(p, "XXX:", 4))
					{
						parse_level = PLEVEL_NONE;
						sprintf(out, "\n");
						return (1);
					}
					strcpy(out, p);
					return (1);
				}
			}
			else
			{
				parse_level = PLEVEL_NONE;
			}
		}
		break;
	}
	return (0);
}


int main(int argc, char **argv)
{
	int rc = 10;
	char *lbuf, *wbuf;
	FILE *infile;
	FILE *outfile;

	if (argc == 3)
	{
		if( (lbuf = malloc(LINE_BUFFER_SIZE)))
		{
			if( (wbuf = malloc(LINE_BUFFER_SIZE)))
			{
				if( (infile = fopen(argv[1], "r")))
				{
					if( (outfile = fopen(argv[2], "a")))
					{
						rc = 0;

						while (fgets(lbuf, LINE_BUFFER_SIZE, infile))
						{
							if (handle_line(lbuf, wbuf, LINE_BUFFER_SIZE))
							{
								fputs(wbuf, outfile);
							}
						}
						fclose(outfile);
					}
					else
					{
						printf("Couldn't open output file <%s>\n", argv[2]);
					}
					fclose(infile);
				}
				else
				{
					printf("Couldn't open input file <%s>\n", argv[1]);
				}
				free(wbuf);
			}
			else
			{
				printf("Out of memory\n");
			}
			free(lbuf);
		}
		else
		{
			printf("Out of memory\n");
		}
	}
	else
	{
		printf("Usage: %s <infile> <outfile>\n", argv[0]);
		rc = 0;
	}
	return (rc);
}
