/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2004 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2025 Ambient Open Source Team
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
 * $Id: private.c,v 1.17 2025/09/03 15:02:24 piru Exp $
 */

#include "globals.h"


/* public */
#include <string.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <dos/dos.h>
#include <dos/stdio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#if USE_ICONLIB_PNGLIB
#include <proto/utility.h>
#endif

#include <clib/macros.h>        // for MAX()
#include <workbench/icon.h>     // needed for os3.5 crapola
#include <workbench/workbench.h>

/* private */
#include "clib/icon_protos.h"
#include "icon_internal.h"
#include "iconio.h"
#include "file_io.h"
#include "freelist.h"
#if USE_ICONLIB_PNG
#include "pngio.h"
#include "pngicon.h"
#include "default.h"
#include "../classes.h"
#endif
#include "pngimage.h"

#define DB_GETICON 0
#define DB_PUTICON 0

#if USE_ICONLIB_SVG
#include "svgicon.h"
#include "sxmlc.h"
#include "sxmlsearch.h"
#include "sxmlhelp.h"
#endif

#include <clib/debug_protos.h>

#if USE_LEGACY
void exit(int);
#endif

/*
 * Create an icon name (appends .info or disk.info).
 * If freelist is supplied, uses it, otherwise it doesn't.
 */
static STRPTR create_icon_name(CONST_STRPTR name, struct FreeList *fl)
{
	BOOL isdevice;
	STRPTR iconname;
	ULONG allocsize;
	int len;

	ASSERT(name);

	/*
	 * We have to handle the case if we were given
	 * just a devicename. Add 'disk' in that case.
	 */
	len = strlen(name);
	if ((*FilePart(name) == '\0') && (len && name[len - 1] != '/'))
	{
		isdevice = TRUE;
	}
	else
	{
		isdevice = FALSE;
	}

	/* What is that sizeof(ULONG) thing? - piru */
	allocsize = len + (isdevice ? 4 : 0) + 6 + sizeof(ULONG);
	if (fl)
	{
		allocsize = MAX(256, allocsize);
		iconname = FreeAlloc(fl, allocsize, MEMF_ANY);
	}
	else
	{
		iconname = AllocVec(allocsize, MEMF_ANY);
	}

	if (iconname)
	{
		UBYTE *ptr = iconname;

		memcpy(ptr, name, len); ptr += len;
		if (isdevice)
		{
			memcpy(ptr, "disk", 4); ptr += 4;
		}
		memcpy(ptr, ".info", 6);
	}

	return (iconname);
}


/*
 * Only use that function on names allocated
 * *without* freelists! freelist ones are freed
 * automatically on disposal.
 */
static void delete_icon_name(STRPTR name)
{
	ASSERT(name);

	FreeVec(name);
}


/*****i icon.library/GetIcon *************************************************
*
*   NAME
*       GetIcon - read in a DiskObject structure from disk.
*
*   SYNOPSIS
*       status = GetIcon( name, icon, free )
*         D0               A0    A1    A2
*
*       long = char *, struct DiskObject *, struct FreeList *;
*
*   FUNCTION
*       This routine reads in a DiskObject structure, and its
*       associated information.  All memory will be automatically
*       allocated, and stored in the specified FreeList.  The file
*       name of the info file will be the name parameter with a
*       ".info" postpended to it.  If the call fails, a zero will
*       be returned.  The reason for the failure may be obtained
*       via IoErr().
*
*       Users are encouraged to use GetDiskObject instead of this
*       routine.  This routine will fail if the icon is not a
*       "version one" icon.
*
*   INPUTS
*       name -- name of the object (pointer to a character string)
*       icon -- a pointer to a DiskObject
*       free -- a pointer to a FreeList
*
*   RESULTS
*       status -- non-zero if the call succeeded.
*
*   SEE ALSO
*
*   BUGS
*       None
*
******************************************************************************
*/

BOOL GetIcon(CONST_STRPTR name, struct DiskObject *icon, struct FreeList *fl)
{
	D(GETICON,bug("name <%s> diskobj 0x%lx fl 0x%lx\n", name, (ULONG)icon, (ULONG)fl));
	
	if (name && icon && fl)
	{
		STRPTR iconname;
		struct OwnDiskObject *odo = (struct OwnDiskObject *)icon;
		ULONG isown = ISOWN(odo);

		if ((iconname = create_icon_name(name, isown ? fl : NULL)))
		{
			ULONG retval;

			memset(icon, 0, sizeof(*icon));
			/*
			 * Fill-in defaults.
			 */
			icon->do_Magic = WB_DISKMAGIC;
			icon->do_Version = 1;
			icon->do_Type = WBDISK;
			icon->do_Gadget.Activation = GACT_RELVERIFY;
			icon->do_Gadget.GadgetType = GTYP_BOOLGADGET;
			icon->do_CurrentX = NO_ICON_POSITION;
			icon->do_CurrentY = NO_ICON_POSITION;
			icon->do_StackSize = 4096;
			
			odo->svgdoc = NULL;
			
			/* XXX: DOpus uses g->MutualExclude it seems.. how to load it ? */
			if (isown)
			{
				odo->path = iconname;
			}

			D(GETICON,bug("iconname <%s>\n",iconname));
			retval = icon_read(iconname, icon, ICONTAG_FreeList, fl, TAG_DONE);
			
			D(GETICON,bug("read_icon retval 0x%lx\n",retval));

			
			#if USE_ICONLIB_PNG
			if (!retval && isown)
			{
				/*
				 * OwnDiskObject. Try to load a PNG then.
				 */
				D(GETICON,bug("trying to load PNG..\n"));
				if ((odo->png_context = pngio_create(iconname, FALSE)))
				{
					UBYTE *data;
					ULONG size;
					ULONG type;

					D(GETICON,bug("got context 0x%lx\n", (ULONG)odo->png_context));

					if ((data = pngio_get_chunkdata(odo->png_context, pngicon_id, &size)))
					{
						D(GETICON,bug("got data 0x%lx, size %ld\n", (ULONG)data, size));
						pngio_read_tags(odo, data, size, fl);
					}

					/*
					 * Setting up fake image.
					 */
					odo->diskobj.do_Gadget.Width = default_png_image.Width;
					odo->diskobj.do_Gadget.Height = default_png_image.Height;
					odo->diskobj.do_Gadget.Flags = 4;
					odo->diskobj.do_Gadget.Activation = 0;
					odo->diskobj.do_Gadget.GadgetType = 1;
					odo->diskobj.do_Gadget.GadgetRender = (APTR)&default_png_image;
							
					/*
					 * Try to find out the type.
					 */
					type = icon_gettype((STRPTR) name);

					if (!(type == WBTOOL && odo->diskobj.do_Type == WBPROJECT) || (type == WBGARBAGE)) /* XXX: perhaps WBGARBAGE is no longer necessary.. it used to default to that for .info without file/dir */
					{
						odo->diskobj.do_Type = type;
					}

					D(GETICON,bug("finished reading.. returning\n"));
					retval = TRUE;
				}
				else
				{
					D(GETICON,bug("can't get png context\n"));
				}
			}
			#endif
			
			#if 1
			#if USE_ICONLIB_SVG
					
			if(!retval && isown && svg_signature(iconname))
			{
				ULONG type;
				D(GETICON,bug("SVG icon detected\n"));

				/* Vector icons in SVG format */
				
				/* A bit of a trick here. Some subroutines called by read_svgtags() checks this to know if we're dealing with an svg file*/
				odo->svgdoc = 1; 
				odo->svgdoc = read_svgtags(iconname, odo, fl);
				
				/*
				 * Setting up fake image.
				 */
				odo->diskobj.do_Gadget.Width = default_png_image.Width;
				odo->diskobj.do_Gadget.Height = default_png_image.Height;
				odo->diskobj.do_Gadget.Flags = 4;
				odo->diskobj.do_Gadget.Activation = 0;
				odo->diskobj.do_Gadget.GadgetType = 1;
				odo->diskobj.do_Gadget.GadgetRender = (APTR)&default_png_image;
				
				/*
				 * Try to find out the type.
				 */
				
				type = icon_gettype((STRPTR) name);
				
				if (!(type == WBTOOL && odo->diskobj.do_Type == WBPROJECT) || (type == WBGARBAGE)) /* XXX: perhaps WBGARBAGE is no longer necessary.. it used to default to that for .info without file/dir */
				{
					odo->diskobj.do_Type = type;
				}
				
				retval = TRUE;
			}
			#endif
			#endif
			
			if (isown)
			{
				BPTR l;

				/*
				 * Ambient really needs an absolute path.
				 */
				if ((l = Lock(iconname, ACCESS_READ)))
				{
					NameFromLock(l, iconname, 256);
					UnLock(l);
				}
			}
			else
			{
				delete_icon_name(iconname);
			}
			D(GETICON,bug("done,retval 0x%lx\n",retval));
			return (retval);
		}
		else
		{
			D(GETICON,bug("can't alloc iconname mem\n"));
			SetIoErr(ERROR_NO_FREE_STORE);
		}
	}
	else
	{
		D(GETICON,bug("no name\n"));
		SetIoErr(ERROR_REQUIRED_ARG_MISSING);
	}
	D(GETICON,bug("failed\n"));
	return (FALSE);
}


/*****i icon.library/PutIcon *************************************************
*
*   NAME
*	PutIcon - write out a DiskObject to disk.
*
*   SYNOPSIS
*	status = PutIcon( name, icon )
*	  D0               A0    A1
*
*	BOOL PutIcon(char *, struct DiskObject *);
*
*   FUNCTION
*	This routine writes out a DiskObject structure, and its
*	associated information.  The file name of the info file
*	will be the name parameter with a ".info" postpended to it.
*	If the call fails, a zero will be returned.  The reason for
*	the failure may be obtained via IoErr().
*
*	PutDiskObject() and PutIcon() are functionally identical.
*	They are both provided so there is a Put/Get/Free triple
*	for disk objects.
*
*	Users are encouraged to use PutDiskObject instead of this
*	routine.  This routine assumes that the icon is a "version
*	one" icon.
*
*   INPUTS
*	name -- name of the object
*	icon -- a pointer to a DiskObject
*
*   RESULTS
*	status -- TRUE if the call succeeded else FALSE
*
******************************************************************************
*/

BOOL PutIcon(CONST_STRPTR name, struct DiskObject *icon)
{
	ULONG retval = FALSE;
	ULONG imagetype;
	
	//kprintf("%s: name=%s\n", __FUNCTION__, name);

	if (name && icon && icon->do_Gadget.GadgetRender && ((struct Image *)icon->do_Gadget.GadgetRender)->ImageData)
	{
		STRPTR iconname;

		if ((iconname = create_icon_name(name, NULL)))
		{
			struct OwnDiskObject *odo = (struct OwnDiskObject *)icon;
			
			//kprintf("%s: iconname=%s, odo=0x%p, svgdoc=0x%p\n", __FUNCTION__, iconname, odo, odo->svgdoc);
			D(PUTICON,bug("iconname: <%s>, icon: 0x%lx\n", iconname, (ULONG)icon));

			#if USE_ICONLIB_PNG
			if (ISOWN(icon) && odo->png_context)
			{
				/*
				 * Move the DiskObj fields back into
				 * ic0n tags.
				 */
				if (odo->diskobj.do_CurrentX != NO_ICON_POSITION && odo->diskobj.do_CurrentY != NO_ICON_POSITION)
				{
					if (!pngio_tag_add(odo->png_context, PNGICON_LocationX, &odo->diskobj.do_CurrentX)) goto failput;
					if (!pngio_tag_add(odo->png_context, PNGICON_LocationY, &odo->diskobj.do_CurrentY)) goto failput;
				}

				if (odo->diskobj.do_DrawerData)
				{
					ULONG viewmode = MV_Icon_ViewMode_Icon;
					ULONG sortmode = MV_Icon_SortMode_Name;

					if (!pngio_tag_add(odo->png_context, PNGICON_DrawerLeft, &odo->diskobj.do_DrawerData->dd_NewWindow.LeftEdge)) goto failput;
					if (!pngio_tag_add(odo->png_context, PNGICON_DrawerTop, &odo->diskobj.do_DrawerData->dd_NewWindow.TopEdge)) goto failput;
					if (!pngio_tag_add(odo->png_context, PNGICON_DrawerWidth, &odo->diskobj.do_DrawerData->dd_NewWindow.Width)) goto failput;
					if (!pngio_tag_add(odo->png_context, PNGICON_DrawerHeight, &odo->diskobj.do_DrawerData->dd_NewWindow.Height)) goto failput;
					
					switch (odo->diskobj.do_DrawerData->dd_ViewModes)
					{
						case DDVM_BYNAME:
							viewmode = MV_Icon_ViewMode_Lister;
							break;

						default: /* XXX: for now */
							if (odo->diskobj.do_DrawerData->dd_Flags == 3)
							{
								viewmode = MV_Icon_ViewMode_IconAll;
							}
							break;
					}

					if (viewmode != MV_Icon_ViewMode_Lister)
					{
						switch (odo->diskobj.do_DrawerData->dd_ViewModes)
						{
							case DDVM_BYDATE:
								sortmode = MV_Icon_SortMode_Date;
								break;

							case DDVM_BYSIZE:
								sortmode = MV_Icon_SortMode_Size;
								break;

							case DDVM_BYTYPE:
								sortmode = MV_Icon_SortMode_Type;
								break;

							default:
								sortmode = MV_Icon_SortMode_Name;
								break;
						}
					}

					if (!pngio_tag_add(odo->png_context, PNGICON_DrawerViewMode, &viewmode)) goto failput;
					if (!pngio_tag_add(odo->png_context, PNGICON_DrawerSortMode, &sortmode)) goto failput;
				}

				if (!pngio_tag_add(odo->png_context, PNGICON_StackSize, &odo->diskobj.do_StackSize)) goto failput;

				if (odo->diskobj.do_DefaultTool)
				{
					if (!pngio_tag_add(odo->png_context, PNGICON_DefaultTool, odo->diskobj.do_DefaultTool)) goto failput;
				}

				if (odo->diskobj.do_ToolTypes)
				{
					char **p = (char **)odo->diskobj.do_ToolTypes;

					while (*p)
					{
						if (!pngio_tag_add(odo->png_context, PNGICON_ToolType, *p)) goto failput;
						p++;
					}
				}
					
				{
					APTR fh;
					
					pngimage_create(iconname, odo);
					
					if ( (fh = file_open(iconname, MODE_NEWFILE)) )
					{
						if (!pngio_save(iconname, odo->png_context, fh))
						{
							file_close(fh);
							goto failput;
						}
						
						/* Save second image */
						
						if(odo->pngimage2)
						{
							pngio_add_bitmap(odo->png_context, odo->pngimage2, 1);
							if (!pngio_save(iconname, odo->png_context, fh))
							{
								file_close(fh);
								goto failput;
							}
						}
						
						file_close(fh);
					}
				}

				retval = TRUE;

				failput:

				if (!retval)
				{
					SetIoErr(ERROR_ACTION_NOT_KNOWN);
				}
			}
			else
			#endif
			
			#if USE_ICONLIB_SVG
			if(ISOWN(icon) && odo->svgdoc)
			{
				XMLDoc *svgdoc = odo->svgdoc;
				//kprintf("going to save the SVG\n");
			
				if(svgdoc)
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
								D(ICONIO, bug("found metadata node\n"));
								morphosicon = getnode_xp(metadata, "ambient:icondata", FALSE);
								if(morphosicon)
								{
									int nodenr = getnodenum(metadata, morphosicon);
									XMLNode_remove_child(metadata, nodenr, true);
								}
								else
								{
									D(ICONIO, bug("did not find a morphos icon node\n"));
								}
							}
							else
							{
								D(ICONIO, bug("did not find a metadata node, creating one\n"));
								metadata = XMLNode_alloc();
								if (!XMLNode_set_tag(metadata, C2SX("metadata")) ||
								    !XMLNode_set_type(metadata, XTAG_FATHER) ||
								    !XMLNode_insert_child(svgnode, XINSERT_TOP, metadata))
								{
									XMLNode_free(metadata);
									metadata = NULL;
								}
							}

							struct FreeList *freelist = create_freelist(NULL);
							if (freelist)
							{
								D(ICONIO, bug("creating a new morphos icon node\n"));
								morphosicon = XMLNode_alloc();

								if (SetSVGIconContents(odo, morphosicon, freelist) &&
								    XMLNode_add_child(metadata, morphosicon))
								{
									retval = save_xmldoc(svgdoc, iconname);
								}
								else
								{
									XMLNode_free(morphosicon);
								}
								FreeFreeList(freelist);
							}
						}
						else
							D(ICONIO, bug("this does not appear to be an SVG file\n"));
					}
					else
						D(ICONIO, bug("Could not get the the root node for the document\n"));
				}
			}
			
			else
			#endif
			
			{
				APTR fh;

				if ((fh = file_open(iconname, MODE_NEWFILE)))
				{
					struct Image *img;
					LONG rev;

					SetVBuf((BPTR)fh, NULL, BUF_FULL, 2048);

					D(PUTICON,bug("file opened\n"));

					if (!file_write(fh, icon, sizeof(*icon))) goto error;

					D(PUTICON,bug("wrote DiskObject structure\n"));
						
					if (icon->do_DrawerData)
					{
						D(PUTICON,bug("need DrawerData..\n"));
						if (!file_write(fh, icon->do_DrawerData, sizeof(struct OldDrawerData))) goto error;
						D(PUTICON,bug("wrote\n"));
					}

					img = (struct Image *)icon->do_Gadget.GadgetRender;

					if (!file_write(fh, img, sizeof(*img))) goto error;

					D(PUTICON,bug("Image1 structure written\n"));

					if (!file_write(fh, img->ImageData, RASSIZE(img->Width, img->Height) * img->Depth)) goto error;

					D(PUTICON,bug("Image1 data written\n"));

					if (icon->do_Gadget.SelectRender)
					{
						img = (struct Image *)icon->do_Gadget.SelectRender;
					
						D(PUTICON,bug("need Image2..\n"));

						if (!file_write(fh, img, sizeof(*img))) goto error;

						D(PUTICON,bug("Image2 structure written\n"));

						if (!file_write(fh, img->ImageData, RASSIZE(img->Width, img->Height) * img->Depth)) goto error;

						D(PUTICON,bug("Image2 data written\n"));
					}

					if (icon->do_DefaultTool)
					{
						D(PUTICON,bug("need DefaultTool..\n"));
						if (!icon_write_infostring(fh, (STRPTR)icon->do_DefaultTool, NULL)) goto error;
						D(PUTICON,bug("DefaultTool written\n"));
					}

					if (icon->do_ToolTypes)
					{
						char **p;
						ULONG count = 0;
						ULONG countlong;

						D(PUTICON,bug("need tooltypes\n"));

						p = (char **)icon->do_ToolTypes;

						while (*p)
						{
							count++;
							p++;
						}
						
						count++; /* add NULL termination */

						countlong = count << 2;

						D(PUTICON,bug("%ld tooltypes to write..\n", count));

						if (!file_write(fh, &countlong, sizeof(countlong))) goto error;

						p = (char **)icon->do_ToolTypes;

						while (*p)
						{
							D(PUTICON,bug("writing tooltype <%s>\n", *p));
							if (!icon_write_infostring(fh, (STRPTR)*p, NULL)) goto error;
							p++;
						}
					}
					
					rev = ((LONG)icon->do_Gadget.UserData) & WB_DISKREVISIONMASK;

					if (icon->do_DrawerData && rev > 0 && rev <= WB_DISKREVISION)
					{
						D(PUTICON,bug("need less old DrawerData..\n"));
						if (!file_write(fh, &icon->do_DrawerData->dd_Flags, 6)) goto error;
						D(PUTICON,bug("written\n"));
					}

					/*
					 * Check for glowicon crap ancillary data.
					 * Glowicons are not handled otherwise because it requires
					 * SetFunction()ing screen related functions to be able to
					 * track the palette, which is nonsense.
					 */
					if (ISOWN(icon) && odo->glowchunk)
					{
						if (!file_write(fh, odo->glowchunk, odo->glowsize)) goto error;
					}

					D(PUTICON,bug("all fine, retval set to true, woho!\n"));
					retval = TRUE;

					error:
					file_close(fh);
				}
			}
			delete_icon_name(iconname);
		}
		else
		{
			SetIoErr(ERROR_NO_FREE_STORE);
		}
	}
	else
	{
		D(PUTICON,bug("no argument\n"));
		SetIoErr(ERROR_REQUIRED_ARG_MISSING);
	}

	return (retval);
}


#if USE_ICONLIB_MORONCATCHER
/*
 * The following is "OS" 3.5 crap.
 */

/* support macros */
#define DUMPMORON(x) { if (VERSION < 44) { dprintf("author of <%s> is a moron and called icon.library/", FindTask(NULL)->tc_Node.ln_Name); dprintf("%s() using the wrong version\n", #x); } else { dprintf("%s()\n", #x);} }
#define DUMPTAGS(t) { if (t) { struct TagItem *ti = t; while (ti->ti_Tag != TAG_DONE) { dprintf("tag %s (0x%08lx) data 0x%08lx\n", (ti->ti_Tag >= ICONA_Dummy && ti->ti_Tag <= ICONDRAWA_IsLink) ? IconTags[ti->ti_Tag - ICONA_Dummy] : (STRPTR)"", ti->ti_Tag, ti->ti_Data); ti++; } } }
#else
#define DUMPMORON(x)
#define DUMPTAGS(t)
#endif

void FreeFree(struct FreeList *fl, APTR address)
{
	DUMPMORON(FreeFree);
}


static const CONST_STRPTR IconTags[]=
{       
	"ICONA_Dummy",                       
	"ICONA_ErrorCode",                   
	"ICONCTRLA_SetGlobalScreen",         
	"ICONCTRLA_GetGlobalScreen",         
	"ICONCTRLA_SetGlobalPrecision",      
	"ICONCTRLA_GetGlobalPrecision",      
	"ICONCTRLA_SetGlobalEmbossRect",     
	"ICONCTRLA_GetGlobalEmbossRect",     
	"ICONCTRLA_SetGlobalFrameless",      
	"ICONCTRLA_GetGlobalFrameless",      
	"ICONCTRLA_SetGlobalNewIconsSupport",
	"ICONCTRLA_GetGlobalNewIconsSupport",
	"ICONCTRLA_SetGlobalIdentifyHook",   
	"ICONCTRLA_GetGlobalIdentifyHook",   
	"ICONCTRLA_GetImageMask1",           
	"ICONCTRLA_GetImageMask2",           
	"ICONCTRLA_SetTransparentColor1",    
	"ICONCTRLA_GetTransparentColor1",    
	"ICONCTRLA_SetTransparentColor2",    
	"ICONCTRLA_GetTransparentColor2",    
	"ICONCTRLA_SetPalette1",             
	"ICONCTRLA_GetPalette1",             
	"ICONCTRLA_SetPalette2",             
	"ICONCTRLA_GetPalette2",             
	"ICONCTRLA_SetPaletteSize1",         
	"ICONCTRLA_GetPaletteSize1",         
	"ICONCTRLA_SetPaletteSize2",         
	"ICONCTRLA_GetPaletteSize2",         
	"ICONCTRLA_SetImageData1",           
	"ICONCTRLA_GetImageData1",           
	"ICONCTRLA_SetImageData2",           
	"ICONCTRLA_GetImageData2",           
	"ICONCTRLA_SetFrameless",            
	"ICONCTRLA_GetFrameless",            
	"ICONCTRLA_SetNewIconsSupport",      
	"ICONCTRLA_GetNewIconsSupport",      
	"ICONCTRLA_SetAspectRatio",          
	"ICONCTRLA_GetAspectRatio",          
	"ICONCTRLA_SetWidth",                
	"ICONCTRLA_GetWidth",                
	"ICONCTRLA_SetHeight",               
	"ICONCTRLA_GetHeight",               
	"ICONCTRLA_IsPaletteMapped",         
	"ICONCTRLA_GetScreen",               
	"ICONCTRLA_HasRealImage2",           
	"ICONGETA_GetDefaultType",           
	"ICONGETA_GetDefaultName",           
	"ICONGETA_FailIfUnavailable",        
	"ICONGETA_GetPaletteMappedIcon",     
	"ICONGETA_IsDefaultIcon",            
	"ICONGETA_RemapIcon",                
	"ICONGETA_GenerateImageMasks",       
	"ICONGETA_Label",                    
	"ICONPUTA_NotifyWorkbench",          
	"ICONPUTA_PutDefaultType",           
	"ICONPUTA_PutDefaultName",           
	"ICONPUTA_DropPlanarIconImage",      
	"ICONPUTA_DropChunkyIconImage",      
	"ICONPUTA_DropNewIconToolTypes",     
	"ICONPUTA_OptimizeImageSpace",       
	"ICONDUPA_DuplicateDrawerData",      
	"ICONDUPA_DuplicateImages",          
	"ICONDUPA_DuplicateImageData",       
	"ICONDUPA_DuplicateDefaultTool",     
	"ICONDUPA_DuplicateToolTypes",       
	"ICONDUPA_DuplicateToolWindow",      
	"ICONDRAWA_DrawInfo",                
	"ICONCTRLA_SetGlobalMaxNameLength",  
	"ICONCTRLA_GetGlobalMaxNameLength",  
	"ICONGETA_Screen",                   
	"ICONDRAWA_Frameless",               
	"ICONDRAWA_EraseBackground",         
	"ICONPUTA_OnlyUpdatePosition",       
	"ICONA_Reserved1",                   
	"ICONA_Reserved2",                   
	"ICONA_ErrorTagItem",                                   
	"ICONA_Reserved3",
	"ICONCTRLA_SetGlobalColorIconSupport",
	"ICONCTRLA_GetGlobalColorIconSupport",
	"ICONCTRLA_IsNewIcon",
	"ICONCTRLA_IsNativeIcon",
	"ICONA_Reserved4",
	"ICONDUPA_ActivateImageData",
	"ICONDRAWA_Borderless",
	"ICONPUTA_PreserveOldIconImages",
	"ICONA_Reserved5",
	"ICONA_Reserved6",                   
	"ICONA_Reserved7",                                      
	"ICONA_Reserved8",                                      
	"ICONDRAWA_IsLink"                  // v45
	};

struct DiskObject *DupDiskObjectA(struct DiskObject *diskObject, struct TagItem *tags)
{
	DUMPMORON(DupDiskObjectA);
	DUMPTAGS(tags);

	return (NULL);
}


ULONG IconControlA(struct DiskObject *icon, struct TagItem *tags)
{
	DUMPMORON(IconControlA);
	DUMPTAGS(tags);

	return (NULL);
}


void DrawIconStateA(struct RastPort *rp, struct DiskObject *icon, CONST_STRPTR label, LONG leftOffset, LONG topOffset, ULONG state, struct TagItem *tags)
{
	DUMPMORON(DrawIconStateA);
	DUMPTAGS(tags);
}


BOOL GetIconRectangleA(struct RastPort *rp, struct DiskObject *icon, CONST_STRPTR label, struct Rectangle *rect, struct TagItem *tags)
{
	DUMPMORON(GetIconRectangleA);
	DUMPTAGS(tags);

	return (FALSE);
}


struct DiskObject *NewDiskObject(LONG type)
{
	DUMPMORON(NewDiskObject);

	return (NULL);
}


#if !defined(ICONGETA_PNGBitMap)
#define ICONGETA_PNGBitMap                   (ICONA_Dummy + 256) /* struct BitMap ** */
#define ICONGETA_PNGBitMap_Width             (ICONA_Dummy + 257) /* ULONG ** */
#define ICONGETA_PNGBitMap_Height            (ICONA_Dummy + 258) /* ULONG ** */
#endif

struct DiskObject *GetIconTagList(CONST_STRPTR name, struct TagItem *tags)
{
	struct DiskObject *icon;

	#if !USE_ICONLIB_PNGLIB
	DUMPMORON(GetIconTagList);
	DUMPTAGS(tags);
	#endif
	
	icon = GetDiskObject(name);

	#if USE_ICONLIB_PNGLIB
	/*
	 * Mostly for Jaca's pointers.
	 */
	if (icon)
	{
		struct TagItem *ti;

		if ((ti = FindTagItem(ICONGETA_PNGBitMap, tags)) && ti->ti_Data)
		{
			STRPTR iconname;
			struct OwnDiskObject *odo = (struct OwnDiskObject *)icon;

			ASSERT(ISOWN(icon));

			if ((iconname = create_icon_name(name, NULL)))
			{
				if (pngimage_create(iconname, odo))
				{
					*((struct BitMap **)ti->ti_Data) = odo->pngimage;
					if ((ti = FindTagItem(ICONGETA_PNGBitMap_Width, tags)) && ti->ti_Data)
					{
						*(ULONG *)ti->ti_Data = odo->pngimage_width;
					}
					if ((ti = FindTagItem(ICONGETA_PNGBitMap_Height, tags)) && ti->ti_Data)
					{
						*(ULONG *)ti->ti_Data = odo->pngimage_height;
					}
				}
				delete_icon_name(iconname);
			}
		}
	}
	#endif

	return (icon);
}


BOOL PutIconTagList(CONST_STRPTR name, struct DiskObject *icon, struct TagItem *tags)
{
	ULONG retval;

	DUMPMORON(PutIconTagList);
	DUMPTAGS(tags);
	
	retval = PutIcon(name, icon);

	return (retval);
}


BOOL LayoutIconA(struct DiskObject *icon, struct Screen *screen, struct TagItem *tags)
{
	DUMPMORON(LayoutIconA);
	DUMPTAGS(tags);

	return (FALSE);
}


void ChangeToSelectedIconColor( struct ColorRegister *cr)
{
	DUMPMORON(ChangeToSelectedIconColor);
}


#if USE_LEGACY
#if 0
void exit(int a)
{
}
#endif
#endif
