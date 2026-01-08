/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber
 * Copyright 2005-2016 Ambient Open Source Team
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
 * $Id: iconio.c,v 1.32.6.4 2025/01/04 21:34:03 piru Exp $
 */

#include "ambient.h"


/* public */
#include <intuition/intuition.h>
#include <proto/graphics.h>
#include <workbench/workbench.h>
#include <proto/dos.h>
#ifdef BUILD_ICONLIB
#include <exec/memory.h>
#include <proto/icon.h>
#endif

/* private */
#ifdef BUILD_ICONLIB
#include <libraries/mui.h>
#include "classes.h"
#else
#include "newicon.h"
#include "pngicon.h"
#include "screen.h"
#endif /* !BUILD_ICONLIB */
#include "mui_func.h"
#include "threads.h"
#include "glowicon.h"
#include "methodstack.h"
#include "iconmem.h"
#include "image.h"
#include "iconio.h"
#include "file_io.h"
#include "pngio.h"
#include "iconchunk.h"
#include "tooltypelist.h"
#include "name.h"
#include "smartreq.h"
#include "deficon.h"
#include "deficon_getpath.h"
#include "deficonpool.h"
#include "datatypeicon.h"
#include "notify.h"
#include "file_func.h"
#include "prefs.h"
#include "svgicon.h"
#include "sxmlc.h"
#include "sxmlhelp.h"

#define IOBUFFERSIZE (8192 * 2)
#define MAXTOOLTYPES (8192)


/*
 * Reads an icon string (braindead format).
 * If 'size' is supplied, store the length there.
 * Protection: no strings longer than 255 chars
 */
#ifdef BUILD_ICONLIB
STRPTR icon_read_infostring(APTR fh, ULONG *size, struct FreeList *fl)
#else
STRPTR icon_read_infostring(APTR fh, ULONG *size)
#endif
{
	ULONG len;

	THREAD;
	/*
	 * Read ULONG specifying the string size including NULL terminator,
	 * and force that NULL termination.
	 */
	if (file_read(fh, &len, sizeof(len)))
	{
		if (len && (len < 256))
		{
			STRPTR p;

			if ( (p = icon_malloc(len)) )
			{
				if (file_read(fh, p, len))
				{
					p[len - 1] = '\0';

					if (size)
					{
						*size = len;
					}

					return (p);
				}

				icon_free(p);
			}
		}
	}
	//errormsg(ERR_READERROR); /* XXX: find something better.. */
	return (0);
}


/*
 * Writes an icon string (still braindead format).
 * Strings can be any length (as long as 'any' means 32-bit).
 * If 'len' is NULL, it finds out the size by itself.
 * 'len' *includes* the '\0' terminator.
 */
ULONG icon_write_infostring(APTR fh, CONST_STRPTR s, ULONG len)
{
	THREAD;
	if (!len)
	{
		len = strlen(s) + 1;
	}
	//ASSERT(len - 1);

	if (file_write(fh, &len, sizeof(len)))
	{
		if (file_write(fh, s, len))
		{
			return (TRUE);
		}
	}
	//errormsg(ERR_WRITEERROR); /* XXX: find something better.. */
	return (FALSE);
}


#ifdef BUILD_ICONLIB
/*
 * Given a name (without .info), tries to
 * find out what icontype it is.
 */
ULONG icon_gettype(CONST_STRPTR name)
{
	ULONG type = WBTOOL; /* let's assume .info without dir/file are that kind */
	ULONG len;
	TEXT buf[PATH_SIZE];

	ASSERT(name);

	len = strlen(name);
	deficon_getpath(buf, PATH_SIZE, "def_");

	if (!stricmp("disk", FilePart(name)) ||
		(len >= 16 && !strnicmp(buf, name, strlen(buf)) && !stricmp("disk", &name[len - 4]))
	)
	{
		type = WBDISK; /* XXX: no clue about WBDEVICE yet */
	}
	else
	{
		BPTR l;

		if ((l = Lock(name, ACCESS_READ)))
		{
			D_S(struct FileInfoBlock, fib);

			if (Examine(l, fib))
			{
				if (fib->fib_DirEntryType < 0) /* XXX: doesn't handle softlinks properly.. */
				{
					type = WBTOOL;
				}
				else
				{
					type = WBDRAWER;
				}
			}
			UnLock(l);
		}
	}
	return (type);
}
#else
/*
 * Given a name (with .info), tries to
 * find out the icon filetype.
 */
static ULONG icon_getfiletype(STRPTR name)
{
	ULONG type = MV_Icon_FileType_None;
	THREAD;
	ASSERT(name);

	if (!stricmp("disk.info", FilePart(name)))
	{
		type = MV_Icon_FileType_Device;
	}
	else
	{
		BPTR l;
		APTR truncation = name_truncateinfo( name );

		if ( (l = Lock(name, ACCESS_READ)) )
		{
			D_S(struct FileInfoBlock, fib);

			if (Examine(l, fib))
			{
				type = fib->fib_DirEntryType;
			}
			UnLock(l);
		}
		else
		{
			type = MV_Icon_FileType_File;
		}

		name_restoreinfo( name, truncation );
	}

	return (type);
}

static ULONG get_icontype(ULONG filetype)
{
	ULONG type = WBTOOL;

	switch (filetype)
	{
		case MV_Icon_FileType_File:
		case MV_Icon_FileType_Hardlink_File:
			type = WBTOOL;
			break;

		case MV_Icon_FileType_Directory:
		case MV_Icon_FileType_Hardlink_Directory:
			type = WBDRAWER;
			break;

		case MV_Icon_FileType_Device:
			type = WBDISK;
			break;

		case MV_Icon_FileType_Softlink:
			/* XXX: implement.. bah */
			break;

		#ifndef DEBUG
		default:
			PDB(("eek! no default\n"));
			break;
		#endif
	}

	return type;
}


/*
 * Depending on the file, set the correct filetype/tool
 * value.
 */
static void set_filetype(APTR obj, STRPTR filename, ULONG filetype)
{
	ULONG type;

	if (!filetype)
	{
		filetype = icon_getfiletype(filename);
	}

	if (filetype)
	{
		methodstack_push(obj, 3,
			MUIM_Set,
			MA_Icon_FileType, filetype
		);
	}

	type = get_icontype(filetype);

	methodstack_push(obj, 3,
		MUIM_Set,
		MA_Icon_Type, type
	);
}


static void	set_icontype_from_filetype(LONG filetype, ULONG *icontype)
{
	ASSERT(icontype);

	switch (filetype)
	{
		case MV_Icon_FileType_File:
			if (!(*icontype == WBTOOL || *icontype == WBPROJECT))
			{
				*icontype = WBTOOL;
			}
			break;

		case MV_Icon_FileType_Directory:
		case MV_Icon_FileType_Hardlink_Directory:
			*icontype = WBDRAWER;
			break;

		case MV_Icon_FileType_Device:
			if (!(*icontype == WBDISK || *icontype == WBDEVICE))
			{
				*icontype = WBDISK;
			}
			break;

		case MV_Icon_FileType_Softlink:
			/* XXX: implement support for that! */
			break;

		default:
			/* for example the famous shell.info case */
			break;
	}
}
#endif


#ifndef BUILD_ICONLIB
static STRPTR assign_to_filename(CONST_STRPTR filename)
{
	STRPTR s, p, n;
	ULONG len;

	p = strchr(filename, ':');
	n = NULL;

	if (p)
	{
		len = p - filename + 2;

		s = malloc(len);

		if (s)
		{
			BPTR l2;

			stccpy(s, filename, len);

			if ( (l2 = Lock(s, ACCESS_READ)) )
			{
				ULONG pathlen = 128;

				do
				{
					n = malloc(pathlen + sizeof(".info"));

					if (!n)
						break;

					if (NameFromLock(l2, n, pathlen))
					{
						int len = strlen(n);

						if (!len || n[len - 1] != ':')
						{
							strcpy(n + len, ".info");
							break;
						}

						free(n);
						n = NULL;
						break;
					}

					free(n);
					pathlen += 128;
					n = NULL;
				}
				while (IoErr() == ERROR_LINE_TOO_LONG);

				UnLock(l2);
			}
			free(s);
		}
	}

	return (n);
}
#endif


static BPTR lock_icon(CONST_STRPTR filename, LONG is_assign)
{
	BPTR l;

	#ifndef BUILD_ICONLIB
	if (is_assign)
	{
		STRPTR s;

		s = assign_to_filename(filename);
		l = 0;

		if (s)
		{
			l = Lock(s, ACCESS_READ);
			free(s);
		}
	}
	else
	#endif
	{
		l = Lock(filename, ACCESS_READ);
	}

	return (l);
}


static APTR open_icon(CONST_STRPTR filename, ULONG mode, LONG is_assign)
{
	APTR fh;

	#ifndef BUILD_ICONLIB
	if (is_assign)
	{
		STRPTR s;

		s = assign_to_filename(filename);
		fh = 0;

		if (s)
		{
			fh = file_open(s, mode);
			free(s);
		}
	}
	else
	#endif
	{
		fh = file_open(filename, mode);
	}

	return (fh);
}


#ifndef BUILD_ICONLIB
ULONG iconio_get_default_icon(STRPTR realfile, APTR obj)
{
	STRPTR filename = name_build_info(realfile);
	ULONG rc = FALSE;

	if (filename)
	{
		ULONG filetype, icontype;

		filetype = icon_getfiletype(realfile);
		set_icontype_from_filetype(filetype, &icontype);

		D(ICONIO,bug("Filetype is %ld, icontype is %ld.\n", filetype, icontype));

		if (deficonpool_apply_default_icon(obj, icontype, realfile, (APTR)DEFICON_MIMETYPE_RECOGNIZE))
		{
			D(ICONIO,bug("Default icon found.\n"));
			methodstack_push(obj, 3, MUIM_Set, MA_Icon_Infowin, TRUE);
			methodstack_push_sync(obj, 3, MUIM_Set, MA_Icon_IsDefault, TRUE);
			rc = TRUE;
		}

		name_delete(filename);
	}

	return rc;
}
#endif


/*
 * Reads an Icon and sets all the attributes of
 * the IconObject. Is designed to be called from
 * another task.
 *
 * - filename: filename of the file, with .info at the end
 * - obj: object to send methods to (iconclass)
 */
#define ICF_ANCILLARY (1 << 0UL)
#define ICF_POSITION  (1 << 1UL)
#define ICF_END       (1 << 2UL)
#define ICF_INFOWIN   (1 << 3UL)
#define ICF_DEFICON   (1 << 4UL)
#define ICF_ASSIGN    (1 << 5UL)
#define ICF_GETIMAGE  (1 << 6UL)

ULONG v_icon_read(STRPTR filename, APTR obj, struct TagItem *tags)
{
	APTR fh;
	ULONG retval = FALSE;
	BPTR l = 0;
	#ifdef BUILD_ICONLIB
	struct FreeList *fl = NULL;
	#endif
	ULONG len = 0;
	ULONG filetype = 0;
	ULONG flags = (ICF_POSITION | ICF_GETIMAGE | ICF_END);
	#ifndef BUILD_ICONLIB
	ULONG icontype = MV_Icon_Type_None;
	#endif

	THREAD;
	ASSERT(filename);
	CHECKOBJECT(obj);

	FORTAG(tags)
	{
		case ICONTAG_FreeList:
			#ifndef BUILD_ICONLIB
			PDB(("freelist is only for iconlib\n"));
			#else
			fl = (struct FreeList *)tag->ti_Data;
			#endif
			break;

		case ICONTAG_FileSize:
			len = tag->ti_Data;
			break;

		case ICONTAG_FileType:
			filetype = tag->ti_Data;
			break;

		case ICONTAG_Ancillary:
			if (tag->ti_Data)
			{
				flags |= ICF_ANCILLARY;
			}
			else
			{
				flags &= ~ICF_ANCILLARY;
			}
			break;

		case ICONTAG_Deficon:
			if (tag->ti_Data)
			{
				flags |= ICF_DEFICON;
			}
			else
			{
				flags &= ~ICF_DEFICON;
			}
			break;

		case ICONTAG_Position:
			if (tag->ti_Data)
			{
				flags |= ICF_POSITION;
			}
			else
			{
				flags &= ~ICF_POSITION;
			}
			break;

		case ICONTAG_End:
			if (tag->ti_Data)
			{
				flags |= ICF_END;
			}
			else
			{
				flags &= ~ICF_END;
			}
			break;

		case ICONTAG_Infowin:
			if (tag->ti_Data)
			{
				flags |= ICF_INFOWIN;
			}
			else
			{
				flags &= ~ICF_INFOWIN;
			}
			break;

		#ifndef BUILD_ICONLIB
		case ICONTAG_IsAssign:
			if (tag->ti_Data)
			{
				filetype = MV_Icon_FileType_Directory;
				flags |= ICF_ASSIGN;
			}
			else
			{
				flags &= ~ICF_ASSIGN;
			}
			break;

		case ICONTAG_GetImage:
			if (tag->ti_Data)
			{
				flags |= ICF_GETIMAGE;
			}
			else
			{
				flags &= ~ICF_GETIMAGE;
			}
			break;
		#endif

		#ifdef DEBUG
		default:
			PDB(("unknown tag 0x%lx\n", tag->ti_Tag));
			break;
		#endif
	}
	NEXTTAG

	#ifdef BUILD_ICONLIB
	ASSERT(fl);
	#endif
	/*
	 * we use sync here, because filename might get modifies in the process. Safety++
	 */

	methodstack_push_sync(obj, 3,
		MUIM_Set,
		MA_Icon_PathInfo, filename
	);

	D(ICONIO, bug("trying to open <%s>..\n", filename));

	if ( (l = lock_icon(filename, flags & ICF_ASSIGN)) )
	{
		D(ICONIO, bug("file <%s> locked\n", filename));
	}

	#ifndef BUILD_ICONLIB
	if (flags & ICF_INFOWIN)
	{
		methodstack_push(obj, 3,
			MUIM_Set,
			MA_Icon_Infowin, TRUE
		);
	}

	if (!filetype)
	{
		filetype = icon_getfiletype(filename);
	}
	set_icontype_from_filetype(filetype, &icontype);
	#endif

	if (!l)
	{
		ULONG found_default = FALSE;

		#if 0
		methodstack_push(obj, 2,
			MM_Icon_ErrorString, "Filesystem error: couldn't lock .info file"
		);
		#endif
		#ifndef BUILD_ICONLIB
		if (flags & ICF_DEFICON)
		{
			STRPTR realfile;

			if ( (realfile = name_build_noinfo(filename)) )
			{
				if ( (l = Lock(realfile, ACCESS_READ)) )
				{
					if ( (found_default = deficonpool_apply_default_icon(obj, icontype, filename, NULL)) )
					{
						set_filetype(obj, filename, filetype);

						/*
						 * Should handle some states nicer, but here we go:
						 * Setup IsDeficon to TRUE (seems to not be set?)
						 * Setup IsLink accordingly
						 */

						methodstack_push(obj, 3,
							MUIM_Set,
							MA_Icon_IsDefault, TRUE
						);
						methodstack_push(obj, 3,
							MUIM_Set,
							MA_Icon_IsLink, islink( realfile )
						);
					}

					UnLock(l);
				}
				name_delete(realfile);
			}
		}
		#endif

		#ifndef BUILD_ICONLIB
		if (flags & ICF_END)
		#endif
		{
			methodstack_push_sync(obj, 1,
				MM_Icon_End
			);
		}
		#ifndef BUILD_ICONLIB
		else
		{
			methodstack_check(FALSE);
		}
		#endif
		return (found_default);
	}

	D(ICONIO, bug("filename to open: <%s>\n", filename));

	#ifndef BUILD_ICONLIB
	if (!len)
	{
		D_S(struct FileInfoBlock, fib);
		if (Examine(l, fib))
		{
			len = fib->fib_Size;  /* icon files are supposed to stay <2GB */
		}
		D(ICONIO, bug("file length is %lu\n", len));
	}

	if (!len)
	{
		/*
		 * Bah, can happen with buggy filesystems like
		 * Olsen's smbfs.
		 */
		methodstack_push(obj, 2,
			MM_Icon_ErrorString, "Filesystem error: no file length"
		);
		if (flags & ICF_DEFICON)
		{
			deficonpool_apply_default_icon(obj, icontype, filename, NULL);
		}

		if (flags & ICF_END)
		{
			methodstack_push_sync(obj, 1,
				MM_Icon_End
			);
		}
		else
		{
			methodstack_check(FALSE);
		}

		UnLock(l);
		return (FALSE);
	}
	#endif

	if ( (fh = open_icon(filename, MODE_OLDFILE, flags & ICF_ASSIGN)) )
	{
		struct DiskObject diskobj;

		D(ICONIO, bug("file <%s> opened successfully\n", filename));

		#ifdef BUILD_ICONLIB
		SetVBuf((BPTR)fh, NULL, BUF_FULL, 2048);
		#endif
		/*
		 * Read the first 8 bytes to find out the format.
		 */
		if (file_read(fh, &diskobj, 8))
		{
			if (diskobj.do_Magic == WB_DISKMAGIC && diskobj.do_Version == WB_DISKVERSION)
			{
				/*
				 * Old icon
				 */
				D(ICONIO, bug("icon signature found\n"));
				if (file_read(fh, (UBYTE *)&diskobj + 8, sizeof(diskobj) - 8))
				{
					LONG rev = TRUE; /* that var is abused twice :) */

					if (diskobj.do_Type == WBAPPICON)
					{
						diskobj.do_Type = WBPROJECT;
					}

					#ifndef BUILD_ICONLIB

					/* XXX: I don't remember why I put this here */

					/* Actually it makes sense to set it according to real target type,
					   it avoids having a unsuited tool/project icon for a drawer/disk. (fab) */
					#if 0
					if (icontype != MV_Icon_Type_None)
					{
						diskobj.do_Type = icontype;
					}
					#endif

					if (filetype)
					{
						methodstack_push(obj, 3,
							MUIM_Set,
							MA_Icon_FileType, filetype
						);
					}
					else
					{
						switch (diskobj.do_Type)
						{
							case WBDRAWER:
								methodstack_push(obj, 3,
									MUIM_Set,
									MA_Icon_FileType, MV_Icon_FileType_Directory
								);
								break;

							case WBTOOL:
							case WBPROJECT:
								methodstack_push(obj, 3,
									MUIM_Set,
									MA_Icon_FileType, MV_Icon_FileType_File
								);
								break;
						}
					}
					#endif

					methodstack_push(obj, 3,
						MUIM_Set,
						MA_Icon_Type, (ULONG)diskobj.do_Type
					);

					methodstack_push(obj, 3,
						MUIM_Set,
						MA_Icon_StackSize, diskobj.do_StackSize /* XXX: for a deficon this is wrong too */
					);

					#ifndef BUILD_ICONLIB
					if (flags & ICF_POSITION)
					#endif
					{
						if (!((diskobj.do_CurrentX == (LONG)NO_ICON_POSITION) || (diskobj.do_CurrentY == (LONG)NO_ICON_POSITION))) /* XXX: what to do for a deficon ? */
						{
							if (diskobj.do_CurrentY < 0) /* XXX: this is not smart.. */
							{
								diskobj.do_CurrentY = 0;
							}
							methodstack_push(obj, 3,
								MUIM_Set,
								MA_Icon_Y, diskobj.do_CurrentY
							);

							if (diskobj.do_CurrentX < 0)
							{
								diskobj.do_CurrentX = 0;
							}

							methodstack_push(obj, 3,
								MUIM_Set,
								MA_Icon_X, diskobj.do_CurrentX
							);

							methodstack_push(obj, 3,
								MUIM_Set,
								MA_Icon_HasPos, TRUE
							);
						}
					}

					D(ICONIO, bug("icon specs: type: %ld, stacksize: %ld, x: %ld, y: %ld\n",
						(ULONG)diskobj.do_Type, diskobj.do_StackSize, diskobj.do_CurrentX, diskobj.do_CurrentY));

					if (diskobj.do_DrawerData)
					{
						struct OldDrawerData dd;

						/*
						 * Relic from the past. We have an OldDrawerData structure
						 * here.
						 */
						if (file_read(fh, &dd, sizeof(struct OldDrawerData)))
						{
							methodstack_push(obj, 3,
								MUIM_Set,
								MA_Icon_WindowTop, (ULONG)dd.dd_NewWindow.TopEdge
							);

							methodstack_push(obj, 3,
								MUIM_Set,
								MA_Icon_WindowLeft, (ULONG)dd.dd_NewWindow.LeftEdge
							);

							methodstack_push(obj, 3,
								MUIM_Set,
								MA_Icon_WindowHeight, (ULONG)dd.dd_NewWindow.Height
							);

							methodstack_push(obj, 3,
								MUIM_Set,
								MA_Icon_WindowWidth, (ULONG)dd.dd_NewWindow.Width
							);

							methodstack_push(obj, 3,
								MUIM_Set,
								MA_Icon_OffsetY, dd.dd_CurrentY
							);

							methodstack_push(obj, 3,
								MUIM_Set,
								MA_Icon_OffsetX, dd.dd_CurrentX
							);

							D(ICONIO, bug("OldDrawerData: win x: %ld, win y: %ld, win xs: %ld, win ys: %ld, offs x: %ld, offs y: %ld\n", (LONG)dd.dd_NewWindow.LeftEdge, (LONG)dd.dd_NewWindow.TopEdge, (LONG)dd.dd_NewWindow.Width, (LONG)dd.dd_NewWindow.Height, (LONG)dd.dd_CurrentX, (LONG)dd.dd_CurrentY));
						}
						else
						{
							D(ICONIO, bug("error reading OldDrawerData\n"));
							methodstack_push(obj, 2,
								MM_Icon_ErrorString, "Filesystem error: no file length"
							);
							rev = FALSE;
						}
					}

					if (rev)
					{
						#ifndef BUILD_ICONLIB
						if (flags & ICF_ANCILLARY)
						#endif
						{
							methodstack_push(obj, 4,
								MM_Icon_AddAncillary, MV_Icon_Ancillary_Gadget, NULL, &diskobj.do_Gadget
							);
						}

						D(ICONIO, bug("reading first image.. offset: 0x%llx\n", file_seek(fh, 0, OFFSET_CURRENT)));
						#ifdef BUILD_ICONLIB
						if (read_image(fh, obj, MV_Icon_BitMap_Normal, NULL, fl))
						#else
						if (read_image(fh, obj, MV_Icon_BitMap_Normal, flags & ICF_ANCILLARY))
						#endif
						{
							ULONG *ttmem = 0;

							if (diskobj.do_Gadget.SelectRender)
							{
								/*
								 * Read in second icon image (selected state).
								 */
								D(ICONIO, bug("reading second image.. offset: 0x%llx\n", file_seek(fh, 0, OFFSET_CURRENT)));
								#ifdef BUILD_ICONLIB
								if (!read_image(fh, obj, MV_Icon_BitMap_Selected, NULL, fl))
								#else
								if (!read_image(fh, obj, MV_Icon_BitMap_Selected, flags & ICF_ANCILLARY))
								#endif
								{
									methodstack_push(obj, 2,
										MM_Icon_ErrorString, "bogus 2nd image"
									);
									rev = FALSE;
								}
							}

							if (rev)
							{
								if (diskobj.do_DefaultTool)
								{
									D(ICONIO, bug("reading DefaultTool.., offset 0x%llx\n", file_seek(fh, 0, OFFSET_CURRENT)));
									#ifdef BUILD_ICONLIB
									if ( (diskobj.do_DefaultTool = icon_read_infostring(fh, NULL, fl)) )
									#else
									if ( (diskobj.do_DefaultTool = icon_read_infostring(fh, NULL)) )
									#endif
									{
										if (diskobj.do_DefaultTool[0])
										{
											D(ICONIO, bug("do_DefaultTool: <%s>\n", diskobj.do_DefaultTool));
											methodstack_push(obj, 3,
												MUIM_Set,
												MA_Icon_DefaultTool, diskobj.do_DefaultTool
											);
										}
										else
										{
											/*
											 * Bah.. discard those braindead empty strings.
											 */
											icon_free(diskobj.do_DefaultTool);
											diskobj.do_DefaultTool = NULL;
										}
									}
								}

								/*
								 * Tooltypes
								 */
								if (diskobj.do_ToolTypes)
								{
									ULONG ttnum;

									D(ICONIO, bug("tooltype mess, offset 0x%llx\n", file_seek(fh, 0, OFFSET_CURRENT)));
									/*
									 * XXX: We should protect ourself from buggy icons with
									 * a wrong array size here..
									 */
									if (file_read(fh, &ttnum, sizeof(ttnum))) /* number of *LONGs* to allocate */
									{
										ULONG i;

										/*
										 * Allocate an array for handling the
										 * tooltype allocations.
										 */
										if (ttnum && !(ttnum & 3) && ttnum <= MAXTOOLTYPES) /* should be enough for everyone (tm) */
										{
											D(ICONIO, bug("processing %ld of them (array size: %ld)\n", ttnum >> 2, ttnum));
											#ifdef BUILD_ICONLIB
											methodstack_push(obj, 2,
												MM_Icon_ToolTypesNum, ttnum >> 2
											);
											#endif /* BUILD_ICONLIB */

											if ( (ttmem = icon_malloc(ttnum)) )
											{
												ULONG idx = 0;
												ULONG sz;
												STRPTR buf;

												memset(ttmem, 0, ttnum);

												for (i = 0; i < (ttnum >> 2) - 1; i++) /* there's one tooltype more, even if it doesn't exist physically.. who knows what they had in mind.. */
												{
													#ifdef BUILD_ICONLIB
													if ( (buf = icon_read_infostring(fh, &sz, fl)) )
													#else
													if ( (buf = icon_read_infostring(fh, &sz)) )
													#endif
													{
														ttmem[idx++] = (ULONG)buf;

														#if USE_NEWICONS
														if (newicon_find(fh, buf)) /* XXX: length.. I think :) */
														{
															#ifndef BUILD_ICONLIB
															if (flags & ICF_GETIMAGE)
															#endif
															{
																D(ICONIO, bug("reading first newicon image..\n"));
																if (newicon_read_image(fh, obj, MV_Icon_BitMap_Normal, flags & ICF_ANCILLARY))
																{
																	D(ICONIO, bug("reading second newicon image..\n"));
																	if (!newicon_read_image(fh, obj, MV_Icon_BitMap_Selected, flags & ICF_ANCILLARY))
																	{
																		/*
																		 * That's ok.. 2nd image is optional.
																		 * XXX: detect if there was a real failure
																		 */
																		//D(ICONIO, bug("failed\n"));
																		//rev = 0;
																	}
																}
																else
																{
																	D(ICONIO, bug("failed\n"));
																	rev = 0;
																}
															}
															break; /* out of the loop */
														}
														#endif /* USE_NEWICONS */
														D(ICONIO,bug("inserting tooltype <%s>, size: %ld\n", buf, sz));

														#ifdef BUILD_ICONLIB
														methodstack_push_sync(obj, 3,
															MM_Icon_InsertToolType, sz, buf
														);
														#else
														if (!methodstack_push_sync(obj, 3,
															MM_Icon_InsertToolType, sz, buf
														))
														{
															methodstack_push(obj, 2,
																MM_Icon_ErrorString, "out of mem"
															);
														}
														#endif
													}
													else
													{
														D(ICONIO, bug("empty tooltype string.. continuing\n"));
														continue;
													}
												}

											}
											else
											{
												methodstack_push(obj, 2,
													MM_Icon_ErrorString, "out of mem"
												);
												rev = 0;
											}
										}
										else
										{
											D(ICONIO, bug("no tooltype array, or bogus tooltype array: %ld\n", ttnum));
											ttnum = 0;
										}
									}
									else
									{
										methodstack_push(obj, 2,
											MM_Icon_ErrorString, "partly missing tooltype array"
										);
										rev = 0;
									}
								}

								if (rev)
								{
									#if USE_GLOWICONS || defined(BUILD_ICONLIB)
									QUAD seekpos = 0;
									ULONG has_glow = FALSE;
									APTR glowbuf = NULL;
									#endif
   
									if (diskobj.do_ToolWindow)
									{
										D(ICONIO,bug("is supposed to have a toolwindow..\n"));
										#ifdef BUILD_ICONLIB
										if ( (diskobj.do_ToolWindow = icon_read_infostring(fh, NULL, fl)) )
										#else
										if ( (diskobj.do_ToolWindow = icon_read_infostring(fh, NULL)) )
										#endif
										{
											/*
											 * Ok, this is used by old C/asm startup codes
											 * to open a window specified here which will be
											 * put as stdin/stdout and pr_WindowPtr.
											 * A quick check shows that the SAS/C startup code
											 * uses it but the libnix one doesn't. Is it really
											 * worth supporting ?
											 */
											D(ICONIO, bug("wow, has toolwindow..\n"));
										}
									}

									rev = ((LONG)diskobj.do_Gadget.UserData) & WB_DISKREVISIONMASK;

									/*
									 * This is the weird (previously undocumented) drawerdata
									 * extension stuff. In fact, that half formes
									 * a complete DrawerData structure combined
									 * with OldDrawerData. DOpus stores things there,
									 * icon.library from "OS" 3.5 strips everything
									 * making users whine. Hm, or maybe not..
									 */
									if (diskobj.do_DrawerData)
									{
										if (rev > 0 && rev <= WB_DISKREVISION)
										{
											struct DrawerData dd;
											D(ICONIO,bug("reading drawerdata.. less old style\n"));
											if (file_read(fh, &dd.dd_Flags, 6))
											{
												ULONG sortmode;

												switch (dd.dd_ViewModes)
												{
													default: /* XXX: for now.. */
														methodstack_push(obj, 3,
															MUIM_Set,
															MA_Icon_ViewMode, (dd.dd_Flags == 3) ? MV_Icon_ViewMode_IconAll : MV_Icon_ViewMode_Icon /* bit 0 and 1 must be set. yeah.. */
														);
														break;

													case DDVM_BYNAME:
														methodstack_push(obj, 3,
															MUIM_Set,
															MA_Icon_ViewMode, MV_Icon_ViewMode_Lister
														);
														break;
												}

												switch (dd.dd_ViewModes)
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

												methodstack_push(obj, 3,
													MUIM_Set,
													MA_Icon_SortMode, sortmode
												);
											}
											methodstack_push(obj, 3,
												MUIM_Set,
												MA_Icon_HasDrawerData, MV_Icon_HasDrawerData_LessOld
											);
										}
										else
										{
											D(ICONIO,bug("old drawerdata style so no extention to read\n"));
											methodstack_push(obj, 3,
												MUIM_Set,
												MA_Icon_HasDrawerData, MV_Icon_HasDrawerData_Old
											);
										}
									}

									#if USE_GLOWICONS || defined(BUILD_ICONLIB)
									D(ICONIO, bug("testing for glowicon.., offset: 0x%llx\n", file_seek(fh, 0, OFFSET_CURRENT)));

									/*
									 * And now the IFF extensions.
									 */
									#ifndef BUILD_ICONLIB
									if (flags & ICF_ANCILLARY)
									#endif
									{
										seekpos = file_seek(fh, 0, OFFSET_CURRENT);
									}
									#ifdef BUILD_ICONLIB
									has_glow = glowicon_read(fh, obj, fl);
									#else
									has_glow = flags & ICF_GETIMAGE ? glowicon_read(fh, obj) : 0;
									#endif

									D(ICONIO, bug("glowicon stuff finished: %s\n", has_glow ? "this is a glowicon" : "not a glowicon"));
									#ifdef BUILD_ICONLIB
									if (has_glow)
									#else
									if (has_glow && (flags & ICF_ANCILLARY))
									#endif
									{
										QUAD currentpos = file_seek(fh, 0, OFFSET_CURRENT);
										if (currentpos != -1LL)
										{
											UQUAD glowsize = currentpos - seekpos;

											D(ICONIO, bug("glowicon chunk size: %lld\n", glowsize));

											if ( glowsize < 0x7ffffff0ULL && (glowbuf = icon_malloc(glowsize)) )
											{
												if ( file_seek(fh, seekpos, OFFSET_BEGINNING) != -1LL &&
												     file_read(fh, glowbuf, glowsize) )
												{
													methodstack_push(obj, 4,
														MM_Icon_AddAncillary, MV_Icon_Ancillary_Glowicon_Chunk, (ULONG)glowsize, glowbuf
													);
												}
												else
												{
													methodstack_push(obj, 2,
														MM_Icon_ErrorString, "Ancillary data I/O error"
													);
												}
											}
											else
											{
												methodstack_push(obj, 2,
													MM_Icon_ErrorString, "out of mem"
												);
											}
										}
										else
										{
											methodstack_push(obj, 2,
												MM_Icon_ErrorString, "Ancillary data I/O error"
											);
										}
									}
									#endif /* USE_GLOWICONS */

									#ifndef BUILD_ICONLIB
									if (flags & ICF_END)
									#endif
									{
										methodstack_push_sync(obj, 1,
											MM_Icon_End
										);
									}
									#ifndef BUILD_ICONLIB
									else
									{
										methodstack_check(FALSE);
									}
									#endif

									retval = TRUE;

									#if USE_GLOWICONS
									if (glowbuf)
									{
										icon_free(glowbuf);
									}
									#endif

									D(ICONIO, bug("icon fully processed\n"));

									if (ttmem)
									{
										ULONG i = 0;

										while (ttmem[i])
										{
											icon_free((APTR)ttmem[i]);
											i++;
										}
										icon_free(ttmem);
									}

									D(ICONIO, bug("icon I/O resources freed\n"));

									if (diskobj.do_ToolWindow)
									{
										icon_free(diskobj.do_ToolWindow);
									}
								}
								if (diskobj.do_DefaultTool)
								{
									icon_free(diskobj.do_DefaultTool);
								}
							}
						}
					}
				}
				else
				{
					methodstack_push(obj, 2,
						MM_Icon_ErrorString, "Mangled DiskObj"
					);
				}
			}
			#ifndef BUILD_ICONLIB
			else
			{
				/*
				 * PNG icon
				 */
				#if USE_PNGICONS
				D(ICONIO, bug("checking if it's a PNGicon..\n"));
				if (pngio_sigvalid((UBYTE *)&diskobj))
				{
					D(ICONIO,bug("PNGicon detected\n"));

					set_filetype(obj, filename, filetype);

					if (flags & ICF_ANCILLARY)
					{
						APTR ctx;

						if ( (ctx = pngio_create(filename, FALSE)) )
						{
							methodstack_push(obj, 4,
								MM_Icon_AddAncillary, MV_Icon_Ancillary_PNGicon_Ctx, NULL, ctx
							);
						}
						/* XXX: hm, this is quite serious */
					}

					retval = pngicon_read(fh, obj, flags & ICF_END, flags & ICF_GETIMAGE);

					if (!retval)
					{
						STRPTR realfile;

						D(ICONIO,bug("PNGicon for %s had no image.\n", filename));

						#if 1
						// Slow but you get icon right away
						if ((realfile = name_build_noinfo(filename)) )
						{
							retval = deficonpool_apply_default_icon(obj, icontype, realfile, (APTR)DEFICON_MIMETYPE_RECOGNIZE);
							name_delete(realfile);
						}
						#else
						// Fast but you get icon delayed. MA_Icon_IsDefault should not be set... (breaks infowinclass.c!)
						if ((retval = deficonpool_apply_default_icon(obj, icontype, filename, NULL)))
						{
							methodstack_push(obj, 3, MUIM_Set, MA_Icon_IsDefault, TRUE );
						}
						#endif
					}

					if (!(flags & ICF_POSITION))
					{
						/* Position should not be used, clear Icon_HasPos */
						methodstack_push(obj, 3, MUIM_Set, MA_Icon_HasPos, FALSE);
					}
				}
				#endif
				#if USE_PNGICONS || USE_DTICONS
				else
				{
				#endif
				#if USE_DTICONS
				//{
					D(ICONIO,bug("try DTicon..\n"));
					set_filetype(obj, filename, filetype);

					retval = datatypeicon_read(filename, obj, flags & ICF_END, get_screen());
				//}
				#endif /* USE_DTICONS */
			
				#if USE_PNGICONS || USE_DTICONS
				}
				#endif

				#if USE_SVGICONS

				PDB(("Testing for svg icon %s\n", filename));
				if(!retval && svg_signature(filename))
				{
					PDB(("svg icon detected\n"));
					D(ICONIO,bug("svg icon detected\n"));

					set_filetype(obj, filename, filetype);

					/* Vector icons in SVG format */
					retval = (ULONG)svgicon_read(filename, obj, flags & ICF_END, 0);

					if (retval)
						methodstack_push(obj, 4,
							MM_Icon_AddAncillary, MV_Icon_Ancillary_SVGDoc, NULL, retval
						);
				}
				#endif
				
				if (!retval)
				{
					D(ICONIO,bug("not an icon\n"));
					methodstack_push(obj, 2,
						MM_Icon_ErrorString, "Not an icon"
					);
				}
			}
			#endif
		}
		else
		{
			D(ICONIO,bug("filesystem error: read failed\n"));
			methodstack_push(obj, 2,
				MM_Icon_ErrorString, "Filesystem error: read failed"
			);
	    }
		file_close(fh);
	}
	else
	{
		TEXT buf[64];
		snprintf(buf, sizeof(buf), "Filesystem error: couldn't open file (%ld)", IoErr()); /* XXX: locale + string size detect */

		/* we need to use methodstack_push_sync() there.. */
		methodstack_push_sync(obj, 2,
			MM_Icon_ErrorString, buf
		);
	}

	if (!retval)
	{
		#ifndef BUILD_ICONLIB
		if (flags & ICF_DEFICON)
		{
			deficonpool_apply_default_icon(obj, icontype, filename, NULL);
		}
		#endif
		#ifndef BUILD_ICONLIB
		if (flags & ICF_END)
		#endif
		{
			methodstack_push_sync(obj, 1,
				MM_Icon_End
			);
		}
		#ifndef BUILD_ICONLIB
		else
		{
			methodstack_check(FALSE);
		}
		#endif

	}

	UnLock(l);

	D(ICONIO, bug("returning %ld..\n", retval));
	//icon_checkmem();
	return (retval);
}


#ifndef BUILD_ICONLIB
STATIC VOID iconio_push_pngdata(APTR obj, APTR ctx)
{
	ULONG type;
	ULONG val, has_drawer_data;

	D(ICONIO,bug("context loaded\n"));

	methodstack_push(obj, 3, OM_GET, MA_Icon_Type, &type );
	methodstack_push(obj, 3, OM_GET, MA_Icon_HasPos, &val);
	methodstack_push_sync(obj, 3, OM_GET, MA_Icon_HasDrawerData, &has_drawer_data);

	/* add the tags */
	D(ICONIO,bug("taglist created\n"));

	/*
	 * Positionning
	 */
	if (val)
	{
		ULONG x, y;

		methodstack_push(obj, 3, OM_GET, MA_Icon_X, &x);
		methodstack_push_sync(obj, 3, OM_GET, MA_Icon_Y, &y);

		pngio_tag_add(ctx, PNGICON_LocationX, &x);
		pngio_tag_add(ctx, PNGICON_LocationY, &y);
	}

	/*
	 * Stacksize
	 */
	if (type == MV_Icon_Type_Tool || type == MV_Icon_Type_Project)
	{
		methodstack_push_sync(obj, 3, OM_GET, MA_Icon_StackSize, &val);
		pngio_tag_add(ctx, PNGICON_StackSize, &val);
	}

	/*
	 * DrawerData
	 */
	if (has_drawer_data)
	{
		ULONG top, left, width, height, view, sort;

		methodstack_push(obj, 3, OM_GET, MA_Icon_WindowTop, &top);
		methodstack_push(obj, 3, OM_GET, MA_Icon_WindowLeft, &left);
		methodstack_push(obj, 3, OM_GET, MA_Icon_WindowWidth, &width);
		methodstack_push(obj, 3, OM_GET, MA_Icon_WindowHeight, &height);
		methodstack_push(obj, 3, OM_GET, MA_Icon_ViewMode, &view);
		methodstack_push_sync(obj, 3, OM_GET, MA_Icon_SortMode, &sort);

		pngio_tag_add(ctx, PNGICON_DrawerTop, &top);
		pngio_tag_add(ctx, PNGICON_DrawerLeft, &left);
		pngio_tag_add(ctx, PNGICON_DrawerWidth, &width);
		pngio_tag_add(ctx, PNGICON_DrawerHeight, &height);
		pngio_tag_add(ctx, PNGICON_DrawerViewMode, &view);
		pngio_tag_add(ctx, PNGICON_DrawerSortMode, &sort);
	}

	/*
	 * DefaultTool
	 */
	if (type == MV_Icon_Type_Project)
	{
		methodstack_push_sync(obj, 3, OM_GET, MA_Icon_DefaultTool, &val);
		pngio_tag_add(ctx, PNGICON_DefaultTool, (APTR)val);
	}
	else
	{
		pngio_tag_delete(ctx, PNGICON_DefaultTool);
	}

	/*
	 * ToolTypes
	 */
	if (type == MV_Icon_Type_Project || type == MV_Icon_Type_Tool || type == MV_Icon_Type_Drawer)
	{
		struct ttnode *tn;

		pngio_tag_delete(ctx, PNGICON_ToolType);
		methodstack_push_sync(obj, 3, OM_GET, MA_Icon_ToolTypeList, &val);

		ITERATELIST(tn, val)
		{
			D(ICONIO,bug("adding tooltype <%s>\n", tn->tt));
			pngio_tag_add(ctx, PNGICON_ToolType, tn->tt);
		}
	}
}
#endif

//#if USE_SVGICONS || USE_ICONLIB_SVG
#if 1
#ifdef BUILD_ICONLIB
BOOL SetSVGIconContents(APTR obj, XMLNode *morphosicon, struct FreeList *fl)
#else
BOOL SetSVGIconContents(APTR obj, XMLNode *morphosicon)
#endif
{
	ULONG type;
	ULONG val, has_drawer_data;
	XMLNode *icondata = NULL;
	char strbuf[64];

	D(ICONIO,bug("context loaded\n"));

	if (!XMLNode_set_tag(morphosicon, C2SX("ambient:icondata")) ||
	    XMLNode_set_attribute(morphosicon, C2SX("xmlns"), "http://www.morphos-team.net/morphos-icon-metadata/v1") == -1 ||
	    !XMLNode_set_type(morphosicon, XTAG_FATHER))
	{
		goto outofmem;
	}

	methodstack_push(obj, 3, OM_GET, MA_Icon_Type, &type );
	methodstack_push(obj, 3, OM_GET, MA_Icon_HasPos, &val);
	methodstack_push_sync(obj, 3, OM_GET, MA_Icon_HasDrawerData, &has_drawer_data);

	/* add the xml nodes */
	D(ICONIO,bug("xml nodes created\n"));

	/*
	 * Positionning
	 */
	if (val)
	{
		ULONG x, y;

		methodstack_push(obj, 3, OM_GET, MA_Icon_X, &x);
		methodstack_push_sync(obj, 3, OM_GET, MA_Icon_Y, &y);

		icondata = XMLNode_alloc();

		if (!XMLNode_set_tag(icondata, C2SX("position")) ||
		    !XMLNode_set_type(icondata, XTAG_FATHER))
		{
			goto outofmem;
		}

		snprintf(strbuf, sizeof(strbuf), "%ld", x);
		if (XMLNode_set_attribute(icondata, C2SX("x"), strbuf) == -1)
		{
			goto outofmem;
		}
		snprintf(strbuf, sizeof(strbuf), "%ld", y);
		if (XMLNode_set_attribute(icondata, C2SX("y"), strbuf) == -1 ||
		   !XMLNode_add_child(morphosicon, icondata))
		{
			goto outofmem;
		}

		icondata = NULL;
	}

	/*
	 * Stacksize
	 *
	 * The check for MV_Icon_FileType_None will add "stacksize" to
	 * all types of icons when snapshot/unsnapshot. I don't care
	 * about this side effect much. - Piru
	 */
	if (type == MV_Icon_Type_Tool || type == MV_Icon_Type_Project || type == MV_Icon_FileType_None)
	{
		methodstack_push_sync(obj, 3, OM_GET, MA_Icon_StackSize, &val);

		icondata = XMLNode_alloc();

		snprintf(strbuf, sizeof(strbuf), "%ld", val);
		if (!XMLNode_set_tag(icondata, C2SX("stack")) ||
		    !XMLNode_set_type(icondata, XTAG_FATHER) ||
		    XMLNode_set_attribute(icondata, C2SX("size"), strbuf) == -1 ||
		    !XMLNode_add_child(morphosicon, icondata))
		{
			goto outofmem;
		}

		icondata = NULL;
	}

	/*
	 * DrawerData
	 */
	if (has_drawer_data)
	{
		ULONG top, left, width, height, view, sort;

		methodstack_push(obj, 3, OM_GET, MA_Icon_WindowTop, &top);
		methodstack_push(obj, 3, OM_GET, MA_Icon_WindowLeft, &left);
		methodstack_push(obj, 3, OM_GET, MA_Icon_WindowWidth, &width);
		methodstack_push(obj, 3, OM_GET, MA_Icon_WindowHeight, &height);
		methodstack_push(obj, 3, OM_GET, MA_Icon_ViewMode, &view);
		methodstack_push_sync(obj, 3, OM_GET, MA_Icon_SortMode, &sort);

		icondata = XMLNode_alloc();

		if (!XMLNode_set_tag(icondata, C2SX("drawer")) ||
		    !XMLNode_set_type(icondata, XTAG_FATHER))
		{
			goto outofmem;
		}

		snprintf(strbuf, sizeof(strbuf), "%ld", top);
		if (XMLNode_set_attribute(icondata, C2SX("top"), strbuf) == -1)
		{
			goto outofmem;
		}
		snprintf(strbuf, sizeof(strbuf), "%ld", left);
		if (XMLNode_set_attribute(icondata, C2SX("left"), strbuf) == -1)
		{
			goto outofmem;
		}
		snprintf(strbuf, sizeof(strbuf), "%ld", width);
		if (XMLNode_set_attribute(icondata, C2SX("width"), strbuf) == -1)
		{
			goto outofmem;
		}
		snprintf(strbuf, sizeof(strbuf), "%ld", height);
		if (XMLNode_set_attribute(icondata, C2SX("height"), strbuf) == -1)
		{
			goto outofmem;
		}
		snprintf(strbuf, sizeof(strbuf), "%ld", view);
		if (XMLNode_set_attribute(icondata, C2SX("viewmode"), strbuf) == -1)
		{
			goto outofmem;
		}
		snprintf(strbuf, sizeof(strbuf), "%ld", sort);
		if (XMLNode_set_attribute(icondata, C2SX("sortmode"), strbuf) == -1 ||
		    !XMLNode_add_child(morphosicon, icondata))
		{
			goto outofmem;
		}

		icondata = NULL;
	}

	/*
	 * DefaultTool
   *
   * NOTE: read_svgtags() explicitly sets type to MV_Icon_Type_Project if
   * "defaulttool" is found. Hence the check for MV_Icon_FileType_None is
   * kind of redundant.
	 */
	if (type == MV_Icon_Type_Project || type == MV_Icon_FileType_None)
	{
		val = 0;
		methodstack_push_sync(obj, 3, OM_GET, MA_Icon_DefaultTool, &val);

		if (val)
		{
			icondata = XMLNode_alloc();
			if (!XMLNode_set_tag(icondata, C2SX("defaulttool")) ||
			    !XMLNode_set_type(icondata, XTAG_FATHER) ||
			    XMLNode_set_attribute(icondata, C2SX("path"), (char*)val) == -1 ||
			    !XMLNode_add_child(morphosicon, icondata))
			{
				goto outofmem;
			}

			icondata = NULL;
		}
	}

	/*
	 * ToolTypes
	 */
	if (type == MV_Icon_Type_Project || type == MV_Icon_Type_Tool || type == MV_Icon_Type_Drawer || type == MV_Icon_FileType_None)
	{
		struct ttnode *tn;

		methodstack_push_sync(obj, 3, OM_GET, MA_Icon_ToolTypeList, &val);

		ITERATELIST(tn, val)
		{
			D(ICONIO,bug("adding tooltype <%s>\n", tn->tt));

			icondata = XMLNode_alloc();
			if (!XMLNode_set_tag(icondata, C2SX("tooltype")) ||
			    !XMLNode_set_type(icondata, XTAG_FATHER) ||
			    XMLNode_set_attribute(icondata, C2SX("name"), (char *)tn->tt) == -1 ||
			    !XMLNode_add_child(morphosicon, icondata))
			{
				goto outofmem;
			}
			icondata = NULL;
		}
	}

	/*
	 * Success
	 */
	return TRUE;

outofmem:
	if (icondata)
	{
		XMLNode_free(icondata);
	}

	return FALSE;
}
#endif

/*
 * Writes an Icon out of an IconObject. Is designed to
 * be called from another task.
 * Warning: writing an old icon works, writing a PNG icon require an
 * already existing source (overwrite only). XXX: fix that..
 *
 * - filename: filename of the file, with .info at the end. If it's not supplied, it's taken from 'obj'
 * - obj: object to get methods from (iconclass)
 */
#ifndef BUILD_ICONLIB
ULONG tr_icon_write(APTR obj, CONST_STRPTR filename)
{
	ULONG retval = FALSE;
	ULONG imagetype;

	THREAD;
	CHECKOBJECT(obj);

	D(ICONIO,bug("about to write icon.. obj: %p\n", obj));

	if (!filename)
	{
		D(ICONIO,bug("no filename, getting it from iconobj..\n"));
		methodstack_push_sync(obj, 3,
			OM_GET,
			MA_Icon_PathInfo, &filename
		);
	}

	ASSERT(filename);

	methodstack_push_sync(obj, 3,
		OM_GET,
		MA_Icon_ImageType, &imagetype
	);

	D(ICONIO, bug("writing icon <%s>\n", filename));

	#if USE_PNGICONS
	if (imagetype == MV_Icon_ImageType_PNGicon || imagetype == MV_Icon_ImageType_DTicon)
	{
		APTR ctx = NULL;
		ULONG has_ctx;
		APTR fh = NULL;

		D(ICONIO, bug("PNGicon\n"));

		/*
		 * Check for an ancillary context first
		 * and use that if so.
		 */
		methodstack_push_sync(obj, 4,
			MM_Icon_GetAncillary, MV_Icon_Ancillary_PNGicon_Ctx, &ctx, NULL
		);

		if (ctx)
		{
			has_ctx = TRUE;
		}
		else
		{
			has_ctx = FALSE;
		}

		if (has_ctx || (ctx = pngio_create(filename, TRUE)))
		{
			#if 1 && USE_DTICONS
			/* XXX: support re-encoding of png icons in case original icon was lost */
			//if (imagetype == MV_Icon_ImageType_DTicon)
			if (!pngio_has_image(ctx))
			{
				APTR bm;

				methodstack_push_sync(obj, 3, OM_GET, MA_Icon_ImageNormal, &bm);
#error "NOTE: pngio_add_bitmap with index 0 is currently broken"
				pngio_add_bitmap(ctx, bm, 0);
			}
			#endif

			iconio_push_pngdata(obj, ctx);

			if(_conf(icon_dualpng))
			{
				/* now save second image for dualpng format (hack++, we could get rid of duplicate text chunks i guess) */
				APTR bm;
				methodstack_push_sync(obj, 3, OM_GET, MA_Icon_ImageSelected, &bm);
							
				if(bm)
				{
					pngio_add_bitmap(ctx, bm, 1);
				}
			}

			/*
			 * Save the chunk now..
			 */
				if ( (fh = file_open(filename, MODE_NEWFILE)) )
				{
					if (pngio_save(filename, ctx, fh))
					{
						retval = TRUE;
						D(ICONIO,bug("done!\n"));
					}

		   	 	file_close(fh);
		   	}

			if (!has_ctx)
			{
				pngio_delete(ctx);
			}
		}
		else
		{
			smartreq_info(SRT_DOSERROR, MV_Notification_Error, "Icon %s went away", filename);
			D(ICONIO,bug("icon went away\n"));
		}
	}
	else
	#endif
	#if USE_SVGICONS
	if (imagetype == MV_Icon_ImageType_SVGicon)
	{
		XMLDoc *svgdoc = NULL;
		
		methodstack_push_sync(obj, 4,
			MM_Icon_GetAncillary, MV_Icon_Ancillary_SVGDoc, &svgdoc, NULL
		);
		
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
						D(ICONIO,bug("found metadata node\n"));
						morphosicon = getnode_xp(metadata, "ambient:icondata", FALSE);
						if(morphosicon)
						{
							int nodenr = getnodenum(metadata, morphosicon);
							XMLNode_remove_child(metadata, nodenr, true);
						}
						else
						{
							D(ICONIO,bug("did not find a morphos icon node\n"));
						}
					}
					else
					{
						D(ICONIO,bug("did not find a metadata node, creating one\n"));
						metadata = XMLNode_alloc();
						if (!XMLNode_set_tag(metadata, C2SX("metadata")) ||
						    !XMLNode_set_type(metadata, XTAG_FATHER) ||
						    !XMLNode_insert_child(svgnode, XINSERT_TOP, metadata))
						{
							XMLNode_free(metadata);
							metadata = NULL;
						}
					}

					D(ICONIO,bug("creating a new morphos icon node\n"));

					morphosicon = XMLNode_alloc();

					if (SetSVGIconContents(obj, morphosicon) &&
					    XMLNode_add_child(metadata, morphosicon))
					{
						retval = save_xmldoc(svgdoc, filename);
					}
					else
					{
						XMLNode_free(morphosicon);
					}
				}
				else{
					D(ICONIO,bug("this does not appear to be an SVG file\n"));
				}
			}
			else{
				D(ICONIO,bug("Could not get the the root node for the document\n"));
			}
		}
	}
	else
	#endif
	{
		APTR fh;
		D(ICONIO, bug("old icon writing\n"));
		if ( (fh = file_open(filename, MODE_NEWFILE)) )
		{
			struct DiskObject diskobj;
			ULONG val;
			ULONG img1size, img2size;
			struct MinList *ni1tt, *ni2tt, *tl;
			ULONG ttcount = 0;

			D(ICONIO, bug("file opened for writing\n"));

			diskobj.do_Magic = WB_DISKMAGIC;
			diskobj.do_Version = WB_DISKVERSION;

			/* XXX: be more careful with the type ? */
			methodstack_push_sync(obj, 3,
				OM_GET,
				MA_Icon_Type, &val
			);

			diskobj.do_Type = (UBYTE)val;

			methodstack_push(obj, 3,
				OM_GET,
				MA_Icon_StackSize, (ULONG)&diskobj.do_StackSize
			);

			methodstack_push_sync(obj, 3,
				OM_GET,
				MA_Icon_HasPos, &val
			);

			if (val)
			{
				methodstack_push(obj, 3,
					OM_GET,
					MA_Icon_X, (ULONG)&diskobj.do_CurrentX
				);

				methodstack_push(obj, 3,
					OM_GET,
					MA_Icon_Y, (ULONG)&diskobj.do_CurrentY
				);
			}
			else
			{
				diskobj.do_CurrentX = NO_ICON_POSITION;
				diskobj.do_CurrentY = NO_ICON_POSITION;
			}


			methodstack_push_sync(obj, 3,
				OM_GET,
				MA_Icon_HasDrawerData, &val
			);
			diskobj.do_DrawerData = (struct DrawerData *)val;

			methodstack_push(obj, 4,
				MM_Icon_GetAncillary, MV_Icon_Ancillary_Gadget, &diskobj.do_Gadget, NULL
			);

			methodstack_push(obj, 4,
				MM_Icon_GetAncillary, MV_Icon_Ancillary_ImageNormal, &diskobj.do_Gadget.GadgetRender, &img1size
			);

			methodstack_push(obj, 4,
				MM_Icon_GetAncillary, MV_Icon_Ancillary_ImageSelected, &diskobj.do_Gadget.SelectRender, &img2size
			);

			methodstack_push(obj, 3,
				OM_GET, MA_Icon_DefaultTool, &diskobj.do_DefaultTool
			);

			diskobj.do_ToolWindow = NULL; /* this is rubbish.. strip it */

			/* tooltypes */
			methodstack_push(obj, 4,
				MM_Icon_GetAncillary, MV_Icon_Ancillary_NewiconNormalTT, &ni1tt, NULL
			);

			methodstack_push(obj, 4,
				MM_Icon_GetAncillary, MV_Icon_Ancillary_NewiconSelectedTT, &ni2tt, NULL
			);

			methodstack_push_sync(obj, 3,
				OM_GET, MA_Icon_ToolTypeList, &tl
			);

			if (diskobj.do_DefaultTool && !diskobj.do_DefaultTool[0])
			{
				diskobj.do_DefaultTool = NULL;
			}

			if (diskobj.do_DrawerData == (APTR)MV_Icon_HasDrawerData_LessOld)
			{
				diskobj.do_Gadget.UserData = (APTR)1;
			}
			else
			{
				diskobj.do_Gadget.UserData = NULL;
			}

			if (!ISLISTEMPTY(ni1tt) || !ISLISTEMPTY(ni2tt) || !ISLISTEMPTY(tl))
			{
				struct iconchunk *ic;
				struct ttnode *tn;

				/* count them */
				ITERATELIST(tn, tl)
				{
					D(ICONIO,bug("normal tooltype added\n"));
					ttcount++;
				}

				if (!ISLISTEMPTY(ni1tt))
				{
					ttcount += 2; /* we have to add the 2 newicon lines */
				}

				ITERATELIST(ic, ni1tt)
				{
					D(ICONIO,bug("ni1 tooltype added\n"));
					ttcount++;
				}

				ITERATELIST(ic, ni2tt)
				{
					D(ICONIO,bug("ni2 tooltype added\n"));
					ttcount++;
				}
			}
			diskobj.do_ToolTypes = ttcount ? (STRPTR *)1 : (STRPTR *)0;

			D(ICONIO, bug("resulting diskobj: do_Type: %ld\n", (LONG)diskobj.do_Type));
			D(ICONIO, bug("do_DefaultTool: %p\n", diskobj.do_DefaultTool));
			D(ICONIO, bug("do_ToolTypes: %p (ttcount: %ld)\n", diskobj.do_ToolTypes, ttcount));
			D(ICONIO, bug("do_CurrentX/Y: %ld, %ld\n", diskobj.do_CurrentX, diskobj.do_CurrentY));
			D(ICONIO, bug("do_ToolWindow: %p\n", diskobj.do_ToolWindow));
			D(ICONIO, bug("do_StackSize: %ld\n", diskobj.do_StackSize));

			/*
			 * Start writing the stuff.
			 */
			if (file_write(fh, &diskobj, sizeof(diskobj)))
			{
				struct DrawerData dd;

				if (diskobj.do_DrawerData)
				{
					D(ICONIO,bug("diskobj written to disk\n"));

					methodstack_push_sync(obj, 3,
						OM_GET,
						MA_Icon_WindowTop, &val
					);
					dd.dd_NewWindow.TopEdge = (WORD)val;

					methodstack_push_sync(obj, 3,
						OM_GET,
						MA_Icon_WindowLeft, &val
					);
					dd.dd_NewWindow.LeftEdge = (WORD)val;

					methodstack_push_sync(obj, 3,
						OM_GET,
						MA_Icon_WindowWidth, &val
					);
					dd.dd_NewWindow.Width = (WORD)val;

					methodstack_push_sync(obj, 3,
						OM_GET,
						MA_Icon_WindowHeight &val
					);
					dd.dd_NewWindow.Height = (WORD)val;

					/*
					 * Default values.
					 */
					dd.dd_NewWindow.DetailPen = 255;
					dd.dd_NewWindow.BlockPen = 255;
					dd.dd_NewWindow.IDCMPFlags = 0;
					dd.dd_NewWindow.Flags = 0x240027f,
					dd.dd_NewWindow.FirstGadget = NULL;
					dd.dd_NewWindow.CheckMark = NULL;
					dd.dd_NewWindow.Title = NULL;
					dd.dd_NewWindow.Screen = NULL;
					dd.dd_NewWindow.BitMap = NULL;
					dd.dd_NewWindow.MinWidth = 90;
					dd.dd_NewWindow.MinHeight = 40;
					dd.dd_NewWindow.MaxWidth = 65535;
					dd.dd_NewWindow.MaxHeight = 65535;
					dd.dd_NewWindow.Type = 1;

					methodstack_push(obj, 3,
						OM_GET,
						MA_Icon_OffsetX, &dd.dd_CurrentX
					);

					methodstack_push_sync(obj, 3,
						OM_GET,
						MA_Icon_OffsetY, &dd.dd_CurrentY
					);

					if (diskobj.do_DrawerData == (APTR)MV_Icon_HasDrawerData_LessOld)
					{
						ULONG mode;

						methodstack_push(obj, 3,
							OM_GET,
							MA_Icon_ViewMode, &mode
						);

						switch (mode)
						{
							case MV_Icon_ViewMode_Lister:
								dd.dd_Flags = 1;
								dd.dd_ViewModes = DDVM_BYNAME;
								break;

							case MV_Icon_ViewMode_Icon:
								dd.dd_Flags = 1;
								break;

							case MV_Icon_ViewMode_IconAll:
								dd.dd_Flags = 3;
								break;

							#ifdef DEBUG
							default:
								PDB(("no handling for viewmode %ld\n", mode));
								break;
							#endif
						}

						if (mode != MV_Icon_ViewMode_Lister)
						{
							methodstack_push(obj, 3,
								OM_GET,
								MA_Icon_SortMode, &mode
							);

							switch (mode)
							{
								case MV_Icon_SortMode_Name:
									dd.dd_ViewModes = DDVM_BYICON;
									break;

								case MV_Icon_SortMode_Date:
									dd.dd_ViewModes = DDVM_BYDATE;
									break;

								case MV_Icon_SortMode_Size:
									dd.dd_ViewModes = DDVM_BYSIZE;
									break;

								case MV_Icon_SortMode_Type:
									dd.dd_ViewModes = DDVM_BYTYPE;
									break;
							}
						}
					}

					/* writing first part of DrawerData */
					if (file_write(fh, &dd, sizeof(struct OldDrawerData)))
					{
						D(ICONIO,bug("OldDrawerData written to disk..\n"));
					}
					else
					{
						errormsg(ERR_WRITEERROR);
						goto write_error;
					}
				}

				/* writing image */
				if (file_write(fh, diskobj.do_Gadget.GadgetRender, sizeof(struct Image)))
				{
					ULONG ttcountlong;

					if (file_write(fh, ((struct Image *)diskobj.do_Gadget.GadgetRender)->ImageData, img1size))
					{
						D(ICONIO,bug("first image written to disk.. SelectRender is %p\n", diskobj.do_Gadget.SelectRender));

						if (diskobj.do_Gadget.SelectRender)
						{
							D(ICONIO,bug("writing 2nd image to disk (size: 0x%lx)..\n", img2size));
							if (file_write(fh, diskobj.do_Gadget.SelectRender, sizeof(struct Image))) /* XXX */
							{
								if (!file_write(fh, ((struct Image *)diskobj.do_Gadget.SelectRender)->ImageData, img2size))
								{
									errormsg(ERR_WRITEERROR);
									goto write_error;
								}
							}
							else
							{
								errormsg(ERR_WRITEERROR);
								goto write_error;
							}
						}

						/* writing defaulttool */
						if (diskobj.do_DefaultTool)
						{
							D(ICONIO,bug("writing defaulttool to disk..\n"));
							if (!icon_write_infostring(fh, diskobj.do_DefaultTool, 0)) goto write_error;
						}

						/* writing tooltypes */
						if (diskobj.do_ToolTypes)
						{
							/*
							 * Well, it seems that icons have a non-existent tooltype at
							 * the end. If we don't add it, DOpus only displays
							 * one image and "OS" 3.5 doesn't display anything
							 * at all :)
							 */
							ttcount++;

							ttcountlong = ttcount << 2;

							D(ICONIO,bug("%ld tooltypes to go..\n", ttcount));

							if (file_write(fh, &ttcountlong, sizeof(ttcountlong)))
							{
								struct iconchunk *ic;
								struct ttnode *tn;

								ITERATELIST(tn, tl)
								{
									D(ICONIO,bug("writing tooltype string <%s>..\n", tn->tt));
									if (!icon_write_infostring(fh, tn->tt, 0)) goto write_error;
								}

								if (!ISLISTEMPTY(ni1tt))
								{
									/* add the newicon headers */
									if (!icon_write_infostring(fh, " ", 2)) goto write_error;
									if (!icon_write_infostring(fh, newiconstart, sizeof(newiconstart))) goto write_error;

									ITERATELIST(ic, ni1tt)
									{
										D(ICONIO,bug("writing IM1 string <%s>..\n", ic->data));
										if (!icon_write_infostring(fh, (STRPTR)ic->data, ic->size)) goto write_error;
									}

									ITERATELIST(ic, ni2tt)
									{
										D(ICONIO,bug("writing IM2 string <%s>..\n", ic->data));
										if (!icon_write_infostring(fh, (STRPTR)ic->data, ic->size)) goto write_error;
									}
								}

								D(ICONIO,bug("finished writing tooltypes\n"));

							}
							else
							{
								errormsg(ERR_WRITEERROR);
								goto write_error;
							}
						}

						if (diskobj.do_DrawerData == (APTR)MV_Icon_HasDrawerData_LessOld)
						{
							/* end of drawerdata */
							if (file_write(fh, &dd.dd_Flags, 6))
							{
								D(ICONIO,bug("finished writing lessold drawerdata\n"));
							}
							else
							{
								errormsg(ERR_WRITEERROR);
								goto write_error;
							}
						}

						retval = TRUE;

						methodstack_push_sync(obj, 4,
							MM_Icon_GetAncillary, MV_Icon_Ancillary_Glowicon_Chunk, &tl, NULL
						);

						if (!ISLISTEMPTY(tl))
						{
							struct iconchunk *ic;

							ITERATELIST(ic, tl)
							{
								D(ICONIO,bug("we have glowicons there at %p, size %ld..\n", ic->data, ic->size));
								if (!file_write(fh, ic->data, ic->size))
								{
									errormsg(ERR_WRITEERROR);
									goto write_error;
								}
							}
						}
					}
					else
					{
						errormsg(ERR_WRITEERROR);
						goto write_error;
					}
				}
				else
				{
					errormsg(ERR_WRITEERROR);
					goto write_error;
				}
			}
			write_error:
			file_close(fh);
		}
	}

	/*
	 * Since we overwrite we just tell we
	 * create a new file without bothering with
	 * the old one.
	 */
	if (retval)
	{
		notify_action(filename, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create);
	}
	return (retval);
}

ULONG tr_icon_write_dummy(APTR obj, CONST_STRPTR filename)
{
	ULONG retval = FALSE;
	APTR ctx = NULL;
	APTR fh = NULL;

	THREAD;
	CHECKOBJECT(obj);
	ASSERT(filename);

	D(ICONIO,bug("about to write icon.. obj: %p\n", obj));

	if ((ctx = pngio_create(filename, PNGIO_NO_IMAGE)))
	{
		iconio_push_pngdata(obj, ctx);

		/*
		 * Save the chunk now..
		 */
		if (pngio_finalize_iconless(ctx) && (fh = file_open(filename, MODE_NEWFILE)))
		{
			pngio_save(filename, ctx, fh);
  		 	file_close(fh);
			notify_action(filename, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create);

			retval = TRUE;
		}

		pngio_delete(ctx);
	}

	return (retval);
}


/*
 * Sets the file modes and writes the icon.
 */
static ULONG tr_set_file_and_write_icon(APTR obj, STRPTR filename, ULONG flags, CONST_STRPTR comment, ULONG mode)
{
	ULONG retval = FALSE;

	/*
	 * We must *WRITE* the icon first
	 * so that the notify doesn't bother us.
	 * BUT we cannot write any icon if none is available! (chaozer)
	 * XXX: really remove that notify
	 */

		switch (mode)
		{
			case TV_Icon_Write_Mode_DefIcon:
				retval = tr_icon_write_dummy(obj, filename);
				break;

			case TV_Icon_Write_Mode_FullIcon:
				retval = tr_icon_write(obj, filename);
				break;

			default:
				retval = TRUE;
				break;
		}

		{
			APTR truncation = name_truncateinfo( filename );

			if (comment) /* XXX: huh.. we should remove +w protection first.. */
			{
				if (SetComment( filename, comment))
				{
					notify_action( filename, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Comment, comment);
				}
				else
				{
					retval = FALSE;
				}
			}

			if (SetProtection( filename, flags)) /* XXX: do a notify_action() once the flags are clear.. */
			{
				notify_action( filename, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Flags, flags);
			}
			else
			{
				retval = FALSE;
			}

			name_restoreinfo( filename, truncation );
		}

	return (retval);
}


ULONG tr_icon_read(APTR obj, STRPTR path, ULONG all)
{
	APTR o;
	ULONG isroot;
	ULONG viewid;

	THREAD;
	CHECKOBJECT(obj);

	methodstack_push_sync(obj, 3, OM_GET, MA_Iconview_IsRoot, &isroot);
	methodstack_push_sync(obj, 3, OM_GET, MA_Viewgroup_ID, &viewid);

	if ( (o = (APTR)methodstack_push_sync(app, 3, MM_Application_CreateIcon, isroot, viewid)) )
	{
		STRPTR iconname;

		if ( (iconname = name_build_info(path)) )
		{
			APTR truncation = NULL;
			APTR path_device = NULL;

			icon_read(iconname, o,
				ICONTAG_Deficon, all,
			TAG_DONE); /* XXX */

			if ( stricmp( FilePart(path), "disk.info" ) )
			{
				truncation = name_truncateinfo( path );
			}
			else
			{
				path_device = name_build_noinfo( path );
			}

			if ( path_device != NULL )
			{
				methodstack_push(o, 3, MUIM_Set, MA_Icon_FileType, MV_Icon_FileType_Device);
			}

			methodstack_push(o, 3, MUIM_Set, MA_Icon_Path, path_device ? path_device : path );
			methodstack_push(o, 3, MUIM_Set, MA_Icon_PathInfo, iconname );
			methodstack_push(o, 3, MUIM_Set, MA_Icon_IsShortcut, isroot );
			methodstack_push(obj, 3, MM_Iconview_AddIcon, o, TRUE);
			methodstack_push_sync(obj, 1, MM_Iconview_DoLayout); /* sync */

			name_restoreinfo( path, truncation );
			name_delete(iconname);
			if ( path_device != NULL )
				name_delete(path_device);
		}
		return (TRUE);
	}
	return (FALSE);
}

ULONG tr_icon_update(APTR obj, CONST_STRPTR path, CONST_STRPTR comment, ULONG flags, ULONG mode)
{
	ULONG rc = FALSE;

	THREAD;
	CHECKOBJECT(obj);

	if (!path)
	{
		methodstack_push_sync(obj, 3, OM_GET, MA_Icon_PathInfo, &path);
	}

	ASSERT(path);

	if(path)
	{
		methodstack_push_sync(app, 3, MM_Application_EnableDOSNotify, path, FALSE);

		if (mode == TV_Icon_Write_Mode_DefIcon)
		{
			BPTR lock = Lock(path, ACCESS_READ);

			if (lock)
			{
				UnLock(lock);
			}
			else
			{
				if (smartreq_request_sync(NULL, SRT_NONE, GSI(MSG_YES_NO_REQ), MV_Notification_Help, GSI(MSG_INFOWIN_CREATE_ICON_REQ), NULL) == 0)
					mode = TV_Icon_Write_Mode_NoIcon;
			}
		}

		if (comment || flags)
		{
			rc = tr_set_file_and_write_icon(obj, (STRPTR)path, flags, comment, mode);
		}
		else
		{
			switch (mode)
			{
				case TV_Icon_Write_Mode_DefIcon:
					rc = tr_icon_write_dummy(obj, path);
					break;

				case TV_Icon_Write_Mode_FullIcon:
					rc = tr_icon_write(obj, path);
					break;
			}
		}

		methodstack_push_sync(app, 3, MM_Application_EnableDOSNotify, path, TRUE);
	}

	return rc;
}

#endif
