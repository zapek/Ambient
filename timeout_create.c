/*
 * Creates a timeout in the future
 * and outputs the var in a file.
 * Input timeout is in days.
 *
 * (c) 2004 by David Gerber <zapek@morphos.net>
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
 * $Id: timeout_create.c,v 1.5 2017/07/29 16:26:47 piru Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char verstr[]
#if __GNUC__ > 2
__attribute__((used))
#endif
= "$VER: timeout_create 1.0 (03.03.2004) © 2002-2004 David Gerber";


int main(int argc, char **argv)
{
	int rc = 10;

	if (argc == 3)
	{
		FILE *outfile;
		int timeout;

		timeout = atoi(argv[1]);

		if (outfile = fopen(argv[2], "w"))
		{
			fprintf(outfile, "/*\n * AUTOMATICALLY GENERATED FILE!\n */\n\ntime_t timeout_exp = %lu;\n",
				time(NULL) + timeout * 24 * 60 * 60);

			rc = 0;

			fclose(outfile);
		}
		else
		{
			printf("couldn't open output file <%s>\n", argv[2]);
		}
	}
	else
	{
		printf("usage: %s <num> <file>\n", argv[0]);
	}
	return (rc);
}
