/*
 * Advanced (not really) text encryption tool.
 * Quick and dirty. Should be rewritten properly one day (with key and stuff)
 *
 * © 2002 by David Gerber <zapek@meanmachine.ch>
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
 * $Id: tcrypt.c,v 1.7 2017/07/29 16:26:47 piru Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_BUFFER_SIZE 8192 /* longest line */

static const char verstr[]
#if __GNUC__ > 2
__attribute__((used))
#endif
= "$VER: tcrypt 1.0 (17.10.2002) © 2002-2004 David Gerber, © 2005-2006 Ambient Open Source Team";

int main(int argc, char *argv[])
{
	unsigned char *lbuf;
	FILE *in;
	FILE *out;
	FILE *hout = NULL;
	/* int linenum = 0; */
	int retval = 5;

	if (argc < 3)
	{
		printf("Usage: %s <in> <out> [hout]\nEncrypts a file.\n", argv[0]);
	}
	else
	{
		if ((lbuf = malloc(LINE_BUFFER_SIZE)))
		{
			if ((in = fopen(argv[1], "r")))
			{
				if (argc == 3 || (argc == 4 && (hout = fopen(argv[3], "w"))))
				{
					if ((out = fopen(argv[2], "w")))
					{
						unsigned long cnt = 0, first, label = 0, skiplab, ranonce, didprint = 0, firstline = 0;
						unsigned char *p;

						ranonce	= 0;
						//printf("encrypting file <%s>..\n", argv[1]);

						while (fgets((char *)lbuf, LINE_BUFFER_SIZE, in))
						{
							if ((p = (unsigned char *)strrchr((char *)lbuf, '\n')))
							{
								*p = '\0';
							
								if (p == lbuf && ranonce)
								{
									fprintf(out, "0x%lx, ", '\n' ^ cnt);
									cnt = (cnt + 1) & 0xff;
								}
							}
							first = 1;
							skiplab = 0;
							p = lbuf;

							#if 0
							if (!*p)
							{
								skiplab = 1;
							}
							#endif

							while (*p)
							{
								if (first && (*p == '#' || *p == ';' || *p == ' '))
								{
									/* comment, skip */
									label = 0;
									firstline = 0;
									break;
								}
								else if (first && *p == '%')
								{
									/* end of label */
									if (ranonce)
									{
										skiplab = 1;
										ranonce = 0;
										break;
									}
									/* new label */
									ranonce = 1;
									label = 1;
									firstline = 0;
									if (*(p + 1))
									{
										if (hout)
										{
											fprintf(hout, "extern char %s[];\n", p + 1);
										}
										fprintf(out, "char %s[] = { \n\t0x0, ", p + 1); /* that one is a marker */
									}
									else
									{
										printf("label error\n");
									}

									/* escape label */
									cnt = 0;
									while (*p++);
									first = 0;
									break;
								}
								else
								{
									/* data */
									if (label)
									{
										if (!didprint && firstline > 1)
										{
											fprintf(out, "0x%lx, ", '\n' ^ cnt);
											if (!(cnt % 80))
											{
												fprintf(out, "\n    ");
											}
											cnt = (cnt + 1) & 0xff;
										}
										fprintf(out, "0x%lx, ", *p ^ cnt);
										cnt = (cnt + 1) & 0xff;
									}
								}
								p++;
								didprint = 1;
								first = 0;
							}
							didprint = 0;
							if (skiplab)
							{
								fprintf(out, "0x%lx\n};\n\n", '\0' ^ cnt);
							}
							firstline++;
						}
						retval = 0;
						fclose(out);
					}
					else
					{
						printf("Couldn't open destination file <%s>\n", argv[2]);
					}
					
					if (argc == 4)
					{
						fclose(hout);
					}
				}
				fclose(in);
			}
			else
			{
				printf("Couldn't open file\n");
			}
			free(lbuf);
		}
		else
		{
			printf("Not enough memory\n");
		}
	}
	return (retval);
}
