/*
 * Converts an icon to an Image structure.
 *
 * (c) 2002 by David Gerber <zapek@meanmachine.ch>
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
 * $Id: getimage.c,v 1.4 2006/04/12 14:01:58 fab Exp $
 */

#include <stdio.h>
#include <exec/types.h>
#include <graphics/gfx.h>
#include <intuition/intuition.h>
#include <workbench/workbench.h>
#include <proto/icon.h>
#include <proto/dos.h>

#define ID1 "$I"
#define ID2 "d$"

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		printf("Usage: %s <filename> <output>\n", argv[0]);
	}
	else
	{
		struct DiskObject *diskobj;
		STRPTR p;
		char filename[256];

		strcpy(filename, argv[1]);

		if (p = (STRPTR)strchr(filename, '.'))
		{
			*p = '\0';
		}

		if (diskobj = GetDiskObject(filename))
		{
			if (diskobj->do_Gadget.GadgetRender)
			{
				FILE *out;

				if (out = fopen(argv[2], "a"))
				{
					struct Image *img = (struct Image *)diskobj->do_Gadget.GadgetRender;
					ULONG imgsize = RASSIZE(img->Width, img->Height) * img->Depth;
					UWORD *imgptr = (UWORD *)img->ImageData;
					ULONG i;

					fprintf(out, "\n\nstatic const UWORD %s_imagedata[] = {\n\t", FilePart(filename));

					for (i = 0; i < imgsize / 2; i++)
					{
						fprintf(out, "0x%04lx", *imgptr++);
						
						if (i < imgsize / 2 - 1)
						{
							fputs(",", out);
							if (!((i + 1) % 8))
							{
								fputs("\n\t", out);
							}
						}
						else
						{
							fputs("\n};\n\n", out);
						}

					}

					fprintf(out, "const struct Image %s_image = {\n\t", FilePart(filename));
					fprintf(out, "%ld, %ld, %ld, %ld, %ld,    /* LeftEdge, TopEdge, Width, Height, Depth */\n\t", img->LeftEdge, img->TopEdge, img->Width, img->Height, img->Depth);
					fprintf(out, "(APTR)&%s_imagedata,\n\t", FilePart(filename));
					fprintf(out, "%lu, %lu, NULL\n};\n", img->PlanePick, img->PlaneOnOff);
				
					printf("wrote %s\n", argv[2]);

					fclose(out);
				}
			}
			else
			{
				printf("no GadgetRender\n");
			}
			FreeDiskObject(diskobj);
		}
	}
	return (0);
}
