/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * © 2001-2004 by David Gerber <zapek@morphos.net>
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
 * $Id: fakemethod.c,v 1.13 2025/09/03 15:18:42 piru Exp $
 */

#include "globals.h"

/* public */
#include <stdarg.h>
#include <string.h>
#include <exec/memory.h>
#include <libraries/mui.h>
#include <workbench/workbench.h>
#include <intuition/intuition.h>

/* private */
#include "icon_internal.h"
#include "clib/icon_protos.h"
#include "../classes.h"
#include "freelist.h"


#define DB_PUSHFM 0

//extern void kprintf(char *, ...);

static struct MinList emptylist =
{
	(struct MinNode *) &emptylist.mlh_Tail,
	NULL,
	(struct MinNode *) &emptylist.mlh_Head
};

static void create_drawerdata(struct DiskObject *diskobj, struct FreeList *fl)
{
	ASSERT(!diskobj->do_DrawerData);
	
	if ((diskobj->do_DrawerData = FreeAlloc(fl, sizeof(struct DrawerData), MEMF_ANY | MEMF_CLEAR)))
	{
		/*
		 * And we fill-in the defaults.
		 */
		diskobj->do_DrawerData->dd_NewWindow.DetailPen = 255;
		diskobj->do_DrawerData->dd_NewWindow.BlockPen = 255;
		diskobj->do_DrawerData->dd_NewWindow.IDCMPFlags = 0;
		diskobj->do_DrawerData->dd_NewWindow.Flags = 0;
		diskobj->do_DrawerData->dd_NewWindow.FirstGadget = NULL;
		diskobj->do_DrawerData->dd_NewWindow.CheckMark = NULL;
		diskobj->do_DrawerData->dd_NewWindow.Screen = NULL;
		diskobj->do_DrawerData->dd_NewWindow.BitMap = NULL;
		diskobj->do_DrawerData->dd_NewWindow.MinWidth = 90;
		diskobj->do_DrawerData->dd_NewWindow.MinHeight = 40;
		diskobj->do_DrawerData->dd_NewWindow.MaxWidth = 65535;
		diskobj->do_DrawerData->dd_NewWindow.MaxHeight = 65535;
		diskobj->do_DrawerData->dd_NewWindow.Type = WBENCHSCREEN;
		diskobj->do_DrawerData->dd_CurrentX = 0;
		diskobj->do_DrawerData->dd_CurrentY = 0;
		diskobj->do_DrawerData->dd_Flags = DDFLAGS_SHOWDEFAULT;
		diskobj->do_DrawerData->dd_ViewModes = DDVM_BYDEFAULT;
	}
	/* XXX */
}


/*
 * This fakes a pushmethod so that we
 * avoid rewriting code around. obj is a
 * DiskObject and the "method" just a routine
 * to modify it accordingly.
 */
void pushfakemethod(struct FreeList *fl, APTR obj, ULONG cnt, ...);
void pushfakemethod(struct FreeList *fl, APTR obj, ULONG cnt, ...)
{
	struct DiskObject *diskobj;
	ULONG mid;
	va_list va;

	ASSERT(obj);
	ASSERT(cnt);

	diskobj = (struct DiskObject *)obj;

	D(PUSHFM,bug("diskobj 0x%lx fl 0x%lx\n", (ULONG)diskobj, (ULONG)fl));

	va_start(va, cnt);

	mid = va_arg(va, ULONG);

	D(PUSHFM,bug("mid %lu\n",mid));

	switch (mid)
	{
		case MM_Icon_ErrorString:
			/*
			 * Well, there's nothing we can do.
			 */
			{
#ifdef DEBUG
				STRPTR bla = va_arg(va, STRPTR);
				D(PUSHFM,bug("*********************** ERROR! **********************\n"));
				D(PUSHFM,bug("*****************************************************\n"));
				D(PUSHFM,bug("error is: %s\n", bla));
#endif
			}
			break;

		case MM_Icon_End:
			/*
			 * Neither here..
			 */
			break;

		case MM_Icon_ToolTypesNum:
			{
				ULONG ttnum = va_arg(va, ULONG);

				if (ttnum > 1) /* includes NULL array terminator */
				{
					D(PUSHFM,bug("diskobj's fl 0x%lx\n", (ULONG)fl));
					diskobj->do_ToolTypes = (STRPTR *)FreeAlloc(fl, ttnum * sizeof(UBYTE *), MEMF_ANY | MEMF_CLEAR);
					D(PUSHFM,bug("ToolTypes 0x%lx\n", (ULONG)diskobj->do_ToolTypes));

					#if defined(USE_ICONLIB_PNG) || defined(USE_ICONLIB_SVG)
					if (diskobj->do_ToolTypes && ISOWN(diskobj))
					{
						struct OwnDiskObject *odo = (struct OwnDiskObject *)diskobj;

						if (odo->png_context)
						{
							odo->ttnum = ttnum;
						}
					}
					#endif
				}
			}
			break;

		case MM_Icon_InsertToolType:
			/*
			 * Tooltype stuff.
			 */
			#if defined(USE_ICONLIB_PNG) || defined(USE_ICONLIB_SVG)
			/*
			 * We don't know the number of tooltypes in advance so we
			 * have to waste memory here..
			 */
			{
				if (ISOWN(diskobj))
				{
					struct OwnDiskObject *odo = (struct OwnDiskObject *)diskobj;
					if ((odo->png_context || odo->svgdoc) && odo->ttcur == odo->ttnum)
					{
						APTR na;
						/* increase array */
						odo->ttnum += 512;

						D(PUSHFM,bug("increasing array to %ld entries..\n", odo->ttnum));

						if ((na = FreeAlloc(fl, odo->ttnum * sizeof(UBYTE *), MEMF_ANY | MEMF_CLEAR)))
						{
							if (odo->diskobj.do_ToolTypes)
							{
								D(PUSHFM,bug("copying old array to new array..\n"));
								memcpy(na, odo->diskobj.do_ToolTypes, (odo->ttnum - 512) * sizeof(UBYTE *));
							}
							odo->diskobj.do_ToolTypes = (STRPTR *)na;
						}
						/* XXX: ouch.. */
					}
				}
			}
			#endif
			if (diskobj->do_ToolTypes)
			{
				char **p;
				ULONG size = va_arg(va, ULONG);
				STRPTR name = va_arg(va, STRPTR);

				D(PUSHFM,bug("adding tooltype: %s\n", name));

				p = (char **)diskobj->do_ToolTypes;

				while (*p) p++;

				if (!size)
				{
					size = strlen(name) + 1;
				}

				*p = FreeAlloc(fl, size, MEMF_ANY);
				if (*p)
				{
					strcpy(*p, name);
					#if defined(USE_ICONLIB_PNG) || defined(USE_ICONLIB_SVG)
					if (ISOWN(diskobj))
					{
						struct OwnDiskObject *odo = (struct OwnDiskObject *)diskobj;
						if (odo->png_context)
						{
							odo->ttcur++;
						}
					}
					#endif
				}
				/* XXX */
			}
			break;

		case MM_Icon_AddImage:
			{
				ULONG state = va_arg(va, ULONG);
				struct Image *img = va_arg(va, struct Image *);
				UBYTE *imgdata = va_arg(va, UBYTE *);
				ULONG imgsize = va_arg(va, ULONG);

				D(PUSHFM,bug("adding image..\n"));

				/*
				 * Heck, the following is stupid but
				 * who cares..
				 */
				if (state == MV_Icon_AddImage_Normal)
				{
					D(PUSHFM,bug("diskobj's fl 0x%lx\n", (ULONG)fl));
					if ((diskobj->do_Gadget.GadgetRender = FreeAlloc(fl, sizeof(*img), MEMF_ANY)))
					{
						memcpy(diskobj->do_Gadget.GadgetRender, img, sizeof(*img));
						D(PUSHFM,bug("GadgetRender 0x%lx\n", (ULONG)diskobj->do_Gadget.GadgetRender));
						if ((((struct Image *)diskobj->do_Gadget.GadgetRender)->ImageData = FreeAlloc(fl, imgsize, MEMF_ANY)))
						{
							/*
							 * This isn't especially efficient but who cares..
							 * That Image crap stuff is legacy anyway.
							 */
							D(PUSHFM,bug("GadgetRender ImageData 0x%lx\n", (ULONG)((struct Image *)diskobj->do_Gadget.GadgetRender)->ImageData));
							memcpy(((struct Image *)diskobj->do_Gadget.GadgetRender)->ImageData, imgdata, imgsize);
							diskobj->do_Gadget.Width = img->Width;
							diskobj->do_Gadget.Height = img->Height;
							diskobj->do_Gadget.Flags |= GFLG_GADGHCOMP;
						}
						/* XXX: euuuuhh.. */
					} 
				}
				else
				{
					D(PUSHFM,bug("diskobj's fl 0x%lx\n", (ULONG)fl));
					if ((diskobj->do_Gadget.SelectRender = FreeAlloc(fl, sizeof(*img), MEMF_ANY)))
					{
						memcpy(diskobj->do_Gadget.SelectRender, img, sizeof(*img));
						D(PUSHFM,bug("SelectRender 0x%lx\n", (ULONG)diskobj->do_Gadget.SelectRender));
						if ((((struct Image *)diskobj->do_Gadget.SelectRender)->ImageData = FreeAlloc(fl, imgsize, MEMF_ANY)))
						{
							/*
							 * This isn't especially efficient but who cares..
							 * That Image crap stuff is legacy anyway.
							 */
							D(PUSHFM,bug("SelectRender ImageData 0x%lx\n", (ULONG)((struct Image *)diskobj->do_Gadget.SelectRender)->ImageData));
							memcpy(((struct Image *)diskobj->do_Gadget.SelectRender)->ImageData, imgdata, imgsize);
							diskobj->do_Gadget.Flags &= ~GFLG_GADGHCOMP;
							diskobj->do_Gadget.Flags |= GFLG_GADGHIMAGE;
							if (img->Width > diskobj->do_Gadget.Width)
							{
								diskobj->do_Gadget.Width = img->Width;
							}

							if (img->Height > diskobj->do_Gadget.Height)
							{
								diskobj->do_Gadget.Height = img->Height;
							}
						}
						/* XXX: euuuuhh.. */
					} 
				}
			}
			break;

		case MM_Icon_AddAncillary:
			{
				ULONG type = va_arg(va, ULONG);
				ULONG size = va_arg(va, ULONG);
				APTR data = va_arg(va, APTR);

				switch (type)
				{
					case MV_Icon_Ancillary_Glowicon_Chunk:
						{
							struct OwnDiskObject *odo = (struct OwnDiskObject *)diskobj;
						
							ASSERT(ISOWN(diskobj));

							odo->glowchunk = data; /* that one is freed by the freelists */
							odo->glowsize = size;
						}
						break;

					case MV_Icon_Ancillary_Gadget:
						memcpy(&diskobj->do_Gadget, data, sizeof(diskobj->do_Gadget));
						break;

					#ifdef DEBUG
					default:
						D(PUSHFM,bug("out of bound\n"));
						break;
					#endif
				}
			}
			break;

		case MM_Icon_AddBitMap:
			{
				struct BitMap *bm = va_arg(va, struct BitMap *);
				ULONG width = va_arg(va, ULONG);
				ULONG height = va_arg(va, ULONG);
				ULONG type = va_arg(va, ULONG);
				ULONG state = va_arg(va, ULONG);

				ASSERT(ISOWN(diskobj));

				switch (type)
				{
					case MV_Icon_BitMap_PNGicon:
						{
							switch (state)
							{
								case MV_Icon_BitMap_Normal:
									{
										struct OwnDiskObject *odo = (struct OwnDiskObject *)diskobj;

										ASSERT(!odo->pngimage);
										odo->pngimage = bm;
										odo->pngimage_width = width;
										odo->pngimage_height = height;
									}
									break;

								case MV_Icon_BitMap_Selected:
									{
										struct OwnDiskObject *odo = (struct OwnDiskObject *)diskobj;

										ASSERT(!odo->pngimage2);
										odo->pngimage2 = bm;
										/* second image, better have same dimensions as first one */
										odo->pngimage_width = width;
										odo->pngimage_height = height;
									}
									break;
							}
						}
						break;
				}
			}
			break;

		case MUIM_Set:
		{
			D(PUSHFM,bug("MUIM_Set\n"));
			switch (va_arg(va, ULONG))
			{
				case MA_Icon_PathInfo:
					D(PUSHFM,bug("name: %s\n", (STRPTR)va_arg(va, STRPTR)));
					break;

				case MA_Icon_Type:
					diskobj->do_Type = (UBYTE)va_arg(va, ULONG);
					D(PUSHFM,bug("type: %ld\n", (ULONG)diskobj->do_Type));
					break;

				case MA_Icon_HasPos:
					{
						ULONG haspos = va_arg(va, ULONG);
						/*if (ISOWN(diskobj))
						{
							struct OwnDiskObject *odo = (struct OwnDiskObject *)diskobj;
							odo->haspos = haspos;
						}*/
						if (!haspos)
						{
							diskobj->do_CurrentX = NO_ICON_POSITION;
							diskobj->do_CurrentY = NO_ICON_POSITION;
						}
					}
					break;

				case MA_Icon_HasDrawerData:
					{
						ULONG hasdrawerdata = va_arg(va, ULONG);
						/*if (ISOWN(diskobj))
						{
							struct OwnDiskObject *odo = (struct OwnDiskObject *)diskobj;
							odo->hasdrawerdata = hasdrawerdata;
						}*/
						if (!diskobj->do_DrawerData && hasdrawerdata)
						{
							create_drawerdata(diskobj, fl);
						}
						else if (diskobj->do_DrawerData && !hasdrawerdata)
						{
							_FreeFree(fl, diskobj->do_DrawerData);
							diskobj->do_DrawerData = NULL;
						}
					}
					break;

				case MA_Icon_StackSize:
					diskobj->do_StackSize = (LONG)va_arg(va, ULONG);
					D(PUSHFM,bug("stacksize: %ld\n", diskobj->do_StackSize));
					break;

				case MA_Icon_Y:
					diskobj->do_CurrentY = (LONG)va_arg(va, ULONG);
					break;

				case MA_Icon_X:
					diskobj->do_CurrentX = (LONG)va_arg(va, ULONG);
					break;

				case MA_Icon_WindowTop:
					if (!diskobj->do_DrawerData)
					{
						create_drawerdata(diskobj, fl);
					}
					if (diskobj->do_DrawerData)
					{
						diskobj->do_DrawerData->dd_NewWindow.TopEdge = (LONG)va_arg(va, ULONG);
					}
					break;

				case MA_Icon_WindowLeft:
					if (!diskobj->do_DrawerData)
					{
						create_drawerdata(diskobj, fl);
					}
					if (diskobj->do_DrawerData)
					{
						diskobj->do_DrawerData->dd_NewWindow.LeftEdge = (LONG)va_arg(va, ULONG);
					}
					break;

				case MA_Icon_WindowHeight:
					if (!diskobj->do_DrawerData)
					{
						create_drawerdata(diskobj, fl);
					}
					if (diskobj->do_DrawerData)
					{
						diskobj->do_DrawerData->dd_NewWindow.Height = (WORD)va_arg(va, ULONG);
					}
					break;

				case MA_Icon_WindowWidth:
					if (!diskobj->do_DrawerData)
					{
						create_drawerdata(diskobj, fl);
					}
					if (diskobj->do_DrawerData)
					{
						diskobj->do_DrawerData->dd_NewWindow.Width = (WORD)va_arg(va, ULONG);
					}
					break;

				case MA_Icon_OffsetY:
					if (!diskobj->do_DrawerData)
					{
						create_drawerdata(diskobj, fl);
					}
					if (diskobj->do_DrawerData)
					{
						diskobj->do_DrawerData->dd_CurrentY = (LONG)va_arg(va, ULONG);
					}
					break;

				case MA_Icon_OffsetX:
					if (!diskobj->do_DrawerData)
					{
						create_drawerdata(diskobj, fl);
					}
					if (diskobj->do_DrawerData)
					{
						diskobj->do_DrawerData->dd_CurrentX = (LONG)va_arg(va, ULONG);
					}
					break;

				case MA_Icon_DefaultTool:
					{
						STRPTR p = va_arg(va, STRPTR);

						D(PUSHFM,bug("diskobj's fl 0x%lx\n", (ULONG)fl));
						if ((diskobj->do_DefaultTool = FreeAlloc(fl, strlen(p) + 1, MEMF_ANY)))
						{
							D(PUSHFM,bug("DefaultTool 0x%lx\n", (ULONG)diskobj->do_DefaultTool));
							strcpy(diskobj->do_DefaultTool, p);
						}
						
						if (ISOWN(diskobj))
						{
							struct OwnDiskObject *odo = (struct OwnDiskObject *)diskobj;
							
							if (odo->png_context)
							{
								diskobj->do_Type = WBPROJECT;
							}
						}
					}
					break;

				case MA_Icon_ViewMode:
					{
						ULONG viewmode = va_arg(va, ULONG);
						if (diskobj->do_DrawerData)
						{
							switch (viewmode)
							{
								case MV_Icon_ViewMode_Icon:
									diskobj->do_DrawerData->dd_Flags = DDFLAGS_SHOWICONS;
									break;

								case MV_Icon_ViewMode_IconAll:
									diskobj->do_DrawerData->dd_Flags = DDFLAGS_SHOWICONS | DDFLAGS_SHOWALL;
									break;

								case MV_Icon_ViewMode_Lister:
									diskobj->do_DrawerData->dd_Flags = DDFLAGS_SHOWICONS;
									diskobj->do_DrawerData->dd_ViewModes = DDVM_BYNAME;
									break;

								/* XXX: missing actionlister */
							}
						}
						if (ISOWN(diskobj))
						{
							struct OwnDiskObject *odo = (struct OwnDiskObject *)diskobj;
							/* store the actual viewmode */
							odo->viewmode = viewmode;
							if (diskobj->do_DrawerData)
							{
								/* Keep values we can compare against in get */
								odo->origflags = diskobj->do_DrawerData->dd_Flags;
								odo->origviewmodes = diskobj->do_DrawerData->dd_ViewModes;
							}
							else
							{
								odo->origflags = ~0;
								odo->origviewmodes = ~0;
							}
						}
					}
					break;

				case MA_Icon_SortMode:
					if (diskobj->do_DrawerData)
					{
						ULONG sortmode = va_arg(va, ULONG);

						/* we don't get this attribute set if we are a lister.. */

						switch (sortmode)
						{
							case MV_Icon_SortMode_Name:
								diskobj->do_DrawerData->dd_ViewModes = DDVM_BYICON;
								break;

							case MV_Icon_SortMode_Date:
								diskobj->do_DrawerData->dd_ViewModes = DDVM_BYDATE;
								break;

							case MV_Icon_SortMode_Size:
								diskobj->do_DrawerData->dd_ViewModes = DDVM_BYSIZE;
								break;
							
							case MV_Icon_SortMode_Type:
								diskobj->do_DrawerData->dd_ViewModes = DDVM_BYTYPE;
								break;
						}
					}
					break;
#ifdef DEBUG
				default:
					//dprintf("unsupported tag!\n");
					break;
#endif
			}
			break;
		}

		case OM_GET:
		{
			ULONG attr = va_arg(va, ULONG);
			ULONG *ptr = va_arg(va, ULONG *);
			D(PUSHFM,bug("OM_GET\n"));
			switch (attr)
			{
				case MA_Icon_Type:
					*ptr = diskobj->do_Type;
					break;

				case MA_Icon_HasPos:
					/*if (ISOWN(diskobj))
					{
						struct OwnDiskObject *odo = (struct OwnDiskObject *)diskobj;
						*ptr = odo->haspos;
					}
					else*/
					{
						*ptr = diskobj->do_CurrentX != NO_ICON_POSITION &&
						       diskobj->do_CurrentY != NO_ICON_POSITION;
					}
					break;

				case MA_Icon_HasDrawerData:
					/*if (ISOWN(diskobj))
					{
						struct OwnDiskObject *odo = (struct OwnDiskObject *)diskobj;
						*ptr = odo->hasdrawerdata;
					}
					else*/
					{
						*ptr = diskobj->do_DrawerData != NULL ? 1 : 0;
					}
					break;

				case MA_Icon_ToolTypeList:
					D(PUSHFM,bug("MA_Icon_ToolTypeList %p\n", ptr));
					if (ISOWN(diskobj))
					{
						struct OwnDiskObject *odo = (struct OwnDiskObject *)diskobj;
						char **tt;
						struct ToolTypeNode *node, *succnode;
						/* Free previous tooltypelist */
						for (node = (struct ToolTypeNode *) odo->tooltypelist.mlh_Head;
						     (succnode =  (struct ToolTypeNode *) node->n.mln_Succ);
						     node = succnode)
						{
							_FreeFree(fl, node);
						}
						NEWLIST(&odo->tooltypelist);
						/* Convert current do_ToolTypes to a tooltype list */
						tt = diskobj->do_ToolTypes;
						if (tt)
						{
							for (; *tt; tt++)
							{
								size_t len = strlen(*tt) + 1;
								node = FreeAlloc(fl, sizeof(*node) + len, MEMF_ANY);
								if (!node)
									break;
								memcpy(node->tt, *tt, len);
								ADDTAIL(&odo->tooltypelist, &node->n);
							}
						}
						*ptr = (IPTR) &odo->tooltypelist;
					}
					else
					{
						*ptr = (IPTR) &emptylist; /* not supported */
					}
					break;

				case MA_Icon_X:
					*ptr = diskobj->do_CurrentX;
					break;

				case MA_Icon_Y:
					*ptr = diskobj->do_CurrentY;
					break;

				case MA_Icon_StackSize:
					*ptr = diskobj->do_StackSize;
					break;

#define MKDD(a,b) \
				case a: \
					*ptr = diskobj->do_DrawerData ? diskobj->do_DrawerData->b : 0; \
					break

				MKDD(MA_Icon_WindowLeft, dd_NewWindow.LeftEdge);
				MKDD(MA_Icon_WindowTop, dd_NewWindow.TopEdge);
				MKDD(MA_Icon_WindowWidth, dd_NewWindow.Width);
				MKDD(MA_Icon_WindowHeight, dd_NewWindow.Height);
				MKDD(MA_Icon_OffsetX, dd_CurrentX);
				MKDD(MA_Icon_OffsetY, dd_CurrentY);

				case MA_Icon_DefaultTool:
					*ptr = (IPTR) diskobj->do_DefaultTool;
					break;

				case MA_Icon_ViewMode:
					if (ISOWN(diskobj))
					{
						struct OwnDiskObject *odo = (struct OwnDiskObject *)diskobj;
						/* If the flags/modes were not touched, use the original viewmode value */
						if (diskobj->do_DrawerData &&
						    odo->origflags == diskobj->do_DrawerData->dd_Flags &&
						    odo->origviewmodes == diskobj->do_DrawerData->dd_ViewModes)
						{
							*ptr = odo->viewmode;
							break;
						}
					}

					/* Best effort fallback - this can't be 100% */
					if (!diskobj->do_DrawerData)
					{
						*ptr = MV_Icon_ViewMode_Icon;
						break;
					}
					if (diskobj->do_DrawerData->dd_Flags == DDFLAGS_SHOWICONS)
					{
						if (diskobj->do_DrawerData->dd_ViewModes == DDVM_BYNAME)
							*ptr = MV_Icon_ViewMode_Lister;
						else
							*ptr = MV_Icon_ViewMode_Icon;
					}
					else if (diskobj->do_DrawerData->dd_Flags == (DDFLAGS_SHOWICONS | DDFLAGS_SHOWALL))
						*ptr = MV_Icon_ViewMode_IconAll;
					else
						*ptr = MV_Icon_ViewMode_Icon;
					break;


				case MA_Icon_SortMode:
					if (!diskobj->do_DrawerData)
					{
						*ptr = 0;
						break;
					}
					switch (diskobj->do_DrawerData->dd_ViewModes)
					{
						case DDVM_BYICON:
							*ptr = MV_Icon_SortMode_Name;
							break;
						case DDVM_BYDATE:
							*ptr = MV_Icon_SortMode_Date;
							break;
						case DDVM_BYSIZE:
							*ptr = MV_Icon_SortMode_Size;
							break;
						case DDVM_BYTYPE:
							*ptr = MV_Icon_SortMode_Type;
							break;
						default:
							*ptr = 0;
							break;
					}
					break;
			}
			break;
		}

		/* XXX: add a way to add an image */
#ifdef DEBUG
		default:
			//dprintf("unsupported method 0x%lx! argh\n", mid);
			break;
#endif
	}
	
	D(PUSHFM,bug("done diskobj 0x%lx fl 0x%lx\n", (ULONG)diskobj, (ULONG)fl));
	va_end(va);
}
