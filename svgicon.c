/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2017 by Antoine Dubourg <tcheko@no-log.org>
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
 * $Id: svgicon.c,v 1.24 2025/09/13 10:34:01 piru Exp $
 */

#include "ambient.h"

#if defined(BUILD_ICONLIB)
#include <proto/vgraphics.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/graphics.h>
#include <workbench/workbench.h>
#include "svgicon.h"
#endif


#if USE_SVGICONS || defined(BUILD_ICONLIB)

/* public */

/* private */
#include "classes.h"
#include "mui_func.h"
#include "iconmem.h"
#include "methodstack.h"
#include "svgicon.h"
#include "pngicon_specs.h"
#include "gfx_mask.h"
#include "gfx_alpha.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "iconio.h"
#include "sxmlc.h"
#include "sxmlsearch.h"
#include "sxmlhelp.h"
#include <proto/cybergraphics.h>



/*
 Icon metadata is stored within the SVG content with the help
 of a simple xml comment which is format as follow:
 
 Here is a sample xml icon tag:

	<metadata>
		<ambient:icondata xmlns="http://www.morphos-team.net/morphos-icon-metadata/v1">
			<position x="0" y="0" />
			<drawer left="0" top="0" width="0" height="0" viewmode="0" sortmode="0" />
			<stack size="33792"/>
			<defaulttool path="Geeks:Development/GG/bin/a2p"/>
			<tooltype name="(TEST)"/>
			<tooltype name="(TEST2)"/>
		</ambient:icondata>
	</metadata>	
*/

BOOL svg_signature(STRPTR filename)
{
	BOOL rc = FALSE;

	BPTR lock = Open(filename, MODE_OLDFILE);

	if(lock)
	{
		char header[5];

		int len = Read(lock, header, 5);

		Close(lock);

		if (svg_signature_buffer(header, len))
			rc = TRUE;
	}

	return rc;
}

BOOL svg_signature_buffer(const void *buffer, size_t len)
{
	const char *header = buffer;
	return len >= 5 &&
	       header[0] == '<' &&
	       (strncasecmp(header+1, "?xml", 4) == 0 ||
	        strncasecmp(header+1, "svg", 3) == 0 ||
	        memcmp(header+1, "!--", 3) == 0);
}

#ifdef BUILD_ICONLIB
APTR svgicon_read(STRPTR filename, APTR obj, ULONG mode, struct FreeList *fl)
#else
APTR svgicon_read(STRPTR filename, APTR obj, ULONG mode, __unused ULONG ancillary)
#endif
{
	APTR retval = FALSE;
	#ifdef BUILD_ICONLIB
	struct Library *VGraphicsBase = OpenLibrary("vgraphics.library", 0);
	#endif

	if(VGraphicsBase)
	{
		APTR svg = VG_ImportSVG(filename, VG_Nano, TRUE, TAG_END);

		if(svg)
		{
			struct RastPort rp;
			FLOAT vec_width;
			FLOAT vec_height;
			FLOAT scalex;
			FLOAT scaley;
			FLOAT movementx = 0;
			FLOAT movementy = 0;

			/* scale the vector to fit a 64x64 icon */
			VG_GetAttr(svg, VG_BBWidth, (ULONG*)&vec_width);
			VG_GetAttr(svg, VG_BBHeight,(ULONG*) &vec_height);

			scalex = (64. / vec_width);
			scaley = (64. / vec_height);

			VG_SetAttrs(svg, VG_ScaleX, (ULONG)&scalex, TAG_END);
			VG_SetAttrs(svg, VG_ScaleY, (ULONG)&scaley, TAG_END);
			VG_SetAttrs(svg, VG_MovementX, (ULONG)&movementx, TAG_END);
			VG_SetAttrs(svg, VG_MovementY, (ULONG)&movementy, TAG_END);

			/* create a dummy rastport */
			InitRastPort(&rp);

			if( (rp.BitMap = AllocBitMap(64, 64, 32, BMF_CLEAR | BMF_SPECIALFMT | SHIFT_PIXFMT(PIXFMT_ARGB32), NULL)) != NULL)
			{
				struct bitmap_ctx *ct;

				#ifdef BUILD_ICONLIB
				if ((ct = AllocVec(sizeof(*ct), MEMF_PUBLIC)))
				#else
				if ((ct = malloc(sizeof(*ct))))
				#endif
				{
					ct->isnative = FALSE;
					ct->usecount = 1;
					ct->ref = NULL;
					ct->width  = 64;
					ct->height = 64;
					ct->depth  = 32;
					ct->bm = rp.BitMap;
					ct->bpr = GetCyberMapAttr(ct->bm, CYBRMATTR_XMOD);
					ct->modulo = ct->bpr / GetCyberMapAttr(ct->bm, CYBRMATTR_BPPIX);

					/* render vectors */
					VG_Render(svg, VGR_DestWidth, 64, VGR_DestHeight, 64, VGR_DestDepth, 32, TAG_DONE);
					/* and blit that */
					VG_Blit(&rp, svg, VGR_RawAlpha, TRUE, TAG_DONE);

					methodstack_push(obj, 4,
						MM_Icon_AddBitMap, ct, MV_Icon_BitMap_SVGicon, MV_Icon_BitMap_Normal
					);

					if (mode)
					{
						methodstack_push_sync(obj, 1, MM_Icon_End);
					}

					retval = (APTR)TRUE;
				}
				else
				{
					FreeBitMap(rp.BitMap);
					methodstack_push(obj, 2,
						MM_Icon_ErrorString, "svgicon; out of memory"
					);
				}
			}
			else
			{
				methodstack_push(obj, 2,
					MM_Icon_ErrorString, "svgicon; out of memory"
				);
			}
			
			#if USE_VECTOR_SCALER
				methodstack_push(obj, 4,
					MM_Icon_AddAncillary, MV_Icon_Ancillary_VGObj, NULL, svg
				);
			#endif

			#if defined(BUILD_ICONLIB) || !USE_VECTOR_SCALER
			VG_DisposeVGObject(svg);
			#endif
		}
		
		#ifdef BUILD_ICONLIB
		CloseLibrary(VGraphicsBase);
		read_svgtags(filename, obj, fl);
		#else
		retval = read_svgtags(filename, obj);
		#endif
	}
	
	return retval;
}

#ifdef BUILD_ICONLIB
APTR read_svgtags(char *filename, APTR obj, struct FreeList *fl)
#else
APTR read_svgtags(char *filename, Object *obj)
#endif

{
	//BOOL did_setpos = FALSE;
	//BOOL did_setdrawer = FALSE;
	APTR ret = FALSE;

	XMLDoc *svgdoc = (XMLDoc*)AllocMem(sizeof(XMLDoc), MEMF_PUBLIC);

	if(svgdoc)
	{
		XMLDoc_init(svgdoc);
		if(XMLDoc_parse_file_DOM(filename, svgdoc))
		{
			XMLNode *svgnode = XMLDoc_root(svgdoc);

			if(svgnode)
			{
				XMLNode *metadata = getnode_xp(svgnode, "svg/metadata", FALSE);

				XMLNode *morphosicon = NULL;

				if(strcmp(svgnode->tag, "svg") == 0)
				{
					if(metadata)
					{
						morphosicon = getnode_xp(metadata, "/ambient:icondata", TRUE);
						if(morphosicon)
						{
							int nodenr = getnodenum(metadata, morphosicon);
							XMLSearch *search = (XMLSearch *)AllocMem(sizeof(XMLSearch), MEMF_PUBLIC);
							int stack, posx, posy;
							int drawertop, drawerleft, drawerheight, drawerwidth, drawerviewmode, drawersortmode;
							BOOL did_setpos = FALSE;
							BOOL did_setdrawer = FALSE;
							char *defaulttool;

							if(getval_xp(morphosicon, "/stack[@size]", &stack, TRUE))
							{
								methodstack_push(obj, 3, MUIM_Set, MA_Icon_StackSize, stack);
							}

							if(getval_xp(morphosicon, "/position[@x]", &posx, TRUE))
							{
								methodstack_push(obj, 3, MUIM_Set, MA_Icon_X, posx);
								if(!did_setpos)
								{
									methodstack_push(obj, 3, MUIM_Set, MA_Icon_HasPos, TRUE);
									did_setpos = TRUE;
								}
							}

							if(getval_xp(morphosicon, "/position[@y]", &posy, TRUE))
							{
								methodstack_push(obj, 3, MUIM_Set, MA_Icon_Y, posy);
								if(!did_setpos)
								{
									methodstack_push(obj, 3, MUIM_Set, MA_Icon_HasPos, TRUE);
									did_setpos = TRUE;
								}
							}

							if(getval_xp(morphosicon, "/drawer[@left]", &drawerleft, TRUE))
							{
								methodstack_push(obj, 3, MUIM_Set, MA_Icon_WindowLeft, drawerleft);
								if(!did_setdrawer)
								{
									methodstack_push(obj, 3, MUIM_Set, MA_Icon_HasDrawerData, TRUE);
									did_setdrawer = TRUE;
								}
							}

							if(getval_xp(morphosicon, "/drawer[@top]", &drawertop, TRUE))
							{
								methodstack_push(obj, 3, MUIM_Set, MA_Icon_WindowTop, drawertop);
								if(!did_setdrawer)
								{
									methodstack_push(obj, 3, MUIM_Set, MA_Icon_HasDrawerData, TRUE);
									did_setdrawer = TRUE;
								}
							}

							if(getval_xp(morphosicon, "/drawer[@width]", &drawerwidth, TRUE))
							{
								methodstack_push(obj, 3, MUIM_Set, MA_Icon_WindowWidth, drawerwidth);
								if(!did_setdrawer)
								{
									methodstack_push(obj, 3, MUIM_Set, MA_Icon_HasDrawerData, TRUE);
									did_setdrawer = TRUE;
								}
							}

							if(getval_xp(morphosicon, "/drawer[@height]", &drawerheight, TRUE))
							{
								methodstack_push(obj, 3, MUIM_Set, MA_Icon_WindowHeight, drawerheight);
								if(!did_setdrawer)
								{
									methodstack_push(obj, 3, MUIM_Set, MA_Icon_HasDrawerData, TRUE);
									did_setdrawer = TRUE;
								}
							}

							if(getval_xp(morphosicon, "/drawer[@viewmode]", &drawerviewmode, TRUE))
							{
								methodstack_push(obj, 3, MUIM_Set, MA_Icon_ViewMode, drawerviewmode);
								if(!did_setdrawer)
								{
									methodstack_push(obj, 3, MUIM_Set, MA_Icon_HasDrawerData, TRUE);
									did_setdrawer = TRUE;
								}
							}

							if(getval_xp(morphosicon, "/drawer[@sortmode]", &drawersortmode, TRUE))
							{
								methodstack_push(obj, 3, MUIM_Set, MA_Icon_SortMode, drawersortmode);
								if(!did_setdrawer)
								{
									methodstack_push(obj, 3, MUIM_Set, MA_Icon_HasDrawerData, TRUE);
									did_setdrawer = TRUE;
								}
							}

							if((defaulttool = getstr_xp(morphosicon, "/defaulttool[@path]", TRUE)))
							{

								ULONG filetype;

								methodstack_push_sync(obj, 3, OM_GET, MA_Icon_FileType, &filetype);
								methodstack_push(obj, 3, MUIM_Set, MA_Icon_Type, MV_Icon_Type_Project);

								methodstack_push_sync(obj, 3, MUIM_Set, MA_Icon_DefaultTool, defaulttool);

							}

							if(search)
							{
								memset(search, 0, sizeof(XMLSearch));
								if(XMLSearch_init_from_XPath(C2SX("tooltype"), search))
								{
									XMLNode *snode = morphosicon;
									while((snode = XMLSearch_next(snode, search)) != NULL && snode->father == morphosicon)
									{
										char *tooltype = getstr_xattr(snode, "name");
										if(tooltype)
										{
											ULONG size;
											size = strlen(tooltype) + 1;
											methodstack_push_sync(obj, 3, MM_Icon_InsertToolType, size, tooltype);
										}
									}

									XMLSearch_free(search, true);
								}
								FreeMem(search, sizeof(XMLSearch));
							}

							XMLNode_remove_child(metadata, nodenr, true);
						}
						else
						{
							//kprintf("did not find a morphos icon node\n");
						}
					}

					ret = svgdoc;
				}
				/*else
					kprintf("this does not appear to be an SVG file\n");*/
			}
			/*else
				kprintf("Could not get the the root node for the document\n");*/
		}
		/*else
			kprintf("failed to parse the SVG file\n");*/

		if(!ret)
		{
			XMLDoc_free(svgdoc);
			FreeMem(svgdoc, sizeof(XMLDoc));
		}
	}

	return ret;
}


#endif
